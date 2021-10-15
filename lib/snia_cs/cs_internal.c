/**
 * @file cs_internal.c
 * @author Alex Hsu (ahsu@petaio.com)
 * @brief 
 * @version 0.1
 * @date 2021-10-06
 * 
 * @copyright Copyright (c) 2021
 * 
 */

/**********************************************************/
/*                                                        */  
/* INCLUDE FILE DECLARATIONS                              */
/*                                                        */
/**********************************************************/
#include "spdk/cs.h"
#include "spdk/cs_vendor.h"
#include "cs_msg.h"
#include "cs_common.h"
#include "spdk/queue.h"
#include "spdk/thread.h"
#include "spdk/log.h"

/**********************************************************/
/*                                                        */  
/* NAMING CONSTANT DECLARATIONS                           */  
/* {Constants define for LOCAL reference ONLY.}           */  
/*                                                        */  
/**********************************************************/
#define MAX_CSX_CFG_STR_LEN		64
#define MAX_FULL_SCRIPT_CMD_LEN		64
#define BASE_NAME_SEPARATOR		'-'

#define SCAN_CSX_SCRIPT_CMD		"scan_csx.sh"
/**********************************************************/
/*                                                        */
/* MACRO FUNCTION DECLARATIONS                            */
/* {MACRO functions define for LOCAL reference ONLY.}     */
/*                                                        */
/**********************************************************/

/**********************************************************/
/*                                                        */
/* DATA TYPE DECLARATIONS                                 */
/* {DATA TYPE define for LOCAL reference ONLY.}           */
/*                                                        */  
/**********************************************************/
TAILQ_HEAD(cs_csx_list, _cs_csx);

typedef struct _cs_mgmt {
	struct spdk_thread 	*app_thread;
	char			*script_dir_path;
	struct cs_csx_list	csx_list;
} cs_mgmt_t;

typedef struct _poll_evnet {
	volatile int		issued_msg_cnt;
	volatile int		completed_msg_cnt;
} poll_evnet_t;

/**********************************************************/
/*                                                        */
/* LOCAL SUBPROGRAM DECLARATIONS                          */
/* {Function routines define for LOCAL reference ONLY.}   */
/*                                                        */
/**********************************************************/
static
void cs_msg_attach_csx_done_cb(void *cb_ctx);

/**********************************************************/
/*                                                        */
/* STATIC VARIABLE DECLARATIONS                           */
/* {STATIC VARIABLES defines for LOCAL reference ONLY.}   */
/*                                                        */
/**********************************************************/
static cs_mgmt_t g_cs_mgmt = {
	.csx_list = TAILQ_HEAD_INITIALIZER(g_cs_mgmt.csx_list)
};

static poll_evnet_t g_poll_event;
/**********************************************************/
/*                                                        */
/* EXPORTED SUBPROGRAM BODIES                             */
/* {C code body of each EXPORTED function routine.}       */
/*                                                        */
/**********************************************************/

//---------------------------------------------
// vendor cs APIs
//
void cs_init(cs_init_param_t *param)
{
	g_cs_mgmt.app_thread = spdk_get_thread();
	g_cs_mgmt.script_dir_path = param->script_dir_path;
}

CS_STATUS cs_scan_csxes(void)
{
	FILE *fp = NULL;
	char buffer[MAX_CSX_CFG_STR_LEN + 1] = {0};
	char script_cmd[MAX_FULL_SCRIPT_CMD_LEN + 1] = {0};
	CS_STATUS sts = CS_SUCCESS;

	memset(&g_poll_event, 0, sizeof(g_poll_event));

	// run script to scan CSx
	sprintf(script_cmd, "%s/%s", g_cs_mgmt.script_dir_path, SCAN_CSX_SCRIPT_CMD);
	fp = popen(script_cmd, "r");
	if (fp == NULL) {
		SPDK_ERRLOG("popen fail\n");
		return CS_OUT_OF_RESOURCES;
	}

	//----------------------------------------
	// 1. get cfg string and do CSxes probe
	// 2. for NVMe CSx with PCIe, the string format is "base_name-DDDD:BB:DD.FF"
	//
	while (fgets(buffer, MAX_CSX_CFG_STR_LEN, fp) != NULL) {
		int maxlen, len;
		bool b_clean_up = true;

		SPDK_NOTICELOG("csx_addr:%s\n", buffer);

		//------------------------------------
		// init msg_ctx
		//

		// allocate msg_ctx
		cs_msg_attach_nvme_csx_ctx_t *msg_ctx = calloc(1, sizeof(*msg_ctx));
		if (msg_ctx == NULL) {
			SPDK_ERRLOG("calloc fail\n");
			goto cleanup;
		}

		// allocate CSx
		msg_ctx->csx = calloc(1, sizeof(*msg_ctx->csx));
		if (msg_ctx->csx == NULL) {
			SPDK_ERRLOG("calloc fail\n");
			goto cleanup;
		}

		// process for CSx cfg string
		char *base_name = &buffer[0];
		char *trtype = strchr(base_name, BASE_NAME_SEPARATOR);
		if (trtype == NULL) {
			SPDK_ERRLOG("parse csx_addr fail\n");
			goto cleanup;
		} else {
			*trtype = '\0';
			trtype++;
		}

		char *traddr = strchr(trtype, BASE_NAME_SEPARATOR);
		if (traddr == NULL) {
			SPDK_ERRLOG("parse csx_addr fail\n");
			goto cleanup;
		} else {
			*traddr = '\0';
			traddr++;
		}

		// init base name
		maxlen = sizeof(msg_ctx->base_name);
		len = strnlen(base_name, maxlen);
		if (len == maxlen) {
			SPDK_ERRLOG("parse csx_addr fail\n");
			goto cleanup;
		} else {
			memcpy(msg_ctx->base_name, base_name, (len + 1));	
		}
		

		// init trid
		maxlen = sizeof(msg_ctx->trid.traddr);
		len = strnlen(traddr, maxlen);
		if (len == maxlen) {
			SPDK_ERRLOG("parse csx_addr fail\n");
			goto cleanup;
		} else {
			memcpy(msg_ctx->trid.traddr, traddr, (len + 1));	
		}

		if (spdk_nvme_transport_id_populate_trstring(&msg_ctx->trid, trtype) < 0) {
			SPDK_ERRLOG("populate trstring fail\n");
			goto cleanup;
		}

		if (spdk_nvme_transport_id_parse_trtype(&msg_ctx->trid.trtype, trtype) < 0) {
			SPDK_ERRLOG("parse trtype fail(%s,%ld)\n", trtype, strlen(trtype));
			goto cleanup;
		}

		// init ctrlr options
		spdk_nvme_ctrlr_get_default_ctrlr_opts(&msg_ctx->opts, sizeof(msg_ctx->opts));

		// init others...
		msg_ctx->max_bdev_cnt = MAX_BDEVS_PER_CSX;
		msg_ctx->prchk_flags = 0;

		// init cb
		msg_ctx->cpl_cb_fn = cs_msg_attach_csx_done_cb;
		msg_ctx->cb_ctx = msg_ctx;

		// send msg cmd
		if (spdk_thread_send_msg(g_cs_mgmt.app_thread, cs_msg_attach_nvme_csx, msg_ctx) < 0) {
			SPDK_ERRLOG("send cs_msg fail\n");
			goto cleanup;
		}

		// no error if we reach here, set clean up flag to false
		g_poll_event.issued_msg_cnt++;
		b_clean_up = false;

	cleanup:
		if (b_clean_up == true) {
			if (msg_ctx->csx != NULL) {
				free(msg_ctx->csx);
			}

			if (msg_ctx != NULL) {
				free(msg_ctx);
			}

			sts = CS_NOT_ENOUGH_MEMORY;
			break;
		}
	}

	pclose(fp);
	while(g_poll_event.issued_msg_cnt != g_poll_event.completed_msg_cnt);

	return sts;
}

//---------------------------------------------
// snia cs APIs
//
CS_STATUS csGetCSxFromPath(char *Path, unsigned int *Length, char *DevName)
{
	return CS_SUCCESS;
}

/**********************************************************/
/*                                                        */
/* LOCAL SUBPROGRAM BODIES                                */
/* {C code body of each LOCAL function routines.}         */
/*                                                        */
/**********************************************************/
static
void cs_msg_attach_csx_done_cb(void *cb_ctx) {

	cs_msg_attach_nvme_csx_ctx_t *msg_ctx = (cs_msg_attach_nvme_csx_ctx_t *)cb_ctx;
	assert(msg_ctx->rc != CS_MSG_RC_BUSY);
	if (msg_ctx->rc == CS_MSG_RC_SUCESS) {
		TAILQ_INSERT_TAIL(&g_cs_mgmt.csx_list, msg_ctx->csx, next);
		//SPDK_NOTICELOG("CSx:%s\n", msg_ctx->csx->bdev_names[(msg_ctx->csx->bdev_count - 1)]);
		SPDK_NOTICELOG("CSx:%s\n", msg_ctx->csx->bdev_names[msg_ctx->csx->bdev_count]);
	} else {
		SPDK_ERRLOG("attach csx fail\n");
	}

	free(msg_ctx);
	g_poll_event.completed_msg_cnt++;
}
/* End of CS_INTERNAL_C */

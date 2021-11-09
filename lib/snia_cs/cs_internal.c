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
#include "spdk/nvme_csd_spec.h"
#include "spdk/util.h"

/**********************************************************/
/*                                                        */  
/* NAMING CONSTANT DECLARATIONS                           */  
/* {Constants define for LOCAL reference ONLY.}           */  
/*                                                        */  
/**********************************************************/
#define MAX_CSX_CFG_STR_LEN		64
#define MAX_FULL_SCRIPT_CMD_LEN		64
#define BASE_NAME_SEPARATOR		'-'

#define CMB_TEST_PATTERN		0x5aa5aa55

#define SCAN_CSX_SCRIPT_CMD		"scan_csx.sh"

#define VA_ALIGN_BYTES			64
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
static
void cs_msg_get_cse_cnt_done_cb(void *cb_ctx);
static
void cs_msg_get_cse_list_done_cb(void *cb_ctx);
static
void cs_msg_map_cmb_done_cb(void *cb_ctx);
static
void cs_msg_exec_program_done_cb(void *cb_ctx);
static
void cs_msg_exec_program_done_cb(void *cb_ctx);
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
		msg_ctx->cpl_cb_ctx = msg_ctx;

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
	while (g_poll_event.issued_msg_cnt != g_poll_event.completed_msg_cnt);

	return sts;
}

CS_STATUS cs_get_cse_list(void)
{	
	CS_STATUS sts = CS_SUCCESS;

	memset(&g_poll_event, 0, sizeof(g_poll_event));

	cs_csx_t *csx;
	TAILQ_FOREACH(csx, &g_cs_mgmt.csx_list, next) {
		bool b_clean_up = true;

		// allocate msg_ctx
		cs_msg_get_cse_list_ctx_t *msg_ctx = calloc(1, sizeof(*msg_ctx));
		if (msg_ctx == NULL) {
			SPDK_ERRLOG("calloc fail\n");
			goto cleanup;
		}

		// init msg_ctx
		msg_ctx->cpl_cb_fn = cs_msg_get_cse_cnt_done_cb;
		msg_ctx->cpl_cb_ctx = msg_ctx;

		msg_ctx->csx = csx;
		csx->cse_list_size = sizeof(csx->p_cse_list->number_of_compute_engine);
		csx->cse_list_size = SPDK_ALIGN_CEIL(csx->cse_list_size, sizeof(uint32_t));

		csx->p_cse_list = spdk_zmalloc(csx->cse_list_size, 64, NULL,
					SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);
		if (csx->p_cse_list == NULL) {
			SPDK_ERRLOG("spdk_zmalloc fail\n");
			goto cleanup;
		}


		// send msg cmd
		if (spdk_thread_send_msg(g_cs_mgmt.app_thread, cs_msg_get_cse_list, msg_ctx) < 0) {
			SPDK_ERRLOG("send cs_msg fail\n");
			goto cleanup;
		}

		g_poll_event.issued_msg_cnt++;
		b_clean_up = false;

	cleanup:
		if (b_clean_up == true) {
			if (csx->p_cse_list != NULL) {
				spdk_free(csx->p_cse_list);
				csx->cse_list_size = 0;
			}

			if (msg_ctx != NULL) {
				free(msg_ctx);
			}

			sts = CS_NOT_ENOUGH_MEMORY;
			break;
		}					
	}

	while (g_poll_event.issued_msg_cnt != g_poll_event.completed_msg_cnt);
	return sts;	
}

CS_STATUS cs_map_cmb(void)
{
	CS_STATUS sts = CS_SUCCESS;

	memset(&g_poll_event, 0, sizeof(g_poll_event));

	cs_csx_t *csx;
	TAILQ_FOREACH(csx, &g_cs_mgmt.csx_list, next) {
		bool b_clean_up = true;

		// allocate msg_ctx
		cs_msg_map_cmb_ctx_t *msg_ctx = calloc(1, sizeof(*msg_ctx));
		if (msg_ctx == NULL) {
			SPDK_ERRLOG("calloc fail\n");
			goto cleanup;
		}

		// init msg_ctx
		msg_ctx->cpl_cb_fn = cs_msg_map_cmb_done_cb;
		msg_ctx->cpl_cb_ctx = msg_ctx;

		msg_ctx->csx = csx;

		// send msg cmd
		if (spdk_thread_send_msg(g_cs_mgmt.app_thread, cs_msg_map_cmb, msg_ctx) < 0) {
			SPDK_ERRLOG("send cs_msg fail\n");
			goto cleanup;
		}

		g_poll_event.issued_msg_cnt++;
		b_clean_up = false;

	cleanup:
		if (b_clean_up == true) {
			if (msg_ctx != NULL) {
				free(msg_ctx);
			}

			sts = CS_NOT_ENOUGH_MEMORY;
			break;
		}					
	}

	while (g_poll_event.issued_msg_cnt != g_poll_event.completed_msg_cnt);
	return sts;		
}

void* cs_get_in_cmb_buf(void)
{
	cs_csx_t *csx = TAILQ_FIRST(&g_cs_mgmt.csx_list);
	void *align_in_addr;

	align_in_addr = (void *)SPDK_ALIGN_CEIL((uint64_t)csx->cmb_va, VA_ALIGN_BYTES);
	return align_in_addr;
}

void* cs_get_out_cmb_buf(void)
{
	cs_csx_t *csx = TAILQ_FIRST(&g_cs_mgmt.csx_list);
	void *align_out_addr;

	align_out_addr = (void *)SPDK_ALIGN_CEIL(((uint64_t)csx->cmb_va + (csx->cmb_size / 2)), VA_ALIGN_BYTES);
	return align_out_addr;
}

CS_STATUS cs_exec_program(void *in_buf, void *out_buf)
{
	bool b_clean_up = true;
	CS_STATUS sts = CS_SUCCESS;
	cs_csx_t *csx = TAILQ_FIRST(&g_cs_mgmt.csx_list);

	memset(&g_poll_event, 0, sizeof(g_poll_event));

	// allocate msg_ctx
	cs_msg_exec_program_ctx_t *msg_ctx = calloc(1, sizeof(*msg_ctx));
	if (msg_ctx == NULL) {
		SPDK_ERRLOG("calloc fail\n");
		goto cleanup;
	}

	// init msg_ctx
	msg_ctx->cpl_cb_fn = cs_msg_exec_program_done_cb;
	msg_ctx->cpl_cb_ctx = msg_ctx;

	msg_ctx->csx = csx;

	msg_ctx->in_buf_addr = in_buf;
	msg_ctx->out_buf_addr = out_buf;

	// send msg cmd
	if (spdk_thread_send_msg(g_cs_mgmt.app_thread, cs_msg_exec_program, msg_ctx) < 0) {
		SPDK_ERRLOG("send cs_msg fail\n");
		goto cleanup;
	}

	g_poll_event.issued_msg_cnt++;
	b_clean_up = false;

	cleanup:
	if (b_clean_up == true) {
		if (msg_ctx != NULL) {
			free(msg_ctx);
		}

		sts = CS_NOT_ENOUGH_MEMORY;
	}

	while (g_poll_event.issued_msg_cnt != g_poll_event.completed_msg_cnt);
	return sts;	
}

//---------------------------------------------
// snia cs APIs
//
CS_STATUS csGetCSxFromPath(char *Path, unsigned int *Length, char *DevName)
{
	cs_csx_t *csx;

	if (DevName == NULL) {
		*Length = 0;
		TAILQ_FOREACH(csx, &g_cs_mgmt.csx_list, next) {
			// add extra one for comma seperator or end of string
			*Length += (strlen(csx->bdev_names[csx->bdev_count]) + 1);
		}
	} else {
		int cur_len = 0;
		int max_len = *Length;

		TAILQ_FOREACH(csx, &g_cs_mgmt.csx_list, next) {
			// add extra one for comma seperator or end of string
			int name_size = (strlen(csx->bdev_names[csx->bdev_count]) + 1);

			if ((cur_len + name_size) > max_len) {
				return CS_NOT_ENOUGH_MEMORY;
			}

			// has name entry in front, replace end of string to comma seperator
			if (cur_len != 0) {
				DevName[(cur_len - 1)] = ';';
			}
			// auto add end of string
			sprintf(&DevName[cur_len], "%s", csx->bdev_names[csx->bdev_count]);
			cur_len += name_size;
		}
	}

	return CS_SUCCESS;
}

CS_STATUS csQueryCSEList(char *FunctionName, int *Length, char *Buffer)
{
	return CS_SUCCESS;
}

CS_STATUS csQueueComputeRequest(
	CsComputeRequest 	*Req,
	void 			*Context,
	csQueueCallbackFn 	CallbackFn,
	CS_EVT_HANDLE 		EventHandle,
	u32 			*CompValue)
{
	return CS_SUCCESS;
}

CS_STATUS csAllocMem(
	CS_DEV_HANDLE 	DevHandle,
	int 		Bytes, 
	unsigned int 	MemFlags,
	CS_MEM_HANDLE 	*MemHandle, 
	CS_MEM_PTR 	*VAddressPtr)
{
	return CS_SUCCESS;
}

void csHelperSetComputeArg(CsComputeArg *ArgPtr, CS_COMPUTE_ARG_TYPE Type,...)
{
	va_list args;
    	va_start(args, Type);

	ArgPtr->Type = Type;
	switch (Type) {
	case CS_AFDM_TYPE:
		ArgPtr->u.DevMem.MemHandle = va_arg(args, CS_MEM_HANDLE);
		ArgPtr->u.DevMem.ByteOffset = va_arg(args, unsigned long);
		break;
	case CS_32BIT_VALUE_TYPE:
		ArgPtr->u.Value32 = va_arg(args, u32);
		break;
	case CS_64BIT_VALUE_TYPE:
		ArgPtr->u.Value64 = va_arg(args, u64);
		break;
	case CS_STREAM_TYPE:
		ArgPtr->u.StreamHandle = va_arg(args, CS_STREAM_HANDLE);
		break;
	case CS_DESCRIPTOR_TYPE:						
	default:
		break;
	}

	va_end(args);    

}
/**********************************************************/
/*                                                        */
/* LOCAL SUBPROGRAM BODIES                                */
/* {C code body of each LOCAL function routines.}         */
/*                                                        */
/**********************************************************/
static
void cs_msg_attach_csx_done_cb(void *cb_ctx) 
{
	cs_msg_attach_nvme_csx_ctx_t *msg_ctx = (cs_msg_attach_nvme_csx_ctx_t *)cb_ctx;

	if (msg_ctx->rc == CS_MSG_RC_SUCESS) {
		TAILQ_INSERT_TAIL(&g_cs_mgmt.csx_list, msg_ctx->csx, next);
		//SPDK_NOTICELOG("CSx:%s\n", msg_ctx->csx->bdev_names[(msg_ctx->csx->bdev_count - 1)]);
		msg_ctx->csx->csx_name = msg_ctx->csx->bdev_names[msg_ctx->csx->bdev_count];
		SPDK_NOTICELOG("CSx:%s\n", msg_ctx->csx->csx_name);
	} else {
		SPDK_ERRLOG("attach csx fail\n");
	}

	free(msg_ctx);
	g_poll_event.completed_msg_cnt++;
}

static
void cs_msg_get_cse_cnt_done_cb(void *cb_ctx)
{
	cs_msg_get_cse_list_ctx_t *msg_ctx = (cs_msg_get_cse_list_ctx_t *)cb_ctx;
	cs_csx_t *csx = msg_ctx->csx;
	
	csx->cse_cnt = (msg_ctx->rc == CS_MSG_RC_SUCESS) ? csx->p_cse_list->number_of_compute_engine : 0;
	// free cse list in csx first to allocate mem for fully cse list
	if (csx->p_cse_list != NULL) {
		spdk_free(csx->p_cse_list);
		csx->cse_list_size = 0;
	}

	if (msg_ctx->rc == CS_MSG_RC_SUCESS) {
		bool b_clean_up = true;

		SPDK_NOTICELOG("%s:cse_cnt=%d\n", csx->csx_name, csx->cse_cnt);
		
		// allocate msg_ctx
		cs_msg_get_cse_list_ctx_t *req_msg_ctx = calloc(1, sizeof(*req_msg_ctx));
		if (req_msg_ctx == NULL) {
			SPDK_ERRLOG("calloc fail\n");
			goto cleanup;
		}

		// init msg_ctx
		req_msg_ctx->cpl_cb_fn = cs_msg_get_cse_list_done_cb;
		req_msg_ctx->cpl_cb_ctx = req_msg_ctx;

		req_msg_ctx->csx = csx;
		csx->cse_list_size = sizeof(struct spdk_csd_compute_engine_list) + (csx->cse_cnt * sizeof(uint16_t));
		csx->cse_list_size = SPDK_ALIGN_CEIL(csx->cse_list_size, sizeof(uint32_t));

		csx->p_cse_list = spdk_zmalloc(csx->cse_list_size, 64, NULL,
					SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);
		if (csx->p_cse_list == NULL) {
			SPDK_ERRLOG("spdk_zmalloc fail\n");
			goto cleanup;
		}

		// send msg cmd
		if (spdk_thread_send_msg(g_cs_mgmt.app_thread, cs_msg_get_cse_list, req_msg_ctx) < 0) {
			SPDK_ERRLOG("send cs_msg fail\n");
			goto cleanup;
		}

		b_clean_up = false;
	cleanup:
		if (b_clean_up == true) {
			if (csx->p_cse_list != NULL) {
				spdk_free(csx->p_cse_list);
				csx->cse_list_size = 0;
			}

			if (req_msg_ctx != NULL) {
				free(req_msg_ctx);
			}

			g_poll_event.completed_msg_cnt++;
		}
	} else {
		g_poll_event.completed_msg_cnt++;
	}

	free(msg_ctx);
}

static
void cs_msg_get_cse_list_done_cb(void *cb_ctx)
{
	cs_msg_get_cse_list_ctx_t *msg_ctx = (cs_msg_get_cse_list_ctx_t *)cb_ctx;
	uint32_t i;
	cs_csx_t *csx = msg_ctx->csx;

	for (i = 0; i < csx->cse_cnt; i++) {
		SPDK_NOTICELOG("CSE_%d id:%d\n", i, csx->p_cse_list->compute_engine_identifier[i]);
	}

	g_poll_event.completed_msg_cnt++;
	free(msg_ctx);
}

static
void cs_msg_map_cmb_done_cb(void *cb_ctx)
{
	cs_msg_map_cmb_ctx_t *msg_ctx = (cs_msg_map_cmb_ctx_t *)cb_ctx;
	cs_csx_t *csx = msg_ctx->csx;

	SPDK_NOTICELOG("%s:cmb_va=%p,cmb_pa=%08lX,size=%ld,cmb_base_pa=%08lX\n", 
		csx->csx_name, csx->cmb_va, 
		(csx->cmb_va) ? spdk_vtophys(csx->cmb_va, NULL) : 0,
		csx->cmb_size, csx->cmb_base_pa);

	uint32_t *cmb_buf = (uint32_t *)csx->cmb_va;
	*cmb_buf = CMB_TEST_PATTERN;
	if (*cmb_buf != CMB_TEST_PATTERN) {
		SPDK_ERRLOG("cmb test fail\n");
	}

	g_poll_event.completed_msg_cnt++;
	free(msg_ctx);	
}

static
void cs_msg_exec_program_done_cb(void *cb_ctx)
{
	cs_msg_exec_program_ctx_t *msg_ctx = (cs_msg_exec_program_ctx_t *)cb_ctx;	

	g_poll_event.completed_msg_cnt++;
	free(msg_ctx);
}
/* End of CS_INTERNAL_C */

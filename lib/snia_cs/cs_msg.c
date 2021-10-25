/**
 * @file cs_msg.c
 * @author Alex Hsu (ahsu@petaio.com)
 * @brief 
 * @version 0.1
 * @date 2021-10-05
 * 
 * @copyright Copyright (c) 2021
 * 
 */

/**********************************************************/
/*                                                        */  
/* INCLUDE FILE DECLARATIONS                              */
/*                                                        */
/**********************************************************/
#include "cs_msg.h"
#include "spdk/nvme.h"
#include "spdk/nvme_csd_spec.h"
#include "../module/bdev/nvme/bdev_nvme.h"

/**********************************************************/
/*                                                        */  
/* NAMING CONSTANT DECLARATIONS                           */  
/* {Constants define for LOCAL reference ONLY.}           */  
/*                                                        */  
/**********************************************************/

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

/**********************************************************/
/*                                                        */
/* LOCAL SUBPROGRAM DECLARATIONS                          */
/* {Function routines define for LOCAL reference ONLY.}   */
/*                                                        */
/**********************************************************/
static 
void bdev_nvme_create_done_cb(void *cb_ctx, size_t bdev_count, int rc);
static
void get_cse_list_complete_cb(struct spdk_bdev_io *bdev_io, bool success, void *cb_arg);
static
void exec_program_complete_cb(struct spdk_bdev_io *bdev_io, bool success, void *cb_arg);
//-------------------------
static void
csx_bdev_event_cb(enum spdk_bdev_event_type type, struct spdk_bdev *bdev, void *event_ctx);
static 
int init_get_log_page_cmd(struct spdk_nvme_cmd *cmd, uint8_t log_page_id, uint32_t payload_size);
/**********************************************************/
/*                                                        */
/* STATIC VARIABLE DECLARATIONS                           */
/* {STATIC VARIABLES defines for LOCAL reference ONLY.}   */
/*                                                        */
/**********************************************************/

/**********************************************************/
/*                                                        */
/* EXPORTED SUBPROGRAM BODIES                             */
/* {C code body of each EXPORTED function routine.}       */
/*                                                        */
/**********************************************************/

void cs_msg_attach_nvme_csx(void *ctx)
{
	cs_msg_attach_nvme_csx_ctx_t *msg_ctx = (cs_msg_attach_nvme_csx_ctx_t *)ctx;
	struct spdk_nvme_host_id hostid = {};

	// init default value
	msg_ctx->rc = CS_MSG_RC_SUCESS;

	int rc = bdev_nvme_create(&msg_ctx->trid, &hostid, 
				  msg_ctx->base_name, msg_ctx->csx->bdev_names, msg_ctx->max_bdev_cnt,
				  NULL, msg_ctx->prchk_flags,
				  bdev_nvme_create_done_cb, msg_ctx, 
				  &msg_ctx->opts);
	if (rc < 0) {
		SPDK_ERRLOG("bdev_nvme_create fail\n");

		msg_ctx->rc = CS_MSG_RC_GEN_FAIL;
		if (msg_ctx->cpl_cb_fn) {
			msg_ctx->cpl_cb_fn(msg_ctx);
		}		
	}
}

void cs_msg_get_cse_list(void *ctx)
{
	cs_msg_get_cse_list_ctx_t *msg_ctx = (cs_msg_get_cse_list_ctx_t *)ctx;
	cs_csx_t *csx = msg_ctx->csx;
	int rc = 0;

	// init default value
	msg_ctx->rc = CS_MSG_RC_SUCESS;
	msg_ctx->bdev_desc = NULL;

	// get bdev desc
	rc = spdk_bdev_open_ext(csx->csx_name, true, 
				csx_bdev_event_cb, NULL,
				&msg_ctx->bdev_desc);
	if (rc) {
		SPDK_ERRLOG("Could not open bdev: %s\n", csx->csx_name);
		msg_ctx->rc = CS_MSG_RC_GEN_FAIL;
		goto out;
	}

	/* Open I/O channel */
	struct spdk_io_channel *bdev_io_channel = spdk_bdev_get_io_channel(msg_ctx->bdev_desc);
	if (bdev_io_channel == NULL) {
		SPDK_ERRLOG("Could not create bdev I/O channel!!\n");
		msg_ctx->rc = CS_MSG_RC_GEN_FAIL;
		goto out;
	}

	/* get log page command */
	struct spdk_nvme_cmd cmd = {};
	init_get_log_page_cmd(&cmd, SPDK_CSD_LOG_COMPUTE_ENGINE_LIST, csx->cse_list_size);
	rc = spdk_bdev_nvme_admin_passthru(msg_ctx->bdev_desc, bdev_io_channel, &cmd, 
					   csx->p_cse_list, csx->cse_list_size,
					   get_cse_list_complete_cb, msg_ctx);
	if (rc) {		
		goto out;
	}

	return;

out:
	if (msg_ctx->bdev_desc) {
		spdk_bdev_close(msg_ctx->bdev_desc);
	}

	if (msg_ctx->cpl_cb_fn) {
		msg_ctx->cpl_cb_fn(msg_ctx);
	}
}

void cs_msg_map_cmb(void *ctx)
{
	cs_msg_map_cmb_ctx_t *msg_ctx = (cs_msg_map_cmb_ctx_t *)ctx;
	cs_csx_t *csx = msg_ctx->csx;
	int rc = 0;

	// init default value
	msg_ctx->rc = CS_MSG_RC_SUCESS;
	msg_ctx->bdev_desc = NULL;

	// get bdev desc
	rc = spdk_bdev_open_ext(csx->csx_name, true, 
				csx_bdev_event_cb, NULL,
				&msg_ctx->bdev_desc);
	if (rc) {
		SPDK_ERRLOG("Could not open bdev: %s\n", csx->csx_name);
		msg_ctx->rc = CS_MSG_RC_GEN_FAIL;
		goto out;
	}

	rc = spdk_bdev_nvme_map_cmb(msg_ctx->bdev_desc, &msg_ctx->csx->cmb_va, &msg_ctx->csx->cmb_size);
	if (rc) {
		SPDK_ERRLOG("Could not map cmb: %s\n", csx->csx_name);
		msg_ctx->rc = CS_MSG_RC_GEN_FAIL;
		goto out;
	}

out:
	if (msg_ctx->bdev_desc) {
		spdk_bdev_close(msg_ctx->bdev_desc);
	}

	if (msg_ctx->cpl_cb_fn) {
		msg_ctx->cpl_cb_fn(msg_ctx);
	}	
}

void cs_msg_exec_program(void *ctx)
{
	cs_msg_exec_program_ctx_t *msg_ctx = (cs_msg_exec_program_ctx_t *)ctx;
	cs_csx_t *csx = msg_ctx->csx;
	int rc = 0;

	// init default value
	msg_ctx->rc = CS_MSG_RC_SUCESS;
	msg_ctx->bdev_desc = NULL;

	// get bdev desc
	rc = spdk_bdev_open_ext(csx->csx_name, true, 
				csx_bdev_event_cb, NULL,
				&msg_ctx->bdev_desc);
	if (rc) {
		SPDK_ERRLOG("Could not open bdev: %s\n", csx->csx_name);
		msg_ctx->rc = CS_MSG_RC_GEN_FAIL;
		goto out;
	}

	/* Open I/O channel */
	struct spdk_io_channel *bdev_io_channel = spdk_bdev_get_io_channel(msg_ctx->bdev_desc);
	if (bdev_io_channel == NULL) {
		SPDK_ERRLOG("Could not create bdev I/O channel!!\n");
		msg_ctx->rc = CS_MSG_RC_GEN_FAIL;
		goto out;
	}

	/* get log page command */
	struct spdk_nvme_cmd cmd = {};

	rc = spdk_bdev_nvme_io_passthru(msg_ctx->bdev_desc, bdev_io_channel,
					&cmd, NULL, 0,
					exec_program_complete_cb, msg_ctx);

out:
	if (msg_ctx->bdev_desc) {
		spdk_bdev_close(msg_ctx->bdev_desc);
	}

	if (msg_ctx->cpl_cb_fn) {
		msg_ctx->cpl_cb_fn(msg_ctx);
	}	
}
/**********************************************************/
/*                                                        */
/* LOCAL SUBPROGRAM BODIES                                */
/* {C code body of each LOCAL function routines.}         */
/*                                                        */
/**********************************************************/
static 
void bdev_nvme_create_done_cb(void *cb_ctx, size_t bdev_count, int rc) 
{
	cs_msg_attach_nvme_csx_ctx_t *msg_ctx = (cs_msg_attach_nvme_csx_ctx_t *)cb_ctx;

	if (rc == 0) {
		msg_ctx->csx->bdev_count = bdev_count;
	} else {
		msg_ctx->rc = CS_MSG_RC_GEN_FAIL;
	}

	if (msg_ctx->cpl_cb_fn) {
		msg_ctx->cpl_cb_fn(msg_ctx);
	}
}

static
void get_cse_list_complete_cb(struct spdk_bdev_io *bdev_io, bool success, void *cb_arg) 
{
	cs_msg_get_cse_list_ctx_t *msg_ctx = (cs_msg_get_cse_list_ctx_t *)cb_arg;

	if (!success) {
		msg_ctx->rc = CS_MSG_RC_GEN_FAIL;
	}

	spdk_bdev_free_io(bdev_io);
	spdk_bdev_close(msg_ctx->bdev_desc);

	if (msg_ctx->cpl_cb_fn) {
		msg_ctx->cpl_cb_fn(msg_ctx);
	}
}

static
void exec_program_complete_cb(struct spdk_bdev_io *bdev_io, bool success, void *cb_arg) 
{
	cs_msg_get_cse_list_ctx_t *msg_ctx = (cs_msg_get_cse_list_ctx_t *)cb_arg;

	if (!success) {
		msg_ctx->rc = CS_MSG_RC_GEN_FAIL;
	}

	spdk_bdev_free_io(bdev_io);
	spdk_bdev_close(msg_ctx->bdev_desc);

	if (msg_ctx->cpl_cb_fn) {
		msg_ctx->cpl_cb_fn(msg_ctx);
	}
}

//-------------------------
static void
csx_bdev_event_cb(enum spdk_bdev_event_type type, struct spdk_bdev *bdev, void *event_ctx)
{
	SPDK_NOTICELOG("Unsupported bdev event: type %d\n", type);
}

static 
int init_get_log_page_cmd(struct spdk_nvme_cmd *cmd, uint8_t log_page_id, uint32_t payload_size) {
	uint32_t numd, numdl, numdu;

	if (payload_size == 0) {
		return -EINVAL;
	}

	memset(cmd, 0, sizeof(*cmd));

	numd = spdk_nvme_bytes_to_numd(payload_size);
	numdl = numd & 0xFFFFu;
	numdu = (numd >> 16) & 0xFFFFu;

	cmd->opc = SPDK_NVME_OPC_GET_LOG_PAGE;
	cmd->cdw10_bits.get_log_page.numdl = numdl;
	cmd->cdw10_bits.get_log_page.lid = log_page_id;

	cmd->cdw11_bits.get_log_page.numdu = numdu;

	return 0;
}
/* End of CS_MSG_C */

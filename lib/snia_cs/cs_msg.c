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

	assert(msg_ctx->rc == CS_MSG_RC_BUSY);

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

	msg_ctx->rc = (rc < 0) ? CS_MSG_RC_GEN_FAIL : CS_MSG_RC_SUCESS;
	msg_ctx->csx->bdev_count = bdev_count;

	if (msg_ctx->cpl_cb_fn) {
		msg_ctx->cpl_cb_fn(msg_ctx);
	}
}

/* End of CS_MSG_C */

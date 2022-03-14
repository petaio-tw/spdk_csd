/**
 * @file cs_csx.c
 * @author Alex Hsu (ahsu@petaio.com)
 * @brief 
 * @version 0.1
 * @date 2022-02-08
 * 
 * @copyright Copyright (c) 2022
 * 
 */

/**********************************************************/
/*                                                        */  
/* INCLUDE FILE DECLARATIONS                              */
/*                                                        */
/**********************************************************/
#include "spdk/log.h"
#include "spdk/nvme_spec.h"
#include "spdk/thread.h"
#include "cs_mgr.h"
#include "cs_csx.h"
#include "cs_cse.h"

/**********************************************************/
/*                                                        */  
/* NAMING CONSTANT DECLARATIONS                           */  
/* {Constants define for LOCAL reference ONLY.}           */  
/*                                                        */  
/**********************************************************/
#define CS_UNKNOWN_FUNC_NAME	"UNKNOWN"

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
void cs_csx_construct(void *ctx);
static 
void cs_csx_removed_event(enum spdk_bdev_event_type type, struct spdk_bdev *bdev, void *event_ctx);
static 
int cs_csx_reset(void *arg);
//----------------------------------
// spdk bdev complete callback
//----------------------------------
static
void cs_csx_get_cse_cnt_cplt(struct spdk_bdev_io *bdev_io, bool success, void *arg);
static 
void cs_csx_get_cse_list_cplt(struct spdk_bdev_io *bdev_io, bool success, void *arg);
static 
void cs_csx_get_cse_info_cplt(struct spdk_bdev_io *bdev_io, bool success, void *arg);
static 
void cs_csx_get_func_cnt_cplt(struct spdk_bdev_io *bdev_io, bool success, void *arg);
static
void cs_csx_get_func_list_cplt(struct spdk_bdev_io *bdev_io, bool success, void *arg);
static
void cs_csx_prog_act_mgmt_cplt(struct spdk_bdev_io *bdev_io, bool success, void *arg);
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
int cs_csx_init(struct cs_csx *csx, struct spdk_bdev *bdev, cs_csx_init_cplt_cb cb, void *cb_arg)
{
	struct spdk_bdev_alias *tmp;
	int rc = 0;

	printf("csx:%s\n", bdev->name);
	TAILQ_FOREACH(tmp, spdk_bdev_get_aliases(bdev), tailq) {
		printf("csx alias:%s\n", tmp->alias.name);
	}

	//------------------------------
	// init csx 
	memset(csx, 0, sizeof(struct cs_csx));
	
	csx->state = CSX_STATE_INIT_START;
	TAILQ_INIT(&csx->csx_handle_list);
	csx->bdev = bdev;
	csx->init_cplt_cb = cb;
	csx->init_cplt_cb_arg = cb_arg;
	csx->thread = spdk_thread_create(bdev->name, NULL);
	if (!csx->thread) {
		SPDK_ERRLOG("spdk_thread_create fail:csx:%s\n", spdk_bdev_get_name(bdev));
		goto _err_out;
	}

	rc = spdk_thread_send_msg(csx->thread, cs_csx_construct, csx);
	if (rc != 0) {
		SPDK_ERRLOG("spdk_thread_send_msg fail:csx:%s\n", spdk_bdev_get_name(bdev));
		goto _err_out;
	}

	return 0;

_err_out:
	csx->state = CSX_STATE_INIT_FAIL;
	rc = (rc != 0) ? rc : -1;
	csx->init_cplt_cb(csx->init_cplt_cb_arg, rc);
	return rc;
}

//-------------------------------------------------
// csx job APIs 
// 1. need be called from spdk_thread_send_msg
//-------------------------------------------------
void cs_csx_job_prog_act_mgmt(void *ctx)
{
	struct cs_csx_job_parm *job_parm = (struct cs_csx_job_parm *)ctx;
	struct cs_cse_act_func *cse_act_func = job_parm->u.prog_act_mgmt.cse_act_func;
	struct cs_func *func = cse_act_func->func;
	struct cs_cse *cse = cse_act_func->cse;
	struct cs_csx *csx = cse->csx;
	bool activate = job_parm->u.prog_act_mgmt.activate;

	int rc = spdk_bdev_cs_prog_act_mgmt(csx->bdev_desc, csx->ch, func->pid, cse->ceid,
					    activate, cs_csx_prog_act_mgmt_cplt, job_parm);
	if (rc != 0) {
		if (rc != -ENOMEM) {
			SPDK_ERRLOG("prog act mgmt fail:cs=%s,rc=%d\n", spdk_bdev_get_name(csx->bdev), rc);
			cs_csx_prog_act_mgmt_cplt(NULL, false, job_parm);
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
void cs_csx_construct(void *ctx)
{
	struct cs_csx *csx = (struct cs_csx *)ctx;
	int rc = 0;

	rc = spdk_bdev_open_ext(spdk_bdev_get_name(csx->bdev), true, cs_csx_removed_event, csx,
				&csx->bdev_desc);
	if (rc != 0) {
		SPDK_ERRLOG("spdk_bdev_open_ext fail:cs=%s\n", spdk_bdev_get_name(csx->bdev));
		goto _err_out;
	}

	csx->ch = spdk_bdev_get_io_channel(csx->bdev_desc);
	if (!csx->ch) {
		SPDK_ERRLOG("spdk_bdev_get_io_channel fail:csx=%s\n", spdk_bdev_get_name(csx->bdev));
		goto _err_out;
	}

	csx->state = CSX_STATE_GET_CSE_CNT;
	csx->reset_poller = SPDK_POLLER_REGISTER(cs_csx_reset, csx, 0);
	if (csx->reset_poller == NULL) {
		goto _err_out;
	}

	return;

_err_out:
	csx->state = CSX_STATE_INIT_FAIL;
	rc = (rc != 0) ? rc : -1;
	csx->init_cplt_cb(csx->init_cplt_cb_arg, rc);
	return;
}

static 
void cs_csx_removed_event(enum spdk_bdev_event_type type, struct spdk_bdev *bdev, void *event_ctx)
{
	struct cs_csx *csx = (struct cs_csx *)event_ctx;

	printf("%s(%d)%s\n", __FUNCTION__, type, spdk_bdev_get_name(csx->bdev));
}

static 
int cs_csx_reset(void *arg)
{
	struct cs_csx *csx = (struct cs_csx *)arg;
	int rc = 0;

	switch (csx->state)
	{
	case CSX_STATE_GET_CSE_CNT: {
		size_t payload_size = SPDK_NVME_GET_CE_LIST_LOG_SIZE(0);

		csx->tmp_buf = spdk_zmalloc(payload_size, SPDK_CACHE_LINE_SIZE, NULL, 
					SPDK_ENV_LCORE_ID_ANY, SPDK_MALLOC_DMA);
		if (csx->tmp_buf == NULL) {
			SPDK_ERRLOG("spdk_zmalloc fail:cs=%s\n", spdk_bdev_get_name(csx->bdev));
			cs_csx_get_cse_cnt_cplt(NULL, false, csx);
		} else {			
			rc = spdk_bdev_cs_get_log_page(csx->bdev_desc, csx->ch, 
				                       SPDK_NVME_LOG_COMPUTE_ENGINE_LIST, 0,
						       csx->tmp_buf, payload_size,
						       cs_csx_get_cse_cnt_cplt, csx);
			if (rc != 0) {
				if (rc != -ENOMEM) {
					SPDK_ERRLOG("get cse cnt fail:cs=%s,rc=%d\n", spdk_bdev_get_name(csx->bdev), rc);
					cs_csx_get_cse_cnt_cplt(NULL, false, csx);
				}		
			} else {
				csx->state = CSX_STATE_WAIT_FOR_CSE_CNT;
			}
		}
		return SPDK_POLLER_IDLE;
	}
	case CSX_STATE_GET_CSE_LIST: {
		size_t payload_size = SPDK_NVME_GET_CE_LIST_LOG_SIZE(csx->num_cse);

		csx->tmp_buf = spdk_zmalloc(payload_size, SPDK_CACHE_LINE_SIZE, NULL, 
					SPDK_ENV_LCORE_ID_ANY, SPDK_MALLOC_DMA);
		if (csx->tmp_buf == NULL) {
			SPDK_ERRLOG("spdk_zmalloc fail:cs=%s\n", spdk_bdev_get_name(csx->bdev));
			cs_csx_get_cse_list_cplt(NULL, false, csx);
		} else {
			rc = spdk_bdev_cs_get_log_page(csx->bdev_desc, csx->ch, 
						       SPDK_NVME_LOG_COMPUTE_ENGINE_LIST, 0,
						       csx->tmp_buf, payload_size,
						       cs_csx_get_cse_list_cplt, csx);
			if (rc != 0) {
				if (rc != -ENOMEM) {
					SPDK_ERRLOG("get cse list fail:cs=%s,rc=%d\n", spdk_bdev_get_name(csx->bdev), rc);
					cs_csx_get_cse_list_cplt(NULL, false, csx);
				}	
			} else {
				csx->state = CSX_STATE_WAIT_FOR_CSE_LIST;
			}
		}
		return SPDK_POLLER_IDLE;
	}
	case CSX_STATE_GET_CSE_INFO: {
		size_t payload_size = sizeof(struct spdk_nvme_ce_info_log);

		csx->tmp_buf = spdk_zmalloc(payload_size, SPDK_CACHE_LINE_SIZE, NULL, 
					SPDK_ENV_LCORE_ID_ANY, SPDK_MALLOC_DMA);
		if (csx->tmp_buf == NULL) {
			SPDK_ERRLOG("spdk_zmalloc fail:cs=%s\n", spdk_bdev_get_name(csx->bdev));
			cs_csx_get_cse_info_cplt(NULL, false, csx);
		} else {
			struct cs_cse *cse = &csx->cse_list[csx->init_cse_idx];
			rc = spdk_bdev_cs_get_log_page(csx->bdev_desc, csx->ch, 
						       SPDK_NVME_LOG_COMPUTE_ENGINE_INFORMATION, cse->ceid,
						       csx->tmp_buf, payload_size,
						       cs_csx_get_cse_info_cplt, csx);
			if (rc != 0) {
				if (rc != -ENOMEM) {
					SPDK_ERRLOG("get cse info fail:cs=%s,rc=%d\n", spdk_bdev_get_name(csx->bdev), rc);
					cs_csx_get_cse_info_cplt(NULL, false, csx);
				}	
			} else {
				csx->state = CSX_STATE_WAIT_FOR_CSE_INFO;
			}
		}
		return SPDK_POLLER_IDLE;


	}
	case CSX_STATE_GET_FUNC_CNT: {		
		size_t payload_size = SPDK_NVME_GET_PROG_INFO_LOG_SIZE(0);

		csx->tmp_buf = spdk_zmalloc(payload_size, SPDK_CACHE_LINE_SIZE, NULL, 
					SPDK_ENV_LCORE_ID_ANY, SPDK_MALLOC_DMA);
		if (csx->tmp_buf == NULL) {
			SPDK_ERRLOG("spdk_zmalloc fail:cs=%s\n", spdk_bdev_get_name(csx->bdev));
			cs_csx_get_func_cnt_cplt(NULL, false, csx);
		} else {
			rc = spdk_bdev_cs_get_log_page(csx->bdev_desc, csx->ch, 
						       SPDK_NVME_LOG_PROGRAM_INFORMATION, 0, 
						       csx->tmp_buf, payload_size,
						       cs_csx_get_func_cnt_cplt, csx);
			if (rc != 0) {
				if (rc != -ENOMEM) {
					SPDK_ERRLOG("get func cnt fail:cs=%s,rc=%d\n", spdk_bdev_get_name(csx->bdev), rc);
					cs_csx_get_func_cnt_cplt(NULL, false, csx);
				}		
			} else {
				csx->state = CSX_STATE_WAIT_FOR_FUNC_CNT;
			}
		}
		return SPDK_POLLER_IDLE;
	}
	case CSX_STATE_GET_FUNC_LIST: {
		size_t payload_size = SPDK_NVME_GET_PROG_INFO_LOG_SIZE(csx->num_func);

		csx->tmp_buf = spdk_zmalloc(payload_size, SPDK_CACHE_LINE_SIZE, NULL, 
					SPDK_ENV_LCORE_ID_ANY, SPDK_MALLOC_DMA);
		if (csx->tmp_buf == NULL) {
			SPDK_ERRLOG("spdk_zmalloc fail:cs=%s\n", spdk_bdev_get_name(csx->bdev));
			cs_csx_get_func_list_cplt(NULL, false, csx);
		} else {					
			rc = spdk_bdev_cs_get_log_page(csx->bdev_desc, csx->ch, 
						       SPDK_NVME_LOG_PROGRAM_INFORMATION, 0, 
						       csx->tmp_buf, payload_size,
						       cs_csx_get_func_list_cplt, csx);
			if (rc != 0) {
				if (rc != -ENOMEM) {
					SPDK_ERRLOG("get func list fail:cs=%s,rc=%d\n", spdk_bdev_get_name(csx->bdev), rc);
					cs_csx_get_func_list_cplt(NULL, false, csx);
				}		
			} else {
				csx->state = CSX_STATE_WAIT_FOR_FUNC_LIST;
			}
		}
		return SPDK_POLLER_IDLE;
	}
	case CSX_STATE_INIT_DONE:
	case CSX_STATE_INIT_FAIL:
		rc = (csx->state == CSX_STATE_INIT_DONE) ? 0 : -1;
		spdk_poller_unregister(&csx->reset_poller);
		csx->init_cplt_cb(csx->init_cplt_cb_arg, rc);
		return SPDK_POLLER_BUSY;
	default:
		return SPDK_POLLER_IDLE;		
	}
}

static
void cs_csx_get_cse_cnt_cplt(struct spdk_bdev_io *bdev_io, bool success, void *arg)
{
	struct cs_csx *csx = (struct cs_csx *)arg;
	struct spdk_nvme_ce_list_log *ce_list_log = (struct spdk_nvme_ce_list_log *)csx->tmp_buf;

	if ((success) && (ce_list_log->num_of_ces > 0)) {
		csx->num_cse = ce_list_log->num_of_ces;
		assert(csx->num_cse <= CS_MAX_CSE_PER_CSX);		

		csx->state = CSX_STATE_GET_CSE_LIST;
		printf("%s:num_cse=%d\n", spdk_bdev_get_name(csx->bdev), csx->num_cse);		
	} else {
		csx->state = CSX_STATE_INIT_FAIL;
	}

	if (bdev_io != NULL) {
		spdk_bdev_free_io(bdev_io);
	}

	if (csx->tmp_buf != NULL) {
		spdk_free(csx->tmp_buf);
		csx->tmp_buf = NULL;
	}
}

static 
void cs_csx_get_cse_list_cplt(struct spdk_bdev_io *bdev_io, bool success, void *arg)
{
	struct cs_csx *csx = (struct cs_csx *)arg;
	struct spdk_nvme_ce_list_log *ce_list_log = (struct spdk_nvme_ce_list_log *)csx->tmp_buf;

	if (success) {
		assert(csx->num_cse == ce_list_log->num_of_ces);

		for (int i = 0; i < csx->num_cse; i++) {
			struct cs_cse *cse = &csx->cse_list[i];		

			// init CSE
			cse->ceid = ce_list_log->ce_id[i];
			if (cse->ceid == 0) {
				printf("get invalid ceid=%d\n", cse->ceid);
				csx->state = CSX_STATE_INIT_FAIL;
				break;
			}

			sprintf(cse->name, "%s_cse%d", spdk_bdev_get_name(csx->bdev), cse->ceid);
			printf("%s\n", cse->name);
			cse->csx = csx;
			TAILQ_INIT(&cse->cse_handle_list);
			TAILQ_INIT(&cse->cse_act_func_list);		
		}		
		csx->state = CSX_STATE_GET_CSE_INFO;
	} else {
		csx->state = CSX_STATE_INIT_FAIL;
	}

	if (bdev_io != NULL) {
		spdk_bdev_free_io(bdev_io);
	}

	if (csx->tmp_buf != NULL) {
		spdk_free(csx->tmp_buf);
		csx->tmp_buf = NULL;
	}
}

static 
void cs_csx_get_cse_info_cplt(struct spdk_bdev_io *bdev_io, bool success, void *arg)
{
	struct cs_csx *csx = (struct cs_csx *)arg;
	struct cs_cse *cse = &csx->cse_list[csx->init_cse_idx];
	struct spdk_nvme_ce_info_log *ce_info_log = (struct spdk_nvme_ce_info_log *)csx->tmp_buf;

	assert(csx->init_cse_idx < csx->num_cse);
	if (success) {
		cse->max_cse_act_func = ce_info_log->characteristics.max_actived_progs;
		assert(cse->max_cse_act_func > 0);
		printf("%s:max_act_func=%d\n", cse->name, cse->max_cse_act_func);
		if (++csx->init_cse_idx < csx->num_cse) {
			csx->state = CSX_STATE_GET_CSE_INFO;
		} else {
			csx->state = CSX_STATE_GET_FUNC_CNT;
		}
	} else {
		csx->state = CSX_STATE_INIT_FAIL;
	}

	if (bdev_io != NULL) {
		spdk_bdev_free_io(bdev_io);
	}

	if (csx->tmp_buf != NULL) {
		spdk_free(csx->tmp_buf);
		csx->tmp_buf = NULL;
	}
}

static 
void cs_csx_get_func_cnt_cplt(struct spdk_bdev_io *bdev_io, bool success, void *arg)
{
	struct cs_csx *csx = (struct cs_csx *)arg;
	struct spdk_nvme_prog_info_log *prog_info_log = (struct spdk_nvme_prog_info_log *)csx->tmp_buf;

	if ((success) && (prog_info_log->num_of_records > 0)) {
		csx->num_func = prog_info_log->num_of_records;
		csx->state = CSX_STATE_GET_FUNC_LIST;
		printf("%s:num_func=%d\n", spdk_bdev_get_name(csx->bdev), csx->num_func);
	} else {
		csx->state = CSX_STATE_INIT_FAIL;
	}

	if (bdev_io != NULL) {
		spdk_bdev_free_io(bdev_io);
	}

	if (csx->tmp_buf != NULL) {
		spdk_free(csx->tmp_buf);
		csx->tmp_buf = NULL;
	}	
}

static
void cs_csx_get_func_list_cplt(struct spdk_bdev_io *bdev_io, bool success, void *arg)
{
	struct cs_csx *csx = (struct cs_csx *)arg;

	if (success) {
		int payload_size = SPDK_NVME_GET_PROG_INFO_LOG_SIZE(csx->num_func);

		csx->nvme_prog_info = (struct spdk_nvme_prog_info_log *)calloc(1, payload_size);
		memcpy((void *)csx->nvme_prog_info, (void *)csx->tmp_buf, payload_size);
		
		//---------------------------------------------
		// init cs_func objects by nvme_prog_info_log
		assert(csx->num_func == csx->nvme_prog_info->num_of_records);
		assert(csx->num_func <= CS_MAX_FUNC_PER_CSX);
		for (int func_idx = 0; func_idx < csx->num_func; func_idx++) {
			struct spdk_nvme_prog_info_data *prog = &csx->nvme_prog_info->prog[func_idx];
			struct cs_func *func = &csx->func_list[func_idx];
			
			func->puid = prog->id.puid;
			func->pid = func_idx;
			func->csx = csx;
			func->func_name = cs_mgr_get_func_name(prog->id.puid);
			if (func->func_name == NULL) {
				func->func_name = CS_UNKNOWN_FUNC_NAME;
			}
			func->is_occupied = prog->entry.prog_ety_occupied;
			func->is_downloadable = prog->entry.prog_ety_type_downloadable;
			
			assert(prog->num_of_assoc_ces > 0);
			for (int asce_idx = 0; asce_idx < prog->num_of_assoc_ces; asce_idx++) {
				struct spdk_nvme_ce_desc_data *asce = &prog->assoc_ce_desc[asce_idx];

				assert(asce->sts.activated == 0);
				// search for associated cse object
				int cse_idx;
				for (cse_idx = 0; cse_idx < csx->num_cse; cse_idx++) {
					struct cs_cse *cse = &csx->cse_list[cse_idx];

					if (cse->ceid == asce->ce_id) {
						func->assoc_cse_list[func->num_assoc_cse++] = cse;
						break;
					}
				}
				assert(cse_idx < csx->num_cse);
			}			
		}

		csx->state = CSX_STATE_INIT_DONE;
	} else {
		csx->state = CSX_STATE_INIT_FAIL;
	}

	if (bdev_io != NULL) {
		spdk_bdev_free_io(bdev_io);
	}

	if (csx->tmp_buf != NULL) {
		spdk_free(csx->tmp_buf);
		csx->tmp_buf = NULL;
	}	
}

static
void cs_csx_prog_act_mgmt_cplt(struct spdk_bdev_io *bdev_io, bool success, void *arg)
{
	struct cs_csx_job_parm *job_parm = (struct cs_csx_job_parm *)arg;	

	if (bdev_io != NULL) {
		spdk_bdev_free_io(bdev_io);
	}

	assert(job_parm->job_cplt_cb != NULL);
	job_parm->job_cplt_cb(job_parm, success);		
}
/* End of CS_CSX_C */

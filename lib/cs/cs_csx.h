/**
 * @file cs_csx.h
 * @author Alex Hsu (ahsu@petaio.com)
 * @brief 
 * @version 0.1
 * @date 2022-02-08
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef CS_CSX_H
#define CS_CSX_H
/************************************************************/
/*                                                          */
/* INCLUDE FILE DECLARATIONS                                */
/*                                                          */
/************************************************************/
#include "spdk/env.h"
#include "spdk/bdev_module.h"
#include "spdk/nvme_spec.h"
#include "cs_common.h"
#include "cs_cse.h"

/************************************************************/
/*                                                          */
/* NAMING CONSTANT DECLARATIONS                             */ 
/* {Constants defines for other components reference.}      */
/*                                                          */
/************************************************************/ 

/************************************************************/
/*                                                          */
/* MACRO FUNCTION DECLARATIONS                              */          
/* {MACRO functions defines for other components reference.}*/
/*                                                          */ 
/************************************************************/

/************************************************************/
/*                                                          */
/* DATA TYPE DECLARATIONS                                   */ 
/* {DATA TYPE defines for other components reference.}      */
/*                                                          */
/************************************************************/
typedef void (*cs_csx_job_cplt_cb)(void *ctx, bool success);

struct cs_csx_job_parm {
	union {
		struct {
			struct cs_cse_act_func *cse_act_func;
			bool activate;
			struct cs_cse_handle *cse_handle;
		} prog_act_mgmt;
	} u;

	cs_csx_job_cplt_cb job_cplt_cb;
};

/************************************************************/
/*                                                          */ 
/* EXPORTED SUBPROGRAM SPECIFICATIONS                       */
/* {Function routine define for other components reference.}*/
/*                                                          */
/************************************************************/

int cs_csx_init(struct cs_csx *csx, struct spdk_bdev *bdev, cs_csx_init_cplt_cb cb, void *cb_arg);

//-------------------------------------------------
// csx job APIs 
// 1. need be called from spdk_thread_send_msg
//-------------------------------------------------
void cs_csx_job_prog_act_mgmt(void *ctx);
/* End of CS_CSX_H */
#endif

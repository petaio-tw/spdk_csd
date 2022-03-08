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
typedef enum {
	CSX_STATE_INIT_START = 0,
	CSX_STATE_GET_CSE_CNT,
	CSX_STATE_WAIT_FOR_CSE_CNT,
	CSX_STATE_GET_CSE_LIST,
	CSX_STATE_WAIT_FOR_CSE_LIST,
	CSX_STATE_GET_FUNC_CNT,
	CSX_STATE_WAIT_FOR_FUNC_CNT,
	CSX_STATE_GET_FUNC_LIST,
	CSX_STATE_WAIT_FOR_FUNC_LIST,
	CSX_STATE_INIT_DONE,
	CSX_STATE_INIT_FAIL
} csx_state;

typedef void (*cs_csx_init_cplt_cb)(void *cb_arg, int rc);

struct cs_csx_ctxt {
	struct cs_csx *csx;
	void *csx_usr_ctxt;
	uint32_t checksum;	// make sure above fields are const context

	TAILQ_ENTRY(cs_csx_ctxt) link;
} __attribute__((packed));

struct cs_csx {
	// spdk relative
	struct spdk_bdev *bdev;
	struct spdk_bdev_desc *bdev_desc;
	struct spdk_io_channel *ch;
	struct spdk_thread *thread;

	struct spdk_nvme_prog_info_log *nvme_prog_info;
	int num_func;
	
	TAILQ_HEAD(, cs_cse) cse_list;
	int num_cse;

	TAILQ_HEAD(, cs_csx_ctxt) csx_ctxt_list;
	int num_csx_ctxt;

	TAILQ_ENTRY(cs_csx) link;

	void *tmp_buf;

	struct spdk_poller *reset_poller;

	cs_csx_init_cplt_cb init_cplt_cb;
	void *init_cplt_cb_arg;

	csx_state state;
};

/************************************************************/
/*                                                          */ 
/* EXPORTED SUBPROGRAM SPECIFICATIONS                       */
/* {Function routine define for other components reference.}*/
/*                                                          */
/************************************************************/

int cs_csx_init(struct cs_csx *csx, struct spdk_bdev *bdev, cs_csx_init_cplt_cb cb, void *cb_arg);

/* End of CS_CSX_H */
#endif

/**
 * @file cs_common.h
 * @author Alex Hsu (ahsu@petaio.com)
 * @brief 
 * @version 0.1
 * @date 2022-03-09
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef CS_COMMON_H
#define CS_COMMON_H
/************************************************************/
/*                                                          */
/* INCLUDE FILE DECLARATIONS                                */
/*                                                          */
/************************************************************/
#include "spdk/env.h"
#include "spdk/bdev_module.h"
#include "spdk/nvme_spec.h"

/************************************************************/
/*                                                          */
/* NAMING CONSTANT DECLARATIONS                             */ 
/* {Constants defines for other components reference.}      */
/*                                                          */
/************************************************************/ 
#define CS_MAX_FUNC_PER_CSX		16
#define CS_MAX_CSE_PER_CSX		16

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
//-----------------------------------
// cs_cse
//-----------------------------------
struct cs_cse_handle {
	struct cs_cse *cse;
	void *cse_usr_ctx;
	uint32_t checksum;	// make sure above fields are const context

	volatile uint32_t num_issued_cmd;
	TAILQ_ENTRY(cs_cse_handle) link;
} __attribute__((packed));

struct cs_cse_act_func {
	struct cs_func *func;
	struct cs_cse *cse;
	void *func_usr_ctx;

	TAILQ_ENTRY(cs_cse_act_func) link;
};

struct cs_cse {
	spdk_nvme_ceid_t ceid;
	struct cs_csx *csx;
	char name[NAME_MAX];

	// since there is no clear definition about activated program in TP4091,
	// we assume the same program can't be activated twice in the same cse.
	int max_cse_act_func;

	TAILQ_HEAD(, cs_cse_act_func) cse_act_func_list;
	int num_cse_act_func;

	TAILQ_HEAD(, cs_cse_handle) cse_handle_list;
	int num_cse_handle;
};

//-----------------------------------
// cs_func
//-----------------------------------
struct cs_func {
	spdk_nvme_puid puid;	// program unique identifier
	uint32_t pid;	// program identifier
	struct cs_csx *csx;
	char *func_name;
	bool is_occupied;
	bool is_downloadable;
	struct cs_cse* assoc_cse_list[MAX_COMPUTE_ENGINE_DESC];
	int num_assoc_cse;
};

//-----------------------------------
// cs_csx
//-----------------------------------
typedef enum {
	CSX_STATE_INIT_START = 0,
	CSX_STATE_GET_CSE_CNT,
	CSX_STATE_WAIT_FOR_CSE_CNT,
	CSX_STATE_GET_CSE_LIST,
	CSX_STATE_WAIT_FOR_CSE_LIST,
	CSX_STATE_GET_CSE_INFO,
	CSX_STATE_WAIT_FOR_CSE_INFO,	
	CSX_STATE_GET_FUNC_CNT,
	CSX_STATE_WAIT_FOR_FUNC_CNT,
	CSX_STATE_GET_FUNC_LIST,
	CSX_STATE_WAIT_FOR_FUNC_LIST,
	CSX_STATE_INIT_DONE,
	CSX_STATE_INIT_FAIL
} csx_state;

typedef void (*cs_csx_init_cplt_cb)(void *cb_arg, int rc);

struct cs_csx_handle {
	struct cs_csx *csx;
	void *csx_usr_ctx;
	uint32_t checksum;	// make sure above fields are const context

	TAILQ_ENTRY(cs_csx_handle) link;
} __attribute__((packed));

struct cs_csx {
	// spdk relative
	struct spdk_bdev *bdev;
	struct spdk_bdev_desc *bdev_desc;
	struct spdk_io_channel *ch;
	struct spdk_thread *thread;

	struct spdk_nvme_prog_info_log *nvme_prog_info;

	struct cs_func func_list[CS_MAX_FUNC_PER_CSX];	
	int num_func;
	
	struct cs_cse cse_list[CS_MAX_CSE_PER_CSX];
	int num_cse;

	TAILQ_HEAD(, cs_csx_handle) csx_handle_list;
	int num_csx_handle;

	TAILQ_ENTRY(cs_csx) link;

	void *tmp_buf;

	struct spdk_poller *reset_poller;

	cs_csx_init_cplt_cb init_cplt_cb;
	void *init_cplt_cb_arg;
	int init_cse_idx;

	csx_state state;
};

/************************************************************/
/*                                                          */ 
/* EXPORTED SUBPROGRAM SPECIFICATIONS                       */
/* {Function routine define for other components reference.}*/
/*                                                          */
/************************************************************/

/* End of CS_COMMON_H */
#endif

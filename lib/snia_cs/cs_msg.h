/**
 * @file cs_msg.h
 * @author Alex Hsu (ahsu@petaio.com)
 * @brief 
 * @version 0.1
 * @date 2021-10-05
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef CS_MSG_H
#define CS_MSG_H
/************************************************************/
/*                                                          */
/* INCLUDE FILE DECLARATIONS                                */
/*                                                          */
/************************************************************/
#include "cs_common.h"
#include "spdk/nvme.h"

/************************************************************/
/*                                                          */
/* NAMING CONSTANT DECLARATIONS                             */ 
/* {Constants defines for other components reference.}      */
/*                                                          */
/************************************************************/ 
#define MAX_BASE_NAME_LEN		32

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
	CS_MSG_RC_SUCESS			= 0,
	CS_MSG_RC_MEM_ALLOC_FAIL,
	CS_MSG_RC_GEN_FAIL
} cs_msg_rc_t;

typedef void (*cs_msg_cpl_cb_fn)(void *cb_ctx);

//------------------------------------
// cs_msg_attach_nvme_csx
//
typedef struct _cs_msg_attach_nvme_csx_ctx {
	// --- input ---
	char 				base_name[MAX_BASE_NAME_LEN + 1];	
	struct spdk_nvme_transport_id 	trid;
	struct spdk_nvme_ctrlr_opts 	opts;	
	uint32_t 			max_bdev_cnt;
	uint32_t 			prchk_flags;				// protect check flags	
	cs_msg_cpl_cb_fn 		cpl_cb_fn;
	void 				*cpl_cb_ctx;
	// --- output ---
	cs_csx_t			*csx;
	cs_msg_rc_t 			rc;
	// --- internal ---

} cs_msg_attach_nvme_csx_ctx_t; 

//------------------------------------
// cs_msg_get_cse_list
//
typedef struct _cs_msg_get_cse_list_ctx {
	// --- input ---	
	cs_msg_cpl_cb_fn 		cpl_cb_fn;
	void 				*cpl_cb_ctx;
	// --- output ---
	cs_csx_t			*csx;
	cs_msg_rc_t 			rc;
	// --- internal ---
	struct spdk_bdev_desc 		*bdev_desc;
} cs_msg_get_cse_list_ctx_t;

/************************************************************/
/*                                                          */ 
/* EXPORTED SUBPROGRAM SPECIFICATIONS                       */
/* {Function routine define for other components reference.}*/
/*                                                          */
/************************************************************/
void cs_msg_attach_nvme_csx(void *ctx);
void cs_msg_get_cse_list(void *ctx);

/* End of CS_MSG_H */
#endif

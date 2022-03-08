/**
 * @file cs_cse.h
 * @author Alex Hsu (ahsu@petaio.com)
 * @brief 
 * @version 0.1
 * @date 2022-02-10
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef CS_CSE_H
#define CS_CSE_H
/************************************************************/
/*                                                          */
/* INCLUDE FILE DECLARATIONS                                */
/*                                                          */
/************************************************************/
#include "spdk/env.h"
#include "spdk/nvme_spec.h"
#include "cs_csx.h"

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
struct cs_cse_ctxt {
	struct cs_cse *cse;
	void *cse_usr_ctxt;
	uint32_t checksum;	// make sure above fields are const context

	TAILQ_ENTRY(cs_cse_ctxt) link;
} __attribute__((packed));

struct cs_cse {
	spdk_nvme_ceid_t nvme_ceid;
	struct cs_csx *csx;
	char name[NAME_MAX];			
	int max_actived_prog;
	int cur_actived_prog;

	TAILQ_HEAD(, cs_cse_ctxt) cse_ctxt_list;
	int num_cse_ctxt;

	TAILQ_ENTRY(cs_cse) link;
};

/************************************************************/
/*                                                          */ 
/* EXPORTED SUBPROGRAM SPECIFICATIONS                       */
/* {Function routine define for other components reference.}*/
/*                                                          */
/************************************************************/

/* End of CS_CSE_H */
#endif

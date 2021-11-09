/**
 * @file cs_common.h
 * @author Alex Hsu (ahsu@petaio.com)
 * @brief 
 * @version 0.1
 * @date 2021-10-06
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef CS_COMMON_H
#define CS_COMMON_H
/************************************************************/
/*                                                          */
/* INCLUDE FILE DECLARATIONS                                */
/*                                                          */
/************************************************************/
#include "spdk/stdinc.h"
#include "spdk/queue.h"

/************************************************************/
/*                                                          */
/* NAMING CONSTANT DECLARATIONS                             */ 
/* {Constants defines for other components reference.}      */
/*                                                          */
/************************************************************/ 
#define MAX_BDEVS_PER_CSX 		16

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
typedef struct _cs_csx {
	TAILQ_ENTRY(_cs_csx) next;      		// next attached CSx
	size_t bdev_count;				// total bdev count (NSID) in CSx
	const char *bdev_names[MAX_BDEVS_PER_CSX];	// last entry is the name of CSx
	const char *csx_name;
	uint32_t cse_cnt;
	struct spdk_csd_compute_engine_list *p_cse_list;
	uint32_t cse_list_size;
	void *cmb_va;
	uint64_t cmb_base_pa;
	size_t cmb_size;
} cs_csx_t;

/************************************************************/
/*                                                          */ 
/* EXPORTED SUBPROGRAM SPECIFICATIONS                       */
/* {Function routine define for other components reference.}*/
/*                                                          */
/************************************************************/

/* End of CS_COMMON_H */
#endif

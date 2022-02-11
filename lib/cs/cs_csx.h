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
struct cs_csx {
	struct spdk_bdev *bdev;
	TAILQ_HEAD(, cs_cse) cse_list;
	TAILQ_ENTRY(cs_csx) link;	
};

/************************************************************/
/*                                                          */ 
/* EXPORTED SUBPROGRAM SPECIFICATIONS                       */
/* {Function routine define for other components reference.}*/
/*                                                          */
/************************************************************/
int cs_csx_init(struct cs_csx *csx, struct spdk_bdev *bdev);

/* End of CS_CSX_H */
#endif

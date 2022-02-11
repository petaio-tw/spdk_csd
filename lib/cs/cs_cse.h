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
struct cs_cse {
	TAILQ_ENTRY(cs_cse) link;
};

/************************************************************/
/*                                                          */ 
/* EXPORTED SUBPROGRAM SPECIFICATIONS                       */
/* {Function routine define for other components reference.}*/
/*                                                          */
/************************************************************/
int cs_cse_init(struct cs_cse *cse);

/* End of CS_CSE_H */
#endif

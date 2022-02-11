/**
 * @file cs_mgr.h
 * @author Alex Hsu (ahsu@petaio.com)
 * @brief 
 * @version 0.1
 * @date 2022-02-10
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef CS_MGR_H
#define CS_MGR_H
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


/************************************************************/
/*                                                          */ 
/* EXPORTED SUBPROGRAM SPECIFICATIONS                       */
/* {Function routine define for other components reference.}*/
/*                                                          */
/************************************************************/
int cs_mgr_init(void);

/* End of CS_MGR_H */
#endif

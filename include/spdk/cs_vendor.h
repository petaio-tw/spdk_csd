/**
 * @file cs_vendor.h
 * @author Alex Hsu (ahsu@petaio.com)
 * @brief 
 * @version 0.1
 * @date 2021-10-07
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef CS_VENDOR_H
#define CS_VENDOR_H
/************************************************************/
/*                                                          */
/* INCLUDE FILE DECLARATIONS                                */
/*                                                          */
/************************************************************/
#include "spdk/cs.h"

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
typedef struct _cs_init_param {
	char 	*script_dir_path;
} cs_init_param_t;

/************************************************************/
/*                                                          */ 
/* EXPORTED SUBPROGRAM SPECIFICATIONS                       */
/* {Function routine define for other components reference.}*/
/*                                                          */
/************************************************************/
void cs_init(cs_init_param_t *param);
CS_STATUS cs_scan_csxes(void);
CS_STATUS cs_get_cse_list(void);
CS_STATUS cs_map_cmb(void);
void* cs_get_in_cmb_buf(void);
void* cs_get_out_cmb_buf(void);
CS_STATUS cs_exec_program(void *in_buf, void *out_buf);
/* End of CS_VENDOR_H */
#endif

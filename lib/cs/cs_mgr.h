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
#define NUM_PUID_MAP_ETY()	(sizeof(g_puid_map) / sizeof(puid_name_map_entry))

/************************************************************/
/*                                                          */
/* DATA TYPE DECLARATIONS                                   */ 
/* {DATA TYPE defines for other components reference.}      */
/*                                                          */
/************************************************************/
typedef struct _puid_name_map_entry {
	spdk_nvme_puid	puid;
	char		func_name[NAME_MAX];
} puid_name_map_entry;

/************************************************************/
/*                                                          */ 
/* EXPORTED SUBPROGRAM SPECIFICATIONS                       */
/* {Function routine define for other components reference.}*/
/*                                                          */
/************************************************************/
extern const puid_name_map_entry g_puid_map[];

int cs_mgr_init(void);
// valid after call cs_mgr_init() and it's success
bool cs_mgr_is_init_done(void);
struct cs_csx* cs_mgr_get_csx(char *name, struct cs_csx *cur_csx);
char *cs_mgr_get_func_name(spdk_nvme_puid puid);
bool cs_mgr_get_puid(char *func_name, spdk_nvme_puid *puid);
/* End of CS_MGR_H */
#endif

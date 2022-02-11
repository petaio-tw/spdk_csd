/**
 * @file cs_mgr.c
 * @author Alex Hsu (ahsu@petaio.com)
 * @brief 
 * @version 0.1
 * @date 2022-02-10
 * 
 * @copyright Copyright (c) 2022
 * 
 */

/**********************************************************/
/*                                                        */  
/* INCLUDE FILE DECLARATIONS                              */
/*                                                        */
/**********************************************************/
#include "spdk/stdinc.h"
#include "spdk/bdev_module.h"
#include "cs_mgr.h"
#include "cs_csx.h"

/**********************************************************/
/*                                                        */  
/* NAMING CONSTANT DECLARATIONS                           */  
/* {Constants define for LOCAL reference ONLY.}           */  
/*                                                        */  
/**********************************************************/

/**********************************************************/
/*                                                        */
/* MACRO FUNCTION DECLARATIONS                            */
/* {MACRO functions define for LOCAL reference ONLY.}     */
/*                                                        */
/**********************************************************/

/**********************************************************/
/*                                                        */
/* DATA TYPE DECLARATIONS                                 */
/* {DATA TYPE define for LOCAL reference ONLY.}           */
/*                                                        */  
/**********************************************************/
typedef struct cs_mgr {
	TAILQ_HEAD(, cs_csx) csx_list;
	int csx_cnt;
} cs_mgr_st;

/**********************************************************/
/*                                                        */
/* LOCAL SUBPROGRAM DECLARATIONS                          */
/* {Function routines define for LOCAL reference ONLY.}   */
/*                                                        */
/**********************************************************/

/**********************************************************/
/*                                                        */
/* STATIC VARIABLE DECLARATIONS                           */
/* {STATIC VARIABLES defines for LOCAL reference ONLY.}   */
/*                                                        */
/**********************************************************/
static cs_mgr_st g_cs_mgr = {
	.csx_list = TAILQ_HEAD_INITIALIZER(g_cs_mgr.csx_list),
	.csx_cnt = 0,
};
/**********************************************************/
/*                                                        */
/* EXPORTED SUBPROGRAM BODIES                             */
/* {C code body of each EXPORTED function routine.}       */
/*                                                        */
/**********************************************************/

int cs_mgr_init(void)
{
	struct spdk_bdev *bdev;

	bdev = spdk_bdev_first_leaf();
	while (bdev != NULL) {
		struct cs_csx *csx = NULL;
		int rc = 0;

		//--------------------------------
		// allocate and init csx object
		csx = calloc(1, sizeof(struct cs_csx));
		if (csx == NULL) {
			return -ENOMEM;
		}		

		rc = cs_csx_init(csx, bdev);
		if (rc != 0) {
			return rc;
		}

		//--------------------------------
		// insert csx object to cs_mgr
		TAILQ_INSERT_TAIL(&g_cs_mgr.csx_list, csx, link);
		g_cs_mgr.csx_cnt++;

		//--------------------------------
		// find next cs device
		bdev = spdk_bdev_next_leaf(bdev);
	}

	return (g_cs_mgr.csx_cnt > 0) ? 0 : -ENXIO;
}

/**********************************************************/
/*                                                        */
/* LOCAL SUBPROGRAM BODIES                                */
/* {C code body of each LOCAL function routines.}         */
/*                                                        */
/**********************************************************/

/* End of CS_MGR_C */

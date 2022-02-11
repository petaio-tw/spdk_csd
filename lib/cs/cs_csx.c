/**
 * @file cs_csx.c
 * @author Alex Hsu (ahsu@petaio.com)
 * @brief 
 * @version 0.1
 * @date 2022-02-08
 * 
 * @copyright Copyright (c) 2022
 * 
 */

/**********************************************************/
/*                                                        */  
/* INCLUDE FILE DECLARATIONS                              */
/*                                                        */
/**********************************************************/
#include "cs_csx.h"
#include "cs_cse.h"

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

/**********************************************************/
/*                                                        */
/* EXPORTED SUBPROGRAM BODIES                             */
/* {C code body of each EXPORTED function routine.}       */
/*                                                        */
/**********************************************************/
int cs_csx_init(struct cs_csx *csx, struct spdk_bdev *bdev)
{
	struct spdk_bdev_alias *tmp;
	struct cs_cse *cse;

	printf("bdev name:%s\n", bdev->name);
	TAILQ_FOREACH(tmp, spdk_bdev_get_aliases(bdev), tailq) {
		printf("bdev alias name:%s\n", tmp->alias.name);
	}

	TAILQ_INIT(&csx->cse_list);
	csx->bdev = bdev;

	//TODO: query csx for information about cse and init cse object.
	// currently we assume only one cse for testing.
	cse = calloc(1, sizeof(struct cs_cse));
	if (csx == NULL) {
		return -ENOMEM;
	}

	cs_cse_init(cse);
	TAILQ_INSERT_TAIL(&csx->cse_list, cse, link);

	return 0;
}


/**********************************************************/
/*                                                        */
/* LOCAL SUBPROGRAM BODIES                                */
/* {C code body of each LOCAL function routines.}         */
/*                                                        */
/**********************************************************/

/* End of CS_CSX_C */

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
#include "spdk/log.h"
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
	int num_csx;

	//---------------------
	// internal use
	int num_init_csx;
	pthread_mutex_t mutex;
} cs_mgr_st;

/**********************************************************/
/*                                                        */
/* LOCAL SUBPROGRAM DECLARATIONS                          */
/* {Function routines define for LOCAL reference ONLY.}   */
/*                                                        */
/**********************************************************/
static
void cs_mgr_csx_init_cplt(void *cb_arg, int rc);

/**********************************************************/
/*                                                        */
/* STATIC VARIABLE DECLARATIONS                           */
/* {STATIC VARIABLES defines for LOCAL reference ONLY.}   */
/*                                                        */
/**********************************************************/
static cs_mgr_st g_cs_mgr;

const puid_name_map_entry g_puid_map[] = {
	{.puid.field.oui_36 = 0x123456789, .puid.field.upi = 0x01, .func_name = "rgb2gray"},
	{.puid.field.oui_36 = 0x123456789, .puid.field.upi = 0x02, .func_name = "crc32"},
};
SPDK_STATIC_ASSERT((sizeof(g_puid_map) % sizeof(puid_name_map_entry)) == 0, "Incorrect size");
/**********************************************************/
/*                                                        */
/* EXPORTED SUBPROGRAM BODIES                             */
/* {C code body of each EXPORTED function routine.}       */
/*                                                        */
/**********************************************************/

int cs_mgr_init(void)
{
	struct spdk_bdev *bdev;

	//------------------------------
	// init default value for cs_mgr
	memset(&g_cs_mgr, 0, sizeof(cs_mgr_st));
	TAILQ_INIT(&g_cs_mgr.csx_list);
	pthread_mutex_init(&g_cs_mgr.mutex, NULL);

	//------------------------------
	// scan csx
	bdev = spdk_bdev_first_leaf();
	while (bdev != NULL) {
		struct cs_csx *csx = NULL;
		int rc = 0;

		csx = calloc(1, sizeof(struct cs_csx));
		if (csx == NULL) {
			return -ENOMEM;
		}		

		g_cs_mgr.num_init_csx++;
		rc = cs_csx_init(csx, bdev, cs_mgr_csx_init_cplt, csx);
		if (rc != 0) {
			cs_mgr_csx_init_cplt(csx, rc);
			return rc;
		}

		bdev = spdk_bdev_next_leaf(bdev);
	}

	return (g_cs_mgr.num_init_csx > 0) ? 0 : -ENXIO;
}

bool cs_mgr_is_init_done(void)
{
	return (g_cs_mgr.num_init_csx == g_cs_mgr.num_csx) ? true : false;
}

struct cs_csx* cs_mgr_get_csx(char *name, struct cs_csx *cur_csx)
{
	struct cs_csx *csx;
	bool fetch_csx = (cur_csx == NULL) ? true : false;

	TAILQ_FOREACH(csx, &g_cs_mgr.csx_list, link) {
		struct spdk_bdev *bdev = csx->bdev;
		assert((bdev != NULL));

		// check if we need output csx with match query condition
		if (!fetch_csx) {
			// only check candidate after current csx
			if (csx == cur_csx) {
				fetch_csx = true;
			}
			continue;			
		}

		// get csx by name
		if (name == NULL) {
			return csx;
		} else {			
			if (strcmp(spdk_bdev_get_name(bdev), name) == 0) {
				return csx;
			} else {
				struct spdk_bdev_alias *item;

				TAILQ_FOREACH(item, spdk_bdev_get_aliases(bdev), tailq) {
					if (strcmp(item->alias.name, name) == 0) {
						return csx;
					}
				}			
			}
		}
	}

	return NULL;
}

char *cs_mgr_get_func_name(spdk_nvme_puid puid) 
{
	uint32_t i;

	for (i = 0; i < NUM_PUID_MAP_ETY(); i++) {
		if (g_puid_map[i].puid.val == puid.val) {
			return (char *)g_puid_map[i].func_name;
		}
	}

	return NULL;
}
/**********************************************************/
/*                                                        */
/* LOCAL SUBPROGRAM BODIES                                */
/* {C code body of each LOCAL function routines.}         */
/*                                                        */
/**********************************************************/
static
void cs_mgr_csx_init_cplt(void *cb_arg, int rc)
{
	struct cs_csx *csx = (struct cs_csx *)cb_arg;

	pthread_mutex_lock(&g_cs_mgr.mutex);
	TAILQ_INSERT_TAIL(&g_cs_mgr.csx_list, csx, link);
	g_cs_mgr.num_csx++;
	pthread_mutex_unlock(&g_cs_mgr.mutex);

	if (rc == 0) {
		assert(csx->state == CSX_STATE_INIT_DONE);
	} else {
		SPDK_ERRLOG("cs_mgr: csx init fail\n");
		assert(csx->state != CSX_STATE_INIT_DONE);
	}
}
/* End of CS_MGR_C */

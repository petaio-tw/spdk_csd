/**
 * @file cs_shell.c
 * @author Alex Hsu (ahsu@petaio.com)
 * @brief 
 * @version 0.1
 * @date 2021-10-07
 * 
 * @copyright Copyright (c) 2021
 * 
 */

/**********************************************************/
/*                                                        */  
/* INCLUDE FILE DECLARATIONS                              */
/*                                                        */
/**********************************************************/
#include <stddef.h>
#include "spdk/shell.h"
#include "spdk/event.h"
#include "spdk/cs.h"
#include "spdk/cs_vendor.h"

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
//-----------------------------------------
// cs shell command
static
int32_t cs_shell_cmd_scan_csx(void);
//-----------------------------------------
static 
int cs_shell_parse_arg(int ch, char *arg);
static 
void cs_shell_usage(void);
static 
void cs_shell_start(void *arg);
static
void* cs_shell_app(void* data);
/**********************************************************/
/*                                                        */
/* STATIC VARIABLE DECLARATIONS                           */
/* {STATIC VARIABLES defines for LOCAL reference ONLY.}   */
/*                                                        */
/**********************************************************/
// Shell Command Table
static const SHELL_CMD_TABLE_T shell_cmd_table[] =
{
	// Command,			Explanation,				Function pointer
	{"help",			"print command list",			spdk_shell_cmd_print_list},
	{"scsx",			"scan csx",				cs_shell_cmd_scan_csx},			
	{NULL, NULL, NULL},		// should end with NULL
};

static pthread_t g_user_app;
static char *g_script_dir_path;
/**********************************************************/
/*                                                        */
/* EXPORTED SUBPROGRAM BODIES                             */
/* {C code body of each EXPORTED function routine.}       */
/*                                                        */
/**********************************************************/

int
main(int argc, char **argv)
{
	struct spdk_app_opts opts = {};
	int rc = 0;

	/* Set default values in opts structure. */
	spdk_app_opts_init(&opts, sizeof(opts));
	opts.name = "cs_shell";

	/*
	 * Parse built-in SPDK command line parameters as well
	 * as our custom one(s).
	 */
	if ((rc = spdk_app_parse_args(argc, argv, &opts, "P:", NULL, 
				      cs_shell_parse_arg, cs_shell_usage)) != 
	    SPDK_APP_PARSE_ARGS_SUCCESS) {
		exit(rc);
	};

	/*
	 * spdk_app_start() will initialize the SPDK framework, call hello_start(),
	 * and then block until spdk_app_stop() is called (or if an initialization
	 * error occurs, spdk_app_start() will return with rc even without calling
	 * hello_start().
	 */
	rc = spdk_app_start(&opts, cs_shell_start, NULL);
	if (rc) {
		SPDK_ERRLOG("ERROR starting application\n");
	}

	/* At this point either spdk_app_stop() was called, or spdk_app_start()
	 * failed because of internal error.
	 */

	/* When the app stops, free up memory that we allocated. */
	//spdk_dma_free();

	/* Gracefully close out all of the SPDK subsystems. */
	spdk_app_fini();
	return rc;
}


/**********************************************************/
/*                                                        */
/* LOCAL SUBPROGRAM BODIES                                */
/* {C code body of each LOCAL function routines.}         */
/*                                                        */
/**********************************************************/
static
int32_t cs_shell_cmd_scan_csx(void) {
	cs_scan_csxes();
	return 0;
}

/*
 * This function is called to parse the parameters that are specific to this application
 */
static 
int cs_shell_parse_arg(int ch, char *arg)
{
	switch (ch) {
	case 'P':
		g_script_dir_path = arg;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

/*
 * Usage function for printing parameters that are specific to this application
 */
static 
void cs_shell_usage(void)
{
	printf(" -P <path>		   path of script\n");
}

/*
 * Our initial event that kicks off everything from main().
 */
static 
void cs_shell_start(void *arg)
{	
	cs_init_param_t param;

	param.script_dir_path = g_script_dir_path;
	cs_init(&param);

	// start shell app
	pthread_create(&g_user_app, NULL, cs_shell_app, "cs_shell");
}

static
void* cs_shell_app(void* data)
{
	spdk_shell_init(shell_cmd_table);
	spdk_shell_task();

	return 0;
}
/* End of CS_SHELL_C */
















/**
 * @file csd_shell.c
 * @author Alex Hsu (ahsu@petaio.com)
 * @brief 
 * @version 0.1
 * @date 2021-08-30
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
#include <pthread.h>
#include "spdk/stdinc.h"
#include "spdk/thread.h"
#include "spdk/bdev.h"
#include "spdk/env.h"
#include "spdk/event.h"
#include "spdk/log.h"
#include "spdk/string.h"
#include "spdk/bdev_zone.h"
#include "spdk/shell.h"

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
/*
 * We'll use this struct to gather housekeeping hello_context to pass between
 * our events and callbacks.
 */
struct hello_context_t {
	struct spdk_bdev *bdev;
	struct spdk_bdev_desc *bdev_desc;
	struct spdk_io_channel *bdev_io_channel;
	char *buff;
	char *bdev_name;
	struct spdk_bdev_io_wait_entry bdev_io_wait;
	struct spdk_poller *shell_poller;
	volatile bool get_cmd_resp;
	pthread_t user_app;
	struct spdk_thread *app_thread;
};

/**********************************************************/
/*                                                        */
/* LOCAL SUBPROGRAM DECLARATIONS                          */
/* {Function routines define for LOCAL reference ONLY.}   */
/*                                                        */
/**********************************************************/
static
void* user_app(void* data);
static
int32_t shell_hello_write(void);
static
int32_t shell_hello_read(void);
static
int32_t shell_stop_app(void);
static 
int32_t shell_open_bdev(void);
static 
void hello_open_bdev(void *arg);
static 
void hello_start(void *arg1);
static 
void hello_bdev_usage(void);
static 
int hello_bdev_parse_arg(int ch, char *arg);
static 
void hello_stop_app(void *arg) ;
static 
void read_complete(struct spdk_bdev_io *bdev_io, bool success, void *cb_arg);
static 
void hello_read(void *arg);
static 
void write_complete(struct spdk_bdev_io *bdev_io, bool success, void *cb_arg);
static 
void hello_write(void *arg);
static 
void hello_bdev_event_cb(enum spdk_bdev_event_type type, struct spdk_bdev *bdev, void *event_ctx);
static 
void reset_zone_complete(struct spdk_bdev_io *bdev_io, bool success, void *cb_arg);
static 
void hello_reset_zone(void *arg);
/**********************************************************/
/*                                                        */
/* STATIC VARIABLE DECLARATIONS                           */
/* {STATIC VARIABLES defines for LOCAL reference ONLY.}   */
/*                                                        */
/**********************************************************/
static char *g_bdev_name = "Malloc0";
static struct hello_context_t g_hello_context = {};

// Shell Command Table
static const SHELL_CMD_TABLE_T shell_cmd_table[] =
{
	// Command,		Explanation,						Function pointer
	{"help",		"print command list",					spdk_shell_cmd_print_list},
	{"write",		"write hello bdev",					shell_hello_write},
	{"read",		"read hello bdev",					shell_hello_read},
	{"stop",		"stop app",						shell_stop_app},			
	{"open",		"open bdev",						shell_open_bdev},			
	{NULL, NULL, NULL},		// should end with NULL
};

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
	opts.name = "hello_bdev";

	/*
	 * Parse built-in SPDK command line parameters as well
	 * as our custom one(s).
	 */
	if ((rc = spdk_app_parse_args(argc, argv, &opts, "b:", NULL, hello_bdev_parse_arg,
				      hello_bdev_usage)) != SPDK_APP_PARSE_ARGS_SUCCESS) {
		exit(rc);
	}
	g_hello_context.bdev_name = g_bdev_name;

	/*
	 * spdk_app_start() will initialize the SPDK framework, call hello_start(),
	 * and then block until spdk_app_stop() is called (or if an initialization
	 * error occurs, spdk_app_start() will return with rc even without calling
	 * hello_start().
	 */
	rc = spdk_app_start(&opts, hello_start, &g_hello_context);
	if (rc) {
		SPDK_ERRLOG("ERROR starting application\n");
	}

	/* At this point either spdk_app_stop() was called, or spdk_app_start()
	 * failed because of internal error.
	 */

	/* When the app stops, free up memory that we allocated. */
	spdk_dma_free(g_hello_context.buff);

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
void* user_app(void* data)
{
	spdk_shell_init(shell_cmd_table);
	spdk_shell_task();

	return 0;
}

static
int32_t shell_hello_write(void)
{
	bool blocking;
	uint32_t argv[1];

	if (false == spdk_shell_common_get_parameters_uint32(argv, 1)) {
		return SHELL_ERROR_CODE_WRONG_NUM_PARAMETERS;
	}

	blocking = (bool)argv[0];

	g_hello_context.get_cmd_resp = false;
	spdk_thread_send_msg(g_hello_context.app_thread, hello_write, &g_hello_context);

	if (blocking == true) {
		SPDK_NOTICELOG("wait write result...\n");
		while(g_hello_context.get_cmd_resp == false);
		SPDK_NOTICELOG("get write result...\n");
	}

	return 0;
}

static
int32_t shell_hello_read(void)
{
	bool blocking;
	uint32_t argv[1];

	if (false == spdk_shell_common_get_parameters_uint32(argv, 1)) {
		return SHELL_ERROR_CODE_WRONG_NUM_PARAMETERS;
	}

	blocking = (bool)argv[0];

	g_hello_context.get_cmd_resp = false;
	spdk_thread_send_msg(g_hello_context.app_thread, hello_read, &g_hello_context);

	if (blocking == true) {
		SPDK_NOTICELOG("wait read result...\n");
		while(g_hello_context.get_cmd_resp == false);
		SPDK_NOTICELOG("get read result...\n");
	}

	return 0;	
}

static
int32_t shell_stop_app(void)
{
	bool blocking;
	uint32_t argv[1];

	if (false == spdk_shell_common_get_parameters_uint32(argv, 1)) {
		return SHELL_ERROR_CODE_WRONG_NUM_PARAMETERS;
	}

	blocking = (bool)argv[0];

	g_hello_context.get_cmd_resp = false;
	spdk_thread_send_msg(g_hello_context.app_thread, hello_stop_app, &g_hello_context);

	if (blocking == true) {
		SPDK_NOTICELOG("wait stop...\n");
		while(g_hello_context.get_cmd_resp == false);
		SPDK_NOTICELOG("get stop...\n");
	}

	return 0;	
}

static
int32_t shell_open_bdev(void)
{
	struct hello_context_t *hello_context = &g_hello_context;

	char *bdev_name = NULL;

	bdev_name = (char *)spdk_shell_common_get_parameter_string(0);
	if (bdev_name != NULL) {
		SPDK_NOTICELOG("set bdev_name=%s\n", bdev_name);
		hello_context->bdev_name = bdev_name;
	}

	spdk_thread_send_msg(g_hello_context.app_thread, hello_open_bdev, &g_hello_context);

	return 0;
}

static 
void hello_open_bdev(void *arg)
{	
	struct hello_context_t *hello_context = arg;
	uint32_t blk_size, buf_align;
	int rc = 0;

	hello_context->bdev = NULL;
	hello_context->bdev_desc = NULL;

	SPDK_NOTICELOG("Successfully started the application\n");

	/*
	 * There can be many bdevs configured, but this application will only use
	 * the one input by the user at runtime.
	 *
	 * Open the bdev by calling spdk_bdev_open_ext() with its name.
	 * The function will return a descriptor
	 */
	SPDK_NOTICELOG("Opening the bdev %s\n", hello_context->bdev_name);
	rc = spdk_bdev_open_ext(hello_context->bdev_name, true, hello_bdev_event_cb, NULL,
				&hello_context->bdev_desc);
	if (rc) {
		SPDK_ERRLOG("Could not open bdev: %s\n", hello_context->bdev_name);
		spdk_app_stop(-1);
		return;
	}

	/* A bdev pointer is valid while the bdev is opened. */
	hello_context->bdev = spdk_bdev_desc_get_bdev(hello_context->bdev_desc);


	SPDK_NOTICELOG("Opening io channel\n");
	/* Open I/O channel */
	hello_context->bdev_io_channel = spdk_bdev_get_io_channel(hello_context->bdev_desc);
	if (hello_context->bdev_io_channel == NULL) {
		SPDK_ERRLOG("Could not create bdev I/O channel!!\n");
		spdk_bdev_close(hello_context->bdev_desc);
		spdk_app_stop(-1);
		return;
	}

	/* Allocate memory for the write buffer.
	 * Initialize the write buffer with the string "Hello World!"
	 */
	blk_size = spdk_bdev_get_block_size(hello_context->bdev);
	buf_align = spdk_bdev_get_buf_align(hello_context->bdev);
	hello_context->buff = spdk_dma_zmalloc(blk_size, buf_align, NULL);
	if (!hello_context->buff) {
		SPDK_ERRLOG("Failed to allocate buffer\n");
		spdk_put_io_channel(hello_context->bdev_io_channel);
		spdk_bdev_close(hello_context->bdev_desc);
		spdk_app_stop(-1);
		return;
	}
	snprintf(hello_context->buff, blk_size, "%s", "Hello World!\n");

	if (spdk_bdev_is_zoned(hello_context->bdev)) {
		hello_reset_zone(hello_context);
		/* If bdev is zoned, the callback, reset_zone_complete, will call hello_write() */
		return;
	}
}

/*
 * Our initial event that kicks off everything from main().
 */
static 
void hello_start(void *arg1)
{	
	g_hello_context.app_thread = spdk_get_thread();
	pthread_create(&g_hello_context.user_app, NULL, user_app, "usr_app");	
}


/*
 * Usage function for printing parameters that are specific to this application
 */
static 
void hello_bdev_usage(void)
{
	printf(" -b <bdev>                 name of the bdev to use\n");
}

/*
 * This function is called to parse the parameters that are specific to this application
 */
static 
int hello_bdev_parse_arg(int ch, char *arg)
{
	switch (ch) {
	case 'b':
		g_bdev_name = arg;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static 
void hello_stop_app(void *arg) 
{
	struct hello_context_t *hello_context = arg;

	spdk_put_io_channel(hello_context->bdev_io_channel);
	spdk_bdev_close(hello_context->bdev_desc);
	SPDK_NOTICELOG("Stopping app\n");
	spdk_app_stop(0);
	hello_context->get_cmd_resp = true;
}


/*
 * Callback function for read io completion.
 */
static 
void read_complete(struct spdk_bdev_io *bdev_io, bool success, void *cb_arg)
{
	struct hello_context_t *hello_context = cb_arg;

	if (success) {
		SPDK_NOTICELOG("Read string from bdev : %s\n", hello_context->buff);
	} else {
		SPDK_ERRLOG("bdev io read error\n");
	}

	/* Complete the bdev io and close the channel */
	spdk_bdev_free_io(bdev_io);

	hello_context->get_cmd_resp = true;
}

static 
void hello_read(void *arg)
{
	struct hello_context_t *hello_context = arg;
	int rc = 0;
	uint32_t length = spdk_bdev_get_block_size(hello_context->bdev);

	SPDK_NOTICELOG("Reading io\n");
	rc = spdk_bdev_read(hello_context->bdev_desc, hello_context->bdev_io_channel,
			    hello_context->buff, 0, length, read_complete, hello_context);

	if (rc == -ENOMEM) {
		SPDK_NOTICELOG("Queueing io\n");
		/* In case we cannot perform I/O now, queue I/O */
		hello_context->bdev_io_wait.bdev = hello_context->bdev;
		hello_context->bdev_io_wait.cb_fn = hello_read;
		hello_context->bdev_io_wait.cb_arg = hello_context;
		spdk_bdev_queue_io_wait(hello_context->bdev, hello_context->bdev_io_channel,
					&hello_context->bdev_io_wait);
	} else if (rc) {
		SPDK_ERRLOG("%s error while reading from bdev: %d\n", spdk_strerror(-rc), rc);
		spdk_put_io_channel(hello_context->bdev_io_channel);
		spdk_bdev_close(hello_context->bdev_desc);
		spdk_app_stop(-1);
	}
}

/*
 * Callback function for write io completion.
 */
static 
void write_complete(struct spdk_bdev_io *bdev_io, bool success, void *cb_arg)
{
	struct hello_context_t *hello_context = cb_arg;
	uint32_t length;

	/* Complete the I/O */
	spdk_bdev_free_io(bdev_io);

	if (success) {
		SPDK_NOTICELOG("bdev io write completed successfully\n");
	} else {
		SPDK_ERRLOG("bdev io write error: %d\n", EIO);
		spdk_put_io_channel(hello_context->bdev_io_channel);
		spdk_bdev_close(hello_context->bdev_desc);
		spdk_app_stop(-1);
		return;
	}

	/* Zero the buffer so that we can use it for reading */
	length = spdk_bdev_get_block_size(hello_context->bdev);
	memset(hello_context->buff, 0, length);

	hello_context->get_cmd_resp = true;
}

static 
void hello_write(void *arg)
{
	struct hello_context_t *hello_context = arg;
	int rc = 0;
	uint32_t length = spdk_bdev_get_block_size(hello_context->bdev);

	SPDK_NOTICELOG("Writing to the bdev\n");
	rc = spdk_bdev_write(hello_context->bdev_desc, hello_context->bdev_io_channel,
			     hello_context->buff, 0, length, write_complete, hello_context);

	if (rc == -ENOMEM) {
		SPDK_NOTICELOG("Queueing io\n");
		/* In case we cannot perform I/O now, queue I/O */
		hello_context->bdev_io_wait.bdev = hello_context->bdev;
		hello_context->bdev_io_wait.cb_fn = hello_write;
		hello_context->bdev_io_wait.cb_arg = hello_context;
		spdk_bdev_queue_io_wait(hello_context->bdev, hello_context->bdev_io_channel,
					&hello_context->bdev_io_wait);
	} else if (rc) {
		SPDK_ERRLOG("%s error while writing to bdev: %d\n", spdk_strerror(-rc), rc);
		spdk_put_io_channel(hello_context->bdev_io_channel);
		spdk_bdev_close(hello_context->bdev_desc);
		spdk_app_stop(-1);
	}
}

static 
void hello_bdev_event_cb(enum spdk_bdev_event_type type, struct spdk_bdev *bdev, void *event_ctx)
{
	SPDK_NOTICELOG("Unsupported bdev event: type %d\n", type);
}

static 
void reset_zone_complete(struct spdk_bdev_io *bdev_io, bool success, void *cb_arg)
{
	struct hello_context_t *hello_context = cb_arg;

	/* Complete the I/O */
	spdk_bdev_free_io(bdev_io);

	if (!success) {
		SPDK_ERRLOG("bdev io reset zone error: %d\n", EIO);
		spdk_put_io_channel(hello_context->bdev_io_channel);
		spdk_bdev_close(hello_context->bdev_desc);
		spdk_app_stop(-1);
		return;
	}

	hello_write(hello_context);
}

static 
void hello_reset_zone(void *arg)
{
	struct hello_context_t *hello_context = arg;
	int rc = 0;

	rc = spdk_bdev_zone_management(hello_context->bdev_desc, hello_context->bdev_io_channel,
				       0, SPDK_BDEV_ZONE_RESET, reset_zone_complete, hello_context);

	if (rc == -ENOMEM) {
		SPDK_NOTICELOG("Queueing io\n");
		/* In case we cannot perform I/O now, queue I/O */
		hello_context->bdev_io_wait.bdev = hello_context->bdev;
		hello_context->bdev_io_wait.cb_fn = hello_reset_zone;
		hello_context->bdev_io_wait.cb_arg = hello_context;
		spdk_bdev_queue_io_wait(hello_context->bdev, hello_context->bdev_io_channel,
					&hello_context->bdev_io_wait);
	} else if (rc) {
		SPDK_ERRLOG("%s error while resetting zone: %d\n", spdk_strerror(-rc), rc);
		spdk_put_io_channel(hello_context->bdev_io_channel);
		spdk_bdev_close(hello_context->bdev_desc);
		spdk_app_stop(-1);
	}
}

/* End of CSD_SHELL_C */
















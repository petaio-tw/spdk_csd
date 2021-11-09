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
#pragma pack(push, 1)
/*
 * Bitmap file header
 */
typedef struct tagBITMAPFILEHEADER {
    uint8_t    	bfType[2];    /* 2 bytes */
    uint32_t   	bfSize;       /* 4 bytes */
    uint16_t 	bfReserved1;  /* 2 bytes */
    uint16_t 	bfReserved2;  /* 2 bytes */
    uint32_t  	bfOffBits;    /* 4 bytes */
} BITMAPFILEHEADER;

/*
 * Bitmap info header (Windows)
 */
typedef struct tagBITMAPINFOHEADER {
    uint32_t   	biSize;          /* 4 bytes */
    uint32_t    biWidth;         /* 4 bytes */
    uint32_t    biHeight;        /* 4 bytes */
    uint16_t 	biPlanes;        /* 2 bytes */
    uint16_t 	biBitCount;      /* 2 bytes */
    uint32_t   	biCompression;   /* 4 bytes */
    uint32_t   	biSizeImage;     /* 4 bytes */
    uint32_t    biXPixPerMeter;  /* 4 bytes */
    uint32_t    biYPixPerMeter;  /* 4 bytes */
    uint32_t  	biClrUsed;       /* 4 bytes */
    uint32_t  	biClrImportant;  /* 4 bytes */
} BITMAPINFOHEADER;

/*
 * Bitmap core header (OS/2)
 */
typedef struct tagBITMAPCOREHEADER {
    uint32_t   	bcSize;      /* 4 bytes */
    uint16_t    bcWidth;     /* 2 bytes */
    uint16_t    bcHeight;    /* 2 bytes */
    uint16_t 	bcPlanes;    /* 2 bytes */
    uint16_t 	bcBitCount;  /* 2 bytes */
} BITMAPCOREHEADER;
#pragma pack(pop)
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
static
int32_t cs_shell_cmd_get_csxes(void);
static
int32_t cs_shell_cmd_exec_program(void);
static
int32_t cs_shell_cmd_mem_write(void);
static
int32_t cs_shell_cmd_mem_read(void);
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
	{"csxes",			"get csxes",				cs_shell_cmd_get_csxes},
	{"execp",			"exec prog",				cs_shell_cmd_exec_program},
	{"mw",				"mem write",				cs_shell_cmd_mem_write},
	{"mr",				"mem read",				cs_shell_cmd_mem_read},		
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
int32_t cs_shell_cmd_scan_csx(void) 
{
	cs_scan_csxes();
	cs_get_cse_list();
	cs_map_cmb();
	return 0;
}

static
int32_t cs_shell_cmd_get_csxes(void) 
{
	uint32_t length;
	char *buf = NULL;

	csGetCSxFromPath(NULL, &length, NULL);
	printf("str length of CSxes = %d\n", length);

	buf = calloc(1, length);
	if (buf != NULL) {
		csGetCSxFromPath(NULL, &length, buf);
		printf("CSxes = %s\n", buf);
		free(buf);
	}

	return 0;
}

static
int32_t cs_shell_cmd_exec_program(void)
{	
#if 0	
	int i;

	// allocate device and host memory 
	for (i = 0; i < 2; i++) { 
		status = csAllocMem(dev, 4096, 0, &AFDMArray[i], &vaArray[i]);

		if (status != CS_SUCCESS) {
			ERROR_OUT("AFDM alloc error\n"); 
		}
	}

	// allocate request buffer for 3 args 
	req = calloc(1, sizeof(CsComputeRequest) + (sizeof(CsComputeArg) * 3)); 
	if (!req) {
		ERROR_OUT("memory alloc error\n");
	}

	// read file content to AFDM via p2p access 
	if (!pread(hFile, vaArray[0], 4096, 0)) {
		ERROR_OUT("file read error\n");
	}

	// setup work request 
	req->DevHandle = dev;  
	req->FunctionId = functId; 
	req->NumArgs = 3; 
	argPtr = &req->Args[0]; 
	csHelperSetComputeArg(&argPtr[0], CS_AFDM_TYPE, AFDMArray[0], 0); 
	csHelperSetComputeArg(&argPtr[1], CS_32BIT_VALUE_TYPE, 4096); 
	csHelperSetComputeArg(&argPtr[2], CS_AFDM_TYPE, AFDMArray[1], 0); 

	// do synchronous work request 
	status = csQueueComputeRequest(req, NULL, NULL, NULL, NULL); 
	if (status != CS_SUCCESS) {
		ERROR_OUT("Compute exec error\n"); 
	}
#else

#if 0	// bit flipping example
	uint32_t *in_buf, *out_buf;

	in_buf = cs_get_in_cmb_buf();
	*in_buf = 0x5AA5AA55;

	out_buf = cs_get_out_cmb_buf();
	cs_exec_program(in_buf, out_buf);

	printf("cs exec result=%08X\n", *out_buf);
#else	// RGB to gray example

	/* declare a file pointer */
	FILE    *infile, *outfile;
	char    *in_buf = (char *)cs_get_in_cmb_buf();	
	char    *out_buf = (char *)cs_get_out_cmb_buf();
	long    numbytes;
	size_t  read_cnt;
	
	/* open an existing file for reading */
	infile = fopen("flowers.bmp", "r");
	
	/* quit if the file does not exist */
	if (infile == NULL) {
		return 1;
	}
	
	/* Get the number of bytes */
	fseek(infile, 0L, SEEK_END);
	numbytes = ftell(infile);
	
	/* reset the file position indicator to 
	the beginning of the file */
	fseek(infile, 0L, SEEK_SET);
	
	/* copy all the text into the buffer */
	read_cnt = fread(in_buf, sizeof(char), numbytes, infile);
	printf("file size=%ld, read cnt=%ld\n", numbytes, read_cnt);	
	fclose(infile);


	BITMAPFILEHEADER *file_header = (BITMAPFILEHEADER *)in_buf;
	printf("bfType=%c%c,bfSize=%d\n", file_header->bfType[0], file_header->bfType[1], file_header->bfSize);
	uint32_t *info_header_size = (uint32_t *)((uint64_t)in_buf + sizeof(BITMAPFILEHEADER));
	if (*info_header_size == 12) {
		BITMAPCOREHEADER *core_header = (BITMAPCOREHEADER *)info_header_size;
		printf("(core)w=%d,h=%d\n",core_header->bcWidth, core_header->bcHeight);
	} else if (*info_header_size == 40) {
		BITMAPINFOHEADER *info_header = (BITMAPINFOHEADER *)info_header_size;
		printf("(info)w=%d,h=%d\n",info_header->biWidth, info_header->biHeight);
	} else {
		printf("info_header_size=%d\n", *info_header_size);
		return -1;
	}


	cs_exec_program(in_buf, out_buf);

	outfile = fopen("flowers_gray.bmp", "w");
	/* quit if the file does not exist */
	if (outfile == NULL) {
		return 1;
	}

	fwrite(out_buf, numbytes, 1, outfile);
	fclose(outfile);
#endif
#endif
	return 0;
}

static
int32_t cs_shell_cmd_mem_write(void)
{
	uint32_t *addr;
	char *addr_str, *end_ptr;	
	uint32_t data;

	addr_str = (char *)spdk_shell_common_get_parameter_string(0);
	if (addr_str == NULL) {
		return SHELL_ERROR_CODE_WRONG_NUM_PARAMETERS;
	}

	addr = (uint32_t *)strtoull(addr_str, &end_ptr, 16);
	if (!end_ptr) {
		return SHELL_ERROR_CODE_WRONG_NUM_PARAMETERS;
	}

	data = spdk_shell_common_get_parameter_uint32(1);

	*addr = data;
	printf("[%p]<-%08X\n", addr, data);
	
	return 0;
}

static
int32_t cs_shell_cmd_mem_read(void)
{
	uint32_t *addr;
	char *addr_str, *end_ptr;

	addr_str = (char *)spdk_shell_common_get_parameter_string(0);
	if (addr_str == NULL) {
		return SHELL_ERROR_CODE_WRONG_NUM_PARAMETERS;
	}

	addr = (uint32_t *)strtoull(addr_str, &end_ptr, 16);
	if (!end_ptr) {
		return SHELL_ERROR_CODE_WRONG_NUM_PARAMETERS;
	}

	printf("[%p]->%08X\n", addr, *addr);
	
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
















/***************************************************************************//**

  @file         main.c

  @author       Stephen Brennan

  @date         Thursday,  8 January 2015

  @brief        LSH (Libstephen SHell)

*******************************************************************************/

#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//---------------------------------- spdk start
// add header here
#include "spdk/event.h"
#include "spdk/thread.h"
#include "spdk/cs_spdk.h"
#include "spdk/cs.h"
#include <libgen.h>
//---------------------------------- spdk end

//---------------------------------- spdk start
// add define
#define REQ_FUNC_NAME         "RGB2GRAY"
#define ALLOC_AFDM_SIZE_BYTES (2 * 1024 * 1024)
#define QUEUE_DEPTH           (2)
#define NUM_IN_IMG_FILES      (6)
#define NUM_CS_CMPT_REQ_ARGS  (3)

typedef enum {
  IO_TYPE_IN = 0,
  IO_TYPE_OUT,
  NUM_IO_TYPE
} io_type;

typedef enum {
  JOB_STATE_IDLE = 0,
  JOB_STATE_INPUT,
  JOB_STATE_COMPUTE,
  JOB_STATE_OUTPUT,
  JOB_STATE_FAIL,
  NUM_JOB_STATE
} job_state;

typedef struct {
  char in_img_path[PATH_MAX];
  int in_img_size;
  CS_MEM_HANDLE afdm_handle[NUM_IO_TYPE];
  CS_MEM_PTR  va[NUM_IO_TYPE];
  volatile job_state state;
} rgb2gray_job;
//---------------------------------- spdk end

//---------------------------------- spdk start
// add static variable here
static struct spdk_thread *g_main_thread;
static rgb2gray_job g_r2g_jobs[QUEUE_DEPTH];
//---------------------------------- spdk end

/*
  Function Declarations for builtin shell commands:
 */
static int lsh_rgb2gray(char **args);
static int lsh_cd(char **args);
static int lsh_help(char **args);
static int lsh_exit(char **args);

static int lsh_num_builtins(void);
static int lsh_launch(char **args);
static int lsh_execute(char **args);
static char *lsh_read_line(void);
static char **lsh_split_line(char *line);
static void lsh_loop(void);
//---------------------------------- spdk start
// add static function here
static
void cs_compute_req_complete_cb(void *ctx, CS_STATUS status);
static
void* app_thread(void* data);
static 
int cs_shell_parse_arg(int ch, char *arg);
static 
void cs_shell_usage(void);
static 
void cs_shell_start(void *arg);
//---------------------------------- spdk end

/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
//---------------------------------- spdk start
// add shell builtin str here
  "r2g",
//---------------------------------- spdk end    
  "cd",
  "help",
  "exit",
};

int (*builtin_func[]) (char **) = {
//---------------------------------- spdk start
// add shell builtin func here
  &lsh_rgb2gray,
//---------------------------------- spdk end  
  &lsh_cd,
  &lsh_help,
  &lsh_exit,  
};

static int lsh_num_builtins(void) {
  return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/

/**
   @brief Bultin command: sample code for rgb2gray process
   @param args List of args.  
    args[0] is "r2g"
    args[1] is CSx path
    args[2] is bmp image files path
   @return Always returns 1, to continue executing.
 */
static int lsh_rgb2gray(char **args)
{
  int func_list_len = 0;
  int csx_name_len = 0;
  int cse_list_len = 0;
  char *csx_name = NULL;
  char *cse_name = NULL;
  char *func_list = NULL;
  char *cse_list = NULL;
  char *csx_path = NULL;
  char *scr_files_dir = NULL;
  const char* delim = ",";  
  CS_DEV_HANDLE dev_handle;
  CS_CSE_HANDLE cse_handle;
  CS_FUNCTION_ID func_id;
  CS_STATUS rc;

  //-------------------------------------------
  // check argument input

  // CSx device path
  if (args[1] == NULL) {
    fprintf(stderr, "expected CSx path argument!\n");
    goto __out;
  } else {
    csx_path = args[1];
  }

  // source image path
  if (args[2] == NULL) {
    fprintf(stderr, "expected input image file directory argument!\n");
    goto __out;
  } else {
    scr_files_dir = args[2];
  }

  //---------------------------------------------
  // 1. get func_list of specific CSx from path
  // 2. check requect func in func_list
  rc = csQueryFunctionList(csx_path, &func_list_len, NULL);
  if ((rc != CS_SUCCESS) || (func_list_len <= 0)) {
    fprintf(stderr, "get func list size fail!\n");
    goto __out;
  }

  func_list = calloc(1, func_list_len);
  if (func_list == NULL) {
    fprintf(stderr, "calloc func list buffer fail!\n");
    goto __out;
  }

  rc = csQueryFunctionList(csx_path, &func_list_len, func_list);
  if (rc != CS_SUCCESS) {
    fprintf(stderr, "get func list fail!\n");
    goto __out;
  }
  printf("func list:%s\n", func_list);

  // check request func in func list
  bool get_gunc = false;
  char *func_name = strtok(func_list, delim);
    
  while (func_name != NULL) {
      printf("func:%s\n", func_name);
      if (strcmp(REQ_FUNC_NAME, func_name) == 0) {
        get_gunc = true;
        break;
      }
      func_name = strtok(NULL, delim);		   
  }

  if (!get_gunc) {
    fprintf(stderr, "can't get desired func!\n");
    goto __out;    
  }

  //-----------------------------------------
  // get CSx name from path
  rc = csGetCSxFromPath(csx_path, &csx_name_len, NULL);
  if ((rc != CS_SUCCESS) || (csx_name_len <= 0)) {
    fprintf(stderr, "no CSx device found!\n");
    goto __out;
  }

  csx_name = calloc(1, csx_name_len);
  if (csx_name == NULL) {
    fprintf(stderr, "calloc CSx name buffer fail!\n");
    goto __out;
  }

  rc = csGetCSxFromPath(csx_path, &csx_name_len, csx_name);
  if (rc != CS_SUCCESS) {
    fprintf(stderr, "get CSx name fail!\n");
    goto __out;
  }
  printf("CSx name:%s\n", csx_name);

  //------------------------------------------
  // open CSx
  rc = csOpenCSx(csx_name, NULL, &dev_handle);
  if (rc != CS_SUCCESS) {
    fprintf(stderr, "open CSx:%s fail\n", csx_name);
    goto __out;
  }

  //-----------------------------------------
  // get CSE with request func in opened CSx
  rc = csQueryCSEList(REQ_FUNC_NAME, &cse_list_len, NULL);
  if ((rc != CS_SUCCESS) || (cse_list_len <= 0)) {
    fprintf(stderr, "no CSE device found!\n");
    goto __out;
  }

  cse_list = calloc(1, cse_list_len);
  if (cse_list == NULL) {
    fprintf(stderr, "calloc CSE list buffer fail!\n");
    goto __out;
  }

  rc = csQueryCSEList(REQ_FUNC_NAME, &cse_list_len, cse_list);
  if (rc != CS_SUCCESS) {
    fprintf(stderr, "get CSE list fail!\n");
    goto __out;
  }
  printf("CSE list:%s\n", cse_list);  

  // we may get more than one CSE that support request func, 
  // get first CSE for use
  cse_name = strtok(cse_list, delim);
  if (cse_name == NULL) {
    fprintf(stderr, "get CSE name from cse list fail!\n");
    goto __out;    
  }   

  //-----------------------------------------
  // open CSE
  rc = csOpenCSE(cse_name, NULL, &cse_handle);
  if (rc != CS_SUCCESS) {
    fprintf(stderr, "open CSE:%s fail\n", cse_name);
    goto __out;
  }

  //----------------------------------------
  // load func by name and get func id
  rc = csGetFunction(cse_handle, REQ_FUNC_NAME, NULL, &func_id);
  if (rc != CS_SUCCESS) {
    fprintf(stderr, "get func fail\n");
    goto __out;
  }

  //----------------------------------------
  // init rgb2gray jobs
  for (int qd = 0; qd < QUEUE_DEPTH; qd++) {
    rgb2gray_job *job = &g_r2g_jobs[qd];

    job->state = JOB_STATE_IDLE;
    for (int io_type = IO_TYPE_IN; io_type < NUM_IO_TYPE; io_type++) {

      rc = csAllocMem(dev_handle, ALLOC_AFDM_SIZE_BYTES, 0, 
                      &job->afdm_handle[io_type], &job->va[io_type]);
      if (rc != CS_SUCCESS) {
        fprintf(stderr, "csAllocMem fail\n");
        goto __out;
      }                 
    }
  }

  //---------------------------------------
  // start to queue rgb2tray jobs
  int in_file_idx = 0;
  int job_idx = 0;
  while (in_file_idx < NUM_IN_IMG_FILES) {
    rgb2gray_job *job = &g_r2g_jobs[job_idx];

    if (job->state == JOB_STATE_FAIL) {
      goto __out;
    }  

    // wait job complete
    while (job->state != JOB_STATE_IDLE);

    //--------------------------------------
    // [INPUT]
    // read file content to input buffer

    // dirname() returns the string up to, but not including, the final '/'
    char *in_img_dir = dirname(scr_files_dir);
    snprintf(job->in_img_path, PATH_MAX, "%s/img_%d.bmp", in_img_dir, in_file_idx);
    FILE *in_file = fopen(job->in_img_path, "r");
    if (in_file == NULL) {
      fprintf(stderr, "open file fail:%s\n", job->in_img_path);
      goto __out;
    }

    // get file size
    fseek(in_file, 0L, SEEK_END);
    job->in_img_size = ftell(in_file);
    fseek(in_file, 0L, SEEK_SET);

    // file read to input buffer
    bool rd_file_pass = false;
    if (job->in_img_size <= ALLOC_AFDM_SIZE_BYTES) {     
      int read_cnt = fread(job->va[IO_TYPE_IN], sizeof(char), job->in_img_size, in_file);
      if (read_cnt == job->in_img_size) {
        rd_file_pass = true;
      } else {
        fprintf(stderr, "fread fail:%s\n", job->in_img_path);
      }
    } else {
      fprintf(stderr, "too large file size:%s(%d bytes)\n", job->in_img_path, job->in_img_size);
    }
    fclose(in_file);

    if (rd_file_pass == false) {
      goto __out;
    }

    //----------------------------------
    // [COMPUTE]
    // queue compute job
    CsComputeRequest *cmpt_req = (CsComputeRequest *)calloc(1, (sizeof(CsComputeRequest) + (sizeof(CsComputeArg) * NUM_CS_CMPT_REQ_ARGS))); 
    if (!cmpt_req) {
      fprintf(stderr, "calloc compute request fail\n");
      goto __out;
    }

    cmpt_req->CSEHandle = cse_handle;  
    cmpt_req->FunctionId = func_id; 
    cmpt_req->NumArgs = NUM_CS_CMPT_REQ_ARGS; 
    CsComputeArg *cs_arg = &cmpt_req->Args[0]; 
    csHelperSetComputeArg(&cs_arg[0], CS_AFDM_TYPE, job->afdm_handle[IO_TYPE_IN], 0); 
    csHelperSetComputeArg(&cs_arg[1], CS_32BIT_VALUE_TYPE, job->in_img_size); 
    csHelperSetComputeArg(&cs_arg[2], CS_AFDM_TYPE, job->afdm_handle[IO_TYPE_OUT], 0); 
 
    job->state = JOB_STATE_COMPUTE;
    rc = csQueueComputeRequest(cmpt_req, (void *)job, cs_compute_req_complete_cb, NULL, NULL); 
    if (rc == CS_QUEUED) {
      job_idx = ((job_idx + 1) % QUEUE_DEPTH);
    } else {
      fprintf(stderr, "queue compute request fail\n");
      goto __out;
    }
  }

__out:
  //-------------------------------------
  // wait all jobs complete
  for (int qd = 0; qd < QUEUE_DEPTH; qd++) {
    rgb2gray_job *job = &g_r2g_jobs[qd];

    while (job->state != JOB_STATE_IDLE);
  }

  //-------------------------------------
  // free memory
  if (func_list != NULL) {
    free(func_list);
  }

  if (csx_name != NULL) {
    free(csx_name);
  }

  if (cse_list != NULL) {
    free(cse_list);
  }

  for (int qd = 0; qd < QUEUE_DEPTH; qd++) {
    rgb2gray_job *job = &g_r2g_jobs[qd];

    for (int io_type = IO_TYPE_IN; io_type < NUM_IO_TYPE; io_type++) {
      if (job->afdm_handle[io_type] != NULL) {
        csFreeMem(job->afdm_handle[io_type]);
      }
    }
  }

  return 1;
}

/**
   @brief Bultin command: change directory.
   @param args List of args.  args[0] is "cd".  args[1] is the directory.
   @return Always returns 1, to continue executing.
 */
static int lsh_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("lsh");
    }
  }
  return 1;
}

/**
   @brief Builtin command: print help.
   @param args List of args.  Not examined.
   @return Always returns 1, to continue executing.
 */
static int lsh_help(char **args)
{
  int i;
  printf("Stephen Brennan's LSH\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < lsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

/**
   @brief Builtin command: exit.
   @param args List of args.  Not examined.
   @return Always returns 0, to terminate execution.
 */
static int lsh_exit(char **args)
{
  // add spdk_app_stop() to exit spdk app
  spdk_app_stop(0);
  return 0;
}

/**
  @brief Launch a program and wait for it to terminate.
  @param args Null terminated list of arguments (including program).
  @return Always returns 1, to continue execution.
 */
static int lsh_launch(char **args)
{
  pid_t pid;
  int status;

  pid = fork();
  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
      perror("lsh");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // Error forking
    perror("lsh");
  } else {
    // Parent process
    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

/**
   @brief Execute shell built-in or launch program.
   @param args Null terminated list of arguments.
   @return 1 if the shell should continue running, 0 if it should terminate
 */
static int lsh_execute(char **args)
{
  int i;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < lsh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return lsh_launch(args);
}

/**
   @brief Read a line of input from stdin.
   @return The line from stdin.
 */
static char *lsh_read_line(void)
{
#ifdef LSH_USE_STD_GETLINE
  char *line = NULL;
  ssize_t bufsize = 0; // have getline allocate a buffer for us
  if (getline(&line, &bufsize, stdin) == -1) {
    if (feof(stdin)) {
      exit(EXIT_SUCCESS);  // We received an EOF
    } else  {
      perror("lsh: getline\n");
      exit(EXIT_FAILURE);
    }
  }
  return line;
#else
#define LSH_RL_BUFSIZE 1024
  int bufsize = LSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    // Read a character
    c = getchar();

    if (c == EOF) {
      exit(EXIT_SUCCESS);
    } else if (c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize) {
      bufsize += LSH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
#endif
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
/**
   @brief Split a line into tokens (very naively).
   @param line The line.
   @return Null-terminated array of tokens.
 */
static char **lsh_split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token, **tokens_backup;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += LSH_TOK_BUFSIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
		free(tokens_backup);
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

/**
   @brief Loop getting input and executing it.
 */
static void lsh_loop(void)
{
  char *line;
  char **args;
  int status;

  do {
    printf("> ");
    line = lsh_read_line();
    args = lsh_split_line(line);
    status = lsh_execute(args);

    free(line);
    free(args);
  } while (status);
}

// replace main() by spdk app
#if 0
/**
   @brief Main entry point.
   @param argc Argument count.
   @param argv Argument vector.
   @return status code
 */
int main(int argc, char **argv)
{
  // Load config files, if any.

  // Run command loop.
  lsh_loop();

  // Perform any shutdown/cleanup.

  return EXIT_SUCCESS;
}
#endif

static
void cs_compute_req_complete_cb(void *ctx, CS_STATUS status) {
  rgb2gray_job *job = (rgb2gray_job *)ctx;
  bool output_pass = false;

  if (status == CS_SUCCESS) {
    //--------------------------------------
    // [OUTPUT]
    // write file content to output buffer
    char path_buf[PATH_MAX];
    char out_img_path[PATH_MAX];
    char *img_name;
    char *img_dir;

    strncpy(path_buf, job->in_img_path, PATH_MAX);
    img_name = strrchr(path_buf, '/');
    *img_name = 0;
    img_name++;
    img_dir = path_buf;

    snprintf(out_img_path, PATH_MAX, "%s/%s%s", img_dir, "gray_", img_name);
    FILE *out_file = fopen(out_img_path, "w");
    if (out_file != NULL) {
      int write_cnt = fwrite(job->va[IO_TYPE_OUT], job->in_img_size, 1, out_file);
      if (write_cnt == job->in_img_size) {
        output_pass = true;
      } else {
        fprintf(stderr, "write file fail:%s\n", out_img_path);
      }
      fclose(out_file);
    } else {
      fprintf(stderr, "open file fail:%s\n", out_img_path);
    }   
  }

  if (output_pass == true) {
    job->state = JOB_STATE_IDLE;
  } else {
    job->state = JOB_STATE_FAIL;
  }
}
//---------------------------------- spdk start
// add spdk function here

/*
 * real application thread, not spdk_thread
 */
static
void* app_thread(void* data)
{
  lsh_loop();

  return 0;
}

/*
 * This function is called to parse the parameters that are specific to this application
 */
static 
int cs_shell_parse_arg(int ch, char *arg)
{
	return 0;
}

/*
 * Usage function for printing parameters that are specific to this application
 */
static 
void cs_shell_usage(void)
{
  return;
}

/*
 * Our initial event that kicks off everything from main().
 */
static 
void cs_shell_start(void *arg)
{
  // for cs init
  spdk_cs_mgr_init();

  // management for main spdk_thread
  g_main_thread = spdk_get_thread();

  // create application thread
  pthread_t app_thread_handle;
  pthread_create(&app_thread_handle, NULL, app_thread, NULL);
}

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
	if ((rc = spdk_app_parse_args(argc, argv, &opts, "", NULL, 
				                        cs_shell_parse_arg, cs_shell_usage)) != SPDK_APP_PARSE_ARGS_SUCCESS) {
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
  #if 0
	spdk_dma_free();
  #endif

	/* Gracefully close out all of the SPDK subsystems. */
	spdk_app_fini();
	return rc;
}

//---------------------------------- spdk end 
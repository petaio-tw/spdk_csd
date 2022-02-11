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
//---------------------------------- spdk end

//---------------------------------- spdk start
// add static variable here
static struct spdk_thread *g_main_thread;
//---------------------------------- spdk end

/*
  Function Declarations for builtin shell commands:
 */
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
  "cd",
  "help",
  "exit",
//---------------------------------- spdk start
// add shell builtin str here
//---------------------------------- spdk end  
};

int (*builtin_func[]) (char **) = {
  &lsh_cd,
  &lsh_help,
  &lsh_exit,
//---------------------------------- spdk start
// add shell builtin func here
//---------------------------------- spdk end  
};

static int lsh_num_builtins(void) {
  return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/

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
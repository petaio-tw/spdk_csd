#ifndef __SHELL_COMMON_H__
#define __SHELL_COMMON_H__

#include "common_type.h"
#include "spdk/shell.h"

#define SHELL_COMMON_MAX_COMMAND_LINE_BUF		(128)
#define SHELL_COMMON_MAX_PARAMETER			(8)

typedef struct _SHELL_COMMON_T
{
	UINT32	cmd_line_ready;
	UINT32	cmd_line_index;
	UINT32	num_parameters;

	char	cmd_line_string[SHELL_COMMON_MAX_COMMAND_LINE_BUF];
	char*	cmd_string;
	char*	help_string;
	char*	parameter_list[SHELL_COMMON_MAX_PARAMETER];
	const SHELL_CMD_TABLE_T* cmd_table;
} SHELL_COMMON_T;

extern SHELL_COMMON_T g_shell_common[1];


void
shell_common_prompt(void);

bool
shell_common_get_byte(void);

void
shell_common_run(void);

static inline const char*
shell_common_get_cmd_string(void)
{
	const char* cmd_string = g_shell_common->cmd_string;

	return cmd_string;
}

static inline UINT32
shell_common_get_num_parameters(void)
{
	UINT32 num_parameters = g_shell_common->num_parameters;

	return num_parameters;
}

static inline UINT32
shell_common_get_command_ready(void)
{
	UINT32 cmd_line_ready = g_shell_common->cmd_line_ready;

	return cmd_line_ready;
}

INT32
shell_common_get_parameter_int32(UINT32 parameters_index);

#endif	// __SHELL_COMMON_H__

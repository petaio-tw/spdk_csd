#ifndef __SHELL_H__
#define __SHELL_H__
#include <stdint.h>

typedef int32_t (*SHELL_CMD_FUNC_T)(void);

typedef struct _SHELL_CMD_TABLE_T
{
	const char*	cmd_string;
	const char*	help_string;
	const SHELL_CMD_FUNC_T cmd_func;
} SHELL_CMD_TABLE_T;

void
spdk_shell_init(const SHELL_CMD_TABLE_T* shell_cmd_table);
void
spdk_shell_task(void);
int32_t
spdk_shell_cmd_print_list(void);

#endif	// __SHELL_H__

#ifndef __SHELL_H__
#define __SHELL_H__
#include <stdbool.h>
#include <stdint.h>

typedef int32_t (*SHELL_CMD_FUNC_T)(void);

typedef struct _SHELL_CMD_TABLE_T
{
	const char*	cmd_string;
	const char*	help_string;
	const SHELL_CMD_FUNC_T cmd_func;
} SHELL_CMD_TABLE_T;

typedef enum _SHELL_ERROR_CODE_E
{
	SHELL_ERROR_CODE_SUCCESS				= 0x0,
	SHELL_ERROR_CODE_FAIL					= 0x1,	
	SHELL_ERROR_CODE_WRONG_NUM_PARAMETERS			= 0x2,
	SHELL_ERROR_CODE_INVALID_PARAMETER			= 0x3,
} SHELL_ERROR_CODE_E;

void
spdk_shell_init(const SHELL_CMD_TABLE_T* shell_cmd_table);
void
spdk_shell_task(void);
int32_t
spdk_shell_cmd_print_list(void);
bool
spdk_shell_common_get_parameters_uint32(uint32_t* parameter, uint32_t num_parameters);

#endif	// __SHELL_H__

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include "string_util.h"
#include "shell_common.h"


SHELL_COMMON_T g_shell_common[1];

void
shell_common_prompt(void)
{
	printf("pid:%d> ", getpid());	
}

bool
shell_common_get_byte(void)
{
	UINT32 cmd_line_ready = g_shell_common->cmd_line_ready;
	if (cmd_line_ready == 1)
	{
		return true;
	}

	int letter = getchar();

	if (letter == 0x7F || letter == '\b')
	{
		UINT32 cmd_line_index = g_shell_common->cmd_line_index;
		if (cmd_line_index > 0)
		{
			g_shell_common->cmd_line_index = cmd_line_index - 1;
		}
	} else {
		switch (letter)
		{
			case '\r':
			case '\n':
			{
				UINT32 cmd_line_index = g_shell_common->cmd_line_index;
				if (cmd_line_index > 0)
				{
					g_shell_common->cmd_line_string[cmd_line_index] = NULL;
					g_shell_common->cmd_line_index = cmd_line_index + 1;
					g_shell_common->cmd_line_ready = 1;
				} else {
					shell_common_prompt();
				}

				break;
			}
			case EOF:
				return false;
			default:
			{
				UINT32 cmd_line_index = g_shell_common->cmd_line_index;
				if (cmd_line_index < (SHELL_COMMON_MAX_COMMAND_LINE_BUF - 1))
				{
					g_shell_common->cmd_line_index = cmd_line_index + 1;
					g_shell_common->cmd_line_string[cmd_line_index] = (char)letter;
				}
				else
				{
					printf("\n[Shell] buffer overflow\n");

					g_shell_common->cmd_line_string[0] = NULL;
					g_shell_common->num_parameters = 0;
					g_shell_common->cmd_line_index = 0;
					g_shell_common->cmd_line_ready = 0;

					shell_common_prompt();
				}
			}
		}
	}

	return true;
}


static void
shell_common_parse(void)
{
	// convert command line to lower case
	char* cmd_line_string = g_shell_common->cmd_line_string;

#if 0
	string_to_lower(cmd_line_string);
#endif	

	// get shell command
	UINT32 whitespace = string_get_whitespace(cmd_line_string);
	cmd_line_string += whitespace;
	g_shell_common->cmd_string = cmd_line_string;

	UINT32 validspace = string_get_validspace(cmd_line_string);
	cmd_line_string += validspace;

	UINT32 num_parameters = 0;
	while (1)
	{
		whitespace = string_get_whitespace(cmd_line_string);
		cmd_line_string += whitespace;

		UINT32 validspace = string_get_validspace(cmd_line_string);
		if (validspace == 0)
		{
			break;
		}

		if (num_parameters >= SHELL_COMMON_MAX_PARAMETER)
		{
			printf("[Shell] Too many parameters\n");
			break;
		}

		g_shell_common->parameter_list[num_parameters] = cmd_line_string;

		cmd_line_string += validspace;

		num_parameters++;
	}

	g_shell_common->num_parameters = num_parameters;

	// convert whitespace to NULL
	cmd_line_string = g_shell_common->cmd_line_string;
	string_convert_whitespace_to_null(cmd_line_string);
}

static void
shell_common_execute(const SHELL_CMD_FUNC_T shell_cmd_func)
{
	INT32 result = (*shell_cmd_func)();

	if (result != SHELL_ERROR_CODE_SUCCESS)
	{
		switch (result)
		{
			case SHELL_ERROR_CODE_WRONG_NUM_PARAMETERS:
			{
				printf("[Shell] Wrong number of parameters\n");
				printf("       => %s %s\n",g_shell_common->cmd_string, g_shell_common->help_string);
				break;
			}
			case SHELL_ERROR_CODE_INVALID_PARAMETER:
			{
				printf("[Shell] Invalid parameters\n");

				break;
			}
			case SHELL_ERROR_CODE_FAIL:
			default:
			{
				printf("[Shell] Fail to run Command\n");
			}
		}
	}
}

void
shell_common_run(void)
{
	UINT32 cmd_line_ready = g_shell_common->cmd_line_ready;
	if (cmd_line_ready == 0)
	{
		return;
	}

	shell_common_parse();

	const char* cmd_string = g_shell_common->cmd_string;
	const SHELL_CMD_TABLE_T* shell_cmd_table = g_shell_common->cmd_table;

	while (1)
	{
		const char* table_cmd_string = shell_cmd_table->cmd_string;
		if (table_cmd_string == NULL)
		{
			printf("[Shell] Invalid command\n");
			break;
		}

		UINT32 string_is_same = string_compare(table_cmd_string, cmd_string);
		if (string_is_same == 1)
		{
			const SHELL_CMD_FUNC_T shell_cmd_func = shell_cmd_table->cmd_func;

			g_shell_common->help_string = (char*)shell_cmd_table->help_string;

			shell_common_execute(shell_cmd_func);
			break;
		}

		shell_cmd_table++;
	}

	g_shell_common->cmd_line_string[0] = NULL;
	g_shell_common->num_parameters = 0;
	g_shell_common->cmd_line_index = 0;
	g_shell_common->cmd_line_ready = 0;

	shell_common_prompt();
}

INT32
shell_common_get_parameter_int32(UINT32 parameters_index)
{
	UINT32 num_parameters = g_shell_common->num_parameters;
	if (parameters_index >= num_parameters)
	{
		printf("[Shell] Enter a parameter - %d\n", parameters_index);

		return 0;
	}

	const char* parameter_string = g_shell_common->parameter_list[parameters_index];

	INT32 number = string_to_int32(parameter_string);

	return number;
}

//-----------------------------------------------
// export functions
//
uint32_t
spdk_shell_common_get_parameter_uint32(uint32_t parameters_index)
{
	uint32_t num_parameters = g_shell_common->num_parameters;
	if (parameters_index >= num_parameters)
	{
		printf("[Shell] Enter a parameter - %d\n", parameters_index);

		return 0;
	}

	const char* parameter_string = g_shell_common->parameter_list[parameters_index];

	uint32_t number = string_to_uint32(parameter_string);

	return number;
}

bool
spdk_shell_common_get_parameters_uint32(uint32_t* parameter, uint32_t num_parameters)
{
	if (shell_common_get_num_parameters() != num_parameters) {
		return FALSE;
	}

	for (uint32_t index = 0; index < num_parameters; index++) {
		parameter[index] = spdk_shell_common_get_parameter_uint32(index);
	}

	return TRUE;
}

void
spdk_shell_init(const SHELL_CMD_TABLE_T* shell_cmd_table)
{
	memset(g_shell_common, 0, sizeof(SHELL_COMMON_T));

	g_shell_common->cmd_table = shell_cmd_table;
}

const char*
spdk_shell_common_get_parameter_string(uint32_t parameters_index)
{
	UINT32 num_parameters = g_shell_common->num_parameters;
	if (parameters_index >= num_parameters)
	{
		printf("[Shell] Enter a parameter - %d\n", parameters_index);

		return NULL;
	}

	const char* parameter_string = g_shell_common->parameter_list[parameters_index];

	return parameter_string;
}

void
spdk_shell_task(void)
{
	while (1)
	{
		if (shell_common_get_byte() == false) {
			exit(0);
		}
		shell_common_run();
	}
}

int32_t
spdk_shell_cmd_print_list(void)
{
	// You should the number of parameters.
	UINT32 num_parameters = shell_common_get_num_parameters();
	if (num_parameters != 0)
	{
		return -1;
	}

	const SHELL_CMD_TABLE_T* shell_cmd_table = g_shell_common->cmd_table;

	UINT32 index = 0;
	while (1)
	{
		if (shell_cmd_table->cmd_string == NULL)
		{
			break;
		}

		printf("%d: %-14s - %s\n", index, shell_cmd_table->cmd_string, shell_cmd_table->help_string);

		index++;
		shell_cmd_table++;
	}

	return 0;
}
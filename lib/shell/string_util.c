
#include "string_util.h"

void
string_to_lower(char* string)
{
	while (1)
	{
		char letter = *string;
		if (letter == NULL)
		{
			break;
		}

		if ('A' <= letter && letter <= 'Z')
		{
			*string = letter - 'A' + 'a';
		}

		string++;
	}
}

void
string_convert_whitespace_to_null(char* string)
{
	while (1)
	{
		char letter = *string;
		if (letter == NULL)
		{
			break;
		}

		if (letter == ' ' || letter == '\t')
		{
			*string = NULL;
		}

		string++;
	}
}

UINT32
string_hex_to_uint32(const char* string)
{
	UINT32 result = 0;
	while (1)
	{
		char letter = *string;
		UINT32 number;
		UINT32 is_digit = string_letter_is_digit(letter);
		if (is_digit == 1)
		{
			number = (UINT32)(letter - '0');
		}
		else
		{
			if ('a' <= letter && letter <= 'f')
			{
				number = ((UINT32)(letter - 'a')) + 10;
			}
			else
			{
				if ('A' <= letter && letter <= 'F')
				{
					number = ((UINT32)(letter - 'A')) + 10;
				}
				else
				{
					break;
				}
			}
		}

		result = (result * 16) + number;

		string++;
	}

	return result;
}

INT32
string_dec_to_int32(const char* string)
{
	UINT32 sign_flag = 0;
	if (*string == '-')
	{
		string++;
		sign_flag = 1;
	}

	INT32 result = 0;
	while (1)
	{
		char letter = *string;
		UINT32 is_digit = string_letter_is_digit(letter);
		if (is_digit != 1)
		{
			break;
		}

		INT32 number = (INT32)(letter - '0');

		result = (result * 10) + number;

		string++;
	}

	if (sign_flag == 0)
	{
		return result;
	}
	else
	{
		return (result * (-1));
	}
}


UINT32
string_dec_to_uint32(const char* string)
{
	UINT32 result = 0;
	while (1)
	{
		char letter = *string;
		UINT32 is_digit = string_letter_is_digit(letter);
		if (is_digit != 1)
		{
			break;
		}

		UINT32 number = (INT32)(letter - '0');

		result = (result * 10) + number;

		string++;
	}

	return result;
}

INT32
string_to_int32(const char* string)
{
	INT32 result;

	if (string[0] == '0' && string[1] == 'x')
	{
		// Hex
		string += 2;
		result = (INT32)string_hex_to_uint32(string);
	}
	else
	{
		// Decimal
		result = string_dec_to_int32(string);

	}

	return result;
}

UINT32
string_to_uint32(const char* string)
{
	UINT32 result;

	if (string[0] == '0' && string[1] == 'x')
	{
		// Hex
		string += 2;
		result = string_hex_to_uint32(string);
	}
	else
	{
		// Decimal
		result = string_dec_to_uint32(string);

	}

	return result;
}


UINT32
string_compare(const char* string0, const char* string1)
{
	char letter0;
	char letter1;

	while (1)
	{
		letter0 = *string0;
		letter1 = *string1;

		if (letter0 == NULL)
		{
			break;
		}

		if (letter0 != letter1)
		{
			return 0;
		}

		string0++;
		string1++;
	}

	if (letter1 == NULL)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

UINT32
string_get_whitespace(const char* string)
{
	UINT32 count = 0;

	// count white space
	while (1)
	{
		char letter = *string;
		if (letter == ' ' || letter == '\t')
		{
			count++;
			string++;
		}
		else
		{
			break;
		}
	}

	return count;
}

UINT32
string_get_validspace(const char* string)
{
	UINT32 count = 0;

	// count valid space
	while (1)
	{
		char letter = *string;

		if (letter == NULL)
		{
			break;
		}

		if (letter == ' ' || letter == '\t')
		{
			break;
		}
		else
		{
			count++;
			string++;
		}
	}

	return count;
}


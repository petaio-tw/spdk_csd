#ifndef __STRING_UTIL_H__
#define __STRING_UTIL_H__

#include "common_type.h"

static inline UINT32
string_letter_is_digit(char letter)
{
	if ('0' <= letter && letter <= '9')
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

static inline char
string_letter_to_lower(char letter)
{
	if ('A' <= letter && letter <= 'Z')
	{
		letter = letter - 'A' + 'a';
	}

	return letter;
}

void
string_to_lower(char* string);

void
string_convert_whitespace_to_null(char* string);

UINT32
string_hex_to_uint32(const char* string);

INT32
string_dec_to_int32(const char* string);
UINT32
string_dec_to_uint32(const char* string);

INT32
string_to_int32(const char* string);
UINT32
string_to_uint32(const char* string);

UINT32
string_compare(const char* string0, const char* string1);

UINT32
string_get_whitespace(const char* string);

UINT32
string_get_validspace(const char* string);

#endif	// __STRING_UTIL_H__


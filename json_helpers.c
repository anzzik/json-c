#include "json_helpers.h"

int json_is_num(char c)
{
	if (c >= 48 && c <= 57)
		return 1;

	return 0;
}

int json_is_object_start(char c)
{
	if (c == '{' || c == '[')
		return 1;

	return 0;
}

int json_is_ws(char c)
{
	if (c == ' ' || c == '\n' || c == '\t')
		return 1;

	return 0;
}



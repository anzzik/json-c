#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "json.h"

int main(void)
{
	char *json_str;

	json_str = json_read_file("testdata.json");
	if (!json_str)
	{
		fprintf(stderr, "File reading failed\n");

		return -1;
	}

	JSONObject_t *obj = json_start(json_str);
	if (!obj)
	{
		fprintf(stderr, "Failed to parse JSON string\n");

		return -1;
	}

	json_print(obj);

	json_free(obj);

	return 0;
}


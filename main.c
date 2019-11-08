#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "json.h"

int main(void)
{
	char str[] = "{ \"some key key\": [1,2], \"someotherkey\": \"some str\", \"obj_key\": { \"key\": 1, \"key2\": { \"inner_key1\": 32.5 } } }";
	char str2[] = "[1,2,{\"a\": 1}]";

	JSONObject_t *obj = json_start(str);
	if (!obj)
	{
		fprintf(stderr, "Failed to parse JSON string\n");

		return -1;
	}

	JSONObject_t *obj2 = json_start(str2);
	if (!obj)
	{
		fprintf(stderr, "Failed to parse JSON string\n");

		return -1;
	}

	json_print(obj);
	printf("\n\n");
	json_print(obj2);

	return 0;
}



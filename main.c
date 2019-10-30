#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "json.h"


int main(void)
{
	char *str = "{ \"some key key\" : 1, \"someotherkey\": \"some str\", \"obj_key\": { \"key\": 1, \"key2\": { \"inner_key1\": 32.5 } } }";

	int off;
	int r;

	JSONObject_t *obj = json_object_new(0);

	r = json_parse_object(str, obj, &off);
	if (r < 0)
	{
		fprintf(stderr, "JSON string decoding failed.\n");

		return -1;
	}

	json_object_print(obj);

	return 0;
}



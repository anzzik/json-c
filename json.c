#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json.h"
#include "json_lib.h"
#include "json_hndl.h"

JSONObject_t *json_start(char *json_str)
{
	JSONStateHandler_t *handlers = json_hndl_get_handlers();
	JSONObject_t *json = 0;
	int i = 0;
	int r;

	while (json_str[i] != '\0')
	{
		if (json_is_ws(json_str[i]))
		{
			i++;

			continue;
		}

		if (!json_is_object_start(json_str[i]))
		{
			fprintf(stderr, "Starting parenthisis not found\n");

			return 0;
		}

		if (json)
		{
			fprintf(stderr, "The string contains multiple separate objects, this is not supported\n");

			return 0;
		}

		json = json_object_new(0);

		if (json_str[i] == '{')
		{
			r = json_parse_object(json_str + i, json, &i, handlers);
			if (r)
			{
				fprintf(stderr, "json_parse_object() failed\n");
				return 0;
			}
		}

		if (json_str[i] == '[')
		{
			json->is_array = 1;

			r = json_parse_array(json_str + i, json, &i, handlers);
			if (r)
			{
				fprintf(stderr, "json_parse_array() failed\n");
				return 0;
			}
		}
	}

	return json;
}

char *json_read_file(char *filename)
{
	FILE *fp;
	int sz;
	int read;
	char *buf;
       
	fp = fopen(filename, "r");
	if (!fp)
	{
		fprintf(stderr, "Opening file %s failed\n", filename);
		return 0;
	}

	fseek(fp, 0, SEEK_END);
	sz = ftell(fp);
	rewind(fp);

	buf = calloc(sz + 1, 1);

	read = fread(buf, 1, sz, fp);
	if (read != sz)
	{
		fprintf(stderr, "Failed to read the file completely\n");
		fclose(fp);

		return 0;
	}

	fclose(fp);

	return buf;
}

void json_print(JSONObject_t *json_obj)
{
	if (!json_obj->first_kvp)
		return;

	if (json_obj->is_array)
		json_object_array_print(json_obj);
	else
		json_object_print(json_obj);
}

void json_free(JSONObject_t *json_obj)
{
	json_object_free(json_obj);
}



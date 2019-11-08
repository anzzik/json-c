#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json_lib.h"
#include "json_helpers.h"

JSONObject_t *json_object_new(JSONObject_t *parent)
{
	JSONObject_t *obj = 0;

	obj	   = malloc(sizeof(JSONObject_t));
	obj->data  = 0;
	obj->depth = 0;
	obj->ready = 0;
	obj->is_array = 0;

	if (parent)
		obj->depth = parent->depth + 1;

	return obj;
}

JSONData_t *json_data_new()
{
	JSONData_t *d = 0;
	
	d	  = malloc(sizeof(JSONData_t));
	d->type   = UNDEFINED;
	d->key    = 0;
	d->val    = 0;
	d->next   = 0;

	return d;
}

JSONDataArray_t *json_data_array_new()
{
	JSONDataArray_t *da = 0;
	
	da	   = malloc(sizeof(JSONDataArray_t));
	da->type   = UNDEFINED;
	da->data   = 0;
	da->next   = 0;

	return da;
}

int json_parse_object(char *json_str, JSONObject_t *output_obj, int *offset, JSONStateHandler_t *state_handlers)
{
	JSONObject_t *json = output_obj;
	JSONParseState_t state = INITIAL;
	int len;
	int i;
	int r;

	len = strlen(json_str);
	i = 0;
	while (!json->ready && json_str[i] != '\0')
	{
		if (json_is_ws(json_str[i]))
		{
			i++;
			continue;
		}

		for (int j = 0; j < STATE_COUNT; j++)
		{
			if (j != state)
				continue;

			r = state_handlers[state].func(json_str + i, json, &state);
			if (r < 0)
				return -1;

			break;
		}


		i += r;
		if (i > len)
			return -1;
	}

	*offset = i;

	return 0;
}

int json_parse_array(char *json_str, JSONObject_t *output_obj, int *offset, JSONStateHandler_t *state_handlers)
{
	JSONObject_t *json = output_obj;
	JSONParseState_t state = ARRAY_INITIAL;
	int i = 0;
	int r;
	int len = strlen(json_str);

	while (json_str[i] != '\0')
	{
		if (json_is_ws(json_str[i]))
		{
			i++;
			continue;
		}

		break;
	}

	if (json_str[i] != '[')
	{
		fprintf(stderr, "Expected array opening, got %c instead\n", json_str[i]);

		return -1;
	}

	while (json_str[i] != '\0')
	{
		if (json_is_ws(json_str[i]))
		{
			i++;
			continue;
		}

		if (json_str[i] == ']')
		{
			i++;
			break;
		}

		for (int j = 0; j < STATE_COUNT; j++)
		{
			if (j != state)
				continue;

			r = state_handlers[state].func(json_str + i, json, &state);
			if (r < 0)
				return -1;

			break;
		}

		i += r;
		if (i > len)
		{
			fprintf(stderr, "String index got too big in array parsing\n");
			return -1;
		}
	}

	*offset = i;

	return 0;
}

JSONData_t *json_get_top_data(JSONObject_t *json)
{
	JSONData_t *d;

	d = json->data;
	if (!d)
		return 0;

	while (d->next)
		d = d->next;

	return d;
}

char *json_data_str_new(char *buf)
{
	int  sz;
	char *key_buf;

	sz = strlen(buf);

	if (sz > LITERAL_MAX_SIZE)
	{
		fprintf(stderr, "Requesting too big string size: %db (%s)\n", sz, buf);
		return 0;
	}

	key_buf = calloc(sz + 1, 1);
	memcpy(key_buf, buf, sz);

	return key_buf;
}

int json_data_set_val(JSONData_t *data, void *val)
{
	switch (data->type)
	{
		case NUMBER:
		case STRING:
			data->val = calloc(strlen((char*)val) + 1, 1);
			strncpy(data->val, (char*)val, strlen((char*)val) + 1);
			break;

		case OBJECT:
			data->val = val;
			break;

		default:
			fprintf(stderr, "Cannot set value for JSONData, check the type\n");
			return -1;
	}

	return 0;
}

int json_object_push(JSONObject_t *json_obj, JSONData_t *json_data)
{
	JSONData_t *tmp;

	if (!json_obj->data)
	{
		json_obj->data = json_data;

		return 0;
	}

	tmp = json_obj->data;
	while (tmp->next)
		tmp = tmp->next;

	tmp->next = json_data;
	json_data->next = 0;

	return 0;
}

int json_data_array_push(JSONObject_t *json_array_obj, JSONDataArray_t *json_array_data)
{
	JSONDataArray_t *da;

	if (!json_array_obj->data)
	{
		fprintf(stderr, "JSON object not initialized properly for array push\n");

		return -1;
	}

	if (json_array_obj->data->type != ARRAY)
	{
		fprintf(stderr, "JSON object's data is not ARRAY type\n");

		return -1;
	}

	da = json_array_obj->data->val;
	if (!da)
	{
		json_array_obj->data->val = json_array_data;

		return 0;
	}

	while (da->next)
		da = da->next;

	da->next = json_array_data;
	json_array_data->next = 0;

	return 0;
}

int json_parse_key(char *str, char *output, int *offset)
{
	int i = 0;
	int ob_c = 0;
	int waiting_end = 0;
	int in_quote = 0;
	int esc = 0;
	char c;

	while (str[i] != '\0')
	{
		c = str[i++];

		if (in_quote)
		{
			if (c == '\\')
			{
				esc = 1;
				continue;
			}

			if (!esc && c == '"')
				break;

			output[ob_c++] = c;
			if (esc)
				esc = 0;

			continue;
		}

		if (c == '"')
		{
			if (waiting_end)
			{
				fprintf(stderr, "Invalid key\n");

				return -1;
			}

			in_quote = 1;

			continue;
		}

		return -1;
	}

	output[ob_c] = '\0';
	*offset = i;

	return 0;
}

int json_parse_value(char *str, char *output, JSONDataType_t *output_type, int *offset)
{
	int i = 0;
	int in_quote = 0;
	int ob_c = 0;
	int esc = 0;
	char c;
	JSONDataType_t type = UNDEFINED;

	while (str[i] != '\0')
	{
		c = str[i++];

		if (ob_c == LITERAL_MAX_VALUE - 1)
		{
			fprintf(stderr, "Value is too long\n");

			return -1;
		}

		if (!in_quote && json_is_ws(c))
			continue;

		if (in_quote)
		{
			if (c == '\\')
			{
				esc = 1;
				continue;
			}

			if (!esc && c == '"')
				break;

			output[ob_c++] = c;

			if (esc)
				esc = 0;

			continue;
		}

		if (c == '"')
		{
			type = STRING;
			in_quote = 1;

			continue;
		}

		if (type == UNDEFINED)
		{
			if (c == '{')
			{
				type = OBJECT;

				break;
			}

			if (c == '[')
			{
				type = ARRAY;

				break;
			}

			if (!json_is_num(c))
				return -1;

			type = NUMBER;
		}

		if (type == NUMBER)
		{
			if (!json_is_num(c))
			{
				if (c != '.')
				{
					i--;
					break;
				}
			}

			output[ob_c++] = c;

			continue;
		}
	}

	output[ob_c] = '\0';
	*offset = i;
	*output_type = type;

	return 0;
}

void json_object_print(JSONObject_t *json_obj)
{
	if (!json_obj->data)
		return;

	JSONData_t *d = json_obj->data;

	printf("{\n");
	while (d)
	{
		for (int i = 0; i < json_obj->depth + 1; i++)
			printf("\t");

		printf("\"%s\": ", d->key);

		if (d->type == OBJECT)
		{
			json_object_print(d->val);
		}
		else if (d->type == ARRAY)
		{
			fprintf(stderr, "calling array print\n");
			json_object_array_print(d->val);
			printf(",");
		}
		else
		{
			if (d->type == STRING)
				printf("\"%s\",\n", (char*)d->val);
			else
				printf("%s,\n", (char*)d->val);
		}

		d = d->next;
	}

	for (int i = 0; i < json_obj->depth; i++)
		printf("\t");

	printf("}");
	if (json_obj->depth > 0)
		printf(",");

	printf("\n");
}

void json_object_array_print(JSONObject_t *json_obj)
{
	if (!json_obj->data)
		return;

	JSONDataArray_t *da;

	da = json_obj->data->val;

	printf("[");
	while (da)
	{
		if (da->type == OBJECT)
		{
			json_object_print(da->data);
		}
		else if (da->type == ARRAY)
		{
			json_object_array_print(da->data);
			printf(",");
		}
		else
		{
			if (da->type == STRING)
				printf("\"%s\"", (char*)da->data);
			else
				printf("%s", (char*)da->data);
		}

		da = da->next;
		if (da)
			printf(",");
	}

	printf("],\n");
}


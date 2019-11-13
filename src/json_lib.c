#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json_lib.h"
#include "json_helpers.h"

JSONObject_t *json_object_new(JSONObject_t *parent)
{
	JSONObject_t *obj = 0;

	obj	        = malloc(sizeof(JSONObject_t));
	obj->first_kvp  = 0;
	obj->depth      = 0;
	obj->ready      = 0;
	obj->is_array   = 0;

	if (parent)
		obj->depth = parent->depth + 1;

	return obj;
}

void json_object_free(JSONObject_t *json_obj)
{
	JSONKeyValuePair_t *kvp;

	if (!json_obj)
		return;

	while (json_obj->first_kvp)
		json_kvp_pop(json_obj, 1);

	free(json_obj);
}

JSONKeyValuePair_t *json_kvp_new()
{
	JSONKeyValuePair_t *kvp = 0;
	
	kvp	    = malloc(sizeof(JSONKeyValuePair_t));
	kvp->type   = UNDEFINED;
	kvp->key    = 0;
	kvp->val    = 0;
	kvp->next   = 0;

	return kvp;
}

void json_kvp_free(JSONKeyValuePair_t *kvp)
{
	int r;

	if (!kvp)
		return;

	json_str_free(kvp->key);

	if (kvp->type == STRING || kvp->type == NUMBER)
		json_str_free(kvp->val);

	if (kvp->type == OBJECT)
		json_object_free(kvp->val);

	if (kvp->type == ARRAY)
	{
		while (kvp->val)
		{
			r = json_data_array_pop(kvp, 1);
			if (r < 0)
				fprintf(stderr, "Error in json_data_array_pop()");
		}
	}

	free(kvp);
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

void json_data_array_free(JSONDataArray_t* da)
{
	if (!da->data)
		return;

	if (da->type == STRING || da->type == NUMBER)
		json_str_free(da->data);

	if (da->type == OBJECT)
		json_object_free(da->data);

	if (da->type == ARRAY)
	{
		JSONObject_t *array_obj = da->data;
		while (array_obj->first_kvp->val)
			json_data_array_pop(array_obj->first_kvp, 1);
	}

	free(da);
}

int json_parse_object(char *json_str, JSONObject_t *output_obj, int *offset, JSONStateHandler_t *state_handlers)
{
	JSONObject_t *json = output_obj;
	JSONParseState_t state = INITIAL;
	int i = 0;
	int len = strlen(json_str);
	int r;

	while (!json->ready && json_str[i] != '\0')
	{
		if (json_is_ws(json_str[i]))
		{
			i++;
			continue;
		}

		for (int s = 0; s < STATE_COUNT; s++)
		{
			if (s != state)
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
	int len = strlen(json_str);
	int i = 0;
	int r;

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

JSONKeyValuePair_t *json_kvp_top(JSONObject_t *json)
{
	JSONKeyValuePair_t *kvp;

	kvp = json->first_kvp;
	if (!kvp)
		return 0;

	while (kvp->next)
		kvp = kvp->next;

	return kvp;
}

int json_kvp_list_len(JSONObject_t *json)
{
	JSONKeyValuePair_t **kvp_mem;
	int c = 0;

	kvp_mem = &json->first_kvp;
	if (!(*kvp_mem))
		return 0;

	while (*kvp_mem)
	{
		kvp_mem = &((*kvp_mem)->next);
		c++;
	}

	return c;
}

int json_kvp_pop(JSONObject_t *json, int do_free)
{
	JSONKeyValuePair_t **kvp_mem;

	kvp_mem = &json->first_kvp;
	if (!(*kvp_mem))
		return 0;

	while ((*kvp_mem)->next)
		kvp_mem = &((*kvp_mem)->next);

	if (do_free)
		json_kvp_free(*kvp_mem);

	*kvp_mem = 0;

	return 0;
}

char *json_str_new(char *buf)
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

void json_str_free(char *buf)
{
	if (!buf)
		return;

	free(buf);
}

int json_kvp_set_val(JSONKeyValuePair_t *kvp, void *val)
{
	switch (kvp->type)
	{
		case NUMBER:
		case STRING:
			kvp->val = calloc(strlen((char*)val) + 1, 1);
			strncpy(kvp->val, (char*)val, strlen((char*)val) + 1);
			break;

		case OBJECT:
			kvp->val = val;
			break;

		default:
			fprintf(stderr, "Cannot set value for JSONData, check the type\n");
			return -1;
	}

	return 0;
}

int json_object_push(JSONObject_t *json_obj, JSONKeyValuePair_t *kvp)
{
	JSONKeyValuePair_t *kvp_it;

	if (!json_obj->first_kvp)
	{
		json_obj->first_kvp = kvp;

		return 0;
	}

	kvp_it = json_obj->first_kvp;
	while (kvp_it->next)
		kvp_it = kvp_it->next;

	kvp_it->next = kvp;
	kvp->next = 0;

	return 0;
}

int json_data_array_push(JSONObject_t *json_array_obj, JSONDataArray_t *json_array_data)
{
	JSONDataArray_t *da;

	da = json_array_obj->first_kvp->val;
	if (!da)
	{
		json_array_obj->first_kvp->val = json_array_data;

		return 0;
	}

	while (da->next)
		da = da->next;

	da->next = json_array_data;
	json_array_data->next = 0;

	return 0;
}

int json_data_array_list_len(JSONKeyValuePair_t *kvp)
{
	JSONDataArray_t **da_mem;
	int c = 0;

	da_mem = (JSONDataArray_t**)&kvp->val;
	while (*da_mem)
	{
		da_mem = &(*da_mem)->next;
		c++;
	}

	return c;
}

int json_data_array_pop(JSONKeyValuePair_t *kvp, int do_free)
{
	JSONDataArray_t **da_mem;

	if (!kvp)
		return -1;

	if (kvp->type != ARRAY)
	{
		fprintf(stderr, "JSON object's data is not ARRAY type\n");

		return -1;
	}

	da_mem = (JSONDataArray_t**)&kvp->val;
	if (!(*da_mem))
		return 0;

	while ((*da_mem)->next)
		da_mem = &(*da_mem)->next;

	if (do_free)
		json_data_array_free(*da_mem);

	*da_mem = 0;

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

		if (ob_c == LITERAL_MAX_SIZE - 1)
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
	if (!json_obj->first_kvp)
		return;

	JSONKeyValuePair_t *kvp = json_obj->first_kvp;

	printf("{\n");
	while (kvp)
	{
		for (int i = 0; i < json_obj->depth + 1; i++)
			printf("\t");

		printf("\"%s\": ", kvp->key);

		if (kvp->type == OBJECT)
		{
			json_object_print(kvp->val);
		}
		else if (kvp->type == ARRAY)
		{
			fprintf(stderr, "calling array print\n");
			json_object_array_print(kvp->val);
			printf(",");
		}
		else
		{
			if (kvp->type == STRING)
				printf("\"%s\",\n", (char*)kvp->val);
			else
				printf("%s,\n", (char*)kvp->val);
		}

		kvp = kvp->next;
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
	if (!json_obj->first_kvp)
		return;

	JSONDataArray_t *da;

	da = json_obj->first_kvp->val;

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


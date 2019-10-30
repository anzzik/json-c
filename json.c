#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json.h"

#define KEY_MAX_SIZE 64 
#define LITERAL_MAX_SIZE 1024 

static int json_hndl_initial_cb(char *json_str, void *u_ptr, JSONParseState_t *state)
{
	char c;
	int off;

	off = 0;
	c = json_str[0];

	while (json_is_ws(json_str[off]))
		off++;

	if (json_str[off] != '{')
	{
		fprintf(stderr, "JSON object is missing the opening parenthisis\n");

		return -1;
	}

	off++;

	*state = WAITING_KEY_OR_END;

	return off;
}

static int json_hndl_wkoe_cb(char *json_str, void *u_ptr, JSONParseState_t *state)
{
	JSONObject_t *json;
	JSONData_t   *data;
	char c;
	int  off;
	int  r;
	char buffer[KEY_MAX_SIZE] = {'\0'};

	c = json_str[0];

	if (c == '}')
	{
		*state = READY;

		return 1;
	}

	r = json_parse_key(json_str, buffer, &off);
	if (r < 0)
	{
		fprintf(stderr, "JSON key parsing failed at '%s'\n", json_str);

		return -1;
	}

	json = u_ptr;

	data      = json_data_new();
	data->key = json_data_str_new(buffer);

	json_object_push(json, data);

	*state = WAITING_VALUE;

	return off;
}

static int json_hndl_wv_cb(char *json_str, void *u_ptr, JSONParseState_t *state)
{
	JSONObject_t   *json;
	JSONData_t     *data;
	JSONDataType_t  type;

	char buffer[LITERAL_MAX_SIZE] = {'\0'};
	int  r;
	int  off;

	json = u_ptr;

	data = json_get_top_data(json);

	r = json_parse_value(json_str, buffer, &type, &off);
	if (r != 0)
	{
		fprintf(stderr, "JSON value parsing failed at %s\n", json_str);
		
		return -1;
	}

	data->type = type;

	if (data->type == NUMBER || data->type == STRING)
	{
		data->val = json_data_str_new(buffer);
	}

	if (data->type == OBJECT)
	{
		JSONObject_t *o = json_object_new(json);

		r = json_parse_object(json_str, o, &off);
		if (r < 0)
		{
			fprintf(stderr, "JSON object parsing failed at '%s'\n", json_str);

			return -1;
		}

		data->val = o;
	}

	*state = WAITING_KEY_OR_END;

	return off;
}

static int json_hndl_ready_cb(char *json_str, void *u_ptr, JSONParseState_t *state)
{
	JSONObject_t *json;

	json = u_ptr;
	json->ready = 1;

	return 0;
}

JSONStateHandler_t state_hndl[STATE_COUNT] = {
	{ INITIAL, json_hndl_initial_cb },
	{ WAITING_KEY_OR_END, json_hndl_wkoe_cb },
	{ WAITING_VALUE, json_hndl_wv_cb },
	{ READY, json_hndl_ready_cb }
};

JSONObject_t *json_object_new(JSONObject_t *parent)
{
	JSONObject_t *obj = 0;

	obj	   = malloc(sizeof(JSONObject_t));
	obj->data  = 0;
	obj->depth = 0;
	obj->ready = 0;

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
	if (sz > KEY_MAX_SIZE)
	{
		fprintf(stderr, "Requesting too big key size: %d b\n", sz);
		return 0;
	}

	key_buf = calloc(sz + 1, 1);
	memcpy(key_buf, buf, sz);

	return key_buf;
}

int json_is_num(char c)
{
	if (c >= 48 && c <= 57)
		return 1;

	return 0;
}

int json_is_ws(char c)
{
	if (c == ' ' || c == '\n' || c == '\t')
		return 1;

	return 0;
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
			{
				in_quote = 0;
				waiting_end = 1;

				continue;
			}

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

		if (json_is_ws(c))
			continue;

		if (c == ':')
			break;

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

		if (in_quote)
		{
			if (c == '\\')
			{
				esc = 1;
				continue;
			}

			if (!esc && c == '"')
			{
				in_quote = 0;

				continue;
			}

			output[ob_c++] = c;

			if (esc)
				esc = 0;

			continue;
		}

		if (json_is_ws(c))
			continue;

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

			if (!json_is_num(c))
				return -1;

			type = NUMBER;
		}

		if (c == ',' || c == '}')
		{
			break;
		}

		if (type == NUMBER && !json_is_num(c))
		{
			if (c != '.')
				return -1;
		}

		output[ob_c++] = c;
	}

	output[ob_c] = '\0';
	*offset = i;
	*output_type = type;

	return 0;
}

int json_parse_object(char *json_str, JSONObject_t *output_obj, int *offset)
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

		r = state_hndl[state].func(json_str + i, json, &state);
		if (r < 0)
			return -1;

		i += r;
		if (i > len)
			return -1;
	}

	*offset = i;

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


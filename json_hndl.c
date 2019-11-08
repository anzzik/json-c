#include <stdio.h>
#include "json_hndl.h"
#include "json_helpers.h"
#include "json_lib.h"

int debug = 0;

static int json_hndl_initial_cb(char *json_str, void *u_ptr, JSONParseState_t *state);
static int json_hndl_wk_cb(char *json_str, void *u_ptr, JSONParseState_t *state);
static int json_hndl_colon_cb(char *json_str, void *u_ptr, JSONParseState_t *state);
static int json_hndl_wv_cb(char *json_str, void *u_ptr, JSONParseState_t *state);
static int json_hndl_wve_cb(char *json_str, void *u_ptr, JSONParseState_t *state);
static int json_hndl_ready_cb(char *json_str, void *u_ptr, JSONParseState_t *state);
static int json_hndl_ai_cb(char *json_str, void *u_ptr, JSONParseState_t *state);
static int json_hndl_awv_cb(char *json_str, void *u_ptr, JSONParseState_t *state);
static int json_hndl_awve_cb(char *json_str, void *u_ptr, JSONParseState_t *state);

static JSONStateHandler_t state_hndl[STATE_COUNT] = {
	{ INITIAL, json_hndl_initial_cb },
	{ WAITING_KEY, json_hndl_wk_cb },
	{ WAITING_COLON, json_hndl_colon_cb },
	{ WAITING_VALUE, json_hndl_wv_cb },
	{ WAITING_VALUE_END, json_hndl_wve_cb },
	{ ARRAY_INITIAL, json_hndl_ai_cb },
	{ ARRAY_WAITING_VALUE, json_hndl_awv_cb },
	{ ARRAY_WAITING_VALUE_END, json_hndl_awve_cb },
	{ READY, json_hndl_ready_cb }
};

JSONStateHandler_t *json_hndl_get_handlers()
{
	return state_hndl;
}


static int json_hndl_initial_cb(char *json_str, void *u_ptr, JSONParseState_t *state)
{
	int off;

	off = 0;
	while (json_is_ws(json_str[off]))
		off++;

	if (json_str[off] != '{')
	{
		fprintf(stderr, "JSON object is missing the opening parenthisis, got %c instead\n", json_str[off]);

		return -1;
	}

	if (debug)
		fprintf(stderr, "state to WAITING_KEY\n");

	*state = WAITING_KEY;

	off++;

	return off;
}

static int json_hndl_wk_cb(char *json_str, void *u_ptr, JSONParseState_t *state)
{
	JSONObject_t *json;
	JSONData_t   *data;
	int  off;
	int  r;
	char buffer[LITERAL_MAX_SIZE] = {'\0'};

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

	if (debug)
		fprintf(stderr, "state to WAITING_COLON\n");
	
	*state = WAITING_COLON;

	return off;
}

static int json_hndl_colon_cb(char *json_str, void *u_ptr, JSONParseState_t *state)
{
	int off;

	off = 0;
	while (json_is_ws(json_str[off]))
		off++;

	if (json_str[off] != ':')
	{
		fprintf(stderr, "Expected \";\" at '%s'\n", json_str + off);

		return -1;
	}

	off++;

	if (debug)
		fprintf(stderr, "state to WAITING_VALUE\n");

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

	off = 0;
	while (json_is_ws(json_str[off]))
		off++;

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
		data->val = json_data_str_new(buffer);

	if (data->type == OBJECT)
	{
		JSONObject_t *o = json_object_new(json);

		r = json_parse_object(json_str, o, &off, state_hndl);
		if (r < 0)
		{
			fprintf(stderr, "JSON object parsing failed at '%s'\n", json_str + off);

			return -1;
		}

		data->val = o;
	}

	if (data->type == ARRAY)
	{
		JSONObject_t *o = json_object_new(json);

		r = json_parse_array(json_str, o, &off, state_hndl);
		if (r < 0)
		{
			fprintf(stderr, "JSON array parsing failed at '%s'\n", json_str + off);

			return -1;
		}

		data->val = o;
	}

	if (debug)
		fprintf(stderr, "state to WAITING_VALUE_END\n");

	*state = WAITING_VALUE_END;

	return off;
}

static int json_hndl_wve_cb(char *json_str, void *u_ptr, JSONParseState_t *state)
{
	int  off;
	char c;

	off = 0;
	while (json_is_ws(json_str[off]))
		off++;

	c = json_str[off];

	if (c != ',' && c != '}')
	{
		fprintf(stderr, "Expected \",\" or \"}\" at '%s'\n", json_str + off);
		return -1;
	}

	if (c == ',')
	{
		if (debug)
			fprintf(stderr, "state to WAITING_KEY\n");

		*state = WAITING_KEY;
	}

	if (c == '}')
	{
		if (debug)
			fprintf(stderr, "state to READY (object)\n");

		*state = READY;
	}

	off++;

	return off;
}

static int json_hndl_ready_cb(char *json_str, void *u_ptr, JSONParseState_t *state)
{
	JSONObject_t *json;

	json = u_ptr;
	json->ready = 1;

	return 0;
}

static int json_hndl_ai_cb(char *json_str, void *u_ptr, JSONParseState_t *state)
{
	JSONObject_t *json;
	JSONData_t *array_data;
	int off;


	off = 0;
	while (json_is_ws(json_str[off]))
		off++;

	if (json_str[off] != '[')
	{
		fprintf(stderr, "JSON array object is missing the opening parenthisis, got %c instead\n", json_str[off]);

		return -1;
	}

	json = u_ptr;
	
	array_data = json_data_new();
	array_data->type = ARRAY;
	json_object_push(json, array_data);

	if (debug)
		fprintf(stderr, "state to ARRAY_WAITING_VALUE\n");

	*state = ARRAY_WAITING_VALUE;

	off++;

	return off;
}

static int json_hndl_awv_cb(char *json_str, void *u_ptr, JSONParseState_t *state)
{
	JSONObject_t   *json;
	JSONDataType_t  type;
	JSONDataArray_t *da;

	char buffer[LITERAL_MAX_SIZE] = {'\0'};
	int  r;
	int  off;

	off = 0;
	while (json_is_ws(json_str[off]))
		off++;

	json = u_ptr;

	r = json_parse_value(json_str, buffer, &type, &off);
	if (r < 0)
	{
		fprintf(stderr, "JSON value parsing failed at %s\n", json_str);
		
		return -1;
	}

	da = json_data_array_new();
	json_data_array_push(json, da);

	da->type = type;

	if (type == NUMBER || type == STRING)
		da->data = json_data_str_new(buffer);

	if (type == OBJECT)
	{
		JSONObject_t *o = json_object_new(json);

		r = json_parse_object(json_str, o, &off, state_hndl);
		if (r < 0)
		{
			fprintf(stderr, "JSON object parsing failed at '%s'\n", json_str);

			return -1;
		}

		da->data = o;
	}

	if (type == ARRAY)
	{
		JSONObject_t *o = json_object_new(json);

		r = json_parse_array(json_str, o, &off, state_hndl);
		if (r < 0)
		{
			fprintf(stderr, "JSON array parsing failed at '%s'\n", json_str);

			return -1;
		}

		da->data = o;
	}


	if (debug)
		fprintf(stderr, "state to ARRAY_WAITING_VALUE_END\n");

	*state = ARRAY_WAITING_VALUE_END;

	return off;
}

static int json_hndl_awve_cb(char *json_str, void *u_ptr, JSONParseState_t *state)
{
	int  off;
	char c;

	off = 0;
	while (json_is_ws(json_str[off]))
		off++;

	c = json_str[off];

	if (c != ',' && c != ']')
	{
		fprintf(stderr, "Expected \",\" or \"]\" at '%s'\n", json_str + off);
		return -1;
	}

	if (c == ',')
	{
		if (debug)
			fprintf(stderr, "state to ARRAY_WAITING_VALUE\n");

		*state = ARRAY_WAITING_VALUE;
	}

	if (c == ']')
	{
		if (debug)
			fprintf(stderr, "state to READY (array)\n");

		*state = READY;
	}

	off++;

	return off;
}


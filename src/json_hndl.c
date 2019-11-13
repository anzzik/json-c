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
	JSONKeyValuePair_t   *kvp;
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

	kvp      = json_kvp_new();
	kvp->key = json_str_new(buffer);

	json_object_push(json, kvp);

	if (debug)
		fprintf(stderr, "state to WAITING_COLON\n");
	
	*state = WAITING_COLON;

	return off;

l_err:
	json_kvp_pop(json, 1);
	return -1;
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
	JSONKeyValuePair_t     *kvp;
	JSONDataType_t  type;
	JSONObject_t *new_obj = 0;

	char buffer[LITERAL_MAX_SIZE] = {'\0'};
	int  r;
	int  off;
	int  value_len;

	off = 0;
	while (json_is_ws(json_str[off]))
		off++;

	json = u_ptr;

	kvp = json_kvp_top(json);

	r = json_parse_value(json_str + off, buffer, &type, &value_len);
	if (r != 0)
	{
		fprintf(stderr, "JSON value parsing failed at %s\n", json_str);
		
		return -1;
	}

	kvp->type = type;
	switch (kvp->type)
	{
		case STRING:
		case NUMBER:
			kvp->val = json_str_new(buffer);
			break;

		case OBJECT:
			new_obj = json_object_new(json);

			r = json_parse_object(json_str + off, new_obj, &value_len, state_hndl);
			if (r < 0)
			{
				fprintf(stderr, "JSON object parsing failed at '%s'\n", json_str + off);

				goto l_err;
			}
			kvp->val = new_obj;
			break;

		case ARRAY:
			new_obj = json_object_new(json);

			r = json_parse_array(json_str + off, new_obj, &value_len, state_hndl);
			if (r < 0)
			{
				fprintf(stderr, "JSON array parsing failed at '%s'\n", json_str + off);

				goto l_err;
			}

			kvp->val = new_obj;
			break;
	}

	if (debug)
		fprintf(stderr, "state to WAITING_VALUE_END\n");

	off += value_len;
	*state = WAITING_VALUE_END;

	return off;

l_err:
	json_kvp_pop(json, 1);
	return -1;
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
		fprintf(stderr, "Eexpected \",\" or \"}\" at '%.5s'\n", json_str + off);
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
	JSONKeyValuePair_t *array_kvp;
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
	
	array_kvp = json_kvp_new();
	array_kvp->type = ARRAY;
	json_object_push(json, array_kvp);

	if (debug)
		fprintf(stderr, "state to ARRAY_WAITING_VALUE\n");

	*state = ARRAY_WAITING_VALUE;

	off++;

	return off;

l_err:
	json_kvp_pop(json, 1);
	return -1;
}

static int json_hndl_awv_cb(char *json_str, void *u_ptr, JSONParseState_t *state)
{
	JSONObject_t    *json;
	JSONObject_t    *new_obj;
	JSONDataArray_t *da;
	JSONDataType_t   type;

	char buffer[LITERAL_MAX_SIZE] = {'\0'};
	int  r;
	int  off = 0;
	int  value_len = 0;

	json = u_ptr;

	if (!json->first_kvp)
	{
		fprintf(stderr, "JSON object not initialized properly for array push\n");

		return -1;
	}

	if (json->first_kvp->type != ARRAY)
	{
		fprintf(stderr, "JSON object's data is not ARRAY type\n");

		return -1;
	}

	while (json_is_ws(json_str[off]))
		off++;

	da = json_data_array_new();
	json_data_array_push(json, da);

	r = json_parse_value(json_str + off, buffer, &type, &value_len);
	if (r < 0)
	{
		fprintf(stderr, "JSON value parsing failed at %s\n", json_str);
		
		goto l_err;
	}

	da->type = type;
	switch (da->type)
	{
		case NUMBER: 
		case STRING:
			da->data = json_str_new(buffer);
			break;

		case OBJECT:
			new_obj = json_object_new(json);

			r = json_parse_object(json_str + off, new_obj, &value_len, state_hndl);
			if (r < 0)
			{
				fprintf(stderr, "JSON object parsing failed at '%.30s'\n", json_str + off);

				goto l_err;
			}

			da->data = new_obj;
			break;

		case ARRAY:
			new_obj = json_object_new(json);

			r = json_parse_array(json_str + off, new_obj, &value_len, state_hndl);
			if (r < 0)
			{
				fprintf(stderr, "JSON array parsing failed at '%10.s'\n", json_str + off);

				goto l_err;
			}

			da->data = new_obj;
			break;
	}

	if (debug)
		fprintf(stderr, "state to ARRAY_WAITING_VALUE_END\n");

	off += value_len;
	*state = ARRAY_WAITING_VALUE_END;

	return off;

l_err:
	json_data_array_pop(json->first_kvp, 1);
	return -1;
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
		fprintf(stderr, "Expected \",\" or \"]\" at '%.5s'\n", json_str + off);
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


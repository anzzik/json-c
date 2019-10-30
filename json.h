#ifndef JSON_H
#define JSON_H

typedef enum JSONDataType_e JSONDataType_t;
typedef enum JSONParseState_e JSONParseState_t;
typedef struct JSONData_s JSONData_t;
typedef struct JSONObject_s JSONObject_t;
typedef struct JSONStateHandler_s JSONStateHandler_t;

enum JSONDataType_e
{
	UNDEFINED = 0,
	NUMBER,
	STRING,
	OBJECT,
	ARRAY,
	TYPE_COUNT
};

enum JSONParseState_e
{
	INITIAL = 0,
	WAITING_KEY_OR_END,
	WAITING_COLON,
	WAITING_VALUE,
	WAITING_VALUE_END,
	READY,
	STATE_COUNT
};

struct JSONData_s
{
	JSONDataType_t type;
	char *key;
	void *val;

	JSONData_t *next;
};

struct JSONObject_s
{
	JSONData_t *data;
	int ready;
	int depth;
};

struct JSONStateHandler_s
{
	JSONParseState_t state;
	int (*func)(char*, void*, JSONParseState_t*);
};

JSONObject_t *json_object_new(JSONObject_t *parent);
JSONData_t *json_data_new();
JSONData_t *json_get_top_data(JSONObject_t *json);
int json_is_num(char c);
int json_is_ws(char c);
char *json_data_str_new(char *buf);
int json_data_set_val(JSONData_t *data, void *val);
int json_object_push(JSONObject_t *json_obj, JSONData_t *json_data);
int json_parse_key(char *str, char *output, int *offset);
int json_parse_value(char *str, char *output, JSONDataType_t *output_type, int *offset);
int json_parse_object(char *json_str, JSONObject_t *output_obj, int *offset);
void json_object_print(JSONObject_t *json_obj);

#endif


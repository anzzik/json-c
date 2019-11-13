#ifndef JSON_TYPES_H
#define JSON_TYPES_H

#define LITERAL_MAX_SIZE 1024 

typedef enum JSONDataType_e JSONDataType_t;
typedef enum JSONParseState_e JSONParseState_t;

typedef struct JSONObject_s JSONObject_t;
typedef struct JSONKeyValuePair_s JSONKeyValuePair_t;
typedef struct JSONDataArray_s JSONDataArray_t;
typedef struct JSONStateHandler_s JSONStateHandler_t;

struct JSONObject_s
{
	JSONKeyValuePair_t *first_kvp;
	int is_array;
	int ready;
	int depth;
};

enum JSONDataType_e
{
	UNDEFINED = 0,
	NUMBER,
	STRING,
	OBJECT,
	ARRAY,
	TYPE_COUNT
};

struct JSONKeyValuePair_s
{
	JSONDataType_t type;
	char *key;
	void *val;

	JSONKeyValuePair_t *next;
};

struct JSONDataArray_s
{
	JSONDataType_t type;
	void *data;

	JSONDataArray_t *next;
};

enum JSONParseState_e
{
	INITIAL = 0,
	WAITING_KEY,
	WAITING_COLON,
	WAITING_VALUE,
	WAITING_VALUE_END,
	ARRAY_INITIAL,
	ARRAY_WAITING_VALUE,
	ARRAY_WAITING_VALUE_END,
	READY,
	STATE_COUNT
};

struct JSONStateHandler_s
{
	JSONParseState_t state;
	int (*func)(char*, void*, JSONParseState_t*);
};

#endif


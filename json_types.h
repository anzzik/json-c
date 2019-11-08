#ifndef JSON_TYPES_H
#define JSON_TYPES_H

#define LITERAL_MAX_SIZE 1024 
#define KEY_MAX_SIZE 64 

typedef enum JSONDataType_e JSONDataType_t;
typedef enum JSONParseState_e JSONParseState_t;

typedef struct JSONObject_s JSONObject_t;
typedef struct JSONData_s JSONData_t;
typedef struct JSONDataArray_s JSONDataArray_t;
typedef struct JSONStateHandler_s JSONStateHandler_t;

struct JSONObject_s
{
	JSONData_t *data;
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

struct JSONData_s
{
	JSONDataType_t type;
	char *key;
	void *val;

	JSONData_t *next;
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


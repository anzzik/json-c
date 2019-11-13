#ifndef JSON_LIB_H
#define JSON_LIB_H

#include "json_types.h"

JSONObject_t *json_object_new(JSONObject_t *parent);
void json_object_free(JSONObject_t *json_obj);
JSONKeyValuePair_t *json_kvp_new();
void json_kvp_free(JSONKeyValuePair_t *kvp);
JSONDataArray_t *json_data_array_new();
void json_data_array_free(JSONDataArray_t* da);
int json_parse_object(char *json_str, JSONObject_t *output_obj, int *offset, JSONStateHandler_t *state_handlers);
int json_parse_array(char *json_str, JSONObject_t *output_obj, int *offset, JSONStateHandler_t *state_handlers);
JSONKeyValuePair_t *json_kvp_top(JSONObject_t *json);
int json_kvp_list_len(JSONObject_t *json);
int json_kvp_pop(JSONObject_t *json, int do_free);
char *json_str_new(char *buf);
void json_str_free(char *buf);
int json_kvp_set_val(JSONKeyValuePair_t *kvp, void *val);
int json_object_push(JSONObject_t *json_obj, JSONKeyValuePair_t *kvp);
int json_data_array_push(JSONObject_t *json_array_obj, JSONDataArray_t *json_array_data);
int json_data_array_list_len(JSONKeyValuePair_t *kvp);
int json_data_array_pop(JSONKeyValuePair_t *kvp, int do_free);

int json_parse_key(char *str, char *output, int *offset);
int json_parse_value(char *str, char *output, JSONDataType_t *output_type, int *offset);
void json_object_print(JSONObject_t *json_obj);
void json_object_array_print(JSONObject_t *json_obj);

#endif


#ifndef JSON_LIB_H
#define JSON_LIB_H

#include "json_types.h"

JSONObject_t *json_object_new(JSONObject_t *parent);
JSONData_t *json_data_new();
JSONDataArray_t *json_data_array_new();
int json_parse_object(char *json_str, JSONObject_t *output_obj, int *offset, JSONStateHandler_t *state_handlers);
int json_parse_array(char *json_str, JSONObject_t *output_obj, int *offset, JSONStateHandler_t *state_handlers);
JSONData_t *json_get_top_data(JSONObject_t *json);
char *json_data_str_new(char *buf);
int json_data_set_val(JSONData_t *data, void *val);
int json_object_push(JSONObject_t *json_obj, JSONData_t *json_data);
int json_data_array_push(JSONObject_t *json_array_obj, JSONDataArray_t *json_array_data);
int json_parse_key(char *str, char *output, int *offset);
int json_parse_value(char *str, char *output, JSONDataType_t *output_type, int *offset);
void json_object_print(JSONObject_t *json_obj);
void json_object_array_print(JSONObject_t *json_obj);

#endif


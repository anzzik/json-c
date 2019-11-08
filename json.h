#ifndef JSON_H
#define JSON_H

#include "json_types.h"
#include "json_helpers.h"

JSONObject_t *json_start(char *json_str);
void json_print(JSONObject_t *json_obj);
char *json_read_file(char *filename);

#endif


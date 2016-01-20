#ifndef __JSON_UTILS_H_
#define __JSON_UTILS_H_

#include "definitions.h"

const gchar* get_json_member_string(JsonReader *reader, const gchar* member);

gboolean load_json_from_file(JsonParser* parser, gchar* path);

gboolean load_json_from_data(JsonParser* parser, gchar* data, gssize length);

const gchar* get_value_of_member(jsonreply* data, gchar* search);

#endif
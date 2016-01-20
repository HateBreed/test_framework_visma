#ifndef __JSON_UTILS_H_
#define __JSON_UTILS_H_

#include "definitions.h"

gchar* get_json_member_string(JsonReader *reader, const gchar* member);

gboolean load_json_from_file(JsonParser* parser, gchar* path);

gboolean load_json_from_data(JsonParser* parser, gchar* data, gssize length);

gchar* get_value_of_member(jsonreply* data, gchar* search, gchar* search2);

gboolean set_value_of_member(jsonreply* data, gchar* member, gchar* value);

jsonreply* create_delete_reply(gchar* member, gchar* value);

#endif
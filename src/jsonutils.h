#ifndef __JSON_UTILS_H_
#define __JSON_UTILS_H_

#include "definitions.h"

gchar* get_json_member_string(JsonReader *reader, const gchar* member);

gchar* get_json_member_integer(JsonReader *reader, const gchar* member);

gboolean load_json_from_file(JsonParser* parser, const gchar* path);

gboolean load_json_from_data(JsonParser* parser, const gchar* data, const gssize length);

gchar* get_value_of_member(jsonreply* data, const gchar* search, const gchar* search2);

gboolean set_value_of_member(jsonreply* data, const gchar* member, const gchar* value);

jsonreply* create_delete_reply(const gchar* member, const gchar* value);

gboolean verify_server_response(jsonreply* request, jsonreply* response);

gboolean replace_required_member(GHashTable* filetable, testfile* tfile, gint index);

gboolean replace_getinfo_member(testfile* tfile, gint index, const gchar* url);

#endif
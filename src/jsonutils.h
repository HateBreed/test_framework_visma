#ifndef __JSON_UTILS_H_
#define __JSON_UTILS_H_

#include "definitions.h"

void set_integer_fields(GSList *intfields);

gchar* get_json_member_string(JsonReader *reader, const gchar* member);

gchar* get_json_member_integer(JsonReader *reader, const gchar* member);

gboolean load_json_from_file(JsonParser* parser, const gchar* path);

gboolean load_json_from_data(JsonParser* parser, const gchar* data, const gssize length);

gchar* get_value_of_member(jsonreply* data, const gchar* search, const gchar* search2);

gboolean set_value_of_member(jsonreply* data, const gchar* member, const gchar* value);
gboolean set_values_of_all_members(jsonreply* jsondata, GHashTable* replace);

jsonreply* create_delete_reply(const gchar* member, const gchar* value);

gboolean verify_server_response(jsonreply* request, jsonreply* response);

gboolean replace_required_member(GHashTable* filetable, testfile* tfile, gint index);
gboolean add_required_member_value_to_list(GHashTable* filetable, testfile* tfile, gint index);

gboolean replace_getinfo_member(testfile* tfile, gint index, const gchar* url);
gboolean add_getinfo_member_value_to_list(testfile* tfile, gint index, const gchar* url);

#endif
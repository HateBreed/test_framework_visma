#ifndef __JSON_UTILS_H_
#define __JSON_UTILS_H_

#include <glib.h>
#include <json-glib/json-glib.h>

const gchar* get_json_member_string(JsonReader *reader, const gchar* member);

#endif
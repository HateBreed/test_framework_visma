#include "jsonutils.h"

const gchar* get_json_member_string(JsonReader *reader, const gchar* member) {
	if(!reader || !member) return NULL;
	json_reader_read_member(reader,member);
	const gchar* string = json_reader_get_string_value(reader);
	json_reader_end_member(reader);
	return string;
}
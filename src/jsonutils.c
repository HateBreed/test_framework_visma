#include "jsonutils.h"

const gchar* get_json_member_string(JsonReader *reader, const gchar* member) {
	if(!reader || !member) return NULL;
	json_reader_read_member(reader,member);
	const gchar* string = json_reader_get_string_value(reader);
	json_reader_end_member(reader);
	return string;
}
gboolean load_json_from_file(JsonParser* parser, gchar* filepath) {
	if(!parser || !filepath) return FALSE;
	
	GError *error = NULL;
	json_parser_load_from_file(parser, filepath, &error);
		
	if (error) {
		g_print ("Cannot parse file \"%s\". Reason: %s\n", filepath, error->message);
		g_error_free(error);
		g_object_unref(parser);
		return FALSE;
	}
	
	return TRUE;
}
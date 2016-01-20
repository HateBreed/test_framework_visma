#include "jsonutils.h"

const gchar* get_json_member_string(JsonReader *reader, const gchar* member) {
	if(!reader || !member) return NULL;
	const gchar* string = NULL;
	
	// If a member was found
	if(json_reader_read_member(reader,member)) string = g_strdup(json_reader_get_string_value(reader));
	
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

gboolean load_json_from_data(JsonParser* parser, gchar* data, gssize length) {
	if(!parser || !data || length == 0) return FALSE;
	
	GError *error = NULL;
	json_parser_load_from_data(parser, data, length, &error);
		
	if (error) {
		g_print ("Cannot parse data. Reason: %s\n", error->message);
		g_error_free(error);
		g_object_unref(parser);
		return FALSE;
	}
	
	return TRUE;
}

const gchar* get_value_of_member(jsonreply* jsondata, gchar* search) {
	if(!jsondata || !search) return NULL;
	
	const gchar* value = NULL;
	JsonParser *parser = json_parser_new();
	
	if(load_json_from_data(parser,jsondata->data,jsondata->length)) {
		JsonReader *reader = json_reader_new (json_parser_get_root (parser));
		
		if(json_reader_read_member(reader,"data")) {
		
			if(json_reader_is_array(reader)) {
				for(gint idx = 0; idx < json_reader_count_elements(reader); idx++) {
					json_reader_read_element(reader,idx);
					value = get_json_member_string(reader,search);
					json_reader_end_element(reader);
					if(value) break;
				}
			}
			else if(json_reader_is_object(reader)) {
				value = get_json_member_string(reader,search); 
			}
			// TODO check if is on other type
		}
		else g_print("No such member data\n");
		
		json_reader_end_member(reader);
		
		g_object_unref(reader);
	}
	
	g_object_unref(parser);
	return value;
}
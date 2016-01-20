#include "jsonutils.h"
#include "jsonhandler.h"

gchar* get_json_member_string(JsonReader *reader, const gchar* member) {
	if(!reader || !member) return NULL;
	gchar* string = NULL;
	
	// If a member was found, create a duplicate of string for returning
	if(json_reader_read_member(reader,member))
		string = g_strdup(json_reader_get_string_value(reader));
	
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

gchar* get_value_of_member(jsonreply* jsondata, gchar* search, gchar* search2) {
	if(!jsondata || !search) return NULL;
	
	gchar* value = NULL;
	JsonParser *parser = json_parser_new();
	
	// Load json data
	if(load_json_from_data(parser,jsondata->data,jsondata->length)) {
		JsonReader *reader = json_reader_new (json_parser_get_root (parser));
		
		// Replies contain either data or error, only data is checked now
		if(json_reader_read_member(reader,"data")) {
		
			// If within an array
			if(json_reader_is_array(reader)) {
				for(gint idx = 0; idx < json_reader_count_elements(reader); idx++) {
					json_reader_read_element(reader,idx);
					value = get_json_member_string(reader,search);
					json_reader_end_element(reader);
					if(value) break;
				}
			}
			// Plain json object
			else if(json_reader_is_object(reader)) {
				gboolean success = FALSE;
				if(search2) {
					g_print("search: %s search2: %s\n",search,search2);
					if(json_reader_read_member(reader,search2)) {
						success = TRUE;
					}
				}
				value = get_json_member_string(reader,search);
				if(success) json_reader_end_member(reader);
			}
			// TODO check if is on other type
		}
		
		json_reader_end_member(reader);
		
		g_object_unref(reader);
	}
	
	g_object_unref(parser);
	return value;
}

// TODO use a hashtable to include all values to be changed
gboolean set_value_of_member(jsonreply* jsondata, gchar* member, gchar* value) {
	if(!jsondata || !member || !value) return FALSE;
	
	JsonParser *parser = json_parser_new();
	
	// Load existing data
	if(load_json_from_data(parser,jsondata->data,jsondata->length)) {
	
		// Initialize a new builder to construct new json with
		// a new value for a member
		JsonBuilder *builder = json_builder_new();
		json_builder_begin_object(builder);
		
		// Start reading json and get list of members
		JsonReader *reader = json_reader_new (json_parser_get_root (parser));
		gchar** members = json_reader_list_members(reader);
		
		// Go through members
		for(gint membidx = 0; members[membidx] != NULL; membidx++) {
		
			// Get member and set it as current member name in new json
			gchar* membstring = get_json_member_string(reader,members[membidx]);
			json_builder_set_member_name(builder,members[membidx]);
			
			// If this is what we're looking for, use value instead of existing one
			if(g_strcmp0(member,members[membidx]) == 0) json_builder_add_string_value(builder, value);
			else json_builder_add_string_value(builder, membstring);
			
			g_free(membstring);
		}
		// End building
		json_builder_end_object(builder);

		// Generate new json from the built data
		JsonGenerator *generator = json_generator_new();
		json_generator_set_root(generator, json_builder_get_root(builder));
		
		// Free previous data and assign new to this json
		g_free(jsondata->data);
		jsondata->data = json_generator_to_data(generator,&(jsondata->length));
		
		g_object_unref(generator);
		
		g_object_unref(builder);
	}
	else return FALSE;
	
	g_object_unref(parser);
	
	return TRUE;
}

jsonreply* create_delete_reply(gchar* member, gchar* value) {
	if(!value || !member) return NULL;
	
	// Storage for reply
	jsonreply* delreply = jsonreply_initialize();
	
	// Begin object
	JsonBuilder *builder = json_builder_new();
	json_builder_begin_object(builder);
	
	// Set member and value
	json_builder_set_member_name(builder,g_strdup(member));
	json_builder_add_string_value(builder,value);

	// Close object
	json_builder_end_object(builder);
	
	// Generate new json from the built data
	JsonGenerator *generator = json_generator_new();
	json_generator_set_root(generator, json_builder_get_root(builder));		
		
	// Assign data to struct
	delreply->data = json_generator_to_data(generator,&(delreply->length));
		
	g_object_unref(generator);
	
	g_object_unref(builder);
	
	return delreply;
}
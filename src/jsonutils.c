#include "jsonutils.h"
#include "preferences.h"

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
		g_error ("Cannot parse file \"%s\". Reason: %s\n", filepath, error->message);
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
		g_error ("Cannot parse data. Reason: %s\n", error->message);
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
				if(search2) {
					g_debug("get_value_of_member: search: %s search2: %s.",search,search2);
					if(json_reader_read_member(reader,search2)) {
						g_debug(".. found\n");
					}
				}
				value = get_json_member_string(reader,search);
				if(search2) json_reader_end_member(reader);
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

gboolean verify_in_array(JsonParser *parser, gchar* check_value1, gchar* check_value2) {
	
	if(!parser || !check_value1 || !check_value2) return FALSE;
	
	gboolean success = TRUE;
	
	// Reader for response
	JsonReader *reader = json_reader_new (json_parser_get_root (parser));
	
	// Start reading array
	if(json_reader_read_member(reader,"data")) {
	
		// If it is an array
		if(json_reader_is_array(reader)) {
		
			// Go through elements
			for(gint idx = 0; idx < json_reader_count_elements(reader); idx++) {
				gboolean found = FALSE;
				json_reader_read_element(reader,idx);
				
				gchar *value1 = NULL, *value2 = NULL;
				
				// Get members of this element and go through each
				gchar** members = json_reader_list_members(reader);
				
				for(gint membidx = 0; members[membidx] != NULL; membidx++) {
				
					// Title was found as identifier that this is correct element
					if(g_strcmp0(members[membidx],"title") == 0) {
						// This is correct element if the values of title is same as first input
						value1 = get_json_member_string(reader,members[membidx]);
						if(g_strcmp0(value1,check_value1) == 0) found = TRUE;
					}
					
					// If this is correct element and this is "formatted_value" member
					if(g_strcmp0(members[membidx],"formatted_value") == 0 && found) {
					
						// Get the value of this member
						value2 = get_json_member_string(reader,members[membidx]);
						
						// If values do not match
						if(g_strcmp0(value2,check_value2) != 0) {
							success = FALSE;
							g_print(".. failure.\n\"%s\" values \"%s\" and \"%s\" differ\n",
								check_value1, check_value2, value2);
						}
					}
				} // for
				
				g_free(value1);
				g_free(value2);
				g_strfreev(members);
				json_reader_end_element(reader);
			} // for			
		}
		else g_debug("not array\n");
	}
	else g_debug("no data member\n");
	
	json_reader_end_element(reader);
	g_object_unref(reader);
	
	return success;
}

gboolean verify_server_response(jsonreply* request, jsonreply* response) {

	gboolean test_ok = TRUE;
	gboolean array = FALSE;
	JsonParser *req_parser = json_parser_new();
	JsonParser *res_parser = json_parser_new();
	
	
	// Load both jsons from memory
	if(load_json_from_data(req_parser,request->data,request->length) &&
		load_json_from_data(res_parser,response->data,response->length)) {
		
		// Initialize reader for request only
		JsonReader *req_reader = json_reader_new (json_parser_get_root (req_parser));
		
		// If this member contains data it must be dealt differenlty from plain json objects
		if(json_reader_read_member(req_reader,"data")) {
		
			// This is an array of json objects
			if(json_reader_is_array(req_reader)) {
				array = TRUE;
				
				// Go through all array elements
				for(gint idx = 0; idx < json_reader_count_elements(req_reader); idx++) {
				
					// Read element
					json_reader_read_element(req_reader,idx);
					
					gchar *value1 = NULL;
					gchar *value2 = NULL;
					
					// Get members and go through each
					gchar** members = json_reader_list_members(req_reader);
					
					for(gint membidx = 0; members[membidx] != NULL; membidx++) {

						// TODO rethink this
						// A hardcoded way to check only two fields "title" and "formatted_value"
						// Get the value of "title"
						if(g_strcmp0(members[membidx],"title") == 0)
								value1 = get_json_member_string(req_reader,members[membidx]);
								
						// Get the value of "formatted_value"
						if(g_strcmp0(members[membidx],"formatted_value") == 0)
								value2 = get_json_member_string(req_reader,members[membidx]);
					}
					
					// Check if the response contains same value as the value2 (formatted_value)
					g_print("Checking that \"%s\" is \"%s\" .",value1,value2);
					test_ok = verify_in_array(res_parser,value1,value2);
					if(test_ok) g_print("..ok.\n");
					
					g_free(value1);
					g_free(value2);
					g_strfreev(members);
					json_reader_end_element(req_reader);
				}
			}
			
			// Close member (array)
			json_reader_end_member(req_reader);
		}
		else {
			// Member (array with "data" member) has to be closed if it was not found
			if(!array) json_reader_end_member(req_reader);
		
			// Get memberlist of request and go through each
			gchar** members = json_reader_list_members(req_reader);
		
			for(gint membidx = 0; members[membidx] != NULL; membidx++) {
			
				// Get the value of current member
				gchar* req_membstring = get_json_member_string(req_reader,members[membidx]);
				
				// Get the corresponding value from the response
				gchar* res_membstring = get_value_of_member(response,members[membidx],NULL);
				
				g_debug("value %s: %s\n",members[membidx], req_membstring);
			
				// Both were found
				if(req_membstring && res_membstring) {
				
					// Check if they match
					if(g_strcmp0(req_membstring,res_membstring) != 0) {
						g_print("Values of %s differ (request: %s - response: %s)\n",
							members[membidx], req_membstring, res_membstring);
						test_ok = FALSE;
					}
				}
				
				g_free(req_membstring);
				g_free(res_membstring);
			}
			g_strfreev(members);
		}
		g_object_unref(req_reader);
	}
	
	g_object_unref(req_parser);
	g_object_unref(res_parser);
	return test_ok;
}
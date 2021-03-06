#include "jsonutils.h"
#include "preferences.h"
#include "connectionutils.h"

GSList *integer_fields = NULL;

/**
* Set the integer field list (set a pointer, nothing more)
*@param new integer field list
*/
void set_integer_fields(GSList *intfields) {
	integer_fields = intfields;
}

/**
* Print check initial string
*
* @param member Member field
* @param value MEmber field value
*/
void print_check_init(const gchar* member, const gchar* value) {
	g_print("Checking that \"%s\" is \"%s\" ",member,value);
}

/**
* Print check failure string
* 
* @param request Value that was requeste
* @param response Value set by the server
*/
void print_check_failure(const gchar* request, const gchar* response) {
	if(!request  || !response) return;
	g_print("[failure]\n\tValues differ: \n\t\trequest:\t%s\n\t\tresponse:\t%s\n",
		request, response);
}

/**
* Print check when value is missing (null), calls print_check_failure.
*
* @param Member field value
*/
void print_check_missing(const gchar* membervalue) {
	print_check_failure(membervalue,"null");
}

/**
* Print ok for check
*/
void print_check_ok() { g_print("[ok]\n"); }

/**
* Check if member field contains integers only
* 
* @param member Name of the member to check (or member itself)
*
* @return TRUE the member contains integers only
*/
gboolean is_member_integer(const gchar* member) {

	if(member && integer_fields) {
		// Go through list
		for(gint index = 0; index < g_slist_length(integer_fields); index++) {
			if(g_strcmp0(member,(gchar*)g_slist_nth_data(integer_fields,index)) == 0)
				return TRUE;
		}
	}
	return FALSE;
}

/**
* Get string value of member field from json loaded in reader
* 
* @param reader Reader containing json which is used to get the member field value
* @param member NAme of the member field to use 
*
* @return Pointer to newly allocated gchar (has to be freed with g_free())
*/
gchar* get_json_member_string(JsonReader *reader, const gchar* member) {
	if(!reader || !member) return NULL;
	gchar* string = NULL;
	
	// If a member was found, create a duplicate of string for returning
	if(json_reader_read_member(reader,member))
		string = g_strdup(json_reader_get_string_value(reader));
	
	json_reader_end_member(reader);
	return string;
}

/**
* Get integer value of member field as string from json loaded in reader.
* 
* Gets the value as double and prints it to gchar pointer (with g_strdup_printf())
* 
* @param reader Reader containing json which is used to get the member field value
* @param member NAme of the member field to use 
*
* @return Pointer to newly allocated gchar (has to be freed with g_free())
*/
gchar* get_json_member_integer(JsonReader *reader, const gchar* member) {
	if(!reader || !member) return 0;
	gchar* intval = NULL;
	gdouble dval = 0.0;
	
	// If a member was found, create a duplicate of string for returning
	if(json_reader_read_member(reader,member))
		//intval = json_reader_get_int_value(reader);
		dval = json_reader_get_double_value(reader);
	
	json_reader_end_member(reader);
	intval = g_strdup_printf("%0.4f",dval);
	return intval;
}

/**
* Load json from file to given parser
* 
* @param parser Parser to which the specified json is loaded to
* @param filepath path to file to be loaded
*
* @return TRUE when file was loaded
*/
gboolean load_json_from_file(JsonParser *parser, const gchar* filepath) {
	if(!parser || !filepath) return FALSE;
	
	gboolean rval = FALSE;
	
	GError *error = NULL;
	
	// Load file and get returnvalue
	rval = json_parser_load_from_file(parser, filepath, &error);
		
	if (error && !rval) {
		g_print ("Cannot parse file \"%s\". Reason: %s\n", filepath, error->message);
		g_error_free(error);
	}
	
	return rval;
}

/**
* Load json from data to given parser
* 
* @param parser Parser to which the specified json is loaded to
* @param data buffer containing json as charstring
* @param length length of the data
*
* @return TRUE when data was loaded
*/
gboolean load_json_from_data(JsonParser* parser, const gchar* data, const gssize length) {
	if(!parser || !data || length == 0) return FALSE;

	gboolean rval = FALSE;	
	GError *error = NULL;
	
	rval = json_parser_load_from_data(parser, data, length, &error);
		
	if (error) {
		g_error ("Cannot parse data. Reason: %s\n", error->message);
		g_error_free(error);
	}
	
	return rval;
}

/**
* Retrieve given value from given json as data. Can be used to search from an array or
* from an object with two level search. First search value is the member name whose value
* is retrieved. Second value defines whether there is a object within that has to be accessed.
*
* @param jsondata JSON as data in form of jsonreply_t
* @param search Member name whose value is retrieved
* @param search2 Search parameter that can increase the depth of search
*
* @return A newly allocated (by get_json_member_string()) gchar that must be free'd with g_free()
*/
gchar* get_value_of_member(jsonreply* jsondata, const gchar* search, const gchar* search2) {
	if(!jsondata || !search) return NULL;
	
	gchar *value = NULL;
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
					
					// Depth increased?
					if(search2) {
						g_debug("get_value_of_member: search: %s search2: %s.",search,search2);
						if(json_reader_read_member(reader,search2)) {
							g_debug(".. found\n");
						}
					}
					
					// If member contains integer
					if(is_member_integer(search)) value = get_json_member_integer(reader,search);
					// Plain string
					else value = get_json_member_string(reader,search);
					
					// End "depth" member
					if(search2) json_reader_end_member(reader);
					
					// End this element in array
					json_reader_end_element(reader);
					
					// Found? 
					if(value) break;
				}
			}
			// Plain json object
			else if(json_reader_is_object(reader)) {
			
				// Depth increased?
				if(search2) {
					g_debug("get_value_of_member: search: %s search2: %s.",search,search2);
					if(json_reader_read_member(reader,search2)) {
						g_debug(".. found\n");
					}
				}
				// Member contains integers?
				if(is_member_integer(search)) value = get_json_member_integer(reader,search);
				// Plain string
				else value = get_json_member_string(reader,search);
				
				// End "depth" member
				if(search2) json_reader_end_member(reader);
			}
		}
		
		json_reader_end_member(reader); // data
		
		g_object_unref(reader);
	}
	
	g_object_unref(parser);
	return value;
}


/** 
* Replace value of given member field in the json. Creates a new json from the given jsonreply_t
* while replacing a single value in the json data. Clumsy, a hash table should be used instead to
* replace all values at the same time.
*
* @param jsondata Pointer to structure containing json
* @param member Member field name whose value is to be replaced
* @param value New value for the member field
*
* @return TRUE when data could be loaded
*/
gboolean set_value_of_member(jsonreply* jsondata, const gchar* member, const gchar* value) {
	if(!jsondata || !member || !value) return FALSE;
	
	gboolean rval = TRUE;
	
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
		g_object_unref(reader);

		// Generate new json from the built data
		JsonGenerator *generator = json_generator_new();
		json_generator_set_root(generator, json_builder_get_root(builder));
		
		// Free previous data and assign new to this json
		g_free(jsondata->data);
		jsondata->data = json_generator_to_data(generator,&(jsondata->length));
		
		g_strfreev(members);
		g_object_unref(generator);
		
		g_object_unref(builder);
	}
	else rval = FALSE;
	
	g_object_unref(parser);
	
	return rval;
}

/** 
* Replace all values in json if corresponding member value is found from the
* given hash table. Creates a new json from the given jsonreply_t
* while replacing all value in the json data.
*
* @param jsondata Pointer to structure containing json
* @param replace Hash table containing member-value pairs (members as keys)
*
* @return TRUE when data could be loaded and all values were replaced
*/
gboolean set_values_of_all_members(jsonreply* jsondata, GHashTable* replace) {
	if(!jsondata || !replace) return FALSE;
	
	// List empty
	if(g_hash_table_size(replace) == 0) return TRUE;
	
	gboolean rval = TRUE;
	gint replaced = 0;
	
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
			
			// Find value from hash table
			gchar* new_value = (gchar*)g_hash_table_find(replace,
				(GHRFunc)find_from_hash_table, 
				members[membidx]);
			
			// If value for this member is found in replace hash table
			// use new value instead of existing one
			if(new_value) {
				json_builder_add_string_value(builder, new_value);
				replaced++;
			}
			else json_builder_add_string_value(builder, membstring);
			
			g_free(membstring);
		}
		// End building
		json_builder_end_object(builder);
		g_object_unref(reader);

		// Generate new json from the built data
		JsonGenerator *generator = json_generator_new();
		json_generator_set_root(generator, json_builder_get_root(builder));
		
		// Free previous data and assign new to this json
		g_free(jsondata->data);
		jsondata->data = json_generator_to_data(generator,&(jsondata->length));
		
		g_strfreev(members);
		g_object_unref(generator);
		
		g_object_unref(builder);
	}
	else rval = FALSE;
	
	g_object_unref(parser);
	
	// All were not replaced
	if(replaced != g_hash_table_size(replace)) {
		g_print("Replaced %d values but hash table contains: %d values. Errors may exist in tests\n",replaced,g_hash_table_size(replace));
		rval = FALSE;
	}
	return rval;
}

/**
* Create a JSON delete reply containing only one given member with given value
*
* @param member member field to include
* @value value Value to assign to member
*
* @return jsonreply containing the newly created reply
*/
jsonreply* create_delete_reply(const gchar* member, const gchar* value) {
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


/**
* Verify a JSON array loaded in parser that it has the given two values in one element,
* more specificly that when check_value1 is found from a member within an element
* having "title" as member name, the check_value2 is checked for equality to that elements
* "formatted_value" field.
*
* @param parser Parser in which the JSON is loaded
* @param check_value1 Value of "title" member field
* @param check_value2 Value to be checked, the value of "formatted_value" member field
*
* @return TRUE only when equal match is found
*/
gboolean verify_in_array(JsonParser *parser, const gchar* check_value1, const gchar* check_value2) {
	
	if(!parser || !check_value1 || !check_value2) return FALSE;
	
	gboolean success = FALSE;
	
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
				
					// These values are always in string format according to REST API
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
							print_check_failure(check_value2, value2);
						}
						else print_check_ok();
					}
				} // for
				
				g_free(value1);
				g_free(value2);
				g_strfreev(members);
				json_reader_end_element(reader);
			} // for			
		}
		else g_print("Cannot verify: not an array.\n");
	}
	else g_print("Cannot verify: no \"data\" member present in json.\n");
	
	json_reader_end_element(reader);
	g_object_unref(reader);
	
	return success;
}

/**
* Verify two JSONs for equality of values. The members of request are
* looped through and corresponding values are searched from response JSON.
* Can iterate through arrays, which have all members within "data" member
* and works for plain JSON objects. If an array is to be verified 
* verify_in_array() is called.
*
* @param request Request jsonreply_t containing requested values
* @param response Server response jsonreply_t containing values set by server
*
* @return TRUE when all values in request could be found from response and they match
*/
gboolean verify_server_response(jsonreply* request, jsonreply* response) {

	if(!request  || !response) return FALSE;

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
					print_check_init(value1,value2);
					test_ok = verify_in_array(res_parser,value1,value2);
					if(test_ok) print_check_ok();
					
					g_free(value1);
					g_free(value2);
					g_strfreev(members);
					json_reader_end_element(req_reader);
				}
			}
			else {
				g_print("Member field \"data\" is not an array as expected. Cannot verify.\n");
				test_ok = FALSE;
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
						
				print_check_init(members[membidx],req_membstring);
				// Response was found
				if(res_membstring) {
					// Length of the request
					guint length = strlen(req_membstring);
					
					// Check if they match by comparing only the amount of characters
					// the request value contains
					if(g_ascii_strncasecmp(req_membstring,res_membstring,length) != 0) {
						print_check_failure(req_membstring, res_membstring);
						test_ok = FALSE;
					}
					else print_check_ok();
				}
				else {
					print_check_missing(req_membstring);
					test_ok = FALSE;
				}
				
				g_free(req_membstring);
				g_free(res_membstring);
			}
			g_strfreev(members);
		}
		g_object_unref(req_reader);
	}
	else test_ok = FALSE;
	
	g_object_unref(req_parser);
	g_object_unref(res_parser);
	return test_ok;
}

/**
* Replace value of a required member in the sent JSON at given index
* in the list of required members. Gets the member name and corresponding
* jsonreply_t from which the three member field values are read:
* (search_file, search_member and root_Task). These fields tell from
* which file response, which member and whether sub-element is required to be used.
* Calls get_value_of_member() to get values from JSON. And for setting
* the value calls set_value_of_member().
*
* @param filetable GHashTable of testfile_t structs from which the file is searched
* @param tfile Current testfile_t containing the member to be replaced
* @param index Position of member name and JSON at testfile_t structures
*
* @return TRUE when value was replaced
*/
gboolean replace_required_member(GHashTable* filetable, testfile* tfile, gint index) {

	if(!filetable || !tfile) return FALSE;
	
	gboolean rval = TRUE;
	gchar *search_file = NULL;	
	gchar *search_member = NULL;
	gchar *search_root = NULL;
	gboolean root_task = FALSE;
	
	// Get member to be replaced
	const gchar* member = (gchar*)g_slist_nth_data(tfile->required,index);
	
	// Get the json holding the details for this parameter
	jsonreply* info = (jsonreply*)g_slist_nth_data(tfile->reqinfo,index);
	
	// Found json
	if(info) {
#ifdef G_MESSAGES_DEBUG
		g_print("member %s info: %s\n",member,info->data);
#endif
		
		// Get path and method from file
		search_file = get_value_of_member(info,"search_file",NULL);
		search_member = get_value_of_member(info,"search_member",NULL);
		search_root = get_value_of_member(info,"root_task",NULL);
		if(search_root && g_strcmp0(search_root,"yes") == 0) root_task = TRUE;
		
		// Get the file
		testfile* req_file = (testfile*)g_hash_table_find(filetable,
			(GHRFunc)find_from_hash_table, 
			search_file);

		// Something to search for?
		if(search_member) {
			gchar* new_value = get_value_of_member(
				req_file->recv,
				search_member,
				root_task ? "root_task" : NULL);
	
			// Create new json using the "value" and save it
			if(new_value && set_value_of_member(tfile->send, member, new_value)) {
#ifdef G_MESSAGES_DEBUG
				g_print("Replaced member %s value to %s\n",member,new_value);
#endif
				rval = TRUE;
			}
		g_free(new_value);
		}
	}
	else rval = FALSE;
	
	
	g_free(search_file);
	g_free(search_root);
	g_free(search_member);
	
	return rval;
}


/**
* Add value of a required member in the testfile replace hash table.
* Gets the member name and corresponding jsonreply_t from which the 
* three member field values are read: (search_file, search_member 
* and root_Task). These fields tell from which file response, which
* member and whether sub-element is required to be used.
*
* Calls get_value_of_member() to get values from JSON.
*
* @param filetable GHashTable of testfile_t structs from which the file is searched
* @param tfile Current testfile_t containing the member to be replaced
* @param index Position of member name and JSON at testfile_t structures
*
* @return TRUE when value was added to hash table (or replaced a value of existing)
*/
gboolean add_required_member_value_to_list(GHashTable* filetable, testfile* tfile, gint index) {

	if(!filetable || !tfile) return FALSE;
	
	// List empty
	if(!tfile->required) return TRUE;
	
	gboolean rval = TRUE;
	gchar *search_file = NULL;	
	gchar *search_member = NULL;
	gchar *search_root = NULL;
	gboolean root_task = FALSE;
	
	// Get member to be replaced
	const gchar* member = (gchar*)g_slist_nth_data(tfile->required,index);
	
	// Get the json holding the details for this parameter
	jsonreply* info = (jsonreply*)g_slist_nth_data(tfile->reqinfo,index);
	
	// Found json
	if(info) {
#ifdef G_MESSAGES_DEBUG
		g_print("member %s info: %s\n",member,info->data);
#endif
		
		// Get path and method from file
		search_file = get_value_of_member(info,"search_file",NULL);
		search_member = get_value_of_member(info,"search_member",NULL);
		search_root = get_value_of_member(info,"root_task",NULL);
		if(search_root && g_strcmp0(search_root,"yes") == 0) root_task = TRUE;
		
		// Get the file
		testfile* req_file = (testfile*)g_hash_table_find(filetable,
			(GHRFunc)find_from_hash_table, 
			search_file);

		// Something to search for?
		if(search_member) {
			gchar* new_value = get_value_of_member(
				req_file->recv,
				search_member,
				root_task ? "root_task" : NULL);
	
			// Create new json using the "value" and save it
			if(new_value) {
				g_hash_table_insert(tfile->replace,g_strdup(member),g_strdup(new_value));
				rval = TRUE;
			}
		g_free(new_value);
		}
	}
	else rval = FALSE;
	
	
	g_free(search_file);
	g_free(search_root);
	g_free(search_member);
	
	return rval;
}

/**
* Replace value of a member requiring more information from the server.
* The JSON having the details is read at index of tfile structures and
* an request is sent to url+"path" field in the JSON using "method". The
* reply to this response contains a member field matching one at index in
* the list of getinfo-member fields in tfile. 
*
* Calls get_value_of_member() to read the pfererences from getinfo JSON.
* Calls http_post() to send the request (most likely GET).
* Calls get_value_of_member() to get the value from the server response.
* Calls set_value_of_member() to replace the member value.
*
* @param tfile Current testfile_t containing the member to be replaced
* @param index Position of member name and JSON at testfile_t structures
* @param url Base url to which request is sent
*
* @return TRUE when value was found in reply and replaced
*/
gboolean replace_getinfo_member(testfile* tfile, gint index, const gchar* url) {

	if(!tfile  || !url) return FALSE;
	
	gboolean rval = TRUE;
	// Get member to be replaced
	const gchar* member = (gchar*)g_slist_nth_data(tfile->moreinfo,index);
	
	// Get the json holding the details for this parameter
	jsonreply* infosend = (jsonreply*)g_slist_nth_data(tfile->infosend,index);
	jsonreply* inforecv = NULL;
	
	// Found json
	if(infosend) {
	
		// Get path and method from file
		gchar* infopath = get_value_of_member(infosend,"path",NULL);
		gchar* method = get_value_of_member(infosend,"method",NULL);
		
		// Construct url
		gchar* infourl = g_strjoin("/",url,infopath,NULL);
						
		// Send an empty json to server to retrieve information
		inforecv = http_post(infourl,NULL,method);
		
		// Search the value and replace it
		gchar* value = get_value_of_member(inforecv,"guid",NULL);
		if(value && set_value_of_member(tfile->send,member,value)) {
#ifdef G_MESSAGES_DEBUG
				g_print("Replaced member %s value to %s\n",member,value);
#endif
			rval = TRUE;
		}
		
		// Add result to list
		tfile->inforecv = g_slist_append(tfile->inforecv,inforecv);
		
		g_free(infopath);
		g_free(method);
		g_free(infourl);
		g_free(value);
	}
	else rval = FALSE;
	return rval;
}


/**
* Add a value of a member requiring more information from the server
* to the testfile replace hash table. The JSON having the details is
* read at index of tfile structures and an request is sent to url+"path"
* field in the JSON using "method". The reply to this response contains
* a member field matching one at index in the list of getinfo-member
* fields in tfile. This new member-value pair is added to the testfile
* replace hash table where member is the key.
*
* Calls get_value_of_member() to read the pfererences from getinfo JSON.
* Calls http_post() to send the request (most likely GET).
* Calls get_value_of_member() to get the value from the server response.
*
* @param tfile Current testfile_t containing the member to be replaced
* @param index Position of member name and JSON at testfile_t structures
* @param url Base url to which request is sent
*
* @return TRUE when value was found in reply and added to/replaced in hash table
*/
gboolean add_getinfo_member_value_to_list(testfile* tfile, gint index, const gchar* url) {

	if(!tfile  || !url) return FALSE;

	// List empty
	if(!tfile->moreinfo) return TRUE;
	
	gboolean rval = TRUE;
	
	// Get member to be replaced
	const gchar* member = (gchar*)g_slist_nth_data(tfile->moreinfo,index);
	
	// Get the json holding the details for this parameter
	jsonreply* infosend = (jsonreply*)g_slist_nth_data(tfile->infosend,index);
	jsonreply* inforecv = NULL;
	
	// Found json
	if(infosend) {
	
		// Get path and method from file
		gchar* infopath = get_value_of_member(infosend,"path",NULL);
		gchar* method = get_value_of_member(infosend,"method",NULL);
		
		// Construct url
		gchar* infourl = g_strjoin("/",url,infopath,NULL);
						
		// Send an empty json to server to retrieve information
		inforecv = http_post(infourl,NULL,method);
		
		// Search the value and replace it
		gchar* value = get_value_of_member(inforecv,"guid",NULL);
		if(value) {
			g_hash_table_insert(tfile->replace,g_strdup(member),g_strdup(value));
			rval = TRUE;
		}
		
		// Add result to list
		tfile->inforecv = g_slist_append(tfile->inforecv,inforecv);
		
		g_free(infopath);
		g_free(method);
		g_free(infourl);
		g_free(value);
	}
	else rval = FALSE;
	return rval;
}
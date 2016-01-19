#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include <json-glib/json-glib.h>

#include "utils.h"
#include "jsonutils.h"
#include "jsonhandler.h"


gchar testpath[] = "tests/";

int main(int argc, char *argv[]) {
	JsonParser *parser = NULL;
	//JsonNode *root = NULL;
	GError *error = NULL;

	parser = json_parser_new();

	json_parser_load_from_file(parser,"tests/john.doe@severa.com/preferences.json",&error);

	if (error) {
		g_print ("Cannot parse %s\n", error->message);
		g_error_free(error);
		g_object_unref(parser);
		return EXIT_FAILURE;
	}

	//root = json_parser_get_root(parser);

	JsonReader *reader = json_reader_new (json_parser_get_root (parser));

	json_reader_read_member(reader,"tests");

	if(json_reader_is_object(reader)) {
		
	}
	if(json_reader_is_array(reader)) {
		for(gint i = 0; i < json_reader_count_elements(reader); i++)
		{
			json_reader_read_element(reader,i);
			g_print("Members: %d\n",json_reader_count_members(reader));

			const gchar *url = get_json_member_string(reader,"URL");
			const gchar *name = get_json_member_string(reader,"testname");
			
			g_print("Test \"%s\" URL = %s\n",name,url);

			json_reader_read_member(reader,"files");
			if(json_reader_is_array(reader)) {
				g_print("%d files:\n",json_reader_count_elements(reader));
				for(gint fileidx = 0; fileidx < json_reader_count_elements(reader); fileidx++) {
					json_reader_read_element(reader,fileidx);
					
					const gchar *id = get_json_member_string(reader,"id");
					const gchar *file = get_json_member_string(reader,"file");
					const gchar *path = get_json_member_string(reader,"path");
					const gchar *method = get_json_member_string(reader,"method");
					
					if(g_strcmp0(id,"login") == 0 || string_is_integer(id)) {
					
						g_print("\tid :%s, file:%s, path:%s, method:%s\n",id,file,path,method);
					}
					json_reader_end_element(reader);
				}
				g_print("\n");
			}

			json_reader_end_element(reader);
		}
	}
	json_reader_end_member(reader);

	
	/*
		Read preferences for given user
		For each test in array, new struct in list
			For each file in test do a hashmap with id
				Conduct each test by uploading json
					If json has {parent} markings, use values from id 0 (case creation
					file, response must be saved (load from data)
					 - if none found, abort
				Verify from result that values in json are ok
	*/
	g_object_unref(reader);
	g_object_unref(parser);

	return EXIT_SUCCESS;

}

#include "tests.h"
#include "connectionutils.h"

static GSList *test_sequence = NULL;

/**
 * Placeholder for initialization
 */
void tests_initialize(testcase* test) {
	// Initialize http with encoding
	http_init(test->encoding);
}

/**
* Clear the test environment. Currently; clear test sequence
*/
void tests_destroy(testcase* test) {
	g_slist_free_full(test_sequence,(GDestroyNotify)free_key);
	test_sequence = NULL;
	
	g_hash_table_foreach(test->files,(GHFunc)testcase_reset_file,NULL);
	
	// Cleanup http
	http_close();
}

/**
* Run all tests of the specified user.
* @param username User whose test is to run
* @param test Test structure containing test details
* 
* @return Result of tests, TRUE if all were verified
*/
gboolean tests_run_test(gchar* username, testcase* test) {

	gboolean rval = TRUE;

	// Establish path to test
	gchar* testpath = tests_make_path_for_test(username,test);

	// Check which fields from the case creation reply have to be stored for future use
	// This is done after loading the testfile in test_conduct_tests() to save resources
	//g_hash_table_foreach(test->files, (GHFunc)tests_check_fields_from_testfiles, testpath);
	
	// Create the sequence of sending tests (json files as charstring data)
	tests_build_test_sequence(test);
	
	// Set list of integer member fields for jsonutils to use
	set_integer_fields(test->intfields);

	// Do tests
	rval = tests_conduct_tests(test,testpath);

	tests_unload_tests(test,testpath);
		
	g_free(testpath);
	
	return rval;
}

/**
* Do all tests according to the testing sequence
* @param test Test details
* @testpath Base path of tests
* @return TRUE when all tests were verified ok
*/
gboolean tests_conduct_tests(testcase* test, gchar* testpath) {

	if(!test || !testpath) return FALSE;

	gboolean rval = TRUE;

	// Go through the test sequence
	for(gint testidx = 0; testidx < g_slist_length(test_sequence); testidx++) {

		// Get the item in test sequence
		gchar* searchparam = g_slist_nth_data(test_sequence,testidx);
		
#ifdef G_MESSAGES_DEBUG
		g_print("Press enter to continue with test \"%s\" file id=\"%s\"",test->name,searchparam);
		gchar c = '0';
		while(c != '\n') c = getc(stdin);
#endif
		// Get the data with the searchparameter from hash table
		testfile* tfile = (testfile*)g_hash_table_find(test->files,
			(GHRFunc)find_from_hash_table, 
			searchparam);
		
		// Read any other file except Empty.json	
		if(g_strcmp0(tfile->file,"Empty.json") != 0) {
			// Create path for the file to be read
			gchar* filepath = g_strjoin("/",testpath,tfile->file,NULL);
		
			JsonParser *parser = json_parser_new();
		
			// Read json detailed by this data (stucture)
			if(!load_json_from_file(parser, filepath)) return FALSE;
			
			tests_check_fields_from_loaded_testfile(parser, tfile, testpath);
		
			// Establish a generator to get the character representation
			JsonGenerator *generator = json_generator_new();
			json_generator_set_root(generator, json_parser_get_root(parser));
	
			// Create new jsonreply and set it to contain json as string data
			tfile->send = jsonreply_initialize();
			tfile->send->data = json_generator_to_data(generator,&(tfile->send->length));
		
			g_object_unref(generator);
			g_object_unref(parser);
		}
		
		// If path contains {id} it needs to be replaced with case id
		if(g_strrstr(tfile->path,"{id}")) {
		
			// Get case file
			testfile* temp = (testfile*)g_hash_table_find(test->files,
				(GHRFunc)find_from_hash_table, 
				"0");
			
			// Get case id
			gchar* caseid = get_value_of_member(temp->recv,"guid",NULL);
			
			if(caseid) {
				
				// Tokenize path
				gchar** split_path = g_strsplit(tfile->path,"/",5);
				
				// Go through the tokens and replace {id} with case id
				for(gint splitidx = 0; split_path[splitidx] ; splitidx++) {
					if(g_strcmp0(split_path[splitidx],"{id}") == 0) {
						g_free(split_path[splitidx]);
						split_path[splitidx] = caseid;
					}
				}
				
				// Replace the path with new
				g_free(tfile->path);
				tfile->path = g_strjoinv("/",split_path);
				g_strfreev(split_path);
			}
		}
		
		// Create url
		gchar* url = g_strjoin("/",test->URL,tfile->path,NULL);
		
#ifdef G_MESSAGES_DEBUG
		g_print("Conducting test id \"%s\"\n",test->name);
#endif
				
		// First is login, it is always first in the list
		if(testidx == 0) {
			tfile->recv = http_post(url,tfile->send,tfile->method);
			if(tfile->recv) {
				gchar* token = get_value_of_member(tfile->recv,"token",NULL);
				set_token(token);
				g_free(token);
			}
			else rval = FALSE;
		}
		
		// Case creation is second
		else if(testidx == 1) {
			tfile->recv = http_post(url,tfile->send,tfile->method);
			if(tfile->recv && verify_server_response(tfile->send,tfile->recv)) {
				g_print ("Case added correctly\n\n\n");
			}
			else rval = FALSE;
		}
		
		// From third start the tests, here the required fields are checked and replaced
		else {
			gint index = 0;
			// Go through all fields having {parent} as value
			for(index = 0; index < g_slist_length(tfile->required); index++) {
				//replace_required_member(test->files,tfile,index);
				
				// Use new function just to add member-new value pairs to hash table
				add_required_member_value_to_list(test->files,tfile,index);
			}
			
			// Go through the list of items requiring more info
			for(gint index = 0; index < g_slist_length(tfile->moreinfo); index++) {
				//replace_getinfo_member(tfile,index,test->URL);
				
				// Use new function just to add member-new value pairs to hash table
				add_getinfo_member_value_to_list(tfile,index,test->URL);
			}
			
			// Replace all values in the jsonreply_t data using the member-value
			// pairs in the replace hash table
			set_values_of_all_members(tfile->send, tfile->replace);

			tfile->recv = http_post(url,tfile->send,tfile->method);
			
			// If there is something to verify
			if(tfile->send) {
				g_print("Verifying test id \"%s\" (file: %s):\n",tfile->id,tfile->file);
				if(verify_server_response(tfile->send,tfile->recv)) {
					g_print ("Test id \"%s\" was added correctly\n",tfile->id);
				}
				else {
					g_print("Test id \"%s\" was not added correctly\n", tfile->id);
					rval = FALSE;
				}
				g_print("\n\n");
			}
		}

		g_free(url);	
	}
	return rval;
}

/** 
* Unload (or DELETE) tests from server, done in reverse order starting from last test
* @param test Test details
* @testpath base path to tests
*/
void tests_unload_tests(testcase* test,gchar* testpath) {
	
	jsonreply *deldata = NULL;
	jsonreply *delresp = NULL;
	gchar *url = NULL;
	gchar *value = NULL;
	
	// Go through the sequence in reverse
	for(gint testidx = g_slist_length(test_sequence) -1 ; testidx >= 0; testidx--) {

		// First the login information need to be added, then tasks in number order
		gchar* searchparam = g_slist_nth_data(test_sequence,testidx);
		
#ifdef G_MESSAGES_DEBUG
		g_print("Press enter to DELETE test \"%s\" file id=\"%s\"",test->name,searchparam);
		gchar c = '0';
		while(c != '\n') c = getc(stdin);
#endif
		
		// Get item in test sequence
		testfile* tfile = (testfile*)g_hash_table_find(test->files,
			(GHRFunc)find_from_hash_table, 
			searchparam);
		
		// If we got a reply we can get all details
		if(tfile->recv) {
		
			// First (here last) is login, it is always first in the list
			if(testidx == 0) {
				value = get_value_of_member(tfile->recv,"user_guid",NULL);

				deldata = create_delete_reply("user_guid",value);
			
				if(deldata && value) {
					url = g_strjoin("/",test->URL,"SignOut",value,NULL);
					delresp = http_post(url,deldata,"GET");
				}
			}
		
			// Rest in reverse order
			else if(tfile->need_delete){
				
				value = get_value_of_member(tfile->recv,"guid",NULL);

				if(value) {
					deldata = create_delete_reply("guid",value);			
					url = g_strjoin("/",test->URL,tfile->path,value,NULL);
					delresp = http_post(url,deldata,"DELETE");
				}
			}
		}
		g_free(value);
		g_free(url);
		free_jsonreply(delresp);
		free_jsonreply(deldata);
	}
	
}

/**
* Create sequence in which tests are conducted
* @param test Test details
*/
void tests_build_test_sequence(testcase* test) {
	// First file with login
	for(gint testidx = 0; testidx < g_hash_table_size(test->files); testidx++) {
		
		// First the login information need to be added, then tasks in number order
		gchar* searchparam = (testidx == 0 ? g_strdup("login") : g_strdup_printf("%d",testidx-1));
		
		// Get the data with the searchparameter from hash table
		testfile* tfile = (testfile*)g_hash_table_find(test->files,
			(GHRFunc)find_from_hash_table, 
			searchparam);
					
		// First is login, it is always first in the list
		if(testidx == 0) test_sequence = g_slist_prepend(test_sequence,g_strdup(tfile->id));
		// Rest are added in order after login credentials
		else test_sequence = g_slist_append(test_sequence,g_strdup(tfile->id));
		g_free(searchparam);
	}
}

/**
* Checks the fields from the files defined for this test that which contain {parent} and {getinfo}
* values. These member names containing such values are added to the lists of the test files to be
* replaced later when the particular test is conducted. 
*
* This is called only by the hash table foreach function and is not meant to be used otherwise.
* 
* @param key - Key in hash table
* @param value - Value corresponding to the key in hashtable
* @param testpath - Base path to tests
*/
void tests_check_fields_from_testfiles(gpointer key, gpointer value, gpointer testpath) {

	JsonParser *parser = json_parser_new();
	
	// Cast the input value to testfile and create path based on filename
	testfile* tfile = (testfile*)value;
	gchar* filepath = g_strjoin("/",testpath,tfile->file,NULL);
	
	// Load json with this filename
	if(!load_json_from_file(parser, filepath)) return;
		
	JsonReader *reader = json_reader_new (json_parser_get_root (parser));

	gchar** members = json_reader_list_members(reader);
	
	// Go through all members of this json
	for(gint membidx = 0; members[membidx] != NULL; membidx++) {

		// Get the value of current member
		gchar* membstring = get_json_member_string(reader,members[membidx]);
		
		// Requires information from other file
		if(g_strcmp0(membstring,"{parent}") == 0) {
		
			// Add member name to list
			tfile->required = g_slist_append(tfile->required,g_strdup(members[membidx]));
			
			JsonParser *par_parser = json_parser_new();
			
			// Create file offering more information
			gchar* par_infopath = g_strjoin(".",filepath,"info",members[membidx],"json",NULL);
			
			if(load_json_from_file(par_parser,par_infopath)) {
				JsonGenerator *par_generator = json_generator_new();
				json_generator_set_root(par_generator, json_parser_get_root(par_parser));
				
				// Initialize struct for the new json and store json
				jsonreply* info = jsonreply_initialize();
				info->data = json_generator_to_data(par_generator,&(info->length));
				
				// To verify that this item is in correct position in the list 
				// and corresponds to the member string location
				gint add_position = g_slist_length(tfile->required) - 1;
				tfile->reqinfo = g_slist_insert(tfile->reqinfo,info,add_position);

				g_object_unref(par_generator);
			}
			g_free(par_infopath);
			g_object_unref(par_parser);
		}
		
		// Requires more information from the server
		if(g_strcmp0(membstring,"{getinfo}") == 0) {
		
			// Add member name to list
			tfile->moreinfo = g_slist_append(tfile->moreinfo,g_strdup(members[membidx]));
			
			JsonParser *info_parser = json_parser_new();
			
			// Create path to the file offering more information
			gchar* infopath = g_strjoin(".",filepath,"getinfo",members[membidx],"json",NULL);
			
			// Load the json file
			if(load_json_from_file(info_parser,infopath)) {
				JsonGenerator *info_generator = json_generator_new();
				json_generator_set_root(info_generator, json_parser_get_root(info_parser));
				
				// Initialize struct for the new json and store json
				jsonreply* info = jsonreply_initialize();
				info->data = json_generator_to_data(info_generator,&(info->length));
				
				// To verify that this item is in correct position in the list 
				// and corresponds to the member string location
				gint add_position = g_slist_length(tfile->moreinfo) - 1;
				tfile->infosend = g_slist_insert(tfile->infosend,info,add_position);

				g_object_unref(info_generator);
			}
			g_free(infopath);
			g_object_unref(info_parser);
		}
		g_free(membstring);
	}
	
	//g_print("\n");	
	g_strfreev(members);
	g_object_unref(reader);
	g_object_unref(parser);
	parser = NULL;
	g_free(filepath);
}

/**
* Checks the fields from the file loaded in parser for presence of {parent} and
* {getinfo} values. These member names containing such values are added to the
* lists of the test files to be replaced later. 
* 
* @param parser - Parser where file is loaded
* @param tfile - testfile containing details
* @param testpath - Base path to tests
*/
void tests_check_fields_from_loaded_testfile(JsonParser *parser,testfile *tfile, gchar* testpath) {
		
	gchar* filepath = g_strjoin("/",testpath,tfile->file,NULL);
	JsonReader *reader = json_reader_new (json_parser_get_root (parser));

	gchar** members = json_reader_list_members(reader);
	
	// Go through all members of this json
	for(gint membidx = 0; members[membidx] != NULL; membidx++) {

		// Get the value of current member
		gchar* membstring = get_json_member_string(reader,members[membidx]);
		
		// Requires information from other file
		if(g_strcmp0(membstring,"{parent}") == 0) {
		
			// Add member name to list
			tfile->required = g_slist_append(tfile->required,g_strdup(members[membidx]));
			
			JsonParser *par_parser = json_parser_new();
			
			// Create file offering more information
			gchar* par_infopath = g_strjoin(".",filepath,"info",members[membidx],"json",NULL);
			
			if(load_json_from_file(par_parser,par_infopath)) {
				JsonGenerator *par_generator = json_generator_new();
				json_generator_set_root(par_generator, json_parser_get_root(par_parser));
				
				// Initialize struct for the new json and store json
				jsonreply* info = jsonreply_initialize();
				info->data = json_generator_to_data(par_generator,&(info->length));
				
				// To verify that this item is in correct position in the list 
				// and corresponds to the member string location
				gint add_position = g_slist_length(tfile->required) - 1;
				tfile->reqinfo = g_slist_insert(tfile->reqinfo,info,add_position);

				g_object_unref(par_generator);
			}
			g_free(par_infopath);
			g_object_unref(par_parser);
		}
		
		// Requires more information from the server
		if(g_strcmp0(membstring,"{getinfo}") == 0) {
		
			// Add member name to list
			tfile->moreinfo = g_slist_append(tfile->moreinfo,g_strdup(members[membidx]));
			
			JsonParser *info_parser = json_parser_new();
			
			// Create path to the file offering more information
			gchar* infopath = g_strjoin(".",filepath,"getinfo",members[membidx],"json",NULL);
			
			// Load the json file
			if(load_json_from_file(info_parser,infopath)) {
				JsonGenerator *info_generator = json_generator_new();
				json_generator_set_root(info_generator, json_parser_get_root(info_parser));
				
				// Initialize struct for the new json and store json
				jsonreply* info = jsonreply_initialize();
				info->data = json_generator_to_data(info_generator,&(info->length));
				
				// To verify that this item is in correct position in the list 
				// and corresponds to the member string location
				gint add_position = g_slist_length(tfile->moreinfo) - 1;
				tfile->infosend = g_slist_insert(tfile->infosend,info,add_position);

				g_object_unref(info_generator);
			}
			g_free(infopath);
			g_object_unref(info_parser);
		}
		g_free(membstring);
	}
		
	g_strfreev(members);
	g_free(filepath);
	g_object_unref(reader);
}

/**
* Make path for the tests using username and test name
*
* @param username USername to use in path
* @param test Test details
*
* @return Path to this tests' base
*/
gchar* tests_make_path_for_test(gchar* username, testcase* test) {
	gchar* filepath = g_strjoin("/",TESTPATH,username,test->name,NULL);
	return filepath;
}
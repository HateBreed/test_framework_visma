#include "tests.h"
#include "connectionutils.h"

static GSList *test_sequence = NULL;

void print_hashtable_strings(gpointer key, gpointer value, gpointer userdata) {
	g_print("\"%s\":\"%s\"\n",(gchar*)key, (gchar*)value);
}

gboolean find_from_hash_table(gpointer key, gpointer value, gpointer user_data) {
	if(!key || !user_data) return FALSE;
	return g_strcmp0((gchar*)key,(gchar*)user_data) == 0 ? TRUE : FALSE;
}

void tests_initialize() {

}

void tests_destroy() {
	g_slist_free_full(test_sequence,(GDestroyNotify)free_key);
}

gboolean tests_run_test(gchar* username, testcase* test) {

	http_init();
	gchar* testpath = tests_make_path_for_test(username,test);

	// Check which fields from the case creation reply have to be stored for future use
	g_hash_table_foreach(test->files, (GHFunc)tests_check_fields_from_testfiles, testpath);
	
	// Create the sequence of sending tests (json files as charstring data)
	tests_build_test_sequence(test);
	
	tests_conduct_tests(test,testpath);
	
	tests_unload_tests(test,testpath);
	
	g_free(testpath);
	http_close();
	
	return TRUE;
}

void tests_conduct_tests(testcase* test, gchar* testpath) {

	// Go through the test sequence
	for(gint testidx = 0; testidx < g_slist_length(test_sequence); testidx++) {
		JsonParser *parser = json_parser_new();
		
		// Get the item in test sequence
		gchar* searchparam = g_slist_nth_data(test_sequence,testidx);
		
		// Get the data with the searchparameter from hash table
		testfile* tfile = (testfile*)g_hash_table_find(test->files,
			(GHRFunc)find_from_hash_table, 
			searchparam);
		
		// Create path for the file to be read
		gchar* filepath = g_strjoin("/",testpath,tfile->file,NULL);
		
		// Read json detailed by this data (stucture)
		if(!load_json_from_file(parser, filepath)) return;	
		
		// Establish a generator to get the character representation
		JsonGenerator *generator = json_generator_new();
		json_generator_set_root(generator, json_parser_get_root(parser));
	
		// Create new jsonreply and set it to contain json as string data
		tfile->send = jsonreply_initialize();
		tfile->send->data = json_generator_to_data(generator,&(tfile->send->length));
		
		// Create url
		gchar* url = NULL;
		
		if(g_strrstr(tfile->path,"{id}")) {
			testfile* temp = (testfile*)g_hash_table_find(test->files,
				(GHRFunc)find_from_hash_table, 
				"0");
			gchar* caseid = get_value_of_member(temp->recv,"guid",NULL);
			if(caseid) {
				gchar** split_path = g_strsplit(tfile->path,"/",5);
				for(gint splitidx = 0; split_path[splitidx] ; splitidx++) {
					if(g_strcmp0(split_path[splitidx],"{id}") == 0) {
						g_free(split_path[splitidx]);
						split_path[splitidx] = caseid;
					}
					g_print("%s - ",split_path[splitidx]);
				}
				
				g_free(tfile->path);
				tfile->path = g_strjoinv("/",split_path);
				g_strfreev(split_path);
			}
		}
		
		url = g_strjoin("/",test->URL,tfile->path,NULL);
				
		// First is login, it is always first in the list
		if(testidx == 0) {
			tfile->recv = http_post(url,tfile->send,tfile->method);
			gchar* token = get_value_of_member(tfile->recv,"token",NULL);
			set_token(token);
			g_free(token);
		}
		
		// Case creation is second
		else if(testidx == 1) {
			tfile->recv = http_post(url,tfile->send,tfile->method);
		}
		
		// From third start the tests, here the required fields are checked and replaced
		else {
			for(gint regidx = 0; regidx < g_slist_length(tfile->required); regidx++) {
			
				gchar *search_file = NULL;	
				gchar *search_member = NULL;
				gchar *value = NULL;
				gboolean root_task = FALSE;	
				
				// Get member to be replaced
				gchar* member = (gchar*)g_slist_nth_data(tfile->required,regidx);
				
				// TODO rethink this
				// If task guid is required, it can be gotten from root task of the case
				if(g_strcmp0(member,"task_guid") == 0) {
					root_task = TRUE;
					search_member = g_strdup("guid");
					search_file = g_strdup("0");
				}
				// If worktype guid is required it can be gotten from root task of the case
				else if(g_strcmp0(member,"worktype_guid") == 0) {
					root_task = TRUE;
					search_member = g_strdup("default_worktype_guid");
					search_file = g_strdup("0");
				}
				
				// If user guid is required it can be gotten from the login info
				else if(g_strcmp0(member,"user_guid") == 0) {
					search_member = g_strdup(member);
					search_file = g_strdup("login");
				}
				
				// Otherwise the parameter to be searched is the last string after "_"
				// and details are in the case file
				else {
					gchar **search_members = g_strsplit(member,"_",3);
					if(search_members[1]) search_member = g_strdup(search_members[1]);
					g_strfreev(search_members);
					search_file = g_strdup("0");
				}
				
				// Get the file
				testfile* temp = (testfile*)g_hash_table_find(test->files,
					(GHRFunc)find_from_hash_table, 
					search_file);

				// Something to search for?
				if(search_member) {
					gchar* value = 
						get_value_of_member(temp->recv,search_member, root_task ? "root_task" : NULL);
								
					// Create new json using the "value" and save it
					if(set_value_of_member(tfile->send, member, value)) {
						g_print(" ");
					}
				}
				g_free(search_file);
				g_free(value);
				g_free(search_member);
				
			}
			
			// Go through the list of items requiring more info
			for(gint moreidx = 0; moreidx < g_slist_length(tfile->moreinfo); moreidx++) {
			
				// Get member to be replaced
				gchar* member = (gchar*)g_slist_nth_data(tfile->moreinfo,moreidx);
				
				// Get the json holding the details for this parameter
				jsonreply* infosend = (jsonreply*)g_slist_nth_data(tfile->infosend,moreidx);
				jsonreply* inforecv = NULL;
				
				// Found json
				if(infosend) {
				
					// Get path and method from file
					gchar* infopath = get_value_of_member(infosend,"path",NULL);
					gchar* method = get_value_of_member(infosend,"method",NULL);
					
					// Construct url
					gchar* infourl = g_strjoin("/",test->URL,infopath,NULL);
									
					// Send an empty json to server to retrieve information
					inforecv = http_post(infourl,NULL,method);
					
					// Search the value and replace it
					gchar* value = get_value_of_member(inforecv,"guid",NULL);
					if(value) set_value_of_member(tfile->send,member,value);
					
					// Add result to list
					tfile->inforecv = g_slist_append(tfile->inforecv,inforecv);
					
					g_free(infopath);
					g_free(method);
					g_free(infourl);
					g_free(value);
				}
			}
			tfile->recv = http_post(url,tfile->send,tfile->method);
		}

		g_free(url);

		g_object_unref(generator);
		g_object_unref(parser);	
	}
}

void tests_unload_tests(testcase* test,gchar* testpath) {
	
	jsonreply *deldata = NULL;
	jsonreply *delresp = NULL;
	gchar* url = NULL;
	
	// Go through the sequence in reverse
	for(gint testidx = g_slist_length(test_sequence) -1 ; testidx >= 0; testidx--) {

		// First the login information need to be added, then tasks in number order
		gchar* searchparam = g_slist_nth_data(test_sequence,testidx);
		
		// Get item in test sequence
		testfile* tfile = (testfile*)g_hash_table_find(test->files,
			(GHRFunc)find_from_hash_table, 
			searchparam);
		
		// If we got a reply we can get all details
		if(tfile->recv) {
		
			// First (here last) is login, it is always first in the list
			if(testidx == 0) {
				gchar *value = get_value_of_member(tfile->recv,"user_guid",NULL);

				deldata = create_delete_reply("user_guid",value);
			
				if(deldata && value) {
					url = g_strjoin("/",test->URL,"SignOut",value,NULL);
					delresp = http_post(url,deldata,"GET");
					g_free(value);
					g_free(url);
					free_jsonreply(delresp);
				}
				free_jsonreply(deldata);
			
			}
		
			// Rest in reverse order
			else {
				gchar *value = get_value_of_member(tfile->recv,"guid",NULL);

				if(value) {
					deldata = create_delete_reply("guid",value);			
					url = g_strjoin("/",test->URL,tfile->path,value,NULL);
					delresp = http_post(url,deldata,"DELETE");
					g_free(value);
					g_free(url);
					free_jsonreply(delresp);
					free_jsonreply(deldata);
					}
			}
		}
	}
	
}

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

void tests_check_fields_from_testfiles(gpointer key, gpointer value, gpointer testpath) {

	JsonParser *parser = json_parser_new();
	
	// Cast the input value to testfile and create path based on filename
	testfile* tfile = (testfile*)value;
	gchar* filepath = g_strjoin("/",testpath,tfile->file,NULL);
	
	// Load json with this filename
	if(!load_json_from_file(parser, filepath)) return;
		
	JsonReader *reader = json_reader_new (json_parser_get_root (parser));

	gchar** members = json_reader_list_members(reader);
	
	//g_print("File: %s\n",filepath);
	for(gint membidx = 0; members[membidx] != NULL; membidx++) {
		//g_print("%s\n",members[membidx]);
		gchar* membstring = get_json_member_string(reader,members[membidx]);
		
		if(g_strcmp0(membstring,"{parent}") == 0) {
			tfile->required = g_slist_append(tfile->required,g_strdup(members[membidx]));
			//g_print ("Added %s\n", membstring);
		}
		if(g_strcmp0(membstring,"{getinfo}") == 0) {
			tfile->moreinfo = g_slist_append(tfile->moreinfo,g_strdup(members[membidx]));
			
			JsonParser *infoparser = json_parser_new();
			gchar* infopath = g_strjoin(".",filepath,"getinfo",members[membidx],"json",NULL);
			
			if(load_json_from_file(infoparser,infopath)) {
				JsonGenerator *generator = json_generator_new();
				json_generator_set_root(generator, json_parser_get_root(infoparser));
				
				jsonreply* info = jsonreply_initialize();
				info->data = json_generator_to_data(generator,&(info->length));
				//g_print("%s (%s)-> %s\n",members[membidx],tfile->file,info->data);
				
				gint add_position = g_slist_length(tfile->moreinfo) - 1;
				tfile->infosend = g_slist_insert(tfile->infosend,info,add_position);

				g_object_unref(generator);
			}
			g_object_unref(infoparser);
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


gchar* tests_make_path_for_test(gchar* username, testcase* test) {
	gchar* filepath = g_strjoin("/",TASKPATH,username,test->name,NULL);
	return filepath;
}
/*
	Select given test from user_preferences
	Create new parser and load each file separately
	Check fields of the file for {parent} unless 0
	Create JsonGenerator from parser root JsonNode
	use connectionutils to establish http connection with curl to the URL of test
	get data from JsonGenerator to be sent to server
	Get the response from connectionutils
	Check the same fields that were in the file
	If id == 0, store the json/required fields in a hashtable
	To get required fields while processing first all files:
		Get all member names having {parent}
		Add each of the member names into hashtable as keys with "" content
		When creating the case (with id 0) get each of these member names from
			the response and store them into the hashtable where these can be
			retrieved when required
			
	Last, delete all file contents that were sent (with DELETE method) and
	finally delete the case
*/
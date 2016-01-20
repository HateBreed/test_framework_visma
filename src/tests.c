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

	gchar* testpath = tests_make_path_for_test(username,test);

	// Check which fields from the case creation reply have to be stored for future use
	g_hash_table_foreach(test->files, (GHFunc)tests_check_fields_from_testfiles, testpath);
	
	// Create the sequence of sending tests (json files as charstring data)
	tests_build_test_sequence(test);
	
	tests_conduct_tests(test,testpath);
	
	g_free(testpath);
	
	return TRUE;
}

void tests_conduct_tests(testcase* test, gchar* testpath) {

	// First file with login
	for(gint testidx = 0; testidx < g_slist_length(test_sequence); testidx++) {
		JsonParser *parser = json_parser_new();
		
		// First the login information need to be added, then tasks in number order
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
	
		// Create new test item and set it to contain json as string data
		tfile->send = jsonreply_initialize();
		tfile->send->data = json_generator_to_data(generator,&(tfile->send->length));
		
		//g_print("%d->%s %s %s\n",testidx,searchparam, tfile->id,tfile->file);
		// First is login, it is always first in the list
		if(testidx == 0) {
			//g_print("%d->%s %s %s\n",testidx,searchparam, tfile->id,tfile->file);
			//gchar* url = g_strjoin("/",test->URL,tfile->path,NULL);
			//tfile->recv = http_post(url,tfile->send->data,tfile->send->length,tfile->method);
			//g_print("%s\n",tfile->recv->data);
			//g_free(url);
			g_print("%d->%s %s %s\n",testidx,searchparam, tfile->id,tfile->file);
		}
		// Rest are added in order after login credentials
		//else g_print("%d->%s %s %s\n",testidx,searchparam, tfile->id,tfile->file);
		//g_print("%s\n",tfile->send->data);

		g_object_unref(generator);
		g_object_unref(parser);	
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
		const gchar* membstring = get_json_member_string(reader,members[membidx]);
		
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
				g_print("%s (%s)-> %s\n",members[membidx],tfile->file,info->data);
				
				gint add_position = g_slist_length(tfile->moreinfo) - 1;
				tfile->infosend = g_slist_insert(tfile->infosend,info,add_position);

				g_object_unref(generator);
			}
			g_object_unref(infoparser);
		}
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
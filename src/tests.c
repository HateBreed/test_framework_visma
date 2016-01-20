#include "tests.h"
#include "connectionutils.h"

static GHashTable *required_fields = NULL;
static GSList *test_sequence = NULL;

void testitem_destroy(gpointer data) {
	testitem* item = (testitem*)data;
	g_free(item->id);
	g_free(item->data);
	g_free(item);
}

void print_hashtable_strings(gpointer key, gpointer value, gpointer userdata) {
	g_print("\"%s\":\"%s\"\n",(gchar*)key, (gchar*)value);
}

gboolean find_from_hash_table(gpointer key, gpointer value, gpointer user_data) {
	if(!key || !user_data) return FALSE;
	return g_strcmp0((gchar*)key,(gchar*)user_data) == 0 ? TRUE : FALSE;
}

void tests_initialize() {
	if(!required_fields) required_fields = g_hash_table_new_full(
		(GHashFunc)g_str_hash,
		(GEqualFunc)g_str_equal,
		NULL,
		NULL);
}

void tests_destroy() {
	if(!required_fields) {
		g_hash_table_destroy(required_fields);
		g_hash_table_unref(required_fields);
	}
	g_slist_free_full(test_sequence,(GDestroyNotify)testitem_destroy);
}

gboolean tests_run_test(gchar* username, testcase* test) {

	gchar* testpath = tests_make_path_for_test(username,test);

	// Check which fields from the case creation reply have to be stored for future use
	g_hash_table_foreach(test->files, (GHFunc)tests_check_fields_from_testfiles, testpath);
	// Print the fields
	g_hash_table_foreach(required_fields,(GHFunc)print_hashtable_strings,NULL);
	
	// Create the sequence of sending tests (json files as charstring data)
	build_test_sequence(testpath,test);
	
	g_free(testpath);
	
	return TRUE;
}

void build_test_sequence(gchar* testpath, testcase* test) {
	
	// First file with login
	for(gint testidx = 0; testidx < g_hash_table_size(test->files); testidx++) {
		JsonParser *parser = json_parser_new();
		
		// First the login information need to be added, then tasks in number order
		gchar* searchparam = (testidx == 0 ? g_strdup("login") : g_strdup_printf("%d",testidx-1));
		
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
			test_sequence = g_slist_prepend(test_sequence,g_strdup(tfile->id));
			gchar* url = g_strjoin("/",test->URL,tfile->path,NULL);
			tfile->reply = http_post(url,tfile->send->data,tfile->send->length,tfile->method);
			g_print("%s\n",tfile->reply->data);
			g_free(url);
		}
		// Rest are added in order after login credentials
		else test_sequence = g_slist_append(test_sequence,g_strdup(tfile->id));
	
		g_free(searchparam);
		g_object_unref(generator);
		g_object_unref(parser);
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
			if(!required_fields) tests_initialize();
			if(g_hash_table_insert(required_fields,g_strdup(members[membidx]),""))
				g_print("Added %s as required field (size %d).\n",
					members[membidx],
					g_hash_table_size(required_fields));
			else g_print("%s was already a required field.\n",members[membidx]);
		}
		if(g_strcmp0(membstring,"{getinfo}") == 0) {
			// TODO react to this
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
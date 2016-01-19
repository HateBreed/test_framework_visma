#include "tests.h"

static GHashTable* required_fields = NULL;

gboolean strcheck(gconstpointer a, gconstpointer b) {
	const gchar *stra = (gchar*)a;
	const gchar *strb = (gchar*)b;
	
	if(g_strcmp0(stra,strb) == 0) return TRUE;
	return FALSE;
}

void print_hashtable_strings(gpointer key, gpointer value, gpointer userdata) {
	g_print("\"%s\":\"%s\"\n",(gchar*)key, (gchar*)value);
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
}

gboolean tests_run_test(gchar* username, testcase* test) {

	gchar* testpath = tests_make_path_for_test(username,test);

	g_hash_table_foreach(test->files, (GHFunc)tests_check_fields_from_testfiles, testpath);
	g_print("%d required fields\n",g_hash_table_size(required_fields));
	g_hash_table_foreach(required_fields,(GHFunc)print_hashtable_strings,NULL);
	g_free(testpath);
	
	return TRUE;
}

void tests_check_fields_from_testfiles(gpointer key, gpointer value, gpointer testpath) {

	GError *error = NULL;
	JsonParser *parser = NULL;
	
	testfile* tfile = (testfile*)value;
	gchar* filepath = g_strjoin("/",testpath,tfile->file,NULL);
	g_print("%s id :%s, file:%s, path:%s, method:%s \n",(gchar*)key,tfile->id,tfile->file,tfile->path,tfile->method);
		
	parser = json_parser_new();
	json_parser_load_from_file(parser, filepath, &error);
		
	if (error) {
		g_print ("Cannot parse file \"%s\". Reason: %s\n", filepath, error->message);
		g_error_free(error);
		g_object_unref(parser);
		//return;
	}
		
	JsonReader *reader = json_reader_new (json_parser_get_root (parser));
	
	gchar** members = json_reader_list_members(reader);
	
	g_print("File: %s\n",filepath);
	for(gint membidx = 0; members[membidx] != NULL; membidx++) {
		g_print("%s\n",members[membidx]);
		gchar* membstring = get_json_member_string(reader,members[membidx]);
		
		if(g_strcmp0(membstring,"{parent}") == 0) {
			if(!required_fields) tests_initialize();
			if(g_hash_table_insert(required_fields,g_strdup(members[membidx]),""))
				g_print("Added %s as required field (size %d).\n",
					members[membidx],
					g_hash_table_size(required_fields));
			else g_print("%s was already a required field.\n",members[membidx]);
		}
	}
	
	g_print("\n");	
	g_strfreev(members);
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
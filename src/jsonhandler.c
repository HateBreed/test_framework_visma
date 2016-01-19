#include "jsonhandler.h"
#include "utils.h"

static GHashTable* userlist = NULL;

gboolean add_user(user_preference* preference) {
	if(!userlist) userlist = g_hash_table_new_full(
		NULL,
		NULL, 
		(GDestroyNotify)free_key, 
		(GDestroyNotify)free_all_preferences);
		
	return g_hash_table_insert(userlist, g_strdup(preference->username),preference);
}

user_preference* preference_initialize(gchar* username) {
	if(!username) return NULL;
	
	user_preference* pref = g_new(struct user_preference_t,1);
	pref->username = g_strdup(username);
	pref->parser = json_parser_new();
	pref->tests = g_sequence_new((GDestroyNotify)free_testcase);
	
	return pref;
}

gboolean preference_add_test(user_preference* preference, testcase* test) {
	if(!preference || !test) return FALSE;
	g_sequence_append(preference->tests,test);
	return TRUE;
}

testcase* preference_get_test(user_preference* preference, gchar* testname) {
	if(!preference || !testname) return NULL;
	GSequenceIter* iter = NULL;
	for(iter = g_sequence_get_begin_iter(preference->tests);
		g_sequence_iter_is_end(iter); 
		iter = g_sequence_iter_next(iter)) {
		testcase* test = (testcase*)g_sequence_get(iter);
		if(g_strcmp0(test->name,testname) == 0) return test;
	}
	return NULL;
}

testcase* testcase_initialize(const gchar* URL, const gchar* name) {
	if(!URL || !name) return NULL;
	testcase* test = g_new(struct testcase_t,1);
	test->URL = g_strdup(URL);
	test->name = g_strdup(name);
	test->files = g_hash_table_new_full(
		NULL,
		NULL,
		(GDestroyNotify)free_key,
		(GDestroyNotify)free_testfile);
		
	return test;
}

gboolean testcase_add_file(testcase* test, testfile* file) {
	if(!test || !file) return FALSE;
	if(!test->files) test->files = g_hash_table_new_full(
		NULL,
		NULL,
		(GDestroyNotify)free_key,
		(GDestroyNotify)free_testfile);
		
	return g_hash_table_insert(test->files,g_strdup(file->id),file);
}

testfile* testfile_initialize(const gchar* id, const gchar* file, const gchar* path, const gchar* method) {
	if(!id || !file || !path || !method) return NULL;
	
	testfile* tfile = g_new(struct testfile_t,1);
	tfile->id = g_strdup(id);
	tfile->path = g_strdup(path);
	tfile->file = g_strdup(file);
	tfile->method = g_strdup(method);
	g_print("Added id :%s, file:%s, path:%s, method:%s ",tfile->id,tfile->file,tfile->path,tfile->method);
	return tfile;
}

user_preference* load_preferences(gchar* username,gchar* password) {
	if(!username || !password) return NULL;
	
	GError *error = NULL;
	
	user_preference* preferences = preference_initialize(username);
	
	add_user(preferences);
	
	gchar* prefpath = preference_make_path(preferences);
	
	json_parser_load_from_file(preferences->parser,prefpath,&error);
	
	g_free(prefpath);

	if (error) {
		g_print ("Cannot parse preferences for user \"%s\". Reason: %s\n", username, error->message);
		g_error_free(error);
		g_object_unref(preferences->parser);
		return NULL;
	}
	
	if(read_preferences(preferences)) return preferences;
	else return NULL;
}

gboolean read_preferences(user_preference* preferences) {

	gboolean rval = TRUE;
	
	JsonReader *reader = json_reader_new (json_parser_get_root (preferences->parser));

	json_reader_read_member(reader,"tests");

	if(json_reader_is_array(reader)) {
		for(gint testidx = 0; testidx < json_reader_count_elements(reader); testidx++)
		{
			json_reader_read_element(reader,testidx);
			
			testcase* test = testcase_initialize(
				get_json_member_string(reader,"URL"),
				get_json_member_string(reader,"testname"));
				
			preference_add_test(preferences,test);
				
			g_print("Test \"%s\" URL = %s\n",test->name,test->URL);
			
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
					
						//g_print("\tid :%s, file:%s, path:%s, method:%s ",id,file,path,method);
						testfile* tfile = testfile_initialize(id,file,path,method);
						if(testcase_add_file(test,tfile)) g_print("as new testfile to test %s\n",
							test->name);
						else g_print("replaced old data in test %s\n",
							test->name);
					}
					json_reader_end_element(reader);
				}
				g_print("\n");
			}
			
			json_reader_end_member(reader); // files

			json_reader_end_element(reader); // test at testidx
		}
	}
	else rval = FALSE;
	
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
	
	return rval;
}

void destroy_preferences() {
	if(userlist) {
		g_hash_table_destroy(userlist);
		g_hash_table_unref(userlist);
	}
	else g_print("list empty\n");
	userlist = NULL;
}

void free_all_preferences(gpointer data) {
	user_preference* user_pref = (user_preference*)data;
	g_print ("Removing \"%s\" ...", user_pref->username);
	g_free(user_pref->username);
		
	g_object_unref(user_pref->parser);
		
	g_print("clearing %d tests... ",g_sequence_get_length(user_pref->tests));
	g_sequence_free(user_pref->tests);
		
	g_print("done\n");
	g_free(user_pref);
}

gboolean free_preferences(gchar* username) {
	gpointer *user = NULL;

	if(g_hash_table_lookup_extended(userlist,username,user,NULL)) {

		user_preference *user_pref = (user_preference*)user;
		
		g_print ("Removing \"%s\" ...", user_pref->username);
		g_free(user_pref->username);
		
		g_object_unref(user_pref->parser);
		
		g_print("clearing %d tests... ",g_sequence_get_length(user_pref->tests));
		g_sequence_free(user_pref->tests);
		
		g_print("done\n");
		g_free(user_pref);
		return TRUE;
	}
	else g_print("User \"%s\" was not found\n",username);
	return FALSE;
}


void free_testcase(gpointer data) {
	testcase* test = (testcase*)data;
	g_print("%s with %d files...",test->name,g_hash_table_size(test->files));
	g_free(test->URL);
	g_free(test->name);
	if(test->files) {
		g_hash_table_destroy(test->files);
		test->files = NULL;
	}
	g_free(test);
}

void free_testfile(gpointer data) {
	testfile* file = (testfile*)data;
	g_free(file->id);
	g_free(file->file);
	g_free(file->path);
	g_free(file->method);
	g_free(file);
}


gchar* preference_make_path(user_preference* preference) {
	//const gchar separator = G_DIR_SEPARATOR;
	gchar* path = g_strjoin("/",TASKPATH,preference->username,PREFERENCEFILE,NULL);
	g_print("Path is \"%s\"\n",path);
	return path;
}
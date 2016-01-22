#include "utils.h"

static JsonParser* default_parser = NULL;

void set_parser(JsonParser *parser) {
	if(default_parser) g_object_unref(parser);
	parser = default_parser;
}

JsonParser* get_parser() {
	return default_parser;
}

gboolean string_is_integer(const gchar* string) {
	gint length = strlen(string);
	for(gint strindex = 0; strindex < length; strindex++) {
		if (!g_ascii_isdigit(string[strindex])) return FALSE;
	}
	return TRUE;
}

gboolean find_from_hash_table(gpointer key, gpointer value, gpointer user_data) {
	if(!key || !user_data) return FALSE;
	return g_strcmp0((gchar*)key,(gchar*)user_data) == 0 ? TRUE : FALSE;
}

user_preference* preference_initialize(const gchar* username) {
	if(!username) return NULL;
	
	user_preference* pref = g_new0(struct user_preference_t,1);
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

testcase* preference_get_test(user_preference* preference, const gchar* testname) {
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
	testcase* test = g_new0(struct testcase_t,1);
	test->URL = g_strdup(URL);
	test->name = g_strdup(name);
	test->files = g_hash_table_new_full(
		(GHashFunc)g_str_hash,
		(GEqualFunc)g_str_equal,
		(GDestroyNotify)free_key,
		(GDestroyNotify)free_testfile);
		
	return test;
}

gboolean testcase_add_file(testcase* test, testfile* file) {
	if(!test || !file) return FALSE;
	if(!test->files) test->files = g_hash_table_new_full(
		(GHashFunc)g_str_hash,
		(GEqualFunc)g_str_equal,
		(GDestroyNotify)free_key,
		(GDestroyNotify)free_testfile);
		
	return g_hash_table_insert(test->files,g_strdup(file->id),file);
}

testfile* testfile_initialize(const gchar* _id, const gchar* _file, const gchar* _path, const gchar* _method, gboolean _delete) {
	if(!_id || !_file || !_path || !_method) return NULL;
	
	testfile* tfile = g_new0(struct testfile_t,1);
	tfile->id = g_strdup(_id);
	tfile->path = g_strdup(_path);
	tfile->file = g_strdup(_file);
	tfile->method = g_strdup(_method);
	tfile->need_delete = _delete;
	
	tfile->required = NULL;
	tfile->reqinfo = NULL;
	tfile->moreinfo = NULL;
	tfile->infosend = NULL;
	tfile->inforecv = NULL;

	return tfile;
}

jsonreply* jsonreply_initialize() {
	return g_new0(struct jsonreply_t,1);
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

gboolean free_preferences(GHashTable* userlist, const gchar* username) {
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
	testfile* tfile = (testfile*)data;
		
	g_free(tfile->id);
	g_free(tfile->file);
	g_free(tfile->path);
	g_free(tfile->method);
	
	free_jsonreply(tfile->send);
	free_jsonreply(tfile->recv);
	
	g_slist_free_full(tfile->required,(GDestroyNotify)free_key);
	g_slist_free_full(tfile->moreinfo,(GDestroyNotify)free_key);
	
	g_slist_free_full(tfile->reqinfo,(GDestroyNotify)free_jsonreply);
	g_slist_free_full(tfile->infosend,(GDestroyNotify)free_jsonreply);
	g_slist_free_full(tfile->inforecv,(GDestroyNotify)free_jsonreply);

	g_free(tfile);
}

void free_jsonreply(gpointer data) {
	jsonreply* item = (jsonreply*)data;
	if(item) {
		g_free(item->data);
		g_free(item);
	}
}

void free_key(gpointer data) {
	gchar* str = (gchar*)data;
	g_free(str);
	
}


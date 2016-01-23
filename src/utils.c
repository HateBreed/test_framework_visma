#include "utils.h"

static JsonParser* default_parser = NULL;

/**
* Set parser for current test. This parser could be reused
* to reduce resource consumption. This sets a pointer that
* is not required to be free'd.
*
* Normal way is to set this up at the beginning of testing.
*
* @param parser Pointer to parser 
*/
void set_parser(JsonParser *parser) {
	if(default_parser) g_object_unref(parser);
	parser = default_parser;
}

/**
* Get default_parser for reusing it.
* 
* @return pointer to parser
*/
JsonParser* get_parser() {
	return default_parser;
}

/**
* Check whether string contains integers only.
*
* @param string to be checked
*
* @return TRUE when all are integers (g_ascii_isdigit())
*/
gboolean string_is_integer(const gchar* string) {
	gint length = strlen(string);
	for(gint strindex = 0; strindex < length; strindex++) {
		if (!g_ascii_isdigit(string[strindex])) return FALSE;
	}
	return TRUE;
}

/**
* A functions for running foreach() on GHashTable for searching a particular
* key value from it.
* 
* @param key Current key
* @param value value of the current key
* @param user_data pointer to a gchar* to be searched
*
* @return TRUE when key and user_data match
*/
gboolean find_from_hash_table(gpointer key, gpointer value, gpointer user_data) {
	if(!key || !user_data) return FALSE;
	return g_strcmp0((gchar*)key,(gchar*)user_data) == 0 ? TRUE : FALSE;
}

/**
* Initialize user preference struct using g_new0() with given username.
* Sets up also parser and GSequence for tests.
* 
* @param username Name of the user that is duplicated (g_strdup())
*
* @return Pointer to newly allocated user_preference_t that must be free'd
*/
user_preference* preference_initialize(const gchar* username) {
	if(!username) return NULL;
	
	user_preference* pref = g_new0(struct user_preference_t,1);
	pref->username = g_strdup(username);
	pref->parser = json_parser_new();
	pref->tests = g_sequence_new((GDestroyNotify)free_testcase);
	
	return pref;
}

/**
* Add a testcase to preferences "tests" GSequence.
*
* @param preference where the test is added
* @param test to be added
*
* @return TRUE when input are not NULL.
*/
gboolean preference_add_test(user_preference* preference, testcase* test) {
	if(!preference || !test) return FALSE;
	g_sequence_append(preference->tests,test);
	return TRUE;
}

/**
* Get a test from user_preference with specified name.
*
* @param preference from which test is searched
* @param testname to search
*
* @return Pointer to testcase_t if one was found (don't free this), NULL otherwise
*/
testcase* preference_get_test(user_preference* preference, const gchar* testname) {
	if(!preference || !testname) return NULL;
	GSequenceIter* iter = NULL;
	for(iter = g_sequence_get_begin_iter(preference->tests);
		!g_sequence_iter_is_end(iter); 
		iter = g_sequence_iter_next(iter)) {
		testcase* test = (testcase*)g_sequence_get(iter);
		if(g_strcmp0(test->name,testname) == 0) return test;
	}
	return NULL;
}

/**
* Initialize a new testcase with g_new0() that can be free'd by calling free_testcase().
* Sets up url, name, encoding with g_strdup(), initializes filelist GHashTable
* with free_testfile() value GDestroyNotify (keys with free_key()).
*
* @param url Test's REST API URL
* @param testname Name of this test
* @param enc Encoding of the server
*/
testcase* testcase_initialize(const gchar* url, const gchar* testname, const gchar* enc) {
	if(!url || !testname) return NULL;
	testcase* test = g_new0(struct testcase_t,1);
	test->URL = g_strdup(url);
	test->name = g_strdup(testname);
	test->encoding = g_strdup(enc);
	test->files = g_hash_table_new_full(
		(GHashFunc)g_str_hash,
		(GEqualFunc)g_str_equal,
		(GDestroyNotify)free_key,
		(GDestroyNotify)free_testfile);
	test->intfields = NULL;	
	return test;
}

/**
* Add a testfile to a testcase. If the GHashTable was not
* initialized it will be initialized here (should not ever happen).
* Inserts (g_hash_table_insert()) the testfile to tests.
*
* @param test Testcase where to add
* @param file to add to test
*
* @return returns return value of g_hash_table_insert()
*/
gboolean testcase_add_file(testcase* test, testfile* file) {
	if(!test || !file) return FALSE;
	if(!test->files) test->files = g_hash_table_new_full(
		(GHashFunc)g_str_hash,
		(GEqualFunc)g_str_equal,
		(GDestroyNotify)free_key,
		(GDestroyNotify)free_testfile);
		
	return g_hash_table_insert(test->files,g_strdup(file->id),file);
}

/**
* Reset given test - clear data from each testfile
* @param test to reset
*/
void testcase_reset_file(gpointer key, gpointer data, gpointer user) {
 	if(!data) return;
 	testfile *tfile = (testfile*)data;
 	
 	free_jsonreply(tfile->send);
	free_jsonreply(tfile->recv);
	tfile->send = NULL;
	tfile->recv = NULL;
	
	g_slist_free_full(tfile->required,(GDestroyNotify)free_key);
	g_slist_free_full(tfile->moreinfo,(GDestroyNotify)free_key);
	
	g_slist_free_full(tfile->reqinfo,(GDestroyNotify)free_jsonreply);
	g_slist_free_full(tfile->infosend,(GDestroyNotify)free_jsonreply);
	g_slist_free_full(tfile->inforecv,(GDestroyNotify)free_jsonreply);
	
	tfile->required = NULL;
	tfile->reqinfo = NULL;
	tfile->moreinfo = NULL;
	tfile->infosend = NULL;
	tfile->inforecv = NULL;
}

/**
* Initialize a testfile with g_new0(). 
* Sets up a testfile_t that must be free'd with free_testfile().
* Duplicates all given strings and sets all GSequences to NULL.
*
* @param tid Test file id
* @param jsonfile Name of the JSON file of this test
* @param tpath Path to be added to test URL
* @param tmethod Method to use when sending the JSON file
* @param tdelete Toggle whether this test needs to be deleted at server
*
* @return Pointer to newly allocated testfile
*/
testfile* testfile_initialize(const gchar* tid, const gchar* jsonfile, const gchar* tpath, const gchar* tmethod, gboolean tdelete) {
	if(!tid || !jsonfile || !tpath || !tmethod) return NULL;
	
	testfile* tfile = g_new0(struct testfile_t,1);
	tfile->id = g_strdup(tid);
	tfile->path = g_strdup(tpath);
	tfile->file = g_strdup(jsonfile);
	tfile->method = g_strdup(tmethod);
	tfile->need_delete = tdelete;
	
	tfile->send = NULL;
	tfile->recv = NULL;
	
	tfile->required = NULL;
	tfile->reqinfo = NULL;
	tfile->moreinfo = NULL;
	tfile->infosend = NULL;
	tfile->inforecv = NULL;

	return tfile;
}

/**
* Initialize a jsonreply_t with g_new0() and return a pointer to it.
*
* @return newly allocated jsonreply_t as pointer
*/
jsonreply* jsonreply_initialize() {
	return g_new0(struct jsonreply_t,1);
}

/**
* Free a single preference, called only by the destructor of
* GHashTable when clearing user list.
* 
* @param data pointer which contains user_preference_t
*
*/
void free_all_preferences(gpointer data) {
	if(!data) return;
	user_preference* user_pref = (user_preference*)data;
	g_print ("Removing \"%s\" ...", user_pref->username);
	g_free(user_pref->username);
		
	g_object_unref(user_pref->parser);
		
	g_print("clearing %d tests... ",g_sequence_get_length(user_pref->tests));
	g_sequence_free(user_pref->tests);
		
	g_print("done\n");
	g_free(user_pref);
}

/**
* Free a preference file from userlist with given username.
*
* @param userlist List of users.
* @param username Name of user to clear
*
* @return TRUE when user was found from the list
*/
gboolean free_preferences(GHashTable* userlist, const gchar* username) {

	if(!userlist || !username) return FALSE;
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

/**
* Free a single testcase, called by GHashTable destroy notification.
* Clears strings with g_free(), calls to destroy file GHashTable with
* g_hash_table_destroy() and frees list of intfields with
* g_slist_free_full() using free_key().
*
* @param data pointer to testcase to free
*/
void free_testcase(gpointer data) {
	if(!data) return;
	testcase* test = (testcase*)data;
	g_print("%s with %d files",test->name,g_hash_table_size(test->files));
	g_free(test->URL);
	g_free(test->name);
	g_free(test->encoding);
	if(test->files) {
		g_print("...");
		g_hash_table_destroy(test->files);
		test->files = NULL;
	}
	g_slist_free_full(test->intfields,(GDestroyNotify)free_key);
	g_free(test);
}

/**
* Free a single testfile, called by GHashTable destroy notification.
* Clears strings with g_free() and frees GSLists with
* g_slist_free_full() using free_key() for list of strings and
* free_jsonreply() for the lists of JSONS as well as send and recv
* structures.
*
* @param data pointer to testfile to free
*/
void free_testfile(gpointer data) {
	if(!data) return;
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

/** 
* Free a single jsonreply_t.
*
* @param Pointer to jsonreply to free.
*/
void free_jsonreply(gpointer data) {
	jsonreply* item = (jsonreply*)data;
	if(item) {
		g_free(item->data);
		g_free(item);
	}
}

/**
* Free a single key or any gchar*.
*
* @param data Pointer to gchar to free.
*/
void free_key(gpointer data) {
	gchar* str = (gchar*)data;
	g_free(str);
	
}


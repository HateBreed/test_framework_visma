#include "preferences.h"
#include "utils.h"

static GHashTable* userlist = NULL;

/**
* Adds user to userlist with g_hash_table_insert().
*
* @param preference user_preference to add
*
* @return Return value of g_hash_table_insert()
*/
gboolean add_user(user_preference* preference) {
	if(!userlist) userlist = g_hash_table_new_full(
		(GHashFunc)g_str_hash,
		(GEqualFunc)g_str_equal,
		(GDestroyNotify)free_key, 
		(GDestroyNotify)free_all_preferences);
		
	return g_hash_table_insert(userlist, g_strdup(preference->username),preference);
}

/**
* Load user preferences for given username. Creates path with
* preference_make_path() to user folder. When preferences was found
* also reads the preferences from preferences.json by calling
* read_preferences().
*
* @param username username to use.
*
* @return pointer to newly allocated user_preference_t or NULL if user not found
*/
user_preference* load_preferences(const gchar* username) {
	if(!username) return NULL;
	
	user_preference* preferences = preference_initialize(username);
	
	add_user(preferences);
	
	gchar* prefpath = preference_make_path(preferences);
	
	if(load_json_from_file(preferences->parser,prefpath)) {
		if(read_preferences(preferences)) {
			g_print("Preferences loaded and read for user \"%s\"\n", username);
			g_free(prefpath);
			return preferences;
		}
		else g_print("Cannot read preference file for user \"%s\"\n", username);
	}
	else g_print ("Cannot parse preferences for user \"%s\"\n", username);
	
	g_free(prefpath);
	
	free_all_preferences((gpointer)preferences);
	return NULL;
}

/**
* Reads preference file of the given user. First reads the
* "tests" array from the parser loaded in user_preferences.
* Then gets the values of URL, testname, encoding fields
* and proceeds to reading integerfields array/element and
* last reads the preferences of each testfile from an array
* called files.
* 
* Integerfields member names are only used so these have no
* naming conventions. Values are not used, can be "". If
* there is no such field or it has no members all member fields
* are treated as strings.
*
* Files have to have following 5 member field names: id, file,
* path, method, delete.
*
* @param preferences in which the parser is that has the file loaded
*
* @return TRUE when preferences were read;
*/
gboolean read_preferences(user_preference* preferences) {

	if(!preferences) return FALSE;
	
	gboolean rval = TRUE;
	gint fileidx = 0;
	
	JsonReader *reader = json_reader_new (json_parser_get_root (preferences->parser));

	// Get tests
	json_reader_read_member(reader,"tests");

	// Go throug each test
	if(json_reader_is_array(reader)) {
		for(gint testidx = 0; testidx < json_reader_count_elements(reader); testidx++)
		{
			// Start array element
			json_reader_read_element(reader,testidx);
			
			// Read three values
			gchar *url = get_json_member_string(reader,"URL");
			gchar *name = get_json_member_string(reader,"testname");
			gchar *encoding = get_json_member_string(reader,"encoding");
			
			if(!url || !name || !encoding) rval = FALSE;
			
			// Init testcase
			testcase* test = testcase_initialize(url,name,encoding);
			
			if(!test) rval = FALSE;
			
			g_free(url);
			g_free(name);
			g_free(encoding);
				
			// Add test 
			preference_add_test(preferences,test);
			
			// Read integerfields
			if(!json_reader_read_member(reader,"integerfields")) rval = FALSE;
			
			// Go through each array element
			if(json_reader_is_array(reader)) {
				for(fileidx = 0; fileidx < json_reader_count_elements(reader); fileidx++) {
				
					// Read single element
					json_reader_read_element(reader,fileidx);
					
					gchar** members = json_reader_list_members(reader);
					
					// Go through all member fields and add each to list
					for(gint membidx = 0; members[membidx] != NULL; membidx++) {
						test->intfields = g_slist_append(
							test->intfields,
							g_strdup(members[membidx]));
					}
					g_strfreev(members);
					
					// End this element
					json_reader_end_element(reader);
				}
			}
			// No intfields list - not madnatory, all fields are treated as strings
			else g_print("Cannot read integer field list, \"integerfields\" is not an array\n");
			
			// End integerfields
			json_reader_end_member(reader);
			
			// Try to read files
			if(!json_reader_read_member(reader,"files")) rval = FALSE;
			else {
				if(json_reader_is_array(reader)) {

					// Go through each array element
					for(fileidx = 0; fileidx < json_reader_count_elements(reader); fileidx++) {
						json_reader_read_element(reader,fileidx);
						gboolean need_delete = FALSE;
					
						// Read 5 member field values
						gchar *id = get_json_member_string(reader,"id");
						gchar *file = get_json_member_string(reader,"file");
						gchar *path = get_json_member_string(reader,"path");
						gchar *method = get_json_member_string(reader,"method");
						gchar *delete = get_json_member_string(reader,"delete");
					
						// This testfile is tobe deleted from server
						if(g_strcmp0(delete,"yes") == 0) need_delete = TRUE;
					
						// Check if id is either "login" or an integer - others are discarded
						if(g_strcmp0(id,"login") == 0 || string_is_integer(id)) {
							
							// Initialize new testfile_t
							testfile* tfile = testfile_initialize(id,file,path,method,need_delete);
							
							// add it
							if(!testcase_add_file(test,tfile)) 
								g_print("replaced old data in test %s\n", test->name);
						}
						// End this element
						json_reader_end_element(reader);
					
						g_free(id);
						g_free(file);
						g_free(path);
						g_free(method);
						g_free(delete);
					}
					g_print("\n");
				}
				else g_print("Cannot read file list, \"files\" is not an array\n");
			}
			json_reader_end_member(reader); // files

			json_reader_end_element(reader); // test at testidx
		}
	}
	else rval = FALSE;
	
	json_reader_end_member(reader);

	g_object_unref(reader);
	
	return rval;
}

/**
* Clear the userlist containing preferences.
* Calls g_hash_table_destroy() and g_hash_table_unref()
*/
void destroy_preferences() {
	if(userlist) {
		g_hash_table_destroy(userlist);
		g_hash_table_unref(userlist);
	}
	else g_print("list empty\n");
	userlist = NULL;
}

/**
* Form a newly allocated path using TESTPATH and PREFERENCEFILE
* definitions in definitions.h and username defined in preferences
* to establish path: TESTPATH/username/PREFERENCEFILE
*
* @param preference containing user details
*
* @return New charstring to be free'd with g_free()
*/
gchar* preference_make_path(user_preference* preference) {
	gchar* path = g_strjoin("/",TESTPATH,preference->username,PREFERENCEFILE,NULL);
	g_print("Path is \"%s\"\n",path);
	return path;
}
#include "preferences.h"
#include "utils.h"

static GHashTable* userlist = NULL;

/**
* Adds user to userlist 
* @param preference 
*/
gboolean add_user(user_preference* preference) {
	if(!userlist) userlist = g_hash_table_new_full(
		(GHashFunc)g_str_hash,
		(GEqualFunc)g_str_equal,
		(GDestroyNotify)free_key, 
		(GDestroyNotify)free_all_preferences);
		
	return g_hash_table_insert(userlist, g_strdup(preference->username),preference);
}

user_preference* load_preferences(const gchar* username) {
	if(!username) return NULL;
	
	user_preference* preferences = preference_initialize(username);
	
	add_user(preferences);
	
	gchar* prefpath = preference_make_path(preferences);
	
	if(!load_json_from_file(preferences->parser,prefpath)) {
		g_print ("Cannot parse preferences for user \"%s\"\n", username);
		return NULL;
	}
	g_free(prefpath);
	
	if(read_preferences(preferences)) return preferences;
	else return NULL;
}

gboolean read_preferences(user_preference* preferences) {

	if(!preferences) return FALSE;
	
	gboolean rval = TRUE;
	gint fileidx = 0;
	
	JsonReader *reader = json_reader_new (json_parser_get_root (preferences->parser));

	json_reader_read_member(reader,"tests");

	if(json_reader_is_array(reader)) {
		for(gint testidx = 0; testidx < json_reader_count_elements(reader); testidx++)
		{
			json_reader_read_element(reader,testidx);
			
			gchar *url = get_json_member_string(reader,"URL");
			gchar *name = get_json_member_string(reader,"testname");
			gchar *encoding = get_json_member_string(reader,"encoding");
			
			testcase* test = testcase_initialize(url,name,encoding);
			
			g_free(url);
			g_free(name);
			g_free(encoding);
				
			preference_add_test(preferences,test);
				
			//g_print("Test \"%s\" URL = %s\n",test->name,test->URL);
			
			json_reader_read_member(reader,"integerfields");
			
			if(json_reader_is_array(reader)) {
				for(fileidx = 0; fileidx < json_reader_count_elements(reader); fileidx++) {
					json_reader_read_element(reader,fileidx);
					
					gchar** members = json_reader_list_members(reader);
					
					for(gint membidx = 0; members[membidx] != NULL; membidx++) {
						test->intfields = g_slist_append(
							test->intfields,
							g_strdup(members[membidx]));
					}
					g_strfreev(members);
					json_reader_end_element(reader);
				}
			}
			else g_print("Cannot read integer field list, \"integerfields\" is not an array\n");
			
			json_reader_end_member(reader);
			
			json_reader_read_member(reader,"files");
			
			if(json_reader_is_array(reader)) {
				//g_print("%d files:\n",json_reader_count_elements(reader));
				for(fileidx = 0; fileidx < json_reader_count_elements(reader); fileidx++) {
					json_reader_read_element(reader,fileidx);
					gboolean need_delete = FALSE;
					
					gchar *id = get_json_member_string(reader,"id");
					gchar *file = get_json_member_string(reader,"file");
					gchar *path = get_json_member_string(reader,"path");
					gchar *method = get_json_member_string(reader,"method");
					gchar *delete = get_json_member_string(reader,"delete");
					
					if(g_strcmp0(delete,"yes") == 0) need_delete = TRUE;
					
					if(g_strcmp0(id,"login") == 0 || string_is_integer(id)) {
					
						//g_print("\tid :%s, file:%s, path:%s, method:%s ",id,file,path,method);
						testfile* tfile = testfile_initialize(id,file,path,method,need_delete);
						if(!testcase_add_file(test,tfile)) 
							g_print("replaced old data in test %s\n", test->name);
					}
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
			
			json_reader_end_member(reader); // files

			json_reader_end_element(reader); // test at testidx
		}
	}
	else rval = FALSE;
	
	json_reader_end_member(reader);

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

gchar* preference_make_path(user_preference* preference) {
	gchar* path = g_strjoin("/",TASKPATH,preference->username,PREFERENCEFILE,NULL);
	g_print("Path is \"%s\"\n",path);
	return path;
}
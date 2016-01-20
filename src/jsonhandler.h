#ifndef __JSON_HANLDER_H_
#define __JSON_HANDLER_H_

#include "jsonutils.h"
#include "definitions.h"


user_preference* load_preferences(gchar* username,gchar* password);
gboolean read_preferences(user_preference* preferences);

void destroy_preferences();
void free_all_preferences(gpointer data);
gboolean free_preferences(gchar* username);
void free_testcase(gpointer testcase);
void free_testfile(gpointer testfile);
void free_key(gpointer data);

gboolean add_user(user_preference* preference);

user_preference* preference_initialize(gchar* username);
gboolean preference_add_test(user_preference* preference, testcase* test);
testcase* preference_get_test(user_preference* preference, gchar* testname);
gchar* preference_make_path(user_preference* preference);

testcase* testcase_initialize(const gchar* URL, const gchar* name);
gboolean testcase_add_file(testcase* test, testfile* file);

testfile* testfile_initialize(const gchar* id, const gchar* file, const gchar* path, const gchar* method);

jsonreply* jsonreply_initialize();

#endif
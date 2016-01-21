#ifndef __UTILS_H_
#define __UTILS_H_

#include <string.h>
#include "definitions.h"

gboolean string_is_integer(const gchar* string);

user_preference* preference_initialize(gchar* username);
gboolean preference_add_test(user_preference* preference, testcase* test);
testcase* preference_get_test(user_preference* preference, gchar* testname);
gchar* preference_make_path(user_preference* preference);

testcase* testcase_initialize(const gchar* URL, const gchar* name);
gboolean testcase_add_file(testcase* test, testfile* file);

testfile* testfile_initialize(const gchar* id, const gchar* file, const gchar* path, const gchar* method, gboolean delete);

jsonreply* jsonreply_initialize();

void free_all_preferences(gpointer data);
gboolean free_preferences(GHashTable* userlist,gchar* username);
void free_testcase(gpointer testcase);
void free_testfile(gpointer testfile);
void free_jsonreply(gpointer data);
void free_key(gpointer data);
void free_key(gpointer data);

#endif


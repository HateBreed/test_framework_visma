#ifndef __JSON_HANLDER_H_
#define __JSON_HANDLER_H_

#define TASKPATH "tasks"
#define PREFERENCEFILE "preferences.json"

#include <glib.h>
#include <json-glib/json-glib.h>

#include "jsonutils.h"

typedef struct user_preference_t {
	gchar *username;
	JsonParser *parser;
	GSequence *tests;
} user_preference;

typedef struct testcase_t {
	gchar *URL;
	gchar *name;
	GHashTable *files;
} testcase ;

typedef struct testfile_t {
	gchar *id;
	gchar *file;
	gchar *path;
	gchar *method;
} testfile;

gboolean load_preferences(gchar* username,guchar* password);
gboolean read_preferences(user_preference* preferences);

void destroy();
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

#endif
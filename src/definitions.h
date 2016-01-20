#ifndef __DEFINITIONS_H_
#define __DEFINITIONS_H_

#define TASKPATH "tests"
#define PREFERENCEFILE "preferences.json"

#define EXIT_FAILURE -1
#define EXIT_SUCCESS 0

#include <glib.h>
#include <json-glib/json-glib.h>


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

typedef struct jsonreply_t {
  	gchar *data;
  	gsize length;
} jsonreply;

typedef struct testfile_t {
	gchar *id;
	gchar *file;
	gchar *path;
	gchar *method;
	jsonreply *send;
	jsonreply *recv;
} testfile;



#endif
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
	gchar *id; // File id in preferences.json
	gchar *file; // Filename in test folder
	gchar *path; // Path for REST API URL
	gchar *method; // Method to use
	jsonreply *send; // File data as json string
	jsonreply *recv; // Reply sent by the server as json string
	GSList *required; // List of required members from id 0 (case creation)
	GSList *moreinfo; // List of fields that require more information
	GSList *infosend; // List of jsons that are to be used to get more info
	GSList *inforecv; // List of json replies sent by the server
} testfile;



#endif
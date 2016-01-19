#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include <json-glib/json-glib.h>

#include "utils.h"
#include "jsonutils.h"
#include "connectionutils.h"
#include "tests.h"

gchar testpath[] = "tests/";

int main(int argc, char *argv[]) {

	user_preference* prefs = NULL;
	if((prefs = load_preferences("john.doe@severa.com","password"))) g_print("load\n");
	
	GSequenceIter* iter = g_sequence_get_iter_at_pos(prefs->tests,0);
	testcase* test = (testcase*)g_sequence_get(iter);
	g_print("%s %s (%d)\n",test->URL,test->name,g_hash_table_size(test->files));
	tests_initialize();
	tests_run_test(prefs->username,test);
	tests_destroy();
	
	destroy_preferences();
	
	return 0;
}

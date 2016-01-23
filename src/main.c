#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include <json-glib/json-glib.h>

#include "utils.h"
#include "jsonutils.h"
#include "connectionutils.h"
#include "tests.h"
#include "definitions.h"

#define USERNAME_MAX_CHAR 51
#define TEMPLEN 100

/** 
* Test selection part of UI. Lists tests for this user
* and awaits for selection. Runs test if it is found and
* asks whether to rerun test or to quit or to return to main.
*
* @param user username to use for loading preferences
*
* @return TRUE if preferences were found and loaded
*/
gboolean run_test_selection(gchar* user) {

	user_preference* prefs = NULL;
	gchar* temp = (gchar*)g_try_malloc(TEMPLEN);
	gboolean rval = TRUE;

	// Load preferences for this user
	if((prefs = load_preferences(user))) {
		
		g_print("%d test%s loaded for %s:\n",
			g_sequence_get_length(prefs->tests),
			g_sequence_get_length(prefs->tests) > 1 ? "s " : "",
			prefs->username);
	
		gboolean loop = TRUE, rerun = FALSE;
		gchar tnumber = '\0';
	
		while(loop) {
			gint idx = 0;
			GSequenceIter* iter = NULL;
			
			if(!rerun) { 
				g_print("id\tname\tfiles\turl\n");
				g_print("-------------------------------------------------------------------------------\n");
			
				for(iter = g_sequence_get_begin_iter(prefs->tests); 
					!g_sequence_iter_is_end(iter) ; 
					iter = g_sequence_iter_next(iter))
				{
					testcase* t = (testcase*)g_sequence_get(iter);
			
					g_print(" %d\t%s\t%d\t%s\n",idx+1,t->name,g_hash_table_size(t->files),t->URL);
					idx++;
				}
	
				g_print("\nSelect test id to run, q to quit, m to return to user selection: ");
				tnumber = getc(stdin);
		
				// Consume newline markers
				if(tnumber != '\n') temp = fgets(temp,TEMPLEN,stdin);
			}
			else {
				g_print("Starting rerun of test %d\n",g_ascii_digit_value(tnumber));
				rerun = FALSE;
			}

			if(g_ascii_isdigit(tnumber) && 
				g_ascii_digit_value(tnumber) <= g_sequence_get_length(prefs->tests) &&
				g_ascii_digit_value(tnumber) > 0) {
			
				set_parser(prefs->parser);
			
				GSequenceIter* testpos = g_sequence_get_iter_at_pos(prefs->tests,
					g_ascii_digit_value(tnumber)-1);
				
				testcase* test = (testcase*)g_sequence_get(testpos);
			
				g_print("Running test %c:\"%s\" to %s (with %d files)\n",
					tnumber,test->name,test->URL,g_hash_table_size(test->files));
				
				tests_initialize(test);
			
				if(tests_run_test(prefs->username,test)) 
					g_print("Test %s completed with failures.\n",test->name);
				else g_print("Test %s complete\n",test->name);
			
				tests_destroy(test);
				
				g_print("Redo test (r) or quit (q) or go to main (m): ");
				
				gchar response = getc(stdin);
			
				switch (response) {
					case 'r':
						rerun = TRUE;
						break;
					case 'q':
						loop = FALSE;
						break;
					case 'm':
						loop = FALSE;
						rval = FALSE;
						break;
					default:
						g_print("Invalid selection. Return to main.\n");
						rerun = FALSE;
						break;
				} // switch
				// Consume newline markers
				if(response != '\n') temp = fgets(temp,TEMPLEN,stdin);
			} // if
			else if(tnumber == 'q') loop = FALSE;
			else if(tnumber == 'm') {
				loop = FALSE;
				rval = FALSE;
			}
			else g_print("Invalid test id (%d)\n",g_ascii_digit_value(tnumber));
			iter = NULL;
		} // while
		destroy_preferences();
	} // if
	else {
		g_print("Preferences for user \"%s\" were not found\n",user);
		rval = FALSE;
	}
	
	g_free(temp);
	
	return rval;
}

void run_user_loop() {
	gboolean loop_user = TRUE;
	gchar* user = g_try_malloc0(USERNAME_MAX_CHAR);
	
	while(loop_user) {
		g_print("Give username whose tests to run (max. %d chars) or quit (q): ",USERNAME_MAX_CHAR-1);
	
		user = fgets(user,USERNAME_MAX_CHAR,stdin);
		
		// Replace newline
		if(user[strlen(user)-1] == '\n') user[strlen(user)-1]= '\0';
		
		// Contains only 1 character or less
		if(strlen(user) < 1) {
			g_print("Empty username given.\n");
			memset(user,'\0',USERNAME_MAX_CHAR);
		}
		// Quitting
		else if (g_strcmp0("quit",user) == 0 || g_strcmp0("q",user) == 0) {
			g_free(user);
			return;
		}
		else {
			if(run_test_selection(user)) loop_user = FALSE;
		}
	}
	g_free(user);
}

gboolean run_user_test(gchar* user, gchar* testname) {
	user_preference* prefs = NULL;
	gboolean rval = FALSE;

	// Load preferences for this user
	if((prefs = load_preferences(user))) {
		testcase* test = preference_get_test(prefs,testname);
		if(test) {
			g_print("Running test \"%s\" to %s (with %d files)\n",
					test->name,test->URL,g_hash_table_size(test->files));
				
			tests_initialize(test);
		
			if(tests_run_test(prefs->username,test)) 
				g_print("Test %s completed with failures.\n",test->name);
			else g_print("Test %s complete\n",test->name);
		
			tests_destroy(test);
			rval = TRUE;
		}
		else {
			rval = FALSE;
			g_print("Test \"%s\" not found\n",testname);
		}
		destroy_preferences();
	}
	else rval = FALSE;
	
	return rval;
}

int main(int argc, char *argv[]) {
	
	extern gchar *optarg;
	extern gint optopt;
	gint optc = -1;
	gchar* user = NULL;
	gchar* test = NULL;
	
	// Check command line options
	while ((optc = getopt(argc,argv,"u:t:")) != -1) {
		switch (optc) {
			case 'u':
				user = optarg;
				break;
			case 't':
				test = optarg;
				break;
			default:
				break;
		}
	}
	
	if(user && test) {
		if(!run_user_test(user,test)) {
			g_print("No such user or test found.\n");
			return 1;
		}
		return 0;
	}
	
	if(!user) run_user_loop();
	else if(!run_test_selection(user)) run_user_loop();
	
	return 0;
}

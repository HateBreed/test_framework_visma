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
	
		// Run main loop for test selection
		while(loop) {
			gint idx = 0;
			GSequenceIter* iter = NULL;
			
			// If rerun of test was not selected print list of tests
			if(!rerun) { 
				g_print("id\tname\tfiles\turl\n");
				g_print("-------------------------------------------------------------------------------\n");
			
				// Go through list of tests
				for(iter = g_sequence_get_begin_iter(prefs->tests); 
					!g_sequence_iter_is_end(iter) ; 
					iter = g_sequence_iter_next(iter))
				{
					testcase* t = (testcase*)g_sequence_get(iter);
					
					// Is a test, print details
					if(t) {
						g_print(" %d\t%s\t%d\t%s\n",idx+1,t->name,g_hash_table_size(t->files),t->URL);
						idx++;
					}
				}
	
				g_print("\nSelect test id to run, q to quit, u to return to user selection: ");
				
				// get a char
				tnumber = getc(stdin);
		
				// Consume newline markers
				if(tnumber != '\n') temp = fgets(temp,TEMPLEN,stdin);
			}
			
			// Test rerun was selected
			else {
				g_print("Starting rerun of test %d\n",g_ascii_digit_value(tnumber));
				rerun = FALSE;
			}

			// If input is a digit between [1...amount of tests]
			if(g_ascii_isdigit(tnumber) && 
				g_ascii_digit_value(tnumber) <= g_sequence_get_length(prefs->tests) &&
				g_ascii_digit_value(tnumber) > 0) {
			
				// Set default parser
				set_parser(prefs->parser);
			
				// Get iterator to at position in testlist
				GSequenceIter* testpos = g_sequence_get_iter_at_pos(prefs->tests,
					g_ascii_digit_value(tnumber)-1);
				
				// Get test
				testcase* test = (testcase*)g_sequence_get(testpos);
			
				g_print("Running test %c:\"%s\" to %s (with %d files)\n",
					tnumber,test->name,test->URL,g_hash_table_size(test->files));
				
				// Initialize
				tests_initialize(test);
			
				// Run
				if(tests_run_test(prefs->username,test)) 
					g_print("Test %s completed with failures.\n",test->name);
				else g_print("Test %s complete\n",test->name);
			
				// Clear and reset test
				tests_reset(test);
				
				g_print("Redo test (r) or quit (q) or go to main (m) or return to user selection (u): ");
				
				// Get char 
				gchar response = getc(stdin);
			
				switch (response) {
					case 'r':
						rerun = TRUE;
						break;
					case 'q':
						loop = FALSE;
						break;
					case 'm':
						g_print("\n\n");
						break;
					case 'u':
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
			else if(tnumber == 'u') {
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

/**
* Run user selection loop. Asks for usename and if its longer than a character (newline marker)
* that user is attempted to be loaded. If failed, username is asked again.
*/
void run_user_loop() {
	gboolean loop_user = TRUE;
	
	// Allocate buffer
	gchar* user = g_try_malloc0(USERNAME_MAX_CHAR);
	
	// Selection loop
	while(loop_user) {
		g_print("Give username whose tests to run (max. %d chars) or quit (q): ",USERNAME_MAX_CHAR-1);
	
		// Get until newline
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

/**
* Run a test directly with specified user. Attempts to load preferences
* with specified username and if successful, tries to get a test with
* specified testname and when found, runs the test and quits.
*
* This is run only when program is called with two input parameters
* using switch -u for username and -t for testname.
*
* @param user User whose preferences is to be loaded
* @param testname Name of the user's test to load
*
* @return TRUE if user and testname was found, FALSE if either is missing or not found
*/
gboolean run_user_test(gchar* user, gchar* testname) {
	user_preference* prefs = NULL;
	gboolean rval = FALSE;
	
	if(!user || !testname) return rval;

	// Load preferences for this user
	if((prefs = load_preferences(user))) {
	
		// Get test and when found run it
		testcase* test = preference_get_test(prefs,testname);
		if(test) {
			g_print("Running test \"%s\" to %s (with %d files)\n",
					test->name,test->URL,g_hash_table_size(test->files));
				
			// Init
			tests_initialize(test);
		
			// Run
			if(tests_run_test(prefs->username,test)) 
				g_print("Test %s completed with failures.\n",test->name);
			else g_print("Test %s complete\n",test->name);
		
			// REset
			tests_reset(test);
			rval = TRUE;
		}
		// Test not found
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
	
	// Run from CLI
	if(user && test) {
		if(!run_user_test(user,test)) {
			g_print("No such user or test found.\n");
			return 1;
		}
		return 0;
	}
	
	// Run without parameters -> user selection
	if(!user) run_user_loop();
	// Run with user -> test selection
	else if(!run_test_selection(user)) run_user_loop();
	
	return 0;
}

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

gchar testpath[] = "tests/";
#define USERNAME_MAX_CHAR 51
#define TEMPLEN 100

gboolean run_test_selection(gchar* user) {

	user_preference* prefs = NULL;
	gchar* temp = (gchar*)g_try_malloc(TEMPLEN);

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
				g_print("id\tname\tfiles\n");
				g_print("-----------------------------\n");
			
				for(iter = g_sequence_get_begin_iter(prefs->tests); 
					!g_sequence_iter_is_end(iter) ; 
					iter = g_sequence_iter_next(iter))
				{
					testcase* t = (testcase*)g_sequence_get(iter);
			
					g_print(" %d\t%s\t%d\n",idx+1,t->name,g_hash_table_size(t->files));
					idx++;
				}
	
				g_print("Select test to run (or q to quit): ");
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
				
				g_print("Redo test (r) or quit (q): ");
				
				gchar response = getc(stdin);
			
				switch (response) {
					case 'r':
						rerun = TRUE;
						break;
					case 'q':
						loop = FALSE;
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
			else g_print("Invalid test id (%d)\n",g_ascii_digit_value(tnumber));
			iter = NULL;
		} // while
		destroy_preferences();
	} // if
	else g_print("Preferences for user \"%s\" were not found\n",user);
	
	g_free(temp);
	
	return TRUE;
}

void run_user_loop() {
	gboolean loop_user = TRUE;
	gchar* user = g_try_malloc0(USERNAME_MAX_CHAR);
	
	while(loop_user) {
		g_print("Give username whose tests to run (max. %d chars): ",USERNAME_MAX_CHAR-1);
	
		user = fgets(user,USERNAME_MAX_CHAR,stdin);
		
		// Replace newline
		if(user[strlen(user)-1] == '\n') user[strlen(user)-1]= '\0';
		
		// Contains only 1 character or less
		if(strlen(user) < 2) {
			g_print("Empty username given.\n");
			memset(user,'\0',USERNAME_MAX_CHAR);
		}
		// Quitting
		else if (g_strcmp0("quit",user) == 0) {
			g_free(user);
			return;
		}
		else {
			if(run_test_selection(user)) loop_user = FALSE;
		}
	}
	g_free(user);
}


int main(int argc, char *argv[]) {
	
	extern gchar *optarg;
	extern gint optopt;
	gint optc = -1;
	gchar* user = NULL;
	
	// Check command line options
	while ((optc = getopt(argc,argv,"u:")) != -1) {
		switch (optc) {
			case 'u':
				user = optarg;
				break;
			default:
				break;
		}
	}
	
	if(!user) run_user_loop();
	else run_test_selection(user);
	
	return 0;
}

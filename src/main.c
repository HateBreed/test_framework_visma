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

int main(int argc, char *argv[]) {

	user_preference* prefs = NULL;
	
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
	
	if(!user) {
		g_print("Give username whose tests to run\n");
		return 0;
	}
	
	if((prefs = load_preferences(user))) {
		
		g_print("%d test%s loaded for %s:\n",
			g_sequence_get_length(prefs->tests),
			g_sequence_get_length(prefs->tests) > 1 ? "s " : "",
			prefs->username);
		
		gboolean loop = TRUE;//, rerun = FALSE;
		
		while(loop) {
			gint idx = 0;
			GSequenceIter* iter = NULL;
			
			for(iter = g_sequence_get_begin_iter(prefs->tests); 
				!g_sequence_iter_is_end(iter) ; 
				iter = g_sequence_iter_next(iter))
			{
				testcase* t = (testcase*)g_sequence_get(iter);
				g_print("id\tname\n");
				g_print("-----------------\n");
				g_print(" %d\t%s\n",idx+1,t->name);
				idx++;
			}
		
			g_print("Select test to run: ");
			gchar tnumber = getc(stdin);
			
			getc(stdin);

			if(g_ascii_isdigit(tnumber) && 
				g_ascii_digit_value(tnumber) <= idx &&
				g_ascii_digit_value(tnumber) > 0) {
				
				set_parser(prefs->parser);
				
				GSequenceIter* testpos = g_sequence_get_iter_at_pos(prefs->tests,
					g_ascii_digit_value(tnumber)-1);
				testcase* test = (testcase*)g_sequence_get(testpos);
				g_print("Running test %c:\"%s\" to %s (with %d files)\n",
					tnumber,test->name,test->URL,g_hash_table_size(test->files));
				tests_initialize();
				tests_run_test(prefs->username,test);
				tests_destroy();
				g_print("Test %s complete\n",test->name);
				
				g_print("Redo test (r) or quit (q): ");
				gchar response = getc(stdin);
				
				switch (response) {
					case 'r':
						//rerun = TRUE;
						break;
					case 'q':
						loop = FALSE;
						break;
					default:
						g_print("Invalid selection. Return to main\n");
						break;
				}
				getc(stdin);
			}
			else if(tnumber == 'q') loop = FALSE;
			else g_print("Invalid test id (%d)\n",g_ascii_digit_value(tnumber));
			iter = NULL;
		}
		
		destroy_preferences();
	}
	return 0;
}

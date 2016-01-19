#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include <json-glib/json-glib.h>

#include "utils.h"
#include "jsonutils.h"
#include "jsonhandler.h"
#include "connectionutils.h"
#include "tests.h"

gchar testpath[] = "tests/";

int main(int argc, char *argv[]) {

	if(load_preferences("john.doe@severa.com","password")) g_print("load\n");
	destroy_preferences();
	
	return 0;
}

#ifndef __TESTS_H_
#define __TESTS_H_

#include "definitions.h"
#include "jsonhandler.h"
#include "jsonutils.h"
#include "utils.h"

void tests_initialize();
void tests_destroy();

gboolean tests_run_test(gchar* username, testcase* test);

void tests_check_fields_from_testfiles(gpointer key, gpointer value, gpointer testpath);

gchar* tests_make_path_for_test(gchar* username, testcase* test);

void tests_build_test_sequence(testcase* test);

void tests_conduct_tests(testcase* test, gchar* testpath);

#endif
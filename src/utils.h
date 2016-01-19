#ifndef __UTILS_H_
#define __UTILS_H_

#define EXIT_FAILURE -1
#define EXIT_SUCCESS 0

#include <string.h>
#include <glib.h>

gboolean string_is_integer(const gchar* string);
void free_key(gpointer data);

#endif


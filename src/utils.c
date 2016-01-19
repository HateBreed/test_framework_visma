#include "utils.h"

gboolean string_is_integer(const gchar* string) {
	gint length = strlen(string);
	for(gint strindex = 0; strindex < length; strindex++) {
		if (!g_ascii_isdigit(string[strindex])) return FALSE;
	}
	return TRUE;
}
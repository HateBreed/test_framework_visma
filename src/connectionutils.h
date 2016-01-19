#ifndef __CONNECTION_UTILS_H_
#define __CONNECTION_UTILS_H_

#include <curl/curl.h>

gint http_post(gchar* url, gchar* data, gsize length, gchar* method);

#endif
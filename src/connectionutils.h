#ifndef __CONNECTION_UTILS_H_
#define __CONNECTION_UTILS_H_

#include <curl/curl.h>

typedef struct jsonreply_t {
  	gchar *data;
  	gsize length;
} jsonreply;

jsonreply* http_post(gchar* url, gchar* data, gsize length, gchar* method);

#endif
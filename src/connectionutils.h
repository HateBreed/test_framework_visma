#ifndef __CONNECTION_UTILS_H_
#define __CONNECTION_UTILS_H_

#include <curl/curl.h>
#include "definitions.h"

void http_init(gchar* encoding);
void http_close();
void set_token(gchar* new_token);

jsonreply* http_post(gchar* url, jsonreply* jsondata, gchar* method);

#endif
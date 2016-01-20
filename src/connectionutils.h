#ifndef __CONNECTION_UTILS_H_
#define __CONNECTION_UTILS_H_

#include <curl/curl.h>
#include "definitions.h"

jsonreply* http_post(gchar* url, jsonreply* jsondata, gchar* method);

#endif
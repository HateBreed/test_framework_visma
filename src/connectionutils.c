#include <glib.h>
#include <string.h>
#include "connectionutils.h"

/** This was inspired by the examples at http://curl.haxx.se/libcurl/c/example.html */


static gsize http_get_json_reply_callback(gchar* contents, gsize size, gsize nmemb, gpointer userp)
{
	gsize realsize = size * nmemb;
	jsonreply *reply = (jsonreply*)userp;
 
 	if(reply->data) reply->data = (gchar*)g_try_realloc(reply->data, reply->length + realsize + 1);
 	else reply->data = (gchar*)g_try_malloc0(reply->length + realsize + 1);
	
	if(reply->data == NULL) {
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
  }
 
	memcpy(&(reply->data[reply->length]), contents, realsize);
	reply->length += realsize;
	reply->data[reply->length] = 0;
 
	return realsize;
}


jsonreply* http_post(gchar* url, gchar* data, gsize length, gchar* method) {
	CURL *curl;
	CURLcode res;
	struct curl_slist *headers = NULL;
	
	jsonreply* reply = g_new0(struct jsonreply_t,1);
	 
	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	
	g_print("URL: %s\nContent (%ld):%s \n",url,length, data);

	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		headers = curl_slist_append(headers, "Accept: application/json");
   		headers = curl_slist_append(headers, "Content-Type: application/json");
   		
   		gchar *clength = g_strjoin(" ","Content-Length:",g_strdup_printf("%ld",length),NULL);
   		headers = curl_slist_append(headers, clength);
   		g_print("%s\n",clength);
   		g_free(clength);
   		
   		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
   		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
		
		// For getting response
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_get_json_reply_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, reply);
 
		res = curl_easy_perform(curl);

		if(res != CURLE_OK)	
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			
		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);
	}
	
	curl_global_cleanup();

	return reply;
}
#include <glib.h>
#include <string.h>
#include "connectionutils.h"

/** This was inspired by the examples at http://curl.haxx.se/libcurl/c/example.html */
CURL *curl = NULL;
gchar* token = NULL;

void http_init() {
	token = NULL;
	curl_global_init(CURL_GLOBAL_ALL);
	if(!curl) curl = curl_easy_init();
}

void http_close() {
	g_free(token);
	if(curl) curl_easy_cleanup(curl);
	curl_global_cleanup();
}

void set_token(gchar* new_token) {
	token = g_strjoin(" ","Authorization: ", new_token, NULL);
}

static gsize http_get_json_reply_callback(gchar* contents, gsize size, gsize nmemb, gpointer userp)
{
	gsize realsize = size * nmemb;
	jsonreply *reply = (jsonreply*)userp;
 
 	if(reply->data) reply->data = (gchar*)g_try_realloc(reply->data, reply->length + realsize + 1);
 	else reply->data = (gchar*)g_try_malloc0(reply->length + realsize + 1);
	
	if(reply->data == NULL) {
		g_error("not enough memory (realloc returned NULL)\n");
		return 0;
	}
 
	memcpy(&(reply->data[reply->length]), contents, realsize);
	reply->length += realsize;
	reply->data[reply->length] = 0;
 
	return realsize;
}


jsonreply* http_post(gchar* url, jsonreply* jsondata, gchar* method) {
	
	CURLcode res;
	struct curl_slist *headers = NULL;
	
	if(!url || !method) return NULL;
	if(!curl) http_init();
	
	jsonreply* reply = g_new0(struct jsonreply_t,1);

#ifdef G_MESSAGES_DEBUG
	if(g_strcmp0(method,"POST") == 0) g_print("Content (%ld):%s \n%s To: %s\n", jsondata->length, jsondata->data,method, url);
	else g_print("Content (0)\n%s to %s\n",method,url);
#endif

	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		headers = curl_slist_append(headers, "Accept: application/json");
   		headers = curl_slist_append(headers, "Content-Type: application/json");
   		if(token) headers = curl_slist_append(headers, token);  
   		
   		if(g_strcmp0(method,"POST") == 0) {
   			gchar *clength = g_strjoin(" ","Content-Length:",g_strdup_printf("%ld",jsondata->length),NULL);
	   		headers = curl_slist_append(headers, clength);
	   		g_free(clength);
	   	}
   		
   		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
   		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		if(jsondata) curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsondata->data);
		
		// For getting response
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_get_json_reply_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, reply);
 
		res = curl_easy_perform(curl);

		if(res != CURLE_OK)	
			g_print("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			
		curl_slist_free_all(headers);
	}
#ifdef G_MESSAGES_DEBUG
	g_print("Reply (%ld):%s \n\n", reply->length, reply->data);
#endif
	
	return reply;
}
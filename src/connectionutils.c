#include <glib.h>
#include <string.h>
#include <iconv.h>
#include <errno.h>
#include "connectionutils.h"

/** This was inspired by the examples at http://curl.haxx.se/libcurl/c/example.html */
CURL *curl = NULL;
gchar* token = NULL;

const gchar* server_encoding = NULL;
const gchar* local_encoding = NULL;

void http_init(gchar* server_enc) {
	token = NULL;
	server_encoding = server_enc;
	if(g_get_charset(&local_encoding)) {
#ifdef G_MESSAGES_DEBUG
		g_print("Local encoding: %s\n",local_encoding);
#endif
	}
	g_print("Local encoding: %s\n",local_encoding);
	g_print("Server encoding: %s\n",server_encoding);
	
	// Encoding disabled, not working correctly
	server_encoding = NULL;
	local_encoding = NULL;
	
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

void setup_character_encoding(gchar* server_enc) {
	
}

gchar* convert_to_rest_api(gchar* data, gint length, gsize* newlength) {
	
	gsize in = 0, utf8len = 0;
	
	// First to UTF8
	gchar* strutf8 = g_locale_to_utf8(data,	length, &in, &utf8len, NULL);
	
	// Then to server
	gchar* converted = g_convert(strutf8, utf8len, server_encoding, "UTF-8", &in, newlength, NULL);
	g_free(strutf8);
	return converted;
	//return strutf8;
}

gchar* convert_from_rest_api(gchar* data, gint length, gsize* newlength) {
	gsize in = 0, utf8len = 0;
	
	gchar* strutf8 = g_convert(data, length, "UTF-8", server_encoding, &in, &utf8len, NULL);
		
	gchar* converted = g_locale_from_utf8(strutf8, length, &in, newlength, NULL);
	
	g_free(strutf8);
	return converted;
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
	
	if(!url || !method  || !curl) return NULL;
	
	jsonreply* reply = g_new0(struct jsonreply_t,1);
	gchar* converted = NULL;
	gchar *contentlength = NULL;
	gchar* datalen = NULL;

#ifdef G_MESSAGES_DEBUG
	if(g_strcmp0(method,"POST") == 0) g_print("Content (%ld):%s \n%s To: %s\n", jsondata->length, jsondata->data,method, url);
	else g_print("Content (0)\n%s to %s\n",method,url);
#endif

	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		headers = curl_slist_append(headers, "Accept: application/json");
		headers = curl_slist_append(headers, "Accept-Charset: utf-8");
   		headers = curl_slist_append(headers, "Content-Type: application/json; charset=utf-8");
   		if(token) headers = curl_slist_append(headers, token);  
   		  		
   		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
   		
   		
		if(jsondata && g_strcmp0(method,"POST") == 0) {
		
			if(server_encoding && local_encoding) {
			
				gsize len = 0;
				converted = convert_to_rest_api(jsondata->data,jsondata->length, &len);
				if(len != jsondata->length) g_print("Conversion changed length\n");
			
				datalen = g_strdup_printf("%ld",len);
			
				curl_easy_setopt(curl, CURLOPT_POSTFIELDS, converted);
			}
			else {
				datalen = g_strdup_printf("%ld",jsondata->length);
				curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsondata->data);
			}
			contentlength = g_strjoin(" ","Content-Length:",datalen,NULL);
	   		headers = curl_slist_append(headers, contentlength);
			g_free(datalen);
		}
		
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		
		// For getting response
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_get_json_reply_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, reply);
 
		res = curl_easy_perform(curl);

		if(res != CURLE_OK)	
			g_print("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			
		curl_slist_free_all(headers);
	}
	g_free(converted);
	g_free(contentlength);
	
#ifdef G_MESSAGES_DEBUG
	g_print("Reply (%ld):%s \n\n", reply->length, reply->data);
#endif

	if(server_encoding && local_encoding) {
		gsize l = 0;	
		gchar* back = convert_from_rest_api(reply->data,reply->length, &l);
		g_free(reply->data);
		reply->data = back;
		reply->length = l;
	}
	return reply;
}
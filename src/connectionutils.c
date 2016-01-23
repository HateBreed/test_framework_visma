#include <glib.h>
#include <string.h>
#include <iconv.h>
#include <errno.h>
#include "connectionutils.h"


CURL *curl = NULL;
gchar* token = NULL;

const gchar* server_encoding = NULL;
const gchar* local_encoding = NULL;

/**
* Initialize http "engine". Sets up CURL with curl_global_init()
* and curl_easy_init(). Also resets token to NULL and sets up
* given server enconding (ENCODING IS NOT YET UTILIZED AS IT 
* BREAKS UP FOR SOME REASON).
*
* @param server_enc Server encoding
*/
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

/**
* Shutdown http "engine" by calling curl_easy_cleanup() and
* curl_global_cleanup(). Also frees token with g_free().
* Should include secure memset for token.
*/
void http_close() {
	g_free(token); // TODO set up secure memset
	
	if(curl) curl_easy_cleanup(curl);
	curl = NULL;
	curl_global_cleanup();
}

/**
* Set authentication token to be used in future connections.
* Token is duplicated with g_strjoin() to include "Authorization: "
* text at the beginning.
*
* @param new_token Token to set up.
*/
void set_token(gchar* new_token) {
	token = g_strjoin(" ","Authorization: ", new_token, NULL);
}

/**
* Convert string to charset of the REST API. 
* First converts locale to UTF-8 and then to server encoding if
* server is set to any other than UTF-8. REturned charstring must
* be free'd with g_free().
*
* @param data Data to convert
* @param length length of the data
* @param newlength pointer in which the length of the returned charsting is set
*
* @return New converted gchar as pointer
*/
gchar* convert_to_rest_api(gchar* data, gint length, gsize* newlength) {
	
	gsize in = 0, utf8len = 0;
	
	// First to UTF8
	gchar* strutf8 = g_locale_to_utf8(data,	length, &in, &utf8len, NULL);
	g_print("AS UTF8 (%ld): %s\n",utf8len,strutf8);
	
	// Then to server
	if(g_strcmp0(server_encoding,"UTF-8") == 0) return strutf8;

	gchar* converted = g_convert(strutf8, utf8len, server_encoding, "UTF-8", &in, newlength, NULL);
	g_free(strutf8);
	return converted;
}

/**
* Convert string from charset of the REST API. 
* First converts from the server encoding unless server is configured
* as UTF-8 and then converst UTF-8 to locale. Returned charstring must
* be free'd with g_free().
*
* @param data Data to convert
* @param length length of the data
* @param newlength pointer in which the length of the returned charsting is set
*
* @return New converted gchar as pointer
*/
gchar* convert_from_rest_api(gchar* data, gint length, gsize* newlength) {
	gsize in = 0, utf8len = 0;
	gchar* converted = NULL;
	
	if(g_strcmp0(server_encoding,"UTF-8") != 0) {
		gchar* strutf8 = g_convert(data, length, "UTF-8", server_encoding, &in, &utf8len, NULL);
		
		converted = g_locale_from_utf8(strutf8, utf8len, &in, newlength, NULL);
		g_print("toserver\n");
		g_free(strutf8);
	}
	else converted = g_locale_from_utf8(data, length, &in, newlength, NULL);
	g_print("AS LOCAL (%ld): %s\n",*newlength,converted);

	return converted;
}

/**
* A callback for storing curl response. Called by curl only.
* This was inspired by the examples at http://curl.haxx.se/libcurl/c/example.html
*
* @param contents
* @param nmemb
* @param userp
* 
* @return size 
*/
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

/**
* Send jsondata as Content-Type "application/json" to given url
* with given method using CURL. Initializes Content-Length field only with
* POST method and likewise, adds the given data only with POST.
* Sets up headers: Accept: application/json, Accept-Charset: utf-8 and
* if authentication token is set, appends also Authentication: header.
* A callback set up to get the response which is returned as pointer to
* jsonreply_t.
*
* @param url Where to send
* @param jsondata Data to send, can be NULL
* @param method Method to use (GET, POST, DELETE)
*
* @return Newly allocated jsonreply_t pointer containing reply
*/
jsonreply* http_post(gchar* url, jsonreply* jsondata, gchar* method) {
	
	CURLcode res;
	struct curl_slist *headers = NULL;
	
	if(!url || !method  || !curl) return NULL;
	
	// New struct for reply
	jsonreply* reply = g_new0(struct jsonreply_t,1);
	gchar* converted = NULL;
	gchar *contentlength = NULL;
	gchar* datalen = NULL;

#ifdef G_MESSAGES_DEBUG
	if(g_strcmp0(method,"POST") == 0) g_print("Content (%ld):%s \n%s To: %s\n", jsondata->length, jsondata->data,method, url);
	else g_print("Content (0)\n%s to %s\n",method,url);
#endif

	if(curl) {
		// Setup headers
		curl_easy_setopt(curl, CURLOPT_URL, url);
		headers = curl_slist_append(headers, "Accept: application/json");
		headers = curl_slist_append(headers, "Accept-Charset: utf-8");
   		headers = curl_slist_append(headers, "Content-Type: application/json; charset=utf-8");
   		
   		// Token set, enable authentication
   		if(token) headers = curl_slist_append(headers, token);  
   		 
   		// Set method 		
   		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
   		
   		// If POST method is given with non-null data
		if(jsondata && g_strcmp0(method,"POST") == 0) {
		
			// Encoding set?
			if(server_encoding && local_encoding) {
			
				// Convert
				gsize len = 0;
				converted = convert_to_rest_api(jsondata->data,jsondata->length, &len);
				if(len != jsondata->length) g_print("Conversion changed length (%ld -> %ld)\n",
					jsondata->length, len);
			
				// Set datalength according to converted string
				datalen = g_strdup_printf("%ld",len);
				
				// Add data
				curl_easy_setopt(curl, CURLOPT_POSTFIELDS, converted);
			}
			else {
				// Set datalength according to length
				datalen = g_strdup_printf("%ld",jsondata->length);
				
				// Set data
				curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsondata->data);
			}
			
			// Add content-length header
			contentlength = g_strjoin(" ","Content-Length:",datalen,NULL);
	   		headers = curl_slist_append(headers, contentlength);
			g_free(datalen);
		}
		
		// Add all headers to curl
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

	// Encoding set?
	if(server_encoding && local_encoding) {
		gsize l = 0;	
		gchar* back = convert_from_rest_api(reply->data,reply->length, &l);
		g_free(reply->data);
		reply->data = back;
		reply->length = l;
	}
	return reply;
}
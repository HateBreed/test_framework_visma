#include <glib.h>
#include "connectionutils.h"

gint http_post(gchar* url, gchar* data, gsize length, gchar* method) {
	CURL *curl;
	CURLcode res;
	struct curl_slist *headers = NULL;
 
	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	
	g_print("URL: %s\nContent (%ld):%s \n",url,length, data);

	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		headers = curl_slist_append(headers, "Accept: application/json");
   		headers = curl_slist_append(headers, "Content-Type: application/json");
   		
   		gchar *clength = g_strjoin(" ","Content-Length:",g_strdup_printf("%d",length),NULL);
   		headers = curl_slist_append(headers, clength);
   		g_print("%s\n",clength);
   		g_free(clength);
   		
   		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
   		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
 
		res = curl_easy_perform(curl);

		if(res != CURLE_OK)	
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
	 	g_print("Result: %d\n",res);
		curl_easy_cleanup(curl);
	}
	
	curl_global_cleanup();
	return 0;
}
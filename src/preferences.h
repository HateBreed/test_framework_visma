#ifndef __JSON_HANLDER_H_
#define __JSON_HANDLER_H_

#include "jsonutils.h"
#include "utils.h"
#include "definitions.h"


user_preference* load_preferences(gchar* username,gchar* password);
gboolean read_preferences(user_preference* preferences);
gchar* preference_make_path(user_preference* preference);
void destroy_preferences();
gboolean add_user(user_preference* preference);


#endif
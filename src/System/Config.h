#pragma once

#define FIRMWARE_VERSION "0.30"
#define SYSTEM_DRIVE "" //"/spiffs"
#define PATH_SITE_ROOT SYSTEM_DRIVE "/W"
#define PATH_INTERNAL_ROOT SYSTEM_DRIVE "/INT"
#define PATH_LOGGING_ROOT "/LOG"
#define PATH_TEMP_ROOT "/TMP"

#define ENABLE_EDITOR 1

#define SOCKET_MAX 5
#define DEBUG
#define DEBUG_SECURITY
#define PATH_AUTH_FILE SYSTEM_DRIVE "/INT/authorized.dat"
#define PATH_SYSTEM_CONFIG SYSTEM_DRIVE "/INT/system_config.json"
#define PATH_PUBLIC_PAGES SYSTEM_DRIVE "/INT/public_pages.txt"



#define HTML_REF_CONST_TITLE "$title"
#define HTML_REF_CONST_HEAD "$doc_head"
#define HTML_REF_CONST_HEADER "$header"
#define HTML_REF_CONST_MENU "$menu"
#define HTML_REF_CONST_CONTENT "$content"
#define HTML_REF_CONST_FOOTER "$footer"

#define HEADER_USERNAME "X-USERNAME"
#define HEADER_GROUP    "X-GROUP"
#define HEADER_AUTH "Authorization"
#define HEADER_COOKIE "Cookie"


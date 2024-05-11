#pragma once

#define FIRMWARE_VERSION "0.10"
#define SITE_ROOT "/W"
#define INTERNAL_ROOT "/INT"
#define TEMPLE_VAR_COUNT 50

#define SOCKET_MAX 1 //we should only have one client connected to a socket since the scope of the socket is user-browser
#define DEBUG
#define PATH_AUTH_FILE "/INT/authorized.dat"
#define PATH_SYSTEM_CONFIG "/INT/system_config.json"
#define PATH_PUBLIC_PAGES "/INT/public_pages.txt"

// Can be used if we add option to store in SPIFFS.
// downside to cert in spiffs is that it gets erased when you upload data.
//#define PATH_PUBLIC_CERT_FILE "/INT/cert.cer"
//#define PATH_PRIVATE_KEY_FILE "/INT/key.der"

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


#pragma once

#define FIRMWARE_VERSION "0.7.0"
#define FIRMWARE_DATE __DATE__
#define PROGRAM_TAG "esp32-mvc"

//paths
#define SYSTEM_DRIVE "spiffs"
#define PATH_SITE_ROOT "/" SYSTEM_DRIVE "/W"
#define PATH_INTERNAL_ROOT "/" SYSTEM_DRIVE "/INT"
#define PATH_LOGGING_ROOT "/LOG"
#define PATH_TEMP_ROOT "/" SYSTEM_DRIVE "/TMP"
#define PATH_AUTH_FILE "/" SYSTEM_DRIVE "/INT/authorized.dat"
#define PATH_SYSTEM_CONFIG "/" SYSTEM_DRIVE "/INT/system_config.json"
#define PATH_DEVICE_CONFIG "/" SYSTEM_DRIVE "/INT/device_config.json"
#define PATH_PUBLIC_PAGES "/" SYSTEM_DRIVE "/INT/public_pages.txt"


#define PATH_FACTORY_AUTH_FILE "/" SYSTEM_DRIVE "/FACTORY/authorized.dat"
#define PATH_FACTORY_SYSTEM_CONFIG "/" SYSTEM_DRIVE "/FACTORY/system_config.json"
#define PATH_FACTORY_DEVICE_CONFIG "/" SYSTEM_DRIVE "/FACTORY/device_config.json"
#define PATH_FACTORY_PUBLIC_PAGES "/" SYSTEM_DRIVE "/FACTORY/public_pages.txt"

#define PUBLIC_TEMP_PATH "/" SYSTEM_DRIVE "/TMP/public.cer"
#define PRIVATE_TEMP_PATH "/" SYSTEM_DRIVE "/TMP/private.key"


/* SD */
typedef enum {sd_spi, sd_mmc} sd_type;
#define USE_SD
#define SD_TYPE sd_spi

#define ENABLE_EDITOR 1
#define SOCKET_MAX 5
#define MIN_LOG_BYTES 1024 * 128 //min bytes free to log to a disk

#define FILESYSTEM_BUFFER_SIZE 512

//debug info
// #define DEBUG 1
// #define DEBUG_DEVICE 0
// #define DEBUG_SCHEDULE 1
// #define DEBUG_LOGGING 0
// #define DEBUG_SECURITY 4
// #define DEBUG_FILESYSTEM 0
// #define DEBUG_SOCKET 0
// #define DEBUG_LCD 1
// #define DEBUG_OTA 0

//content parser configuration
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

/* LCD PINS */
#define USE_LCD

#if CONFIG_IDF_TARGET_ESP32
    #define PIN_SDA 21
    #define PIN_SCL 22
#elif CONFIG_IDF_TARGET_ESP32S3
    #define PIN_SDA 11
    #define PIN_SCL 10

    #define PIN_SDMMC_DAT0 40
    #define PIN_SDMMC_DAT1 41
    #define PIN_SDMMC_DAT2 42
    #define PIN_SDMMC_DAT3 47
    #define PIN_SDMMC_CLK  38
    #define PIN_SDMMC_CMD  39
    
#endif


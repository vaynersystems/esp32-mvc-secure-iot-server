#pragma once

#define FIRMWARE_VERSION "0.6.0"
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

#define PUBLIC_TEMP_PATH "/" SYSTEM_DRIVE "/TMP/public.cer"
#define PRIVATE_TEMP_PATH "/" SYSTEM_DRIVE "/TMP/private.key"


/* SD */
typedef enum {sd_spi, sd_mmc} sd_type;
#define USE_SD
#define SD_TYPE sd_mmc
//#define SD_DISK ((SD_TYPE == sd_spi) ? SD : SD_MMC)
// #if !defined(BOARD_HAS_SDMMC) //if board does not support sdmmc, default back to SPI
//     #if SD_TYPE == sd_mmc
//         #undef SD_TYPE
//         #define SD_TYPE sd_spi
//     #endif
// #endif

#define ENABLE_EDITOR 1
#define SOCKET_MAX 5
#define MIN_LOG_BYTES 1024 * 256 //min bytes free to log to a disk

//debug info
// #define DEBUG
// #define DEBUG_DEVICE 0
// #define DEBUG_SCHEDULE 0
// #define DEBUG_LOGGING 0
// #define DEBUG_SECURITY 0
// #define DEBUG_FILESYSTEM 0
// #define DEBUG_SOCKET 0
// #define DEBUG_LCD 0
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
#if CONFIG_IDF_TARGET_ESP32
#define USE_LCD
#define PIN_SDA 21
#define PIN_SCL 22

#elif CONFIG_IDF_TARGET_ESP32S3

#define PIN_SDA 10
#define PIN_SCL 11
#endif



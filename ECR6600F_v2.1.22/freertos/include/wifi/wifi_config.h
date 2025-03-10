#ifndef __WIFI_CONFIG_H__
#define __WIFI_CONFIG_H__

#if defined CONFIG_WIFI_PRODUCT_FHOST
#define WIFI_RX_DESC_SIZE 228 //don't modify
#else
#define WIFI_RX_DESC_SIZE 128 //lmac_test don't modify
#endif


#ifndef CONFIG_WIFI_DRV_RECV_BUF_SIZE
#define CONFIG_WIFI_DRV_RECV_BUF_SIZE  (12288)
#endif

#ifndef CONFIG_WIFI_USER_RECV_BUF_SIZE
#define CONFIG_WIFI_USER_RECV_BUF_SIZE (64 * 1024)
#endif

#if defined(CONFIG_WIFI_LOG_LEVEL_DEBUG)
#define CONFIG_WIFI_LOG_LEVEL 0
#elif defined(CONFIG_WIFI_LOG_LEVEL_INFO)
#define CONFIG_WIFI_LOG_LEVEL 1
#elif defined(CONFIG_WIFI_LOG_LEVEL_WARN)
#define CONFIG_WIFI_LOG_LEVEL 2
#elif defined(CONFIG_WIFI_LOG_LEVEL_ERROR)
#define CONFIG_WIFI_LOG_LEVEL 3
#elif defined(CONFIG_WIFI_LOG_LEVEL_RAW)
#define CONFIG_WIFI_LOG_LEVEL 4
#elif defined(CONFIG_WIFI_LOG_LEVEL_NONE)
#define CONFIG_WIFI_LOG_LEVEL 5
#else //default debug
#define CONFIG_WIFI_LOG_LEVEL 0
#endif

#endif

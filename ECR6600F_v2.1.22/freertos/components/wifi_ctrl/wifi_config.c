#include <stdint.h>
#include "wifi_config.h"

#define __SHAREDRAM __attribute__ ((section("SHAREDRAM")))

#define RXL_BUFFER_SIZE ((CONFIG_WIFI_DRV_RECV_BUF_SIZE + WIFI_RX_DESC_SIZE) / 2)

uint32_t RWNX_MAX_AMSDU_RX = CONFIG_WIFI_DRV_RECV_BUF_SIZE;
uint32_t rxl_hw_buffer1[RXL_BUFFER_SIZE] __SHAREDRAM;
uint32_t rxl_hw_buffer1_size = RXL_BUFFER_SIZE * sizeof(uint32_t);

extern void wifi_init(void);
extern void wifi_ps_set_log(int level);
#if defined(CONFIG_WIFI_PRODUCT_FHOST)
extern void wifi_set_userbuf_max_size(uint32_t size);
extern void wifi_set_agg_max_cnt(uint8_t cnt);
#endif

void wifi_main(void)
{
    wifi_init();

    wifi_ps_set_log(CONFIG_WIFI_LOG_LEVEL);
#if defined(CONFIG_WIFI_PRODUCT_FHOST)
    wifi_set_userbuf_max_size(CONFIG_WIFI_USER_RECV_BUF_SIZE);
    wifi_set_agg_max_cnt(CONFIG_WIFI_AMPDU_TX_CNT);
#endif
}

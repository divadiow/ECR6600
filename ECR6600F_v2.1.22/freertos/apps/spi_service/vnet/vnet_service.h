#ifndef __VNET_SERVICE_H__
#define __VNET_SERVICE_H__

#include "lwip/pbuf.h"
#include "spi_service.h"

int vnet_service_wifi_rx(struct pbuf *p);
int vnet_service_wifi_tx(struct pbuf *p);
int vnet_service_wifi_rx_free(struct pbuf *p);
int vnet_service_wifi_rx_done(spi_service_mem_t *smem);

#endif
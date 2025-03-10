#ifndef __SPI_SERVICE_MAIN_H__
#define __SPI_SERVICE_MAIN_H__
#include "config.h"
#include "spi_service.h"
#include "lwip/pbuf.h"

int spi_service_send_msg(unsigned char *data, unsigned int len);
spi_service_mem_t *spi_mqueue_get(void);
int spi_service_send_data(unsigned char *data, unsigned int len);
int spi_service_send_pbuf(struct pbuf *p_buf);

#endif

#ifndef __SPI_REPEATER_H__
#define __SPI_REPEATER_H__

#include "lwip/pbuf.h"

enum {
    SPI_REPEATER_GET_INFO,
    SPI_REPEATER_START_STA,
    SPI_REPEATER_PACKETIN
} spi_repeater_msg_e;

typedef struct {
    unsigned int msgType;
    unsigned int msgAddr;
} spi_repeater_msg_t;

int spi_repeater_send_msg(spi_repeater_msg_t *msg);
void spi_repeater_init(void);
int spi_repeater_mem_free(struct pbuf *p);

#endif

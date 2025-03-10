#ifndef __SPI_SLAVE_H__
#define __SPI_SLAVE_H__

#include "spi_service.h"

//#define SPI_SLAVE_DEBUG
#define SPI_SLAVE_TYPE_WRITE  0x0
#define SPI_SLAVE_TYPE_READ   0x1
#define SPI_SLAVE_GPIO 4

int spi_slave_init(bool reinit);
int spi_slave_read_data(unsigned int *data, unsigned int len);
int spi_slave_write_data(unsigned int *data, unsigned int len);
void spi_slave_sendto_host(spi_service_mem_t *dataAddr);
void spi_slave_funcset_register(spi_service_func_t funcset);
void spi_slave_tx_host_start(void);

#endif

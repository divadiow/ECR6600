#ifndef __SPI_MASTER_H__
#define __SPI_MASTER_H__

#include "spi_service.h"

//#define SPI_MASTER_DEBUG

#define SPI_MASTER_MAX_TRANSLEN 512
#define SPI_MASTER_MAX_PACKETLEN 2048
#define SPI_MASTER_RX_TASK_RPI 5
#define SPI_MASTER_TX_TASK_RPI 10
#define SPI_MASTER_STACK_DEP 4096
#define SPI_MASTER_QUEUE_DEP 128

#define SPI_MASTER_TYPE_WRITE  0x1
#define SPI_MASTER_TYPE_READ   0x0
#define SPI_MASTER_GPIO 4

int spi_master_init(void);
void spi_master_read_info(unsigned int len);
int spi_master_read_data(unsigned int *data, unsigned int len);
int spi_master_write_data(unsigned int *data, unsigned int len);
void spi_master_sendto_peer(spi_service_mem_t *smem);
void spi_master_funcset_register(spi_service_func_t funcset);

#endif

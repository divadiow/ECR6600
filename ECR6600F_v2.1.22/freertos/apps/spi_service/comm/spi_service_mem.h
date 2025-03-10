#ifndef __SPI_SERVICE_MEM_H__
#define __SPI_SERVICE_MEM_H__
#include "system_def.h"
#include "spi_service.h"
#include "lwip/pbuf.h"

#define SPI_SERVICE_MEM_DEBUG_SUPPORT
#define CO_ALIGN4_HI(val) (((val)+3) & ~3)
#define CO_ALIGN2_HI(val) (val)

enum SPI_SERVICE_MEM
{
    SPI_SERVICE_MEM_RX,
    SPI_SERVICE_MEM_TX,
    SPI_SERVICE_MEM_MAX
};

#define KE_LIST_PATTERN           (0xA55A)
#define KE_ALLOCATED_PATTERN      (0x8338)
#define KE_FREE_PATTERN           (0xF00F)

#ifndef ASSERT_ERR
#define ASSERT_ERR(cond)                                                                 \
    do {                                                                                 \
        if (!(cond)) {                                                                   \
            os_printf(LM_OS, LL_ERR, "[ERR]%s line:%d \""#cond"\"!!!\n", __func__, __LINE__);           \
        }                                                                                \
    } while(0)

#define ASSERT_INFO(cond, param0)             \
    do {                                              \
        if (!(cond)) {                                \
            os_printf(LM_OS, LL_INFO, "[ERR]%s line:%d param 0x%x \""#cond"\"!!!\n", __func__, __LINE__, param0); \
        }                                             \
    } while(0)
#endif

struct mblock_free
{
    uint16_t corrupt_check;
    uint16_t free_size;
    struct mblock_free* next;
    struct mblock_free* previous;
};

struct mblock_used
{
    uint16_t corrupt_check;
    uint16_t size;
};

void spi_service_mem_init(void);
struct mblock_free *spi_service_mem_range_check(void *memPtr);
#if defined  CONFIG_VNET_SERVICE || (CONFIG_SPI_MASTER && CONFIG_SPI_REPEATER)
void *spi_service_mem_pbuf_alloc(enum SPI_SERVICE_MEM memtype, uint32_t size);
void *spi_service_mem_pbuf_free(struct pbuf *p);
#endif
void *spi_service_mem_pool_alloc(enum SPI_SERVICE_MEM memtype, uint32_t size);
void *spi_service_mem_pool_free(void *memPtr);
void *spi_service_smem_rx_alloc(spi_service_ctrl_t *mcfg, spi_service_mem_t *smem);
void *spi_service_smem_rx_free(spi_service_mem_t *smem);
void *spi_service_smem_tx_alloc(spi_service_ctrl_t *mcfg, spi_service_mem_t *smem);
void *spi_service_smem_tx_free(spi_service_mem_t *smem);

#endif


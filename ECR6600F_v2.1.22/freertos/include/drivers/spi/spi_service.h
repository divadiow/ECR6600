#ifndef __SPI_SERVICE_H__
#define __SPI_SERVICE_H__

#include "spi.h"
#include "chip_pinmux.h"

typedef void *(*spi_service_call)(void *);
typedef struct {
    spi_service_call rxPrepare;
    spi_service_call rxDone;
    spi_service_call txPrepare;
    spi_service_call txDone;
    spi_service_call dataHandle;
} spi_service_func_t;

typedef struct {
    unsigned int memAddr;
    unsigned int memOffset;
    unsigned int memLen;
    unsigned int memSlen;
    unsigned int memType;
    unsigned int memNext;
} spi_service_mem_t;

typedef struct {
    unsigned int evt:14;
    unsigned int len:16;
    unsigned int type:2;
} spi_service_ctrl_t;

typedef enum {
    SPI_SERVICE_EVENT_CTRL,
    SPI_SERVICE_EVENT_RX,
    SPI_SERVICE_EVENT_TX,
} spi_service_event_e;

#define SPI_SERVICE_GPIO_REG(X)     SPI_SERVICE_GPIO_REG2(X)
#define SPI_SERVICE_GPIO_REG2(X)    IO_MUX_GPIO##X##_REG
#define SPI_SERVICE_GPIO_BIT(X)     SPI_SERVICE_GPIO_BIT2(X)
#define SPI_SERVICE_GPIO_BIT2(X)    IO_MUX_GPIO##X##_BITS
#define SPI_SERVICE_GPIO_OFFSET(X)  SPI_SERVICE_GPIO_OFFSET2(X)
#define SPI_SERVICE_GPIO_OFFSET2(X) IO_MUX_GPIO##X##_OFFSET
#define SPI_SERVICE_GPIO_NUM(X)     SPI_SERVICE_GPIO_NUM2(X)
#define SPI_SERVICE_GPIO_NUM2(X)    GPIO_NUM_##X
#define SPI_SERVICE_GPIO_FUN(X)     SPI_SERVICE_GPIO_FUN2(X)
#define SPI_SERVICE_GPIO_FUN2(X)    FUNC_GPIO##X##_GPIO##X

#ifdef CONFIG_SPI_REPEATER
#ifdef CONFIG_SPI_MASTER
#define SPI_SERVICE_TRANSCTL_MODE   (SPI_TRANSCTRL_DUALQUAD_DUAL | SPI_TRANSCTRL_DUMMY_CNT_2)
#else
#define SPI_SERVICE_TRANSCTL_MODE   SPI_TRANSCTRL_DUALQUAD_DUAL
#endif
#define SPI_SERVICE_READ_CMD        0x0C
#define SPI_SERVICE_WRITE_CMD       0x52
#define SPI_SERVICE_STATE_CMD       0x15
#else
#define SPI_SERVICE_TRANSCTL_MODE   SPI_TRANSCTRL_DUALQUAD_REGULAR
#define SPI_SERVICE_READ_CMD        0x0B
#define SPI_SERVICE_WRITE_CMD       0x51
#define SPI_SERVICE_STATE_CMD       0x05
#endif

typedef enum {
    SPI_SERVICE_TYPE_INFO,
    SPI_SERVICE_TYPE_HTOS = 0x100,
    SPI_SERVICE_TYPE_STOH = 0x200,
    SPI_SERVICE_TYPE_OTA = 0x300,
    SPI_SERVICE_TYPE_MSG = 0x500,
    SPI_SERVICE_TYPE_INT,
} spi_service_type_e;

#define SPI_SERVICE_CONTROL_LEN 0x8000
#define SPI_SERVICE_CONTROL_INT 0x9000
#define SPI_SERVICE_CONTROL_MSG 0xA000
#define SPI_SERVICE_MSG_MAX_LEN 0x1000

//#define SPI_SERVICE_LOOP_TEST
#ifdef SPI_SERVICE_LOOP_TEST
#define SPI_SERVICE_PM_CNT
//#define SPI_SERVICE_PM_DATA
#endif

#endif

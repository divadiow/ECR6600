#ifndef __SPI_SLAVE_ZERATUL_H__
#define __SPI_SLAVE_ZERATUL_H__

#include "system_def.h"
#include "chip_memmap.h"

//0x10 trans format
#define SPI_FORMAT_ADDRLEN_ONE_BYTE         (0x0 << 16)
#define SPI_FORMAT_ADDRLEN_TWO_BYTE         (0x1 << 16)
#define SPI_FORMAT_ADDRLEN_THREE_BYTE       (0x2 << 16)
#define SPI_FORMAT_ADDRLEN_FOUR_BYTE        (0x3 << 16)
#define SPI_FORMAT_DATA_LEN(X)              ((X) << 8)
#define SPI_FORMAT_DATA_MERGE(X)            ((X) << 7)
#define SPI_FORMAT_MOSI_UNI_DIR             (0x0 << 4)
#define SPI_FORMAT_MOSI_BI_DIR              (0x1 << 4)
#define SPI_FORMAT_MSB                      (0x0 << 3)
#define SPI_FORMAT_LSB                      (0x1 << 3)
#define SPI_FORMAT_SLAVE_MODE               (0x1 << 2)
#define SPI_FORMAT_MASTER_MODE              (0x0 << 2)
#define SPI_FORMAT_CPOL_LOW                 (0x0 << 1)
#define SPI_FORMAT_CPOL_HIGH                (0x1 << 1)
#define SPI_FORMAT_CPHA_ODD                 (0x0 << 0)
#define SPI_FORMAT_CPA_EVEN                 (0x1 << 0)

/* 0x20 - spi transfer control register*/
#define SPI_TRANSCTRL_RCNT(x)               (((x) & 0x1FF) << 0)
#define SPI_TRANSCTRL_WCNT(x)               (((x) & 0x1FF) << 12)
#define SPI_TRANSCTRL_DUALQUAD(x)           (((x) & 0x3) << 22)
#define SPI_TRANSCTRL_TRAMODE(x)            (((x) & 0xF) << 24)

//0x30
#define SPI_CONTROL_TXTHRES(x)              ((x) << 16)
#define SPI_CONTROL_RXTHRES(x)              ((x) << 8)
#define SPI_CONTROL_TXDMA_EN                (0x1 << 4)
#define SPI_CONTROL_TXDMA_DIS               (0x0 << 4)
#define SPI_CONTROL_RXDMA_DIS               (0x0 << 3)
#define SPI_CONTROL_RXDMA_EN                (0x1 << 3)
#define SPI_CONTROL_SPIRST                  BIT(0)
#define SPI_CONTROL_RXFIFORST               BIT(1)
#define SPI_CONTROL_TXFIFORST               BIT(2)

/* 0x34 - spi status register */
#define SPI_STATUS_BUSY                     BIT(0)
#define SPI_STATUS_RXNUM                    (0x1F << 8)
#define SPI_STATUS_RXEMPTY                  BIT(14)
#define SPI_STATUS_TXNUM                    (0x1F << 16)
#define SPI_STATUS_TXFULL                   BIT(23)

//0x38
#define  SPI_INTERRUPT_SLVCMD_EN            BIT(5)
#define  SPI_INTERRUPT_ENDINT_EN            BIT(4)
#define  SPI_INTERRUPT_TXFIFOINT_EN         BIT(3)
#define  SPI_INTERRUPT_RXFIFOINT_EN         BIT(2)
#define  SPI_INTERRUPT_TXFIFOURINT_EN       BIT(1)
#define  SPI_INTERRUPT_RXFIFOORINT_EN       BIT(0)

/* 0x3C - spi intr status register */
#define SPI_INTR_STS_END                    BIT(4)

//0x40
#define SPI_INTERFACE_TIMING(x)             ((x) << 0x0)

#define SPI_TRANSCTRL_DUALQUAD_REGULAR      SPI_TRANSCTRL_DUALQUAD(0)
#define SPI_TRANSCTRL_DUALQUAD_DUAL         SPI_TRANSCTRL_DUALQUAD(1)
#define SPI_TRANSCTRL_DUALQUAD_QUAD         SPI_TRANSCTRL_DUALQUAD(2)

#define SPI_TRANSCTRL_TRAMODE_WRCON         SPI_TRANSCTRL_TRAMODE(0)    /* w/r at the same time */
#define SPI_TRANSCTRL_TRAMODE_WO            SPI_TRANSCTRL_TRAMODE(1)    /* write only */
#define SPI_TRANSCTRL_TRAMODE_RO            SPI_TRANSCTRL_TRAMODE(2)    /* read only */
#define SPI_TRANSCTRL_TRAMODE_WR            SPI_TRANSCTRL_TRAMODE(3)    /* write, read */
#define SPI_TRANSCTRL_TRAMODE_RW            SPI_TRANSCTRL_TRAMODE(4)    /* read, write */
#define SPI_TRANSCTRL_TRAMODE_WDR           SPI_TRANSCTRL_TRAMODE(5)    /* write, dummy, read */
#define SPI_TRANSCTRL_TRAMODE_RDW           SPI_TRANSCTRL_TRAMODE(6)    /* read, dummy, write */
#define SPI_TRANSCTRL_TRAMODE_NONE          SPI_TRANSCTRL_TRAMODE(7)    /* none data */
#define SPI_TRANSCTRL_TRAMODE_DW            SPI_TRANSCTRL_TRAMODE(8)    /* dummy, write */
#define SPI_TRANSCTRL_TRAMODE_DR            SPI_TRANSCTRL_TRAMODE(9)    /* dummy, read */
#define SPI_TRANSCTRL_CMDEN                 ((0x1) << 30)
#define SPI_TRANSCTRL_CMDDIS                ((0x0) << 30)
#define SPI_TRANSCTRL_ADDREN                ((0x1) << 29)
#define SPI_TRANSCTRL_ADDRDIS               ((0x0) << 29)
#define SPI_TRANSCTRL_ADDRFMT(x)            ((x) << 28)

#define SPI_TRANSCTRL_CMD_EN                (1<<30)
#define SPI_TRANSCTRL_ADDR_EN               (1<<29)
#define SPI_TRANSCTRL_ADDR_FMT              (1<<28)
#define SPI_TRANSCTRL_TOKEN_EN              (1<<21)

#define SPI_TRANSCTRL_DUMMY_CNT_1           (0<<9)
#define SPI_TRANSCTRL_DUMMY_CNT_2           (1<<9)
#define SPI_TRANSCTRL_DUMMY_CNT_3           (2<<9)
#define SPI_TRANSCTRL_DUMMY_CNT_4           (3<<9)

#define SPI_CONFIG_TXFIFO_DEPTH_2_WORD      (0x0 << 4)
#define SPI_CONFIG_TXFIFO_DEPTH_4_WORD      (0x1 << 4)
#define SPI_CONFIG_TXFIFO_DEPTH_8_WORD      (0x2 << 4)
#define SPI_CONFIG_TXFIFO_DEPTH_16_WORD     (0x3 << 4)

#define SPI_CONFIG_RXFIFO_DEPTH_2_WORD      (0x0 << 0)
#define SPI_CONFIG_RXFIFO_DEPTH_4_WORD      (0x1 << 0)
#define SPI_CONFIG_RXFIFO_DEPTH_8_WORD      (0x2 << 0)
#define SPI_CONFIG_RXFIFO_DEPTH_16_WORD     (0x3 << 0)

#define SPI_STS_SLAVE_CMD_INT               0x20
#define SPI_INT_SLAVE_CMD_EN                0x20
#define CLOSE   0x00

#define SPI_PREPARE_BUS(X)  \
    do{unsigned int spi_status = 0; \
        do {    \
            spi_status = (X);   \
        } while(spi_status & SPI_STATUS_BUSY);  \
    }while(0)

#define SPI_CLEAR_FIFO(X)       X |= (SPI_CONTROL_RXFIFORST|SPI_CONTROL_TXFIFORST)
#define SPI_CLEAR_FIFOTX(X)     X |= SPI_CONTROL_TXFIFORST
#define SPI_CLEAR_FIFORX(X)     X |= SPI_CONTROL_RXFIFORST

#define SPI_WAIT_RX_READY(X)    \
    do{unsigned int spi_status_r = 0;   \
        do {    \
            spi_status_r = (X); \
        } while(spi_status_r & SPI_STATUS_RXEMPTY); \
    }while(0)

#define SPI_WAIT_TX_READY(X)    \
    do{unsigned int spi_status_t = 0;   \
        do {    \
            spi_status_t = (X); \
        } while(spi_status_t & SPI_STATUS_TXFULL);  \
    }while(0)

typedef enum
{
    SPI_MODE_STANDARD = 0,
    SPI_MODE_DUAL,
    SPI_MODE_QUAD,
    SPI_MODE_QPI,
    SPI_MODE_BI_DIRECTION_MISO,
    SPI_MODE_UNKNOW
} E_SPI_MODE;

typedef enum
{
    SPI_SCLK_DIV_0  = 0xFF,
    SPI_SCLK_DIV_2  = 0x00,
    SPI_SCLK_DIV_4  = 0x01,
    SPI_SCLK_DIV_6  = 0x02,
    SPI_SCLK_DIV_8  = 0x03,
    SPI_SCLK_DIV_10 = 0x04
} E_SPI_SCLK_DIV;

typedef struct _spi_regs {
    volatile unsigned int   edRer;      /* 0x00         - id and revision reg*/
    volatile unsigned int   rev1[3];    /* 0x04-0x0C    - reserved reg */
    volatile unsigned int   transFmt;   /* 0x10         - spi transfer format reg */
    volatile unsigned int   directIO;   /* 0x14         - spi direct io control reg */
    volatile unsigned int   rev2[2];    /* 0x18-0x1C    - reserved reg */
    volatile unsigned int   transCtrl;  /* 0x20         - spi transfer control reg */
    volatile unsigned int   cmd;        /* 0x24         - spi command reg */
    volatile unsigned int   addr;       /* 0x28         - spi address reg */
    volatile unsigned int   data;       /* 0x2C         - spi data reg */
    volatile unsigned int   ctrl;       /* 0x30         - spi control reg */
    volatile unsigned int   status;     /* 0x34         - spi status reg */
    volatile unsigned int   intrEn;     /* 0x38         - spi interrupt enable reg */
    volatile unsigned int   intrSt;     /* 0x3C         - spi interrupt status reg */
    volatile unsigned int   timing;     /* 0x40         - spi interface timing reg */
    volatile unsigned int   rev3[3];    /* 0x44-0x4C    - reserved reg */
    volatile unsigned int   memCtrl;    /* 0x50         - spi memery access control reg */
    volatile unsigned int   rev4[3];    /* 0x54-0x5C    - reserved reg */
    volatile unsigned int   stvSt;      /* 0x60         - spi slave status reg */
    volatile unsigned int   slvDataCnt; /* 0x64         - spi slave data count reg */
    volatile unsigned int   rev5[5];    /* 0x68-0x78    - spi status reg */
    volatile unsigned int   config;     /* 0x7C         - configuration reg */
}spi_reg;

#define SPI_INIT_SUCCESS        0
#define SPI_TX_RET_ENODMA       -1
#define SPI_RX_RET_ENODMA       -2

typedef enum
{
    SPI_SLAVE_CTRL_DATA_EVENT,
    SPI_SLAVE_RX_DATA_EVENT,
    SPI_SLAVE_TX_DATA_EVENT,
} spi_event_e;

typedef struct
{
    unsigned int evt:14;
    unsigned int len:16;
    unsigned int type:2;
} spi_register_t;

extern void  cli_printf(const char *f, ...);
#ifndef READ_REG
#define READ_REG(offset)        (*(volatile uint32_t*)(offset))
#endif
#ifndef WRITE_REG
#define WRITE_REG(offset,value) (*(volatile uint32_t*)(offset) = (uint32_t)(value));
#endif

void spi_slave_rx_data_start(unsigned int length, unsigned int addr);
void spi_slave_tx_data_start(unsigned int length, unsigned int addr);
int spi_slave_init();

typedef void (*spi_slave_callback_func)(spi_event_e evt, unsigned int *cfg);
void spi_slave_cfg_callback_register(spi_slave_callback_func callback);
void spi_slave_gpio_interrupt_start(unsigned int value);
void spi_slave_gpio_interrupt_stop();
void spi_slave_tx_data_len(unsigned short datalen);
void spi_slave_rx_data_len(unsigned short datalen);
void spi_slave_zero_data_len();
void spi_slave_write_data(unsigned char * bufptr ,unsigned int length);
void spi_slave_linkstatus_cs(int link);

#endif

#include "spi_slave_zeratul.h"
#include "chip_pinmux.h"
#include "chip_clk_ctrl.h"
#include "dma.h"
#include "chip_irqvector.h"
#include "oshal.h"
#include "arch_irq.h"
#include <string.h>
#include "gpio.h"
#include "cli.h"

#define ZERATUL_SPI_STATE
#ifdef ZERATUL_SPI_STATE
unsigned int g_spistat[256] = {0};
unsigned int g_spistnum;
unsigned int g_pkgnum;
#endif

#define SLAVE_SPI_CONFIG 1
#define SLAVE_SPI_DMA    2
#define SLAVE_SPI_STATE  3

#define TEST_MODE_EXTPAD_RST_EN 0x201214
spi_slave_callback_func g_slave_call_func;
unsigned int g_spi_state;

void spi_slave_clear_fifo()
{
    spi_reg *p_spi_reg = (spi_reg *)MEM_BASE_SPI1;
    p_spi_reg->ctrl = p_spi_reg->ctrl | SPI_CONTROL_RXFIFORST | SPI_CONTROL_TXFIFORST;

    do
    {
        if (p_spi_reg->ctrl & (SPI_CONTROL_RXFIFORST | SPI_CONTROL_TXFIFORST))
        {
            break;
        }
    } while (1);
}

void spi_slave_load(void);

unsigned int spi_slave_tx_data_ch = 0;
unsigned int spi_slave_rx_data_ch = 0;
unsigned int g_spi_cfg[8];

void spi_slave_rx_dma_isr(void *data)
{
    spi_reg *p_spi_reg = (spi_reg *)MEM_BASE_SPI1;

    while (((p_spi_reg->status) & SPI_STATUS_BUSY) == SPI_STATUS_BUSY);
    /* rx data from spi host done, enter cpu mode */
    //spi_slave_clear_fifo();
    p_spi_reg->ctrl = 0x70000;
    //p_spi_reg->intrEn = 0x20;
    //p_spi_reg->stvSt = 0x7000;

    /* send data to wifi */
    if (g_slave_call_func)
        g_slave_call_func(SPI_SLAVE_RX_DATA_EVENT, NULL);
#ifdef ZERATUL_SPI_STATE
    g_spistat[g_spistnum++] = 0xda;
    g_spistnum = (g_spistnum >= 256) ? 0 : g_spistnum;
    g_spistat[g_spistnum++] = ++g_pkgnum;
    g_spistnum = (g_spistnum >= 256) ? 0 : g_spistnum;
#endif
    p_spi_reg->stvSt = BIT16 | 0x7000;
}

void spi_slave_tx_dma_isr(void *data)
{
    spi_reg *p_spi_reg = (spi_reg *)MEM_BASE_SPI1;

    while (((p_spi_reg->status)& SPI_STATUS_BUSY) == SPI_STATUS_BUSY);
    /* tx data to spi host done, enter cpu mode */
    //spi_slave_clear_fifo();
    p_spi_reg->ctrl = 0x70000;
    //p_spi_reg->intrEn = 0x20;
    //p_spi_reg->stvSt = 0x7000;

    /* callback dothing for tx host */
    if (g_slave_call_func)
        g_slave_call_func(SPI_SLAVE_TX_DATA_EVENT, NULL);
#ifdef ZERATUL_SPI_STATE
    g_spistat[g_spistnum++] = 0xdb;
    g_spistnum = (g_spistnum >= 256) ? 0 : g_spistnum;
#endif
    p_spi_reg->stvSt = BIT16 | 0x7000;
}

void spi_slave_rx_data_start(unsigned int length, unsigned int addr)
{
    spi_reg *p_spi_reg = (spi_reg *)MEM_BASE_SPI1;
    T_DMA_CFG_INFO dma_cfg_info;
    unsigned int value = 0;

    /* enter dma mode */
    spi_slave_clear_fifo();
    value = p_spi_reg->ctrl | BIT(3) | BIT(4) | BIT(16);
    p_spi_reg->ctrl = value;
    //p_spi_reg->intrEn = 0x0;

    /* start rx dma to get data from host */
    dma_cfg_info.dst = (unsigned int)addr;
    dma_cfg_info.src = (unsigned int)&p_spi_reg->data;
    dma_cfg_info.len = length;
    dma_cfg_info.mode = DMA_CHN_SPI_RX;
    drv_dma_stop(spi_slave_rx_data_ch);
    drv_dma_cfg(spi_slave_rx_data_ch, &dma_cfg_info);
    drv_dma_status_clean(spi_slave_rx_data_ch);
    drv_dma_start(spi_slave_rx_data_ch);

    /* notice host dma ready */
    value = p_spi_reg->stvSt & 0xFFFF0000;
    value |= BIT16 | length;
#ifdef ZERATUL_SPI_STATE
    g_spistat[g_spistnum++] = 0x11;
    g_spistnum = (g_spistnum >= 256) ? 0 : g_spistnum;
    g_spistat[g_spistnum++] = value;
    g_spistnum = (g_spistnum >= 256) ? 0 : g_spistnum;
#endif
    p_spi_reg->stvSt = value;
}

void spi_slave_tx_data_start(unsigned int length, unsigned int addr)
{
    spi_reg *p_spi_reg = (spi_reg *)MEM_BASE_SPI1;
    T_DMA_CFG_INFO dma_cfg_info;
    unsigned int value = 0;

    /* enter dma mode */
    spi_slave_clear_fifo();
    value = p_spi_reg->ctrl | BIT(3) | BIT(4) | BIT(16);
    p_spi_reg->ctrl = value;
    //p_spi_reg->intrEn = 0x0;

    /* start tx dma to send data to host */
    dma_cfg_info.dst = (unsigned int)&p_spi_reg->data;
    dma_cfg_info.src = (unsigned int)addr;
    dma_cfg_info.len = length;
    dma_cfg_info.mode = DMA_CHN_SPI_TX;
    drv_dma_stop(spi_slave_tx_data_ch);
    drv_dma_cfg(spi_slave_tx_data_ch, &dma_cfg_info);
    drv_dma_status_clean(spi_slave_tx_data_ch);
    drv_dma_start(spi_slave_tx_data_ch);

    /* notice host dma ready */
    value = p_spi_reg->stvSt & 0xFFFF0000;
    value |= BIT16 | length;
#ifdef ZERATUL_SPI_STATE
    g_spistat[g_spistnum++] = 0x22;
    g_spistnum = (g_spistnum >= 256) ? 0 : g_spistnum;
    g_spistat[g_spistnum++] = value;
    g_spistnum = (g_spistnum >= 256) ? 0 : g_spistnum;
#endif
    p_spi_reg->stvSt = value;
}

void spi_slave_cfg_callback_register(spi_slave_callback_func callback)
{
    g_slave_call_func = callback;
}

void spi_slave_gpio_interrupt_start(unsigned int value)
{
#ifdef ZERATUL_SPI_STATE
    g_spistat[g_spistnum++] = 0x33;
    g_spistnum = (g_spistnum >= 256) ? 0 : g_spistnum;
    g_spistat[g_spistnum++] = value;
    g_spistnum = (g_spistnum >= 256) ? 0 : g_spistnum;
#endif
    drv_gpio_write(GPIO_NUM_17, 1);
}

void spi_slave_linkstatus_cs(int link)
{
    spi_reg *p_spi_reg = (spi_reg *)MEM_BASE_SPI1;
    unsigned int value;

    value = p_spi_reg->stvSt & 0xFFFF0000;
    value |= BIT16 | BIT15 | link;
#ifdef ZERATUL_SPI_STATE
    g_spistat[g_spistnum++] = 0x44;
    g_spistnum = (g_spistnum >= 256) ? 0 : g_spistnum;
    g_spistat[g_spistnum++] = value;
    g_spistnum = (g_spistnum >= 256) ? 0 : g_spistnum;
#endif
    p_spi_reg->stvSt = value;
}


void spi_slave_gpio_interrupt_stop()
{
#ifdef ZERATUL_SPI_STATE
    g_spistat[g_spistnum++] = 0x55;
    g_spistnum = (g_spistnum >= 256) ? 0 : g_spistnum;
#endif

    drv_gpio_write(GPIO_NUM_17, 0);
}

void spi_slave_zero_data_len()
{
    spi_reg *p_spi_reg = (spi_reg *)MEM_BASE_SPI1;
    unsigned int value;

    value = p_spi_reg->stvSt & 0xFFFF0000;
    value |= BIT16;
#ifdef ZERATUL_SPI_STATE
    g_spistat[g_spistnum++] = 0x66;
    g_spistnum = (g_spistnum >= 256) ? 0 : g_spistnum;
    g_spistat[g_spistnum++] = value;
    g_spistnum = (g_spistnum >= 256) ? 0 : g_spistnum;
    g_spistat[g_spistnum++] = p_spi_reg->status;
    g_spistnum = (g_spistnum >= 256) ? 0 : g_spistnum;
#endif
    p_spi_reg->stvSt = value;
}

int spi_slave_init()
{
    unsigned int value = 0;
    //spi hw register
    spi_reg *p_spi_reg = (spi_reg *)MEM_BASE_SPI1;

    // reset tx rx fifo
    spi_slave_clear_fifo();

    WRITE_REG(TEST_MODE_EXTPAD_RST_EN, 0x0);
    os_usdelay(10);
    WRITE_REG(0x202018, 0x88);
    os_usdelay(10);

    // enable all interrupt
    p_spi_reg->intrSt = 0xFFFFFFFF;

    /* enable spi1 clock */
    chip_clk_enable(CLK_SPI1_APB);
    PIN_FUNC_SET(IO_MUX_GPIO0, FUNC_GPIO0_SPI1_CLK);
    PIN_FUNC_SET(IO_MUX_GPIO1, FUNC_GPIO1_SPI1_CS0);
    PIN_FUNC_SET(IO_MUX_GPIO2, FUNC_GPIO2_SPI1_MOSI);
    PIN_FUNC_SET(IO_MUX_GPIO3, FUNC_GPIO3_SPI1_MIS0);

    PIN_FUNC_SET(IO_MUX_GPIO17, FUNC_GPIO17_GPIO17);
    drv_gpio_ioctrl(GPIO_NUM_17, DRV_GPIO_CTRL_SET_DIRCTION, DRV_GPIO_ARG_DIR_OUT);
    drv_gpio_write(GPIO_NUM_17, 0x0);

    /* plo and pha zero */
    value &= (~0x2);
    value &= (~0x1);
    /* plo and pha one */
    //value |= 0x2;
    //value |= 0x1;
    value |= BIT(2) | BIT(7) | (2 << 16) | (7 << 8);
    p_spi_reg->transFmt = value;

    /* cfg spi clk */
    value = p_spi_reg->timing & 0xFFFFFF00;
    p_spi_reg->timing = value | (1 & 0xFF);
    p_spi_reg->ctrl |= 0x70000;

    p_spi_reg->intrEn = 0x20;
    arch_irq_register(VECTOR_NUM_SPI2, spi_slave_load);
    arch_irq_clean(VECTOR_NUM_SPI2);
    arch_irq_unmask(VECTOR_NUM_SPI2);

    spi_slave_tx_data_ch = drv_dma_ch_alloc();
    if (spi_slave_tx_data_ch  < 0)
    {
        return SPI_TX_RET_ENODMA;
    }
    drv_dma_isr_register(spi_slave_tx_data_ch, spi_slave_tx_dma_isr, NULL);

    spi_slave_rx_data_ch  = drv_dma_ch_alloc();
    if (spi_slave_rx_data_ch < 0)
    {
        return SPI_RX_RET_ENODMA;
    }
    drv_dma_isr_register(spi_slave_rx_data_ch, spi_slave_rx_dma_isr, NULL);
    p_spi_reg->stvSt = 0x7000;

    return SPI_INIT_SUCCESS;
}

void spi_slave_read_cfg()
{
    spi_reg * p_spi_reg = (spi_reg * )MEM_BASE_SPI1;
    unsigned int rx_num = 0;
    unsigned int *pdata = g_spi_cfg;
    unsigned int pnum = 0;

    while (((p_spi_reg->status) & SPI_STATUS_BUSY) == 1)
    {
        rx_num = ((p_spi_reg->status) >> 8) & 0x1F;
        if (rx_num > 0)
        {
            pdata[pnum++] = p_spi_reg->data;
        }
    }

    rx_num = ((p_spi_reg->status) >> 8) & 0x1F;
    if(rx_num > 0)
    {
        pdata[pnum++] = p_spi_reg->data;
    }
#ifdef ZERATUL_SPI_STATE
    g_spistat[g_spistnum++] = 0x77;
    g_spistnum = (g_spistnum >= 256) ? 0 : g_spistnum;
    g_spistat[g_spistnum++] = g_spi_cfg[0];
    g_spistnum = (g_spistnum >= 256) ? 0 : g_spistnum;
#endif

    if (g_slave_call_func)
        g_slave_call_func(SPI_SLAVE_CTRL_DATA_EVENT, g_spi_cfg);
}

void spi_slave_write_data(unsigned char *bufptr ,unsigned int length)
{
    unsigned long flags = arch_irq_save();
    spi_reg *p_spi_reg = (spi_reg * )MEM_BASE_SPI1;
    unsigned int *buff = (unsigned int *)bufptr;
    int i;
    spi_slave_clear_fifo();
    int len = (length+3) / 4 ;

    for (i = 0; i < len; i++)
    {
        if ((p_spi_reg ->status & SPI_STATUS_TXFULL) == 0)
        {
            p_spi_reg ->data = buff[i];
        }
        else
        {
            cli_printf("spi tx fifo full\r\n");
            arch_irq_restore(flags);
            return;
        }
    }
#ifdef ZERATUL_SPI_STATE
    g_spistat[g_spistnum++] = 0x88;
    g_spistnum = (g_spistnum >= 256) ? 0 : g_spistnum;
#endif
    arch_irq_restore(flags);
}

void spi_slave_load(void)
{
	#ifdef CONFIG_SYSTEM_IRQ
    unsigned long flags = system_irq_save();
	#endif
	
    spi_reg *p_spi_reg = (spi_reg *)MEM_BASE_SPI1;
    unsigned int temp;
    unsigned int value = 1;

    p_spi_reg->intrSt = SPI_STS_SLAVE_CMD_INT;
    temp = p_spi_reg->cmd & 0xFF;
    switch(temp)
    {
        case 0x51:
        case 0x52:
        case 0x54:
            value = p_spi_reg->ctrl & BIT3;
            if (value == 0)
            {
#ifdef ZERATUL_SPI_STATE
                g_spistat[g_spistnum++] = 0x51;
                g_spistnum = (g_spistnum >= 256) ? 0 : g_spistnum;
#endif
                spi_slave_read_cfg();
            }
            break;
        case 0x0B:
        case 0x0C:
        case 0x0E:
            break;
        case 0x05:
        case 0x15:
        case 0x25:
            value = p_spi_reg->ctrl & BIT3;
            if (value == 0 && (p_spi_reg->status & SPI_STATUS_RXNUM))
            {
#ifdef ZERATUL_SPI_STATE
                g_spistat[g_spistnum++] = 0x05;
                g_spistnum = (g_spistnum >= 256) ? 0 : g_spistnum;
#endif
                spi_slave_read_cfg();
            }
            break;

        default:
            cli_printf("spiSlave: unknow cmd: %d\n", temp);
        break;
    }

	#ifdef CONFIG_SYSTEM_IRQ
	system_irq_restore(flags);
	#endif
}

#ifdef ZERATUL_SPI_STATE
static int spi_slave_stat_dump(cmd_tbl_t *t, int argc, char *argv[])
{
    int idx;

    for (idx = 0; idx < 256; idx++) {
        cli_printf("spistate[%d-%d] 0x%x\n", idx, g_spistnum, g_spistat[idx]);
    }

    return CMD_RET_SUCCESS;
}

CLI_CMD(spi_slave_dump, spi_slave_stat_dump, "spi_slave_stat_dump", "spi_slave_stat_dump");
#endif

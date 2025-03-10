#include "spi_slave.h"
#include "chip_clk_ctrl.h"
#include "dma.h"
#include "chip_irqvector.h"
#include "oshal.h"
#include "arch_irq.h"
#include "gpio.h"
#include "cli.h"

#define SPI_SLAVE_DMA_TIME_MS 2000

#define SPI_SLAVE_RET_CHECK_RETURN(ret) \
    {\
        if ((ret) != 0) \
        {\
            os_printf(LM_OS, LL_ERR, "%s[%d]ret:%d\n", __FUNCTION__, __LINE__, ret); \
            return ret;\
        }\
    }

typedef struct {
    unsigned int count;
    spi_service_mem_t *head;
    spi_service_mem_t *tail;
} spi_slave_queue_t;

typedef struct {
    unsigned int spiDmaCh;
    os_timer_handle_t spiDmaTimer;
    spi_slave_queue_t qInfo;

    spi_service_ctrl_t spiCtlBuff[16];
    spi_service_event_e state;
    spi_service_mem_t *currMem;
    spi_service_func_t funcset;
} spi_slave_priv_t;

spi_slave_priv_t g_spi_priv;

#ifdef SPI_SLAVE_DEBUG
typedef struct {
    spi_service_ctrl_t ctrl;
} spi_slave_debug_t;

spi_slave_debug_t g_mdebug[128];
unsigned int g_mdbg_num;

static void spi_slave_debug_add(void)
{
    spi_slave_priv_t *priv = &g_spi_priv;

    g_mdebug[g_mdbg_num].ctrl = priv->spiCtlBuff[0];
    g_mdbg_num++;

    if (g_mdbg_num == 128) {
        g_mdbg_num = 0;
    }
}

static void spi_slave_debug_dump(void)
{
    int inx;

    os_printf(LM_APP, LL_ERR, "################%d#################\n", g_mdbg_num - 1);
    for (inx = 0; inx < 128; inx++) {
        os_printf(LM_APP, LL_ERR, "event%04d 0x%x:%d:%d\n", inx, g_mdebug[inx].ctrl.evt, g_mdebug[inx].ctrl.len, g_mdebug[inx].ctrl.type);
    }
    os_printf(LM_APP, LL_ERR, "###################################\n");
    while(1);
}
#endif


static void spi_slave_data_handle(void);
static void spi_slave_load_data(void);

static void spi_slave_queue_push(spi_service_mem_t *dataAddr)
{
    spi_slave_priv_t *priv = &g_spi_priv;
    spi_slave_queue_t *queue = &priv->qInfo;

    if (queue->head) {
        queue->tail->memNext = (unsigned int)dataAddr;
    } else {
        queue->head = dataAddr;
    }

    queue->tail = dataAddr;
    queue->tail->memNext = 0;
    queue->count++;
}

static spi_service_mem_t *spi_slave_queue_pop(void)
{
    spi_slave_priv_t *priv = &g_spi_priv;
    spi_slave_queue_t *queue = &priv->qInfo;
    spi_service_mem_t *dataAddr = 0;

    if (queue->count) {
        dataAddr = queue->head;
        queue->head = (spi_service_mem_t *)dataAddr->memNext;
        dataAddr->memNext = 0;
        queue->count--;
    }
    priv->currMem = dataAddr;

    return dataAddr;
}

static int spi_slave_queue_count(void)
{
    spi_slave_priv_t *priv = &g_spi_priv;

    return priv->qInfo.count;
}

void spi_slave_sendto_host(spi_service_mem_t *smem)
{
    unsigned int flags = system_irq_save();

    spi_slave_queue_push(smem);
    system_irq_restore(flags);

    spi_slave_tx_host_start();
}

static void spi_slave_clear_fifo()
{
    spi_reg *spiReg = (spi_reg *)MEM_BASE_SPI1;

    spiReg->ctrl = spiReg->ctrl | SPI_CONTROL_RXFIFORST | SPI_CONTROL_TXFIFORST;
    while (spiReg->ctrl & (SPI_CONTROL_RXFIFORST | SPI_CONTROL_TXFIFORST));
}

static void spi_slave_dma_timeout(os_timer_handle_t timer)
{
    spi_reg *spiReg = (spi_reg *)MEM_BASE_SPI1;
    unsigned int flags = arch_irq_save();
    spi_slave_priv_t *priv = &g_spi_priv;
    int state, dmaLeft;
    unsigned int type = priv->currMem->memType;

    dmaLeft = drv_dma_stop(priv->spiDmaCh);
    while (((spiReg->status) & SPI_STATUS_BUSY) == SPI_STATUS_BUSY);
    spi_slave_clear_fifo();
    if (priv->state == SPI_SERVICE_EVENT_RX) {
        if (priv->funcset.rxDone) {
            priv->funcset.rxDone(priv->currMem);
        }
        state = SPI_SERVICE_EVENT_RX;
    }

    if (priv->state == SPI_SERVICE_EVENT_TX) {
        if (priv->funcset.txDone) {
            priv->funcset.txDone(priv->currMem);
        }
        state = SPI_SERVICE_EVENT_TX;
    }

    spiReg->ctrl = SPI_CONTROL_TXTHRES(7);
    arch_irq_restore(flags);

    os_printf(LM_APP, LL_ERR, "SPI%d DMA TIMEOUT TYPE:0x%x LEFT:0x%x\n", state, type, dmaLeft);
#ifdef SPI_SLAVE_DEBUG
    spi_slave_debug_dump();
#endif
}

static void spi_slave_dma_isr(void *data)
{
    unsigned long flags = system_irq_save();
    spi_reg *spiReg = (spi_reg *)MEM_BASE_SPI1;
    spi_slave_priv_t *priv = &g_spi_priv;

    while (((spiReg->status) & SPI_STATUS_BUSY) == SPI_STATUS_BUSY);
    spi_slave_data_handle();
    spiReg->ctrl = SPI_CONTROL_TXTHRES(7);
    os_timer_stop(priv->spiDmaTimer);
    system_irq_restore(flags);
}

static void spi_slave_rx_data_start(unsigned int addr, unsigned int len, unsigned int stateLen)
{
    spi_reg *spiReg = (spi_reg *)MEM_BASE_SPI1;
    spi_slave_priv_t *priv = &g_spi_priv;
    T_DMA_CFG_INFO dmaInfo;
    unsigned int value = 0;

    os_printf(LM_OS, LL_DBG, "%s[%d]addr:0x%x len:%d stateLen:%d\n", __FUNCTION__, __LINE__, addr, len, stateLen);
    while (((spiReg->status) & SPI_STATUS_BUSY) == SPI_STATUS_BUSY);
    spi_slave_clear_fifo();
    if (len != 0) {
        value = spiReg->ctrl | SPI_CONTROL_RXDMA_EN | SPI_CONTROL_TXDMA_EN | SPI_CONTROL_TXTHRES(1);
        spiReg->ctrl = value;

        dmaInfo.dst = addr;
        dmaInfo.src = (unsigned int)&spiReg->data;
        dmaInfo.len = len;
        dmaInfo.mode = DMA_CHN_SPI_RX;
        drv_dma_cfg(priv->spiDmaCh, &dmaInfo);
        drv_dma_stop(priv->spiDmaCh);
        drv_dma_status_clean(priv->spiDmaCh);
        os_timer_start(priv->spiDmaTimer);
        drv_dma_start(priv->spiDmaCh);
    }

    priv->state = SPI_SERVICE_EVENT_RX;
    value = spiReg->stvSt & 0xFFFF0000;
    value |= BIT16 | stateLen;
    spiReg->stvSt = value;
}

void spi_slave_tx_data_start(unsigned int addr, unsigned int len, unsigned int stateLen)
{
    spi_reg *spiReg = (spi_reg *)MEM_BASE_SPI1;
    spi_slave_priv_t *priv = &g_spi_priv;
    T_DMA_CFG_INFO dmaInfo;
    unsigned int value = 0;

    os_printf(LM_OS, LL_DBG, "%s[%d]addr:0x%x len:%d stateLen:%d\n", __FUNCTION__, __LINE__, addr, len, stateLen);
    while (((spiReg->status) & SPI_STATUS_BUSY) == SPI_STATUS_BUSY);
    spi_slave_clear_fifo();
    if (len != 0) {
        value = spiReg->ctrl | SPI_CONTROL_RXDMA_EN | SPI_CONTROL_TXDMA_EN | SPI_CONTROL_TXTHRES(1);
        spiReg->ctrl = value;
#ifdef CONFIG_SPI_REPEATER
        int dmaLen;
        int offset = 0;

        do {
            dmaLen = (len - offset) > 512 ? 512 : (len - offset);
            dmaInfo.dst = (unsigned int)&spiReg->data;
            dmaInfo.src = addr + offset;
            dmaInfo.len = dmaLen;
            dmaInfo.mode = DMA_CHN_SPI_TX;
            drv_dma_cfg(priv->spiDmaCh, &dmaInfo);
            offset += dmaLen;

            if (offset != len) {
                dmaInfo.dst = (unsigned int)&spiReg->data;
                dmaInfo.src = addr + offset;
                dmaInfo.len = sizeof(unsigned int);
                dmaInfo.mode = DMA_CHN_SPI_TX;
                drv_dma_cfg(priv->spiDmaCh, &dmaInfo);
            }
        } while (offset != len);
#else
        dmaInfo.dst = (unsigned int)&spiReg->data;
        dmaInfo.src = (unsigned int)addr;
        dmaInfo.len = len;
        dmaInfo.mode = DMA_CHN_SPI_TX;
        drv_dma_cfg(priv->spiDmaCh, &dmaInfo);
#endif
        drv_dma_stop(priv->spiDmaCh);
        drv_dma_status_clean(priv->spiDmaCh);
        os_timer_start(priv->spiDmaTimer);
        drv_dma_start(priv->spiDmaCh);
    }

    priv->state = SPI_SERVICE_EVENT_TX;
    value = spiReg->stvSt & 0xFFFF0000;
    value |=  BIT16 | stateLen;

    spiReg->stvSt = value;
}

void spi_slave_funcset_register(spi_service_func_t funcset)
{
    spi_slave_priv_t *priv = &g_spi_priv;

    priv->funcset = funcset;
}

void spi_slave_tx_host_start(void)
{
    drv_gpio_write(SPI_SERVICE_GPIO_NUM(SPI_SLAVE_GPIO), 1);
}

void spi_slave_tx_host_stop(void)
{
    drv_gpio_write(SPI_SERVICE_GPIO_NUM(SPI_SLAVE_GPIO), 0);
}

static void spi_slave_pinmux_set(void)
{
    chip_clk_enable(CLK_SPI1_APB);
    PIN_FUNC_SET(IO_MUX_GPIO0, FUNC_GPIO0_SPI1_CLK);
    PIN_FUNC_SET(IO_MUX_GPIO1, FUNC_GPIO1_SPI1_CS0);
    PIN_FUNC_SET(IO_MUX_GPIO2, FUNC_GPIO2_SPI1_MOSI);
    PIN_FUNC_SET(IO_MUX_GPIO3, FUNC_GPIO3_SPI1_MIS0);

    PIN_MUX_SET(SPI_SERVICE_GPIO_REG(SPI_SLAVE_GPIO), SPI_SERVICE_GPIO_BIT(SPI_SLAVE_GPIO),
        SPI_SERVICE_GPIO_OFFSET(SPI_SLAVE_GPIO), SPI_SERVICE_GPIO_FUN(SPI_SLAVE_GPIO));
    drv_gpio_ioctrl(SPI_SERVICE_GPIO_NUM(SPI_SLAVE_GPIO), DRV_GPIO_CTRL_SET_DIRCTION, DRV_GPIO_ARG_DIR_OUT);
    drv_gpio_write(SPI_SERVICE_GPIO_NUM(SPI_SLAVE_GPIO), 0x0);
}

int spi_slave_init(bool reinit)
{
    unsigned int flags = system_irq_save();
    spi_reg *spiReg = (spi_reg *)MEM_BASE_SPI1;
    spi_slave_priv_t *priv = &g_spi_priv;

    WRITE_REG(TEST_MODE_EXTPAD_RST_EN, 0x0);
    os_usdelay(10);
    WRITE_REG(0x202018, 0x88);
    os_usdelay(10);

    spi_slave_pinmux_set();
    spi_slave_clear_fifo();

    spiReg->transFmt = SPI_FORMAT_CPHA_ODD | SPI_FORMAT_CPOL_LOW | SPI_FORMAT_SLAVE_MODE | SPI_FORMAT_DATA_MERGE(1) | SPI_FORMAT_DATA_LEN(7);
    spiReg->ctrl = SPI_CONTROL_TXTHRES(7);
    spiReg->transCtrl = SPI_SERVICE_TRANSCTL_MODE;

    spiReg->intrEn = SPI_INT_SLAVE_CMD_EN;
    arch_irq_register(VECTOR_NUM_SPI2, spi_slave_load_data);
    arch_irq_clean(VECTOR_NUM_SPI2);
    arch_irq_unmask(VECTOR_NUM_SPI2);

    if (reinit != 0) {
        system_irq_restore(flags);
        return 0;
    }

    priv->spiDmaCh = drv_dma_ch_alloc();
    if (priv->spiDmaCh < 0) {
        SPI_SLAVE_RET_CHECK_RETURN(priv->spiDmaCh);
    }
    drv_dma_isr_register(priv->spiDmaCh, spi_slave_dma_isr, priv);
    priv->spiDmaTimer = os_timer_create("spi_dma_timer", SPI_SLAVE_DMA_TIME_MS, 0, spi_slave_dma_timeout, NULL);
    system_irq_restore(flags);

    return 0;
}

int spi_slave_read_data(unsigned int *data, unsigned int len)
{
    spi_reg *spiReg = (spi_reg *)MEM_BASE_SPI1;
    unsigned int rxNum = 0;
    unsigned int rdNum = 0;
    int ret = 0;

    while (spiReg->status & SPI_STATUS_BUSY) {
        rxNum = (spiReg->status & SPI_STATUS_RXNUM) >> 8;
        if (rxNum > 0) {
            if (rdNum + rxNum >= len) {
                rdNum = 0;
                ret = -1;
            }
            data[rdNum++] = spiReg->data;
        }
    }

    rxNum = (spiReg->status & SPI_STATUS_RXNUM) >> 8;
    if (rxNum > 0) {
        if (rdNum + rxNum >= len) {
            rdNum = 0;
            ret = -1;
        }
        data[rdNum++] = spiReg->data;
    }

    return ret;
}

int spi_slave_write_data(unsigned int *data, unsigned int len)
{
    unsigned int flags = system_irq_save();
    spi_reg *spiReg = (spi_reg *)MEM_BASE_SPI1;
    int i;

    while (((spiReg->status) & SPI_STATUS_BUSY) == SPI_STATUS_BUSY);
    spi_slave_clear_fifo();
    for (i = 0; i < len; i++) {
        if ((spiReg->status & SPI_STATUS_TXFULL) == 0) {
            spiReg->data = data[i];
        } else {
            system_irq_restore(flags);
            SPI_SLAVE_RET_CHECK_RETURN(SPI_STATUS_TXFULL);
        }
    }

    system_irq_restore(flags);
    return 0;
}

static int spi_slave_read_cfg()
{
    spi_slave_priv_t *priv = &g_spi_priv;
    int ret;

    ret = spi_slave_read_data((unsigned int *)priv->spiCtlBuff, sizeof(priv->spiCtlBuff));
    SPI_SLAVE_RET_CHECK_RETURN(ret);

    priv->state = SPI_SERVICE_EVENT_CTRL;
    spi_slave_data_handle();

    return 0;
}

static void spi_slave_read_host(void)
{
    spi_slave_priv_t *priv = &g_spi_priv;
    spi_service_ctrl_t *cfg = priv->spiCtlBuff;

    priv->currMem = priv->funcset.rxPrepare(cfg);
    if (priv->currMem != NULL) {
        spi_slave_rx_data_start(priv->currMem->memAddr + priv->currMem->memOffset, priv->currMem->memLen, priv->currMem->memSlen);
    } else {
        spi_slave_rx_data_start(0, 0, 0);
    }
}

static void spi_slave_send_host(void)
{
    spi_slave_priv_t *priv = &g_spi_priv;
    spi_service_ctrl_t *cfg = priv->spiCtlBuff;
    int dataNum = spi_slave_queue_count();

    priv->currMem = priv->funcset.txPrepare(cfg);
    if (priv->currMem != NULL) {
        if (dataNum == 0) {
            spi_slave_tx_host_stop();
        }
        spi_slave_tx_data_start(priv->currMem->memAddr + priv->currMem->memOffset, priv->currMem->memLen, priv->currMem->memSlen);
        return;
    }

    if (dataNum != 0) {
        priv->currMem = spi_slave_queue_pop();
        dataNum = spi_slave_queue_count();
        if (dataNum == 0) {
            spi_slave_tx_host_stop();
        }

        spi_slave_tx_data_start(priv->currMem->memAddr + priv->currMem->memOffset, priv->currMem->memLen, priv->currMem->memSlen);
    } else {
        // never here as normal
        spi_slave_tx_data_start(0, 0, 0);
    }
}

static void spi_slave_data_handle(void)
{
    spi_slave_priv_t *priv = &g_spi_priv;
    spi_service_ctrl_t *cfg = priv->spiCtlBuff;

    switch (priv->state) {
        case SPI_SERVICE_EVENT_CTRL:
#ifdef SPI_SLAVE_DEBUG
            spi_slave_debug_add();
#endif
            os_printf(LM_OS, LL_DBG, "%s[%d]evt 0x%x len 0x%x type 0x%x\n", __FUNCTION__, __LINE__, cfg->evt, cfg->len, cfg->type);
            switch (cfg->type) {
                case SPI_SLAVE_TYPE_READ:
                    spi_slave_read_host();
                    break;
                case SPI_SLAVE_TYPE_WRITE:
                    spi_slave_send_host();
                    break;
                default:
                    os_printf(LM_OS, LL_ERR, "%s[%d]evt 0x%x len 0x%x type 0x%x\n", __FUNCTION__, __LINE__, cfg->evt, cfg->len, cfg->type);
#ifdef SPI_SLAVE_DEBUG
                    spi_slave_debug_dump();
#endif
            }
            break;
        case SPI_SERVICE_EVENT_TX:
            if (priv->funcset.txDone) {
                priv->funcset.txDone(priv->currMem);
            }
            break;
        case SPI_SERVICE_EVENT_RX:
            if (priv->funcset.dataHandle) {
                priv->funcset.dataHandle(priv->currMem);
            }

            if (priv->funcset.rxDone) {
                priv->funcset.rxDone(priv->currMem);
            }
            break;
    }
}

static void spi_slave_load_data(void)
{
    unsigned int flags = system_irq_save();
    spi_reg *spiReg = (spi_reg *)MEM_BASE_SPI1;

    spiReg->intrSt = SPI_STS_SLAVE_CMD_INT;
    switch (spiReg->cmd & 0xFF) {
        case SPI_SERVICE_READ_CMD:
            break;
        case SPI_SERVICE_WRITE_CMD:
        case SPI_SERVICE_STATE_CMD:
            if ((spiReg->ctrl & SPI_CONTROL_RXDMA_EN) == 0 && (spiReg->status & SPI_STATUS_RXNUM)) {
                spi_slave_read_cfg();
            }
            break;

        default:
            os_printf(LM_OS, LL_ERR, "spiSlave: unknow cmd: %d\n", spiReg->cmd);
        break;
    }

    system_irq_restore(flags);
}


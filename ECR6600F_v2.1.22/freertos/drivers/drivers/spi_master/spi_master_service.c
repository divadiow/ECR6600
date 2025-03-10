#include "spi_master.h"
#include "chip_clk_ctrl.h"
#include "dma.h"
#include "chip_irqvector.h"
#include "oshal.h"
#include "arch_irq.h"
#include "gpio.h"
#include "cli.h"

uint8_t g_drop_mem[SPI_MASTER_MAX_PACKETLEN] __attribute__ ((section("SHAREDRAM")));
unsigned int g_drop_state;
spi_service_mem_t g_drop_smem;

#define SPI_MASTER_DMA_TIME_MS 2000

#define SPI_MASTER_RET_CHECK_RETURN(ret) \
    {\
        if ((ret) != 0) \
        {\
            os_printf(LM_OS, LL_ERR, "%s[%d]ret:%d\n", __FUNCTION__, __LINE__, ret); \
            return ret;\
        }\
    }

typedef struct {
    unsigned int spiDmaCh;
    os_timer_handle_t spiDmaTimer;
    os_sem_handle_t spiBusSem;
    os_sem_handle_t spiIrqSem;
    os_queue_handle_t spiQueue;
    T_GPIO_ISR_CALLBACK gpioCall;
    int gpioValue;

    spi_service_ctrl_t spiCtlBuff[16];
    spi_service_event_e state;
    spi_service_mem_t *currMem;
    spi_service_func_t funcset;
} spi_master_priv_t;

spi_master_priv_t g_spi_priv;

#ifdef SPI_MASTER_DEBUG
typedef struct {
    spi_service_event_e state;
    spi_service_mem_t smem;
    spi_service_ctrl_t ctrl;
} spi_master_debug_t;

spi_master_debug_t g_mdebug[128];
unsigned int g_mdbg_num;

static void spi_master_debug_add(void)
{
    spi_master_priv_t *priv = &g_spi_priv;
    spi_master_debug_t *dbg = &g_mdebug[g_mdbg_num++];

    dbg->state = priv->state;
    dbg->smem = *(priv->currMem);
    dbg->ctrl = priv->spiCtlBuff[0];

    if (g_mdbg_num == 128) {
        g_mdbg_num = 0;
    }
}

static void spi_master_debug_dump(void)
{
    unsigned int flags = arch_irq_save();
    spi_master_debug_t *dbg = NULL;
    int inx;

    os_printf(LM_APP, LL_ERR, "################%d#################\n", g_mdbg_num - 1);
    for (inx = 0; inx < 128; inx++) {
        dbg = &g_mdebug[inx];
        os_printf(LM_APP, LL_ERR, "event%4d  %d\n", inx, dbg->state);
        os_printf(LM_APP, LL_ERR, "ctrl     0x%x:%d:%d\n", dbg->ctrl.evt, dbg->ctrl.len, dbg->ctrl.type);
        os_printf(LM_APP, LL_ERR, "data     0x%x:%d:%d\n", dbg->smem.memType, dbg->smem.memLen, dbg->smem.memSlen);
    }
    os_printf(LM_APP, LL_ERR, "###################################\n");
    arch_irq_restore(flags);
    while(1);
}
#endif

void spi_master_sendto_peer(spi_service_mem_t *smem)
{
    spi_master_priv_t *priv = &g_spi_priv;

    os_queue_send(priv->spiQueue, (char *)&smem, sizeof(spi_service_mem_t *), WAIT_FOREVER);
}

static void spi_master_data_handle(void)
{
    spi_master_priv_t *priv = &g_spi_priv;

    switch (priv->state) {
        case SPI_SERVICE_EVENT_CTRL:
            if (priv->funcset.dataHandle) {
                priv->funcset.dataHandle(priv->currMem);
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

static void spi_master_clear_fifo()
{
    spi_reg *spiReg = (spi_reg *)MEM_BASE_SPI1;

    spiReg->ctrl = spiReg->ctrl | SPI_CONTROL_RXFIFORST | SPI_CONTROL_TXFIFORST;
    while (spiReg->ctrl & (SPI_CONTROL_RXFIFORST | SPI_CONTROL_TXFIFORST));
}

static void spi_master_dma_timeout(os_timer_handle_t timer)
{
    spi_reg *spiReg = (spi_reg *)MEM_BASE_SPI1;
    unsigned int flags = arch_irq_save();
    spi_master_priv_t *priv = &g_spi_priv;
    int state, dmaLeft;
    unsigned int type = priv->currMem->memType;

    dmaLeft = drv_dma_stop(priv->spiDmaCh);
    while (((spiReg->status) & SPI_STATUS_BUSY) == SPI_STATUS_BUSY);
    spi_master_clear_fifo();
    if (priv->state == SPI_SERVICE_EVENT_RX) {
        if (g_drop_state == 0 && priv->funcset.rxDone) {
            priv->funcset.rxDone(priv->currMem);
        } else {
            g_drop_state = 0;
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
    os_sem_post(priv->spiBusSem);
    arch_irq_restore(flags);

    os_printf(LM_APP, LL_ERR, "SPI%d DMA TIMEOUT TYPE:0x%x LEFT:0x%x\n", state, type, dmaLeft);
#ifdef SPI_SLAVE_DEBUG
    spi_slave_debug_dump();
#endif
}

static void spi_master_dma_isr(void *data)
{
    unsigned long flags = system_irq_save();
    spi_reg *spiReg = (spi_reg *)MEM_BASE_SPI1;
    spi_master_priv_t *priv = &g_spi_priv;
    T_DMA_CFG_INFO dmaInfo;

    while (((spiReg->status) & SPI_STATUS_BUSY) == SPI_STATUS_BUSY);
    spi_master_clear_fifo();
    unsigned int transLen = priv->currMem->memSlen > SPI_MASTER_MAX_TRANSLEN ? SPI_MASTER_MAX_TRANSLEN : priv->currMem->memSlen;
    os_printf(LM_OS, LL_DBG, "%s[%d]addr:0x%x len:%d sLen:%d\n", __FUNCTION__, __LINE__, priv->currMem->memAddr, transLen, priv->currMem->memSlen);

    if (transLen && priv->state == SPI_SERVICE_EVENT_TX) {
        dmaInfo.dst = (unsigned int)&spiReg->data;
        dmaInfo.src = priv->currMem->memAddr + priv->currMem->memOffset + priv->currMem->memLen - priv->currMem->memSlen;
        dmaInfo.len = transLen;
        dmaInfo.mode = DMA_CHN_SPI_TX;
        drv_dma_stop(priv->spiDmaCh);
        drv_dma_cfg(priv->spiDmaCh, &dmaInfo);
        drv_dma_status_clean(priv->spiDmaCh);
        drv_dma_start(priv->spiDmaCh);
        spiReg->transCtrl = SPI_SERVICE_TRANSCTL_MODE | SPI_TRANSCTRL_TRAMODE_DW | SPI_TRANSCTRL_CMDEN | SPI_TRANSCTRL_WCNT(transLen - 1);
        spiReg->cmd = SPI_SERVICE_WRITE_CMD;
    }

    if (transLen && priv->state == SPI_SERVICE_EVENT_RX) {
        dmaInfo.dst = priv->currMem->memAddr + priv->currMem->memOffset + priv->currMem->memLen - priv->currMem->memSlen;
        dmaInfo.src = (unsigned int)&spiReg->data;
        dmaInfo.len = transLen;
        dmaInfo.mode = DMA_CHN_SPI_RX;
        drv_dma_stop(priv->spiDmaCh);
        drv_dma_cfg(priv->spiDmaCh, &dmaInfo);
        drv_dma_status_clean(priv->spiDmaCh);
        drv_dma_start(priv->spiDmaCh);
        spiReg->transCtrl = SPI_SERVICE_TRANSCTL_MODE | SPI_TRANSCTRL_TRAMODE_DR | SPI_TRANSCTRL_CMDEN | SPI_TRANSCTRL_RCNT(transLen - 1);
        spiReg->cmd = SPI_SERVICE_READ_CMD;
        drv_dma_start(priv->spiDmaCh);
    }

    priv->currMem->memSlen -= transLen;
    if (transLen == 0) {
        os_timer_stop(priv->spiDmaTimer);
        spiReg->ctrl = SPI_CONTROL_TXTHRES(7);
        if (g_drop_state == 1) {
            g_drop_state = 0;
        } else {
            spi_master_data_handle();
        }
        os_sem_post(priv->spiBusSem);
    }

    system_irq_restore(flags);
}

static void spi_master_rx_data_start(void)
{
    spi_reg *spiReg = (spi_reg *)MEM_BASE_SPI1;
    spi_master_priv_t *priv = &g_spi_priv;
    T_DMA_CFG_INFO dmaInfo;
    unsigned int transLen;
#ifdef SPI_MASTER_DEBUG
    spi_master_debug_add();
#endif
    while (((spiReg->status) & SPI_STATUS_BUSY) == SPI_STATUS_BUSY);
    spi_master_clear_fifo();

    spiReg->ctrl = spiReg->ctrl | SPI_CONTROL_RXDMA_EN | SPI_CONTROL_TXDMA_EN | SPI_CONTROL_TXTHRES(1);
    transLen = (priv->currMem->memLen > SPI_MASTER_MAX_TRANSLEN) ? SPI_MASTER_MAX_TRANSLEN : priv->currMem->memLen;
    priv->currMem->memSlen = priv->currMem->memLen - transLen;
    priv->state = SPI_SERVICE_EVENT_RX;
    os_printf(LM_OS, LL_DBG, "%s[%d]addr:0x%x len:%d sLen:%d\n", __FUNCTION__, __LINE__, priv->currMem->memAddr, transLen, priv->currMem->memSlen);

    dmaInfo.dst = priv->currMem->memAddr + priv->currMem->memOffset;
    dmaInfo.src = (unsigned int)&spiReg->data;
    dmaInfo.len = transLen;
    dmaInfo.mode = DMA_CHN_SPI_RX;
    drv_dma_stop(priv->spiDmaCh);
    drv_dma_cfg(priv->spiDmaCh, &dmaInfo);
    drv_dma_status_clean(priv->spiDmaCh);
    os_timer_start(priv->spiDmaTimer);
    drv_dma_start(priv->spiDmaCh);
    spiReg->transCtrl = SPI_SERVICE_TRANSCTL_MODE | SPI_TRANSCTRL_TRAMODE_DR | SPI_TRANSCTRL_CMDEN | SPI_TRANSCTRL_RCNT(transLen - 1);
    spiReg->cmd = SPI_SERVICE_READ_CMD;
}

void spi_master_tx_data_start(void)
{
    spi_reg *spiReg = (spi_reg *)MEM_BASE_SPI1;
    spi_master_priv_t *priv = &g_spi_priv;
    T_DMA_CFG_INFO dmaInfo;
    unsigned int transLen;

#ifdef SPI_MASTER_DEBUG
    spi_master_debug_add();
#endif
    while (((spiReg->status) & SPI_STATUS_BUSY) == SPI_STATUS_BUSY);
    spi_master_clear_fifo();

    spiReg->ctrl = spiReg->ctrl | SPI_CONTROL_RXDMA_EN | SPI_CONTROL_TXDMA_EN | SPI_CONTROL_TXTHRES(1);
    transLen = (priv->currMem->memLen > SPI_MASTER_MAX_TRANSLEN) ? SPI_MASTER_MAX_TRANSLEN : priv->currMem->memLen;
    priv->currMem->memSlen = priv->currMem->memLen - transLen;
    priv->state = SPI_SERVICE_EVENT_TX;
    os_printf(LM_OS, LL_DBG, "%s[%d]addr:0x%x len:%d sLen:%d\n", __FUNCTION__, __LINE__, priv->currMem->memAddr, transLen, priv->currMem->memSlen);

    dmaInfo.dst = (unsigned int)&spiReg->data;
    dmaInfo.src = priv->currMem->memAddr + priv->currMem->memOffset;
    dmaInfo.len = transLen;
    dmaInfo.mode = DMA_CHN_SPI_TX;
    drv_dma_stop(priv->spiDmaCh);
    drv_dma_cfg(priv->spiDmaCh, &dmaInfo);
    drv_dma_status_clean(priv->spiDmaCh);
    os_timer_start(priv->spiDmaTimer);
    drv_dma_start(priv->spiDmaCh);
    spiReg->transCtrl = SPI_SERVICE_TRANSCTL_MODE | SPI_TRANSCTRL_TRAMODE_DW | SPI_TRANSCTRL_CMDEN | SPI_TRANSCTRL_WCNT(transLen - 1);
    spiReg->cmd = SPI_SERVICE_WRITE_CMD;
}

void spi_master_funcset_register(spi_service_func_t funcset)
{
    spi_master_priv_t *priv = &g_spi_priv;

    priv->funcset = funcset;
}

static void spi_master_gpio_int_handle(void *arg)
{
    spi_master_priv_t *priv = (spi_master_priv_t *)arg;

    drv_gpio_ioctrl(SPI_SERVICE_GPIO_NUM(SPI_MASTER_GPIO), DRV_GPIO_CTRL_INTR_DISABLE, DRV_GPIO_CTRL_INTR_DISABLE);
    os_sem_post(priv->spiIrqSem);
}

static void spi_master_pinmux_set(void)
{
    spi_master_priv_t *priv = &g_spi_priv;

    chip_clk_enable(CLK_SPI1_APB);
    PIN_FUNC_SET(IO_MUX_GPIO0, FUNC_GPIO0_SPI1_CLK);
    PIN_FUNC_SET(IO_MUX_GPIO1, FUNC_GPIO1_SPI1_CS0);
    PIN_FUNC_SET(IO_MUX_GPIO2, FUNC_GPIO2_SPI1_MOSI);
    PIN_FUNC_SET(IO_MUX_GPIO3, FUNC_GPIO3_SPI1_MIS0);

    priv->gpioCall.gpio_callback = spi_master_gpio_int_handle;
    priv->gpioCall.gpio_data = priv;
    PIN_MUX_SET(SPI_SERVICE_GPIO_REG(SPI_MASTER_GPIO), SPI_SERVICE_GPIO_BIT(SPI_MASTER_GPIO),
        SPI_SERVICE_GPIO_OFFSET(SPI_MASTER_GPIO), SPI_SERVICE_GPIO_FUN(SPI_MASTER_GPIO));
    drv_gpio_ioctrl(SPI_SERVICE_GPIO_NUM(SPI_MASTER_GPIO), DRV_GPIO_CTRL_INTR_MODE, DRV_GPIO_ARG_INTR_MODE_HIGH);
    drv_gpio_ioctrl(SPI_SERVICE_GPIO_NUM(SPI_MASTER_GPIO), DRV_GPIO_CTRL_REGISTER_ISR, (int)&priv->gpioCall);
    drv_gpio_ioctrl(SPI_SERVICE_GPIO_NUM(SPI_MASTER_GPIO), DRV_GPIO_CTRL_INTR_ENABLE, DRV_GPIO_CTRL_INTR_ENABLE);
}

int spi_master_read_data(unsigned int *data, unsigned int len)
{
    spi_reg *spiReg = (spi_reg *)MEM_BASE_SPI1;
    unsigned int rxNum = 0;
    unsigned int rdNum = 0;
    int ret = 0;

    while (spiReg->status & SPI_STATUS_BUSY) {
        rxNum = ((spiReg->status) >> 8) & 0x1F;
        if (rxNum > 0) {
            if (rdNum + rxNum >= len) {
                rdNum = 0;
                ret = -1;
            }
            data[rdNum++] = spiReg->data;
        }
    }

    rxNum = ((spiReg->status) >> 8) & 0x1F;
    if (rxNum > 0) {
        if (rdNum + rxNum >= len) {
            rdNum = 0;
            ret = -1;
        }
        data[rdNum++] = spiReg->data;
    }

    return ret;
}

int spi_master_write_data(unsigned int *data, unsigned int len)
{
    unsigned int flags = system_irq_save();
    spi_reg *spiReg = (spi_reg *)MEM_BASE_SPI1;
    int i;

    while (((spiReg->status) & SPI_STATUS_BUSY) == SPI_STATUS_BUSY);
    spi_master_clear_fifo();
    spiReg->transCtrl = SPI_SERVICE_TRANSCTL_MODE | SPI_TRANSCTRL_TRAMODE_DW | SPI_TRANSCTRL_CMDEN | SPI_TRANSCTRL_WCNT(len - 1);
    spiReg->cmd = SPI_SERVICE_WRITE_CMD;
    for (i = 0; i < len/4; i++) {
        if ((spiReg->status & SPI_STATUS_TXFULL) == 0) {
            spiReg->data = data[i];
        } else {
            system_irq_restore(flags);
            SPI_MASTER_RET_CHECK_RETURN(SPI_STATUS_TXFULL);
        }
    }

    system_irq_restore(flags);
    return 0;
}

static int spi_master_read_state(void)
{
    spi_reg *spiReg = (spi_reg *)MEM_BASE_SPI1;
    unsigned int state;
    unsigned int flags;
    int trycnt = 0;

    do {
        flags = system_irq_save();
        while (((spiReg->status) & SPI_STATUS_BUSY) == SPI_STATUS_BUSY);
        spi_master_clear_fifo();
        spiReg->transCtrl = SPI_SERVICE_TRANSCTL_MODE | SPI_TRANSCTRL_TRAMODE_DR | SPI_TRANSCTRL_CMDEN | SPI_TRANSCTRL_RCNT(3);
        spiReg->cmd = SPI_SERVICE_STATE_CMD;
        spi_master_read_data(&state, sizeof(state));
        if (state & 0x10000) {
            system_irq_restore(flags);
            break;
        }
        system_irq_restore(flags);

        if (trycnt > 10) {
            os_msleep(1);
        }
    } while (trycnt++ < 1000);

    return state;
}

static int spi_master_send_packet(spi_service_mem_t *smem)
{
    spi_master_priv_t *priv = &g_spi_priv;
    spi_service_ctrl_t *cfg = priv->spiCtlBuff;
    int state;

    cfg->evt = smem->memType;
    cfg->len = smem->memLen;
    cfg->type = SPI_MASTER_TYPE_WRITE;
    spi_master_write_data((unsigned int *)cfg, sizeof(spi_service_ctrl_t));

    state = spi_master_read_state();
    if ((state & 0x10000) == 0) {
        if (priv->funcset.txDone) {
            priv->funcset.txDone(smem);
        }
        os_sem_post(priv->spiBusSem);
        os_printf(LM_OS, LL_ERR, "spi send state timeout.\n");
#ifdef SPI_MASTER_DEBUG
        spi_master_debug_add();
        spi_master_debug_dump();
#endif
        return -1;
    }

    state = state & 0xFFFF;
    if (smem->memLen != state) {
        if (state) {
            if (priv->funcset.txDone) {
                priv->funcset.txDone(smem);
            }
            os_sem_post(priv->spiBusSem);
            os_printf(LM_OS, LL_ERR, "DMA send not match memLen %d-%d\n", smem->memLen, state);
#ifdef SPI_MASTER_DEBUG
            spi_master_debug_add();
            spi_master_debug_dump();
#endif
            return -1;
        } else {
            os_sem_post(priv->spiBusSem);
            os_msleep(1);
            return -2;
        }
    }

    priv->currMem = smem;
    spi_master_tx_data_start();
    return 0;
}

void spi_master_read_info(unsigned int len)
{
    spi_master_priv_t *priv = &g_spi_priv;
    spi_service_ctrl_t *cfg = priv->spiCtlBuff;
    int state;

    state = os_sem_wait(priv->spiBusSem, WAIT_FOREVER);
    if (state < 0) {
        os_printf(LM_OS, LL_ERR, "spi bus recv info sem take error.\n");
        return;
    }

    cfg->evt = SPI_SERVICE_TYPE_INFO;
    cfg->len = len;
    cfg->type = SPI_MASTER_TYPE_READ;
    spi_master_write_data((unsigned int *)cfg, sizeof(spi_service_ctrl_t));

    state = spi_master_read_state();
    if ((state & 0x10000) == 0) {
        os_sem_post(priv->spiBusSem);
        os_printf(LM_OS, LL_ERR, "spi read info state timeout.\n");
        return;
    }

    if (cfg->len == 0) {
        os_sem_post(priv->spiBusSem);
        os_printf(LM_OS, LL_ERR, "read info may something error.\n");
        return;
    }

    do {
        priv->currMem = priv->funcset.rxPrepare(cfg);
        if (priv->currMem != NULL) {
            spi_master_rx_data_start();
            return;
        } else {
            os_printf(LM_OS, LL_ERR, "read info mem error.\n");
        }

        os_msleep(10);
    } while (priv->currMem == NULL);
}

static void spi_master_read_packet(void)
{
    spi_master_priv_t *priv = &g_spi_priv;
    spi_service_ctrl_t *cfg = priv->spiCtlBuff;
    int state;

    cfg->evt = SPI_SERVICE_TYPE_STOH;
    cfg->len = 0;
    cfg->type = SPI_MASTER_TYPE_READ;
    spi_master_write_data((unsigned int *)cfg, sizeof(spi_service_ctrl_t));

    state = spi_master_read_state();
    if ((state & 0x10000) == 0) {
        os_sem_post(priv->spiBusSem);
#ifdef SPI_MASTER_DEBUG
        spi_master_debug_add();
        spi_master_debug_dump();
#endif
        os_printf(LM_OS, LL_ERR, "spi read state timeout.\n");
        return;
    }

    if ((state & SPI_SERVICE_CONTROL_MSG) == SPI_SERVICE_CONTROL_MSG) {
        cfg->evt = SPI_SERVICE_TYPE_MSG;
        cfg->len = state & 0xFFF;
    } else if ((state & SPI_SERVICE_CONTROL_INT) == SPI_SERVICE_CONTROL_INT) {
        cfg->evt = SPI_SERVICE_TYPE_INT;
    } else {
        cfg->len = state & 0xFFFF;
    }

    if (cfg->len == 0 && cfg->evt != SPI_SERVICE_TYPE_INT) {
        os_sem_post(priv->spiBusSem);
        os_printf(LM_OS, LL_ERR, "read 0x%x may something error\n", cfg->evt);
        return;
    }

    do {
        priv->currMem = priv->funcset.rxPrepare(cfg);
        if (priv->currMem != NULL) {
            spi_master_rx_data_start();
            return;
        }

        if (cfg->evt == SPI_SERVICE_TYPE_INT) {
            os_sem_post(priv->spiBusSem);
            break;
        }

        g_drop_state = 1;
        g_drop_smem.memAddr = (unsigned int)g_drop_mem;
        g_drop_smem.memLen = (cfg->len + 1) & (~1);
        g_drop_smem.memOffset = 0;
        g_drop_smem.memType = SPI_SERVICE_TYPE_STOH;
        priv->currMem = &g_drop_smem;
        spi_master_rx_data_start();
    } while (0);
}

static void spi_master_rx_thread(void *arg)
{
    spi_master_priv_t *priv = (spi_master_priv_t *)arg;
    int ret;

    do {
        priv->gpioValue = drv_gpio_read(SPI_SERVICE_GPIO_NUM(SPI_MASTER_GPIO));
        if (priv->gpioValue == 0) {
            drv_gpio_ioctrl(SPI_SERVICE_GPIO_NUM(SPI_MASTER_GPIO), DRV_GPIO_CTRL_INTR_ENABLE, DRV_GPIO_CTRL_INTR_ENABLE);
            ret = os_sem_wait(priv->spiIrqSem, WAIT_FOREVER);
            if (ret < 0) {
                os_printf(LM_OS, LL_ERR, "spi rx sem take error.\n");
                break;
            }
        }

        ret = os_sem_wait(priv->spiBusSem, WAIT_FOREVER);
        if (ret < 0) {
            os_printf(LM_OS, LL_ERR, "spi bus recv sem take error.\n");
            return;
        }

        spi_master_read_packet();
    } while (1);

    os_printf(LM_OS, LL_ERR, "spi rx thread exit.\n");
}

static void spi_master_tx_thread(void *arg)
{
    spi_master_priv_t *priv = (spi_master_priv_t *)arg;
    spi_service_mem_t *smem;
    int ret;

    do {
        ret = os_queue_receive(priv->spiQueue, (char *)&smem, sizeof(spi_service_mem_t *), WAIT_FOREVER);
        if (ret < 0) {
            os_printf(LM_OS, LL_ERR, "spi queue recv error.\n");
            break;
        }

RETRY:
        ret = os_sem_wait(priv->spiBusSem, WAIT_FOREVER);
        if (ret < 0) {
            os_printf(LM_OS, LL_ERR, "spi bus send sem take error.\n");
            return;
        }

        ret = spi_master_send_packet(smem);
        if (ret == -2) {
            goto RETRY;
        }
    } while (1);

    os_printf(LM_OS, LL_ERR, "spi tx thread exit.\n");
}

int spi_master_init(void)
{
    unsigned int flags = system_irq_save();
    spi_reg *spiReg = (spi_reg *)MEM_BASE_SPI1;
    spi_master_priv_t *priv = &g_spi_priv;

    WRITE_REG(TEST_MODE_EXTPAD_RST_EN, 0x0);
    os_usdelay(10);
    WRITE_REG(0x202018, 0x88);
    os_usdelay(10);

    spi_master_pinmux_set();
    spi_master_clear_fifo();

    spiReg->transFmt = SPI_FORMAT_CPHA_ODD | SPI_FORMAT_CPOL_LOW | SPI_FORMAT_MASTER_MODE | SPI_FORMAT_DATA_MERGE(1) | SPI_FORMAT_DATA_LEN(7);
    spiReg->ctrl = SPI_CONTROL_TXTHRES(7);

    priv->spiDmaCh = drv_dma_ch_alloc();
    if (priv->spiDmaCh < 0) {
        SPI_MASTER_RET_CHECK_RETURN(priv->spiDmaCh);
    }
    drv_dma_isr_register(priv->spiDmaCh, spi_master_dma_isr, priv);
    priv->spiDmaTimer = os_timer_create("spi_dma_timer", SPI_MASTER_DMA_TIME_MS, 0, spi_master_dma_timeout, NULL);
    priv->spiQueue = os_queue_create("spi queue", SPI_MASTER_QUEUE_DEP, sizeof(spi_service_mem_t *), 0);

    priv->spiBusSem = os_sem_create(1, 1);
    priv->spiIrqSem = os_sem_create(1, 0);

    os_task_create("spiRxTask", SPI_MASTER_RX_TASK_RPI, SPI_MASTER_STACK_DEP, spi_master_rx_thread, priv);
    os_task_create("spiTxTask", SPI_MASTER_TX_TASK_RPI, SPI_MASTER_STACK_DEP, spi_master_tx_thread, priv);
    system_irq_restore(flags);

    return 0;
}

#ifdef SPI_MASTER_DEBUG
static int spi_master_debug_get(cmd_tbl_t *t, int argc, char *argv[])
{
    spi_master_debug_dump();

    return CMD_RET_SUCCESS;
}

CLI_CMD(spi_master_debug, spi_master_debug_get, "spi_master_debug_get", "spi_master_debug_get");
#endif


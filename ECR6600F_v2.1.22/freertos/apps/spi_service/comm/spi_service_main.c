#include "spi_service_main.h"
#include "spi_service_mem.h"
#ifdef CONFIG_SPI_REPEATER
#include "spi_repeater.h"
#endif
#include "spi_master.h"
#include "spi_slave.h"
#include "vnet_filter.h"
#include "vnet_service.h"
#include "vnet_register.h"
#include "vnet_int.h"
#include "lwip/tcpip.h"
#include "ota.h"
#include "pit.h"

unsigned int g_packet_size;
unsigned int g_packet_count;
unsigned int g_time_cnt;

#ifdef SPI_SERVICE_LOOP_TEST
static int spi_service_packet_pm(spi_service_mem_t *smem)
{
#ifdef SPI_SERVICE_PM_CNT
    unsigned int time_cnt;

    g_packet_size += smem->memLen;
    if (g_time_cnt == 0) {
        g_time_cnt = drv_pit_get_tick();
    }

    g_packet_count++;
    time_cnt = drv_pit_get_tick();
    time_cnt -= g_time_cnt;
    if (time_cnt / 40 > 1000000) {
        os_printf(LM_OS, LL_INFO, "%u[%u][%u]\n", g_packet_size, g_packet_count, time_cnt);
        g_time_cnt = 0;
        g_packet_size = 0;
        g_packet_count = 0;
    }
#endif
#ifdef SPI_SERVICE_PM_DATA
    int idx;
    unsigned char *pm = (unsigned char *)smem->memAddr + smem->memOffset;

    os_printf(LM_OS, LL_ERR, "###############addr:0x%x:%d################\n", smem->memAddr, smem->memLen);
    for (idx = 0; idx < smem->memLen; idx++) {
        os_printf(LM_OS, LL_ERR, "0x%02x ", pm[idx]);
        if ((idx + 1) % 16 == 0) {
            os_printf(LM_OS, LL_ERR, "\n");
        }
    }
    os_printf(LM_OS, LL_ERR, "\n");
    os_printf(LM_OS, LL_ERR, "##########################################\n");
#endif

    return 0;
}
#endif

#define SPI_SMEM_QUEUE_MAX 128
typedef struct spi_smem_queue_ {
    spi_service_mem_t smem;
    struct spi_smem_queue_ *next;
} spi_smem_queue_t;

spi_smem_queue_t *g_smem_queue;

void spi_mqueue_init(void)
{
    int idx;
    spi_smem_queue_t *mqueue;

    for (idx = 0; idx < SPI_SMEM_QUEUE_MAX; idx++) {
        mqueue = (spi_smem_queue_t *)os_malloc(sizeof(spi_smem_queue_t));
        if (mqueue != NULL) {
            memset(mqueue, 0, sizeof(spi_smem_queue_t));
            mqueue->next = g_smem_queue;
            g_smem_queue = mqueue;
        }
    }
}

spi_service_mem_t *spi_mqueue_get(void)
{
    unsigned int flag = system_irq_save();
    spi_smem_queue_t *mqueue = NULL;

    if (g_smem_queue != NULL) {
        mqueue = g_smem_queue;
        g_smem_queue = g_smem_queue->next;
        mqueue->next = NULL;
    }
    system_irq_restore(flag);
    return (spi_service_mem_t *)mqueue;
}

void spi_mqueue_put(spi_service_mem_t *queue)
{
    unsigned int flag = system_irq_save();
    spi_smem_queue_t *mqueue = (spi_smem_queue_t *)queue;

    if (mqueue != NULL) {
        memset(mqueue, 0, sizeof(spi_smem_queue_t));
        mqueue->next = g_smem_queue;
        g_smem_queue = mqueue;
    }

    system_irq_restore(flag);
}

void *spi_service_handle(void *arg)
{
    spi_service_mem_t *smem = (spi_service_mem_t *)arg;

    switch (smem->memType) {
        case SPI_SERVICE_TYPE_HTOS:
#ifdef SPI_SERVICE_LOOP_TEST
            return (void *)spi_service_packet_pm(smem);
#endif
#ifdef CONFIG_VNET_SERVICE
{
            /* vnet service send data to wifi */
            struct pbuf *p = (struct pbuf *)smem->memAddr;
            vnet_service_wifi_tx(p);
}
#endif
            break;

        case SPI_SERVICE_TYPE_STOH:
#ifdef SPI_SERVICE_LOOP_TEST
            return (void *)spi_service_packet_pm(smem);
#endif
#ifdef CONFIG_SPI_MASTER
#ifdef CONFIG_SPI_REPEATER
{
            spi_repeater_msg_t msg;
            msg.msgType = SPI_REPEATER_PACKETIN;
            msg.msgAddr = smem->memAddr;

            if (spi_repeater_send_msg(&msg) != 0) {
                os_printf(LM_OS, LL_ERR, "spi_repeater_send_msg failed\n");
            }
}
#endif
#endif
            break;

        case SPI_SERVICE_TYPE_OTA:
            ota_update_image((unsigned char *)smem->memAddr, smem->memLen);
            break;

        case SPI_SERVICE_TYPE_MSG: {
            int idx;
            unsigned char *pm = (unsigned char *)smem->memAddr;
            for (idx = 0; idx < smem->memLen; idx++) {
                os_printf(LM_OS, LL_ERR, "0x%02x ", pm[idx]);
                if ((idx + 1) % 16 == 0) {
                    os_printf(LM_OS, LL_ERR, "\n");
                }
            }
            os_printf(LM_OS, LL_ERR, "\n");
        }
        break;

        case SPI_SERVICE_TYPE_INFO:
#ifdef CONFIG_VNET_SERVICE
            vnet_reg_op_call(smem->memAddr);
#endif
#ifdef CONFIG_SPI_MASTER
#ifdef CONFIG_SPI_REPEATER
{
            spi_repeater_msg_t msg;
            msg.msgType = SPI_REPEATER_START_STA;
            spi_repeater_send_msg(&msg);
}
#endif
#endif
            break;
        default:
            os_printf(LM_OS, LL_ERR, "unkown type 0x%x\n", smem->memType);
            break;
    }

    return 0;
}

void *spi_service_rx_prepare(void *arg)
{
    spi_service_ctrl_t *scfg = (spi_service_ctrl_t *)arg;
    spi_service_mem_t *smem = NULL;

#ifdef CONFIG_SPI_MASTER
#ifdef CONFIG_SPI_REPEATER
    if (scfg->evt == SPI_SERVICE_TYPE_INT) {
        vnet_reg_get_peer_value();
        return NULL;
    }
#endif
#endif
    smem = spi_mqueue_get();
    if (smem != NULL) {
        if (spi_service_smem_rx_alloc(scfg, smem) == NULL) {
            spi_mqueue_put(smem);
            smem = NULL;
        }
    }

    return smem;
}

void *spi_service_rx_done(void *arg)
{
    spi_service_mem_t *smem = (spi_service_mem_t *)arg;

    spi_service_smem_rx_free(smem);
    spi_mqueue_put(smem);
    return 0;
}

void *spi_service_tx_prepare(void *arg)
{
    spi_service_mem_t *smem = NULL;
#ifdef CONFIG_VNET_SERVICE
    spi_service_ctrl_t *scfg = (spi_service_ctrl_t *)arg;
    unsigned int sintAddr = vnet_interrupt_get_addr();
    int sintNum = vnet_interrupt_get_num();
    smem = spi_mqueue_get();
    unsigned int stype = scfg->evt;
    if (smem && sintNum >= 0 && stype >= SPI_SERVICE_TYPE_HTOS) {
        smem->memAddr = sintAddr;
        smem->memOffset = 0;
        smem->memType = SPI_SERVICE_TYPE_INT;
        smem->memLen = sizeof(unsigned int);
        smem->memSlen = SPI_SERVICE_CONTROL_INT | SPI_SERVICE_CONTROL_LEN | sintNum;
        vnet_interrupt_clear(sintNum);
        return smem;
    }

    if (smem && scfg->evt == SPI_SERVICE_TYPE_INFO) {
        if (spi_service_smem_tx_alloc(scfg, smem) == NULL) {
            spi_mqueue_put(smem);
            smem = NULL;
        }
    } else {
        spi_mqueue_put(smem);
        smem = NULL;
    }
#endif
    return smem;
}

void *spi_service_tx_done(void *arg)
{
    spi_service_mem_t *smem = (spi_service_mem_t *)arg;

    spi_service_smem_tx_free(smem);
    spi_mqueue_put(smem);
    return 0;
}

int spi_service_send_msg(unsigned char *data, unsigned int len)
{
    spi_service_ctrl_t mcfg;
    spi_service_mem_t *smem = spi_mqueue_get();

    if (smem == NULL) {
        return -1;
    }

    if (len >= SPI_SERVICE_MSG_MAX_LEN || len == 0) {
        os_printf(LM_APP, LL_ERR, "%s[%d] msg(%d) len err\n", __FUNCTION__, __LINE__, len);
        return -1;
    }

    mcfg.evt = SPI_SERVICE_TYPE_MSG;
    mcfg.type = SPI_SLAVE_TYPE_WRITE;
    mcfg.len = len;
    if (spi_service_smem_tx_alloc(&mcfg, smem) == NULL) {
        spi_mqueue_put(smem);
        return -1;
    }

    memcpy((void *)smem->memAddr, data, len);
#ifdef CONFIG_SPI_MASTER
        spi_master_sendto_peer(smem);
#endif
#ifdef CONFIG_SPI_SLAVE
        smem->memSlen = SPI_SERVICE_CONTROL_MSG | SPI_SERVICE_CONTROL_LEN | CO_ALIGN2_HI(len);
        spi_slave_sendto_host(smem);
#endif

    return 0;
}

int spi_service_send_pbuf(struct pbuf *p_buf)
{
    spi_service_ctrl_t mcfg;
    spi_service_mem_t *smem = NULL;
    unsigned int offset = 0;

    if (p_buf == NULL || p_buf->tot_len >= SPI_SERVICE_CONTROL_LEN || p_buf->tot_len == 0) {
        os_printf(LM_APP, LL_ERR, "%s[%d] pbuf err\n", __FUNCTION__, __LINE__);
        return -1;
    }

    smem = spi_mqueue_get();
    if (smem == NULL) {
        return -1;
    }
#ifdef CONFIG_SPI_MASTER
    mcfg.evt = SPI_SERVICE_TYPE_HTOS;
#endif
#ifdef CONFIG_SPI_SLAVE
    mcfg.evt = SPI_SERVICE_TYPE_STOH;
#endif
    mcfg.type = SPI_SLAVE_TYPE_WRITE;
    mcfg.len = p_buf->tot_len;
    if (spi_service_smem_tx_alloc(&mcfg, smem) == NULL) {
        spi_mqueue_put(smem);
        return -1;
    }

    while (p_buf != NULL) {
        memcpy((void *)(smem->memAddr + smem->memOffset + offset), p_buf->payload, p_buf->len);
        offset += p_buf->len;
        p_buf = p_buf->next;
    }
#ifdef CONFIG_SPI_MASTER
    spi_master_sendto_peer(smem);
#endif
#ifdef CONFIG_SPI_SLAVE
    spi_slave_sendto_host(smem);
#endif

    return 0;
}

int spi_service_send_data(unsigned char *data, unsigned int len)
{
    spi_service_ctrl_t mcfg;
    spi_service_mem_t *smem = NULL;

    if (len >= SPI_SERVICE_CONTROL_LEN || len == 0) {
        os_printf(LM_APP, LL_ERR, "%s[%d] data len(%d) err\n", __FUNCTION__, __LINE__, len);
        return -1;
    }
    smem = spi_mqueue_get();
    if (smem == NULL) {
        return -1;
    }
#ifdef CONFIG_SPI_MASTER
    mcfg.evt = SPI_SERVICE_TYPE_HTOS;
#endif
#ifdef CONFIG_SPI_SLAVE
    mcfg.evt = SPI_SERVICE_TYPE_STOH;
#endif
    mcfg.type = SPI_SLAVE_TYPE_WRITE;
    mcfg.len = len;
    if (spi_service_smem_tx_alloc(&mcfg, smem) == NULL) {
        spi_mqueue_put(smem);
        return -1;
    }

    memcpy((void *)(smem->memAddr + smem->memOffset), data, len);
#ifdef CONFIG_SPI_MASTER
    spi_master_sendto_peer(smem);
#endif
#ifdef CONFIG_SPI_SLAVE
    spi_slave_sendto_host(smem);
#endif

    return 0;
}

void spi_service_main(void)
{
    spi_service_func_t funcset;

    spi_service_mem_init();
    spi_mqueue_init();

    funcset.rxPrepare = spi_service_rx_prepare;
    funcset.rxDone = spi_service_rx_done;
    funcset.txPrepare = spi_service_tx_prepare;
    funcset.txDone = spi_service_tx_done;
    funcset.dataHandle = spi_service_handle;
#ifdef CONFIG_SPI_MASTER
    spi_master_init();
    spi_master_funcset_register(funcset);
    tcpip_pbuf_vnet_free(spi_repeater_mem_free);
    spi_repeater_init();
#else
    spi_slave_init(0);
    spi_slave_funcset_register(funcset);
#endif

#ifdef CONFIG_VNET_SERVICE
    vnet_reg_t *vreg = vnet_reg_get_addr();
    memset(vreg, 0, sizeof(vnet_reg_t));
    typedef int (*rxl_vnet_packet_parse)(uint16_t type, unsigned char *data);
    void rxl_vnet_packet_call(rxl_vnet_packet_parse cb);
    tcpip_pbuf_vnet_handle(vnet_service_wifi_rx);
    tcpip_pbuf_vnet_free(vnet_service_wifi_rx_free);
    rxl_vnet_packet_call(vnet_filter_wifi_handle);
    vnet_set_default_filter(VNET_PACKET_DICTION_HOST);
    vnet_reg_get_info();
#endif
}


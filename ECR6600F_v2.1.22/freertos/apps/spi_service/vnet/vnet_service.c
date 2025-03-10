#include "vnet_service.h"
#include "vnet_filter.h"
#include "vnet_register.h"
#include "spi_service_main.h"
#include "spi_service_mem.h"
#include "spi_slave.h"
#include "system_wifi.h"
#include "lwip/etharp.h"

extern void fhost_tx_free(void *buf);
extern int fhost_tx_start(void *net_if, void *net_buf, uint8_t is_raw);
#define VNET_MIN_FRAME_SIZE 42

static struct pbuf *vnet_service_bcopy_send(struct pbuf *p)
{
    struct pbuf *pt = NULL;
    unsigned int bcopy = 0;

    if (p != NULL && p->tot_len >= VNET_MIN_FRAME_SIZE) {
        struct eth_hdr *ethhdr = p->payload;
        short type = lwip_ntohs(ethhdr->type);
        unsigned char *packet = (p->payload + sizeof(struct eth_hdr));

        if (type == ETHTYPE_ARP) {
            // only need to dispatch ARP Reply to Host_MCU
            struct etharp_hdr *arp_hdr = (struct etharp_hdr*)(p->payload + sizeof(struct eth_hdr));
            short arp_opcode = lwip_ntohs(arp_hdr->opcode);
            if (arp_opcode == ARP_REPLY) {
                bcopy = 1;
            }
        }

        if (type == ETHTYPE_IP && packet[9] == IPPROTO_ICMP) {
            bcopy = 1;
        }

        if (bcopy == 0 && vnet_ipv4_packet_blist_filter(packet) == VNET_PACKET_DICTION_BOTH) {
            bcopy = 1;
        }
    }

    if (bcopy) {
        pt = spi_service_mem_pbuf_alloc(SPI_SERVICE_MEM_TX, p->len);
        memcpy(pt->payload, p->payload, p->len);
        pt->len = p->len;
        pt->tot_len = p->len;
        return pt;
    }

    return NULL;
}

int vnet_service_wifi_rx(struct pbuf *p)
{
    spi_service_mem_t *smem = NULL;
    vnet_reg_t *vReg = vnet_reg_get_addr();
    int ret = -1;

    if (spi_service_mem_range_check(p) == NULL) {
        p = vnet_service_bcopy_send(p);
        if (p == NULL) {
            os_printf(LM_APP, LL_DBG, "%s[%d]\n", __FUNCTION__, __LINE__);
            return 0;
        }
        ret = 0;
    }

    if (vReg->status == VNET_WIFI_LINK_DOWN) {
        if ((p->flags & PBUF_FLAG_IS_CUSTOM) != 0) {
            struct pbuf_custom *pc = (struct pbuf_custom *)p;
            ASSERT_ERR(pc->custom_free_function != NULL);
            pc->custom_free_function(p);
        } else {
            pbuf_free(p);
        }
        return ret;
    }

    smem = spi_mqueue_get();
    if (smem == NULL) {
        if ((p->flags & PBUF_FLAG_IS_CUSTOM) != 0) {
            struct pbuf_custom *pc = (struct pbuf_custom *)p;
            ASSERT_ERR(pc->custom_free_function != NULL);
            pc->custom_free_function(p);
        } else {
            pbuf_free(p);
        }
        return ret;
    }
    memset(smem, 0, sizeof(spi_service_mem_t));
    smem->memAddr = (unsigned int)p;
    smem->memOffset = (unsigned int)p->payload - (unsigned int)p;
    smem->memSlen = smem->memLen = CO_ALIGN2_HI(p->len);
    smem->memType = SPI_SERVICE_TYPE_STOH;
    spi_slave_sendto_host(smem);

    return ret;
}

int vnet_service_wifi_tx(struct pbuf *p)
{
    struct netif *staif = NULL;
    wifi_work_mode_e wifiMode = wifi_get_opmode();
    staif = get_netif_by_index(wifiMode);

    if (staif == NULL) {
        os_printf(LM_APP, LL_DBG, "%s[%d]\n", __FUNCTION__, __LINE__);
        spi_service_mem_pool_free(p);
        return 0;
    }

    if (fhost_tx_start(staif, p, 0) != 0) {
        fhost_tx_free(p);
    }

    return 0;
}

int vnet_service_wifi_rx_free(struct pbuf *p)
{
    return spi_service_mem_pool_free(p) ? 0 : 1;
}

int vnet_service_wifi_rx_done(spi_service_mem_t *smem)
{
    struct pbuf *p = (struct pbuf *)smem->memAddr;

    if ((p->flags & PBUF_FLAG_IS_CUSTOM) != 0) {
        struct pbuf_custom *pc = (struct pbuf_custom *)p;
        ASSERT_ERR(pc->custom_free_function != NULL);
        pc->custom_free_function(p);
    } else {
        pbuf_free(p);
    }

    return 0;
}

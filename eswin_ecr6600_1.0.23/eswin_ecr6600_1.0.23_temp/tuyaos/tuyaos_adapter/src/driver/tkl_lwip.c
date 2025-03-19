#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include "lwip/tcpip.h"
#include "lwip/icmp.h"
#include "lwip/inet.h"
#include "netif/etharp.h"
#include "lwip/err.h"
#include <lwip/stats.h>
#include <lwip/snmp.h>
#include "lwip/init.h"
#include "netif/ppp/pppoe.h"
#include "ethernetif.h"

#include "CH390.h"
#include "cli.h"
#include "tkl_spi_ethernet.h"
#include "tkl_system.h"
#include "tkl_lwip.h"
#include "tkl_mutex.h"

spi_ethernet_txrx_stats_t ch390_txrx_stats;

extern void cli_printf(const char *f, ...);
extern err_t net_if_output(struct netif *net_if, struct pbuf *p_buf);

#define SPI_LAN_PKT_TRACE

#ifdef SPI_LAN_PKT_TRACE
#include "tal_log.h"
#define CH390_PKT_TRACE_BASIC 0x01
#define CH390_PKT_TRACE_DETIL 0x02
unsigned char drv_pkt_trace_en = 0;
extern void tuya_ethernetif_pkt_decode(unsigned char direction,  TKL_NETIF_HANDLE netif, TKL_PBUF_HANDLE pbuf);
#endif

#ifdef SPI_LAN_LOCK_SUPPORT
extern TKL_MUTEX_HANDLE spi_ethernet_lock;
#endif /* SPI_LAN_LOCK_SUPPORT */
extern unsigned char is_need_reset_tx_fifo;
extern unsigned int reset_tx_fifo_cnt;
extern unsigned char last_interrupt_status;

/**
 * @brief ethernet interface hardware init
 *
 * @param[in]      netif     the netif to which to send the packet
 * @return  err_t  SEE "err_enum_t" in "lwip/err.h" to see the lwip err(ERR_OK: SUCCESS other:fail)
 */
OPERATE_RET tkl_ethernetif_init(TKL_NETIF_HANDLE netif)
{
    return OPRT_OK;
}

/**
 * @brief ethernet interface sendout the pbuf packet
 *
 * @param[in]      netif     the netif to which to send the packet
 * @param[in]      p         the packet to be send, in pbuf mode
 * @return  err_t  SEE "err_enum_t" in "lwip/err.h" to see the lwip err(ERR_OK: SUCCESS other:fail)
 */
OPERATE_RET tkl_ethernetif_output(TKL_NETIF_HANDLE netif, TKL_PBUF_HANDLE p)
{
    struct pbuf *p_buf = (struct pbuf *)p;
    struct netif *p_netif = (struct netif *)netif;
#ifdef SPI_LAN_PKT_TRACE
    if (drv_pkt_trace_en & CH390_PKT_TRACE_BASIC) {
        tuya_ethernetif_pkt_decode(1, p_netif, p_buf);
    }
#endif
    return net_if_output(p_netif, p_buf);
}

/**
 * @brief ethernet interface recv the packet
 *
 * @param[in]      netif       the netif to which to recieve the packet
 * @param[in]      total_len   the length of the packet recieved from the netif
 * @return  void
 */
OPERATE_RET tkl_ethernetif_recv(TKL_NETIF_HANDLE netif, TKL_PBUF_HANDLE p)
{
    struct pbuf *p_buf = (struct pbuf *)p;
    struct netif *p_netif = (struct netif *)netif;
#ifdef SPI_LAN_PKT_TRACE
    if (drv_pkt_trace_en & CH390_PKT_TRACE_BASIC) {
        tuya_ethernetif_pkt_decode(0, p_netif, p_buf);
    }
#endif
	if (p_netif->input(p_buf, p_netif) != OPRT_OK) {
        return OPRT_COM_ERROR;
    }
    
	return OPRT_OK;
}

/**
 * In this function, the hardware should be initialized.
 * Called from ethernetif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
static void low_level_init(struct netif *netif)
{
    //SPI_LAN_DBG("%s:------>", __func__);
    //struct ethernetif *ethernetif = netif->state;
    ch390_default_config();

    /* set MAC hardware address length */
    netif->hwaddr_len = ETHARP_HWADDR_LEN;

    /* set MAC hardware address */
    ch390_get_mac(netif->hwaddr);

    /* maximum transfer unit */
    //netif->mtu = 1500;
    netif->mtu = LWIP_SPI_ETHERNET_MAX_MTU;

    /* device capabilities */
    /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

    /* Do whatever else is needed to initialize interface. */
    //SPI_LAN_DBG("%s:<------", __func__);
}

/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become availale since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */
static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
//  struct ethernetif *ethernetif = netif->state;
    uint8_t is_tx_blocked;
    struct pbuf *q;
    uint16_t tx_p = 0, tx_n = 0, tx_offset = 0;
    //SYS_TICK_T start_time, end_time;

    //SPI_LAN_DBG("%s:------>", __func__);
    SPI_ETHERNET_LOCK_X();
    #if ETH_PAD_SIZE
    pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
    #endif

    for(q = p; q != NULL; q = q->next) {
        /* Send the data from the pbuf to the interface, one pbuf at a
           time. The size of the data in each pbuf is kept in the ->len
           variable. */
#ifdef SPI_LAN_PKT_TRACE /* packet trace here */
    if (drv_pkt_trace_en & CH390_PKT_TRACE_BASIC) {
        tuya_ethernetif_pkt_decode(1, netif, q);
    }
    if (drv_pkt_trace_en & CH390_PKT_TRACE_DETIL) {
        TAL_PR_HEXDUMP_NOTICE("send eth packet", q->payload, q->len);
    }
#endif
        if (is_need_reset_tx_fifo) {
            SPI_LAN_ERR("%s: need reset ch390 tx fifo before send packet", __func__);
            ch390_write_reg(CH390_MPTRCR, 0x02);  //Reset TX FIFO pointer
            is_need_reset_tx_fifo = 0;
            reset_tx_fifo_cnt++;
        }

        //start_time = tkl_system_get_millisecond();
        do {
            tx_p = ch390_MWR();
 #if 0
        SPI_LAN_DBG("%s:--> TRPAL=0x%x,TRPAH=0x%x,MWRL=0x%x,MWRH=0x%x",
            __func__, ch390_read_reg(CH390_TRPAL),  ch390_read_reg(CH390_TRPAH), 
            ch390_read_reg(CH390_MWRL),  ch390_read_reg(CH390_MWRH));
 #endif           
            ch390_write_mem(q->payload, q->len);
#if 0
        SPI_LAN_DBG("%s: <-- TRPAL=0x%x,TRPAH=0x%x,MWRL=0x%x,MWRH=0x%x",
            __func__, ch390_read_reg(CH390_TRPAL),  ch390_read_reg(CH390_TRPAH), 
            ch390_read_reg(CH390_MWRL),  ch390_read_reg(CH390_MWRH));
#endif
            tx_n = ch390_MWR();
            if (tx_n > tx_p) {
                tx_offset = tx_n - tx_p;
                //if (tx_offset != q->len) {
                //    ch390_write_reg(CH390_MPTRCR, 0x02);  //Reset TX FIFO pointer
                //    SPI_LAN_ERR("%s: send packet failed, "
                //    "reset TX FIFO pointer(tx_n=0x%x,tx_p=0x%x,q_len=%d,tx_offset=%d)\r\n",
                //        __func__, tx_n, tx_p, q->len, tx_offset);
                //} else {
 #ifdef SPI_LAN_DBG_TRACE                   
                    //SPI_LAN_DBG("%s: send packet success(tx_offset=%d,q->len=%d)", __func__, tx_offset, q->len);
 #endif                   
                //}
            } else {
                tx_offset = 0xc00 - tx_p + tx_n;
            }
            //end_time = tkl_system_get_millisecond();
            if (tx_offset != q->len) {
                ch390_write_reg(CH390_MPTRCR, 0x02);  //Reset TX FIFO pointer
                SPI_LAN_ERR("%s: send packet failed, ch390 TX invalid", __func__);
                ch390_txrx_stats.tx_fail++;
            }
        //} while ((tx_offset != q->len) && (end_time - start_time <= 1000));
        } while (tx_offset != q->len);


        //if (tx_offset != q->len) {
        //    ch390_write_reg(CH390_MPTRCR, 0x02);  //Reset TX FIFO pointer
        //    SPI_LAN_ERR("%s: send packet failed, ch390 TX invalid", __func__);
        //    ch390_txrx_stats.tx_fail++;
        //    return ERR_IF;
        //}
    }

    #if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
    #endif

//#ifdef SPI_LAN_DBG_TRACE 
    //SPI_LAN_DBG("%s: wait TCR_TXREQ start CH390_NSR=0x%x,TRPAL=0x%x,TRPAH=0x%x,MWRL=0x%x,MWRH=0x%x",
    //    __func__, ch390_read_reg(CH390_NSR), ch390_read_reg(CH390_TRPAL), ch390_read_reg(CH390_TRPAH),
    //    ch390_read_reg(CH390_MWRL), ch390_read_reg(CH390_MWRH));
//#endif    
    // Wait until last transmit complete
    while(ch390_read_reg(CH390_TCR) & TCR_TXREQ);
 #if 0   
    start_time = tkl_system_get_millisecond();
    do {
        is_tx_blocked = ch390_read_reg(CH390_TCR) & TCR_TXREQ;
        end_time = tkl_system_get_millisecond();
    } while (is_tx_blocked && (end_time - start_time <= 1000));

    if (is_tx_blocked) {
        SPI_LAN_ERR("%s: send packet failed, "
            "because of last transmit has not completed", __func__);
        ch390_txrx_stats.tx_fail++;    
        return ERR_IF;
    }
#endif    
//#ifdef SPI_LAN_DBG_TRACE    
    //SPI_LAN_DBG("%s: wait TCR_TXREQ end", __func__);
//#endif

    // Set current packet length
    ch390_write_reg(CH390_TXPLL, p->tot_len & 0xff);
    ch390_write_reg(CH390_TXPLH, (p->tot_len >> 8) & 0xff);
    // Issue transmit request
    ch390_send_request();
    LINK_STATS_INC(link.xmit);
    ch390_txrx_stats.tx_ok++;

    //SPI_LAN_DBG("%s:<------", __func__);
    SPI_ETHERNET_UNLOCK_X();
    return ERR_OK;
}

/**
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
 */
static struct pbuf *low_level_input(struct netif *netif)
{
    struct ethernetif *ethernetif = netif->state;
    struct pbuf *p, *q;
    u16_t len;

    u8_t rx_ready;
    u8_t ReceiveData[4];

     SPI_ETHERNET_LOCK_X();
    //SPI_LAN_DBG("%s:------>", __func__);

    ch390_read_reg(CH390_MRCMDX);
    rx_ready = ch390_read_reg(CH390_MRCMDX);

    if (rx_ready & CH390_PKT_ERR)
    {
        // Reset RX FIFO pointer
        ch390_write_reg(CH390_RCR, 0);        //RX disable
        ch390_write_reg(CH390_MPTRCR, 0x01);  //Reset RX FIFO pointer
        ch390_write_reg(CH390_MRRH, 0x0c);
        os_usdelay(1000);
        ch390_write_reg(CH390_RCR, RCR_RXEN); //RX Enable
        LINK_STATS_INC(link.drop);
        ethernetif->rx_len = 0;
        ch390_txrx_stats.rx_fail++;
        SPI_LAN_ERR("%s: receive packet err(rx_ready=0x%x)", __func__, rx_ready);
        SPI_ETHERNET_UNLOCK_X();
        return NULL;
    }
    if (!(rx_ready & CH390_PKT_RDY))
    {
        ethernetif->rx_len = 0;
        //if (last_interrupt_status & 0x04) {
        //    ch390_write_reg(CH390_RCR, 0);        //RX disable
        //    ch390_write_reg(CH390_MPTRCR, 0x01);  //Reset RX FIFO pointer
        //    ch390_write_reg(CH390_MRRH, 0x0c);
        //    os_usdelay(1000);
        //    ch390_write_reg(CH390_RCR, RCR_RXEN); //RX Enable
        //    SPI_LAN_DBG("%s: =====receive packet RX not ready(rx_ready=0x%x)====", __func__, rx_ready);
        //}

        //ch390_txrx_stats.rx_fail++;
//#ifdef SPI_LAN_DBG_TRACE       
        //SPI_LAN_DBG("%s: receive packet RX not ready(rx_ready=0x%x)", __func__, rx_ready);
//#endif
        SPI_ETHERNET_UNLOCK_X();
        return NULL;
    }

    ch390_read_mem(ReceiveData, 4);
    ethernetif->rx_status = ReceiveData[1];
    ethernetif->rx_len = ReceiveData[2] | (ReceiveData[3] << 8);

    /* Obtain the size of the packet and put it into the "len"
     variable. */
    len = ethernetif->rx_len;

    if ((ethernetif->rx_status & 0x3f) || (len > CH390_PKT_MAX)) {
        SPI_LAN_ERR("%s: invaild input packet(status=0x%x,len=%d)",
            __func__, ethernetif->rx_status, len);
        ethernetif->rx_len = 0;
        ch390_txrx_stats.rx_fail++;
        SPI_ETHERNET_UNLOCK_X();
        return NULL;
    }

    #if ETH_PAD_SIZE
    len += ETH_PAD_SIZE; /* allow room for Ethernet padding */
    #endif

    /* We allocate a pbuf chain of pbufs from the pool. */
    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);

    if (p != NULL) {

        #if ETH_PAD_SIZE
        pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
        #endif

        /* We iterate over the pbuf chain until we have read the entire
         * packet into the pbuf. */
    for(q = p; q != NULL; q = q->next) {
        /* Read enough bytes to fill this pbuf in the chain. The
        * available data in the pbuf is given by the q->len
        * variable.
        * This does not necessarily have to be a memcpy, you can also preallocate
        * pbufs for a DMA-enabled MAC and after receiving truncate it to the
        * actually received size. In this case, ensure the tot_len member of the
        * pbuf is the sum of the chained pbuf len members.
        */
        ch390_read_mem(q->payload, q->len);
#ifdef SPI_LAN_PKT_TRACE /* packet trace */
        if (drv_pkt_trace_en & CH390_PKT_TRACE_BASIC) {
            tuya_ethernetif_pkt_decode(0, netif, q);
        }
        if (drv_pkt_trace_en & CH390_PKT_TRACE_DETIL) {
            TAL_PR_HEXDUMP_NOTICE("rcv eth packet", q->payload, q->len);
        }
#endif
    }
//    acknowledge that packet has been read();

    #if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
    #endif

    ch390_txrx_stats.rx_ok++;
    LINK_STATS_INC(link.recv);
    } else {
        ch390_drop_packet(len);
        SPI_LAN_LOG("%s: failed to alloc pbuf drop the "
                "packet(rx_ready=0x%x,status=0x%x,len=%d)",
                __func__, rx_ready, ethernetif->rx_status, ethernetif->rx_len);
        LINK_STATS_INC(link.memerr);
        LINK_STATS_INC(link.drop);
        ch390_txrx_stats.rx_drop++;
    }

    //SPI_LAN_DBG("%s:<------", __func__);
    SPI_ETHERNET_UNLOCK_X();
    return p;
}

/**
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif the lwip network interface structure for this ethernetif
 */
void ethernetif_input(struct netif *netif)
{
//    struct ethernetif *ethernetif;
    struct eth_hdr *ethhdr;
    struct pbuf *p;

//    ethernetif = netif->state;
    //SPI_LAN_DBG("%s:------>", __func__);

    /* move received packet into a new pbuf */
    p = low_level_input(netif);
    /* no packet could be read, silently ignore this */
    if (p == NULL) {
        return;
    }
    /* points to packet payload, which starts with an Ethernet header */
    ethhdr = p->payload;

    switch (htons(ethhdr->type)) {
    /* IP or ARP packet? */
    case ETHTYPE_IP:
    case ETHTYPE_ARP:
#if PPPOE_SUPPORT
    /* PPPoE packet? */
    case ETHTYPE_PPPOEDISC:
    case ETHTYPE_PPPOE:
#endif /* PPPOE_SUPPORT */
        /* full packet send to tcpip_thread to process */
        if (netif->input(p, netif)!=ERR_OK)
        {
            LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
            SPI_LAN_ERR("%s: IP input error", __func__);
            pbuf_free(p);
            p = NULL;
        }
        break;

    default:
        pbuf_free(p);
        p = NULL;
        break;
  }
  //SPI_LAN_DBG("%s:<------", __func__);
}

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t
ethernetif_init(struct netif *netif)
{
    struct ethernetif *ethernetif;

    LWIP_ASSERT("netif != NULL", (netif != NULL));
    //SPI_LAN_DBG("%s:------>", __func__);

    ethernetif = (struct ethernetif *)mem_malloc(sizeof(struct ethernetif));
    if (ethernetif == NULL) {
        LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_init: out of memory\n"));
        SPI_LAN_ERR("%s: memory malloc failed", __func__);
        return ERR_MEM;
    }

    memset(ethernetif, 0, sizeof(struct ethernetif));
    memset(&ch390_txrx_stats, 0, sizeof(ch390_txrx_stats));

#if LWIP_NETIF_HOSTNAME
    /* Initialize interface hostname */
    netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

    /*
     * Initialize the snmp variables and counters inside the struct netif.
     * The last argument should be replaced with your link speed, in units
     * of bits per second.
     */
    NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, 100);

    netif->state = ethernetif;
    netif->name[0] = SPI_ETHERNET_IFNAME0;
    netif->name[1] = SPI_ETHERNET_IFNAME1;

#if LWIP_NETIF_HOSTNAME
    netif->hostname = "tuyasmart-en";
#endif /* LWIP_NETIF_HOSTNAME */
    /* We directly use etharp_output() here to save a function call.
     * You can instead declare your own function an call etharp_output()
     * from it if you have to do some checks before sending (e.g. if link
     * is available...) */
    netif->output = etharp_output;
    netif->linkoutput = low_level_output;

//  ethernetif->ethaddr = (struct eth_addr *)&(netif->hwaddr[0]);

    /* initialize the hardware */
    low_level_init(netif);

    //SPI_LAN_DBG("%s:<------", __func__);
    return ERR_OK;
}

 /*============================================================================
 *                                                                            *
 * Copyright (C) by Tuya Inc                                                  *
 * All rights reserved                                                        *
 *                                                                            *
 =============================================================================*/


/*============================ INCLUDES ======================================*/
#include "tuya_cloud_types.h"
#include "tuya_error_code.h"
#include "hal_gpio.h"
#include "chip_pinmux.h"
#include "spi.h"
#include "cli.h"
#include "tkl_gpio.h"
#include "oshal.h"
#include "ethernetif.h"
#include "lwip/netifapi.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "lwip_dhcpc.h"

#include "tkl_wired.h"
#include "tkl_thread.h"
#include "tkl_mutex.h"
#include "tkl_system.h"

#include "CH390.h"
#include "tkl_wired.h"
#include "tkl_spi_ethernet.h"

#define CH390_TX_ERR_CHK_EN

#ifdef SPI_LAN_DEMO_BORAD_EN
#define INT_PIN  TUYA_GPIO_NUM_21
#else
#define INT_PIN  TUYA_GPIO_NUM_4
#endif
#define RST_PIN  TUYA_GPIO_NUM_16
#define CS_PIN   TUYA_GPIO_NUM_1
#define SCK_PIN  TUYA_GPIO_NUM_0
#define MISO_PIN TUYA_GPIO_NUM_3
#define MOSI_PIN TUYA_GPIO_NUM_2
#define WOL_PIN  TUYA_GPIO_NUM_17

typedef enum {
    TY_SPI_LAN_MSG_TYPE_LINKCHG_UP,             /* Link status change UP */
    TY_SPI_LAN_MSG_TYPE_LINKCHG_DOWN,           /* Link status change DOWN */
    TY_SPI_LAN_MSG_TYPE_RCV_CNT_OVERFLOW,       /* Receive overflow counter overflow */
    TY_SPI_LAN_MSG_TYPE_RCV_OVERFLOW,           /* Receive overflow */
    TY_SPI_LAN_MSG_TYPE_PKT_TRANSMITTED,        /* Packet transmitted */
    TY_SPI_LAN_MSG_TYPE_PKT_RECEIVED,           /* Packet received */
    TY_SPI_LAN_MSG_TYPE_PKT_UNKNOW,             /* Unknow */
} spi_lan_msg_type_t;

typedef struct {
    spi_lan_msg_type_t   type;                  /* ISR message type */
    uint32_t             data;                  /* Poniter addr of message data */
} spi_lan_msg_t;

unsigned char spi_ethernet_initiated = 0;
QueueHandle_t spi_ethernet_queue = NULL;
#ifdef SPI_LAN_LOCK_SUPPORT
TKL_MUTEX_HANDLE spi_ethernet_lock = NULL;
#endif /* SPI_LAN_LOCK_SUPPORT */
TKL_THREAD_HANDLE spi_ethernet_thread_handle = NULL;
static unsigned int interrupt_cnt = 0;
unsigned char last_interrupt_status = 0;
unsigned char ch390_link_status = 0; 
static unsigned char recv_overflow_reset = 0;
unsigned char ch390_reg_dbg_en = 0;

#ifdef CH390_TX_ERR_CHK_EN
static unsigned short trpa_value = 0;
static unsigned short mwr_value = 0;
unsigned int reset_tx_fifo_cnt = 0;
static unsigned char reset_tx_chk = 0;
unsigned char is_need_reset_tx_fifo = 0;
#endif /* CH390_TX_ERR_CHK_EN */

extern struct netif *netif_default;
extern spi_ethernet_txrx_stats_t ch390_txrx_stats;
extern TKL_WIRED_STATUS_CHANGE_CB ch390_netif_link_chg_cb;

extern void cli_printf(const char *f, ...);
extern void  ethernetif_input(struct netif *netif);
extern void pin_func_set_gpio(int gpio_num);

static void lan_chip_rst(uint8_t level)
{
#ifdef SPI_LAN_HARDWARE_RESET_EN    
    (void)tkl_gpio_write(RST_PIN, level);
    SPI_LAN_DBG("%s: level %d\r\n", __func__, level);
#endif    
}

static void spi_lan_scs(uint8_t level)
{
    //(void)tkl_gpio_write(CS_PIN, level);
    //SPI_LAN_DBG("%s: level %d\r\n", __func__, level);
}

static uint8_t spi_exchange_byte(uint8_t byte)
{
    int ret;
    uint8_t read_buf[32] = { 0 };
    uint8_t write_buf[32] = { 0 };

    write_buf[0] = byte;
    ret = spi_master_write_read(write_buf, read_buf, 1, 1);
    if (ret < 0) {
        SPI_LAN_DBG("%s: call spi_master_write_read failed(ret=%d)\r\n", __func__, ret);
    }
    
    return read_buf[0];
}

static void spi_lan_delay_us(uint32_t time)
{
    (void)os_usdelay(time);
}

ch390_interface_t ch390_interface = {
    lan_chip_rst,
    spi_lan_delay_us,
    spi_lan_scs,
    spi_exchange_byte,
};

#ifdef SPI_LAN_DBG_EN
static void ch390_print_info()
{
    uint8_t ver;
    uint16_t vid, pid;
    uint8_t mac_addr[6] = { 0 };

    ver = ch390_get_revision();
    vid = ch390_get_vendor_id();
    pid = ch390_get_product_id();
    ch390_get_mac(mac_addr);
    SPI_LAN_DBG("%s: thernet adapter %04x%04x mac %02x:%02x:%02x:%02x:%02x:%02x ver %d\r\n",
        __func__, vid, pid, mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4],
        mac_addr[5], ver);
}
#endif /* SPI_LAN_DBG_EN */

static void ch390_interrupt_handler(void *args)
{
    int ret;
    uint8_t int_status;
    uint8_t link_status;

    spi_lan_msg_t message;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
#ifdef CH390_TX_ERR_CHK_EN
    uint8_t trpal, trpah, mwrl, mwrh;
#endif
    uint32_t irq_status = taskENTER_CRITICAL_FROM_ISR();

    memset(&message, 0, sizeof(message));
    message.type = TY_SPI_LAN_MSG_TYPE_PKT_UNKNOW;

    int_status = ch390_get_int_status();
    //SPI_LAN_DBG("%s: int_status 0x%x\r\n", __func__, int_status);

    if (int_status & ISR_LNKCHG) {
        os_msdelay(100);
        ch390_write_phy(CH390_GPR, 0);
        while(ch390_read_phy(CH390_GPCR));
        link_status = ch390_get_link_status();
        if (link_status) {
            message.type = TY_SPI_LAN_MSG_TYPE_LINKCHG_UP;
            ch390_write_reg(CH390_ISR, ISR_LNKCHG);
        } else {
            message.type = TY_SPI_LAN_MSG_TYPE_LINKCHG_DOWN;
        }
    }

    if (int_status & ISR_ROS) {
        message.type = TY_SPI_LAN_MSG_TYPE_RCV_OVERFLOW;
#if 1
        if (!recv_overflow_reset) {       
            // Reset RX FIFO pointer
            ch390_write_reg(CH390_RCR, 0);        //RX disable
            ch390_write_reg(CH390_MPTRCR, 0x01);  //Reset RX FIFO pointer
            ch390_write_reg(CH390_MRRH, 0x0c);
            os_msdelay(1);
            ch390_write_reg(CH390_RCR, RCR_RXEN); //RX Enable
            SPI_LAN_DBG("%s: =======recv overflow=========", __func__);
            recv_overflow_reset = 1;
        }
#endif        
    } else {
        recv_overflow_reset = 0;
    }

    if (int_status & ISR_ROO) {
        message.type = TY_SPI_LAN_MSG_TYPE_RCV_CNT_OVERFLOW;
    }

    if (int_status & ISR_PR) {
        message.type = TY_SPI_LAN_MSG_TYPE_PKT_RECEIVED;
    }

    if (int_status & ISR_PT) {
        message.type = TY_SPI_LAN_MSG_TYPE_PKT_TRANSMITTED;
//#ifdef SPI_LAN_DBG_TRACE
        //SPI_LAN_DBG("%s: TRPAL=0x%x,TRPAH=0x%x,MWRL=0x%x,MWRH=0x%x",
        //    __func__, ch390_read_reg(CH390_TRPAL),  ch390_read_reg(CH390_TRPAH), 
        //    ch390_read_reg(CH390_MWRL),  ch390_read_reg(CH390_MWRH));
//#endif

#ifdef CH390_TX_ERR_CHK_EN
        trpal = ch390_read_reg(CH390_TRPAL);
        trpah = ch390_read_reg(CH390_TRPAH);
        mwrl = ch390_read_reg(CH390_MWRL);
        mwrh = ch390_read_reg(CH390_MWRH);
        
        trpa_value = trpal | (trpah << 8);
        mwr_value = mwrl | (mwrh << 8);
 
        if ((trpa_value == mwr_value) || (trpa_value <= (mwr_value + 3))) {
            reset_tx_chk = 0;
        } else {
            reset_tx_chk++; 
        }

       //SPI_LAN_DBG("%s: TRPAL=0x%x,TRPAH=0x%x,MWRL=0x%x,MWRH=0x%x,TRPA=0x%x,MWR=0x%x,reset_tx_chk=%d",
        //    __func__, trpal, trpah, mwrl, mwrh, trpa_value, mwr_value, reset_tx_chk);

        if (reset_tx_chk >= 1) {
            SPI_LAN_DBG("%s: Tx blocked of CH390, reset TX FIFO pointer(TRPAL=0x%x,TRPAH=0x%x,MWRL=0x%x,MWRH=0x%x,TRPA=0x%x,MWR=0x%x)",
                    __func__, trpal, trpah, mwrl, mwrh, trpa_value, mwr_value);
            reset_tx_chk = 0;
            is_need_reset_tx_fifo = 1;
        }
#endif /* CH390_TX_ERR_CHK_EN */
    }

    if (   (TY_SPI_LAN_MSG_TYPE_LINKCHG_UP == message.type)
        || (TY_SPI_LAN_MSG_TYPE_LINKCHG_DOWN == message.type)
        || (TY_SPI_LAN_MSG_TYPE_PKT_RECEIVED == message.type) ) {
        ret = xQueueSendFromISR(spi_ethernet_queue, &message, &xHigherPriorityTaskWoken);
        if (pdTRUE == xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
        if (pdFAIL == ret) {
            SPI_LAN_DBG("%s: call xQueueSendFromISR failed(ret=%d,int_status=0x%x)\r\n", __func__, ret, int_status);
        }
    }

#if 1
    if (   (TY_SPI_LAN_MSG_TYPE_PKT_TRANSMITTED != message.type)
       &&  (TY_SPI_LAN_MSG_TYPE_LINKCHG_UP != message.type)
       &&  (TY_SPI_LAN_MSG_TYPE_LINKCHG_DOWN != message.type)
       &&  (TY_SPI_LAN_MSG_TYPE_PKT_RECEIVED != message.type) ) {
            SPI_LAN_DBG("%s: recv message.type = %d", __func__, message.type);
       }
#endif

    interrupt_cnt++;
    last_interrupt_status = int_status;
    taskEXIT_CRITICAL_FROM_ISR(irq_status);
}

static void spi_ethernet_sys_deinit()
{
    if (spi_ethernet_queue) {
        vQueueDelete(spi_ethernet_queue);
        spi_ethernet_queue = NULL;
    }
 #ifdef SPI_LAN_LOCK_SUPPORT
    if (spi_ethernet_lock) {
        tkl_mutex_release(spi_ethernet_lock);
        spi_ethernet_lock = NULL;
    }
 #endif
    if (spi_ethernet_thread_handle) {
        tkl_thread_release(spi_ethernet_thread_handle);
        spi_ethernet_thread_handle = NULL;
    }

    if (spi_ethernet_initiated) {
        drv_spi_master_close();
    }

    spi_ethernet_initiated = 0;
}

static void spi_ethernet_thread(void *args)
{
    struct netif *ch390_netif;
    struct ethernetif *ether;
    spi_lan_msg_t message;

    ch390_netif = tuya_ethernetif_get_netif_by_index(NETIF_ETH_IDX);
    if (NULL == ch390_netif) {
        SPI_LAN_ERR("%s: start spi ethernet failed,"
                        "because of null netif", __func__);
        return;
    }
    ether = ch390_netif->state;

    while(xQueueReceive(spi_ethernet_queue, &message, portMAX_DELAY)) {
        //SPI_LAN_DBG("%s: receive message type %d available space %d\r\n", __func__, message.type, uxQueueSpacesAvailable(spi_ethernet_queue));
        switch(message.type) {
        case TY_SPI_LAN_MSG_TYPE_LINKCHG_UP:
            netifapi_netif_set_default(ch390_netif);
            netifapi_netif_set_link_up(ch390_netif);
 #ifdef CH390_TX_ERR_CHK_EN     
            //ch390_write_reg(CH390_MPTRCR, 0x02);  //Reset TX FIFO pointer
            // Reset RX FIFO pointer
            //ch390_write_reg(CH390_RCR, 0);        //RX disable
            //ch390_write_reg(CH390_MPTRCR, 0x01);  //Reset RX FIFO pointer
            //ch390_write_reg(CH390_MRRH, 0x0c);
            //os_msdelay(1);
            //ch390_write_reg(CH390_RCR, RCR_RXEN); //RX Enable
 #endif           
            netifapi_dhcp_start(ch390_netif);
            if (ch390_netif_link_chg_cb) {
                ch390_netif_link_chg_cb(TKL_WIRED_LINK_UP);
            }
            ch390_link_status = 1;
#if 0
            #include "tkl_wifi.h"
            extern WIFI_EVENT_CB wpas_conn_evt_cb;
            if (wpas_conn_evt_cb) {
                wpas_conn_evt_cb(WFE_CONNECTED, NULL);
            }
#endif
            break;
        case TY_SPI_LAN_MSG_TYPE_LINKCHG_DOWN:
            netifapi_dhcp_stop(ch390_netif);
            netifapi_netif_set_link_down(ch390_netif);
            if (ch390_netif_link_chg_cb) {
                ch390_netif_link_chg_cb(TKL_WIRED_LINK_DOWN);
            }
            ch390_link_status = 0;
            break;
        case TY_SPI_LAN_MSG_TYPE_RCV_CNT_OVERFLOW:
            break;
        case TY_SPI_LAN_MSG_TYPE_PKT_TRANSMITTED:
            break;
        case TY_SPI_LAN_MSG_TYPE_RCV_OVERFLOW:
            break;
        case TY_SPI_LAN_MSG_TYPE_PKT_RECEIVED:
            //SPI_LAN_DBG("%s: is up %d, is link up %d, is default netif %d",
            //    __func__, netif_is_up(ch390_netif), netif_is_link_up(ch390_netif),
            //    (netif_default == ch390_netif) ? 1 : 0);
            do{
                ethernetif_input(ch390_netif);
            }while(ether->rx_len != 0);
            break;
        case TY_SPI_LAN_MSG_TYPE_PKT_UNKNOW:
            break;
        default:
            SPI_LAN_ERR("%s: invaild message type %d", __func__, message.type);
            break;                             
        } /* end switch(message.type) { */
    } /* end while(xQueueReceive(spi_ethernet_queue, &message, portMAX_DELAY)) { */

    spi_ethernet_sys_deinit();
}

static int spi_ethernet_sys_init(void)
{
    int ret = -1;

    ch390_interface_register(ch390_interface);

    spi_ethernet_queue = xQueueCreate(SPI_ETHERNET_QUEUE_NUM, sizeof(spi_lan_msg_t));
    if (NULL == spi_ethernet_queue) {
        SPI_LAN_ERR("%s: call xQueueCreate create spi ethernet queue failed", __func__);
        goto err_out;
    }

 #ifdef SPI_LAN_LOCK_SUPPORT 
    ret = tkl_mutex_create_init(&spi_ethernet_lock);
    if (ret < 0) {
        SPI_LAN_ERR("%s: call tkl_mutex_create_init  failed", __func__);
        goto err_out;
    }
  
    if (NULL == spi_ethernet_lock) {
        SPI_LAN_ERR("%s: create spi ehternet lockfailed", __func__);
        goto err_out;
    }
#endif /* SPI_LAN_LOCK_SUPPORT */
    
    ret = tkl_thread_create(&spi_ethernet_thread_handle, CH390_THRAD_NAME,\
        SPI_ETHERNET_THREAD_SIZE, SPI_ETHERNET_THREAD_PRIORITY, spi_ethernet_thread, NULL);
    if (ret < 0) {
        SPI_LAN_ERR("%s: call tkl_thread_create failed(ret=%d)\r\n", __func__, ret);
        goto err_out;
    }

    if (NULL == spi_ethernet_thread_handle) {
        SPI_LAN_ERR("%s: create spi ethernet thead failed", __func__);
        ret = -1;
        goto err_out;
    }

    return ret;

err_out:
    if (spi_ethernet_queue) {
        vQueueDelete(spi_ethernet_queue);
    }
 #ifdef SPI_LAN_LOCK_SUPPORT
    if (spi_ethernet_lock) {
        tkl_mutex_release(spi_ethernet_lock);
    }
 #endif
    if (spi_ethernet_thread_handle) {
        tkl_thread_release(spi_ethernet_thread_handle);
    }
 
    return ret;
}

static int spi_ethernet_interrupt_init(UINT32_T gpio_num, CONST TUYA_GPIO_IRQ_T *cfg)
{
    int trigger = 0;
    int ret;

    pin_func_set_gpio(gpio_num);
    ret = hal_gpio_dir_set(gpio_num, GPIO_INPUT);
    if (ret != GPIO_RET_SUCCESS) {
        SPI_LAN_ERR("%s: call hal_gpio_dir_set init gpio %d "
                "failed(gpio_num=%d,ret=%d)", __func__, gpio_num, ret);
        return -1;
    }

    switch (cfg->mode) {
    case TUYA_GPIO_IRQ_RISE:
        trigger = DRV_GPIO_ARG_INTR_MODE_P_EDGE; 
        break;
    case TUYA_GPIO_IRQ_FALL:
        trigger = DRV_GPIO_ARG_INTR_MODE_N_EDGE; 
        break;
    case TUYA_GPIO_IRQ_LOW:
        trigger = DRV_GPIO_ARG_INTR_MODE_LOW; 
        break;
    case TUYA_GPIO_IRQ_HIGH:
        trigger = DRV_GPIO_ARG_INTR_MODE_HIGH; 
        break;
    default:
        return -1;
    }

    ret = hal_gpio_callback_register(gpio_num,cfg->cb, cfg->arg);
    if (ret != GPIO_RET_SUCCESS) {
        SPI_LAN_ERR("%s: call hal_gpio_callback_register set callback for gpio "
                "failed(gpio_num=%d,ret=%d)", __func__, gpio_num, ret);
        return -1;
    }

    ret = hal_gpio_intr_mode_set(gpio_num, trigger);
    if (ret != GPIO_RET_SUCCESS) {
        SPI_LAN_ERR("%s: call hal_gpio_intr_mode_set set mode %d for gpio "
                "failed(gpio_num=%d,ret=%d)", __func__, trigger, gpio_num, ret);
        return -1;
    }

    ret = hal_gpio_intr_enable(gpio_num, 0);
    if (ret != GPIO_RET_SUCCESS) {
        SPI_LAN_ERR("%s: call hal_gpio_intr_enable set gpio intr "
                "failed(gpio_num=%d,ret=%d)", __func__,  gpio_num, ret);
        return -1;
    }

    return ret;
}

static int spi_ethernet_gpio_init(void)
{
    int ret;
    TUYA_GPIO_IRQ_T int_pin;
    TUYA_GPIO_BASE_CFG_T rst_pin;

    memset(&int_pin, 0, sizeof(int_pin));
    memset(&rst_pin, 0, sizeof(rst_pin));

    int_pin.mode = TUYA_GPIO_IRQ_HIGH;
    int_pin.cb = ch390_interrupt_handler;
    int_pin.arg = NULL;

    ret = spi_ethernet_interrupt_init(INT_PIN, &int_pin);
    if (ret < 0) {
        SPI_LAN_ERR("%s: call tkl_gpio_irq_init init gpio %d failed", __func__, INT_PIN);
        return -1;
    }

#ifdef SPI_LAN_HARDWARE_RESET_EN
    rst_pin.direct = TUYA_GPIO_OUTPUT;
    rst_pin.mode = TUYA_GPIO_PULLUP;
    rst_pin.level = TUYA_GPIO_LEVEL_LOW;
    ret = tkl_gpio_init(RST_PIN, &rst_pin);
    if (ret < 0) {
        SPI_LAN_ERR("%s: call tkl_gpio_init init gpio %d failed", __func__, RST_PIN);
        return -1;
    }

    tkl_gpio_write(RST_PIN, TUYA_GPIO_LEVEL_HIGH);
#endif    
    return 0;
}

int spi_ethernet_init(void) 
{
    int ret;
    spi_interface_config_t spi_cfg;

    if (spi_ethernet_initiated) {
        SPI_LAN_ERR("%s: spi ethernet has been initiated %d",
                    __func__, spi_ethernet_initiated);
        return -1;
    }

    /* Init system function */
    if (spi_ethernet_sys_init() < 0) {
        return -1;
    }

    /* Init spi ehternet GPIOs */
    if (spi_ethernet_gpio_init() < 0) {
        spi_ethernet_sys_deinit();
        return -1;
    }

    /* Init SPI */
    spi_cfg.addr_len = 3;
    spi_cfg.data_len = 8;
    spi_cfg.spi_clk_pol = 0;
    spi_cfg.spi_clk_pha = 0;
    spi_cfg.spi_trans_mode = SPI_MODE_STANDARD;
    //spi_cfg.master_clk = 2;
    spi_cfg.master_clk = 1;
    spi_cfg.addr_pha_enable = 0;
    spi_cfg.cmd_read = SPI_TRANSCTRL_TRAMODE_WR | SPI_TRANSCTRL_CMDDIS;
    spi_cfg.cmd_write = SPI_TRANSCTRL_TRAMODE_WR | SPI_TRANSCTRL_CMDDIS;
    spi_cfg.dummy_bit = SPI_TRANSCTRL_DUMMY_CNT_1;
    spi_cfg.spi_dma_enable = 0;

    ret = spi_init_cfg(&spi_cfg);
    if (ret < 0) {
        SPI_LAN_ERR("%s: call spi_ini_cfg init spi failed\r\n", __func__);
        spi_ethernet_sys_deinit();
        return -1;
    }
#ifdef SPI_LAN_HARDWARE_RESET_EN
    /* Reset CH390 PHY */
    ch390_hardware_reset();
#else
    ch390_software_reset();
#endif

    ret = hal_gpio_intr_enable(INT_PIN, 1);
    if (ret != GPIO_RET_SUCCESS) {
        SPI_LAN_ERR("%s: call hal_gpio_intr_enable "
                "enable intr failed(ret=%d)", __func__, ret);
         spi_ethernet_sys_deinit();       
        return -1;
    }

#ifdef SPI_LAN_DBG_EN
    //ch390_print_info();
#endif /* SPI_LAN_DBG_EN */

    spi_ethernet_initiated = 1;
    return 0;
}

#if 1
static int ch390_register_dump(cmd_tbl_t *t, int argc, char *argv[])
{
    ch390_reg_dump();
    SPI_LAN_LOG("%s: interrupt counter: %u", __func__, interrupt_cnt);
    SPI_LAN_LOG("%s: last interrupt status: 0x%x", __func__, last_interrupt_status);
    SPI_LAN_LOG("%s: Number of Tx packets success: %u", __func__, ch390_txrx_stats.tx_ok);
    SPI_LAN_LOG("%s: Number of Tx packets fail: %u", __func__, ch390_txrx_stats.tx_fail);
    SPI_LAN_LOG("%s: Number of Rx packets success: %u", __func__, ch390_txrx_stats.rx_ok);
    SPI_LAN_LOG("%s: Number of Rx packets fail: %u", __func__, ch390_txrx_stats.rx_fail);
    SPI_LAN_LOG("%s: Number of Rx packets drop: %u", __func__, ch390_txrx_stats.rx_drop);
#ifdef CH390_TX_ERR_CHK_EN
    SPI_LAN_LOG("%s: Number of reset Tx FIFO pointer: %u", __func__, reset_tx_fifo_cnt);
#endif /* CH390_TX_ERR_CHK_EN */ 
    return 0;
}

CLI_CMD(ch390_register_dump, ch390_register_dump, "ch390_register_dump", "ch390_register_dump");

static int ch390_clear_tx(cmd_tbl_t *t, int argc, char *argv[])
{
    ch390_write_reg(CH390_MPTRCR, 0x02);  //Reset TX FIFO pointer
    return 0;
}

CLI_CMD(ch390_clear_tx, ch390_clear_tx, "ch390_clear_tx", "ch390_clear_tx");


static int ch390_show_status(cmd_tbl_t *t, int argc, char *argv[])
{
    NW_IP_S ip;
    TKL_WIRED_STAT_E link_status;
    int i;
    ip_addr_t * dns_srv;

    tkl_wired_get_ip(&ip);
    tkl_wired_get_status(&link_status);
    SPI_LAN_DBG("[LANDBG]%s: get wired ip %s mask %s gw %s",
        __func__, ip.ip, ip.mask, ip.gw);
    for (i = 0; i < DNS_MAX_SERVERS; i++) {
            dns_srv = dns_getserver(i);
            if (dns_srv && dns_srv->addr) {
                SPI_LAN_DBG("[LANDBG]%s: DNS server %i ip %d.%d.%d.%d",
                    __func__, i, dns_srv->addr & 0xFF, (dns_srv->addr >> 8) & 0xFF, (dns_srv->addr >> 16) & 0xFF, (dns_srv->addr >> 24) & 0xFF);
            }
    }    
    return 0;
}

CLI_CMD(ch390_show_status, ch390_show_status, "ch390_show_status", "ch390_show_status");

extern unsigned char drv_pkt_trace_en;
static int ch390_pkt_trace(cmd_tbl_t *t, int argc, char *argv[])
{
    drv_pkt_trace_en = atoi(argv[1]);
    SPI_LAN_LOG("%s: set packet trace to %d", __func__, drv_pkt_trace_en);
    return 0;
}
CLI_CMD(ch390_pkt_trace, ch390_pkt_trace, "ch390_pkt_trace", "ch390_pkt_trace");

static int ch390_reg_dbg(cmd_tbl_t *t, int argc, char *argv[])
{
    ch390_reg_dbg_en = atoi(argv[1]);
    SPI_LAN_LOG("%s: set ch390_reg_dbg_en to %d", __func__, ch390_reg_dbg_en);
    return 0;
}
CLI_CMD(ch390_reg_dbg, ch390_reg_dbg, "ch390_reg_dbg", "ch390_reg_dbg");
#endif


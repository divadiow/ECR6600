#include "spi_repeater.h"
#include "spi_master.h"
#include "spi_service_main.h"
#include "spi_service_mem.h"
#include "system_wifi.h"
#include "system_wifi_def.h"
#include "system_lwip.h"
#include "system_config.h"
#include "lwip/inet.h"
#include "net_def.h"
#include "vnet_register.h"
#include "cli.h"

typedef struct {
    int taskHandle;
    os_queue_handle_t queue;
} spi_repeater_priv_t;

unsigned char g_repeater_apssid[WIFI_SSID_MAX_LEN] = "repeater";
unsigned char g_repeater_appwd[WIFI_PWD_MAX_LEN] = "12345678";

unsigned char g_repeater_apgw[] = "20.20.20.1";
unsigned char g_repeater_apip[] = "20.20.20.1";
unsigned char g_repeater_apmask[] = "255.255.255.1";

spi_repeater_priv_t g_repeater_priv;

extern uint8_t fhost_tx_check_is_shram(void *p);

int spi_repeater_mem_free(struct pbuf *p)
{
    return spi_service_mem_pbuf_free(p) ? 0 : 1;
}

static err_t spi_repeater_sta_send(struct netif *net_if, struct pbuf *p_buf)
{
    spi_service_send_pbuf(p_buf);

    return 0;
}

static void spi_repeater_create_apnet(void)
{
    wifi_config_u wifiCfg = {0};
    wifi_work_mode_e wifi_mode = WIFI_MODE_AP;
    //struct ip_info ipconfig = {0};

    wifi_set_opmode(wifi_mode);
    wifiCfg.ap.channel = WIFI_CHANNEL_1;
    wifiCfg.ap.authmode = AUTH_WPA_WPA2_PSK;

    if (strlen((char *)g_repeater_apssid) == 0) {
        os_printf(LM_OS, LL_ERR, "ap ssid can not empty\n");
        return;
    }
    snprintf((char *)wifiCfg.ap.ssid, sizeof(wifiCfg.ap.ssid), "%s", g_repeater_apssid);

    if (strlen((char *)g_repeater_appwd) != 0) {
        snprintf((char *)wifiCfg.ap.password, sizeof(wifiCfg.ap.password), "%s", g_repeater_appwd);
    }
    wifi_stop_softap();
#if 0
    if (strlen((char *)g_repeater_apip) != 0 && strlen((char *)g_repeater_apgw) != 0 && strlen((char *)g_repeater_apmask) != 0) {
        inet_aton((char *)g_repeater_apip, &ipconfig.ip);
        inet_aton((char *)g_repeater_apgw, &ipconfig.gw);
        inet_aton((char *)g_repeater_apmask, &ipconfig.netmask);
        set_softap_ipconfig(&ipconfig);
    }
#endif
    wifi_start_softap(&wifiCfg);
#ifdef CONFIG_SPI_MASTER
    enable_lwip_napt(SOFTAP_IF, 1);
#endif
}

void spi_repeater_create_stanet(void)
{
    vnet_reg_t * vReg = vnet_reg_get_addr();
    net_if_t *net_if = get_netif_by_index(STATION_IF);
    struct ip_info ipconfig = {0};
    unsigned char mac[NETIF_MAX_HWADDR_LEN] = {0};
    unsigned char *tmpChar = NULL;
    int idx;

    if (vReg->status == 0) {
        wifi_set_mac_addr(STATION_IF, mac);
        ipconfig.ip.u_addr.ip4.addr = 0;
        ipconfig.gw.u_addr.ip4.addr = 0;
        ipconfig.netmask.u_addr.ip4.addr = 0;
        set_sta_ipconfig(&ipconfig);
        wifi_set_ip_info(STATION_IF, &ipconfig);
        net_if_down(net_if);
        return;
    }

    tmpChar = (unsigned char *)&vReg->macAddr0;
    for (idx = 0; idx < NETIF_MAX_HWADDR_LEN; idx++) {
        if (idx < NETIF_MAX_HWADDR_LEN/2) {
            mac[idx] = tmpChar[NETIF_MAX_HWADDR_LEN/2 - idx - 1];
        } else {
            mac[idx] = tmpChar[NETIF_MAX_HWADDR_LEN + NETIF_MAX_HWADDR_LEN/2 - idx];
        }
    }

    wifi_set_mac_addr(STATION_IF, mac);
    ipconfig.ip.u_addr.ip4.addr = lwip_htonl(vReg->ipv4Addr);
    ipconfig.gw.u_addr.ip4.addr = lwip_htonl(vReg->gw0);
    ipconfig.netmask.u_addr.ip4.addr = lwip_htonl(vReg->ipv4Mask);
    set_sta_ipconfig(&ipconfig);
    wifi_set_ip_info(STATION_IF, &ipconfig);
    net_if->linkoutput = spi_repeater_sta_send;
    net_if_up(net_if);
    netif_set_default(net_if);
}

static err_t spi_repeater_sta_recv(unsigned int addr)
{
    net_if_t *net_if = get_netif_by_index(STATION_IF);
    struct pbuf *p_buf = (struct pbuf *)addr;
#if 0
    int idx;
    unsigned char *pdata = p_buf->payload;

    os_printf(LM_OS, LL_ERR, "###########recv data 0x%x-0x%x-%d##############\n", p_buf, p_buf->payload, p_buf->tot_len);
    for (idx = 0; idx < p_buf->tot_len; idx++) {
        os_printf(LM_OS, LL_ERR, "0x%02x ", pdata[idx]);
        if ((idx + 1) % 16 == 0) {
            os_printf(LM_OS, LL_ERR, "\n");
        }
    }
    os_printf(LM_OS, LL_ERR, "\n");

    os_printf(LM_OS, LL_ERR, "###############################################\n");
#endif
    if (net_if) {
        if (net_if->input(p_buf, net_if) != 0) {
            spi_repeater_mem_free(p_buf);
        }
    } else {
        spi_repeater_mem_free(p_buf);
    }

    return 0;
}

int spi_repeater_send_msg(spi_repeater_msg_t *msg)
{
    if (g_repeater_priv.queue) {
        return os_queue_send(g_repeater_priv.queue, (char *)msg, sizeof(spi_repeater_msg_t), 0);
    } else {
        return -1;
    }
}

void spi_repeater_task(void *arg)
{
    spi_repeater_msg_t msg = {0};
    int ret;

    do {
        ret = os_queue_receive(g_repeater_priv.queue, (char *)&msg, sizeof(spi_repeater_msg_t), WAIT_FOREVER);
        if (ret < 0) {
            os_printf(LM_OS, LL_ERR, "spi repeater queue recv error.\n");
            break;
        }

        switch (msg.msgType) {
            case SPI_REPEATER_GET_INFO:
                spi_master_read_info(sizeof(vnet_reg_t));
                break;
            case SPI_REPEATER_START_STA:
                spi_repeater_create_stanet();
                break;
            case SPI_REPEATER_PACKETIN:
                spi_repeater_sta_recv(msg.msgAddr);
                break;
        }
    } while (1);
}

void spi_repeater_init(void)
{
    g_repeater_priv.queue = os_queue_create("spi repeater queue", 32, sizeof(spi_repeater_msg_t), 0);
    g_repeater_priv.taskHandle = os_task_create("spiRepeaterTask", 5, (4*1024), spi_repeater_task, NULL);
}

static int spi_repeater_start_ap(cmd_tbl_t *t, int argc, char *argv[])
{
    spi_repeater_create_apnet();

    return CMD_RET_SUCCESS;
}

CLI_CMD(spi_repeater_ap, spi_repeater_start_ap, "spi_repeater_start_ap", "spi_repeater_start_ap");


#include "spi_service_main.h"
#include "spi_service.h"
#include "cli.h"
#include "system_wifi.h"
#ifdef CONFIG_VNET_SERVICE
#include "vnet_filter.h"
#include "vnet_int.h"

#define VNET_MAX_PORT_RANGE 65535

#define  STRING_STOP_CHECK(X) \
    if ((*(char *)X) != '\0') { \
        os_printf(LM_APP, LL_INFO, "Not support character %s\n", X); \
        return CMD_RET_FAILURE; \
    }

static int spi_slave_filter_qeury_test(cmd_tbl_t *t, int argc, char *argv[])
{
    vnet_ipv4_filter *filter = NULL;
    unsigned int num;
    int index;
    ip4_addr_t addr;
    unsigned char default_dir = vnet_get_default_filter();
    os_printf(LM_APP, LL_INFO, "#################################\n");
    os_printf(LM_APP, LL_INFO, "default dir: %s\n", default_dir ? "host controller" : "local lwip");
    os_printf(LM_APP, LL_INFO, "#################################\n");

    vnet_query_filter((void **)&filter, &num, VNET_FILTER_TYPE_IPV4);
    for (index = 0; index < num; index++) {
        os_printf(LM_APP, LL_INFO, "#################################\n");
        os_printf(LM_APP, LL_INFO, "rules: 0x%x\n", index + 1);
        os_printf(LM_APP, LL_INFO, "dir: %s\n",  (filter[index].dir == VNET_PACKET_DICTION_BOTH) ? "Both" : (filter[index].dir ? "host controller" : "local lwip"));
        addr.addr = lwip_htonl(filter[index].ipaddr);
        os_printf(LM_APP, LL_INFO, "ipaddr: %s[0x%x]\n", inet_ntoa(addr), filter[index].ipaddr);
        os_printf(LM_APP, LL_INFO, "sport: %u[%u-%u]\n", filter[index].srcPort, filter[index].srcPmin, filter[index].srcPmax);
        os_printf(LM_APP, LL_INFO, "dport: %u[%u-%u]\n", filter[index].dstPort, filter[index].dstPmin, filter[index].dstPmax);
        os_printf(LM_APP, LL_INFO, "type: 0x%x\n", filter[index].packeType);
        os_printf(LM_APP, LL_INFO, "mask: 0x%x\n", filter[index].mask);
        os_printf(LM_APP, LL_INFO, "#################################\n");
    }

    if (filter) {
        free(filter);
    }
    return CMD_RET_SUCCESS;
}

CLI_CMD(filter_query, spi_slave_filter_qeury_test, "filter query", "filter_query");

static int spi_slave_filter_parse(vnet_ipv4_filter *filter, int argc, char *argv[])
{
    char *stop;
    unsigned int port;

    filter->dir = strtoul(argv[1], &stop, 0);
    STRING_STOP_CHECK(stop);

    filter->ipaddr = inet_addr(argv[2]);//suport x/x.x/x.x.x/x.x.x.x
    filter->ipaddr = lwip_htonl(filter->ipaddr);
    if (filter->ipaddr != IPADDR_NONE) {
        int cnt = 0;
        char *ipstr = argv[2];
        char *dot;
        while ((dot = strstr(ipstr, ".")) != NULL) {
            ipstr = dot + 1;
            cnt++;
        }

        if (cnt == 3) {
            filter->mask |= VNET_FILTER_MASK_IPADDR;
            if (filter->ipaddr == 0) {
                os_printf(LM_APP, LL_INFO, "Not support ipaddr %s\n", argv[2]);
                return CMD_RET_FAILURE;
            }
        } else {
            filter->ipaddr = strtoul(argv[2], &stop, 0);
            STRING_STOP_CHECK(stop);
            if (filter->ipaddr != 0) {
                os_printf(LM_APP, LL_INFO, "Not support ipaddr %s, format is x.x.x.x\n", argv[2]);
                return CMD_RET_FAILURE;
            }
        }
    } else {
        os_printf(LM_APP, LL_INFO, "Not support ipaddr %s, format is 0-255.0-255.0-255.0-254\n", argv[2]);
        return CMD_RET_FAILURE;
    }

    port = strtoul(argv[3], &stop, 0);
    if (port > VNET_MAX_PORT_RANGE) {
        os_printf(LM_APP, LL_INFO, "src port(%u) out of range\n", port);
        return CMD_RET_FAILURE;
    }

    filter->srcPort = port;
    STRING_STOP_CHECK(stop);
    if (filter->srcPort != 0)
        filter->mask |= VNET_FILTER_MASK_SRC_PORT;
    port = strtoul(argv[4], &stop, 0);
    if (port > VNET_MAX_PORT_RANGE) {
        os_printf(LM_APP, LL_INFO, "src min port(%u) out of range\n", port);
        return CMD_RET_FAILURE;
    }
    filter->srcPmin = port;
    STRING_STOP_CHECK(stop);
    port = strtoul(argv[5], &stop, 0);
    if (port > VNET_MAX_PORT_RANGE) {
        os_printf(LM_APP, LL_INFO, "src max port(%u) out of range\n", port);
        return CMD_RET_FAILURE;
    }
    filter->srcPmax = port;
    STRING_STOP_CHECK(stop);
    if (filter->srcPmin != 0 || filter->srcPmax != 0) {
        filter->mask |= VNET_FILTER_MASK_SRC_PORT_RANGE;
    }

    port = strtoul(argv[6], &stop, 0);
    if (port > VNET_MAX_PORT_RANGE) {
        os_printf(LM_APP, LL_INFO, "dst port(%u) out of range\n", port);
        return CMD_RET_FAILURE;
    }
    filter->dstPort = port;
    STRING_STOP_CHECK(stop);
    if (filter->dstPort != 0)
        filter->mask |= VNET_FILTER_MASK_DST_PORT;
    port = strtoul(argv[7], &stop, 0);
    if (port > VNET_MAX_PORT_RANGE) {
        os_printf(LM_APP, LL_INFO, "dst min port(%u) out of range\n", port);
        return CMD_RET_FAILURE;
    }
    filter->dstPmin = port;
    STRING_STOP_CHECK(stop);
    port = strtoul(argv[8], &stop, 0);
    if (port > VNET_MAX_PORT_RANGE) {
        os_printf(LM_APP, LL_INFO, "dst max port(%u) out of range\n", port);
        return CMD_RET_FAILURE;
    }
    filter->dstPmax = port;
    STRING_STOP_CHECK(stop);
    if (filter->dstPmin != 0 || filter->dstPmax != 0) {
        filter->mask |= VNET_FILTER_MASK_DST_PORT_RANGE;
    }

    filter->packeType = strtoul(argv[9], &stop, 0);
    STRING_STOP_CHECK(stop);
    if (filter->packeType != 0) {
        filter->mask |= VNET_FILTER_MASK_PROTOCOL;
    }

    return CMD_RET_SUCCESS;
}

static int spi_slave_filter_add_test(cmd_tbl_t *t, int argc, char *argv[])
{
    vnet_ipv4_filter filter;
    int ret;

    if (argc != 10) {
        os_printf(LM_APP, LL_INFO, "filter_add dir(0/1/2) ip(x.x.x.x) sport spmin spmax dport dpmin dpmax protocol(6/17)\n");
        return CMD_RET_FAILURE;
    }
    memset(&filter, 0, sizeof(filter));
    ret = spi_slave_filter_parse(&filter, argc, argv);
    if (ret != CMD_RET_SUCCESS)
        return ret;
    ret = vnet_add_filter(&filter, sizeof(filter), VNET_FILTER_TYPE_IPV4);
    if (ret != CMD_RET_SUCCESS)
        return ret;

    return CMD_RET_SUCCESS;
}

CLI_CMD(filter_add, spi_slave_filter_add_test, "filter add", "filter_add dir(0/1/2) ip(x.x.x.x) sport spmin spmax dport dpmin dpmax protocol(6/17)");

static int spi_slave_filter_del_test(cmd_tbl_t *t, int argc, char *argv[])
{
    vnet_ipv4_filter filter;
    int ret;

    if (argc != 10) {
        os_printf(LM_APP, LL_INFO, "filter_del dir(0/1/2) ip(x.x.x.x) sport spmin spmax dport dpmin dpmax protocol(6/17)\n");
        return CMD_RET_FAILURE;
    }
    memset(&filter, 0, sizeof(filter));

    ret = spi_slave_filter_parse(&filter, argc, argv);
    if (ret != CMD_RET_SUCCESS)
        return ret;
    ret = vnet_del_filter(&filter, sizeof(filter), VNET_FILTER_TYPE_IPV4);
    if (ret != CMD_RET_SUCCESS)
        return ret;

    return CMD_RET_SUCCESS;
}

CLI_CMD(filter_del, spi_slave_filter_del_test, "filter delete", "filter_del dir(0/1/2) ip(x.x.x.x) sport spmin spmax dport dpmin dpmax protocol(6/17)");

static int spi_slave_filter_default_test(cmd_tbl_t *t, int argc, char *argv[])
{
    unsigned int default_dir;
    char *stop;
    int ret;

    if (argc != 2) {
        os_printf(LM_APP, LL_INFO, "filter_default local_lwip(0)/host_controller(1)\n");
        return CMD_RET_FAILURE;
    }

    default_dir = strtoul(argv[1], &stop, 0);
    STRING_STOP_CHECK(stop);

    ret = vnet_set_default_filter(default_dir);
    if (ret != CMD_RET_SUCCESS)
        return ret;

    return CMD_RET_SUCCESS;
}

CLI_CMD(filter_default, spi_slave_filter_default_test, "filter default", "filter_default local_lwip(0)/host_controller(1)");

static int spi_slave_interrupt_init(cmd_tbl_t *t, int argc, char *argv[])
{
    unsigned int num;
    char *stop;
    int ret;

    if (argc != 2) {
        os_printf(LM_APP, LL_INFO, "inter_init num(1-29)\n");
        return CMD_RET_FAILURE;
    }

    num = strtoul(argv[1], &stop, 0);
    STRING_STOP_CHECK(stop);
    if (num == 0) {
        os_printf(LM_APP, LL_INFO, "int 0 default as wifi cs link status\n");
        return CMD_RET_FAILURE;
    }

    ret = vnet_interrupt_enable(num);
    if (ret != CMD_RET_SUCCESS)
        return ret;

    return CMD_RET_SUCCESS;
}

CLI_CMD(inter_init, spi_slave_interrupt_init, "spi_slave_interrupt_init", "inter_init num(0-29)");

static int spi_slave_interrupt_handle(cmd_tbl_t *t, int argc, char *argv[])
{
    unsigned int num;
    char *stop;
    int ret;

    if (argc != 2) {
        os_printf(LM_APP, LL_INFO, "inter_handle num(1-29)\n");
        return CMD_RET_FAILURE;
    }

    num = strtoul(argv[1], &stop, 0);
    STRING_STOP_CHECK(stop);
    if (num == 0) {
        os_printf(LM_APP, LL_INFO, "int 0 default as wifi cs link status\n");
        return CMD_RET_FAILURE;
    }

    ret = vnet_interrupt_handle(num);

    if (ret != CMD_RET_SUCCESS)
        return ret;

    return CMD_RET_SUCCESS;
}

CLI_CMD(inter_handle, spi_slave_interrupt_handle, "spi_slave_interrupt_handle", "inter_handle num(0-29)");
#endif

static int spi_slave_msg_send_handle(cmd_tbl_t *t, int argc, char *argv[])
{
    if (argc != 2) {
        os_printf(LM_APP, LL_INFO, "spi_msg xxxxxx\n");
        return CMD_RET_FAILURE;
    }

    spi_service_send_msg((unsigned char *)argv[1], strlen(argv[1]));

    return CMD_RET_SUCCESS;
}

CLI_CMD(spi_msg, spi_slave_msg_send_handle, "spi_slave_msg_send_handle", "spi_slave_msg_send_handle");


static int spi_slave_start_sta(cmd_tbl_t *t, int argc, char *argv[])
{
    wifi_config_u wifiCfg = {0};
    wifi_work_mode_e wifi_mode = WIFI_MODE_STA;

    if (argc != 3 && argc != 2) {
        os_printf(LM_APP, LL_INFO, "spi_sta ssid [pwd]\n");
        return CMD_RET_FAILURE;
    }
    wifi_set_opmode(wifi_mode);
    snprintf((char *)wifiCfg.sta.ssid, sizeof(wifiCfg.sta.ssid), "%s", argv[1]);
    if (argc == 3) {
        snprintf((char *)wifiCfg.sta.password, sizeof(wifiCfg.sta.password), "%s", argv[2]);
    }
    wifi_stop_station();
    wifi_stop_softap();
    wifi_start_station(&wifiCfg);

    return CMD_RET_SUCCESS;
}

CLI_CMD(spi_sta, spi_slave_start_sta, "spi_slave_start_sta", "spi_slave_start_sta");

static int spi_slave_start_ap(cmd_tbl_t *t, int argc, char *argv[])
{
    wifi_config_u wifiCfg = {0};
    wifi_work_mode_e wifi_mode = WIFI_MODE_AP;

    if (argc != 3) {
        os_printf(LM_APP, LL_INFO, "spi_ap ssid pwd\n");
        return CMD_RET_FAILURE;
    }
    wifi_set_opmode(wifi_mode);
    wifiCfg.ap.channel = WIFI_CHANNEL_1;
    wifiCfg.ap.authmode = AUTH_WPA_WPA2_PSK;
    snprintf((char *)wifiCfg.ap.ssid, sizeof(wifiCfg.ap.ssid), "%s", argv[1]);
    snprintf((char *)wifiCfg.ap.password, sizeof(wifiCfg.ap.password), "%s", argv[2]);
    wifi_stop_station();
    wifi_stop_softap();
    wifi_start_softap(&wifiCfg);

    return CMD_RET_SUCCESS;
}

CLI_CMD(spi_ap, spi_slave_start_ap, "spi_slave_start_ap", "spi_slave_start_ap");

#ifdef SPI_SERVICE_LOOP_TEST
static int spi_service_send_loop(cmd_tbl_t *t, int argc, char *argv[])
{
    unsigned int data[512];
    int idx;
    unsigned int num = 1;
    char *stop;

    if (argc == 2) {
        num = strtoul(argv[1], &stop, 0);
        if (num <= 0) {
            num = 1;
        }
    }

    for (idx = 0; idx < 512; idx++) {
        data[idx] = idx;
    }

    do {
        if (spi_service_send_data((unsigned char *)data, 1460) < 0) {
            os_msleep(1);
        }
    } while (--num);

    return CMD_RET_SUCCESS;
}

CLI_CMD(spi_service_loop, spi_service_send_loop, "spi_service_send_loop", "spi_service_send_loop");
#endif

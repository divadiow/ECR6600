#include "vnet_register.h"
#include "vnet_int.h"
#include "system_wifi.h"
#include "system_config.h"
#ifdef CONFIG_SPI_REPEATER
#include "spi_repeater.h"
#endif
#ifdef VNET_REG_DEBUG_SUPPORT
#include "cli.h"
#endif

vnet_reg_t g_vnet_reg __SHAREDRAM;

vnet_reg_op_t g_vnet_reg_table[] =
{
      {0x00,    (unsigned int)&g_vnet_reg.status,      VNET_REG_ATTRI_RO,    NULL},
      {0x04,    (unsigned int)&g_vnet_reg.intFlag,     VNET_REG_ATTRI_RO,    NULL},
      {0x08,    (unsigned int)&g_vnet_reg.intMask,     VNET_REG_ATTRI_RW,    NULL},
      {0x0c,    (unsigned int)&g_vnet_reg.intClr,      VNET_REG_ATTRI_RW,    NULL},
      {0x10,    (unsigned int)&g_vnet_reg.ipv4Addr,    VNET_REG_ATTRI_RO,    NULL},
      {0x14,    (unsigned int)&g_vnet_reg.ipv4Mask,    VNET_REG_ATTRI_RO,    NULL},
      {0x18,    (unsigned int)&g_vnet_reg.macAddr0,    VNET_REG_ATTRI_RO,    NULL},
      {0x1C,    (unsigned int)&g_vnet_reg.macAddr1,    VNET_REG_ATTRI_RO,    NULL},
      {0x20,    (unsigned int)&g_vnet_reg.gw0,         VNET_REG_ATTRI_RO,    NULL},
      {0x24,    (unsigned int)&g_vnet_reg.gw1,         VNET_REG_ATTRI_RO,    NULL},
      {0x28,    (unsigned int)&g_vnet_reg.gw2,         VNET_REG_ATTRI_RO,    NULL},
      {0x2C,    (unsigned int)&g_vnet_reg.gw3,         VNET_REG_ATTRI_RO,    NULL},
      {0x30,    (unsigned int)&g_vnet_reg.dns0,        VNET_REG_ATTRI_RO,    NULL},
      {0x34,    (unsigned int)&g_vnet_reg.dns1,        VNET_REG_ATTRI_RO,    NULL},
      {0x38,    (unsigned int)&g_vnet_reg.dns2,        VNET_REG_ATTRI_RO,    NULL},
      {0x3C,    (unsigned int)&g_vnet_reg.dns3,        VNET_REG_ATTRI_RO,    NULL},
      {0x40,    (unsigned int)&g_vnet_reg.fwVersion,   VNET_REG_ATTRI_RO,    NULL},
      {0x44,    (unsigned int)&g_vnet_reg.powerOff,    VNET_REG_ATTRI_RW,    NULL},
};

#define VNET_REG_TABLE_NUM (sizeof(g_vnet_reg_table) / sizeof(vnet_reg_op_t))

void vnet_reg_op_call(unsigned int addr)
{
    int idx;

    for (idx = 0; idx < VNET_REG_TABLE_NUM; idx++) {
        if (g_vnet_reg_table[idx].regAddr != addr) {
            continue;
        }

        if (g_vnet_reg_table[idx].opCall) {
            g_vnet_reg_table[idx].opCall();
        }
    }
}

vnet_reg_op_t *vnet_reg_get_table(unsigned int offset)
{
    unsigned int getNum = offset / 4;

    if (getNum > VNET_REG_TABLE_NUM) {
        return NULL;
    }

    return &g_vnet_reg_table[getNum];
}

vnet_reg_t *vnet_reg_get_addr(void)
{
    return &g_vnet_reg;
}

void vnet_reg_get_info(void)
{
    wifi_work_mode_e wifiMode = wifi_get_opmode();
    struct netif *staif = get_netif_by_index(wifiMode);
    unsigned char addr[6] = {0};
    unsigned int infoValue = 0;
    int index;

    if (staif == NULL) {
        os_printf(LM_APP, LL_ERR, "wifiMode(0x%x) not support\n", wifiMode);
        return;
    }

    if (wifiMode == WIFI_MODE_STA) {
        hal_system_get_sta_mac(addr);
    }

    if (wifiMode == WIFI_MODE_AP) {
        hal_system_get_ap_mac(addr);
    }

    memset(&g_vnet_reg, 0, sizeof(vnet_reg_t));
    g_vnet_reg.macAddr0 = (addr[0] << 16) | (addr[1] << 8) | addr[2];
    g_vnet_reg.macAddr1 = (addr[3] << 16) | (addr[4] << 8) | addr[5];
    wifi_get_ip_addr(wifiMode, &infoValue);
    g_vnet_reg.ipv4Addr = ((infoValue & 0xFF) << 24) | (((infoValue >> 8) & 0xFF) << 16);
    g_vnet_reg.ipv4Addr |= (((infoValue >> 16) & 0xFF) << 8) | ((infoValue >> 24) & 0xFF);
    wifi_get_mask_addr(wifiMode, &infoValue);
    g_vnet_reg.ipv4Mask = ((infoValue & 0xFF) << 24) | (((infoValue >> 8) & 0xFF) << 16);
    g_vnet_reg.ipv4Mask |= (((infoValue >> 16) & 0xFF) << 8) | ((infoValue >> 24) & 0xFF);
    wifi_get_gw_addr(wifiMode, &infoValue);
    g_vnet_reg.gw0 = ((infoValue & 0xFF) << 24) | (((infoValue >> 8) & 0xFF) << 16);
    g_vnet_reg.gw0 |= (((infoValue >> 16) & 0xFF) << 8) | ((infoValue >> 24) & 0xFF);

    for (index = 0; index < DNS_MAX_SERVERS; index++) {
        wifi_get_dns_addr(index, &infoValue);
        unsigned int *pd = (unsigned int *)&g_vnet_reg.dns0;
        *(pd + index) = ((infoValue & 0xFF) << 24) | (((infoValue >> 8) & 0xFF) << 16);
        *(pd + index) |= (((infoValue >> 16) & 0xFF) << 8) | ((infoValue >> 24) & 0xFF);
    }

    g_vnet_reg.fwVersion = 6600;
}

void vnet_reg_wifilink_status(int link)
{
    unsigned int flag;

    if ((link == VNET_WIFI_LINK_UP) && (link != g_vnet_reg.status)) {
        vnet_reg_get_info();
        flag = system_irq_save();
        g_vnet_reg.status = VNET_WIFI_LINK_UP;
        vnet_interrupt_enable(VNET_INTERRUPT_LINK_CS);
        vnet_interrupt_handle(VNET_INTERRUPT_LINK_CS);
        system_irq_restore(flag);
    }

    if ((link == VNET_WIFI_LINK_DOWN) && (link != g_vnet_reg.status)) {
        flag = system_irq_save();
        g_vnet_reg.status = VNET_WIFI_LINK_DOWN;
        vnet_interrupt_enable(VNET_INTERRUPT_LINK_CS);
        vnet_interrupt_handle(VNET_INTERRUPT_LINK_CS);
        system_irq_restore(flag);
    }
}

#ifdef CONFIG_SPI_MASTER
#ifdef CONFIG_SPI_REPEATER
void vnet_reg_get_peer_value(void)
{
    spi_repeater_msg_t msg;

    msg.msgType = SPI_REPEATER_GET_INFO;
    spi_repeater_send_msg(&msg);
}
#endif
#endif

#ifdef VNET_REG_DEBUG_SUPPORT
static int vnet_reg_get_value(cmd_tbl_t *t, int argc, char *argv[])
{
    int idx = 0;
    int regNum = sizeof(g_vnet_reg_table) / sizeof(vnet_reg_op_t);

    for (idx = 0; idx < regNum; idx++) {
        os_printf(LM_OS, LL_INFO, "addr:0x%08x\t value:0x%08x\n", g_vnet_reg_table[idx].offset,
            *(unsigned int *)(g_vnet_reg_table[idx].regAddr));
    }

    return CMD_RET_SUCCESS;
}

CLI_CMD(vnet_reg_read, vnet_reg_get_value, "vnet_reg_get_value", "vnet_reg_get_value");
#endif

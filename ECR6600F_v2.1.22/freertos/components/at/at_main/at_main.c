// Copyright 2018-2019 trans-semi inc
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#if defined(CONFIG_ECR_BLE)
#include "ble_command.h"
#endif
#include "hal_uart.h"
#include "uart.h"
#include "at_common.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
//#include "util_cli_freertos.h"
#include "hal_system.h"
#include "dce.h"
#include "dce_commands.h"
#include "basic_command.h"
#include "wifi_command.h"
#include "tcpip_command.h"
#if defined(CONFIG_MQTT)
#include "mqtt_command.h"
#endif
#include "at_customer_wrapper.h"
#include "system_config.h"
#include "easyflash.h"
#include "at_ota.h"
#include "system_network.h"
#include "lwip/dns.h"
//#include "dhcpserver.h"
#include "chip_pinmux.h"
#include "cli.h"
#define COMMAND_TASK_PRIORITY 0
#define COMMAND_QUEUE_SIZE    1
#define COMMAND_LINE_LENGTH   4608

static dce_t* dce;

static os_queue_handle_t command_queue = NULL;

E_DRV_UART_NUM     uart_num = E_UART_NUM_1;

void  AT_command_callback(char c)
{
    const char *pCnt = &c;
    dce_handle_input(dce, pCnt, 1);
}

at_state_t AT_command_get_state(void)
{
    return dce->state;
}

void  AT_command_data_callback(char* buff, size_t size)
{
    dce_handle_input(dce, buff, size);
}

void  target_dce_request_process_command_line(dce_t* dce)
{
    if(os_queue_send(command_queue, (char*)dce, sizeof(dce), 0))
    {
		drv_uart_tx_putc(MEM_BASE_UART1, 'b');
		drv_uart_tx_putc(MEM_BASE_UART1, 'u');
		drv_uart_tx_putc(MEM_BASE_UART1, 's');
		drv_uart_tx_putc(MEM_BASE_UART1, 'y');
		drv_uart_tx_putc(MEM_BASE_UART1, '\r');
		drv_uart_tx_putc(MEM_BASE_UART1, '\n');
    }
}

void target_dec_switch_input_state(state_t state)
{
    dce->state = state;
    return;
}


int target_dce_get_state(void)
{
	return dce->state;
}

void* target_dce_get(void)
{
	return (void*)dce;
}

void  at_process_task(void* param)
{
    while(1){          
        os_queue_receive(command_queue, (char *)dce, 0, WAIT_FOREVER);
		dce_process_command_line(dce);
    }
    
}

static void cli_uart_isr(void * data)
{ 	
	dce_t *p_dce_cxt = (dce_t *)data;
	unsigned int uart_base_reg = p_dce_cxt->at_uart_base;

	while(drv_uart_rx_tstc(uart_base_reg))
	{
        char c = drv_uart_rx_getc(uart_base_reg);
        AT_command_callback(c);
    }
}	 	 		

void  target_dce_transmit(const char* data, size_t size)
{
    hal_uart_write(uart_num, (unsigned char*)data, size);
}

void  target_dce_reset(void)
{
    hal_system_reset(RST_TYPE_SOFTWARE_REBOOT);
}

void  target_dce_init_factory_defaults(void)
{
    system_printf("target_dce_init_factory_defaults\n");
}

void  target_dce_assert(const char* message)
{
    hal_uart_write(E_UART_NUM_0, (unsigned char*)"\r\n########\r\n", 12);
    hal_uart_write(E_UART_NUM_0, (unsigned char*)message, strlen(message));
    hal_uart_write(E_UART_NUM_0, (unsigned char*)"\r\n########\r\n", 12);
    hal_system_reset(RST_TYPE_SOFTWARE_REBOOT);
}

void  init_done()
{
    arg_t args[] = {
        { ARG_TYPE_STRING, .value.string = "AT" },
        { ARG_TYPE_STRING, .value.string = VERSION_STRING }
    };
    dce_emit_extended_result_code_with_args(dce, "IREADY", -1, args, 2, 0, false);
}

static int uart_alias_stopbit(int databits, int stopbits)
{   
    //if stop bit is 1.5,databits must be 5 bit
    if(stopbits == 2 && databits != 0){
        return -1;
    }
    //if stop bit is 2,databits must not be 5 bit
    if(stopbits == 3 && databits == 0){
        return -1;
    }
    if((stopbits == 0)||(stopbits == 1)){
        return 0;
    } 
    return 1;

}

extern bool dns_auto_change;
void AT_Net_Info_Init(void)
{
    // SYSMSG
    uint8_t value = 0;
//    uint8_t mac[6] = {0};
    wifi_work_mode_e wifi_mode;
    // struct netif netif_temp;
    
    //system_printf("init SYSMSG \r\n");
    // quxin if(!ef_get_env_blob(NV_SYSMSG_DEF, &value, sizeof(value), NULL))
    {
         //system_printf("at read sysmsg nv error!!!! \r\n");
         goto STA_IP;
    }
    extern uint8_t set_system_config(uint8_t type, uint8_t value);
    set_system_config(COMMAND_CUR, value);
#if 0
STAMAC:
    system_printf("init STAMAC \r\n");
    wifi_mode = wifi_get_opmode();
    if(wifi_mode == WIFI_MODE_STA || wifi_mode == WIFI_MODE_AP_STA)
    {
        if(SYS_ERR != wifi_load_mac_addr(STATION_IF, mac))
        {
            //system_printf("at read sta mac nv error!!!! \r\n");
            goto AP_MAC;
        }
        // quxin hal_lmac_set_mac_address(STATION_IF,mac);
        // quxin hal_lmac_set_enable_mac_addr(STATION_IF, true);
        wifi_set_mac_addr(STATION_IF,mac);
    }

AP_MAC:
    system_printf("init APMAC \r\n");
    if(wifi_mode == WIFI_MODE_AP || wifi_mode == WIFI_MODE_AP_STA)
    {
        if(SYS_ERR != wifi_load_mac_addr(SOFTAP_IF, mac))
        {
            //system_printf("at read ap mac nv error!!!! \r\n");
            goto STA_IP;
        }
        // quxin hal_lmac_set_mac_address(SOFTAP_IF,mac);
        // quxin hal_lmac_set_enable_mac_addr(SOFTAP_IF, true);
        wifi_set_mac_addr(SOFTAP_IF,mac);
    }
#endif
STA_IP:
    system_printf("init STAIP \r\n");
    wifi_mode = wifi_get_opmode();
    if(wifi_mode == WIFI_MODE_STA || wifi_mode == WIFI_MODE_AP_STA)
    {
        ip_info_t ip_info;
        bool      dhcp_sta_status = true;

        if(!hal_system_get_config(CUSTOMER_NV_WIFI_DHCPC_EN, &(dhcp_sta_status), sizeof(dhcp_sta_status))
         || (false==dhcp_sta_status))
        {
            if (!hal_system_get_config(CUSTOMER_NV_WIFI_STA_IP, &(ip_info.ip), sizeof(ip_info.ip)) || 
                !hal_system_get_config(CUSTOMER_NV_WIFI_STA_GATEWAY, &(ip_info.gw), sizeof(ip_info.gw)) || 
                !hal_system_get_config(CUSTOMER_NV_WIFI_STA_NETMASK, &(ip_info.netmask), sizeof(ip_info.netmask)))
            {
                //system_printf("at read sta ip nv error!!!! \r\n");
                goto AP_IP;
            }

            
            if(TCPIP_DHCP_STATIC != wifi_station_dhcpc_status(STATION_IF))
            {
                wifi_station_dhcpc_stop(STATION_IF);
            }
            set_sta_ipconfig(&ip_info);

        }
    }

AP_IP:
    system_printf("init APIP \r\n");
    if(wifi_mode == WIFI_MODE_AP || wifi_mode == WIFI_MODE_AP_STA)
    {
        ip_info_t ip_info;

        if (!hal_system_get_config(CUSTOMER_NV_WIFI_AP_IP, &(ip_info.ip), sizeof(ip_info.ip)) || 
            !hal_system_get_config(CUSTOMER_NV_WIFI_AP_GATEWAY, &(ip_info.gw), sizeof(ip_info.gw)) || 
            !hal_system_get_config(CUSTOMER_NV_WIFI_AP_NETMASK, &(ip_info.netmask), sizeof(ip_info.netmask)))
        {
            //system_printf("at read ap ip nv error!!!! \r\n");
            goto DNS_AUTO;
        }

        if(!wifi_set_ip_info(SOFTAP_IF,&ip_info))
        {
            //system_printf("set at ap ip nv error!!!! \r\n");
            goto DNS_AUTO;
        }
        set_softap_ipconfig(&ip_info);
    }
DNS_AUTO:
    hal_system_get_config(CUSTOMER_NV_DNS_AUTO_CHANGE, &(dns_auto_change), sizeof(dns_auto_change));

    system_printf("init DNS SERVER \r\n");
    ip_addr_t server0,server1;
    ip_2_ip4(&server0)->addr = 0;
    ip_2_ip4(&server1)->addr = 0;
    hal_system_get_config(CUSTOMER_NV_TCPIP_DNS_SERVER0, &(server0), sizeof(server0));
    hal_system_get_config(CUSTOMER_NV_TCPIP_DNS_SERVER1, &(server1), sizeof(server1));

    if(0 != ip_2_ip4(&server0)->addr)
    {
        dns_setserver(0,&server0);
    }
    else
    {
        ip_addr_t serverip;
        const char *serverstr = "208.67.222.222";

        ipaddr_aton(serverstr,&serverip);

        dns_setserver(0,&serverip);
    }
    
    if(0 != (ip_2_ip4(&server1)->addr))
        dns_setserver(1,&server1);  
    // system_printf("init DNS SERVER success\r\n");  

    bool dhcp_ap_status;
    bool dhcp_sta_status;
    struct dhcps_lease please = {0};
    uint32_t lease_time;
    if (!hal_system_get_config(CUSTOMER_NV_WIFI_DHCPS_EN, &(dhcp_ap_status), sizeof(dhcp_ap_status)))
    {
        system_printf("at dhcps nv err\r\n");
        dhcp_ap_status  = 1;    
    }

    if (!hal_system_get_config(CUSTOMER_NV_WIFI_DHCPC_EN, &(dhcp_sta_status), sizeof(dhcp_sta_status)))
    {
        system_printf("at dhcpc nv err! \r\n");
        dhcp_sta_status = 1;
    }

    dhcp_ap_status == 1 ? wifi_dhcp_open(SOFTAP_IF) : wifi_dhcp_close(SOFTAP_IF);
    dhcp_sta_status == 1 ? wifi_dhcp_open(STATION_IF) : wifi_dhcp_close(STATION_IF);

    system_printf("init DHCP_EN:%d,%d \r\n", dhcp_ap_status, dhcp_sta_status);
    
    if((wifi_mode == WIFI_MODE_AP || wifi_mode == WIFI_MODE_AP_STA) && dhcp_ap_status)
    {
        // system_printf("init DHCP SERVER \r\n");
    if (!hal_system_get_config(CUSTOMER_NV_WIFI_DHCPS_IP, &(please), sizeof(please)) ||
        !hal_system_get_config(CUSTOMER_NV_WIFI_DHCPS_LEASE_TIME, &(lease_time), sizeof(lease_time)))
        {
            //system_printf("at read dhcp server nv error!!!! \r\n");
            goto AP_START;
        }
        please.enable = 1;
		please.dhcps_lease = lease_time;
        extern dce_result_t at_softap_set_dhcps_lease(dce_t* dce, struct dhcps_lease *please, uint32_t lease_time, bool reset);
        at_softap_set_dhcps_lease(dce, &please, lease_time, false);
        // system_printf("init DHCP SERVER success\r\n");
    }

AP_START:
    if(wifi_mode == WIFI_MODE_AP || wifi_mode == WIFI_MODE_AP_STA) {
        system_printf("at softap autostart... \r\n");
        wifi_softap_auto_start();
    }

}


void AT_command_init(void)
{
    T_DRV_UART_CONFIG config;
    T_UART_ISR_CALLBACK callback;
    u32_t uart_base_reg;
    int parity_bit;
#ifdef CONFIG_AT_UART_0
    uart_base_reg = MEM_BASE_UART0;
    chip_uart0_pinmux_cfg(UART0_RX_USED_GPIO5, UART0_TX_USED_GPIO6, 0);
    uart_num = E_UART_NUM_0;
#endif
#ifdef CONFIG_AT_UART_1
    uart_base_reg = MEM_BASE_UART1;
    chip_uart1_pinmux_cfg(0); // 0:Do not use CTS RTS
    uart_num = E_UART_NUM_1;
#endif
#ifdef CONFIG_AT_UART_2
    uart_base_reg = MEM_BASE_UART2;
    chip_uart2_pinmux_cfg(UART2_TX_USED_GPIO13);
    uart_num = E_UART_NUM_2;
#endif

    if(!hal_system_get_config(CUSTOMER_NV_UART_CONFIG, &(config), sizeof(config)))
    {
        config.uart_baud_rate   = BAUD_RATE_115200;
        config.uart_data_bits   = UART_DATA_BITS_8;
        config.uart_parity_bit  = UART_PARITY_BIT_NONE;
        config.uart_stop_bits   = UART_STOP_BITS_1;
        config.uart_flow_ctrl   = UART_FLOW_CONTROL_DISABLE; 
        config.uart_tx_mode     = UART_TX_MODE_POLL;
        config.uart_rx_mode     = UART_RX_MODE_USER;
        hal_system_set_config(CUSTOMER_NV_UART_CONFIG, &(config), sizeof(config));
    }
    else
    {
        if((uart_num == 1) && (config.uart_flow_ctrl == 1))
        {
            chip_uart1_pinmux_cfg(1); 
        }
        if((uart_num == 0) && (config.uart_flow_ctrl == 1))
        {
            chip_uart0_pinmux_cfg(UART0_RX_USED_GPIO5, UART0_TX_USED_GPIO6, 1);
        }
    }

    if(config.uart_parity_bit  == 1)
    {
        config.uart_parity_bit = UART_PARITY_BIT_ODD;
        parity_bit = 1;
    }
    else if(config.uart_parity_bit  == 2)
    {
        config.uart_parity_bit = UART_PARITY_BIT_EVEN;
        parity_bit = 2;
    }
    else
    {
        config.uart_parity_bit = UART_PARITY_BIT_NONE;
        parity_bit = 0;
    }
    drv_uart_open(uart_num, &config);
    
    int reg_stop_bit = uart_alias_stopbit(config.uart_data_bits,config.uart_stop_bits);
    if(reg_stop_bit == -1)
    {
        config.uart_stop_bits = UART_STOP_BITS_1;
    }
    else
    {
        config.uart_stop_bits = reg_stop_bit;
    }
    drv_uart_set_lineControl(uart_base_reg, config.uart_data_bits, parity_bit, reg_stop_bit, 0);
    drv_uart_set_baudrate(uart_base_reg, config.uart_baud_rate);
    drv_uart_set_flowcontrol(uart_base_reg, config.uart_flow_ctrl);
	
    dce = dce_init(COMMAND_LINE_LENGTH);
    command_queue = os_queue_create("queue_at", 1, sizeof(dce_t), 0);
    dce_register_basic_commands(dce);
    dce_register_wifi_commands(dce);
    dce_register_tcpip_commands(dce);
    dce_register_ota_commands(dce);
    #if defined(CONFIG_ECR_BLE)
    dce_register_ble_commands(dce);
    #endif
    #if defined(CONFIG_MQTT)
    dce_register_mqtt_commands(dce);
    #endif
    init_done();
	int va=0;int ret=0;
	ret=develop_get_env_blob("BleAdvMode", &va, sizeof(va),NULL);
	if (ret==0xffffffff || ret==0){
	develop_set_env_blob("BleAdvMode",&va,sizeof(va));}

    callback.uart_callback = cli_uart_isr;
    callback.uart_data     = (void *)dce;
    drv_uart_ioctrl(uart_num, DRV_UART_CTRL_REGISTER_RX_CALLBACK, &callback);
    drv_uart_ioctrl(uart_num, DRV_UART_CTRL_GET_REG_BASE, (void *)(&(dce->at_uart_base)));

    os_task_create("AT_PROCESS", 5, 8192, (task_entry_t)at_process_task, NULL);
    //xTaskCreate(nv_read, "AT_NV_READ", (512), NULL,5, NULL);
}

void at_emit_basic_result(at_result_code_t result)
{
    dce_emit_basic_result_code(dce, result);
}

void at_emit_information_response(const char* response, size_t size)
{
    dce_emit_information_response(dce, response, size);
}

void at_emit_extended_result_code(const char* response, size_t size, int reset_command_pending)
{
    dce_emit_extended_result_code(dce, response, size, reset_command_pending);
}

void at_emit_extended_result_code_with_args(const char* command_name,\
                                            size_t size, const at_arg_t* args, \
                                            size_t argc, int reset_command_pending, \
                                            bool arg_in_brackets)
{
    dce_emit_extended_result_code_with_args(dce, command_name, size, (const arg_t*)args, argc, reset_command_pending, arg_in_brackets);
}

void at_register_commands(const at_command_list* list,int cmd_num)
{
    dce_register_command_group(dce, "\0", (const command_desc_t*)list, cmd_num, 0);
}

#ifndef __OS_TASK_CONFIG_H__
#define __OS_TASK_CONFIG_H__


#define configMAX_PRIORITIES     ( 12 )
enum
{
#ifndef CONFIG_CUSTOM_FHOSTAPD
    /// Priority of the WiFi task
    FHOST_WIFI_HIGH_PRIORITY = (configMAX_PRIORITIES - 3),
    /// Priority of the WiFi task
    FHOST_WIFI_PRIORITY = (configMAX_PRIORITIES - 6),
#else
    /// Priority of the WiFi task  //tuya lwip priority 9
    FHOST_WIFI_HIGH_PRIORITY = (configMAX_PRIORITIES - 2),
    /// Priority of the WiFi task
    FHOST_WIFI_PRIORITY = (configMAX_PRIORITIES - 5),
#endif
    /// Priority of the WPA task
    FHOST_WPA_PRIORITY = (configMAX_PRIORITIES - 7),
    /// Priority of the TG send task
    //FHOST_TG_SEND_PRIORITY = (configMAX_PRIORITIES - 5),
    // Priority of the TCP/IP task
    FHOST_TCPIP_PRIORITY = (configMAX_PRIORITIES - 4),
    // Priority of system event loop task
    SYSTEM_EVENT_LOOP_PRIORITY = (configMAX_PRIORITIES - 6),
    ///Priority of the cli-task
    FHOST_CLI_PRIORITY = (configMAX_PRIORITIES - 4),
    /// Priority of data test task, this task create by cli cmd
    FHOST_DATA_TEST_PRIORITY = (configMAX_PRIORITIES - 9),
    //BLE Controller
    BLE_CONTROLLER_PRIORITY = (configMAX_PRIORITIES - 1),
    //BLE Host
    BLE_HOST_PRIORITY = (configMAX_PRIORITIES - 3),
    //BLE APPS
    BLE_APPS_PRIORITY = (configMAX_PRIORITIES - 10),
    //tuya APP
    TUTA_APP_TASK_PRIORITY = configMAX_PRIORITIES - 5,
    BLE_CTRL_PRIORITY = (configMAX_PRIORITIES - 6),
#ifdef CONFIG_WIRELESS_IPERF_3
    LWIP_IPERF_TASK_PRIORITY = (configMAX_PRIORITIES - 5),
    //iperf client task priority
    LWIP_IPERF_TASK_C_PRIORITY = (configMAX_PRIORITIES - 7),
#endif
	TELNET_TASK_PRIOPRITY=(configMAX_PRIORITIES - 8),

	TELNET_LOG_TASK_PRIOPRITY= (configMAX_PRIORITIES - 11),
	
	BLE_TRACE_TASK_PRIOPRITY= (configMAX_PRIORITIES - 11),
};

/// Definitions of the different FHOST task stack size requirements
enum
{
    /// WiFi task stack size
    #ifndef CONFIG_CUSTOM_FHOSTAPD
    FHOST_WIFI_STACK_SIZE = 2048,
    #else
    FHOST_WIFI_STACK_SIZE = 4096,
    #endif
    /// TCP/IP task stack size
    FHOST_TCPIP_STACK_SIZE = 2048,
    /// WPA task stack size
    FHOST_WPA_STACK_SIZE = 10240,
    /// TG send stack size
    FHOST_TG_SEND_STACK_SIZE = 1024,
    /// Data test stack size 
    FHOST_DATA_TEST_STACK_SIZE = 4096,
    /// Cli task  stack size
    #ifndef CONFIG_CUSTOM_FHOSTAPD
    FHOST_CLI_STACK_SIZE = 4096,
    #else
    FHOST_CLI_STACK_SIZE = 4096,
	#endif
    // BLE stack
    BLE_STACK_SIZE = (4*1024),
    // BLE net config app
    BLE_NETCFG_STACK_SIZE = (2*1024),
    // JD BLE net config app
    JD_BLE_NETCFG_STACK_SIZE = (3*1024),
    // BLE APPS
    BLE_APPS_STACK_SIZE = (2*1024),
    // BLE ctrl itf
    BLE_CTRL_STACK_SIZE = (2*1024),
#ifdef CONFIG_WIRELESS_IPERF_3
    LWIP_IPERF_TASK_STACK_SIZE = 4096,
#endif
	TELNET_TASK_STACK_SIZE=2048,
	TELNET_LOG_TASK_STACK_SIZE=2048,
	BLE_TRACE_TASK_STACK_SIZE=2048,
};

#endif

#ifndef __JD_NETCFG_H__
#define __JD_NETCFG_H__

#include "event_groups.h"
#include "bluetooth.h"
EventGroupHandle_t EventGroupHandler; //事件标志组句柄  
#define EVENTBIT_0 (1<<0) //事件位 ble 配网使能
#define EVENTBIT_1 (1<<1) //ble 获得ssid，passwd，并联网

void jd_start_advertising(void);

#endif


#ifndef AT_OTA_H
#define AT_OTA_H


#include "dce.h"
#include "dce_commands.h"

// #include "freeRTOS.h"
#include "FreeRTOS.h"
#include "task.h"
#include "portmacro.h"
#include "queue.h"
#include "projdefs.h"
#include "system_def.h"
#include "system_wifi_def.h"
#include "system_wifi.h"
//#include "util_cmd.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "stdio.h"
#include "event_groups.h"

#include "ota.h"

#define OTA_SERVER_TASK_PRIO                    8

#define OTA_SERVER_TASK_STACK_SIZE              (10 * 1024)
#define OTA_SERVER_TASK_STACK_DEPTH             (OTA_SERVER_TASK_STACK_SIZE / sizeof(StackType_t))

#define OTA_SERVER_TASK_NAME                    "ota_server_task"

typedef struct 
{
    char*                               task_name;          // 任务名
    TaskHandle_t                        task_handle;        // 任务句柄
    TaskFunction_t                      task_main;          // 入口函数
    configSTACK_DEPTH_TYPE              stack_depth;        // 堆栈大小
    void*                               task_parameters;    // 入口函数形参
    UBaseType_t                         task_prio;          // 任务优先级

    char *                              dw_url[128];
    int                                 dw_state;
}ota_server_task_data_t;
typedef ota_server_task_data_t* ota_server_task_data;

void dce_register_ota_commands(dce_t* dce);

#endif

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __HTTP_OS_H__
#define __HTTP_OS_H__

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <unistd.h>
#include <stdint.h>
#include "oshal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OS_SUCCESS 0
#define OS_FAIL    -1
#define ERR_NO_MEM              0x101
#define ERR_INVALID_ARG         0x102
#define ERR_INVALID_STATE       0x103
#define ERR_INVALID_SIZE        0x104
#define ERR_NOT_FOUND           0x105
#define ERR_NOT_SUPPORTED       0x106
#define ERR_TIMEOUT             0x107
#define ERR_INVALID_RESPONSE    0x108
#define ERR_INVALID_CRC         0x109
#define ERR_INVALID_VERSION     0x10A
#define ERR_INVALID_MAC         0x10B

#define ERR_WIFI_BASE           0x3000
#define ERR_MESH_BASE           0x4000
#define ERR_FLASH_BASE          0x6000
#define ERR_HW_CRYPTO_BASE      0xc000

#define ERR_HTTPD_BASE              (0xb000)
#define ERR_HTTPD_HANDLERS_FULL     (ERR_HTTPD_BASE +  1)
#define ERR_HTTPD_HANDLER_EXISTS    (ERR_HTTPD_BASE +  2)
#define ERR_HTTPD_INVALID_REQ       (ERR_HTTPD_BASE +  3)
#define ERR_HTTPD_RESULT_TRUNC      (ERR_HTTPD_BASE +  4)
#define ERR_HTTPD_RESP_HDR          (ERR_HTTPD_BASE +  5)
#define ERR_HTTPD_RESP_SEND         (ERR_HTTPD_BASE +  6)
#define ERR_HTTPD_ALLOC_MEM         (ERR_HTTPD_BASE +  7)
#define ERR_HTTPD_TASK              (ERR_HTTPD_BASE +  8)

typedef TaskHandle_t othread_t;
/*
	TaskHandle_t xTaskCreateStaticPinnedToCore(	TaskFunction_t pvTaskCode,
												const char * const pcName,
												const uint32_t ulStackDepth,
												void * const pvParameters,
												UBaseType_t uxPriority,
												StackType_t * const pxStackBuffer,
												StaticTask_t * const pxTaskBuffer,
												const BaseType_t xCoreID );

    BaseType_t xTaskCreate( TaskFunction_t pxTaskCode,
                            const char * const pcName,
                            const configSTACK_DEPTH_TYPE usStackDepth,
                            void * const pvParameters,
                            UBaseType_t uxPriority,
                            TaskHandle_t * const pxCreatedTask )

*/
static inline int httpd_os_thread_create(othread_t *thread,
                                 const char *name, uint16_t stacksize, int prio,
                                 void (*thread_routine)(void *arg), void *arg,
                                 BaseType_t core_id)
{
    //int ret = xTaskCreatePinnedToCore(thread_routine, name, stacksize, arg, prio, thread, core_id);
    int ret = xTaskCreate(thread_routine, name, stacksize, arg, prio, thread);
    if (ret == pdPASS) {
        return OS_SUCCESS;
    }
    return OS_FAIL;
}

/* Only self delete is supported */
static inline void httpd_os_thread_delete(void)
{
    vTaskDelete(xTaskGetCurrentTaskHandle());
}

static inline void httpd_os_thread_sleep(int msecs)
{
    vTaskDelay(msecs / portTICK_RATE_MS);
}

static inline othread_t httpd_os_thread_handle(void)
{
    return xTaskGetCurrentTaskHandle();
}

#ifdef __cplusplus
}
#endif

#endif /* ! _OSAL_H_ */

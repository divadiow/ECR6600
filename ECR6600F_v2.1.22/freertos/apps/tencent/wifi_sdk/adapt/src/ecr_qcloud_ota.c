/****************************************************************************
  * apps/tencent/wifi_sdk/adapt/src/ecr_qcloud_ota.c
 *
 *   Copyright (C) 2021 ESWIN. All rights reserved.
 *   Author:  WenchengCao <caowencheng@eswin.com>
 *   Revision history:
 *   2021.08.25 - Initial version.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ecr_qcloud_ota.h"
#include "qcloud_iot_import.h"
#include "utils_param_check.h"
#include "utils_param_check.h"
#include "ecr_qcloud_led.h"
#include "hal/hal_system.h"
#include <ota/ota.h>
#include "ecr_qcloud_iot.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct _Ecr_OTAHandle {
    int  partition;
    int handle;
} Ecr_OTAHandle;

typedef struct OTAContextData {
    void *ota_handle;
    void *mqtt_client;
    // remote_version means version for the FW in the cloud and to be downloaded
    char     remote_version[MAX_SIZE_OF_FW_VERSION];
    uint32_t fw_file_size;
    // for resuming download
    char     downloading_version[MAX_SIZE_OF_FW_VERSION];
    uint32_t downloaded_size;
    uint32_t ota_fail_cnt;
    Ecr_OTAHandle *ecr__ota;
    TaskHandle_t task_handle;
    char         local_version[MAX_SIZE_OF_FW_VERSION];
} OTAContextData;


/****************************************************************************
 * Public Data
 ****************************************************************************/

bool           g_fw_downloading   = false;
bool           g_ota_task_running = false;
OTAContextData sg_ota_ctx         = {0};
e_Ota_Type     ota_state_type     = OTA_STATE_INIT;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name:  ecr_ota_memory_write
 * Description:
 * @brief
 * @param[in]   buf: data that ota written to flash
 * @param[in]   len: length of buf
 * @return
 ****************************************************************************/

static int ecr_ota_memory_write(char *buf, int len)
{
    if (ota_write((unsigned char *)buf,len) != 0)
    {
        return QCLOUD_ERR_FAILURE;
    }
    else
    {
        return 0;
    }
}

/****************************************************************************
 * Name:  ecr_ota_board_init
 * Description:
 * @brief      init ota get local update
 * @return  0 when success, or err code for failure
 ****************************************************************************/

static int ecr_ota_board_init(void)
{
    return ota_init();
}

/****************************************************************************
 * Name:  _pre_ota_download
 * Description:
 * @brief       ota init
 * @param[in]   ota_ctx: OTA context data
 * @return  0 when success, or err code for failure
 ****************************************************************************/

static int _pre_ota_download(OTAContextData *ota_ctx)
{
#ifdef SUPPORT_RESUMING_DOWNLOAD
    // re-generate MD5 for resuming download */
    if (ota_ctx->downloaded_size
            && strncmp(ota_ctx->remote_version, ota_ctx->downloading_version, MAX_SIZE_OF_FW_VERSION) == 0) {
        Log_i("----------setup local MD5 with offset: %d for version %s-----------\n", ota_ctx->downloaded_size, ota_ctx->remote_version);
        int ret = _cal_exist_fw_md5(ota_ctx);
        if (ret) {
            Log_e("regen OTA MD5 error: %d\n", ret);
            ota_ctx->downloaded_size = 0;
            return -1;
        }
        Log_i("local MD5 update done!\n");
        return 0;
    }
#endif /* SUPPORT_RESUMING_DOWNLOAD */
    // new download, erase partition first
    ota_ctx->downloaded_size = 0;
    if (ecr_ota_board_init()) {
        Log_e("init ecr_ ota failed\n");
        return QCLOUD_ERR_FAILURE;
    }
    return 0;
}

/****************************************************************************
 * Name:  _post_ota_download
 * Description:
 * @brief       do ota update, but device not restart
 * @param[in]   ota_ctx: OTA context data
 * @return  0 when success, or err code for failure
 ****************************************************************************/

static int _post_ota_download(OTAContextData *ota_ctx)
{
    return ota_done(0);
}

/****************************************************************************
 * Name:  ecr_ota_system_reset
 * Description:
 * @brief       device system reset
 ****************************************************************************/

static void ecr_ota_system_reset(void)
{
    hal_reset_type_init();
    hal_system_reset(RST_TYPE_OTA);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

bool is_fw_downloading(void)
{
    return g_fw_downloading;
}

/****************************************************************************
 * Name:  ecr_qcloud_ota_ctrl
 * Description:
 * @brief       enable qcloud ota task
 * @param[in]   client_chan: mqtt client
 * @return  0: success, -1: failure
 ****************************************************************************/

int ecr_qcloud_ota_ctrl(Qcloud_IoT_Template *client_chan)
{
    int             rc = 0;
    rc = enable_ota_task(&sg_devInfo, IOT_Template_Get_MQTT_Client(client_chan), __RELEASE_VERSION);
    if (rc)
    {
        Log_e("Start OTA task failed!\n");
        return -1;
    }
    return 0;
}

/****************************************************************************
 * Name:  enable_ota_task
 * Description:
 * @brief       enable ota task
 * @param[in]   dev_info: device propery information
 * @param[in]   mqtt_client: mqtt client
 * @param[in]   version: device version information
 * @return  0 when success, or err code for failure
 ****************************************************************************/

int enable_ota_task(DeviceInfo *dev_info, void *mqtt_client, char *version)
{
    POINTER_SANITY_CHECK(dev_info, QCLOUD_ERR_INVAL);
    POINTER_SANITY_CHECK(mqtt_client, QCLOUD_ERR_INVAL);
    POINTER_SANITY_CHECK(version, QCLOUD_ERR_INVAL);
    g_ota_task_running = true;
    if (sg_ota_ctx.ota_handle == NULL) {
        void *ota_handle  = IOT_OTA_Init(dev_info->product_id, dev_info->device_name, mqtt_client);
        if (NULL == ota_handle) {
            Log_e("initialize OTA failed\n");
            return QCLOUD_ERR_FAILURE;
        }
        memset(&sg_ota_ctx, 0, sizeof(sg_ota_ctx));
        sg_ota_ctx.mqtt_client = mqtt_client;
        sg_ota_ctx.ota_handle  = ota_handle;
        int ret = xTaskCreate(ecr_ota_update_task, OTA_CLIENT_TASK_NAME, OTA_CLIENT_TASK_STACK_BYTES, (void *)&sg_ota_ctx,
                              OTA_CLIENT_TASK_PRIO, &sg_ota_ctx.task_handle);
        if (ret != pdPASS) {
            Log_e("create ota task failed: %d\n", ret);
            IOT_OTA_Destroy(sg_ota_ctx.ota_handle);
            memset(&sg_ota_ctx, 0, sizeof(sg_ota_ctx));
            return QCLOUD_ERR_FAILURE;
        }
    }
    /* report current user version */
    if (0 > IOT_OTA_ReportVersion(sg_ota_ctx.ota_handle, version)) {
        Log_e("report OTA version %s failed\n", version);
        g_ota_task_running = false;
        HAL_SleepMs(1000);
        IOT_OTA_Destroy(sg_ota_ctx.ota_handle);
        memset(&sg_ota_ctx, 0, sizeof(sg_ota_ctx));
        return QCLOUD_ERR_FAILURE;
    }
    memset(sg_ota_ctx.local_version, 0, MAX_SIZE_OF_FW_VERSION);
    strncpy(sg_ota_ctx.local_version, version, strlen(version));
    return 0;
}

/****************************************************************************
 * Name:  disable_ota_task
 * Description:
 * @brief       stop ota task
 * @return  0: success
 ****************************************************************************/

int disable_ota_task(void)
{
    if (g_ota_task_running) {
        g_ota_task_running = false;
        Log_w(">>>>>>>>>> Disable OTA upgrade!\n");
        do {
            HAL_SleepMs(1000);
        } while (is_fw_downloading());
        HAL_SleepMs(500);
    }
    return 0;
}

/****************************************************************************
 * Name:  ecr_ota_update_task
 * Description:
 * @brief       main OTA cycle
 * @param[in]   pvParameters: OTA context data
 ****************************************************************************/

void ecr_ota_update_task(void *pvParameters)
{
    OTAContextData *ota_ctx               = (OTAContextData *)pvParameters;
    bool            upgrade_fetch_success = true;
    char *          buf_ota               = NULL;
    int             rc;
    void *          h_ota                 = ota_ctx->ota_handle;
    int             mqtt_disconnect_cnt   = 0;
    Ecr_OTAHandle   ecr__ota              = {0};
    bool            reset_ota             = false;
    if (h_ota == NULL) {
        Log_e("mqtt ota not ready\n");
        ota_state_type = OTA_NULL_FAILED;
        goto end_of_ota;
    }
    ota_ctx->ecr__ota = &ecr__ota;

begin_of_ota:
    while (g_ota_task_running) {
        // recv the upgrade cmd
        if (IOT_OTA_IsFetching(h_ota)) {
            g_fw_downloading = true;
            Log_i(">>>>>>>>>> start firmware download!\n");
            IOT_OTA_Ioctl(h_ota, IOT_OTAG_FILE_SIZE, &ota_ctx->fw_file_size, 4);
            memset(ota_ctx->remote_version, 0, MAX_SIZE_OF_FW_VERSION);
            IOT_OTA_Ioctl(h_ota, IOT_OTAG_VERSION, ota_ctx->remote_version, MAX_SIZE_OF_FW_VERSION);
            rc = _pre_ota_download(ota_ctx);
            if (rc) {
                ota_state_type = OTA_PRE_FAILED;
                Log_e("pre ota download failed: %d\n", rc);
                upgrade_fetch_success = false;
                goto end_of_ota;
            }
            buf_ota = HAL_Malloc(ECR_OTA_BUF_LEN + 1);
            if (buf_ota == NULL) {
                ota_state_type = OTA_MALLOC_FAILED;
                Log_e("malloc ota buffer failed\n");
                upgrade_fetch_success = false;
                goto end_of_ota;
            }
            /*set offset and start http connect*/
            rc = IOT_OTA_StartDownload(h_ota, ota_ctx->downloaded_size, ota_ctx->fw_file_size);
            if (QCLOUD_RET_SUCCESS != rc) {
                ota_state_type = OTA_START_DOWNLODE_FAILED;
                Log_e("OTA download start err,rc:%d\n", rc);
                upgrade_fetch_success = false;
                goto end_of_ota;
            }
            // download and save the fw
            while (!IOT_OTA_IsFetchFinish(h_ota)) {
                if (!g_ota_task_running) {
                    ota_state_type = OTA_STOP_TRAN;
                    Log_e("OTA task stopped during downloading!\n");
                    upgrade_fetch_success = false;
                    goto end_of_ota;
                }
                memset(buf_ota, 0, ECR_OTA_BUF_LEN + 1);
                int len = IOT_OTA_FetchYield(h_ota, buf_ota, ECR_OTA_BUF_LEN + 1, 20);
                if (len > 0) {
                    // Log_i("save fw data %d from addr %u", len, ota_ctx->downloaded_size);
                    rc = ecr_ota_memory_write(buf_ota, len);
                    if (rc) {
                        ota_state_type = OTA_SAVE_DATE_FAILED;
                        Log_e("write data to file failed\n");
                        upgrade_fetch_success = false;
                        goto end_of_ota;
                    }
                } else if (len < 0) {
                    ota_state_type = OTA_REVEICE_FAILED;
                    Log_e("download fail rc=%d, size_downloaded=%u\n", len, ota_ctx->downloaded_size);
                    upgrade_fetch_success = false;
                    goto end_of_ota;
                } else {
                    ota_state_type = OTA_REVEICE_TIMEOUT;
                    Log_e("OTA download timeout! size_downloaded=%u\n", ota_ctx->downloaded_size);
                    upgrade_fetch_success = false;
                    goto end_of_ota;
                }
                // get OTA downloaded size
                IOT_OTA_Ioctl(h_ota, IOT_OTAG_FETCHED_SIZE, &ota_ctx->downloaded_size, 4);
                // delay is needed to avoid TCP read timeout
                HAL_SleepMs(500);
                if (!IOT_MQTT_IsConnected(ota_ctx->mqtt_client)) {
                    mqtt_disconnect_cnt++;
                    Log_e("MQTT disconnect %d during OTA download!\n", mqtt_disconnect_cnt);
                    if (mqtt_disconnect_cnt > 3) {
                        ota_state_type = OTA_DISCONNECT;
                        upgrade_fetch_success = false;
                        goto end_of_ota;
                    }
                    HAL_SleepMs(2000);
                } else {
                    mqtt_disconnect_cnt = 0;
                }
            }
            /* Must check MD5 match or not */
            if (upgrade_fetch_success) {
                uint32_t firmware_valid = 0;
                IOT_OTA_Ioctl(h_ota, IOT_OTAG_CHECK_FIRMWARE, &firmware_valid, 4);
                if (0 == firmware_valid) {
                    ota_state_type = OTA_FIRMWARE_FAILED;
                    Log_e("The firmware is invalid\n");
                    ota_ctx->downloaded_size = 0;
                    upgrade_fetch_success = false;
                    // don't retry for this error
                    ota_ctx->ota_fail_cnt = MAX_OTA_RETRY_CNT + 1;
                    goto end_of_ota;
                } else {
                    Log_i("The firmware is valid\n");
                    rc = _post_ota_download(ota_ctx);
                    if (rc) {
                        ota_state_type = OTA_POST_DATA_FAILED;
                        Log_e("post ota handling failed: %d\n", rc);
                        upgrade_fetch_success = false;
                        // don't retry for this error
                        ota_ctx->ota_fail_cnt = MAX_OTA_RETRY_CNT + 1;
                        goto end_of_ota;
                    }
                    upgrade_fetch_success = true;
                    ota_state_type = OTA_SUCCESS;
                    break;
                }
            }
        } else if (IOT_OTA_GetLastError(h_ota)) {
            Log_e("OTA update failed! last error: %d\n", IOT_OTA_GetLastError(h_ota));
            upgrade_fetch_success = false;
            goto end_of_ota;
        }
        HAL_SleepMs(1000);
    }
end_of_ota:
    if (!upgrade_fetch_success && g_fw_downloading) {
        IOT_OTA_ReportUpgradeFail(h_ota, NULL);
        ota_ctx->ota_fail_cnt++;
    }
    // do it again
    if (g_ota_task_running && IOT_MQTT_IsConnected(ota_ctx->mqtt_client) && !upgrade_fetch_success &&
        ota_ctx->ota_fail_cnt <= MAX_OTA_RETRY_CNT) {
        HAL_Free(buf_ota);
        buf_ota               = NULL;
        g_fw_downloading      = false;
        upgrade_fetch_success = true;
        Log_e("OTA failed! downloaded %u. retry %d time!\n", ota_ctx->downloaded_size, ota_ctx->ota_fail_cnt);
        HAL_SleepMs(1000);
        if (0 > IOT_OTA_ReportVersion(ota_ctx->ota_handle, ota_ctx->local_version)) {
            Log_e("report OTA version %s failed\n", ota_ctx->local_version);
        }
#ifdef SUPPORT_RESUMING_DOWNLOAD
        // resuming download
        if (ota_ctx->downloaded_size) {
            memset(ota_ctx->downloading_version, 0, MAX_SIZE_OF_FW_VERSION);
            strncpy(ota_ctx->downloading_version, ota_ctx->remote_version, MAX_SIZE_OF_FW_VERSION);
        }
#else
        // fresh restart
        ota_ctx->downloaded_size = 0;
#endif  /* SUPPORT_RESUMING_DOWNLOAD */
        ota_state_type = OTA_STATE_INIT;
        goto begin_of_ota;
    }
    else
    {
        if(!IOT_MQTT_IsConnected(ota_ctx->mqtt_client))
        {
            ota_state_type = OTA_DISCONNECT;
        }
    }


    while(ota_state_type == OTA_DISCONNECT)
    {
        if(!IOT_MQTT_IsConnected(ota_ctx->mqtt_client))
        {
            Log_e("--------wait connect------\n");
            HAL_SleepMs(1000);
        }
        else
        {
            Log_i("--------connect success------\n");
            HAL_Free(buf_ota);
            buf_ota               = NULL;
            g_fw_downloading      = false;
            upgrade_fetch_success = true;
            ota_ctx->downloaded_size = 0;
            ota_state_type = OTA_STATE_INIT;
            ota_ctx->ota_fail_cnt = 0;
            goto begin_of_ota;
        }
    }
    if (upgrade_fetch_success && g_fw_downloading) {
        IOT_OTA_ReportUpgradeSuccess(h_ota, NULL);
        Log_w("OTA update success! Reboot after a while\n");
        HAL_SleepMs(2000);
        reset_ota = true;
    } else if (g_fw_downloading) {
        Log_e("OTA failed! Downloaded: %u. Quit the task and reset\n", ota_ctx->downloaded_size);
    }
    g_fw_downloading = false;
    Log_w(">>>>>>>>>> OTA task going to be deleted\n");
    if (buf_ota) {
        HAL_Free(buf_ota);
        buf_ota = NULL;
    }
    IOT_OTA_Destroy(ota_ctx->ota_handle);
    memset(ota_ctx, 0, sizeof(OTAContextData));
    if (reset_ota)
    {
        ecr_ota_system_reset();
    }
    vTaskDelete(NULL);
    return;
}

#ifdef SUPPORT_RESUMING_DOWNLOAD

/****************************************************************************
 * Name:  ecr_ota_memory_read
 * Description:
 * @brief       read ota data from flash
 * @param[in]   offset:
 * @param[in]   dst_buf:
 * @param[in]   read_size_bytes:
 * @return
 ****************************************************************************/

int ecr_ota_memory_read(int offset, void *dst_buf, uint32_t read_size_bytes)
{
    /*Only compression upgrade is supported*/
    int ret = 0;
    ret = ota_read(offset, dst_buf, read_size_bytes);
    if(ret == 0)
    {
        return 0;
    }
    Log_e("read data from flash error:offset =%d\n",offset);
    return ret;
}

/****************************************************************************
 * Name:  _cal_exist_fw_md5
 * Description:
 * @brief       calculate left MD5 for resuming download from break point
 * @param[in]   ota_ctx: OTA context data
 * @return  0 when success, or err code for failure
 ****************************************************************************/

static int _cal_exist_fw_md5(OTAContextData *ota_ctx)
{
    char * buff;
    size_t rlen, total_read = 0;
    int    ret = QCLOUD_RET_SUCCESS;
    ret = IOT_OTA_ResetClientMD5(ota_ctx->ota_handle);
    if (ret) {
        Log_e("reset MD5 failed: %d\n", ret);
        return QCLOUD_ERR_FAILURE;
    }
    buff = HAL_Malloc(ECR_OTA_BUF_LEN);
    if (buff == NULL) {
        Log_e("malloc ota buffer failed\n");
        return QCLOUD_ERR_MALLOC;
    }
    size_t size = ota_ctx->downloaded_size;
    while (total_read < ota_ctx->downloaded_size) {
        rlen = (size > ECR_OTA_BUF_LEN) ? ECR_OTA_BUF_LEN : size;
        ret = ecr_ota_memory_read(total_read, buff, rlen);
        if (ret) {
            Log_e("read data from flash error\n");
            ret = QCLOUD_ERR_FAILURE;
            break;
        }
        IOT_OTA_UpdateClientMd5(ota_ctx->ota_handle, buff, rlen);
        size -= rlen;
        total_read += rlen;
    }
    HAL_Free(buff);
    Log_d("total read: %d\n", total_read);
    return ret;
}
#endif /* SUPPORT_RESUMING_DOWNLOAD */

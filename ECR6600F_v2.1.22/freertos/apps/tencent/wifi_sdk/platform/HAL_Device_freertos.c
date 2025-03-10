/*
 * Tencent is pleased to support the open source community by making IoT Hub
 available.
 * Copyright (C) 2018-2020 THL A29 Limited, a Tencent company. All rights
 reserved.

 * Licensed under the MIT License (the "License"); you may not use this file
 except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT

 * Unless required by applicable law or agreed to in writing, software
 distributed under the License is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 KIND,
 * either express or implied. See the License for the specific language
 governing permissions and
 * limitations under the License.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qcloud_iot_export.h"
#include "qcloud_iot_import.h"
#include "utils_param_check.h"
#include "ecr_qcloud_iot.h"
#include "easyflash.h"

/* Enable this macro (also control by cmake) to use static string buffer to
 * store device info */
/* To use specific storing methods like files/flash, disable this macro and
 * implement dedicated methods */


static char sg_product_id[MAX_SIZE_OF_PRODUCT_ID + 1] ={0};
static char sg_device_name[MAX_SIZE_OF_DEVICE_NAME + 1]={0};
static char sg_device_secret[MAX_SIZE_OF_DEVICE_SECRET + 1]={0};

#ifdef AUTH_MODE_CERT
static char sg_device_cert_file_name[MAX_SIZE_OF_DEVICE_CERT_FILE_NAME + 1] ={0};
static char sg_device_privatekey_file_name[MAX_SIZE_OF_DEVICE_SECRET_FILE_NAME + 1]={0};
#endif

/* region */
static char sg_region[MAX_SIZE_OF_REGION + 1] = "china";

#ifdef DEV_DYN_REG_ENABLED
/* product secret for device dynamic Registration  */
static char sg_product_secret[MAX_SIZE_OF_PRODUCT_SECRET + 1] = "YOUR_PRODUCT_SECRET";
#endif
#ifdef GATEWAY_ENABLED
/* sub-device product id  */
static char sg_sub_device_product_id[MAX_SIZE_OF_PRODUCT_ID + 1] = "PRODUCT_ID";
/* sub-device device name */
static char sg_sub_device_name[MAX_SIZE_OF_DEVICE_NAME + 1] = "YOUR_SUB_DEV_NAME";
#endif

void  ten_dev_info_init(void)
{

	int32_t read_len;
	if((read_len=(customer_get_env_blob("sg_product_id",sg_product_id,sizeof(sg_product_id),NULL)))<=0)
	{
		memcpy(sg_product_id,SG_DEVICE_ID,strlen(SG_DEVICE_ID));

	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"sg_product_id=%s,len=%d\n",sg_product_id,read_len);
	}
	
	if((read_len=(customer_get_env_blob("sg_device_name",sg_device_name,sizeof(sg_device_name),NULL)))<=0)
	{
		memcpy(sg_device_name,SG_DEVICE_NAME,strlen(SG_DEVICE_NAME));
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"sg_device_name=%s,len=%d\n",sg_device_name,read_len);
	}
	#ifdef AUTH_MODE_CERT
	/* public cert file name of certificate device */
	memcpy(sg_device_cert_file_name,"YOUR_DEVICE_NAME_cert.crt",strlen("YOUR_DEVICE_NAME_cert.crt"));
	/* private key file name of certificate device */
	memcpy(sg_device_privatekey_file_name,"YOUR_DEVICE_NAME_private.key",strlen("YOUR_DEVICE_NAME_private.key"));
	
	#else
	/* device secret of PSK device */
	if((read_len=(customer_get_env_blob("sg_device_secret",sg_device_secret,sizeof(sg_device_secret),NULL)))<=0)
	{
		memcpy(sg_device_secret,SG_DEVICE_SECRET,strlen(SG_DEVICE_SECRET));
		
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"sg_device_secret=%s,len=%d\n",sg_device_secret,read_len);
	}
	#endif
}

static int device_info_copy(void *pdst, void *psrc, uint8_t max_len)
{
    if (strlen(psrc) > max_len) {
        return QCLOUD_ERR_FAILURE;
    }
    memset(pdst, '\0', max_len);
    strncpy(pdst, psrc, max_len);
    return QCLOUD_RET_SUCCESS;
}


#if 0
int HAL_SetDevInfo(void *pdevInfo)
{
    POINTER_SANITY_CHECK(pdevInfo, QCLOUD_ERR_DEV_INFO);
    int         ret;
    DeviceInfo *devInfo = (DeviceInfo *)pdevInfo;

    ret = device_info_copy(sg_product_id, devInfo->product_id,
                           MAX_SIZE_OF_PRODUCT_ID);  // set product ID
    ret |= device_info_copy(sg_device_name, devInfo->device_name,
                            MAX_SIZE_OF_DEVICE_NAME);  // set dev name

#ifdef AUTH_MODE_CERT
    ret |= device_info_copy(sg_device_cert_file_name, devInfo->dev_cert_file_name,
                            MAX_SIZE_OF_DEVICE_CERT_FILE_NAME);  // set dev cert file name
    ret |= device_info_copy(sg_device_privatekey_file_name, devInfo->dev_key_file_name,
                            MAX_SIZE_OF_DEVICE_SECRET_FILE_NAME);  // set dev key file name
#else
    ret |= device_info_copy(sg_device_secret, devInfo->device_secret,
                            MAX_SIZE_OF_DEVICE_SECRET);  // set dev secret
#endif

    if (QCLOUD_RET_SUCCESS != ret) {
        Log_e("Set device info err");
        ret = QCLOUD_ERR_DEV_INFO;
    }
    return ret;
}
#endif

int HAL_GetDevInfo(void *pdevInfo)
{
    POINTER_SANITY_CHECK(pdevInfo, QCLOUD_ERR_DEV_INFO);
    int         ret;
    DeviceInfo *devInfo = (DeviceInfo *)pdevInfo;
	
	ten_dev_info_init();
    memset((char *)devInfo, '\0', sizeof(DeviceInfo));

    ret = device_info_copy(devInfo->product_id, sg_product_id,
                           MAX_SIZE_OF_PRODUCT_ID);  // get product ID
    ret |= device_info_copy(devInfo->device_name, sg_device_name,
                            MAX_SIZE_OF_DEVICE_NAME);  // get dev name
    ret |= device_info_copy(devInfo->region, sg_region,
                            MAX_SIZE_OF_REGION);  // get region

#ifdef DEV_DYN_REG_ENABLED
    ret |= device_info_copy(devInfo->product_secret, sg_product_secret,
                            MAX_SIZE_OF_PRODUCT_SECRET);  // get product ID
#endif

#ifdef AUTH_MODE_CERT
    ret |= device_info_copy(devInfo->dev_cert_file_name, sg_device_cert_file_name,
                            MAX_SIZE_OF_DEVICE_CERT_FILE_NAME);  // get dev cert file name
    ret |= device_info_copy(devInfo->dev_key_file_name, sg_device_privatekey_file_name,
                            MAX_SIZE_OF_DEVICE_SECRET_FILE_NAME);  // get dev key file name
#else
    ret |= device_info_copy(devInfo->device_secret, sg_device_secret,
                            MAX_SIZE_OF_DEVICE_SECRET);  // get dev secret
#endif


    if (QCLOUD_RET_SUCCESS != ret) {
        Log_e("Get device info err");
        ret = QCLOUD_ERR_DEV_INFO;
    }
    return ret;
}

#ifdef GATEWAY_ENABLED
int HAL_GetGwDevInfo(void *pgwDeviceInfo)
{
    POINTER_SANITY_CHECK(pgwDeviceInfo, QCLOUD_ERR_DEV_INFO);
    int                ret;
    GatewayDeviceInfo *gwDevInfo = (GatewayDeviceInfo *)pgwDeviceInfo;
    memset((char *)gwDevInfo, 0, sizeof(GatewayDeviceInfo));

    ret = HAL_GetDevInfo(&(gwDevInfo->gw_info));  // get gw dev info
    // only one sub-device is supported now
    gwDevInfo->sub_dev_num  = 1;
    gwDevInfo->sub_dev_info = (DeviceInfo *)HAL_Malloc(sizeof(DeviceInfo) * (gwDevInfo->sub_dev_num));
    memset((char *)gwDevInfo->sub_dev_info, '\0', sizeof(DeviceInfo));
    // copy sub dev info
    ret = device_info_copy(gwDevInfo->sub_dev_info->product_id, sg_sub_device_product_id, MAX_SIZE_OF_PRODUCT_ID);
    ret |= device_info_copy(gwDevInfo->sub_dev_info->device_name, sg_sub_device_name, MAX_SIZE_OF_DEVICE_NAME);

    if (QCLOUD_RET_SUCCESS != ret) {
        Log_e("Get gateway device info err");
        ret = QCLOUD_ERR_DEV_INFO;
    }
    return ret;
}
#endif

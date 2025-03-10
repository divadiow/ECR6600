#include "joylink_sdk.h"
#include "joylink_platform.h"
#include "joylink_adapter.h"
#include "joylink_dev_active.h"
#include "joylink_flash.h"
#include "joylink_softap.h"

#include <sys/time.h>
#include <fcntl.h>
extern uint8_t connect_id;

/*************************************************
Function: jl_adapter_send_data
Description: SDK适配接口，GATTS Characteristic发送数据
Calls: GATTS Characteristic发送数据接口
Called By: @jl_send_frame：SDK内部函数
Input: @data：发送的数据
       @data_len：发送的数据长度
Output: None
Return: 0：成功
       -1：失败
Others:
*************************************************/
extern int32_t jl_adapter_send_data(uint8_t* data, uint32_t data_len);


/*************************************************
Function: jl_adapter_set_config_data
Description: SDK适配接口，获取配网与绑定参数
Calls: 配网与绑定接口
Called By: @jl_process_user_data：SDK内部函数
Input: @data->ssid：配网数据，WiFi SSID
       @data->ssid_len：WiFi SSID长度
       @data->password：配网数据，WiFi密码
       @data->password_len：WiFi密码长度
       @data->url：绑定云端链接
       @data->url_len：绑定云端链接长度
       @data->token：绑定参数
       @data->token_len：绑定参数长度
Output: None
Return: 0：成功
       -1：失败
Others: 设置激活绑定接口：joylink_dev_active_start(data->url, data->token);
*************************************************/

int32_t jl_adapter_set_config_data(jl_net_config_data_t *data)
{
    int status = 0;
    jl_platform_printf("ssid: %s\n", data->ssid);
    jl_platform_printf("password: %s\n", data->password);
    jl_platform_printf("url: %s\n", data->url);
    jl_platform_printf("token: %s\n", data->token);
    //to do
    status = jl_flash_SetString("StaSSID", (char *)data->ssid, data->ssid_len);
    if(0 != status)
    {
      jl_platform_printf("save ssid error,errorno=%d\n",status);
    }
    status = jl_flash_SetString("StaPW", (char *)data->password, data->password_len);
    if(0 != status)
    {
      jl_platform_printf("save ssid password error,errorno=%d\n",status);
    }
    jl_softap_enter_sta_mode((char *)data->ssid, (char *)data->password);
    joylink_dev_active_start((char *)data->url, (char *)data->token);
    return status;
}

/*************************************************
Function: jl_adapter_set_system_time
Description: SDK适配接口，设置系统时间
Calls: 设置系统时间接口
Called By: SDK内部函数
Input: @time->second：秒
       @time->usecond：微妙
Output: None
Return: 0：成功
       -1：失败
Others: None
*************************************************/
int32_t jl_adapter_set_system_time(jl_timestamp_t *time)
{
#if 0
    struct timeval now = { .tv_sec = time->second , .tv_usec = time->usecond};
    settimeofday(&now, NULL);

    struct tm *lt;
    lt = localtime(&lt);
    jl_platform_printf("%d/%d/%d %d:%d:%d\n",lt->tm_year+1900, lt->tm_mon, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);
 #endif
    return 0;
}

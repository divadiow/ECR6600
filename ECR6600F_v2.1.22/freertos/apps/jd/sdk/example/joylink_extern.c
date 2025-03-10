/* --------------------------------------------------
 * @brief: 
 *
 * @version: 1.0
 *
 * @date: 08/01/2018
 * 
 * @desc: Functions in this file must be implemented by the device developers when porting the Joylink SDK.
 *
 * --------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "joylink.h"
#include "joylink_extern.h"
#include "joylink_extern_json.h"

#include "joylink_memory.h"
#include "joylink_socket.h"
#include "joylink_string.h"
#include "joylink_stdio.h"
#include "joylink_stdint.h"
#include "joylink_log.h"
#include "joylink_time.h"
#include "joylink_thread.h"
#include "joylink_extern_ota.h"
#include "joylink_extern_led.h"
#include "joylink_flash.h"
#include "sdk_version.h"

#if defined(CONFIG_JD_FREERTOS_PAL)
#include <wifi_crtl/system_wifi.h>
#include <wifi_crtl/system_wifi_def.h>
#include <mbedtls/config.h>
#include <mbedtls/platform.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/certs.h>
#endif
#ifdef CONFIG_BLE_JD_ADAPTER
#include "joylink_sdk.h"
#include "jd_netcfg.h"
#endif

jl2_d_idt_t user_idt;
static char jlp_uuid[JLP_UUID_LEN] = {0};
static char jlp_soft_ssid[JLP_SOFT_SSID_LEN] = {0};
static char jlp_cid[JLP_CID_LEN] = {0};
static char jlp_brand[JLP_BRAND_LEN] = {0};
static char jlp_cloud_pub_key[JLP_CLOUD_PUB_KEY_LEN] = {0};
static char jlp_cloud_pri_key[JLP_CLOUD_PRI_KEY_LEN] = {0};

extern int joylink_ota_report_status(int status, int progress, char *desc);
#if defined(JOYLINK_SDK_EXAMPLE_TEST) || defined(CONFIG_JD_FREERTOS_PAL)
user_dev_status_t user_dev =
  {
      .Power = 1,
      .Mode = 0,
      .State = 0,
      .BrightLED_old = 100,
      .BrightLED = 100,
      .Color_old = 0,
      .Color = 0,
  };

void user_dev_set()
{
  user_dev_status_t user_dev_flash = {0};
  int ret_get_power;
  int ret_get_mode;
  int ret_get_bright;
  int ret_get_status;
  int ret_get_color;

  ret_get_power = jl_flash_GetString(USER_DATA_POWER,(char *)&user_dev_flash,sizeof(user_dev_status_t));
  if(ret_get_power)
  {
    user_dev.Power = user_dev_flash.Power;
  }
  ret_get_mode = jl_flash_GetString(USER_DATA_MODE,(char *)&user_dev_flash,sizeof(user_dev_status_t));
  if(ret_get_mode)
  {
    user_dev.Mode = user_dev_flash.Mode;;
  }
  ret_get_bright = jl_flash_GetString(USER_DATA_BRIGHTLED,(char *)&user_dev_flash,sizeof(user_dev_status_t));
  if(ret_get_bright)
  {
    user_dev.BrightLED_old = user_dev_flash.BrightLED;
    user_dev.BrightLED = user_dev_flash.BrightLED;
  }
  ret_get_status = jl_flash_GetString(USER_DATA_STATE,(char *)&user_dev_flash,sizeof(user_dev_status_t));
  if(ret_get_status)
  {
    user_dev.State = user_dev_flash.State;
  }
  ret_get_color = jl_flash_GetString(USER_DATA_COLOR,(char *)&user_dev_flash,sizeof(user_dev_status_t));
  if(ret_get_color)
  {
    user_dev.Color_old = user_dev_flash.Color_old;
    user_dev.Color = user_dev_flash.Color;
  }
}
#else
user_dev_status_t user_dev;
#endif

/*E_JLDEV_TYPE_GW*/
#ifdef _SAVE_FILE_
char  *file = "joylink_info.txt";
#endif

/**
 * @brief: 用以返回一个整型随机数
 *
 * @param: 无
 *
 * @returns: 整型随机数
 */
int
joylink_dev_get_random()
{
    /**
     *FIXME:must to do
     */
#if defined(CONFIG_JD_FREERTOS_PAL)
  return jl_get_random();
#else
  static unsigned long int next = 1;
  next = next *1103515245 + 12345;
  return (int)(next/65536) % (1134);
#endif
}

/**
 * @brief: 返回是否可以访问互联网
 *
 * @returns: E_JL_TRUE 可以访问, E_JL_FALSE 不可访问
 */
E_JLBOOL_t joylink_dev_is_net_ok()
{
    /**
     *FIXME:must to do
     */
#if defined(CONFIG_JD_FREERTOS_PAL)
  int wifi_status = 0;
  struct ip_info if_ip;
  
  memset(&if_ip, 0, sizeof(struct ip_info));
  wifi_status = wifi_get_status(STATION_IF);
  wifi_get_ip_info(STATION_IF, &if_ip);
  
  if((wifi_status == STA_STATUS_CONNECTED) && !(ip_addr_isany_val(if_ip.ip)))
  {
      return E_JL_TRUE;
  }
  else
  {
      return E_JL_FALSE;
  }

#else
  return E_JL_TRUE;
#endif
    
}

/**
 * @brief: 此函数用作通知应用层设备与云端的连接状态.
 *
 * @param: st - 当前连接状态  0-Socket init, 1-Authentication, 2-Heartbeat
 *
 * @returns: 
 */
E_JLRetCode_t
joylink_dev_set_connect_st(int st)
{
  /**
  *FIXME:must to do
  */
  char buff[64] = {0};
  int ret = 0;
  char ota_key[] = "ota_key";
  char flag = 0;
  JLOtaOrder_t orderver;

  jl_platform_memset(&orderver, 0, sizeof(JLOtaOrder_t));
  flag = jl_flash_GetString(ota_key,(char *)&orderver,sizeof(JLOtaOrder_t));
  if(st == 2 && flag)
  {
    if(strcmp(RELEASE_VERSION,orderver.versionname) == 0)
    {
      joylink_ota_report_status(OTA_STATUS_SUCCESS, 100, "ota is ok");
      jl_flash_SetString(ota_key,NULL,sizeof(JLOtaOrder_t));
    }
    else
    {
      if(orderver.versionname != 0)
      {
        jl_platform_printf(">>>>>>>OTA failed ..\n");
        joylink_ota_report_status(OTA_STATUS_FAILURE, 0, "ota is failure");
        jl_flash_SetString(ota_key,NULL,sizeof(JLOtaOrder_t));
      }
    }
  }
  jl_platform_sprintf(buff, "{\"conn_status\":\"%d\"}", st);
  log_info("--set_connect_st:%s\n", buff);

  return ret;
}

/**
 * @brief: 传出激活信息
 *
 * @param[in] message: 激活信息
 * 
 * @returns: 0:设置成功
 */
E_JLRetCode_t joylink_dev_active_message(char *message)
{
  log_info("message = %s", message);
  return 0;
}

/**
 * @brief: 存储JLP(Joylink Parameters)信息,将入参jlp结构中的信息持久化存储,如文件、设备flash等方式
 *
 * @param [in]: jlp-JLP structure pointer
 *
 * @returns: 
 */
E_JLRetCode_t
joylink_dev_set_attr_jlp(JLPInfo_t *jlp)
{
  if(NULL == jlp){
    return E_RET_ERROR;
  }
  /**
  *FIXME:must to do
  *Must save jlp info to flash or files
  */
  int ret = E_RET_ERROR;

#ifdef _SAVE_FILE_
    //Sample code for saving JLP info in a file.
  FILE *outfile;
  outfile = jl_platform_fopen(file, "wb+" );
  jl_platform_fwrite(jlp, sizeof(JLPInfo_t), 1, outfile );
  jl_platform_fclose(outfile);
#elif defined(CONFIG_JD_FREERTOS_PAL)
#ifdef CONFIG_BLE_JD_ADAPTER
  if(EventGroupHandler!=NULL)
  {
    if(xEventGroupGetBits(EventGroupHandler) & (1<<1))
    {
      /*新配网发生，且已获得feedid后发送*/
      if((jlp->feedid[17]) >= '0' && (jlp->feedid[17] <= '9'))
      {
        xEventGroupClearBits(EventGroupHandler,EVENTBIT_1);
        jl_send_net_config_state(E_JL_NET_CONF_ST_IOT_CONNECT_SUCCEED, (uint8_t *)jlp->feedid, jl_platform_strlen(jlp->feedid));
        jl_send_net_config_state(E_JL_NET_CONF_ST_CLOUD_CONNECT_SUCCEED, NULL, 0);
      }
    }
  }
#endif
  char jlp_key[] = "jlp_key";
  ret = jl_flash_SetString(jlp_key,(char *)jlp,sizeof(JLPInfo_t));
  return ret;
#endif
  ret = E_RET_OK;

  return ret;
}

/**
 * @brief: 设置设备认证信息
 *
 * @param[out]: pidt--设备认证信息结构体指针,需填入必要信息sig,pub_key,f_sig,f_pub_key,cloud_pub_key
 *
 * @returns: 返回设置成功或失败
 */
E_JLRetCode_t
joylink_dev_get_idt(jl2_d_idt_t *pidt)
{
  if(NULL == pidt){
    return E_RET_ERROR; 
  }
  pidt->type = 0;
  /**
  *FIXME:must to do
  */
  jl_platform_strcpy(pidt->sig, user_idt.sig);
  jl_platform_strcpy(pidt->pub_key, user_idt.pub_key);
  //jl_platform_strcpy(pidt->rand, user_idt.rand);
  jl_platform_strcpy(pidt->f_sig, user_idt.f_sig);
  jl_platform_strcpy(pidt->f_pub_key, user_idt.f_pub_key);
  jl_platform_strcpy(pidt->cloud_pub_key, user_idt.cloud_pub_key);

  return E_RET_OK;
}

/**
 * @brief: 此函数需返回设备的MAC地址
 *
 * @param[out] out: 将设备MAC地址赋值给此参数
 * 
 * @returns: E_RET_OK 成功, E_RET_ERROR 失败
 */
E_JLRetCode_t
joylink_dev_get_user_mac(char *out)
{
  /**
  *FIXME:must to do
  */
#ifdef JOYLINK_SDK_EXAMPLE_TEST
  jl_platform_strcpy(out, "AABBCCDDEE01");
  return E_RET_OK;
#elif defined(CONFIG_JD_FREERTOS_PAL)
  uint8_t mac_addr[6]={0};
  int ret = E_RET_ERROR;
  ret = wifi_get_mac_addr(STATION_IF, mac_addr);
  if(ret == 0)
  {
    jl_platform_sprintf(out, "%02X%02X%02X%02X%02X%02X",
            mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    return E_RET_OK;
  }
  else
  {
    return E_RET_ERROR;
  }
#else
  return -1;
#endif
}

/**
 * @brief: 此函数需返回设备私钥private key,该私钥可从小京鱼后台获取
 *
 * @param[out] out: 将私钥字符串赋值给该参数返回
 * 
 * @returns: E_RET_OK:成功, E_RET_ERROR:发生错误
 */
E_JLRetCode_t
joylink_dev_get_private_key(char *out)
{
  /**
  *FIXME:must to do
  */
#if 0
#ifdef JOYLINK_SDK_EXAMPLE_TEST  
  jl_platform_strcpy(out, IDT_CLOUD_PRI_KEY);
  return E_RET_OK;
#elif defined(CONFIG_JD_FREERTOS_PAL)
  #if defined(CONFIG_BLE_JD_ADAPTER)
      jl_platform_strcpy(out, IDT_CLOUD_PRI_KEY); 
      return E_RET_OK;
  #endif
  jl_platform_strcpy(out, IDT_CLOUD_PRI_KEY); 
  return E_RET_OK;
#else
  return E_RET_ERROR;
#endif
#endif
#ifdef JOYLINK_SDK_EXAMPLE_TEST  
  jl_platform_strcpy(out, jlp_cloud_pri_key);
  return E_RET_OK;
#elif defined(CONFIG_JD_FREERTOS_PAL)
  #if defined(CONFIG_BLE_JD_ADAPTER)
    jl_platform_strcpy(out, jlp_cloud_pri_key);
    return E_RET_OK;
  #endif
    jl_platform_strcpy(out, jlp_cloud_pri_key);
    return E_RET_OK;
#else
  return E_RET_ERROR;
#endif
}
/**k
 * @brief: 从永久存储介质(文件或flash)中读取jlp信息,并赋值给参数jlp,其中feedid, accesskey,localkey,joylink_server,server_port必须正确赋值
 *
 * @param[out] jlp: 将JLP(Joylink Parameters)读入内存,并赋值给该参数
 *
 * @returns: E_RET_OK:成功, E_RET_ERROR:发生错误
 */
E_JLRetCode_t
joylink_dev_get_jlp_info(JLPInfo_t *jlp)
{
  if(NULL == jlp){
    return E_RET_ERROR;
  }
  /**
  *FIXME:must to do
   *Must get jlp info from flash 
  */
  int ret = E_RET_OK;

#ifdef _SAVE_FILE_
JLPInfo_t fjlp;
jl_platform_memset(&fjlp, 0, sizeof(JLPInfo_t));

FILE *infile;
infile = jl_platform_fopen(file, "rb+");
if(infile > 0)
{
  jl_platform_fread(&fjlp, sizeof(fjlp), 1, infile);
  jl_platform_fclose(infile);
}
#elif defined(CONFIG_JD_FREERTOS_PAL)
  JLPInfo_t fjlp;
  char jlp_key[] = "jlp_key";
  jl_platform_memset(&fjlp, 0, sizeof(JLPInfo_t));
  ret = jl_flash_GetString(jlp_key,  (char *)&fjlp, sizeof(JLPInfo_t));
#endif

#if defined(CONFIG_JD_FREERTOS_PAL) || defined(_SAVE_FILE_)
  jl_platform_strcpy(jlp->feedid, fjlp.feedid);
  jl_platform_strcpy(jlp->accesskey, fjlp.accesskey);
  jl_platform_strcpy(jlp->localkey, fjlp.localkey);
  jl_platform_strcpy(jlp->joylink_server, fjlp.joylink_server);
  jlp->server_port = fjlp.server_port;

  jl_platform_strcpy(jlp->gURLStr, fjlp.gURLStr);
  jl_platform_strcpy(jlp->gTokenStr, fjlp.gTokenStr);

  if(joylink_dev_get_user_mac(jlp->mac) < 0){
    jl_platform_strcpy(jlp->mac, fjlp.mac);
  }

  if(joylink_dev_get_private_key(jlp->prikey) < 0){
    jl_platform_strcpy(jlp->prikey, fjlp.prikey);
  }
#endif

  jlp->is_actived = E_JL_TRUE;

  jl_platform_strcpy(jlp->modelCode, JLP_CHIP_MODEL_CODE);
  jlp->model_code_flag = E_JL_FALSE;

  jlp->version = JLP_VERSION;
  jl_platform_strcpy(jlp->uuid, jlp_uuid);

  jlp->devtype = JLP_DEV_TYPE;
  jlp->lancon = JLP_LAN_CTRL;
  jlp->cmd_tran_type = JLP_CMD_TYPE;

  jlp->noSnapshot = JLP_SNAPSHOT;

  return ret;
}

/**
 * @brief: 返回设备状态,通过填充user_data参数,返回设备当前状态
 *
 * @param[out] user_data: 设备状态结构体指针
 * 
 * @returns: 0
 */
int
joylink_dev_user_data_get(user_dev_status_t *user_data)
{
  /**
  *FIXME:must to do
  */
#if defined(JOYLINK_SDK_EXAMPLE_TEST) 
  return userDev->State;
#elif  defined(CONFIG_JD_FREERTOS_PAL)
  return user_data->State;
#endif
  return 0;
}

/**
 * @brief: 获取设备快照json结构,结构中包含返回状态码
 *
 * @param[in] ret_code: 返回状态码
 * @param[out] out_snap: 序列化为字符串的设备快照json结构
 * @param[in] out_max: out_snap可写入的最大长度
 *
 * @returns: 实际写入out_snap的数据长度
 */
int
joylink_dev_get_snap_shot_with_retcode(int32_t ret_code, char *out_snap, int32_t out_max)
{
  if(NULL == out_snap || out_max < 0){
    return 0;
  }
  /**
  *FIXME:must to do
  */
  int len = 0;

  joylink_dev_user_data_get(&user_dev);

  char *packet_data =  joylink_dev_package_info(ret_code, &user_dev);
  if(NULL !=  packet_data){
    len = jl_platform_strlen(packet_data);
    log_info("------>%s:len:%d\n", packet_data, len);
    if(len < out_max){
      jl_platform_memcpy(out_snap, packet_data, len); 
    }else{
      len = 0;
    }
  }

  if(NULL !=  packet_data){
    jl_platform_free(packet_data);
  }
  return len;
}

/**
 * @brief: 获取设备快照json结构
 *
 * @param[out] out_snap: 序列化为字符串的设备快照json结构
 * @param[in] out_max: out_snap可写入的最大长度
 *
 * @returns: 实际写入out_snap的数据长度
 */
int
joylink_dev_get_snap_shot(char *out_snap, int32_t out_max)
{
  return joylink_dev_get_snap_shot_with_retcode(0, out_snap, out_max); 
}

/**
 * @brief: 获取向App返回的设备快照json结构
 *
 * @param[out] out_snap: 序列化为字符串的设备快照json结构
 * @param[in] out_max: out_snap允许写入的最大长度
 * @param[in] code: 返回状态码
 * @param[in] feedid: 设备的feedid
 *
 * @returns: 
 */
int
joylink_dev_get_json_snap_shot(char *out_snap, int32_t out_max, int code, char *feedid)
{
    /**
     *FIXME:must to do
     */
    jl_platform_sprintf(out_snap, "{\"code\":%d, \"feedid\":\"%s\"}", code, feedid);

    return jl_platform_strlen(out_snap);
}

/**
 * @brief: 通过App控制设备,需要实现此函数,根据传入的json_cmd对设备进行控制
 *
 * @param[in] json_cmd: 设备控制命令
 *
 * @returns: E_RET_OK:控制成功, E_RET_ERROR:发生错误 
 */
E_JLRetCode_t 
joylink_dev_lan_json_ctrl(const char *json_cmd)
{
    /**
     *FIXME:must to do
     */
#if defined(CONFIG_JD_FREERTOS_PAL) || defined(JOYLINK_SDK_EXAMPLE_TEST)
  int ret = E_RET_ERROR;
  ret = joylink_dev_parse_ctrl(json_cmd,&user_dev);
  log_debug("json ctrl:%s", json_cmd);
  return ret;
#endif
  log_debug("json ctrl:%s", json_cmd);
  return E_RET_OK;
}

/**
 * @brief: 根据传入的cmd值,设置对应设备属性
 *
 * @param[in] cmd: 设备属性名称
 * @param[out] user_data: 设备状态结构体
 * 
 * @returns: E_RET_OK 设置成功
 */
E_JLRetCode_t
joylink_dev_user_data_set(char *cmd, user_dev_status_t *user_data)
{
  /**
  *FIXME:must to do
  */
#if defined(CONFIG_JD_FREERTOS_PAL)
  int ret = E_RET_ERROR;
  ret = jl_flash_SetString(cmd,(char *)user_data,sizeof(user_dev_status_t));
  return ret;
#endif
  return E_RET_OK;
}

/**
 * @brief:根据src参数传入的控制命令数据包对设备进行控制.调用joylink_dev_parse_ctrl进行控制命令解析,并更改设备属性值
 *
 * @param[in] src: 控制指令数据包
 * @param[in] src_len: src长度
 * @param[in] ctr: 控制码
 * @param[in] from_server: 是否来自server控制 0-App,2-Server
 *
 * @returns: E_RET_OK 成功, E_RET_ERROR 失败
 */

E_JLRetCode_t 
joylink_dev_script_ctrl(const char *src, int src_len, JLContrl_t *ctr, int from_server)
{
  if(NULL == src || NULL == ctr){
    return E_RET_ERROR;
  }
  /**
  *FIXME:must to do
  */
  int ret = -1;
  ctr->biz_code = (int)(*((int *)(src + 4)));
  ctr->serial = (int)(*((int *)(src +8)));

  uint32_t tt = jl_get_time_second(NULL);
  log_info("bcode:%d:server:%d:time:%ld", ctr->biz_code, from_server,(long)tt);

  if(ctr->biz_code == JL_BZCODE_GET_SNAPSHOT){
    /*
    *Nothing to do!
    */
    ret = E_RET_OK;
  }else if(ctr->biz_code == JL_BZCODE_CTRL){
    joylink_dev_parse_ctrl(src + 12, &user_dev);
    int ret_led = joylink_dev_led_ctrl(&user_dev);
    if (ret_led == -1)
    {
      user_dev.BrightLED = user_dev.BrightLED_old; //控制失败，亮度值回调
      user_dev.Color = user_dev.Color_old;
      return E_RET_ERROR;
    }
    return E_RET_OK;
  }else if(ctr->biz_code == JL_BZCODE_MENU){
    joylink_dev_parse_ctrl(src + 12, &user_dev);
    return E_RET_OK;
  }else{
    char buf[50];
    jl_platform_sprintf(buf, "Unknown biz_code:%d", ctr->biz_code);
    log_error("%s", buf);
  }
  return ret;
}

/**
 * @brief: 实现接收到ota命令和相关参数后的动作,可使用otaOrder提供的参数进行具体的OTA操作
 *
 * @param[in] otaOrder: OTA命令结构体
 *
 * @returns: E_RET_OK 成功, E_RET_ERROR 发生错误
 */
E_JLRetCode_t
joylink_dev_ota(JLOtaOrder_t *otaOrder)
{
  jl_thread_t task_id;

  if(NULL == otaOrder){
    return E_RET_ERROR;
  }
  joylink_set_ota_info(otaOrder);
  task_id.thread_task = (threadtask) joylink_ota_task;
  task_id.stackSize = 15*1024;
    task_id.priority = JL_THREAD_PRI_DEFAULT;
    task_id.parameter = (void *)otaOrder;
    jl_platform_thread_start(&task_id);

  return E_RET_OK;
}


/**
 * @brief: OTA执行状态上报,无需返回值
 */
void
joylink_dev_ota_status_upload()
{
  return;  
}

static int joylink_dev_http_parse_content(
  char *response, 
  int response_len, 
  char *content,
  int content_len)
{
  int length = 0;
  content[0] = 0;
    char* p = jl_platform_strstr(response, "\r\n\r\n");
    if (p == NULL) 
  {
    return -1;
  }
  p += 4;
  length = response_len - (p - response);
  length = length > content_len ? content_len - 1 : length;
  jl_platform_strncpy(content, p, length);
  content[length] = 0;
  log_info("content = \r\n%s", content);
    
  return length;
}

/**
 * @name:实现HTTPS的POST请求,请求响应填入revbuf参数
 *
 * @param[in]: host POST请求的目标地址
 * @param[in]: query POST请求的路径、HEADER和Payload
 * @param[out]: revbuf 填入请求的响应信息
 * @param[in]: buflen  revbuf最大长度
 *
 * @returns:   负数 - 发生错误, 非负数 - 实际填充revbuf的长度
 *
 * @note:此函数必须正确实现,否则设备无法激活绑定
 * @note:小京鱼平台HTTPS使用的证书每年都会更新.
 * @note:因为Joylink协议层面有双向认证的安全机制,所以此HTTPS请求设备无需校验server的证书.
 * @note:如果设备必须校验server的证书,那么需要开发者实现时间同步等相关机制.
 */
int joylink_dev_https_post( char* host, char* query ,char *revbuf,int buflen)
{
   int ret = 1, len = 1024;
    int real_len = 0;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_net_context server_fd;
    char *set_buf = NULL;
    char *https_recv_buf = NULL;
    const char *pers = "ssl_client1";
     if(host == NULL || query == NULL || revbuf == NULL) {
        log_error("DNS lookup failed");
        return -1;
    }

    mbedtls_net_init( &server_fd );
    mbedtls_ssl_init( &ssl );
    mbedtls_ssl_config_init( &conf );
    mbedtls_ctr_drbg_init( &ctr_drbg );
    mbedtls_entropy_init( &entropy );

    if( ( ret = mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy,
                               (const unsigned char *) pers,
                               strlen( pers ) ) ) != 0 )
    {
        log_info( "https failed: mbedtls_ctr_drbg_seed returned %d\n", ret );
        goto exit;
    }
    if( ( ret = mbedtls_net_connect( &server_fd, host,
                                         "443", MBEDTLS_NET_PROTO_TCP ) ) != 0 )
    {
        log_info( "https failed:  mbedtls_net_connect returned %d\n", ret );
        goto exit;
    }
    log_info( " https success:mbedtls_net_connect ok\n" );

      if( ( ret = mbedtls_ssl_config_defaults( &conf,
                    MBEDTLS_SSL_IS_CLIENT,
                    MBEDTLS_SSL_TRANSPORT_STREAM,
                    MBEDTLS_SSL_PRESET_DEFAULT ) ) != 0 )
    {
        log_info( "https failed:  mbedtls_ssl_config_defaults returned %d\n", ret );
        goto exit;
    }
    log_info( "https success:mbedtls_ssl_config_defaults ok\n" );

    // /* OPTIONAL is not optimal for security,
    //  * but makes interop easier in this simplified example */
    mbedtls_ssl_conf_authmode( &conf, MBEDTLS_SSL_VERIFY_OPTIONAL );
    mbedtls_ssl_conf_rng( &conf, mbedtls_ctr_drbg_random, &ctr_drbg );
    if( ( ret = mbedtls_ssl_setup( &ssl, &conf ) ) != 0 )
    {
        log_info( "https failed: mbedtls_ssl_setup returned %d\n", ret );
        goto exit;
    }
    log_info( "https success:mbedtls_ssl_setup ok\n" );

    if( ( ret = mbedtls_ssl_set_hostname( &ssl, host ) ) != 0 )
    {
        log_info( "https failed:  mbedtls_ssl_set_hostname returned %d\n", ret );
        goto exit;
    }
    log_info( "https success:mbedtls_ssl_set_hostname ok\n" );

    mbedtls_ssl_set_bio( &ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL );
    log_info( "  . Performing the SSL/TLS handshake..." );
    fflush( stdout );
    while( ( ret = mbedtls_ssl_handshake( &ssl ) ) != 0 )
    {
        if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
        {
            log_info( "https failed:  mbedtls_ssl_handshake returned -0x%x\n", -ret );
            goto exit;
        }
    }
    log_info( "https success:mbedtls_ssl_handshake ok\n" );

    while( ( ret = mbedtls_ssl_write( &ssl, (unsigned char *)query, jl_platform_strlen(query) ) ) <= 0 )
    {
        if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
        {
            log_info( " https failed:  mbedtls_ssl_write returned %d\n", ret );
            goto exit;
        }
    }
    log_info( "https success: mbedtls_ssl_write ok\n" );

    /*
     * 7. Read the HTTP response
     */
    https_recv_buf = (char *)jl_platform_malloc(1024);
    if(https_recv_buf==NULL)
    {
      log_info( "------------https failed: malloc more room------------\n");
      goto exit;
    }
    set_buf = https_recv_buf;

    jl_platform_memset(https_recv_buf, 0, 1024);
    do
    {
        ret = mbedtls_ssl_read( &ssl, (unsigned char *)set_buf, len );
        if( ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE )
            continue;
        if( ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY )
            break;
        if( ret < 0 )
        {
            log_info( "https failed:  mbedtls_ssl_read returned %d\n", ret );
            break;
        }
        if( ret == 0 )
        {
            log_info( "\n\nEOF\n\n" );
            log_info("..https---- read data length = %d\n", real_len);
            ret = joylink_dev_http_parse_content(https_recv_buf, real_len, revbuf, buflen);
            break;
        }
        real_len = real_len + ret;
        set_buf = set_buf + ret;
        len = 1024 - real_len;
        if(len == 0)
        {
          log_info( "------------https failed:  mbedtls_ssl_read malloc less room------------\n");
          ret = -1;
          break;
        }
    }while(1);
exit:
    mbedtls_net_free( &server_fd );
    mbedtls_ssl_free( &ssl );
    mbedtls_ssl_config_free( &conf );
    mbedtls_ctr_drbg_free( &ctr_drbg );
    mbedtls_entropy_free( &entropy );

    if(https_recv_buf)
    {
      jl_platform_free(https_recv_buf);
      log_info("--------free https_recv_buf-----------\n");
    }
  log_info("--------joylink_dev_https_post end---ret=%d--------\n",ret);
  return ret;
}
/**
 * @brief 实现HTTP的POST请求,请求响应填入revbuf参数.
 *
 * @param[in]  host: POST请求的目标主机
 * @param[in]  query: POST请求的路径、HEADER和Payload
 * @param[out] revbuf: 填入请求的响应信息的Body
 * @param[in]  buflen: revbuf的最大长度
 *
 * @returns: 负值 - 发生错误, 非负 - 实际填充revbuf的长度
 *
 * @note: 此函数必须正确实现,否者设备无法校时,无法正常激活绑定
 *
 * */
int joylink_dev_http_post( char* host, char* query, char *revbuf, int buflen)
{
#if defined(__LINUX_PAL__) || defined(CONFIG_JD_FREERTOS_PAL)
  int log_socket = -1;
  int len = 0;
    int ret = -1;
    char *recv_buf = NULL;
    jl_sockaddr_in saServer; 
  char ip[20] = {0};

    if(host == NULL || query == NULL || revbuf == NULL) {
        log_error("DNS lookup failed");
        goto RET;
    }

  jl_platform_memset(ip,0,sizeof(ip)); 
  ret = jl_platform_gethostbyname(host, ip, SOCKET_IP_ADDR_LEN);
  if(ret < 0){
    log_error("get ip error, host %s", host);
    ret = -1;
    goto RET;
  }

  jl_platform_memset(&saServer, 0, sizeof(saServer));
  saServer.sin_family = jl_platform_get_socket_proto_domain(JL_SOCK_PROTO_DOMAIN_AF_INET);
  saServer.sin_port = jl_platform_htons(80);
  saServer.sin_addr.s_addr = jl_platform_inet_addr(ip);

  log_socket = jl_platform_socket(JL_SOCK_PROTO_DOMAIN_AF_INET, JL_SOCK_PROTO_TYPE_SOCK_STREAM, JL_SOCK_PROTO_PROTO_IPPROTO_TCP);
  if(log_socket < 0) {
        log_error("... Failed to allocate socket.");
        goto RET;
    }
  int reuseaddrEnable = 1;
  if (jl_platform_setsockopt(log_socket, 
                JL_SOCK_OPT_LEVEL_SOL_SOCKET,
                JL_SOCK_OPT_NAME_SO_REUSEADDR, 
                (uint8_t *)&reuseaddrEnable, 
                sizeof(reuseaddrEnable)) < 0){
    log_error("set SO_REUSEADDR error");
  }
  
    /*fcntl(log_socket,F_SETFL,fcntl(log_socket,F_GETFL,0)|O_NONBLOCK);*/

    if(jl_platform_connect(log_socket, (jl_sockaddr *)&saServer, sizeof(saServer)) != 0)
  {
        log_error("... socket connect failed");
        goto RET;
    }

    if (jl_platform_send(log_socket, query, jl_platform_strlen(query), 0, 5000) < 0) {
        log_error("... socket send failed");
        goto RET;
    }

  int recv_buf_len = 1024; //2048;
  recv_buf = (char *)jl_platform_malloc(recv_buf_len);
  if(recv_buf == NULL)
  {
    goto RET;
  }

  jl_platform_memset(recv_buf, 0, recv_buf_len);
  len = jl_platform_recv(log_socket, recv_buf, recv_buf_len, 0, 5000);
  if(len <= 0)
  {
    ret = -1;
    goto RET;
  }

  log_info("... read data length = %d, response data = \r\n%s", len, recv_buf);
  ret = joylink_dev_http_parse_content(recv_buf, len, revbuf, buflen);

RET:
  if(-1 != log_socket){
    jl_platform_close(log_socket);
  }
  if (recv_buf)
  {
    /* code */
    jl_platform_free(recv_buf);
  }
  
  return ret;
#else
    return -1;
#endif

}

/**
 * @brief: SDK main loop 运行状态报告,正常情况下此函数每5秒会被调用一次,可以用来判断SDK主任务的运行状态.
 * 
 * @param[in] status: SDK main loop运行状态 0正常, -1异常
 * 
 * @return: reserved 当前此函数仅做通知,调用方不关心返回值.
 */

int joylink_dev_run_status(JLRunStatus_t status)
{
  int ret = -1;
  /**
     *FIXME:must to do
  */
  return ret;
}

/**
 * @brief: 每间隔1个main loop周期此函数将在SDK main loop中被调用,让用户有机会将代码逻辑运行在核心任务中.
 * 
 * @note:  正常情况下一个main loop周期为1s(取决于socket等待接收数据的timeout时间),但不保证精度,请勿用作定时器
 * @note:  仅用作关键的非阻塞任务执行,例如OTA状态上报或设备状态上报.
 * @note:  执行阻塞或耗时较多的代码,将会妨碍主task运行.
 */
void joylink_dev_run_user_code()
{
    //You may add some code run in the main loop if necessary.
}

// 示例信息为格力灯， 对应的MAC为00:06:06:00:00:13
#if 0
static int joylink_set_device_info(void)
{
  char jlpuuid[JLP_UUID_LEN] = {"8085DC"};
  char jlpsoft_ssid[JLP_SOFT_SSID_LEN] = {"JDlll_123"};
  char jlpcid[JLP_CID_LEN] = {"102009"};
  char jlpbrand[JLP_BRAND_LEN] = {"1"};
  char jlpcloud_pub_key[JLP_CLOUD_PUB_KEY_LEN] = {"02206A8A57438DFF99F5B95BB63AE3C3A21FAACC3E9E4515F29DAC9C56A2E0A29F"};
  char jlpcloud_pri_key[JLP_CLOUD_PRI_KEY_LEN] = {"2F50DCBBCB271DA2777B4F60009094192ED5DAD92E36CD2D22F838BBFA2E92BA"};

  // 同一个产品支持WiFi和蓝牙
  if( jl_flash_SetString("jlp_uuid", (char *)&jlpuuid,  sizeof(jlpuuid)) != 0){
    log_info("set uuid String failed\n");
    return -1;
  }

  if( jl_flash_SetString("jlp_cid", (char *)&jlpcid,  sizeof(jlpcid)) != 0){
    log_info("set cid String failed\n");
    return -1;
  }

  if( jl_flash_SetString("jlp_brand", (char *)&jlpbrand,  sizeof(jlpbrand)) != 0){
    log_info("set soft_ssid String failed\n");
    return -1;
  }

  if( jl_flash_SetString("jlp_soft_ssid", (char *)&jlpsoft_ssid,  sizeof(jlpsoft_ssid)) != 0){  //AP热点
    log_info("set soft_ssid String failed\n");
    return -1;
  }
  // 同一个产品，MAC相同，公钥秘钥也是相同
  if( jl_flash_SetString("jlp_cloud_pub_key", (char *)&jlpcloud_pub_key,  sizeof(jlpcloud_pub_key)) != 0){
    log_info("set cid String failed\n");
    return -1;
  }

  if( jl_flash_SetString("jlp_cloud_pri_key", (char *)&jlpcloud_pri_key,  sizeof(jlpcloud_pri_key)) != 0){
    log_info("set jlp_cloud_pri_key String failed\n");
    return -1;
  }

  return 0;
}
#endif
/**
 * brief: 初始化设备信息，从NV内读.
 * 设备信息UUID、私钥、公钥、AP热点、CID、Brand
 */
int joylink_device_info_init(void)
{
  // 预先写入设备信息,后续若NV内有则可以去掉；
  //joylink_set_device_info();

  // 同一个产品支持WiFi和蓝牙
  if( jl_flash_GetString("jlp_uuid", (char *)&jlp_uuid,  sizeof(jlp_uuid)) == 0){
    log_info("Get uuid String failed\n");
    return -1;
  }
  if( jl_flash_GetString("jlp_cid", (char *)&jlp_cid,  sizeof(jlp_cid)) == 0){
    log_info("Get cid String failed\n");
    return -1;
  }

  if( jl_flash_GetString("jlp_brand", (char *)&jlp_brand,  sizeof(jlp_brand)) == 0){
    log_info("Get soft_ssid String failed\n");
    return -1;
  }

  if( jl_flash_GetString("jlp_soft_ssid", (char *)&jlp_soft_ssid,  sizeof(jlp_soft_ssid)) == 0){  //AP热点
    log_info("Get soft_ssid String failed\n");
    return -1;
  }
  // 同一个产品，MAC相同，公钥秘钥也是相同
  if( jl_flash_GetString("jlp_cloud_pub_key", (char *)&jlp_cloud_pub_key,  sizeof(jlp_cloud_pub_key)) == 0){
    log_info("Get cid String failed\n");
    return -1;
  }

  if( jl_flash_GetString("jlp_cloud_pri_key", (char *)&jlp_cloud_pri_key,  sizeof(jlp_cloud_pri_key)) == 0){
    log_info("Get jlp_cloud_pri_key String failed\n");
    return -1;
  }
  //测试读取值是否正确，可去掉
  // jl_platform_printf("================test====================\n");
  // jl_platform_printf("jlp_uuid is %s\n", jlp_uuid);
  // jl_platform_printf("jlp_cid is %s\n", jlp_cid);
  // jl_platform_printf("jlp_brand is %s\n", jlp_brand);
  // jl_platform_printf("jlp_soft_ssid is %s\n", jlp_soft_ssid);
  // jl_platform_printf("jlp_cloud_pub_key is %s\n", jlp_cloud_pub_key);
  // jl_platform_printf("jlp_cloud_pri_key is %s\n", jlp_cloud_pri_key);
  // jl_platform_printf("=================test===================\n");
  joylink_idt_info_init();

  return 0;
}

// jl2_d_idt_t user_idt 结构体初始化, 京东方面的结构体类型，主要为公钥
int joylink_idt_info_init(void)
{
  user_idt.type = 0;
  jl_platform_strcpy(user_idt.cloud_pub_key, jlp_cloud_pub_key);
  jl_platform_strcpy(user_idt.sig, "01234567890123456789012345678901");
  jl_platform_strcpy(user_idt.pub_key, "01234567890123456789012345678901");
  jl_platform_strcpy(user_idt.f_sig, "01234567890123456789012345678901");
  jl_platform_strcpy(user_idt.f_pub_key, "01234567890123456789012345678901");
  return 0;
}

/**
 * brief: 获取设备信息，提供给其它接口
 * 设备信息UUID、私钥、公钥、AP热点、CID、Brand
 */
int joylink_get_device_info(user_dev_info_t *dev_info)
{
  jl_platform_memcpy(dev_info->jlp_uuid, jlp_uuid, sizeof(jlp_uuid));
  jl_platform_memcpy(dev_info->jlp_cid, jlp_cid, sizeof(jlp_cid));
  jl_platform_memcpy(dev_info->jlp_brand, jlp_brand, sizeof(jlp_brand));
  jl_platform_memcpy(dev_info->jlp_soft_ssid, jlp_soft_ssid, sizeof(jlp_soft_ssid));
  return 0;
}

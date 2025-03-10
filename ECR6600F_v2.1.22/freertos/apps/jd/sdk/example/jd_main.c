#include "joylink_stdio.h"
#include "joylink_thread.h"
#include "joylink_extern.h"
#include "joylink.h"
#include "joylink_softap.h"
#include "joylink_flash.h"
#include "joylink_string.h"
#include  "system_wifi.h"
#include "joylink_extern_led.h"
#include "joylink_memory.h"
#include "joylink_socket.h"
#include "softap/joylink_softap_start.h"
#include "os_task_config.h"
#include "easyflash.h"


static jl_timer_t htimer;
#if defined(CONFIG_BLE_JD_ADAPTER)
#include "jd_netcfg.h"
extern int netcfg_task_handle;
extern void jd_netcfg_task(void *arg);
#endif //CONFIG_BLE_TEST_NETCFG
int reset_count_task_handle;
int StartCountFlag = 0;

int _close_r()
{
  return 0;
}

void AP_mode_timeout_func(void* param)
{
  StartCountFlag = 1;
  jl_timer_stop(&htimer);
  jl_timer_delete(&htimer);
}

void start_count_set(void *arg)
{
    int value = 0;
    jl_platform_printf("start_count_set task\n");
    int count = 10;
    while(count--){
        if(1 == StartCountFlag){
            value = 0;
            jl_flash_SetString("StartCount", (char *)&value, sizeof(value));
            StartCountFlag = 0;
            jl_platform_printf("eswin  StartCount reset to:%d\n",value);
            break;
        }
        jl_flash_GetString("StartCount", (char *)&value,  sizeof(value));
        jl_platform_printf("eswin  StartCount:%d\n",value);
        os_msleep(2000);
    }
    os_task_delete(reset_count_task_handle);
}

void 
user_main()
{
  jl_thread_t task_id; 
  JLPInfo_t fjlp;
  char ssid[WIFI_SSID_MAX_LEN] = {0};
  char passwd[WIFI_PWD_MAX_LEN] = {0};
  char feedid[JL_MAX_FEEDID_LEN] = {0};
  char jlp_key[] = "jlp_key";
  char StartCount[] = "StartCount";
  char value = 0,  StartCount_ret = 0;

  joylink_device_info_init(); //初始化设备信息

#if defined(JOYLINK_SDK_EXAMPLE_TEST) || defined(CONFIG_JD_FREERTOS_PAL)
  user_dev_set();
  joylink_dev_led_init();
  joylink_dev_led_ctrl(&user_dev);
#endif

  task_id.thread_task = (threadtask) joylink_main_start;
  task_id.stackSize = 0x5000;
  task_id.priority = JL_THREAD_PRI_DEFAULT;
  task_id.parameter = NULL;
  jl_platform_thread_start(&task_id);

#ifdef CONFIG_BLE_JD_ADAPTER
	EventGroupHandler = xEventGroupCreate();
#endif
  StartCount_ret = jl_flash_GetString(StartCount, (char *)&value,  sizeof(value));
  if (StartCount_ret == 0)
  {
    jl_platform_printf("=jl_flash_GetString  failed \n");
    value = 1;
    jl_flash_SetString(StartCount, (char *)&value, sizeof(value));	 //初始值为1
  }
  else
  {
    value += 1;
    jl_flash_SetString(StartCount, (char *)&value, sizeof(value));
  }

  /*创建task,用于重置重启次数*/
  reset_count_task_handle = os_task_create( "jd_reset_StartCount", 1, 3*1024, (task_entry_t)start_count_set, NULL);

  //创建定时器
  jl_platform_memset(&htimer , 0, sizeof(jl_timer_t));
  htimer.userData = "APModeTimer";
  htimer.periodicInterval = 10 *1000;
  htimer.callback = AP_mode_timeout_func;
  jl_timer_create(&htimer);
  jl_timer_start(&htimer);
  jl_platform_printf("======APModeTimer start======\n");
   
  jl_platform_memset(&fjlp, 0, sizeof(JLPInfo_t));
  char fjlp_ret = jl_flash_GetString(jlp_key, (char *)&fjlp, sizeof(JLPInfo_t));
  if (fjlp_ret != 0){  //值不为0，说明读取成功；
  jl_platform_strncpy(feedid, fjlp.feedid, JL_MAX_FEEDID_LEN-1);
  }

  if (value < 3 && fjlp_ret != 0 && (feedid[17] >= '0' && feedid[17] <= '9'))
  {
    jl_platform_printf("enter to STA mode !\n");
    jl_flash_GetString("StaSSID", ssid, WIFI_SSID_MAX_LEN-1);
    jl_flash_GetString("StaPW", passwd, WIFI_PWD_MAX_LEN-1);

    while (!wifi_is_ready()) { //等待WiFi就绪
    // jl_platform_printf(">>>>>>>wifi is not ready !\n");
      os_msleep(50);
    }

    int ret = 0;
    ret = jl_softap_enter_sta_mode(ssid, passwd);
    if (ret == 0)
    {
      jl_platform_printf("config STA mode OK !\n");
      return ;
    }
#ifdef CONFIG_BLE_JD_ADAPTER
    xEventGroupSetBits(EventGroupHandler,EVENTBIT_0);
#endif
  }
  if(value >= 3)
  {
	  customer_del_env("StaPW");
	  customer_del_env("StaSSID");
	  jl_flash_SetString(jlp_key,NULL,sizeof(JLPInfo_t));
#ifdef CONFIG_BLE_JD_ADAPTER
	  xEventGroupClearBits(EventGroupHandler,EVENTBIT_0);
#endif
  }

#ifdef CONFIG_BLE_JD_ADAPTER
	if(!(xEventGroupGetBits(EventGroupHandler) & (1<<0)))
	{
	   jl_platform_printf("ble adapter Enable !\n");
	   netcfg_task_handle=os_task_create( "jd_netcfg-task", BLE_APPS_PRIORITY, JD_BLE_NETCFG_STACK_SIZE, (task_entry_t)jd_netcfg_task, NULL);
	   xEventGroupSetBits(EventGroupHandler,EVENTBIT_0);
 	 }
#else

  jl_platform_printf("enter to AP mode !\n");
  // set AP mode
  jl_softap_enter_ap_mode();

  joylink_softap_start(5*60*1000);

  jl_platform_msleep(2000);

  // 开启局域网激活模式
  joylink_dev_lan_active_switch(1);

  value = 0;
  jl_flash_SetString(StartCount, (char *)&value, sizeof(value));  //配置完成清零
#endif
  jl_platform_msleep(1000);
  // while(1)
  //   {
  //    jl_platform_msleep(1000);
  //   }
  // return 0;
}

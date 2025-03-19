# 索引

  * [1 tuya_os_adapt_get_systemtickcount](#1-tuya_os_adapt_get_systemtickcount)
  * [2 tuya_os_adapt_get_tickratems](#2-tuya_os_adapt_get_tickratems)
  * [3 tuya_os_adapt_system_sleep](#3-tuya_os_adapt_system_sleep)
  * [4 tuya_os_adapt_system_isrstatus](#4-tuya_os_adapt_system_isrstatus)
  * [5 tuya_os_adapt_system_reset](#5-tuya_os_adapt_system_reset)
  * [6 tuya_os_adapt_system_getheapsize](#6-tuya_os_adapt_system_getheapsize)
  * [7 tuya_os_adapt_system_get_rst_info](#7-tuya_os_adapt_system_get_rst_info)
  * [8 tuya_os_adapt_get_random_data](#8-tuya_os_adapt_get_random_data)
  * [9 tuya_os_adapt_set_cpu_lp_mode](#9-tuya_os_adapt_set_cpu_lp_mode)
  * [10 tuya_os_adapt_system_getMiniheapsize](#10-tuya_os_adapt_system_getMiniheapsize)
  * [11 tuya_os_adapt_wifi_get_lp_mode](#11-tuya_os_adapt_wifi_get_lp_mode)
  * [12 tuya_os_adapt_watchdog_init_start](#12-tuya_os_adapt_watchdog_init_start)
  * [13 tuya_os_adapt_watchdog_refresh](#13-tuya_os_adapt_watchdog_refresh)
  * [14 tuya_os_adapt_watchdog_stop](#14-tuya_os_adapt_watchdog_stop)


------

## 1-tuya_os_adapt_get_systemtickcount

```c
SYS_TICK_T tuya_os_adapt_get_systemtickcount(void)
```

- **函数描述**

  获取系统当前的tick值
 

- **参数**

  无
  

- **返回值**

  系统当前的tick值

## 2-tuya_os_adapt_get_tickratems

```c
unsigned int tuya_os_adapt_get_tickratems(void)
```

- **函数描述**

  获取系统1个ticket是多少个ms
 

- **参数**

  无

- **返回值**

  系统1个ticket是多少个ms


## 3-tuya_os_adapt_system_sleep

```c
void tuya_os_adapt_system_sleep(const unsigned long msTime)
```

- **函数描述**

  线程睡眠
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | msTime | 系统sleep的时间（单位ms） |

- **返回值**

  无

## 4-tuya_os_adapt_system_isrstatus

```c
bool tuya_os_adapt_system_isrstatus(void)
```

- **函数描述**

  获取系统是否处于中断例程里
 

- **参数**

  无

- **返回值**

  bool : TRUE:在中断例程；FALSE:不在中断例程
  
## 5-tuya_os_adapt_system_reset

```c
void tuya_os_adapt_system_reset(void)
```

- **函数描述**

  重启系统
 

- **参数**

  无

- **返回值**

  无

## 6-tuya_os_adapt_system_getheapsize

```c
int tuya_os_adapt_system_getheapsize(void)
```

- **函数描述**

  获取系统剩余内存大小
 
- **参数**

  无

- **返回值**

  剩余内存

## 7-tuya_os_adapt_system_get_rst_info

```c
TY_RST_REASON_E tuya_os_adapt_system_get_rst_info(void)
```

- **函数描述**

  获取系统重启原因
  | TY_RST_REASON_E | —— |
  |--------|--------|
  | TY_RST_POWER_OFF | 电源重启 |
  | TY_RST_HARDWARE_WATCHDOG | 硬件看门狗复位 |
  | TY_RST_FATAL_EXCEPTION | 异代码常 |
  | TY_RST_SOFTWARE_WATCHDOG | 软件看门狗重启 |
  | TY_RST_SOFTWARE | 软件复位 |
  | TY_RST_DEEPSLEEP | 深度睡眠 |
  | TY_RST_HARDWARE | 硬件复位 |
  | TY_RST_OTHER | 其它原因 |
  | TY_RST_UNSUPPORT | 不支持 |
 
- **参数**

  无

- **返回值**

  重启原因 
 
## 8-tuya_os_adapt_get_random_data

```c
int tuya_hal_get_random_data(const unsigned int range)
```

- **函数描述**

  获取随机数
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | range | 随机数range |


- **返回值**

  随机值
  
- **备注**

  真随机数

## 9-tuya_os_adapt_set_cpu_lp_mode

```c
int tuya_os_adapt_set_cpu_lp_mode(const bool en,const TY_CPU_SLEEP_MODE_E mode)
```

- **函数描述**

  设置cpu的低功耗模式
  | mode | —— |
  |--------|--------|
  | TY_CPU_SLEEP | 低功耗模式、os自动唤醒、15mA以内 |
  | TY_CPU_DEEP_SLEEP | 深度睡眠 |
 
- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | en | 是否使能cpu低功耗模式 |
  | [in] | mode | cpu低功耗模式 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_CPU_LPMODE_SET_FAILED ：失败
  ```
## 10-tuya_os_adapt_system_getMiniheapsize

```c
int tuya_os_adapt_system_getMiniheapsize(void)
```

- **函数描述**

  系统最小剩余内存大小
 
- **参数**

无

- **返回值**

  ```
  剩余内存
  ```
## 11-tuya_os_adapt_wifi_get_lp_mode

```c
int tuya_os_adapt_wifi_get_lp_mode(void)
```

- **函数描述**

  获取CPU低功耗标识
 
- **参数**

无

- **返回值**

  ```
  0 ： 非低功耗模式
  1 ： 低功耗模式
  ```
## 12-tuya_os_adapt_watchdog_init_start

```c
unsigned int tuya_os_adapt_watchdog_init_start(const unsigned int timeval)
```

- **函数描述**

  获取CPU低功耗标识
 
- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | timeval | 	看门狗复位检测时间间隔 |

- **返回值**

  ```
 芯片实际设置的看门狗时间
  
  ## 13-tuya_os_adapt_watchdog_refresh

```c
void tuya_os_adapt_watchdog_refresh(void)D_T tuya_hal_watchdog_refresh(VOID_T)
```

- **函数描述**

  用于刷新watchdog（喂狗）
 
- **参数**

无

- **返回值**

无

  ## 14-tuya_os_adapt_watchdog_stop

```c
void tuya_os_adapt_watchdog_stop(void)
```

- **函数描述**

  用于刷新watchdog（喂狗）
 
- **参数**

无

- **返回值**

无
# 索引

  * [1 rtc_os_adapt_init](#1-rtc_os_adapt_init)
  * [2 rtc_os_adapt_time_set](#2-rtc_os_adapt_time_set)
  * [3 rtc_devos_adapt_time_get](#3-rtc_os_adapt_time_get)
------

## 1-rtc_os_adapt_init

```c
static int rtc_os_adapt_init(void)
```

- **函数描述**

  初始化芯片内部RTC

- **返回值**

  OPRT_OS_ADAPTER_OK : 成功void 无返回值


## 2-rtc_os_adapt_time_set

```c
static int rtc_os_adapt_time_set(time_t timestamp)
```

- **函数描述**

  向芯片内部RTC设置时间
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | timestap | 时间戳，单位为秒 |

- **返回值**

  OPRT_OS_ADAPTER_OK : 成功

## 3-rtc_os_adapt_time_get

```c
static int rtc_os_adapt_time_get(time_t *timestamp)
```

- **函数描述**

  从芯片内部rtc时钟获取时间
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [out] | timestamp | RTC时间，单位为秒 |

- **返回值**

  芯片内部的rtc时间，单位为秒

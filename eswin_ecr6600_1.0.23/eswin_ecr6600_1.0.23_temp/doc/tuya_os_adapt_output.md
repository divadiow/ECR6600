# 索引

  * [1 tuya_os_adapt_output_log](#1-tuya_os_adapt_output_log)
  * [2 tuya_os_adapt_log_close](#2-tuya_os_adapt_log_close)
  * [3 tuya_os_adapt_log_open](#3-tuya_os_adapt_log_open)
  
------

## 1-tuya_os_adapt_output_log

```c
void tuya_os_adapt_output_log(const signed char *str)
```

- **函数描述**

  打印字符串
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | str | 字符串指针 |

- **返回值**

  void

## 2-tuya_os_adapt_log_close

```c
int tuya_os_adapt_log_close(void)
```

- **函数描述**

  关闭log口
  业务需要用到log uart的io作为普通io时需要关闭uart功能
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | str | 字符串指针 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_LOG_CLOSE_FAILED ：失败
  ```
  
  
## 3-tuya_hal_log_open

```c
int tuya_os_adapt_log_open(void)
```

- **函数描述**

  恢复log口
  

- **参数**

  无

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_LOG_OPEN_FAILED ：失败
  ```
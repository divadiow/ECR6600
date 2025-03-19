# 索引

  * [1 tuya_os_adapt_ota_start_inform](#1-tuya_os_adapt_ota_start_inform)
  * [2 tuya_os_adapt_ota_data_process](#2-tuya_os_adapt_ota_data_process)
  * [3 tuya_os_adapt_ota_end_inform](#3-tuya_os_adapt_ota_end_inform)

------

## 1-tuya_os_adapt_ota_start_inform

```c
int tuya_os_adapt_ota_start_inform(unsigned int file_size)
```

- **函数描述**

  升级开始通知函数
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | file_size | 固件升级包总大小 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_MALLOC_FAILED ：申请内存失败
  OPRT_OS_ADAPTER_OTA_START_INFORM_FAILED ：失败
  ```

- **备注**
  告知hal层即将开始ota升级，需要hal层预先申请升级ota管理资源，记录ota固件包大小。

## 2-tuya_os_adapt_ota_data_process

```c
int tuya_os_adapt_ota_data_process(const unsigned int total_len, const unsigned int offset,\
const unsigned char* data, const unsigned int len, unsigned int* remain_len, void* pri_data)
```

- **函数描述**

  ota数据包处理
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | total_len | ota升级固件总大小 |
  | [in] | offset | data指针在ota升级固件中的偏移量 |
  | [in] | data | 本次ota数据包buffer指针 |
  | [in] | len | 本次ota数据包长度 |
  | [out] | remain_len | 已经下发但未处理的数据包长度 |
  | [in] | pri_data | 保留参数，暂不使用 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_MALLOC_FAILED ：申请内存失败
  OPRT_OS_ADAPTER_OTA_PROCESS_FAILED ：失败  
  ```

- **备注**

  ```
  此函数用于支撑ota升级，需结合涂鸦升级包拉取业务来实现。
  目前涂鸦有两种升级方式： 1.http固件升级;2.私有串口协议固件升级;但对该hal函数来说无需关心数据包的来源。
  ota升级过程中tuya sdk会循环收包，并将ota数据包通知给此函数，由此函数完成ota数据包写入flash的工作。
  ```

## 3-tuya_os_adapt_ota_end_inform

```c
int tuya_os_adapt_ota_end_inform(bool reset)
```

- **函数描述**

  ota数据下载结束通知
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | reset | 通知hal层下载结束是否需要重启 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK ：成功
  OPRT_OS_ADAPTER_OTA_VERIFY_FAILED ：校验失败
  OPRT_OS_ADAPTER_OTA_END_INFORM_FAILED ：失败
  ```

- **备注**

  ```
  ota数据包传输完成后会调用此通知函数，需在次函数中实现：
    1.释放ota开始时hal层申请的资源。
    2.做固件完整性校验
    3.修改ota参数区，重启之后运行新固件
    4.根据入参reset决定是否需要重启（tuya串口升级成功后的重启逻辑不在这个api实现）
  ```

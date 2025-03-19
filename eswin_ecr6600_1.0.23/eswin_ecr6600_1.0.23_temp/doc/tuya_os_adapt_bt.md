# 索引

  * [1 tuya_os_adapt_bt_port_init](#1-tuya_os_adapt_bt_port_init)
  * [2 tuya_os_adapt_bt_port_deinit](#2-tuya_os_adapt_bt_port_deinit)
  * [3 tuya_os_adapt_bt_gap_disconnect](#3-tuya_hal_bt_gap_disconnect)
  * [4 tuya_os_adapt_bt_send](#4-tuya_os_adapt_bt_send)
  * [5 tuya_os_adapt_bt_reset_adv](#5-tuya_os_adapt_bt_reset_adv)
  * [6 tuya_os_adapt_bt_get_rssi](#6-tuya_os_adapt_bt_get_rssi)
  * [7 tuya_os_adapt_bt_start_adv](#7-tuya_os_adapt_bt_start_adv)
  * [8 tuya_os_adapt_bt_stop_adv](#8-tuya_os_adapt_bt_stop_adv)
  * [9 tuya_os_adapt_bt_assign_scan](#9-tuya_os_adapt_bt_assign_scan)
  * [9 tuya_os_adapt_bt_get_ability](#9-tuya_os_adapt_bt_get_ability)
------

# STRUCTs

## ty_bt_param_t
```c
typedef VOID (*TY_BT_MSG_CB)(int id, ty_bt_cb_event_t e, unsigned char *buf, unsigned int len);

typedef struct {
    char name[DEVICE_NAME_LEN]; //蓝牙名字
    ty_bt_mode_t mode; 
    unsigned char link_num; // 最大支持客户端连接数，默认是1
    TY_BT_MSG_CB cb; // 事件和数据回调函数
    tuya_ble_data_buf_t adv; // 广播内容
    tuya_ble_data_buf_t scan_rsp; // 客户端(APP)扫描响应数据
}ty_bt_param_t;

```

## ty_bt_scan_info_t
```c
typedef struct {
    ty_bt_scan_type_t scan_type; /* 扫描类型 */
    char name[DEVICE_NAME_LEN]; // 蓝牙名字，设置为TY_BT_SCAN_BY_NAME时生效
    char bd_addr[6]; //蓝牙的MAC，设置为TY_BT_SCAN_BY_MAC时生效
    char rssi; // 扫描到后，返回信号强度
    unsigned char channel;  // 扫描到后，返回信道
    unsigned char timeout_s; /* 扫描超时，在规定的时间没扫到，返回失败，单位是second. */
}ty_bt_scan_info_t;
```

# APIs
## 1-tuya_os_adapt_bt_port_init

```c
OPERATE_RET tuya_os_adapt_bt_port_init(ty_bt_param_t *p)
```

- **函数描述**

  初始化蓝牙:在WiFi和蓝牙共用RF的情况，该函数需要支持多次调用，即在调用tuya_bt_port_deinit关闭蓝牙后，可以再次调用该函数来重新初始化蓝牙
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | p | 蓝牙参数结构体指针 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_MALLOC_FAILED ：申请内存失败
  OPRT_OS_ADAPTER_BT_INIT_FAILED ：失败
  ```

- **备注**

  ```c
  1 不能在该函数里面添加sleep函数来等待初始化完成
  2 蓝牙初始化完成后，是通过TY_BT_EVENT_ADV_READY事件回到通知到应用层
  ```

## 2-tuya_os_adapt_bt_port_deinit

```
OPERATE_RET tuya_os_adapt_bt_port_deinit(void)
```

- **函数描述**

  关闭蓝牙初始化

- **参数**

  无

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_BT_DEINIT_FAILED ：失败
  ```

- **备注**

  ```
  1 在WiFi和蓝牙共用RF的情况，关闭WiFi前，需要先调用该函数关闭蓝牙
  2 在低功耗的情况下，调用该函数来关闭蓝牙从而达到低功耗
  ```

## 3-tuya_os_adapt_bt_gap_disconnect

```c
OPERATE_RET tuya_os_adapt_bt_gap_disconnect(void)
```

- **函数描述**

  蓝牙作为slave主动断开连接
 

- **参数**

  无

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_BT_DISCONN_FAILED ：失败
  ```


## 4-tuya_os_adapt_bt_send

```
OPERATE_RET tuya_os_adapt_bt_send(BYTE_T *data, UINT8_T len)
```

- **函数描述**

  蓝牙作为slave发送数据
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | data | 数据指针 |
  | [in] | len | 数据长度 |

- **返回值**
  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_BT_SEND_FAILED ：失败
  ```

## 5-tuya_os_adapt_bt_reset_adv

```
OPERATE_RET tuya_os_adapt_bt_reset_adv(tuya_ble_data_buf_t *adv, tuya_ble_data_buf_t *scan_resp)
```

- **函数描述**

  重置蓝牙广播内容和扫描响应内容
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | adv | 蓝牙广播数据 |
  | [in] | lscan_respen | 蓝牙扫描响应数据 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_BT_ADV_RESET_FAILED ：失败
  ```

## 6-tuya_os_adapt_bt_get_rssi

```
OPERATE_RET tuya_os_adapt_bt_get_rssi(SCHAR_T *rssi)
```

- **函数描述**

  获取蓝牙的信号强度
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [out] | rssi | 信号强度指针 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_BT_RSSI_GET_FAILED ：失败
  ```


## 7-tuya_os_adapt_bt_start_adv

```
OPERATE_RET tuya_os_adapt_bt_start_adv()
```

- **函数描述**

  启动蓝牙广播，直接使用原有的参数
 

- **参数**

  无

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_MALLOC_FAILED ：申请内存失败
  OPRT_OS_ADAPTER_BT_ADV_START_FAILED ：失败
  ```

## 8-tuya_os_adapt_bt_stop_adv

```
OPERATE_RET tuya_os_adapt_bt_start_adv()
```

- **函数描述**

  停止蓝牙广播
 

- **参数**

  无

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_BT_ADV_STOP_FAILED ：失败
  ```


## 9-tuya_os_adapt_bt_assign_scan

```
OPERATE_RET tuya_os_adapt_bt_assign_scan(IN OUT ty_bt_scan_info_t *info)
```

- **函数描述**

  扫描指定蓝牙名字，获取蓝牙信息，用于产测
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [inout] | info | 蓝牙信息 |
 
- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_MALLOC_FAILED ：申请内存失败
  OPRT_OS_ADAPTER_BT_SCAN_FAILED ：失败
  ```

- **备注**
  ```
  1 scan_type为TY_BT_SCAN_BY_NAME，则扫描特定的蓝牙名字的信标
  2 scan_type为TY_BT_SCAN_BY_MAC，则扫描特定的MAC地址的信标
  3 返回的信息中，一定需要携带信号强度
  ```
## 10-tuya_os_adapt_bt_assign_scan

```
OPERATE_RET tuya_os_adapt_bt_get_ability(VOID_T)
```

- **函数描述**

  获取蓝牙能力
 

- **参数**

无
 
- **返回值**

  ```
	BT_ABI_NEED_RESET_STACK : 断连需要重启协议栈
  BT_ABI_OTHER ：其他，待定义
  ```

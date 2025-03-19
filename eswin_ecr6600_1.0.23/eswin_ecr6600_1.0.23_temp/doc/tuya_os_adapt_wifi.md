# 索引

  * [1 tuya_os_adapt_wifi_all_ap_scan](#1-tuya_os_adapt_wifi_all_ap_scan)
  * [2 tuya_os_adapt_wifi_assign_ap_scan](#2-tuya_os_adapt_wifi_assign_ap_scan)
  * [3 tuya_os_adapt_wifi_release_ap](#3-tuya_os_adapt_wifi_release_ap)
  * [4 tuya_os_adapt_wifi_set_cur_channel](#4-tuya_os_adapt_wifi_set_cur_channel)
  * [5 tuya_os_adapt_wifi_get_cur_channel](#5-tuya_os_adapt_wifi_get_cur_channel)
  * [6 tuya_os_adapt_wifi_get_ip](#6-tuya_os_adapt_wifi_get_ip)
  * [7 tuya_os_adapt_wifi_set_mac](#7-tuya_os_adapt_wifi_set_mac)
  * [8 tuya_os_adapt_wifi_get_mac](#8-tuya_os_adapt_wifi_get_mac)
  * [9 tuya_os_adapt_wifi_set_work_mode](#9-tuya_os_adapt_wifi_set_work_mode)
  * [10 tuya_os_adapt_wifi_get_work_mode](#10-tuya_os_adapt_wifi_get_work_mode)
  * [11 tuya_os_adapt_wifi_sniffer_set](#11-tuya_os_adapt_wifi_sniffer_set)
  * [12 tuya_os_adapt_wifi_ap_start](#12-tuya_os_adapt_wifi_ap_start)
  * [13 tuya_os_adapt_wifi_ap_stop](#13-tuya_os_adapt_wifi_ap_stop)
  * [14 tuya_os_adapt_wifi_get_connected_ap_info_v2](#14-tuya_os_adapt_wifi_get_connected_ap_info_v2)
  * [15 tuya_os_adapt_wifi_fast_station_connect_v2](#15-tuya_os_adapt_wifi_fast_station_connect_v2)
  * [16 tuya_os_adapt_wifi_station_connect](#16-tuya_os_adapt_wifi_station_connect)
  * [17 tuya_os_adapt_wifi_station_disconnect](#17-tuya_os_adapt_wifi_station_disconnect)
  * [18 tuya_os_adapt_wifi_station_get_conn_ap_rssi](#18-tuya_os_adapt_wifi_station_get_conn_ap_rssi)
  * [19 tuya_os_adapt_wifi_get_bssid](#19-tuya_os_adapt_wifi_get_bssid)
  * [20 tuya_os_adapt_wifi_station_get_status](#20-tuya_os_adapt_wifi_station_get_status)
  * [21 tuya_os_adapt_wifi_set_country_code](#21-tuya_os_adapt_wifi_set_country_code)
  * [22 tuya_os_adapt_wifi_send_mgnt](#22-tuya_os_adapt_wifi_send_mgnt)
  * [23 tuya_os_adapt_wifi_register_recv_mgnt_callback](#23-tuya_os_adapt_wifi_register_recv_mgnt_callback)
  * [24 tuya_os_adapt_set_wifi_lp_mode](#24-tuya_os_adapt_set_wifi_lp_mode)
  * [25 tuya_os_adapt_wifi_rf_calibrated](#24-tuya_os_adapt_wifi_rf_calibrated)
------
# STRUCTs

## WF_AP_CFG_IF_S
```c
    typedef struct {
          unsigned char ssid[32 +1];
          unsigned char s_len;
          unsigned char passwd[64 +1];
          unsigned char p_len;
          unsigned char chan;
          WF_AP_AUTH_MODE_E md;
          unsigned char ssid_hidden;
          unsigned char max_conn;
          unsigned short ms_interval;
          NW_IP_S ip;
    } WF_AP_CFG_IF_S;

```
## WF_AP_AUTH_MODE_
```c
typedef enum
    {
         WAAM_OPEN = 0,
         WAAM_WEP,
         WAAM_WPA_PSK,
         WAAM_WPA2_PSK,
         WAAM_WPA_WPA2_PSK,
         WAAM_UNKNOWN,
    } WF_AP_AUTH_MODE_
```
# APIs
## 1-tuya_os_adapt_wifi_all_ap_scan

```c
int tuya_os_adapt_wifi_all_ap_scan(AP_IF_S **ap_ary, unsigned int *num)
```

- **函数描述**

  扫描当前环境所有ap信息
  
- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [out] | ap_ary | 扫描结果 |
  | [out] | num | 扫描ap个数 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_AP_SCAN_FAILED ：失败
  ```

- **备注**

  ```
  1，阻塞函数
  2，只能扫描到当前国家码信道列表里的ap
  3，ap_ary需要在该api内分配内存（扫描到多个ap，需要分配连续的内存空间）
  4，释放该内存由tuya sdk主动调用tuya_hal_wifi_release_ap释放
  ```

## 2-tuya_os_adapt_wifi_assign_ap_scan

```c
int tuya_os_adapt_wifi_assign_ap_scan(const signed char *ssid, AP_IF_S **ap)
```

- **函数描述**

  扫描当前环境指定ssid的ap信息


- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | ssid | 指定ssid |
  | [out] | ap | 扫描结果 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_AP_SCAN_FAILED ：扫描失败
  ```

- **备注**

  ```
  1，阻塞函数
  2，只能扫描到当前国家码信道列表里的ap
  3，当环境存在指定ssid的ap，扫描到该ssid的成功率要超过98%
  4，ap需要在该api内分配内存（扫描到多个ap，需要分配连续的内存空间）
  5，释放该内存由tuya sdk主动调用tuya_hal_wifi_release_ap释放
  ```

## 3-tuya_os_adapt_wifi_release_ap

```c
int tuya_os_adapt_wifi_release_ap(AP_IF_S *ap)
```

- **函数描述**

  释放ap信息资源


- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | ap | 需要释放的ap信息 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功 
  OPRT_OS_ADAPTER_AP_RELEASE_FAILED ：失败
  ```



## 4-tuya_os_adapt_wifi_set_cur_channel

```c
int tuya_os_adapt_wifi_set_cur_channel(const unsigned char chan)
```

- **函数描述**

  设置信道


- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | chan | 信道 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_CHAN_SET_FAILED ：失败
  ```

- **备注**

  ```
  1，设置当前国家码信道范围之外的信道报失败
  2，支持在sniffer回调里设置信道
  ```
  
## 5-tuya_os_adapt_wifi_get_cur_channel

```c
int tuya_os_adapt_wifi_get_cur_channel(unsigned char *chan)
```

- **函数描述**

  获取当前信道


- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [out] | chan | 信道 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_CHAN_GET_FAILED ：失败
  ```


## 6-tuya_os_adapt_wifi_get_ip

```c
int tuya_os_adapt_wifi_get_ip(const WF_IF_E wf, NW_IP_S *ip)
```

- **函数描述**

  获取wifi的ip信息（ip地址、网关地址、地址掩码）


- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | wf | wifi模式 |
  | [out] | ip | ip信息 |
  |--------|--------|
  | WF_STATION | station type|
  | WF_AP | ap type |
  
- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_IP_GET_FAILED ：失败
  ```

- **备注**
  需要区分不同模式（依据传参wf）下的ip信息
  

## 7-tuya_os_adapt_wifi_set_mac

```c
int tuya_os_adapt_wifi_set_mac(const WF_IF_E wf, const NW_MAC_S *mac)
```

- **函数描述**

  设置mac地址


- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | wf | wifi模式 |
  | [in] | mac | mac地址 |
  |--------|--------|
  | WF_STATION | station type|
  | WF_AP | ap type |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_MAC_SET_FAILED：失败
  ```


- **备注**

  永久生效，断电不丢失
  
## 8-tuya_os_adapt_wifi_get_mac

```c
int tuya_os_adapt_wifi_get_mac(const WF_IF_E wf, NW_MAC_S *mac)
```

- **函数描述**

  获取mac地址


- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | wf | wifi模式 |
  | [out] | mac | mac地址 |
  |--------|--------|
  | WF_STATION | station type|
  | WF_AP | ap type |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_MAC_GET_FAILED ：失败
  ```

- **备注**

  STATIONAP模式下的mac地址需要区分工作模式（依据传参wf）
  
## 9-tuya_os_adapt_wifi_set_work_mode

```c
int tuya_os_adapt_wifi_set_work_mode(const WF_WK_MD_E mode)
```

- **函数描述**

  设置wifi工作模式


- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | mode | wifi工作模式 |
  |--------|--------|
  | WWM_LOWPOWER | 低功耗（指关闭wifi模块）|
  | WWM_SNIFFER | monitor |
  | WWM_STATION | station |
  | WWM_SOFTAP | softap |
  | WWM_STATIONAP | stationap |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_WORKMODE_SET_FAILED ：失败
  ```

- **备注**

  ```
  1，从设备上电到调用tuya entrance要求在200ms以内。
  2，如果设备初始化时间超过200ms，底层可以做一些必要的初始化之后就调用tuya entrance，
     将耗时间的wifi初始化放在另一个线程执行。
  3，tuya_hal_wifi_set_work_mode是tuya业务使用wifi调用的第一个api，在这个api需要判断wifi初始化是否结束，
     如果没结束，要在这里等到结束才开始往下执行（只需要判断一次）。
  4，如果设备从上电到tuya entrance少于200ms，无需考虑这点。
  ```

## 10-tuya_os_adapt_wifi_get_work_mode

```c
int tuya_os_adapt_wifi_get_work_mode(WF_WK_MD_E *mode)
```

- **函数描述**

  获取wifi工作模式


- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [out] | mode | wifi工作模式 |
  |--------|--------|
  | WWM_LOWPOWER | 低功耗（指关闭wifi模块）|
  | WWM_SNIFFER | monitor |
  | WWM_STATION | station |
  | WWM_SOFTAP | softap |
  | WWM_STATIONAP | stationap |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_WORKMODE_GET_FAILED ：失败
  ```

## 11-tuya_os_adapt_wifi_sniffer_set

```c
int tuya_os_adapt_wifi_sniffer_set(const bool en, const SNIFFER_CALLBACK cb)
```

- **函数描述**

  设置sniffer功能开关


- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | en | 开或关sniffer功能 |
  | [in] | cb | 抓包数据回调 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_SNIFFER_SET_FAILED ：失败
  ```

- **备注**

  返给应用的数据要括管理包数据
  
## 12-tuya_os_adapt_wifi_ap_start

```c
int tuya_os_adapt_wifi_ap_start(const WF_AP_CFG_IF_S *cfg)
```

- **函数描述**

  启动ap模式


- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | cfg | ap配置参数 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_AP_START_FAILED ：失败
  ```
  
- **备注**

  ```
  1，需要根据max_conn来设置ap模式下最多允许几个sta能连接成功。
  2，需要根据ip信息来设置ap模式下的ip信息。
  3，stationap模式，如果ap port有连接的情况下staiton port连接路由器，连接过程以及连接
     失败后ap port不可以有断连情况以及可以收发广播包。
  4，配网ez+ap共存：
    4.1 softap或者stationap模式下可以使能sniffer功能以及可以切换信道。
    4.2 使能sniffer功能之后，扔给回调的数据要包括ap port的设备连接请求包。
  ```
  
## 13-tuya_os_adapt_wifi_ap_stop

```c
int tuya_os_adapt_wifi_ap_stop(void)
```

- **函数描述**

  停止ap模式


- **参数**

  无

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_AP_STOP_FAILED ：失败
  ```

- **备注**

  ```
  1，需要针对设备当前所处模式softap或stationap做不同的关闭逻辑
  2，stationap模式下，如果station port端有连接，关闭ap port过程station port不能有断连情况
  ```


## 14-tuya_os_adapt_wifi_get_connected_ap_info_v2

```c
int tuya_os_adapt_wifi_get_connected_ap_info_v2(FAST_WF_CONNECTED_AP_INFO_V2_S **fast_ap_info)
```

- **函数描述**

  获取所连ap信息，用于快连功能

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [out] | fast_ap_info | 所连ap信息 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_APINFO_GET_FAILED ：失败
  ```
  
- **备注**
  
  ```
  1，fast_ap_info 需要在该api内动态申请内存
  2, 每次重连路由器成功都会获取一次路由器信息，如有更新会同步到flash
  ```
  
## 15-tuya_os_adapt_wifi_fast_station_connect_v2

```c
int tuya_os_adapt_wifi_fast_station_connect_v2(const FAST_WF_CONNECTED_AP_INFO_V2_S *fast_ap_info)
```

- **函数描述**

  快速连接路由器

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | fast_ap_info | 快连所需的ap信息 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_FAST_CONN_FAILED ：失败
  ```
  
- **备注**

  配过网且重启之后第一次连接才会调用这个函数

## 16-tuya_os_adapt_wifi_station_connect

```c
int tuya_os_adapt_wifi_station_connect(const signed char *ssid, const signed char *passwd)
```

- **函数描述**

  连接路由器

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | ssid | ssid |
  | [in] | passwd | passwd |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_CONN_FAILED ：失败
  ```
  
- **备注**

  ```
  1，非阻塞，启动连接流程成功后，上层会每隔1s钟调用tuya_hal_wifi_station_get_status查询wifi连接状态
  2，需要使能自动重连功能，重连时间在1min以内即可，上层断连情况下会每隔1min发起一次重连
  ```

## 17-tuya_os_adapt_wifi_station_disconnect

```c
int tuya_os_adapt_wifi_station_disconnect(void)
```

- **函数描述**

  断开路由器

- **参数**

  无

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_DISCONN_FAILED ：失败
  ```
  
## 18-tuya_os_adapt_wifi_station_get_conn_ap_rssi

```c
int tuya_os_adapt_wifi_station_get_conn_ap_rssi(signed char *rssi)
```

- **函数描述**

  获取所连ap的信号强度

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [out] | rssi | 信号强度 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_RSSI_GET_FAILED ：失败
  ```
  
- **备注**

  ```
  1，不移动设备和路由器的位置，多次获取rssi只能出现小范围的波动
  2，移动设备和路由器的位置，获取rssi会有对应的变化
  ```
  
  
## 19-tuya_os_adapt_wifi_get_bssid

```c
int tuya_os_adapt_wifi_get_bssid(unsigned char *mac)
```

- **函数描述**

  获取所连ap的mac地址

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [out] | mac | mac地址 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_BSSID_GET_FAILED ：失败
  ```
  
## 20-tuya_os_adapt_wifi_station_get_status

```c
int tuya_os_adapt_wifi_station_get_status(WF_STATION_STAT_E *stat)
```

- **函数描述**

  获取wifi当前的连接状态

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [out] | stat | 连接状态 |
  |--------|--------|
  | WSS_IDLE |  |
  | WSS_CONNECTING | 连接中 |
  | WSS_PASSWD_WRONG | 密码错误 |
  | WSS_CONN_FAIL | 连接失败 |
  | WSS_CONN_SUCCESS | 连接成功 |
  | WSS_GOT_IP | dhcp成功 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_STAT_GET_FAILED ：失败
  ```

## 21-tuya_os_adapt_wifi_set_country_code

```c
int tuya_os_adapt_wifi_set_country_code(const COUNTRY_CODE_E ccode)
```

- **函数描述**

  设置国家码

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | ccode | 国家码 |
  |--------|--------|
  | COUNTRY_CODE_CN | 中国区 1-13 |
  | COUNTRY_CODE_US | 美国区 1-11 |
  | COUNTRY_CODE_JP | 日本区 1-14 |
  | COUNTRY_CODE_EU | 欧洲区 1-13 需要考虑欧洲自适应 |
  
- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_CCODE_SET_FAILED ：失败
  ```
  
- **备注**
  ```
  1，国家码暂时只需要支持中国区（CN 1-13）、美国区（US 1-11）、日本区（JP 1-14）、欧洲区（EU 1-13）
  2，针对不同的国家码，需要达到的要求如下：
    2.1 扫描路由器时只能扫到国家码对应信道列表的ap。
    2.2 欧洲区需要实现欧洲自适应功能
    2.3 设置信道时，设置的信道不在当前国家码的信道列表范围之内，设置信道返回error
  ```


## 22-tuya_os_adapt_wifi_send_mgnt

```c
int tuya_os_adapt_wifi_send_mgnt(const unsigned char *buf, const unsigned int len)
```

- **函数描述**

  发送管理包数据

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | buf | 管理包数据buf |
  | [in] | len | 管理包数据长度 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_MGNT_SEND_FAILED ：失败
  ```
  
## 23-tuya_os_adapt_wifi_register_recv_mgnt_callback

```c
int tuya_os_adapt_wifi_register_recv_mgnt_callback(const bool enable, const WIFI_REV_MGNT_CB recv_cb)
```

- **函数描述**

  设置应用层是否接收管理包数据

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | enable |  是否开启接收管理包数据
  | [in] | recv_cb | 接收管理包数据回调 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_MGNT_REG_FAILED ：失败
  ```
  
## 24-tuya_os_adapt_set_wifi_lp_mode

```c
int tuya_os_adapt_set_wifi_lp_mode(const bool en, const unsigned int dtim)
```

- **函数描述**

  wifi低功耗设置

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | en |  是否开启wifi低功耗模式|
  | [in] | dtim | dtim参数 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_WF_LPMODE_SET_FAILED ：失败
  ```
## 25-tuya_os_adapt_wifi_lp_mode

```c
bool tuya_os_adapt_wifi_rf_calibrated(void)
```

- **函数描述**

  检查RF是否校准过

- **参数**

  无

- **返回值**

  ```
  true : 已校准
  false ：未校准
  ```
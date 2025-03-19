# 索引

  * [1 tkl_subg_rf_init](#1-tkl_subg_rf_init)
  * [2 tkl_subg_rf_reinit](#2-tkl_subg_rf_reinit)
  * [3 tkl_subg_rf_get_state](#3-tkl_subg_rf_get_state)
  * [4 tkl_subg_send](#4-tkl_subg_send)
  * [5 tkl_subg_rf_goto_state](#5-tkl_subg_rf_goto_state)
  * [6 tkl_subg_rf_rssi_get](#6-tkl_subg_rf_rssi_get)
  * [7 tkl_subg_rf_set_radio_power](#7-tkl_subg_rf_set_radio_power)
  * [8 tkl_subg_rf_get_radio_power](#8-tkl_subg_rf_get_radio_power)
  * [9 tkl_subg_rf_set_channel](#9-tkl_subg_rf_set_channel)
  * [10 tkl_subg_rf_get_channel](#10-tkl_subg_rf_get_channel)
  * [11 tkl_subg_rf_rx_register](#11-tkl_subg_rf_rx_register)
  * [12 tkl_subg_rf_tx_register](#12-tkl_subg_rf_tx_register)
  * [13 tkl_subg_rf_set_frequency_deviation](#13-tkl_subg_rf_set_frequency_deviation)

------
# APIs

## 1-tkl_subg_rf_init

```c
OPERATE_RET tkl_subg_rf_init(UINT32_T cfg_type)
```

- **函数描述**

  初始化配置Subg RF功能模块
  
- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | cfg_type | 模块使用的配置序号，默认0 |

- **返回值**

  ```
  OPRT_OK : 成功
  Others ：失败
  ```

  
  
## 2-tkl_subg_rf_reinit

```c
OPERATE_RET tkl_subg_rf_reinit( VOID_T )
```

- **函数描述**

  重置Subg RF功能模块到上电默认状态


- **参数**

  无

- **返回值**

  ```
  OPRT_OK : 成功
  Others ：失败
  ```
  
## 3-tkl_subg_rf_get_state

```c
SUBG_RF_STATE_E tkl_subg_rf_get_state( VOID_T )
```

- **函数描述**

  查询SubG RF的当前状态


- **参数**

  无

- **返回值**

  ```
    SUBG_RF_STATE_IDLE:   RF当前闲置休眠状态
    SUBG_RF_STATE_TX:     RF当前准备发射数据
    SUBG_RF_STATE_TX_ING: RF当前正在发射数据
    SUBG_RF_STATE_RX:     RF当前开启接收窗口
    SUBG_RF_STATE_RX_ING: RF当前接收窗口有数据正在接收
  ```
  
## 4-tkl_subg_send

```c
OPERATE_RET tkl_subg_send(UINT8_T *tx_buf, UINT8_T buf_len,SUBG_RF_PACK_TYPE_E pack_type );

```

- **函数描述**

  SubG RF发射数据


- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | tx_buf | 发射的数据包指针 |
  | [in] | buf_len | 发射的数据包数据长度，单位字节 |
  | [in] | pack_type | 发射包使用的帧类型,子设备默认使用SUBG_RF_PACK_DYNAMIC类型 |

- **返回值**

  ```
  OPRT_OK : 成功
  Others ：失败
  ```
  
- **备注**

  ```
  1、帧类型定义pack_type：
      SUBG_RF_PACK_DYNAMIC：动态长度数据帧类型；
      SUBG_RF_PACK_DYNAMIC_WAKEUP：带唤醒前导码的动态长度数据帧类型；
      SUBG_RF_PACK_FIXED：确定长度帧类型，常应用于射频遥控器FIFO发送；
      SUBG_RF_PACK_CARRIRE：载波测试发射；
  2、发射时该接口为未阻塞式发射，配合tkl_subg_rf_tx_register注册的回调实现发射结果的回调。
  ```
  
   
## 5-tkl_subg_rf_goto_state

```c
OPERATE_RET tkl_subg_rf_goto_state(SUBG_RF_WORK_STATE_E state);

```

- **函数描述**

  SubG RF 工作状态设置

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | state | SUBG_RF_RX_FULL：全窗口接收数据<br>SUBG_RF_RX_DUTYCYCLE：占空比接收数据<br>SUBG_RF_IDLE：闲置休眠  SUBG_RF_SLEEP_UNIVERSAL：深度休眠<br>SUBG_RF_SLEEP_DETECT_RSSI：深度休眠开启RSSI检测唤醒（部分芯片支持）|

- **返回值**

  ```
  OPRT_OK : 成功
  Others ：失败
  ```
  
  
## 6-tkl_subg_rf_rssi_get

```c
SCHAR_T tkl_subg_rf_rssi_get( VOID_T )
```

- **函数描述**

  获取当前信道的信号强度


- **参数**

  无

- **返回值**

  ```
  SCHAR_T 信号强度值
  ```

- **备注**

  ```
  1、仅在RF处于SUBG_RF_RX_FULL状态期间可调用。
  ```
  
## 7-tkl_subg_rf_set_radio_power

```c
OPERATE_RET tkl_subg_rf_set_radio_power(INT8_T radio_power_dbm)
```

- **函数描述**

  设置RF的发射功率值


- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | radio_power_dbm | 功率大小，单位dbm |

- **返回值**

  ```
  OPRT_OK : 成功
  Others ：失败
  ```
  
## 8-tkl_subg_rf_get_radio_power

```c
INT8_T tkl_subg_rf_get_radio_power( VOID_T )
```

- **函数描述**

  查询当前设备的发射功率


- **参数**

  无

- **返回值**

  ```
  INT8_T 发射功率大小，单位dbm
  ```
  
  
  
## 9-tkl_subg_rf_set_channel

```c
VOID_T tkl_subg_rf_set_channel(UINT8_T channel)
```

- **函数描述**

  设置RF的工作信道


- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | channel | 信道号 |
  
- **返回值**

  ```
  无
  ```

- **备注**
  需要区分不同模式（依据传参wf）下的ip信息
  
  
  
  
## 10-tkl_subg_rf_get_channel

```c
UINT8_T tkl_subg_rf_get_channel( VOID_T )
```

- **函数描述**

  查询RF的工作信道


- **参数**

  无

- **返回值**

  ```
 UINT8_T 信道号
  ```
  
  

  
## 11-tkl_subg_rf_rx_register

```c
VOID_T tkl_subg_rf_rx_register(SUBG_RF_REVIECE fun_cb)
```

- **函数描述**

  注册SubG RF 接收回调


- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | fun_cb | 接收回调函数 |

- **返回值**

  无


  
## 12-tkl_subg_rf_tx_register

```c
VOID_T tkl_subg_rf_tx_register(SUBG_RF_EVENT fun_cb)
```

- **函数描述**

  注册SubG RF 发射结果回调


- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | fun_cb | 发射结果回调函数 |

- **返回值**

  无


## 13-tkl_subg_rf_set_frequency_deviation

```c
VOID_T tkl_subg_rf_set_frequency_deviation(INT16_T temperature)
```

- **函数描述**

  根据温度，配置频偏（部分芯片支持）


- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | temperature | 芯片环境温度 |

- **返回值**

  无
  
   

# 索引

  * [1 pin_os_adapt_init](#1-pin_os_adapt_init)
  * [2 pin_os_adapt_read](#2-pin_os_adapt_read)
  * [3 pin_os_adapt_write](#3-pin_os_adapt_write)
  * [4 pin_os_adapt_toggle](#4-pin_os_adapt_toggle)
  * [5 pin_os_adapt_control](#5-pin_os_adapt_control)

------

## 1-pin_os_adapt_init

```c
static int pin_os_adapt_init(tuya_pin_name_t pin, tuya_pin_mode_t mode)
```

- **函数描述**

  GPIO初始化设置
  | mode | —— |
  |--------|--------|
  | TY_GPIO_PULLUP | 上拉模式 |
  | TY_GPIO_PULLDOWN | 下拉模式 |
  | TY_GPIO_PULLUP_PULLDOWN | 推挽输出模式 |
  | TY_GPIO_OPENDRAIN | 开漏输出模式 |
  | TY_GPIO_FLOATING | 浮空输入模式 |
- **参数**

  | 输入/输出 | 参数名 | 描述 |
  |--------|--------|--------|
  | [in] | pin | GPIO口 |
  | [in] | mode | 0:输出;1:输入 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_MALLOC_FAILED ：申请内存失败
  ```
   
## 2-pin_os_adapt_read

```c
static int pin_os_adapt_read(tuya_pin_name_t pin)
```

- **函数描述**

  读取GPIO口电平状态函数

- **参数**

  | 输入/输出 | 参数名 | 描述 |
  |--------|--------|--------|
  | [in] | port | GPIO口 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  ```
  
## 3-pin_os_adapt_write

```c
static int pin_os_adapt_write(tuya_pin_name_t pin, tuya_pin_level_t level)
```

- **函数描述**

  设置GPIO口电平状态函数

- **参数**

  | 输入/输出 | 参数名 | 描述 |
  |--------|--------|--------|
  | [in] | port | GPIO口 |
  | [in] | high | 电平状态:0:低电平;1:高电平 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_GPIO_WRITE_FAILED ：失败
  ```
## 5-pin_os_adapt_toggle

```c
static int pin_os_adapt_toggle(tuya_pin_name_t pin)
```

- **函数描述**

  GPIO电平翻转函数

- **参数**

  | 输入/输出 | 参数名 | 描述 |
  |--------|--------|--------|
  | [in] | pin | GPIO口 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  ```
## 5-pin_os_adapt_control

```c
static int pin_os_adapt_control(tuya_pin_name_t pin, uint8_t cmd, void *arg)
```

- **函数描述**

  初始化GPIO口中断函数
  | CMD | —— |
  |--------|--------|
  | TUYA_DRV_SET_INT_CMD | 中断使能 |
  | TUYA_DRV_CLR_INT_CMD | 禁止中断 |
  | TUYA_DRV_SET_ISR_CMD | 分配中断回调函数 |
  
  | trig_type | —— |
  |--------|--------|
  | TY_IRQ_NONE |  |
  | TY_IRQ_RISE | 上升沿触发 |
  | TY_IRQ_FALL | 下降沿触发 |
  | TY_TRQ_BOTH | 双边沿触发 |
  | TY_IRQ_HIGH | 高电平触发 |
  | TY_IRQ_LOW  | 低电平触发 |

- **参数**

  | 输入/输出 | 参数名 | 描述 |
  |--------|--------|--------|
  | [in] | port | GPIO口 |
  | [in] | cb | 分配给GPIO的中断回调函数 |
  | [in] | trig_type | 中断触发源类型 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_GPIO_IRQ_INIT_FAILED ：失败
  ```
  
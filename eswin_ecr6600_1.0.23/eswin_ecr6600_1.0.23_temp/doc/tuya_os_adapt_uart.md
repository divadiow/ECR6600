# 索引

  * [1 uart_os_adapt_init](#1-uart_os_adaptdev_init)
  * [2 uart_os_adapt_deinit](#2-uart_os_adapt_deinit) 
  * [3 uart_os_adapt_write_byte](#3-uart_os_adapt_write_byte)
  * [4 uart_os_adapt_read_byte](#4-uart_os_adapt_read_byte)
  * [5 uart_os_adapt_control](#5-uart_os_adapt_control)

------

##说明
```c
typedef enum {
    TY_UART0 = 0x00,
    TY_UART1,
    TY_UART2,
    TY_UART3,
    TY_UART_NUM,
} TY_UART_PORT_E;

备注：涂鸦上层授权、用户对接用的串口是TY_UART0,如果涂鸦设定的串口对应关系和芯片平台有冲突，请在适配层做调整。
```


## 1-uart_os_adapt_init

```c
static int uart_os_adapt_init(tuya_uart_t *uart, tuya_uart_cfg_t *cfg)
```

- **函数描述**

  串口初始化

- **参数** 

  | 输入/输出 | 参数名 | 描述 |
  |--------|--------|--------|
  | [in] | uart | 串口句柄，需要初始化相关参数 |
  | [in] | arg | 配置参数 |

- **返回值**
  
  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_MALLOC_FAILED ：申请内存失败
  OPRT_OS_ADAPTER_UART_INIT_FAILED ：失败
  ```


## 2-uart_os_adapt_deinit

```c
static int uart_os_adapt_deinit(tuya_uart_t *uart)
```

- **函数描述**

  串口反初始化函数

- **参数** 

  | 输入/输出 | 参数名 | 描述 |
  |--------|--------|--------|
  | [in] | uart | 串口句柄 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_UART_DEINIT_FAILED ：失败
  ```


## 3-uart_os_adapt_write_byte

```c
static int uart_os_adapt_write_byte(tuya_uart_t *uart, uint8_t byte)
```

- **函数描述**

  串口数据发送函数

- **参数** 

  | 输入/输出 | 参数名 | 描述 |
  |--------|--------|--------|
  | [in] | uart | 串口 |
  | [in] | byte | 要发送的字节 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_UART_SEND_FAILED ：失败
  ```

  
### 4-uart_os_adapt_read_byte

```c
static int uart_os_adapt_read_byte(tuya_uart_t *uart, uint8_t *byte)
```

- **函数描述**

  串口读取一个字节数据

- **参数**

  | 输入/输出 | 参数名 | 描述 |
  |--------|--------|--------|
  | [in] | uart | 串口 |
  | [out] | byte | 接收数据的指针 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_UART_READ_FAILED ：失败
  ```
## 5-uart_os_adapt_control

```c
static int pin_os_adapt_control(tuya_pin_name_t pin, uint8_t cmd, void *arg)
```

- **函数描述**
  串口中断控制函数
  | cmd | —— |
  |--------|--------|
  | TUYA_DRV_SET_INT_CMD | 中断使能 |
  | TUYA_DRV_CLR_INT_CMD | 禁用中断 |
  | TUYA_DRV_SET_ISR_CMD | 分配中断回调函数 |
  | trig_type| —— |
  |--------|--------|
  | TUYA_DRV_INT_RX_FLAG | 串口接收中断标志 |
  | TUYA_DRV_INT_TX_FLAG | 串口发送中断标志 |

- **参数**

  | 输入/输出 | 参数名 | 描述 |
  |--------|--------|--------|
  | [in] | uart | 串口 |
  | [in] | cmd | 接收数据的指针 |
  | [in] | arg | 接收数据的指针 |
- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_UART_READ_FAILED ：失败
  ```
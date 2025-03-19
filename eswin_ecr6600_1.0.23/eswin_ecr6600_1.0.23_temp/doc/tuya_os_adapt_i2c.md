# 索引

  * [1 i2c_os_adapt_init](#1-i2c_os_adapt_init)
  * [2 i2c_os_adapt_xfer](#2-i2c_os_adapt_xfer)
------

## 1-i2c_devos_adapt_init

```c
int i2c_os_adapt_init(tuya_i2c_t *i2c, tuya_i2c_cfg_t *cfg)
```

- **函数描述**

  I2C接口初始化
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | i2c | i2c接口句柄 |
  | [in] | cfg | i2c接口配置参数结构体 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  ```

## 2-i2c_os_adapt_xfer

```c
static int i2c_os_adapt_xfer(tuya_i2c_t *i2c, tuya_i2c_msg_t *msgs, uint8_t num)
```

- **函数描述**

  I2C读、写数据
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | i2c  | I2C接口句柄 |
  | [in] | msgs | 与读取数据信息相关指针 |
  | [in] | num  | msgs数（I2C读写次数） |
- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  ```

# PINMUX 驱动

# 简要说明

pinmux用于外设管脚的映射及复用。

# Api描述

## tkl_io_pinmux_config

```
OPERATE_RET tkl_io_pinmux_config(TUYA_PIN_NAME_E pin, TUYA_PIN_FUNC_E pin_func);
```

- 功能描述:
  - 配置IO管脚功能。
- 参数:
  - `pin`: 管脚号。
  - `pin_func`:管脚功能 。
- 返回值:
  - 错误码，参考文件tuya_error_code.h。

## tkl_multi_io_pinmux_config

```
OPERATE_RET tkl_multi_io_pinmux_config(TUYA_MUL_PIN_CFG_T *cfg, UINT16_T num);
```

- 功能描述:

  - pinmux多路配置IO管脚功能。

- 参数:

  - `cfg`:配置结构体指针 。

    **TUYA_MUL_PIN_CFG_T**

    ```
    typedef  struct {
        TUYA_PIN_NAME_E pin;			//管脚号
        TUYA_PIN_FUNC_E pin_func;		//管脚功能
    }TUYA_MUL_PIN_CFG_T;
    ```

  - `num`:配置数量。

- 返回值:

  - 错误码，参考文件tuya_error_code.h。

## tkl_io_pin_to_func

```
INT32_T tkl_io_pin_to_func(UINT32_T pin, TUYA_PIN_TYPE_E pin_type);
```

- 功能描述:
  - 从IO管脚到功port和channel号的查询。
- 参数:
  - `pin`: 管脚号。
  - `pin_func`:查询类型。
- 返回值:
  - 管脚对应port和channel号。
  - 其中bit0-bit7位对应channel(通道)，bit8-bit15对应port(端口)。
  - <0，未查询到。

# 示例

```c
 tkl_io_pinmux_config(TUYA_IO_PIN_0, TUYA_IIC0_SCL);
 tkl_io_pinmux_config(TUYA_IO_PIN_1, TUYA_IIC0_SDA);
```

```
 TUYA_MUL_PIN_CFG_T cfg[2];
 cfg[0].pin = TUYA_IO_PIN_0;
 cfg[0].pin_func = TUYA_IIC0_SCL;
 
 cfg[1].pin = TUYA_IO_PIN_1;
 cfg[1].pin_func = TUYA_IIC0_SDA;

 tkl_multi_io_pinmux_config(cfg,2);
```


# REGISTER 驱动

# 简要说明

register驱动是涂鸦统一的寄存器操作接口，用于对芯片内部寄存器的读取与写入。

# Api描述

## 1.tkl_reg_read

```
UINT32 tkl_reg_read(UINT32_T addr);
```

- 功能描述:
  - 读取寄存器数据。
- 参数:
  - `addr`: 寄存器地址。
- 返回值:
  - 寄存器数据。

## 2.tkl_reg_bit_read

```
UINT32 tkl_reg_bit_read(UINT32_T addr, TUYA_ADDR_BITS_DEF_E start_bit, TUYA_ADDR_BITS_DEF_E end_bit);
```

- 功能描述:
  - 读取寄存器对应bit的数据。
- 参数:
  - `addr`: 寄存器地址。
  - `start_bit`: 寄存器起始bit位，参考TUYA_ADDR_BITS_DEF_E定义。
  - `end_bit`: 寄存器结束bit位，参考TUYA_ADDR_BITS_DEF_E定义。
- 返回值:
  - 寄存器对应bit位的数据。

## 3.tkl_reg_write

```
OPERATE_RET tkl_reg_write(UINT32_T addr, UINT32_T data);
```

- 功能描述:
  - 写入寄存器数据。
- 参数:
  - `addr`: 写入的寄存器地址。
  - `data`: 要写入的数据。
- 返回值:
  - OPRT_OK 成功，其他请参考文件tuya_error_code.h。

## 4.tkl_reg_bit_write

```
OPERATE_RET tkl_reg_bit_write(UINT32_T addr, TUYA_ADDR_BITS_DEF_E start_bit, TUYA_ADDR_BITS_DEF_E end_bit, UINT32_T data);
```

- 功能描述:
  - 写入寄存器数据。
- 参数:
  - `addr`: 写入的寄存器地址。
  - `start_bit`: 寄存器起始bit位，参考TUYA_ADDR_BITS_DEF_E定义。
  - `end_bit`: 寄存器结束bit位，参考TUYA_ADDR_BITS_DEF_E定义。
  - `data`: 要写入的数据。
- 返回值:
  - OPRT_OK 成功，其他请参考文件tuya_error_code.h。


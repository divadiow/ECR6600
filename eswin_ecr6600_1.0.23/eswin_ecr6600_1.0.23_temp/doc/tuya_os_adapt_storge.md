# 索引

  * [1 tuya_os_adapt_flash_read](#1-tuya_os_adapt_flash_read)
  * [2 tuya_os_adapt_flash_write](#2-tuya_os_adapt_flash_write)
  * [3 tuya_os_adapt_flash_erase](#3-tuya_os_adapt_flash_erase)
  * [4 tuya_os_adapt_storage_get_desc](#4-tuya_os_adapt_storage_get_desc)
  * [5 tuya_os_adapt_uf_get_desc](#5-tuya_os_adapt_uf_get_desc)
  * [6 tuya_os_adapt_legacy_swap_get_desc](#6-tuya_os_adapt_legacy_swap_get_desc)
------

## 1-tuya_os_adapt_flash_read

```c
int tuya_os_adapt_flash_read(const unsigned int addr, unsigned char *dst, const unsigned int size)
```

- **函数描述**

  从flash读取数据

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | addr | 读取的flash首地址 |
  | [out] | dst | 接收读取数据的buffer指针 |
  | [in] | size | 读取的buffer大小 |

- **返回值**

  0 : 成功； 其他：失败错误码
  
- **备注**

  ```
  1，由于tuya上层业务可能出现多线程同时调用tuya_hal_flash层的api接口，因此tuya_hal_flash_read，tuya_hal_flash_write，tuya_hal_flash_erase三个操作函数需要共用互斥锁。
  若调用的芯片flash api本身具有互斥性，则无需再加锁；若调用的芯片flash api本身不具互斥性，则需在tuya_hal_flash层添加互斥锁。
  2，如果支持片上运行，在操作flash可能会导致系统tick不准，在操作flash后需要弥补系统tick（重要）。
  3，读一个扇区时间在3ms以内。
  ```

## 2-tuya_os_adapt_flash_write

```c
int tuya_os_adapt_flash_write(const unsigned int addr, const unsigned char *src, const unsigned int size)
```

- **函数描述**

  向flash写入数据
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | addr | 写入的flash首地址 |
  | [in] | dst | 写入数据的buffer指针 |
  | [in] | size | 写入的buffer大小 |

- **返回值**

  0 : 成功； 其他：失败错误码
  
- **备注**

  写一个扇区时间在20ms以内。

## 3-tuya_os_adapt_flash_erase

```c
int tuya_os_adapt_flash_erase(const unsigned int addr, const unsigned int size)
```

- **函数描述**

  擦除flash块
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | addr | 擦除的flash首地址 |
  | [in] | size | 擦除的flash大小 |

- **返回值**

  0 : 成功； 其他：失败错误码
  
- **备注**
  
  擦一个扇区时间在65ms以内。

## 4-tuya_os_adapt_storage_get_desc

```c
UNI_STORAGE_DESC_S *tuya_os_adapt_storage_get_desc(VOID)
```

- **函数描述**

  获取芯片sdk为涂鸦应用数据划分的KV(Key-Value)-flash分区信息。


- **参数**

  无

- **返回值**

  UNI_STORAGE_DESC_S* : KV-flash分区信息结构体指针

- **备注**
  1) 可以在tuya_hal_storge.c中定义一个静态结构体变量，在其中描述芯片为tuya应用数据划分的KV-flash分区信息。例如
```
    static UNI_STORAGE_DESC_S storage = {
        SIMPLE_FLASH_START,
        SIMPLE_FLASH_SIZE,
        PARTITION_SIZE,
        SIMPLE_FLASH_SWAP_START,
        SIMPLE_FLASH_SWAP_SIZE,
        SIMPLE_FLASH_KEY_ADDR
    };
    
    UNI_STORAGE_DESC_S* tuya_hal_storage_get_desc(void)
    {
        return &storage;
    }
```
  
  2) 推荐分区：

      | 分区名 |  描述 | 长度（推荐值）|说明|
      |--------|--------|--------|--------|--------|
      | SIMPLE_FLASH| KV数据 | 0x10000(64K) | 最少32K |
      | SIMPLE_FLASH_SWAP| KV数据的备份区 | 0x4000(16K) | 最少12K |
      | SIMPLE_FLASH_KEY_ADDR| KV加密密钥存储区 | 0x1000(4K) | 大小不可更改 |
  
  3) KV flash定好flash分区之后不能更改，如有改动不支持向前兼容。
  4）tuya 授权信息存放在KV区。

## 5-tuya_os_adapt_uf_get_desc

```c
UF_PARTITION_TABLE_S *tuya_os_adapt_uf_get_desc(VOID)
```

- **函数描述**

  获取芯片sdk为涂鸦应用数据划分的uf_file-flash分区信息。

- **参数**

  无

- **返回值**

  UF_PARTITION_TABLE_S* : UF_file-flash分区信息结构体指针

- **备注**

  1) tuya uf-file是一种类linux文件IO操作api（open,write,read,close等）的flash存储组件，支持应用层通过uf_file.h中的接口操作flash。
  2) 可以在tuya_hal_storge.c中定义一个静态结构体变量，在其中描述芯片为tuya应用数据划分的uf_file-flash分区信息。例如
```c
    static UF_PARTITION_TABLE_S uf_file = {
        .sector_size = PARTITION_SIZE,
        .uf_partition_num = 2,
        .uf_partition = {
            {xx, 0x8000},
            {xx, 0x18000}
        }
    };
```
  3) 推荐分区(几个分区合起来的大小不小于128K)：

  | 分区名 |  描述  | 长度（推荐值）|说明|
  |--------|--------|--------|--------|--------|
  | UF_FILE_1| UF_FILE数据1区 | 0x8000(32K) | 32K，起始位置可以调整 |
  | UF_FILE_2| UF_FILE数据2区 | 0x18000(96K) | 96K，起始位置可以调整 |

## 6-tuya_os_adapt_legacy_swap_get_desc

```c
int tuya_os_adapt_legacy_swap_get_desc(unsigned int *addr, unsigned int *size)
```

- **函数描述**

  获取芯片sdk为涂鸦应用数据划分的swap分区信息


- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | addr | swap flash首地址 |
  | [in] | size | swap分区大小 |

- **返回值**

  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM：传参无效
  OPRT_OS_ADAPTER_NOT_SUPPORTED：不支持
# 索引

  * [1 tuya_os_adapt_system_malloc](#1-tuya_os_adapt_system_malloc)
  * [2 tuya_os_adapt_system_free](#2-tuya_os_adapt_system_free)

------

## 1-tuya_os_adapt_system_malloc

```c
void *tuya_os_adapt_system_malloc(const size_t size)
```

- **函数描述**

  内存分配
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | size | 分配内存大小 |

- **返回值**

  ```
  NULL : 内存分配失败
  其他：分配成功
  ```
  
- **备注**

  适配层其它文件API需要申请内存，一律调用tuya_hal_system_malloc

## 2-tuya_os_adapt_system_free

```c
void tuya_os_adapt_system_free(void* ptr)
```

- **函数描述**

  内存释放
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | ptr | 释放内存的地址 |

- **返回值**

  无
  
- **备注**

  适配层其它文件API需要释放内存，一律调用tuya_hal_system_free
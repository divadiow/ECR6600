# 索引

  * [1 tuya_os_adapt_mutex_create_init](#1-tuya_os_adapt_mutex_create_init)
  * [2 tuya_os_adapt_mutex_release](#2-tuya_os_adapt_mutex_release)
  * [3 tuya_os_adapt_mutex_lock](#3-tuya_os_adapt_mutex_lock)
  * [4 tuya_os_adapt_mutex_unlock](#4-tuya_os_adapt_mutex_unlock)
------

## 1-tuya_os_adapt_mutex_create_init

```c
int tuya_os_adapt_mutex_create_init(MUTEX_HANDLE *pMutexHandle)
```

- **函数描述**

  创建并初始化互斥锁
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [out] | pMutexHandle | 互斥锁句柄的指针 |

- **返回值**

  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_MALLOC_FAILED ：申请内存失败
  OPRT_OS_ADAPTER_MUTEX_CREAT_FAILED ：失败


## 2-tuya_os_adapt_mutex_release

```c
int tuya_os_adapt_mutex_release(const MUTEX_HANDLE mutexHandle)
```

- **函数描述**

  释放互斥锁资源
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | mutexHandle | 信号量句柄 |

- **返回值**

  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_MUTEX_RELEASE_FAILED ：失败



## 3-tuya_os_adapt_mutex_lock

```c
int tuya_os_adapt_mutex_lock(const MUTEX_HANDLE mutexHandle)
```

- **函数描述**

  加锁
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | mutexHandle | 信号量句柄 |

- **返回值**

  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_MUTEX_LOCK_FAILED ：失败

## 4-tuya_os_adapt_mutex_unlock

```c
int tuya_os_adapt_mutex_unlock(const MUTEX_HANDLE mutexHandle)
```

- **函数描述**

  解锁
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | mutexHandle | 信号量句柄 |

- **返回值**

  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_MUTEX_UNLOCK_FAILED ：失败
  
- **备注**

  需要区别线程上下文和中断上下文
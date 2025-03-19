# 索引

  * [1 tuya_os_adapt_semaphore_create_init](#1-tuya_os_adapt_semaphore_create_init)
  * [2 tuya_os_adapt_semaphore_wait](#2-tuya_os_adapt_semaphore_wait)
  * [3 tuya_os_adapt_semaphore_post](#3-tuya_os_adapt_semaphore_post)
  * [4 tuya_os_adapt_semaphore_release](#4-tuya_os_adapt_semaphore_release)
  * [5 tuya_os_adapt_semaphore_waittimeout](#5-tuya_os_adapt_semaphore_waittimeout)
------

## 1-tuya_os_adapt_semaphore_create_init

```c
int tuya_os_adapt_semaphore_create_init(SEM_HANDLE *pHandle, const unsigned int semCnt, \
const unsigned int sem_max)
```

- **函数描述**

  创建并初始化信号量
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [out] | pHandle | 信号量句柄的指针 |
  | [in] | semCnt | 初始化信号量个数 |
  | [in] | sem_max | 最大信号量数 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_MALLOC_FAILED ：申请内存失败
  OPRT_OS_ADAPTER_THRD_CREAT_FAILED ：失败
  ```


## 2-tuya_os_adapt_semaphore_wait

```c
int tuya_os_adapt_semaphore_wait(const SEM_HANDLE semHandle)
```

- **函数描述**

  等待信号量
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | semHandle | 信号量句柄 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_SEM_WAIT_FAILED ：失败
  ```


## 3-tuya_os_adapt_semaphore_post

```c
int tuya_os_adapt_semaphore_post(const SEM_HANDLE semHandle)
```

- **函数描述**

  发送信号量
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | mutexHandle | 信号量句柄 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_SEM_POST_FAILED ：失败
  ```
  
- **备注**

  需要区别线程上下文和中断上下文



## 4-tuya_os_adapt_semaphore_release

```c
int tuya_os_adapt_semaphore_release(const SEM_HANDLE semHandle)
```

- **函数描述**

  释放信号量资源

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | semHandle | 信号量句柄 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_SEM_RELEASE_FAILED ：失败
  ```
  ## 5-tuya_os_adapt_semaphore_waittimeout

```c
int tuya_os_adapt_semaphore_waittimeout(IN const SEM_HANDLE semHandle, unsigned int timeout)
```

- **函数描述**

  设置信号量等待超时

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | semHandle | 信号量句柄 |
  | [in] | timeout | 超时时间 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_SEM_WAIT_FAILED：信号量等待失败
  ```
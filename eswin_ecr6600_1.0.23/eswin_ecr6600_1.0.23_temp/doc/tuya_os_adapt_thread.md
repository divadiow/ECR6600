# 索引

  * [1 tuya_os_adapt_thread_create](#1-tuya_os_adapt_thread_create)
  * [2 tuya_os_adapt_thread_release](#2-tuya_os_adapt_thread_release)
  * [3-tuya_os_adapt_thread_is_self](3-tuya_os_adapt_thread_is_self)
  * [4-tuya_os_adapt_thread_get_watermark](4-tuya_os_adapt_thread_get_watermark)
------

## 1-tuya_os_adapt_thread_create

```c
int tuya_os_adapt_thread_create(THREAD_HANDLE* thread,const char* name,\const unsigned int stack_size,const unsigned int priority,const THREAD_FUNC_T funcconst void* arg)
```

- **函数描述**

  创建并启动线程


- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [out] | thread | 线程句柄的指针 |
  | [in] | name | 线程名字 |
  | [in] | stack_size | 线程堆栈大小,单位是byte,而不是4字节 |
  | [in] | priority | 线程优先级 |
  | [in] | func | 线程函数 |
  | [in] | arg | 用户参数 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 创建成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_MALLOC_FAILED ：申请内存失败
  OPRT_OS_ADAPTER_THRD_CREAT_FAILED ：失败
  ```

- **备注**

  ```
  1，stack_size参数是1个byte为单位
  2，tuya业务会使用0、1、2、3、4、5六种线程优先级（优先级由低到高）
  ```


## 2-tuya_os_adapt_thread_release

```c
int tuya_os_adapt_thread_release(THREAD_HANDLE thread)
```

- **函数描述**

  释放线程资源
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | thread | 信号量句柄 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_THRD_RELEASE_FAILED ：失败
  ```
  
## 3-tuya_os_adapt_thread_is_self

```c
int tuya_os_adapt_thread_is_self(THREAD_HANDLE thread, bool* is_self)
```

- **函数描述**

  判断运行上下文是否在指定线程里
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | thread | 信号量句柄 |
  | [out] | is_self | 是否在指定线程 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误  
  OPRT_OS_ADAPTER_THRD_JUDGE_SELF_FAILED ：失败
  ```
## 4-tuya_os_adapt_thread_is_self

```c
int tuya_os_adapt_thread_get_watermark(THREAD_HANDLE thread, UINT_T* watermark)
```

- **函数描述**

  获取任务的剩余栈空间大小
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | thread | 线程句柄 |
  | [out] | watermark | 剩余栈空间大小 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误  
  ```
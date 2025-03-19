# 索引

  * [1 tuya_os_adapt_queue_create_init](#1-tuya_os_adapt_queue_create_init)
  * [2 tuya_os_adapt_queue_free](#2-tuya_os_adapt_queue_free)
  * [3 tuya_os_adapt_queue_post](#3-tuya_os_adapt_queue_post)
  * [4 tuya_os_adapt_queue_fetch](#4-tuya_os_adapt_queue_fetch)
------

## 1-tuya_os_adapt_queue_create_init

```c
int tuya_os_adapt_queue_create_init(QUEUE_HANDLE *queue, int size)
```

- **函数描述**

  创建并初始化队列
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [out] | queue | 待创建队列 |
  | [in] | size | 创建队列的深度 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_MALLOC_FAILED ：申请内存失败
  OPRT_OS_ADAPTER_QUEUE_CREAT_FAILED ：队列创建失败
  ```

## 2-tuya_os_adapt_queue_free

```c
void tuya_os_adapt_queue_free(QUEUE_HANDLE queue)
```

- **函数描述**

  释放队列

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | queue | 待释放队列 |

- **返回值**

  ```
  无
  ```

## 3-tuya_os_adapt_queue_post

```c
int tuya_os_adapt_queue_post(QUEUE_HANDLE queue, void *msg, unsigned int timeout)
```

- **函数描述**

  向队列中投送消息
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | queue | 接收消息的队列句柄 |
  | [in] | msg | 指向待投送消息的指针 |
  | [in] | timeout | 等待超时时间 |


- **返回值**

  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_QUEUE_SEND_FAIL：消息投送失败
  ```


## 4-tuya_os_adapt_queue_fetch

```c
int tuya_os_adapt_queue_fetch(QUEUE_HANDLE queue, void *msg, unsigned int timeout)
```

- **函数描述**

  从队列中接收一条消息
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | queue | 接收消息的队列句柄 |
  | [in] | msg | 指向待投送消息的指针 |
  | [in] | timeout | 等待超时时间 |

- **返回值**
  ```
  OPRT_OS_ADAPTER_OK : 成功
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  OPRT_OS_ADAPTER_QUEUE_RECV_FAIL：消息接收失败
  ```
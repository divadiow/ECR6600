# 索引

  * [1 tuya_os_adapt_net_get_errno](#1-tuya_os_adapt_net_get_errno)
  * [2 tuya_os_adapt_net_fd_set](#2-tuya_os_adapt_net_fd_set)
  * [3 tuya_os_adapt_net_fd_clear](#3-tuya_os_adapt_net_fd_clear)
  * [4 tuya_os_adapt_net_fd_isset](#4-tuya_os_adapt_net_fd_isset)
  * [5 tuya_os_adapt_net_fd_zero](#5-tuya_os_adapt_net_fd_zero)
  * [6 tuya_os_adapt_net_select](#6-tuya_os_adapt_net_select)
  * [7 tuya_os_adapt_net_get_nonblock](#7-tuya_os_adapt_net_get_nonblock)
  * [8 tuya_os_adapt_net_set_block](#8-tuya_os_adapt_net_set_block)
  * [9 tuya_os_adapt_net_close](#9-tuya_os_adapt_net_close)
  * [10 tuya_os_adapt_net_shutdown](#10-tuya_os_adapt_net_shutdown)
  * [11 tuya_os_adapt_net_socket_create](#11-tuya_os_adapt_net_socket_create)
  * [12 tuya_os_adapt_net_connect](#12-tuya_os_adapt_net_connect)
  * [13 tuya_os_adapt_net_connect_raw](#13-tuya_os_adapt_net_connect_raw)
  * [14 tuya_os_adapt_net_bind](#14-tuya_os_adapt_net_bind)
  * [15 tuya_os_adapt_net_socket_bind](#15-tuya_os_adapt_net_socket_bind)
  * [16 tuya_os_adapt_net_listen](#16-tuya_os_adapt_net_listen)
  * [17 tuya_os_adapt_net_accept](#17-tuya_os_adapt_net_accept)
  * [18 tuya_os_adapt_net_send](#18-tuya_os_adapt_net_send)
  * [19 tuya_os_adapt_net_send_to](#19-tuya_os_adapt_net_send_to)
  * [20 tuya_os_adapt_net_recv](#20-tuya_os_adapt_net_recv)
  * [21 tuya_os_adapt_net_recvfrom](#21-tuya_os_adapt_net_recvfrom)
  * [22 tuya_os_adapt_net_set_bufsize](#22-tuya_os_adapt_net_set_bufsize)
  * [23 tuya_os_adapt_net_set_timeout](#23-tuya_os_adapt_net_set_timeout)
  * [24 tuya_os_adapt_net_set_reuse](#24-tuya_os_adapt_net_set_reuse)
  * [25 tuya_os_adapt_net_disable_nagle](#25-tuya_os_adapt_net_disable_nagle)
  * [26 tuya_os_adapt_net_set_boardcast](#26-tuya_os_adapt_net_set_boardcast)
  * [27 tuya_os_adapt_net_set_keepalive](#27-tuya_os_adapt_net_set_keepalive)
  * [28 tuya_os_adapt_net_gethostbyname](#28-tuya_os_adapt_net_gethostbyname)
  * [29 tuya_os_adapt_net_str2addr](#29-tuya_os_adapt_net_str2addr)
  * [30 tuya_os_adapt_net_addr](#30-tuya_os_adapt_net_addr)
------


## 1-tuya_os_adapt_net_get_errno

```c
int tuya_hal_bt_port_init(ty_bt_param_t *p)
```

- **函数描述**

  获取错误序号
 

- **参数**

  无

- **返回值**

  ```
  错误序列号
  ```


## 2-tuya_os_adapt_net_fd_set

```c
int tuya_os_adapt_net_fd_set(const int fd, UNW_FD_SET_T* fds)
```

- **函数描述**

  向文件描述符集合fds中添加文件描述符fd

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | fd | 文件描述符 |
  | [inout] | fds | 指向文件描述符集合的指针 |

- **返回值**

  ```
	UNW_SUCCESS : 成功
  其他 ：失败
  ```

## 3-tuya_os_adapt_net_fd_clear

```c
int tuya_os_adapt_net_fd_clear(const int fd, UNW_FD_SET_T* fds)
```

- **函数描述**

  清除文件描述符集合中fds所有文件描述符fd
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | fd | 文件描述符 |
  | [inout] | fds | 指向文件描述符集合的指针 |

- **返回值**

  ```
	UNW_SUCCESS : 成功
  其他 ：失败
  ```


## 4-tuya_os_adapt_net_fd_isset

```c
int tuya_os_adapt_net_fd_isset(const int fd, UNW_FD_SET_T* fds)
```

- **函数描述**

  判断fds是否被置位
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | fd | 文件描述符 |
  | [inout] | fds | 指向文件描述符集合的指针 |

- **返回值**
  ```
	0 ：没有可读，可写或执行错误的fd
  其他 ：有可读，可写或执行错误的fd
  ```

## 5-tuya_os_adapt_net_fd_zero

```c
int tuya_os_adapt_net_fd_zero(UNW_FD_SET_T* fds)
```

- **函数描述**

  将套接字描述符集合清零
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [inout] | fds | 指向文件描述符集合的指针 |

- **返回值**

  ```
	UNW_SUCCESS : 成功
  其他 ：失败
  ```

## 6-tuya_os_adapt_net_select

```c
tuya_os_adapt_net_select(const int maxfd, UNW_FD_SET_T *readfds, UNW_FD_SET_T *writefds,\
UNW_FD_SET_T *errorfds, const unsigned int ms_timeout)
```

- **函数描述**

  检测所关注的套接字状态
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | maxfd | 监视对象文件描述符数量 |
  | [inout] | readfds | 指向select监视的可读文件描述符集合指针 |
  | [inout] | writefds | 指向select监视的可写文件描述符集合指针 |
  | [inout] | errorfds | 指向select监视的错误文件描述符集合指针 |
  | [out] | ms_timeout | select的等待超时时间 |

- **返回值**

  ```
  等于 0 : 等待超时,没有可读写或错误的文件
  大于 0 ：纯在文件可读写或出错
  小于 0 ：select 出错
  ```


## 7-tuya_os_adapt_net_get_nonblock

```c
int tuya_os_adapt_net_get_nonblock(const int fd)
```

- **函数描述**

  获取文件描述符阻塞信息
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | fd | 文件描述符 |

- **返回值**

  ```
  等于 0 : 阻塞
  大于 0 : 非阻塞
  小于 0 : 失败
  ```

## 8-tuya_os_adapt_net_set_block

```c
int tuya_os_adapt_net_set_block(const int fd, const bool block)
```

- **函数描述**

  设置文件描述符为阻塞/非阻塞
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | fd | 文件描述符 |
  | [in] | block | 0 : 设置为阻塞 1 ：设置为非阻塞 |

- **返回值**

  ```
  UNW_SUCCESS : 成功
  UNW_FAIL : 失败
  ```


## 9-tuya_os_adapt_net_close

```c
int tuya_os_adapt_net_close(const int fd)
```

- **函数描述**

  关闭所打开的文件描述符，并释放相关资源
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | fd | 文件描述符 |
 
- **返回值**

  ```
  0 : 成功
  其他 : 失败
  ```
## 10-tuya_os_adapt_net_shutdown

```c
int tuya_os_adapt_net_shutdown(const int fd, const int how)
```

- **函数描述**

  关闭文件描述符关闭TCP连接
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | fd | 文件描述符 |
  | [in] | how | SD_RECEIVE: 关闭接收通道; SD_SEND: 关闭发送通道; SD_BOTH: 同时关闭接收通道和发送通道 |
 
- **返回值**

  ```
  0 : 成功
  其他 : 失败
  ```
- **备注**

  ```
  调用shutdown()只是进行了TCP断开, 并没有释放文件描述符
  ```
## 11-tuya_os_adapt_net_socket_create

```c
int tuya_os_adapt_net_socket_create(const UNW_PROTOCOL_TYPE type)
```

- **函数描述**

  创建一个套接字
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | type | 所创建套接字的协议类型 |
 
- **返回值**

  ```
  所创建套接字
  ```
- **备注**

  ```
  如果使用一个无效的 domain 或 type，将使用 AF_INET 和 SOCK_STREAM 替代无效参数
  ```
## 12-tuya_os_adapt_net_connect

```c
int tuya_os_adapt_net_connect(const int fd, const UNW_IP_ADDR_T addr, const unsigned short port)
```

- **函数描述**

  建立套接字初始化连接
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | fd | 需要建立tcp连接的套接字 |
  | [in] | addr | 服务端套接字IP | 
  | [in] | port | 服务端套接字端口号 |

- **返回值**

  ```
  0 : 连接成功
  其他 ： 连接失败
  ```
## 13-tuya_os_adapt_net_connect_raw

```c
int tuya_os_adapt_net_connect_raw(const int fd, void *p_socket_addr, const int len)
```

- **函数描述**

  建立套接字初始化连接
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | fd | 套接字 |
  | [in] | p_socket_addr | 指向套接字想要连接的主机地址和端口号的指针 | 
  | [in] | len | 储存地址结构体长度 |

- **返回值**

  ```
  0 : 连接成功
  其他 ： 连接失败
  ```
  ## 14-tuya_os_adapt_net_bind

```c
int tuya_os_adapt_net_bind(const int fd, const UNW_IP_ADDR_T addr, const unsigned short port)
```

- **函数描述**

  为套接字关联一个相应地址
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | fd | 待关联套接字 |
  | [in] | addr | 待关联IP地址 | 
  | [in] | port | 关联端口号 |

- **返回值**

  ```
  0 : 关联接成功
  其他 ： 关接失败
  ```
  ## 15-tuya_os_adapt_net_socket_bind

```c
int tuya_os_adapt_net_socket_bind(const int fd, const char *ip)
```

- **函数描述**

  为套接字关联一个相应地址
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | fd | 待关联套接字 |
  | [in] | ip | 字符串IP的指针 | 

- **返回值**

  ```
  UNW_SUCCESS : 关联接成功
  UNW_FAIL  ： 关接失败
  ```
- **备注**

  ```
  端口号由系统自动分配
  ```  
  ## 16-tuya_os_adapt_net_listen

```c
int tuya_os_adapt_net_listen(const int fd, const int backlog)
```

- **函数描述**

  监听连接请求
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | fd | 待关联套接字 |
  | [in] | backlog | 该套接字允许的最大连接数 | 

- **返回值**

  ```
  0 : 成功
  其他  ： 关接失败
  ```
- **备注**

  ```
  调用listen导致套接字从CLOSED状态转换为LISTEN状态
  ```
  ## 17-tuya_os_adapt_net_accept

```c
int tuya_os_adapt_net_accept(const int fd, *addr, unsigned short *port)
```

- **函数描述**

  接收连接请求
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | fd | 待关联套接字 |
  | [inout] | addr | 请求连接客服端套接字IP地址 | 
  | [inout] | port | 请求连接客服端套接字端口 | 

- **返回值**

  ```
  客服端套接字标识
  UNW_FAIL ：失败
  ```
  ## 18-tuya_os_adapt_net_send

```c
int tuya_os_adapt_net_send(const int fd, const void *buf, const unsigned int nbytes)
```

- **函数描述**

  向连接的另一端发送数据
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | fd | 接收消息的套接字的文件描述符 |
  | [in] | buf | 待发送数据buf指针 | 
  | [in] | nbytes | 待发送的字节数 | 

- **返回值**

  ```
  实际发送字节数
  其他 ：失败
  ```
- **备注**

  ```
  套接字处于连接状态的时候才能使用
  ```
  ## 19-tuya_os_adapt_net_send_to

```c
int tuya_os_adapt_net_send_to(const int fd, const void *buf, const unsigned int nbytes,\
const UNW_IP_ADDR_T addr,const unsigned short port)
```

- **函数描述**

  将数据以报文形式发给指定主机
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | fd | 接收消息的套接字的文件描述符 |
  | [in] | buf | 待发送数据buf指针 | 
  | [in] | nbytes | 待发送的字节数 | 
  | [in] | addr | 目的主机套接字IP地址 | 
  | [in] | port | 目的主机套接字端口 | 
- **返回值**

  ```
  实际发送字节数
  其他 ：失败
  ```
  ## 20-tuya_os_adapt_net_recv

```c
int tuya_os_adapt_net_recv(const int fd, void *buf, const unsigned int nbytes)
```

- **函数描述**

  接收连接另一端的数据
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | fd | 套接字描述符 |
  | [inout] | buf | 包含待接收数据的缓冲区 | 
  | [in] | nbytes | 接收数据的字节数 | 

- **返回值**

  ```
  实际接收到数据字节数
  其他 ：失败
  ```
  ## 21tuya_os_adapt_net_recvfrom

```c
int tuya_os_adapt_net_recvfrom(const int fd, void *buf, const unsigned int nbytes,\
UNW_IP_ADDR_T *addr, unsigned short *port)
```

- **函数描述**

  接收来自源主机传来的数据报文
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | fd | 套接字描述符 |
  | [inout] | buf | 包含待接收数据的缓冲区 | 
  | [in] | nbytes | 接收数据的字节数 | 
  | [in] | addr | 源主机IP地址 | 
  | [in] | port | 源主机端口号 | 
- **返回值**

  ```
  实际接收到数据字节数
  其他 ：失败
  ```
  ## 22-tuya_os_adapt_net_set_bufsize

```c
int tuya_os_adapt_net_set_bufsize(const int fd, const int buf_size,const UNW_TRANS_TYPE_E type)
```

- **函数描述**

  设置缓存区大小
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | fd | 套接字描述符 |
  | [inout] | buf | 数据的缓冲区 | 
  | [in] | type | 数据传输类型（0 : 接收数据 1 : 发送数据） | 

- **返回值**

  ```
  0: 成功
  其他 ：失败
  ```
  ## 23-tuya_os_adapt_net_set_timeout

```c
int tuya_os_adapt_net_set_timeout(const int fd, const int ms_timeout,const UNW_TRANS_TYPE_E type)
```

- **函数描述**

  设置等待超时时间
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | fd | 套接字描述符 |
  | [in] | ms_timeout | 超时时间 | 
  | [in] | type | 数据传输类型（0 : 接收数据 1 : 发送数据） | 

- **返回值**

  ```
  0: 成功
  其他 ：失败
  ```
  ## 24-tuya_os_adapt_net_set_reuse

```c
int tuya_os_adapt_net_set_reuse(const int fd)
```

- **函数描述**

  允许套接字与已在使用的地址绑定
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | fd | 套接字描述符 |

- **返回值**

  ```
  0: 成功
  其他 ：失败
  ```
  ## 25-tuya_os_adapt_net_disable_nagle

```c
int tuya_os_adapt_net_disable_nagle(const int fd)
```

- **函数描述**

  禁止发送合并
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | fd | 套接字描述符 |

- **返回值**

  ```
  0: 成功
  其他 ：失败
  ```
  ## 26-tuya_os_adapt_net_set_boardcast

```c
int tuya_os_adapt_net_set_boardcast(const int fd)
```

- **函数描述**

  允许发送和接收广播消息
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | fd | 套接字描述符 |

- **返回值**

  ```
  0: 成功
  其他 ：失败
  ```
  ## 27-tuya_os_adapt_net_set_keepalive

```c
tuya_os_adapt_net_set_keepalive(const int fd, const bool alive,const unsigned int idle, \
const unsigned int intr,const unsigned int cnt)
```

- **函数描述**

  TCP连接保活设置
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | fd | 套接字描述符 |
  | [in] | alive | 0 : 保活使能；1 : 保活关闭 |
  | [in] | idle | 保活时间 |
  | [in] | intr | 保活时间间隔 |
  | [in] | cnt | 保活次数 |

- **返回值**

  ```
  0: 成功
  其他 ：失败
  ```
  ## 28-tuya_os_adapt_net_gethostbyname

```c
int tuya_os_adapt_net_gethostbyname(const char *domain, UNW_IP_ADDR_T *addr)
```

- **函数描述**

  TCP连接保活设置
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | domain | 给定主机名 |
  | [inout] | addr | 指向主机地址指针 |

- **返回值**

  ```
  OPRT_OS_ADAPTER_INVALID_PARM ：传参错误
  UNW_SUCCESS : 成功
  UNW_FAIL ：失败
  ```
  ## 29-tuya_os_adapt_net_str2addr

```c
UNW_IP_ADDR_T tuya_os_adapt_net_str2addr(signed char *ip)
```

- **函数描述**

  Ascii网络字符串地址转换为主机序地址
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | ip| 指向Ascii主机字符串地址指针 |

- **返回值**

  ```
  主机序ip地址
  ```
  ## 30-tuya_os_adapt_net_addr

```c
UNW_IP_ADDR_T tuya_os_adapt_net_addr(const signed char *ip)
```

- **函数描述**

  Ascii网络字符串地址转换为网络序地址
 

- **参数**

  | 输入/输出 |  参数名  |  描述  |
  |--------|--------|--------|
  | [in] | ip| 指向Ascii网络字符串地址指针 |

- **返回值**

  ```
  网络序ip地址
  ```
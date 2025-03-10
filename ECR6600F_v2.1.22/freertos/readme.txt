.
├── apps			//通用applications，可用于多平台			
│   ├── iperf
│   └── sntp
├── arch			//CPU架构相关操作，例如开关中断
│   └── andes
├── Boards			//具体参考设计，其中包含boot启动代码、编译选项配置.config、配置头文件autoconfig.h、链接脚本，以及参考设计专用的驱动和应用代码，减少外部drivers和apps代码的冗余度
│   ├── tr6260
│   │   ├── boot
│   │   ├── standalone_at
│   │   │   ├── applications
│   │   │   ├── .config
│   │   │   ├── autoconfig.h
│   │   │   ├── drivers
│   │   │   └── standalone_at.lds
│   │   └── standalone_at_diff
│   └── tr6600
├── components		//功能组件，对上提供API接口，完成相关功能，例如网络通信功能LWIP、mqtt
│   ├── json
│   ├── LWIP
│   ├── mbedtls
│   ├── mqtt
│   └── smartconfig
├── drivers			//具体外设驱动
├── include			//各模块对外头文件
├── libs			//客户提供的lib库和.o文件
├── os				//操作系统和适配层，对上统一使用os_hal文件夹中提供的API接口，对下根据Kconfig配置，选用nuttx或者freertos
│   ├── freertos
│   ├── Kconfig
│   ├── nuttx
│   └── os_hal
├── PS				//协议栈源码，wifi和ble
│   ├── ble
│   └── wifi
├── readme.txt
└── tools		//编译开发工具，Makefile或者scons
    └── scons

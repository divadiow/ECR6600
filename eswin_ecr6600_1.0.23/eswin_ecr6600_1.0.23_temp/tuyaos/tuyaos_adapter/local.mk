# 当前文件所在目录
LOCAL_PATH := $(call my-dir)

#---------------------------------------

# 清除 LOCAL_xxx 变量
include $(CLEAR_VARS)

# 当前模块名
LOCAL_MODULE := $(notdir $(LOCAL_PATH))

# 模块对外头文件（只能是目录）
# 加载至CFLAGS中提供给其他组件使用；打包进SDK产物中；
#LOCAL_TUYA_SDK_INC := $(LOCAL_PATH)/include

# 模块对外CFLAGS：其他组件编译时可感知到
LOCAL_TUYA_SDK_CFLAGS :=

# 模块源代码
LOCAL_SRC_FILES := $(shell find $(LOCAL_PATH)/src -name "*.c" -o -name "*.cpp" -o -name "*.cc")
LOCAL_SRC_FILES += $(shell find $(LOCAL_PATH)/include/init/src -name "*.c" -o -name "*.cpp" -o -name "*.cc")
LOCAL_SRC_FILES += $(shell find $(LOCAL_PATH)/include/utilities/src -name "*.c" -o -name "*.cpp" -o -name "*.cc")

# 模块内部CFLAGS：仅供本组件使用
## 路径定义
-include $(LOCAL_PATH)/platform.mk
TUYAOS_DIR:=$(LOCAL_PATH)/../../../../
PLATFROM_DIR:=$(LOCAL_PATH)/../../
TOOLCHAIN_PATH:=$(PLATFROM_DIR)/toolchain
ESWINOS_DIR:= $(PLATFROM_DIR)/eswin_ecr6600_os

## 平台编译选项
LOCAL_CFLAGS += $(TUYA_PLATFORM_CFLAGS) 

## tuya头文件依赖
### tuyaos include公共头文件
#LOCAL_CFLAGS += $(foreach incpath,$(shell find $(TUYAOS_DIR)/include -type d),-I "$(incpath)")

### tuyaos adapter标准头文件
#LOCAL_CFLAGS += $(foreach incpath,$(shell find $(TUYAOS_DIR)/adapter -type d),-I "$(incpath)")

### tuyaos adapter本地实现头文件
LOCAL_CFLAGS += -I include/
LOCAL_CFLAGS += $(foreach incpath,$(shell find include -type d),-I "$(incpath)")


## eswin头文件依赖
LOCAL_CFLAGS += -I $(ESWINOS_DIR)/Boards/ecr6600/common/include 
LOCAL_CFLAGS += -include $(ESWINOS_DIR)/Boards/ecr6600/tuya/generated/config.h
LOCAL_CFLAGS += $(foreach incpath,$(shell find $(ESWINOS_DIR)/include -type d),-I "$(incpath)")
LOCAL_CFLAGS += -I $(ESWINOS_DIR)/Boards/ecr6600/common/include

# 全局变量赋值
TUYA_SDK_INC += $(LOCAL_TUYA_SDK_INC)  # 此行勿修改
TUYA_SDK_CFLAGS += $(LOCAL_TUYA_SDK_CFLAGS)  # 此行勿修改

# 生成静态库
include $(BUILD_STATIC_LIBRARY)

# 生成动态库
include $(BUILD_SHARED_LIBRARY)

# 导出编译详情
include $(OUT_COMPILE_INFO)

#---------------------------------------


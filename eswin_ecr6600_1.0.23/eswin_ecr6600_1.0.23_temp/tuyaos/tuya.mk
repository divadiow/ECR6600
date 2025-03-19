############################################################
# 目录路径定义
############################################################

TOP_DIR:=../..

# SDK_DIR:=$(TOP_DIR)/sdk/
PLATFROM_DIR:=$(TOP_DIR)/vendor/eswin_ecr6600/

TOOLCHAIN_PATH:=$(PLATFROM_DIR)/toolchain

ESWINOS_DIR:= $(PLATFROM_DIR)/eswin_ecr6600_os
ESWINOSADPT_INC_DIR:= $(PLATFROM_DIR)/tuyaos/tuyaos_adapter/include
ESWINOSADPT_SRC_DIR:= $(PLATFROM_DIR)/tuyaos/tuyaos_adapter/src
# ESWINOS_TUYA_COMMON_DIR:= $(PLATFROM_DIR)/tuya_common

OUTPUTDIR:=output
OUTPUTDIR_APP_OBJS:=$(OUTPUTDIR)/objs/$(APP_PATH)
OUTPUTDIR_LIB:=$(OUTPUTDIR)/libs


############################################################
# 支持linux独立编译，通过build_station.sh调用
# 包含编译参数配置定义文件
############################################################
-include $(TOP_DIR)/build/build_param
-include $(TOP_DIR)/build/tuya_iot.config

############################################################
# GCC工具配置
############################################################

ifneq ($(GCC_PATH),)

CROSS_COMPILE:=nds32le-elf-
CROSSDEV = $(GCC_PATH)$(CROSS_COMPILE)
# Default toolchain
CC := $(CROSSDEV)gcc
AR := $(CROSSDEV)ar

endif
############################################################
# 包含开发环境定义
############################################################
-include $(PLATFROM_DIR)/tuyaos/platform.mk

ifeq ($(TUYA_APP_NAME),eswin_ecr6600_one_plug_demo)

else ifeq ($(TUYA_APP_NAME),eswin_ecr6600_simple_light_demo)

else ifeq ($(TUYA_APP_NAME), cli_station_sample)
CFLAGS+=-DTUYA_SDK_CLI_ADAPTER -DTUYA_HOSTAPD_SUPPORT=1 -DTLS_MODE=3 -DTLS_SESSION=1
endif

#TUYA_PLATFORM_CFLAGS := -include nds32_intrinsic.h -fmessage-length=0 -mcmodel=large -std=gnu99 -fno-omit-frame-pointer -ffunction-sections -fdata-sections  -g
CFLAGS += $(TUYA_PLATFORM_CFLAGS) 
CFLAGS += -DAPP_BIN_NAME='"$(APP_BIN_NAME)"' -DUSER_SW_VER='"$(APP_VERSION)"' -DAPP_VERSION='"$(APP_VERSION)"' 

CFLAGS +=-I include/
CFLAGS += $(foreach incpath,$(shell find include -type d),-I "$(incpath)")

CFLAGS += $(foreach incpath,$(shell find $(ESWINOSADPT_INC_DIR) -type d),-I "$(incpath)")
CFLAGS += $(foreach incpath,$(shell find $(TOP_DIR)/include -type d),-I "$(incpath)")

ifeq ($(TUYA_APP_NAME),cli_station_sample)
CFLAGS += $(foreach incpath,$(shell find $(TOP_DIR)/samples/cli_station_sample -type d -name include),-I "$(incpath)") 
CFLAGS += -I $(TOP_DIR)/samples/cli_station_sample/components/lwip/include/lwip
endif

CFLAGS += -I $(ESWINOS_DIR)/Boards/ecr6600/common/include 
CFLAGS += -include $(ESWINOS_DIR)/Boards/ecr6600/tuya/generated/config.h
CFLAGS += $(foreach incpath,$(shell find $(ESWINOS_DIR)/include -type d),-I "$(incpath)")
CFLAGS += -I $(ESWINOS_DIR)/Boards/ecr6600/common/include

CFLAGS += $(foreach incpath,$(shell find $(TOP_DIR)/adapter -type d),-I "$(incpath)") 



############################################################
# 默认编译目标
############################################################

LIBTARGET := libtuya_app.a
all: $(OUTPUTDIR_LIB)/$(LIBTARGET)

#CSRCS:=  $(wildcard src/*.c)
CSRCS:=

CLI_STATION_SRC_DIRS := $(shell find ./src -type d)
CSRCS+= $(foreach dir, $(CLI_STATION_SRC_DIRS), $(wildcard $(dir)/*.c))

CSRCS+=  $(wildcard $(ESWINOSADPT_SRC_DIR)/*.c $(ESWINOSADPT_SRC_DIR)/system/*.c $(ESWINOSADPT_SRC_DIR)/driver/*.c)

# add tuya adapter c files
CSRCS+=  $(wildcard $(ESWINOSADPT_INC_DIR)/init/src/*.c)
CSRCS+=  $(wildcard $(ESWINOSADPT_INC_DIR)/utilities/src/*.c)

############################################################
# add tuya application components.mk
############################################################
TUYA_COMPONENTS := $(TOP_DIR)/$(APP_PATH)/components.mk
ifeq ($(TUYA_COMPONENTS), $(wildcard $(TUYA_COMPONENTS)))
sinclude $(TUYA_COMPONENTS)
TY_INC_DIRS += $(foreach n, $(COMPONENTS), $(shell find $(TOP_DIR)/components/$(n)/include -type d))
CFLAGS+= $(foreach base_dir, $(TY_INC_DIRS), $(addprefix -I , $(base_dir)))
TY_SRC_DIRS += $(foreach n, $(COMPONENTS), $(shell find $(TOP_DIR)/components/$(n)/src -type d))
CSRCS+= $(foreach dir, $(TY_SRC_DIRS), $(wildcard $(dir)/*.c))
endif

COBJS = $(addprefix $(OUTPUTDIR_APP_OBJS)/,$(CSRCS:.c=.o))
OBJS =  $(COBJS)
HDEPS:=$(OBJS:.o=.d)
-include $(HDEPS)
$(COBJS): $(OUTPUTDIR_APP_OBJS)/%.o: %.c
	@ -mkdir -p $(dir $@)
	@echo "CC:$<"
	@$(CC) -MMD $(CFLAGS) -c $< -o $@

$(OUTPUTDIR_LIB)/$(LIBTARGET): $(OBJS)
	@ -mkdir -p $(dir $@)
	$(AR) -rcs  $@ $(OBJS)

	
clean:
	rm -rf $(OUTPUTDIR)
	rm -rf $(ESWINOS_DIR)/libs/libtuya_app.a

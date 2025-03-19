
TUYA_PLATFORM_DIR := $(dir $(lastword $(MAKEFILE_LIST)))/../


# tuya os adapter includes
TUYA_PLATFORM_CFLAGS := -I$(TUYA_PLATFORM_DIR)/tuya_os_adapter/include
TUYA_PLATFORM_CFLAGS += -I$(TUYA_PLATFORM_DIR)/tuya_os_adapter/include/driver
TUYA_PLATFORM_CFLAGS += -I$(TUYA_PLATFORM_DIR)/tuya_os_adapter/include/system
TUYA_PLATFORM_CFLAGS += -I$(TUYA_PLATFORM_DIR)/tuya_os_adapter/include/lwip


# gaotuoyun  CFLAGS
TUYA_PLATFORM_CFLAGS += -include nds32_intrinsic.h -fmessage-length=0 -mcmodel=large -std=gnu99 -Os -mno-ifc -fno-omit-frame-pointer -ffunction-sections -fdata-sections  -g
#TUYA_PLATFORM_CFLAGS += -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -g2 -w -Os -Wno-pointer-sign -fno-common -fmessage-length=0  -ffunction-sections -fdata-sections -fomit-frame-pointer -fno-short-enums -DF_CPU=166000000L -std=gnu99 -fsigned-char



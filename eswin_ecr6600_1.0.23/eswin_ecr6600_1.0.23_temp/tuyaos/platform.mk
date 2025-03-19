
TUYA_PLATFORM_DIR := $(dir $(lastword $(MAKEFILE_LIST)))/../

# tuya os adapter includes
TUYA_PLATFORM_CFLAGS := -I$(TUYA_PLATFORM_DIR)/tuyaos/tuyaos_adapter/include
TUYA_PLATFORM_CFLAGS += -I$(TUYA_PLATFORM_DIR)/tuyaos/tuyaos_adapter/include/hostapd
TUYA_PLATFORM_CFLAGS += -I$(TUYA_PLATFORM_DIR)/tuyaos/tuyaos_adapter/include/utilities/include
TUYA_PLATFORM_CFLAGS += -I$(TUYA_PLATFORM_DIR)/tuyaos/tuyaos_adapter/include/init/include
TUYA_PLATFORM_CFLAGS += -I$(TUYA_PLATFORM_DIR)/tuyaos/tuyaos_adapter/include/adc
TUYA_PLATFORM_CFLAGS += -I$(TUYA_PLATFORM_DIR)/tuyaos/tuyaos_adapter/include/gpio
TUYA_PLATFORM_CFLAGS += -I$(TUYA_PLATFORM_DIR)/tuyaos/tuyaos_adapter/include/hci
TUYA_PLATFORM_CFLAGS += -I$(TUYA_PLATFORM_DIR)/tuyaos/tuyaos_adapter/include/i2c
TUYA_PLATFORM_CFLAGS += -I$(TUYA_PLATFORM_DIR)/tuyaos/tuyaos_adapter/include/pinmux
TUYA_PLATFORM_CFLAGS += -I$(TUYA_PLATFORM_DIR)/tuyaos/tuyaos_adapter/include/pwm
TUYA_PLATFORM_CFLAGS += -I$(TUYA_PLATFORM_DIR)/tuyaos/tuyaos_adapter/include/rtc
TUYA_PLATFORM_CFLAGS += -I$(TUYA_PLATFORM_DIR)/tuyaos/tuyaos_adapter/include/wifi
TUYA_PLATFORM_CFLAGS += -I$(TUYA_PLATFORM_DIR)/tuyaos/tuyaos_adapter/include/security
TUYA_PLATFORM_CFLAGS += -I$(TUYA_PLATFORM_DIR)/tuyaos/tuyaos_adapter/include/spi
TUYA_PLATFORM_CFLAGS += -I$(TUYA_PLATFORM_DIR)/tuyaos/tuyaos_adapter/include/flash
TUYA_PLATFORM_CFLAGS += -I$(TUYA_PLATFORM_DIR)/tuyaos/tuyaos_adapter/include/system
TUYA_PLATFORM_CFLAGS += -I$(TUYA_PLATFORM_DIR)/tuyaos/tuyaos_adapter/include/timer
TUYA_PLATFORM_CFLAGS += -I$(TUYA_PLATFORM_DIR)/tuyaos/tuyaos_adapter/include/uart
TUYA_PLATFORM_CFLAGS += -I$(TUYA_PLATFORM_DIR)/tuyaos/tuyaos_adapter/include/watchdog
TUYA_PLATFORM_CFLAGS += -I$(TUYA_PLATFORM_DIR)/tuyaos/tuyaos_adapter/include/network
TUYA_PLATFORM_CFLAGS += -I$(TUYA_PLATFORM_DIR)/tuyaos/tuyaos_adapter/include/bluetooth
TUYA_PLATFORM_CFLAGS += -I$(TUYA_PLATFORM_DIR)/tuyaos/tuyaos_adapter/include/pinmux/include
TUYA_PLATFORM_CFLAGS += -I$(TUYA_PLATFORM_DIR)/tuyaos/tuyaos_adapter/include/wakeup
TUYA_PLATFORM_CFLAGS += -I$(TUYA_PLATFORM_DIR)/tuyaos/tuyaos_adapter/include/ethernet
TUYA_PLATFORM_CFLAGS += -I$(TUYA_PLATFORM_DIR)/tuyaos/tuyaos_adapter/include/wired

# gaotuoyun  CFLAGS
TUYA_PLATFORM_CFLAGS += -include nds32_intrinsic.h -fmessage-length=0 -mcmodel=large -std=gnu99 -Os -mno-ifc -fno-omit-frame-pointer -ffunction-sections -fdata-sections  -g
# TUYA_PLATFORM_CFLAGS += -include nds32_intrinsic.h -fmessage-length=0 -mcmodel=large -std=gnu99 -Os -mno-ifc -ffunction-sections -fdata-sections  -g


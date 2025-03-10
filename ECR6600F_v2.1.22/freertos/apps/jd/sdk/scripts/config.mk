# nds32le-elf-gcc (2020-06-11_nds32le-elf-mculib-v3s-72e479136d7) 8.2.0
CROSS_COMPILE := ../../../../../../../build/toolchains/nds32le-elf-mculib-v3s/bin/nds32le-elf-

# x86 arch 
CFLAGS := -include nds32_intrinsic.h -fmessage-length=0 -mcmodel=large -std=gnu99 -fno-omit-frame-pointer -ffunction-sections -fdata-sections  -g
# other arch 
# CFLAGS := -DuECC_PLATFORM=uECC_arch_other -D__LINUX_PAL__ -DJOYLINK_SDK_EXAMPLE_TEST -D_SAVE_FILE_

#LDFLAGS = -lpthread -lm
LINKFLAGS = -lpthread -lm
USE_JOYLINK_JSON=yes

#----------------------------------------------以下固定参数
CFLAGS += -D_IS_DEV_REQUEST_ACTIVE_SUPPORTED_ -D_GET_HOST_BY_NAME_ -DuECC_PLATFORM=uECC_arch_other

TARGET_DIR = ${TOP_DIR}/target
TARGET_LIB = ${TARGET_DIR}/lib
TARGET_BIN = ${TARGET_DIR}/bin

CC=$(CROSS_COMPILE)gcc
AR=$(CROSS_COMPILE)ar
RANLIB=$(CROSS_COMPILE)ranlib
STRIP=$(CROSS_COMPILE)strip

RM = rm -rf
CP = cp -rf
MV = mv -f



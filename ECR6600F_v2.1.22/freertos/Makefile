.PHONY:all clean distclean help FORCE
.PHONY:menuconfig defconfig savedefconfig
.PHONY: pass1 pass1dep
.PHONY: pass2 pass2dep
.PHONY: depend

###########################################
#
#		top	directory path
#
###########################################
TOPDIR				:= $(shell echo $(CURDIR) | sed -e 's/ /\\ /g')
export BOARD_DIR	:= $(BOARD_DIR)
export APP_NAME		:= $(APP_NAME)


###########################################
#
#			Compiler output directory path
#
###########################################
OUTPUTDIR			:= $(TOPDIR)/build/$(notdir $(APP_NAME))
GENERATED_DIR		:= $(BOARD_DIR)/generated

$(shell if [ ! -d $(OUTPUTDIR) ];then mkdir -p $(OUTPUTDIR);fi)
$(shell if [ ! -d $(GENERATED_DIR) ];then mkdir -p $(GENERATED_DIR);fi)


###########################################
#
#prepare environments variables :.config and config.h 
#
###########################################
export KCONFIG_CONFIG = $(GENERATED_DIR)/.config
export KCONFIG_AUTOHEADER = $(GENERATED_DIR)/config.h

$(shell if [ ! -e $(KCONFIG_CONFIG) ];then cp $(BOARD_DIR)/defconfig $(KCONFIG_CONFIG);fi)


###########################################
#
#call function condition : abs directory path and Make.defs
#
###########################################
TOPDIR_ABSPATH			:= $(TOPDIR)
BOARD_DIR_ABSPATH		:= $(BOARD_DIR)
OUTPUTDIR_ABSPATH		:= $(OUTPUTDIR)
GENERATED_DIR_ABSPATH	:= $(GENERATED_DIR)
include $(TOPDIR)/Make.defs


###########################################
#
#			relative directory path
#
###########################################
SCRIPTSDIR	:= $(TOPDIR)/scripts
ARCHDIR		:= $(TOPDIR)/arch/$(CONFIG_CPU_ARCH)

SUBDIRS 	:= $(shell find $(TOPDIR) -maxdepth 1 -type d)
SUBDIRS		:= $(filter-out $(TOPDIR),$(SUBDIRS))


###########################################
#
#			Kconfig
#
###########################################
if_ubuntu20_env="$(shell cat /etc/issue | grep "Ubuntu 2")"
menuconfig: clean
ifneq ($(HOSTOS), Cygwin)  # Ubuntu environment 
ifneq ($(if_ubuntu20_env), "")  # for Ubuntu20 and Ubuntu22
	$(Q) $(TOPDIR)/scripts/bin/kconfig_tool/menuconfig_ubuntu20_22 $(TOPDIR)/Kconfig
else  # for Ubuntu16 and Ubuntu18
	$(Q) $(TOPDIR)/scripts/bin/kconfig_tool/menuconfig_ubuntu16_18 $(TOPDIR)/Kconfig
endif
else
	$(Q) $(TOPDIR)/scripts/bin/kconfig_tool/menuconfig$(HOSTEXEEXT) $(TOPDIR)/Kconfig
endif

#defconfig:
#	$(Q) $(TOPDIR)/scripts/bin/kconfig_tool/defconfig$(HOSTEXEEXT) --kconfig $(TOPDIR)/Kconfig $(BOARD_DIR)/defconfig
defconfig:
	$(Q) echo "cp defconfig:"$(BOARD_DIR)/defconfig
	$(Q) cp $(BOARD_DIR)/defconfig $(KCONFIG_CONFIG)

#savedefconfig:
#	$(Q) $(TOPDIR)/scripts/bin/kconfig_tool/savedefconfig$(HOSTEXEEXT) --kconfig $(TOPDIR)/Kconfig --out $(BOARD_DIR)/defconfig
savedefconfig:
	$(Q) echo "save defconfig:"$(BOARD_DIR)/defconfig
	$(Q) cp $(KCONFIG_CONFIG) $(BOARD_DIR)/defconfig

$(KCONFIG_CONFIG):
	$(Q) $(TOPDIR)/scripts/bin/kconfig_tool/defconfig$(HOSTEXEEXT) --kconfig $(TOPDIR)/Kconfig $(BOARD_DIR)/defconfig

###########################################
#
#		config.h:check configuration
#
###########################################
$(KCONFIG_AUTOHEADER): $(KCONFIG_CONFIG) $(SCRIPTSDIR)/Config.mk $(SCRIPTSDIR)/CMarcoDefine.mk $(ARCHDIR)/Toolchain.defs
	$(Q) $(TOPDIR)/scripts/bin/kconfig_tool/genconfig$(HOSTEXEEXT) --config-out=$(KCONFIG_CONFIG) --header-path=$(KCONFIG_AUTOHEADER)
	$(Q) echo "######################################generate config.h######################################"


###########################################
#
#		target
#
###########################################
all: pass1
	$(Q) make BOARD_DIR=$(BOARD_DIR) STEP_PASS2=y pass2

clean:
	$(Q) -rm -rf $(OUTPUTDIR)
	
distclean:
	$(Q) -rm -rf $(TOPDIR)/build
	$(Q) -rm -rf $(GENERATED_DIR)
	
	
###########################################
#
#		pass1: generate objs and libs
#
###########################################
pass1:pass1dep
	$(call SUBDIR_MAKE,all)

if_lib32_support="$(shell dpkg -l | grep "ii\ \ libc6-i386")"
pass1dep:$(KCONFIG_AUTOHEADER)
	$(Q) echo "pass1dep:"$<
ifneq ($(HOSTOS),Cygwin)
ifeq ($(if_lib32_support), "")
	$(Q) echo "* - * - * - * - * - * - * - * - * - * - * - *"
	$(Q) echo "    Make error! Lack of 32-bit support."
	$(Q) echo "    Input the command："
	$(Q) echo "        sudo apt install libc6-i386"
	$(Q) echo "* - * - * - * - * - * - * - * - * - * - * - *"
	$(Q) exit 1
endif
endif

###########################################
#
#			extra objs and libs 
#
###########################################
EXTRA_OBJ_LIB_PATH=$(TOPDIR)/libs

EXTRA_OBJS:=$(wildcard $(EXTRA_OBJ_LIB_PATH)/*.o)

EXTRA_LIBS:=$(notdir $(wildcard $(EXTRA_OBJ_LIB_PATH)/*.a))
EXTRA_LIBS:=$(filter-out libandes.a,$(EXTRA_LIBS))
EXTRA_LINK_LIBGROUPS 	:= $(patsubst lib%.a,-l%,$(EXTRA_LIBS))


###########################################
#
#		pass2: link objs and libs
#
###########################################
ifeq ($(STEP_PASS2),y)
TARGET		:= $(OUTPUTDIR)/$(notdir $(APP_NAME))

ifeq ($(CONFIG_OUTPUT_FORMAT_ELF),y)
OUTPUT_EXT	:= .elf
else
OUTPUT_EXT	:= .a
endif


###########################################
#
#		OTA config
#
###########################################
ifdef CONFIG_OTA_MODE_STRING
OTA_CONFIG_TYPE := $(patsubst "%",%,$(CONFIG_OTA_MODE_STRING))
else
OTA_CONFIG_TYPE := "OTA_COMPRESSION_UPDATE"
endif


LIBPATHS 		:=$(OUTPUTDIR)/libs
LINK_LIBS 		:= $(notdir $(wildcard $(OUTPUTDIR)/libs/*.a))
LINK_LIBGROUPS 	:= $(patsubst lib%.a,-l%,$(LINK_LIBS))

LINK_OBJS_DIR	:= $(shell find $(OUTPUTDIR)/objs -type d)
#LINK_OBJS 		+= $(foreach link_objs_dir, $(LINK_OBJS_DIR),  $(wildcard $(link_objs_dir)/*.o)) 
#LINK_OBJS+=$(OUTPUTDIR)/objs/board/crt0.o

pass2:$(TARGET)$(OUTPUT_EXT)
	$(Q) echo "######################################generate $(TARGET).bin######################################"
$(TARGET)$(OUTPUT_EXT): FORCE
ifeq ($(CONFIG_OUTPUT_FORMAT_ELF),y)
	$(CC)  -nostartfiles -nostdlib -L$(LIBPATHS) -L$(EXTRA_OBJ_LIB_PATH) -o $@ $(LINK_OBJS) $(EXTRA_OBJS) $(LDSTARTGROUP) $(LINK_LIBGROUPS) $(EXTRA_LINK_LIBGROUPS) $(LDENDGROUP) $(ARCHSCRIPT) $(LDFLAGS) -landes -Wl,-Map=$(TARGET).map
#	$(Q) $(OBJCOPY) -O binary -R .lpus_bss $@ $(TARGET).bin
#	$(Q) $(OBJCOPY) -O binary -j .xip $@ $(OUTPUTDIR)/xip.bin
	$(Q) $(OBJDUMP) -d $@ > $(TARGET).lst
	$(Q) $(NM) -S --demangle --size-sort -s -r $@ > $(TARGET).nm
	$(Q) $(READELF) -e -g -t $@ > $(TARGET).readelf
ifeq ($(CONFIG_OUTPUT_VERSION_STANDALONE_ELF),y)
	$(Q) echo "######################################generate standalone elf:$@######################################"
	$(Q) $(TOPDIR)/scripts/bin/trstool/trstool$(HOSTEXEEXT) elf2image $@ --keyfile $(TOPDIR)/scripts//bin/trstool/key.pem --crc_enable False --image_type XIP --ota_type $(OTA_CONFIG_TYPE) --release_version $(REL_V) -b $(BOARD_DIR_ABSPATH) $(OUTPUTDIR)/ECR6600F_$(notdir $(APP_NAME))_cpu_$(XIP_ADDR).bin
	$(Q) $(TOPDIR)/scripts/bin/makeone_tool/makeone$(HOSTEXEEXT) -b $(BOARD_DIR_ABSPATH) -o $(OUTPUTDIR) -a $(notdir $(APP_NAME))
ifeq ("$(OTA_CONFIG_TYPE)","OTA_COMPRESSION_UPDATE")
	$(Q) $(TOPDIR)/scripts/bin/ota_tool/ota_tool$(HOSTEXEEXT) cmz $(BOARD_DIR)/ref_bin/*Partition*.bin $(OUTPUTDIR)/ECR6600F_$(notdir $(APP_NAME))_cpu_$(XIP_ADDR).bin $(OUTPUTDIR)/ECR6600F_$(notdir $(APP_NAME))_Compress_ota_packeg.bin
endif
else ifeq ($(CONFIG_OUTPUT_VERSION_TRANSPORT_ELF),y)
	$(Q) echo "######################################generate transport elf:$@######################################"
#	$(Q) $(OBJCOPY) -O binary $@ $(TARGET)_transport.bin
	$(Q) $(OBJCOPY) -O binary -j .text $@ $(TARGET)_transport_ilm_0x10000.bin
	$(Q) $(OBJCOPY) -O binary -j .data $@ $(TARGET)_transport_dlm_0x60000.bin
	$(Q) $(OBJCOPY) -O binary -j .text_iram $@ $(TARGET)_transport_iram_0x80000.bin
	$(Q) ls -l $(TARGET)_transport_ilm_0x10000.bin | awk '{printf "%08d", $$5}' > $(TARGET)_transport.bin
	$(Q) cat $(TARGET)_transport_ilm_0x10000.bin >> $(TARGET)_transport.bin
	$(Q) ls -l $(TARGET)_transport_dlm_0x60000.bin | awk '{printf "%08d", $$5}' >> $(TARGET)_transport.bin
	$(Q) cat $(TARGET)_transport_dlm_0x60000.bin >> $(TARGET)_transport.bin
	$(Q) ls -l $(TARGET)_transport_iram_0x80000.bin | awk '{printf "%08d", $$5}' >> $(TARGET)_transport.bin
	$(Q) cat $(TARGET)_transport_iram_0x80000.bin >> $(TARGET)_transport.bin
endif

else
ifeq ($(CONFIG_OUTPUT_FORMAT_LIB),y)
		$(Q) cd $(LIBPATHS) && $(AR) $@ $(LINK_LIBS)
endif
endif

endif

allinone:
	python makeone.py -b $(BOARD_DIR_ABSPATH) -o $(OUTPUTDIR_ABSPATH)

# just used for cacl module size, the elf cannot be run 
size: pass1
	$(Q) $(BOARD_DIR)/../common/mdl_size/gen_mdl_ld.sh $(BOARD_DIR)/../common/mdl_size $(OUTPUTDIR)/objs
ifeq ($(CONFIG_LD_SCRIPT_XIP),y)	
	$(CC) -E -nostdinc -P $(AFLAGS) $(BOARD_DIR)/../common/mdl_size/ld_xip.script.S -o $(GENERATED_DIR)/ld.script
else
	$(CC) -E -nostdinc -P $(AFLAGS) $(BOARD_DIR)/../common/mdl_size/ld_ram.script.S -o $(GENERATED_DIR)/ld.script
endif	
	$(Q) make BOARD_DIR=$(BOARD_DIR) STEP_PASS2=y pass2

###########################################
#
#		depend: generate depend files
#
###########################################
depend:
	$(call SUBDIR_MAKE,depend)

help:
	@echo  ''
	@echo  '  Cleaning targets:'
	@echo  '    clean		- Remove most generated files but keep the config '
	@echo  '    distclean		- Remove all generated files'
	
	@echo  ''
	@echo  '  Configuration targets:'
	@echo  '    menuconfig		- Update current config utilising a menu based program'
	@echo  '    defconfig		- New config with default from supplied ./defconfig'
	@echo  '    savedefconfig	- Save current config as ./defconfig (minimal config)'
	
	@echo  ''
	@echo  '  Other generic targets:'
	@echo  '    all		  	- Build all targets marked with [*]'
	@echo  '    depend	  	-Targets used to create header file dependencies'
	
	@echo  ''
	@echo  '  Command line parameters:'
	@echo  '    make V=0|1 [targets] 		0 => quiet build (default), 1 => verbose build'
	@echo  '    make GCC_PATH=xxx/bin/ [targets] 	xxx => Cross compilation chain path'
	@echo  '    make REL_V=a.b.c [targets] 		a.b.c => release version number'
	
	@echo  ''
	@echo  '  Execute "make all" to build all targets marked with [*] '
	@echo  ''

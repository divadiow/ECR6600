.PHONY:all FORCE depend

include $(TOPDIR)/Make.defs


###########################################
#
#			directory path
#
###########################################
SUBDIRS := $(shell find $(CURDIR) -type d )
SUBDIRS	:= $(filter-out $(CURDIR),$(SUBDIRS))

OUTPUTDIR_OBJS:=$(OUTPUTDIR)/objs/board
OUTPUTDIR_DEPS:=$(OUTPUTDIR)/deps/board
OUTPUTDIR_LIB:=$(OUTPUTDIR)/libs
BOARD_COMMON_DIR :=$(BOARD_DIR)/../common
BOARD_PSM_DIR:=$(BOARD_DIR)/../common/psm

###########################################
#
#	C source and header files 	
#
###########################################
ASRCS	:=
CSRCS	:=
PSM_CSRCS:=
INCPATHS:=
MAKEDEFS:= $(BOARD_DIR)/Make.defs $(BOARD_COMMON_DIR)/Make.defs $(BOARD_PSM_DIR)/Make.defs

include $(BOARD_DIR)/Make.defs
include $(BOARD_COMMON_DIR)/Make.defs


CFLAGS := $(foreach incpath, $(INCPATHS), -I "$(incpath)") $(CFLAGS)
AFLAGS += $(foreach incpath, $(INCPATHS), -I "$(incpath)")



###########################################
#
#		target
#
###########################################

LIBTARGET = libboard.a
LIBVERSION = libversion.a
LIBPSM=libpsm.a
ASRCS	:= $(wildcard $(foreach SRC, $(ASRCS), $(addsuffix /$(SRC), $(subst :, ,$(VPATH)))))
CSRCS	:= $(wildcard $(foreach SRC, $(CSRCS), $(addsuffix /$(SRC), $(subst :, ,$(VPATH)))))
AOBJS = $(addprefix $(OUTPUTDIR_OBJS)/,$(ASRCS:.S=.o))
COBJS = $(addprefix $(OUTPUTDIR_OBJS)/,$(CSRCS:.c=.o))
OBJS = $(AOBJS) $(COBJS)
HDEPS:=$(OBJS:.o=.d)
DEPS := $(addprefix $(OUTPUTDIR_DEPS)/,$(CSRCS:.c=.deps))
all: $(OUTPUTDIR_LIB)/$(LIBTARGET) $(OUTPUTDIR_LIB)/$(LIBVERSION) $(OUTPUTDIR_LIB)/$(LIBPSM)


###########################################
#
#		generate ld script
#
###########################################
ifeq ($(CONFIG_FUNC_TRIM),y)
LD_SCRIPTS      :=ld_xip_yy.script.S
else ifeq ($(CONFIG_LD_SCRIPT_XIP),y)
LD_SCRIPTS	:=ld_xip.script.S
else
LD_SCRIPTS	:=ld_ram.script.S
endif

$(LDSCRIPT):$(BOARD_COMMON_DIR)/$(LD_SCRIPTS) FORCE
	$(Q) echo CC:$@
	$(Q) $(CC) -E -nostdinc -P $(AFLAGS) $< -o $@

###########################################
#
#		generate lib version
#
###########################################
version_builtin = $(shell if [ -f $(BOARD_COMMON_DIR)/sdk_version.c ]; then echo "y"; else echo "n"; fi;)
ifeq ($(version_builtin), y)
DEPS += $(OUTPUTDIR_DEPS)/$(BOARD_COMMON_DIR)/sdk_version.deps
$(OUTPUTDIR_LIB)/$(LIBVERSION): $(BOARD_COMMON_DIR)/sdk_version.c FORCE
	$(Q) -mkdir -p $(dir $@)
	$(Q) -mkdir -p $(dir $<)
	$(call COMPILE, $<, $(OUTPUTDIR_OBJS)/$(BOARD_COMMON_DIR)/sdk_version.o)
	$(call ARCHIVE, $@, $(OUTPUTDIR_OBJS)/$(BOARD_COMMON_DIR)/sdk_version.o)
else 
$(OUTPUTDIR_LIB)/$(LIBVERSION):
endif


###########################################
#
#		generate lib psm
#
###########################################

psm_builtin = $(shell if [ -f $(BOARD_PSM_DIR)/Make.defs ]; then echo "y"; else echo "n"; fi;)
ifeq ($(psm_builtin), y)
include $(BOARD_PSM_DIR)/Make.defs

PSM_CSRCS	:= $(foreach SRC, $(PSM_CSRCS), $(addsuffix /$(SRC), $(subst :, ,$(PSM_VPATH))))
PSM_COBJS   := $(addprefix $(OUTPUTDIR_OBJS)/,$(PSM_CSRCS:.c=.o))
HDEPS+=$(OBJS:.o=.d)
-include $(HDEPS)
DEPS += $(addprefix $(OUTPUTDIR_DEPS)/,$(PSM_CSRCS:.c=.deps))

$(PSM_COBJS): $(OUTPUTDIR_OBJS)/%.o: %.c
	$(Q) -mkdir -p $(dir $@)
	$(call COMPILE, $<, $@)

$(OUTPUTDIR_LIB)/$(LIBPSM): $(PSM_COBJS) 
	$(Q) -mkdir -p $(dir $@)
	$(call ARCHIVE, $@, $(PSM_COBJS))

else
-include $(HDEPS)
$(OUTPUTDIR_LIB)/$(LIBPSM):
endif

###########################################
#
#		generate lib board
#
###########################################

	
$(OUTPUTDIR_LIB)/$(LIBTARGET): $(OBJS) $(LDSCRIPT)
#	$(Q) echo "MAKEDEFS:"$(MAKEDEFS)
	$(Q) -mkdir -p $(dir $@)
	$(call ARCHIVE, $@, $(OBJS))

###########################################
#
#		generate objs
#
###########################################

$(AOBJS): $(OUTPUTDIR_OBJS)/%.o: %.S
	$(Q) -mkdir -p $(dir $@)
	$(call ASSEMBLE, $<, $@)

$(COBJS): $(OUTPUTDIR_OBJS)/%.o: %.c
	$(Q) -mkdir -p $(dir $@)
	$(call COMPILE, $<, $@)

###########################################
#
#		generate deps
#
###########################################
depend: $(DEPS)
$(DEPS):$(OUTPUTDIR_DEPS)/%.deps: %.c
	$(Q) -mkdir -p $(dir $@)
	$(call DEPEND, $<, $@)




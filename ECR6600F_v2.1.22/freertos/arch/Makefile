.PHONY:all depend

include $(TOPDIR)/Make.defs

###########################################
#
#			directory path
#
###########################################
SUBDIRS := $(shell find $(CURDIR)/$(CONFIG_CPU_ARCH) -type d )
SUBDIRS	:= $(filter-out $(CURDIR)/$(CONFIG_CPU_ARCH),$(SUBDIRS))

OUTPUTDIR_OBJS:=$(OUTPUTDIR)/objs/arch
OUTPUTDIR_DEPS:=$(OUTPUTDIR)/deps/arch
OUTPUTDIR_LIB:=$(OUTPUTDIR)/libs

###########################################
#
#	C source and header files 	
#
###########################################
ASRCS	:=
CSRCS	:=
INCPATHS:=
MAKEDEFS:=

$(foreach SDIR, $(SUBDIRS), $(eval $(call INC_template,$(SDIR))))

include $(CONFIG_CPU_ARCH)/Make.defs

CFLAGS := $(foreach incpath, $(INCPATHS), -I "$(incpath)") $(CFLAGS)


###########################################
#
#		target
#
###########################################

LIBTARGET = libarch.a
ASRCS	:= $(wildcard $(foreach SRC, $(ASRCS), $(addsuffix /$(SRC), $(subst :, ,$(VPATH)))))
CSRCS	:= $(wildcard $(foreach SRC, $(CSRCS), $(addsuffix /$(SRC), $(subst :, ,$(VPATH)))))
AOBJS = $(addprefix $(OUTPUTDIR_OBJS)/,$(ASRCS:.S=.o))
COBJS = $(addprefix $(OUTPUTDIR_OBJS)/,$(CSRCS:.c=.o))
OBJS = $(AOBJS) $(COBJS)
HDEPS:=$(OBJS:.o=.d)
all: $(OUTPUTDIR_LIB)/$(LIBTARGET)

depend: $(DEPS)
-include $(HDEPS)
###########################################
#
#		generate lib
#
###########################################	

$(OUTPUTDIR_LIB)/$(LIBTARGET): $(OBJS)
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
$(DEPS):$(OUTPUTDIR_DEPS)/%.deps: %.c
	$(Q) -mkdir -p $(dir $@)
	$(call DEPEND, $<, $@)




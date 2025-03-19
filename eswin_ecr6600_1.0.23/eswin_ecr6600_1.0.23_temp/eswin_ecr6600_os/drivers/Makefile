.PHONY:all depend

include $(TOPDIR)/Make.defs


###########################################
#
#			directory path
#
###########################################
SUBDIRS := $(shell find $(CURDIR) -type d )
SUBDIRS	:= $(filter-out $(CURDIR),$(SUBDIRS))

OUTPUTDIR_OBJS:=$(OUTPUTDIR)/objs/drivers
OUTPUTDIR_DEPS:=$(OUTPUTDIR)/deps/drivers
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

$(foreach dir, $(SUBDIRS), $(eval $(call MAKEDEFS_template,$(dir))))

CFLAGS := $(foreach incpath, $(INCPATHS), -I "$(incpath)") $(CFLAGS)

###########################################
#
#		target
#
###########################################

LIBTARGET = libdrivers.a
ASRCS	:= $(wildcard $(foreach SRC, $(ASRCS), $(addsuffix /$(SRC), $(subst :, ,$(VPATH)))))
CSRCS	:= $(wildcard $(foreach SRC, $(CSRCS), $(addsuffix /$(SRC), $(subst :, ,$(VPATH)))))
AOBJS = $(addprefix $(OUTPUTDIR_OBJS)/,$(ASRCS:.S=.o))
COBJS = $(addprefix $(OUTPUTDIR_OBJS)/,$(CSRCS:.c=.o))
OBJS = $(AOBJS) $(COBJS)
HDEPS:=$(OBJS:.o=.d)
DEPS = $(addprefix $(OUTPUTDIR_DEPS)/,$(CSRCS:.c=.deps))
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




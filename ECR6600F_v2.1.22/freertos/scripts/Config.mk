# Disable all built-in rules

.SUFFIXES:

# Control build verbosity
#
#  V=1,2: Enable echo of commands
#  V=2:   Enable bug/verbose options in tools and scripts

ifeq ($(V),1)
export Q :=
else ifeq ($(V),2)
export Q :=
else
export Q := @
endif

# Control Parallel Build Thread Number
#PARALLEL := -j1
#ifeq ($(CONFIG_PARALLEL_BUILD_FOUR),y)
#PARALLEL := -j4
#endif
#ifeq ($(CONFIG_PARALLEL_BUILD_EIGHT),y)
#PARALLEL := -j8
#endif
PARALLEL := -j$(shell nproc --ignore=1)


###########################################
#
#			cpu arch name 
#
###########################################
CONFIG_CPU_ARCH		:= $(patsubst "%",%,$(strip $(CONFIG_CPU_ARCH)))

###########################################
#
#			host machine os
#
###########################################
HOSTOS = ${shell uname -o 2>/dev/null || echo "Other"}

ifeq ($(HOSTOS),Cygwin)
	HOSTEXEEXT ?= .exe
endif


###########################################
#
#			define function 
#
###########################################

# COMPILE - Default macro to compile one C file
# Example: $(call COMPILE, in-file, out-file)

define COMPILE
	@echo "CC: $1" 
	$(Q) $(CC) -MMD -c $(CFLAGS) $($(strip $1)_CFLAGS) $1 -o $2
endef


# ASSEMBLE - Default macro to assemble one assembly language file
# Example: $(call ASSEMBLE, in-file, out-file)

define ASSEMBLE
	@echo "AS: $1"
	$(Q) $(CC) -MMD -c $(AFLAGS) $1 $($(strip $1)_AFLAGS) -o $2
endef


# ARCHIVE - Add a list of files to an archive
# Example: $(call ARCHIVE, archive-file, "file1 file2 file3 ...")

define ARCHIVE
	$(Q) $(AR) $1 $(2)
endef

# MAKEDEFS_template 
# Example: 
define MAKEDEFS_template
	ifneq ($(wildcard $(1)/Make.defs),)
		MAKEDEFS+=$(1)/Make.defs
		include $(1)/Make.defs
	endif
endef

# SUBDIR_MAKE 
# Example: 
define SUBDIR_MAKE
	$(Q)for dir in $(SUBDIRS) ; do \
		if [ -e $$dir/Makefile ]; then \
			make $(PARALLEL) -C $$dir TOPDIR=$(TOPDIR_ABSPATH) OUTPUTDIR=$(OUTPUTDIR_ABSPATH) BOARD_DIR=$(BOARD_DIR_ABSPATH) GENERATED_DIR=$(GENERATED_DIR_ABSPATH) $(1) || exit $$? ; \
		fi \
	done
endef

# DEPEND 
# Example: 

define DEPEND
	$(Q) $(CC) -M $(CFLAGS) $1 > $2
endef

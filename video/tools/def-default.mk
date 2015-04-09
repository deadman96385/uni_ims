#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 25832 $ $Date: 2014-04-24 15:45:33 +0800 (Thu, 24 Apr 2014) $
#

#
# Do not modify this Makefile
#


# Source Tree definition, relative to the top level
export INCLUDE_DIR := $(BASE_DIR)/include.$(PLATFORM)
export LIB_DIR     := $(BASE_DIR)/lib.$(PLATFORM)
export BUILD_DIR   := $(BASE_DIR)/build.$(PLATFORM)
export OBJ_DIR     := $(BASE_DIR)/obj.$(PLATFORM)

# clear built-ins
.SUFFIXES:

# Remove implicit rules
%.o: %.c
%.o: %.s
%.o: %.S
%.exe: %.c
%.a: %.c
%.a: %.o
%.h: ;

# Remove builtins/environment vars
export CC			:=
export LD			:=
export LDFLAGS		:=
export AR			:=
export OBJCOPY		:=
export STRIP		:=
export OBJDUMP		:=
export CFLAGS		:=
export ALL_CFLAGS 	:=

# -------- All external programs used
# GCC is required for C dependency generation, regardless of target type
export CP			:= cp -f
export MV			:= cp -f
export ECHO			:=@echo
export TOUCH		:= touch
export RM			:= rm -f
export PERL			:= perl
export GREP			:= grep
export GCC			:= gcc
export GXX			:= g++

# Define special characters
export EMPTY:=
export SPACE:=$(EMPTY) $(EMPTY)
export COMMA:=,

# -------- Tags
export TAGFILE		:=$(BASE_DIR)/tags
export TAGGEN		:=ctags

# Release Script
export D2RELTAG		= $(PERL) $(TOOL_DIR)/d2_rev/d2reltag.pl
ifeq ($(VPORT_OS),linux)
ifeq ($(VPORT_OS_VER),26)
# Linux 2.6 only
# For MODULE_OUTPUT=lkm, this adds --lkm flag to the cmd line.
export D2RELTAG		= $(PERL) $(TOOL_DIR)/d2_rev/d2reltag.pl \
		$(subst lkm,--lkm,$(findstring lkm,$(MODULE_OUTPUT)))
endif
endif

# Report Script & output file
export REPORT				:= $(PERL) $(TOOL_DIR)/report-$(PLATFORM).pl
export REPORT_OUTPUT		:= $(LIB_DIR)/report.csv
export REPORT_OUTPUT_TXT	:= $(LIB_DIR)/report.txt

export DEFAULT_CFLAGS_GNU = -Wall -D$(PROVIDER)\
					$(addprefix -I,$(INCLUDE_DIR) $(MODULE_DIR) .) \
				 	$(addprefix -I,$(OS_INCLUDE_DIRS)) \
				 	$(addprefix -I,$(SYSTEM_INCLUDE_DIRS))

# END OF MAKEFILE


#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004-2006 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 5171 $ $Date: 2008-01-18 12:56:23 -0500 (Fri, 18 Jan 2008) $
#

# C Source files
CSRC	= \
		vtsp_ut_stream.c \
		vtsp_ut_manage.c \
		vtsp_ut_stream_conf.c \
		vtsp_ut_stream_tone.c \
		vtsp_ut_preinit.c \
		vtsp_ut_control.c \
		vtsp_ut_event.c \
		vtsp_ut_evread.c \
		vtsp_ut_flow.c \
		vtsp_ut_global.c \
		ut.c

ifeq ($(VPORT_OS),linux)
CSRC	+= \
	   ut_linux.c
endif

ifeq ($(VPORT_OS),linux_pc)
CSRC	+= \
	   ut_linux.c
endif

ifeq ($(VPORT_OS),vxworks)
CSRC	+= \
	   ut_vxworks.c
endif

ifeq ($(VPORT_OS),threadx)
CSRC	+= \
	   ut_threadx.c
endif

# Assembly files
SSRC	= \

# Private Header files
PRIVATE_HEADERS	= \

# Files to export to INCLUDE_DIR
PUBLIC_HEADERS	=  \
	   	vtsp_ut.h

# Files to export to OBJ_DIR
OUTPUT			= archive

include $(TOOL_DIR)/rules.mk

# Build Rule - add custom build commands below this rule
build: default_build

# Clean Rule - add custom clean commands below this rule
clean: default_clean


# END OF MAKEFILE


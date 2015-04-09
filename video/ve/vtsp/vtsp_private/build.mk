#!/bin/make
#
#  THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
#  AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
#  APPLIES:
#  "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 12772 $ $Date: 2010-08-20 05:05:49 +0800 (Fri, 20 Aug 2010) $
#

# C Source files
CSRC	= \
		_vtsp_init.c \
	   	_vtsp_map.c \
		_vtsp_stream.c \
		_vtsp_control.c \
		_vtsp_cmd.c \
        _vtsp_flow.c \
		_vtsp_rtcp.c \
		_vtsp_stream_video.c


# Assembly files
SSRC	= \

# Private Header files
PRIVATE_HEADERS	= \
		_vtsp_private.h \

# Files to export to INCLUDE_DIR
PUBLIC_HEADERS	=

# Files to export to OBJ_DIR
OUTPUT			= archive

include $(TOOL_DIR)/rules.mk

# Build Rule - add custom build commands below this rule
build: default_build

# Clean Rule - add custom clean commands below this rule
clean: default_clean


# END OF MAKEFILE


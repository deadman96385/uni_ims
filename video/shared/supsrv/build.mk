#!/bin/make
#
#  THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
#  AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
#  APPLIES:
#  "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 24174 $ $Date: 2014-01-20 17:23:18 +0800 (Mon, 20 Jan 2014) $
#

# C Source files
CSRC	= \
		_supsrv.c \
		supsrv.c \
		supsrv_task.c \

# Assembly files
SSRC	= \

# Private Header files
PRIVATE_HEADERS	= \
		_supsrv.h

# Files to export to INCLUDE_DIR
PUBLIC_HEADERS	=  \
		supsrv.h	\
		supsrv_task.h \

# Files to export to OBJ_DIR
OUTPUT			= archive

include $(TOOL_DIR)/rules.mk

# Build Rule - add custom build commands below this rule
build: default_build

# Clean Rule - add custom clean commands below this rule
clean: default_clean


# END OF MAKEFILE


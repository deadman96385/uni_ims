#!/bin/make
#
#  THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
#  AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
#  APPLIES:
#  "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 11156 $ $Date: 2010-01-14 17:42:56 -0800 (Thu, 14 Jan 2010) $
#



# C Source files
CSRC	= \
		vhw.c \
		_vhw.c \

# Assembly files
SSRC	= \

# Private Header files
PRIVATE_HEADERS	=

# Files to export to INCLUDE_DIR
PUBLIC_HEADERS	=  \
		vhw.h

# Files to export to OBJ_DIR
OUTPUT			= archive

include $(TOOL_DIR)/rules.mk

# Build Rule - add custom build commands below this rule
build: default_build

# Clean Rule - add custom clean commands below this rule
clean: default_clean


# END OF MAKEFILE


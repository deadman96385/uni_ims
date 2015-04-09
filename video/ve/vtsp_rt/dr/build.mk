#!/bin/make
#
#  THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
#  AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
#  APPLIES: "COPYRIGHT 2004-2008 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 5024 $ $Date: 2007-12-27 08:26:45 +0800 (Thu, 27 Dec 2007) $
#

# C Source files
CSRC	= \
		vtsp_rt_dr.c \


# Assembly files
SSRC	= \

# Private Header files
PRIVATE_HEADERS	= \
		vtsp_rt_dr.h 

# Files to export to INCLUDE_DIR
PUBLIC_HEADERS	=  \

# Files to export to OBJ_DIR
OUTPUT			= archive

include $(TOOL_DIR)/rules.mk

# Build Rule - add custom build commands below this rule
build: default_build

# Clean Rule - add custom clean commands below this rule
clean: default_clean


# END OF MAKEFILE


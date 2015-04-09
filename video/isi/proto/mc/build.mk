#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 28012 $ $Date: 2014-08-08 17:36:29 +0800 (Fri, 08 Aug 2014) $
#

# C Source files
CSRC	= \
	_mc_tone.c \
	_mc_hw.c \
	_mc_coder.c \
	_mc_lipsync.c \
	_mc.c \
	mc_main.c \

# Assembly files
SSRC	= 

# Private Header files
PRIVATE_HEADERS	= \

# Files to export to INCLUDE_DIR
PUBLIC_HEADERS = \
	mc_main.h

# Files to export to OBJ_DIR
OUTPUT			= archive

include $(TOOL_DIR)/rules.mk

# Build Rule - add custom build commands below this rule
build: default_build

# Clean Rule - add custom clean commands below this rule
clean: default_clean

# END OF MAKEFILE


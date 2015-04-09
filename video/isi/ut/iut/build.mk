#!/bin/make
#
#  THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
#  AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
#  APPLIES:
#  "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Date: 2010-07-08 06:10:49 +0800 (Thu, 08 Jul 2010) $ $Revision: 12486 $
#

# C Source files
CSRC	= \
		_iut_app.c \
		_iut_cfg.c \
		_iut_prt.c \
		_iut_menu.c \
		iut_main.c \
		_iut_api.c \

ifeq ($(VPORT_OS),linux)
CSRC	+= _iut_linux.c
endif

ifeq ($(VPORT_OS),linux_pc)
CSRC	+= _iut_linux.c
endif


# Assembly files
SSRC	= \

# Private Header files
PRIVATE_HEADERS	= \

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


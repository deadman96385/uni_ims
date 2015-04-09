#!/bin/make
#
#  THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
#  AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
#  APPLIES:
#  "COPYRIGHT 2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 5072 $ $Date: 2008-01-08 11:40:09 -0600 (Tue, 08 Jan 2008) $
#

# C Source files
CSRC	= \
	rfsm_base.c 	\
	_rfsm_none.c	\
	_rfsm_cs.c		\
	_rfsm_lte.c		\
	_rfsm_wifi.c

# Assembly files
SSRC	= \

# Private Header files
PRIVATE_HEADERS	= \
		rfsm.h

# Files to export to INCLUDE_DIR
PUBLIC_HEADERS	=  \

# Files to export to OBJ_DIR
OUTPUT		= archive

include $(TOOL_DIR)/rules.mk

# Build Rule - add custom build commands below this rule
build: default_build

# Clean Rule - add custom clean commands below this rule
clean: default_clean


# END OF MAKEFILE


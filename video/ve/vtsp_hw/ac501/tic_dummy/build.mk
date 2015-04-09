#!/bin/make
#
#  THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
#  AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
#  APPLIES:
#  "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 5870 $ $Date: 2008-04-09 13:16:29 -0400 (Wed, 09 Apr 2008) $
#



# C Source files
CSRC	= \
		tic.c	\
		_tic.c

# Assembly files
SSRC	= \

# Private Header files
PRIVATE_HEADERS	= \
		_tic.h \

# Files to export to INCLUDE_DIR
PUBLIC_HEADERS	=  \
		tic.h

# Files to export to OBJ_DIR
OUTPUT			= archive

include $(TOOL_DIR)/rules.mk

# Build Rule - add custom build commands below this rule
build: default_build

# Clean Rule - add custom clean commands below this rule
clean: default_clean


# END OF MAKEFILE


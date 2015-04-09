#!/bin/make
#
#  THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
#  AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
#  APPLIES:
#  "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 516 $ $Date: 2006-09-21 08:28:39 +0800 (Thu, 21 Sep 2006) $
#

# C Source files
CSRC	= \
		  cidcws.c \
		  cids.c

# Assembly files
SSRC	= \

# Private Header files
PRIVATE_HEADERS	= \
				  cids.h \
				  cidcws.h

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


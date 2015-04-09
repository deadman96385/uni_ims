#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004-2006 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 9111 $ $Date: 2009-03-12 08:59:56 +0800 (Thu, 12 Mar 2009) $
#

# C Source files
CSRC	= \
        rtp.c

# Assembly files
SSRC	= \

# Private Header files
PRIVATE_HEADERS	= \

# Files to export to INCLUDE_DIR
PUBLIC_HEADERS	=  \
                rtp.h

# Files to export to OBJ_DIR
OUTPUT			= archive

include $(TOOL_DIR)/rules.mk

# Build Rule - add custom build commands below this rule
build: default_build

# Clean Rule - add custom clean commands below this rule
clean: default_clean


# END OF MAKEFILE


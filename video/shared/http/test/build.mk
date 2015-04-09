#!/bin/make
#
#  THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
#  AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
#  APPLIES:
#  "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Date: 2012-10-17 14:00:22 +0800 (Wed, 17 Oct 2012) $ $Revision: 18448 $
#

# C Source files
CSRC    = \
        http_test.c \


# Assembly files
SSRC    = \

# Private Header files
PRIVATE_HEADERS = \


# Files to export to INCLUDE_DIR
PUBLIC_HEADERS  =  \

# Files to export to OBJ_DIR
OUTPUT          = archive

include $(TOOL_DIR)/rules.mk

# Build Rule - add custom build commands below this rule
build: default_build

# Clean Rule - add custom clean commands below this rule
clean: default_clean


# END OF MAKEFILE


#!/bin/make
#
#  THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
#  AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
#  APPLIES:
#  "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Date$ $Revision$
#

include Makefile.inc

# C Source files
CSRC	= \
    $(CURL_CFILES) \


# Assembly files
SSRC	= \

# Private Header files
PRIVATE_HEADERS	= \
    $(CURL_HFILES)

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


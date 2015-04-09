#!/bin/make
#
#  THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
#  AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
#  APPLIES:
#  "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 988 $ $Date: 2006-11-02 17:47:08 -0600 (Thu, 02 Nov 2006) $
#

# C Source files
ifeq ($(EXTERN_ENABLE_EZXML_DUMMY),y)
CSRC	= \
		ezxml_dummy.c
else
CSRC	= \
		_ezxml_list.c \
		ezxml_mem.c \
		ezxml.c
endif

# Assembly files
SSRC	= \

# Private Header files
PRIVATE_HEADERS	= \

# Files to export to INCLUDE_DIR
PUBLIC_HEADERS	=  \
		ezxml.h \
		_ezxml_list.h \
		ezxml_mem.h \

# Files to export to OBJ_DIR
OUTPUT			= archive

include $(TOOL_DIR)/rules.mk

# Build Rule - add custom build commands below this rule
build: default_build

# Clean Rule - add custom clean commands below this rule
clean: default_clean


# END OF MAKEFILE


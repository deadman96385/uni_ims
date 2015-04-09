#!/bin/make
#
#  THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
#  AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
#  APPLIES:
#  "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 657 $ $Date: 2006-10-10 05:13:28 +0800 (Tue, 10 Oct 2006) $
#



# C Source files
CSRC	= \
		dsp_extern.c	\
		dsp.c			\

# Assembly files
SSRC	= \

# Private Header files
PRIVATE_HEADERS	=

# Files to export to INCLUDE_DIR
PUBLIC_HEADERS	=	\
		dsp.h

# Files to export to OBJ_DIR
OUTPUT			= archive

include $(TOOL_DIR)/rules.mk

# Build Rule - add custom build commands below this rule
build: default_build

# Clean Rule - add custom clean commands below this rule
clean: default_clean


# END OF MAKEFILE


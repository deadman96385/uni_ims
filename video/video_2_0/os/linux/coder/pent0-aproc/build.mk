#!/bin/make
#
#  THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
#  AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
#  APPLIES:
#  "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 988 $ $Date: 2006-11-02 15:47:08 -0800 (Thu, 02 Nov 2006) $
#

# C Source files
CSRC	= \
        codec.c \
	    codecSem.c \
        codec_decoder.c \
        codec_h264Encoder.c \
        codec_h263Encoder.c \

# Assembly files
SSRC	= \

# Private Header files
PRIVATE_HEADERS	= \

# Files to export to INCLUDE_DIR
PUBLIC_HEADERS	=  \
        codec.h \
	codecSem.h \
        codec_object.h \

# Files to export to OBJ_DIR
OUTPUT			= archive

include $(TOOL_DIR)/rules.mk

# Build Rule - add custom build commands below this rule
build: default_build

# Clean Rule - add custom clean commands below this rule
clean: default_clean


# END OF MAKEFILE


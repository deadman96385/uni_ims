#!/bin/make
#
#  THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
#  AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
#  APPLIES:
#  "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 12087 $ $Date: 2010-05-20 15:14:56 -0400 (Thu, 20 May 2010) $
#

# C Source files
CSRC	= \
        fsm_base.c  \
        _fsm_idle.c \
        _fsm_decode.c \
        _fsm_encode.c \
        _fsm_encodedecode.c \
        _vce_event.c    \
		vce.c           \
        main.c          \

# Assembly files
SSRC	= \

# Private Header files
PRIVATE_HEADERS	= \

# Files to export to INCLUDE_DIR
PUBLIC_HEADERS	=  \
        vce.h \

# Files to export to OBJ_DIR
OUTPUT			= archive

include $(TOOL_DIR)/rules.mk

# Build Rule - add custom build commands below this rule
build: default_build

# Clean Rule - add custom clean commands below this rule
clean: default_clean


# END OF MAKEFILE


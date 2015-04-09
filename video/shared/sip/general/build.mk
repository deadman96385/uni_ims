#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 24709 $ $Date: 2014-02-20 17:27:12 +0800 (Thu, 20 Feb 2014) $
#

# C Source files
CSRC	= \
	    sip_debug.c \
		sip_clib.c \
		sip_list.c \
		sip_token.c \
		sip_mem.c \
		sip_voipnet.c \
		sip_hdrflds.c \
		sip_msgcodes.c 

# Assembly files
SSRC	= 

# Private Header files
PRIVATE_HEADERS	= \

# Files to export to INCLUDE_DIR
PUBLIC_HEADERS	=  \
		sip_clib.h \
		sip_hdrflds.h \
		sip_list.h \
		sip_msgcodes.h \
		sip_token.h \
		sip_mem.h \
		sip_voipnet.h \
		sip_abnfcore.h \
		sip_debug.h 

# Files to export to OBJ_DIR
OUTPUT			= archive

include $(TOOL_DIR)/rules.mk

# Build Rule - add custom build commands below this rule
build: default_build

# Clean Rule - add custom clean commands below this rule
clean: default_clean



# END OF MAKEFILE





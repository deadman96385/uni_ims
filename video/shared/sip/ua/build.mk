#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 24709 $ $Date: 2014-02-20 17:27:12 +0800 (Thu, 20 Feb 2014) $
#

# C Source files
CSRC	= \
		_sip_helpers.c \
		_sip_callback.c \
		_sip_fsm.c \
		_sip_server.c \
		_sip_client.c \
		_sip_error.c \
		_sip_rsrc.c \
		sip_ua.c \
		sip_app.c 
		
# Assembly files
SSRC	= 

# Private Header files
PRIVATE_HEADERS	= \
		_sip_helpers.h \
		_sip_callback.h \
		_sip_fsm.h

# Files to export to INCLUDE_DIR
PUBLIC_HEADERS = \
		sip_ua.h \
		sip_ua_server.h \
		sip_ua_client.h \
		sip_ua_error.h \
		sip_app.h

# Files to export to OBJ_DIR
OUTPUT			= archive

include $(TOOL_DIR)/rules.mk

# Build Rule - add custom build commands below this rule
build: default_build

# Clean Rule - add custom clean commands below this rule
clean: default_clean



# END OF MAKEFILE


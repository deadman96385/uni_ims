#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 24709 $ $Date: 2014-02-20 17:27:12 +0800 (Thu, 20 Feb 2014) $
#

# C Source files
CSRC	= \
		sip_xport.c \
		_sip_drvr.c \
		_sip_descr.c \
		_sip_tcpRecv.c \
		_sip_resolv.c

# Assembly files
SSRC	= 

# Private Header files
PRIVATE_HEADERS	= \
		_sip_drvr.h \
		_sip_descr.h \
		_sip_resolv.h

# Files to export to INCLUDE_DIR
PUBLIC_HEADERS = \
		sip_xport.h

# Files to export to OBJ_DIR
OUTPUT			= archive

include $(TOOL_DIR)/rules.mk

# Build Rule - add custom build commands below this rule
build: default_build

# Clean Rule - add custom clean commands below this rule
clean: default_clean



# END OF MAKEFILE


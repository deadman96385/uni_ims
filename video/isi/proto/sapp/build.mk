#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 20930 $ $Date: 2013-06-04 15:52:05 +0800 (Tue, 04 Jun 2013) $
#

# C Source files
CSRC	= \
		_sapp.c \
		_sapp_parse_helper.c \
		_sapp_dialog.c \
		_sapp_ussd.c \
		_sapp_radio_update.c \
		_sapp_xml.c \
		_sapp_ipsec.c \
		_sapp_reg.c \
		_sapp_mwi.c \
		_sapp_te.c \
		_sapp_cpim_page.c \
		_sapp_im_page.c \
		_sapp_call_settings.c \
		_sapp_coder_helper.c \
		_sapp_capabilities.c \
		_sapp_conf.c \
		_sapp_emergency.c \
		sapp_main.c 

	
# Assembly files
SSRC	= 

# Private Header files
PRIVATE_HEADERS	= \

# Files to export to INCLUDE_DIR
PUBLIC_HEADERS = \
	sapp_main.h

# Files to export to OBJ_DIR
OUTPUT			= archive

include $(TOOL_DIR)/rules.mk

# Build Rule - add custom build commands below this rule
build: default_build

# Clean Rule - add custom clean commands below this rule
clean: default_clean

# END OF MAKEFILE


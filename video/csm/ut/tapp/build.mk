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
CSRC	= \
		tapp_main.c \
		_tapp.c \
		_tapp_mock_sapp.c \
		_tapp_mock_vpmd.c \
		_tapp_mock_gsm.c \
		_tapp_mock_rir.c \
		_tapp_at_infc.c \
		_tapp_report.c \
		_tapp_xml.c \
		_tapp_mock_xcap.c \
		_tapp_mock_xcap_resources.c \
		_tapp_mock_xcap_helper.c \
		_tapp_xml_xcap.c 

# Assembly files
SSRC	= \

# Private Header files
PRIVATE_HEADERS	= \
		_tapp.h \
        _tapp_mock_sapp.h \
        _tapp_mock_vpmd.h \
        _tapp_mock_gsm.h \
        _tapp_mock_rir.h \
        _tapp_at_infc.h \
		_tapp_report.h \
		_tapp_xml.h \
		_tapp_mock_xcap.h 

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


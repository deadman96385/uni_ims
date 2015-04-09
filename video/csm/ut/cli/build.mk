#!/bin/make
#
#  THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
#  AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
#  APPLIES:
#  "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Date: 2013-04-01 01:07:12 -0700 (Mon, 01 Apr 2013) $ $Revision: 20293 $
#

# C Source files
CSRC	= \
		csm_ut_main.c \
		_csm_ut_call.c \
		_csm_ut_event.c \
		_csm_ut.c \
		_csm_ut_sms.c \
		_csm_ut_ussd.c \
		_csm_ut_utils.c \
		_csm_ut_supsrv.c \
		_csm_ut_service.c \
		_csm_ut_emergency.c \

# Assembly files
SSRC	= \

# Private Header files
PRIVATE_HEADERS	= \
		_csm_ut.h \
        _csm_ut_call.h \
        _csm_ut_utils.h \
        _csm_ut_event.h \
        _csm_ut_sms.h \
        _csm_ut_ussd.h \
        _csm_ut_utils.h \
        _csm_ut_supsrv.h \
        _csm_ut_service.h \
        _csm_ut_emergency.h \

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


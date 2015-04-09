#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 21141 $ $Date: 2013-06-20 17:07:08 +0800 (Thu, 20 Jun 2013) $
#

# C Source files
CSRC	= \
	    xcap_xml_reslist.c \
	    xcap_xml_reslist_parse.c \
	    xcap_xml_rls.c \
	    xcap_xml_rls_parse.c \
	    xcap_xml_parules.c \
		xcap_xml_parules_parse.c \
	    xcap_xml_helper.c \
		xcap_helper.c \
		xcap_resources.c \
	    xcap.c \
		_xcap_cmd.c \
		_xcap_task.c \
		_xcap_xact.c \
		
ifeq ($(GBA),y)
export CSRC        +=    \
		_xcap_cmd_gaa.c \
		_xcap_xact_gaa.c
endif

		

# Assembly files
SSRC	= 

# Private Header files
PRIVATE_HEADERS	= \
		_xcap_cmd.h \
		_xcap_xact.h \
		_xcap_task.h \
		_xcap_dbg.h \

# Files to export to INCLUDE_DIR
PUBLIC_HEADERS	=  \
	    xcap_xml_reslist.h \
	    xcap_xml_reslist_parse.h \
	    xcap_xml_rls.h \
	    xcap_xml_rls_parse.h \
	    xcap_xml_parules.h \
		xcap_xml_parules_parse.h \
	    xcap_xml_helper.h \
		xcap_resources.h \
		xcap_helper.h \
		xcap.h  \
		xcap_api.h 

ifeq ($(GBA),y)
export PUBLIC_HEADERS        +=    \
		_xcap_cmd_gaa.h \
		_xcap_xact_gaa.h
endif

# Files to export to OBJ_DIR
OUTPUT			= archive

include $(TOOL_DIR)/rules.mk

# Build Rule - add custom build commands below this rule
build: default_build

# Clean Rule - add custom clean commands below this rule
clean: default_clean



# END OF MAKEFILE





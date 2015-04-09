#!/bin/make
#
#  THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
#  AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
#  APPLIES:
#  "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Date: 2014-07-21 13:26:30 +0800 (Mon, 21 Jul 2014) $ $Revision: 27660 $
#

# C Source files
CSRC	= \
		_isi_port.c \
		_isi_dbg.c \
		_isi_db.c \
		_isi_queue.c \
		_isi_msg.c \
		_isi_sys.c \
		_isi_call.c \
		_isi_service.c \
		_isi_text.c \
		_isi_file.c \
		_isi_evt.c \
		_isi_conf.c \
		_isi_pres.c \
		_isi_media.c \
		_isi_gchat.c \
		_isi_diag.c \
		_isi_ussd.c \
		_isi_xml.c \
		_isi_list.c \
		_isi_mem.c \
		isi.c \

# Assembly files
SSRC	= \

# Private Header files
PRIVATE_HEADERS	= \

# Files to export to INCLUDE_DIR
PUBLIC_HEADERS	=  \
		isi.h \
		isi_errors.h \
		isip.h \

# Files to export to OBJ_DIR
OUTPUT			= archive

include $(TOOL_DIR)/rules.mk

# Build Rule - add custom build commands below this rule
build: default_build

# Clean Rule - add custom clean commands below this rule
clean: default_clean


# END OF MAKEFILE


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
		  _ve_net.c \
		  _ve_rtp_close.c \
		  _ve_rtp_init.c \
		  _ve_rtp_open.c \
		  _ve_rtp_shutdown.c \
		  _ve_map.c \
		  _ve_rtp_send.c \
		  _ve_rtp_recv.c \
		  _ve_rtcp_close.c \
		  _ve_rtcp_init.c \
		  _ve_rtcp_open.c \
		  _ve_rtcp_recv.c \
		  _ve_rtcp_send.c \
		  _ve_rtcp.c \
		  _ve_event.c \
		  _ve_send.c \
		  _ve_recv.c \
		  _ve_cmd.c \
		  _ve_state.c \
		  _ve_coder.c \
		  _ve_stun.c \
		  _ve_task.c \
		  _ve_default.c \
		  ve.c \
		  _ve_stream.c \

# Assembly files
SSRC	= \

# Private Header files
PRIVATE_HEADERS	= \
		_ve_private.h \
		_ve_const.h \
		_ve_struct.h \

# Files to export to INCLUDE_DIR
PUBLIC_HEADERS	=  \
		ve.h		\

# Files to export to OBJ_DIR
OUTPUT			= archive

include $(TOOL_DIR)/rules.mk

# Build Rule - add custom build commands below this rule
build: default_build

# Clean Rule - add custom clean commands below this rule
clean: default_clean


# END OF MAKEFILE


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
		  _vc_net.c \
		  _vc_rtp_close.c \
		  _vc_rtp_init.c \
		  _vc_rtp_open.c \
		  _vc_map.c \
		  _vc_rtp_send.c \
		  _vc_rtp_recv.c \
		  _vc_rtcp_close.c \
		  _vc_rtcp_init.c \
		  _vc_rtcp_open.c \
		  _vc_rtcp_recv.c \
		  _vc_rtcp_send.c \
		  _vc_rtcp.c \
		  _vc_send.c \
		  _vc_cmd.c \
		  _vc_state.c \
		  _vc_default.c \
          _vc_coder.c \
		  vc.c \
		  vci.c \
		  _vc_stream.c \
# Assembly files
SSRC    = \

# Private Header files
PRIVATE_HEADERS    = \
        _vc_private.h \
        _vc_const.h \
        _vc_struct.h \

# Files to export to INCLUDE_DIR
PUBLIC_HEADERS    =  \
        vci.h        \

# Files to export to OBJ_DIR
OUTPUT            = archive

include $(TOOL_DIR)/rules.mk

# Build Rule - add custom build commands below this rule
build: default_build

# Clean Rule - add custom clean commands below this rule
clean: default_clean


# END OF MAKEFILE


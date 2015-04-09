#!/bin/make
#
#  THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
#  AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
#  APPLIES:
#  "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 17840 $ $Date: 2012-08-03 01:06:57 +0800 (Fri, 03 Aug 2012) $
#

# C Source files
CSRC	= \
	   	vtsp_control.c \
		vtsp_event.c   \
		vtsp_flow.c    \
	   	vtsp_manage.c  \
	   	vtsp_stream.c  \
	   	vtsp_stun.c    \
		vtsp_rtcp.c    \
	   	vtsp_stream_video.c

# Assembly files
SSRC	= \

# Private Header files
PRIVATE_HEADERS	= \

# Files to export to INCLUDE_DIR
PUBLIC_HEADERS	=  \
		vtsp.h          \
		vtsp_constant.h \
	   	vtsp_control.h  \
		vtsp_event.h    \
		vtsp_flow.h     \
	   	vtsp_manage.h   \
		vtsp_rtcp.h     \
	   	vtsp_stream.h   \
		vtsp_struct.h   \
	   	vtsp_stun.h     \
	   	vtsp_stream_video.h

# Files to export to OBJ_DIR
OUTPUT			= archive

include $(TOOL_DIR)/rules.mk

# Build Rule - add custom build commands below this rule
build: default_build

# Clean Rule - add custom clean commands below this rule
clean: default_clean


# END OF MAKEFILE


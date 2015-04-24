#!/bin/make
#
#  THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
#  AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
#  APPLIES:
#  "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 18605 $ $Date: 2012-10-25 16:24:11 -0500 (Thu, 25 Oct 2012) $
#

# C Source files
CSRC    = \

# Assembly files
SSRC    = \

# Private Header files
PRIVATE_HEADERS = \

PUBLIC_HEADERS = \

# Files to export to INCLUDE_DIR
PUBLIC_CURL_HEADERS  =   \
		curl.h   \
		curlver.h \
		curlbuild.h	\
		curlrules.h	\
		easy.h	\
		mprintf.h	\
		multi.h \
		stdcheaders.h \
		typecheck-gcc.h \

OUTPUT        = archive

include $(TOOL_DIR)/rules.mk

# Build Rule - add custom build commands below this rule
build: default_build

# Clean Rule - add custom clean commands below this rule
clean: default_clean


# END OF MAKEFILE

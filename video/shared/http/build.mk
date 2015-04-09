#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 29993 $ $Date: 2014-11-21 12:03:21 +0800 (Fri, 21 Nov 2014) $
#

# C Source files
ifeq ($(EXTERN_ENABLE_HTTP_LIBCURL),y)
CSRC    = \
        http_util.c \
        http_curl.c
endif

ifeq ($(EXTERN_ENABLE_HTTP_STATIC_CURL),y)
CSRC    = \
        http_util.c \
        http_curl.c
endif

ifeq ($(EXTERN_ENABLE_HTTP_LIBFETCH),y)
CSRC    = \
        http_util.c \
        http_fetch.c
endif

ifeq ($(EXTERN_ENABLE_HTTP_VXWORKS),y)
CSRC    = \
        http_util.c \
        http_vxworks.c
endif

ifeq ($(EXTERN_ENABLE_HTTP_LIBDOWNLOAD),y)
CSRC    = \
        http_util.c \
        http_download.c
endif

ifeq ($(EXTERN_ENABLE_HTTP_DUMMY),y)
CSRC    = \
        http_util.c \
        http_dummy.c
endif

ifeq ($(EXTERN_ENABLE_HTTP_DAPS),y)
CSRC    = \
        http_util.c \
        http_daps.c
endif

# Assembly files
SSRC    =

# Private Header files
PRIVATE_HEADERS = \

# Files to export to INCLUDE_DIR
PUBLIC_HEADERS  =  \
        http.h

# Files to export to OBJ_DIR
OUTPUT          = archive

include $(TOOL_DIR)/rules.mk

# Build Rule - add custom build commands below this rule
build: default_build

# Clean Rule - add custom clean commands below this rule
clean: default_clean



# END OF MAKEFILE





#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 28398 $ $Date: 2014-08-21 10:54:22 +0800 (Thu, 21 Aug 2014) $
#

# C Source files
CSRC	= \
	_csm_print.c	    \
	_csm_utils.c  		\
	_csm_task.c    		\
	_csm_calls.c   		\
	_csm_sms.c     		\
	_csm_ussd.c    		\
	_csm_service.c 		\
	_csm_response.c  	\
	_csm_radio_policy.c \
	_csm_isi.c          \
	_csm_isi_call.c     \
	_csm_isi_service.c  \
	_csm_isi_sms.c      \
	_csm_isi_tel.c      \
	_csm_isi_ussd.c     \
	csm_main.c

ifeq ($(SUPSRV),y)
export CSRC		+= \
	_csm_supsrv.c
endif

# Assembly files
SSRC	= 

# Private Header files
PRIVATE_HEADERS	= \

# Files to export to INCLUDE_DIR
PUBLIC_HEADERS = \
	csm_main.h 	\
	csm_event.h \

# Files to export to OBJ_DIR
OUTPUT			= archive

include $(TOOL_DIR)/rules.mk

# Build Rule - add custom build commands below this rule
build: default_build

# Clean Rule - add custom clean commands below this rule
clean: default_clean

# END OF MAKEFILE


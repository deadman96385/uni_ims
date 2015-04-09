#!/bin/make
#
#  THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
#  AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
#  APPLIES:
#  "COPYRIGHT 2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 27006 $ $Date: 2014-06-17 12:29:36 +0800 (Tue, 17 Jun 2014) $
#

# C Source files
CSRC	= \
	cfsm_base.c 				\
	_cfsm_dialing.c 			\
	_cfsm_hanging_up.c 			\
	_cfsm_in_call.c 			\
	_cfsm_in_ip_conf.c 			\
	_cfsm_in_cs_conf.c   		\
	_cfsm_on_hold_local.c 		\
	_cfsm_ringing.c 			\
	_cfsm_terminated.c 			\
	_cfsm_trying.c 				\
	_cfsm_waiting.c 			\
	_cfsm_answered.c 			\
	_cfsm_initializing.c		\
	_cfsm_reset.c               \
	_cfsm_in_ims_conf.c

# Assembly files
SSRC	= \

# Private Header files
PRIVATE_HEADERS	= \

# Files to export to INCLUDE_DIR
PUBLIC_HEADERS	=  \
		cfsm.h

# Files to export to OBJ_DIR
OUTPUT		= archive

include $(TOOL_DIR)/rules.mk

# Build Rule - add custom build commands below this rule
build: default_build

# Clean Rule - add custom clean commands below this rule
clean: default_clean


# END OF MAKEFILE


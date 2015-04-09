#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 11751 $ $Date: 2010-04-10 07:48:23 +0800 (六, 10  4月 2010) $
#

#
# This file is for system level meta-configuration constants
# 
# -------- You may modify this file to include customizations
#

# Global Debug flag for symbolic debugging
# Comment out the line below to globally enable debug
#export CDEBUG	= -DSIP_DEBUG_LOG
#export CDEBUG  += -g

#
# List dirs in order for making
export MODULE_SUB_DIRS		:= 	\
		include	\
		port \
		general \
		sdp \
		parser \
		auth \
		dbase \
		timers \
		xact \
		xport \
		tu \
		session \
		dialog \
		ua \
		mem
		
export MODULE_PREFIX	= VSIP
export MODULE_OUTPUT	= rlib

ifdef OSAL_ENABLE_UCLIBC
export MODULE_OUTPUT	= rlib_uclibc
endif

export EXTERN_LIBS		=

# Unix
ifeq ($(VPORT_OS),linux) 
export MODULE_CDEFINES	= $(CDEFINES_ADD_FOR_USERLAND)
export MODULE_CFLAGS	= $(CFLAGS_ADD_FOR_USERLAND)
endif

ifeq ($(VPORT_OS),linux_pc) 
export MODULE_CDEFINES  = $(CDEFINES_ADD_FOR_USERLAND) 
export MODULE_CFLAGS    = $(CFLAGS_ADD_FOR_USERLAND) 
endif

# END OF MAKEFILE


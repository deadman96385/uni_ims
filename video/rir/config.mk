#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
#

#
# This file is for module level configuration
#
# -------- You may modify this file to include customizations
#

#export CDEBUG  = -g

#
# List dirs in order for making
export MODULE_SUB_DIRS		:= 	\
		../rir \
		${PLATFORM}
		
export MODULE_PREFIX	= RIR
export MODULE_OUTPUT	= rlib

export MODULE_OTHER_LIBS		=	\

ifeq ($(VPORT_OS),linux) 
export MODULE_CDEFINES	= $(CDEFINES_ADD_FOR_USERLAND)
export MODULE_CFLAGS	= $(CFLAGS_ADD_FOR_USERLAND)
endif

ifeq ($(VPORT_OS),linux_pc)
export MODULE_CDEFINES	= $(CDEFINES_ADD_FOR_USERLAND)
export MODULE_CFLAGS	= $(CFLAGS_ADD_FOR_USERLAND)
endif

# END OF MAKEFILE

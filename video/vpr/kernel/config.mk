#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev$ $Date$
#

#
# This file is for module level configuration
#
# -------- You may modify this file to include customizations
#

#export CDEBUG  = -g

#
# List dirs in order for making
export MODULE_SUB_DIRS	:= 	\
	../kernel

export MODULE_PREFIX	= VPR_KERNEL

ifeq ($(VPORT_OS),linux) 
export MODULE_SUB_DIRS	+=	\
	vpr_lkm

export MODULE_OUTPUT	= lkm
export MODULE_CDEFINES	= $(CDEFINES_ADD_FOR_LKM)
export MODULE_CFLAGS	= $(CFLAGS_ADD_FOR_LKM)
endif

# END OF MAKEFILE

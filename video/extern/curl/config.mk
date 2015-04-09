#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 8612 $ $Date: 2009-01-05 15:23:47 -0500 (Mon, 05 Jan 2009) $
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
		include/curl \
		lib \

export MODULE_PREFIX	= EXTERN_CURL

ifeq ($(VPORT_OS),linux) 
export MODULE_OUTPUT	= rlib
export MODULE_CDEFINES	= $(CDEFINES_ADD_FOR_USERLAND)
export MODULE_CFLAGS	= $(CFLAGS_ADD_FOR_USERLAND)
endif

ifeq ($(VPORT_OS),linux_pc)
export MODULE_OUTPUT	= rlib 
export MODULE_CDEFINES	= $(CDEFINES_ADD_FOR_USERLAND)
export MODULE_CFLAGS	= $(CFLAGS_ADD_FOR_USERLAND)
endif

MODULE_CDEFINES += \
    -DHAVE_CONFIG_H \

MODULE_CFLAGS += \
    -I$(MODULE_DIR)/include \
    -I$(MODULE_DIR)/include/curl

# END OF MAKEFILE

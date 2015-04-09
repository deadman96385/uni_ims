#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 28569 $ $Date: 2014-08-29 12:19:55 +0800 (Fri, 29 Aug 2014) $
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
		../mc
		
export MODULE_PREFIX	= MC
export MODULE_OUTPUT	= rlib

export MODULE_OTHER_LIBS	+=	\
		$(LIB_DIR)/ve_vtsp.lib

ifeq ($(VPORT_OS),linux)
export MODULE_CDEFINES	= $(CDEFINES_ADD_FOR_USERLAND)
export MODULE_CFLAGS	= $(CFLAGS_ADD_FOR_USERLAND)
endif

ifeq ($(VPORT_OS),linux_pc)
export MODULE_CDEFINES	= $(CDEFINES_ADD_FOR_USERLAND)
export MODULE_CFLAGS	= $(CFLAGS_ADD_FOR_USERLAND)

export MODULE_OTHER_LIBS		+=	\
		$(LIB_DIR)/ve_vtsp_rt.lib \
		$(LIB_DIR)/ve_vtsp_hw.lib
endif

ifeq ($(VPORT_OS),vxworks)
export MODULE_CDEFINES	= $(CDEFINES_ADD_FOR_USERLAND)
export MODULE_CFLAGS	= $(CFLAGS_ADD_FOR_USERLAND)
export MODULE_OTHER_LIBS		+=	\
		$(LIB_DIR)/ve_vtsp_rt.lib \
		$(LIB_DIR)/ve_vtsp_hw.lib
endif

ifeq ($(VPORT_OS),threadx)
export MODULE_CDEFINES	= $(CDEFINES_ADD_FOR_USERLAND)
export MODULE_CFLAGS	= $(CFLAGS_ADD_FOR_USERLAND)
export MODULE_OTHER_LIBS		+=	\
		$(LIB_DIR)/ve_vtsp_rt.lib \
		$(LIB_DIR)/ve_vtsp_hw.lib
endif

# END OF MAKEFILE

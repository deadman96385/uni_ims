#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 28481 $ $Date: 2014-08-26 14:54:55 +0800 (Tue, 26 Aug 2014) $
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
		../vtsp_ut	
		
export MODULE_PREFIX	= VTSP_UT
export MODULE_OUTPUT	= bare-exe

ifeq ($(4G_PLUS),y)
ifeq ($(VPR),y)
export MODULE_OTHER_LIBS        +=    \
        $(LIB_DIR)/vpr.lib
endif
ifeq ($(VPMD),y)
export MODULE_OTHER_LIBS        +=    \
		$(LIB_DIR)/modemDriver_vpmd_mux.lib \
		$(LIB_DIR)/modemDriver_vpmd.lib
endif
endif

export MODULE_OTHER_LIBS		+=	\
		$(LIB_DIR)/osal_user.lib	\
		$(LIB_DIR)/extern.lib	\
		$(LIB_DIR)/ve_vtsp.lib \

ifeq ($(VPORT_OS),linux)
export MODULE_CDEFINES	= $(CDEFINES_ADD_FOR_USERLAND)
export MODULE_CFLAGS	= $(CFLAGS_ADD_FOR_USERLAND)
endif

ifeq ($(VPORT_OS),linux_pc)
export MODULE_CDEFINES	= $(CDEFINES_ADD_FOR_USERLAND)
export MODULE_CFLAGS	= $(CFLAGS_ADD_FOR_USERLAND)
export MODULE_OTHER_LIBS		+=	\
		$(LIB_DIR)/osal_kernel.lib	\
		$(LIB_DIR)/ve_vtsp_rt.lib \
		$(LIB_DIR)/ve_vtsp_hw.lib
endif

ifeq ($(VPORT_OS),vxworks)
export MODULE_CDEFINES	= $(CDEFINES_ADD_FOR_USERLAND)
export MODULE_CFLAGS	= $(CFLAGS_ADD_FOR_USERLAND)
export MODULE_OTHER_LIBS		+=	\
		$(LIB_DIR)/osal_kernel.lib	\
		$(LIB_DIR)/ve_vtsp_rt.lib	\
		$(LIB_DIR)/ve_vtsp_hw.lib
endif

ifeq ($(VPORT_OS),threadx)
export MODULE_CDEFINES	= $(CDEFINES_ADD_FOR_USERLAND)
export MODULE_CFLAGS	= $(CFLAGS_ADD_FOR_USERLAND)
export MODULE_OTHER_LIBS		+=	\
		$(LIB_DIR)/osal_kernel.lib	\
		$(LIB_DIR)/ve_vtsp_rt.lib	\
		$(LIB_DIR)/ve_vtsp_hw.lib
endif

# END OF MAKEFILE

#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 25442 $ $Date: 2014-04-02 13:36:58 +0800 (Wed, 02 Apr 2014) $
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
		../main
		
export MODULE_PREFIX	= MCE
export MODULE_OUTPUT	= bare-exe

export MODULE_OTHER_LIBS		+=	\
		$(LIB_DIR)/isi_proto_mc.lib	\
		$(LIB_DIR)/extern.lib		\
		$(LIB_DIR)/osal_user.lib    \
		$(LIB_DIR)/osal_kernel.lib
	
ifeq ($(4G_PLUS),y)
export MODULE_OTHER_LIBS    +=      \
        $(LIB_DIR)/vpr.lib 
ifeq ($(VPMD),y)
export MODULE_OTHER_LIBS    +=      \
		$(LIB_DIR)/modemDriver_vpmd_mux.lib \
        $(LIB_DIR)/modemDriver_vpmd.lib 
endif
endif

ifeq ($(VPORT_OS),linux) 
export MODULE_CDEFINES	= $(CDEFINES_ADD_FOR_USERLAND)
export MODULE_CFLAGS	= $(CFLAGS_ADD_FOR_USERLAND)
endif

ifeq ($(VPORT_OS),linux_pc)
export MODULE_CDEFINES	= $(CDEFINES_ADD_FOR_USERLAND)
export MODULE_CFLAGS	= $(CFLAGS_ADD_FOR_USERLAND)
endif



# END OF MAKEFILE

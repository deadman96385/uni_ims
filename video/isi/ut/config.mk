#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 25157 $ $Date: 2014-03-17 00:59:26 +0800 (Mon, 17 Mar 2014) $
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
		iut
		
export MODULE_PREFIX	= ISI_UT
export MODULE_OUTPUT	= bare-exe

export MODULE_OTHER_LIBS		=	\
		$(LIB_DIR)/osal_user.lib \
		$(LIB_DIR)/extern.lib 

ifeq ($(4G_PLUS),y)
export MODULE_OTHER_LIBS		+=	\
		$(LIB_DIR)/isi_rpc_xdr.lib \
		$(LIB_DIR)/isi_rpc_client.lib
ifeq ($(VPAD),y)
export MODULE_OTHER_LIBS		+=	\
		$(LIB_DIR)/modemDriver_vpad.lib 
endif
else
export MODULE_OTHER_LIBS		+=	\
		$(LIB_DIR)/isi_libisi.lib 
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

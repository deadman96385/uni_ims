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
export MODULE_SUB_DIRS        :=     \
        ../main

export MODULE_PREFIX    = VPAD_MAIN
export MODULE_OUTPUT    = bare-exe

export MODULE_OTHER_LIBS        +=	\
	$(LIB_DIR)/osal_user.lib	\
	$(LIB_DIR)/modemDriver_vpad.lib	\
	$(LIB_DIR)/modemDriver_vpad_mux.lib

ifeq ($(VPORT_OS),linux)
export MODULE_CDEFINES    = $(CDEFINES_ADD_FOR_USERLAND)
export MODULE_CFLAGS    = $(CFLAGS_ADD_FOR_USERLAND)
endif

ifeq ($(VPORT_OS),linux_pc)
export MODULE_CDEFINES    = $(CDEFINES_ADD_FOR_USERLAND)
export MODULE_CFLAGS    = $(CFLAGS_ADD_FOR_USERLAND)
endif

# END OF MAKEFILE

#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 27419 $ $Date: 2014-07-14 16:38:14 +0800 (Mon, 14 Jul 2014) $
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

export MODULE_PREFIX    = OSAL_UT_MAIN
export MODULE_OUTPUT    = bare-exe

export MODULE_OTHER_LIBS        +=    \
        $(LIB_DIR)/osal_ut_user.lib 


ifeq ($(VPORT_OS),linux)
export MODULE_CDEFINES    = $(CDEFINES_ADD_FOR_USERLAND)
export MODULE_CFLAGS    = $(CFLAGS_ADD_FOR_USERLAND)
endif

ifeq ($(VPORT_OS),linux_pc)
export MODULE_CDEFINES    = $(CDEFINES_ADD_FOR_USERLAND)
export MODULE_CFLAGS    = $(CFLAGS_ADD_FOR_USERLAND)
endif

ifeq ($(VPORT_OS),threadx)
export MODULE_CDEFINES    = $(CDEFINES_ADD_FOR_USERLAND)
export MODULE_CFLAGS    = $(CFLAGS_ADD_FOR_USERLAND)
endif

# END OF MAKEFILE

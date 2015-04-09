#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 28398 $ $Date: 2014-08-21 10:54:22 +0800 (Thu, 21 Aug 2014) $
#

#
# This file is for module level configuration
#
# -------- You may modify this file to include customizations
#

#export CDEBUG  = -g

#
# List dirs in order for making
export MODULE_SUB_DIRS      :=  \
        isi_service_fsm \
        call_fsm        \

ifeq ($(ISIM),y)
export MODULE_SUB_DIRS		+= \
		isim
else
ifeq ($(MGT),y)
export MODULE_SUB_DIRS      += \
    	mgt
endif
endif

ifeq ($(GBA),y)
export MODULE_SUB_DIRS		+= \
		gbam
endif

export MODULE_SUB_DIRS      +=  \
        ../csm

export MODULE_PREFIX    = CSM
export MODULE_OUTPUT    = rlib

export MODULE_OTHER_LIBS        =   \
        $(LIB_DIR)/isi_libisi.lib   \
        $(LIB_DIR)/rpm.lib \
        
ifeq ($(SUPSRV),y)
export MODULE_OTHER_LIBS		+= \
		$(LIB_DIR)/shared_supsrv.lib
endif

ifeq ($(VPORT_OS),linux)
export MODULE_CDEFINES  = $(CDEFINES_ADD_FOR_USERLAND)
export MODULE_CFLAGS    = $(CFLAGS_ADD_FOR_USERLAND)
endif

ifeq ($(VPORT_OS),linux_pc)
export MODULE_CDEFINES  = $(CDEFINES_ADD_FOR_USERLAND)
export MODULE_CFLAGS    = $(CFLAGS_ADD_FOR_USERLAND)

endif
# END OF MAKEFILE

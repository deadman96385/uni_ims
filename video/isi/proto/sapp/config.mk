#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 24721 $ $Date: 2014-02-21 14:41:28 +0800 (Fri, 21 Feb 2014) $
#

#
# This file is for module level configuration
#
# -------- You may modify this file to include customizations
#

#export CDEBUG  = -g

#
# List dirs in order for making
#
export MODULE_OTHER_LIBS		=	\
		$(LIB_DIR)/shared_sip.lib

ifeq ($(SIMPLE),y)
export MODULE_SUB_DIRS		+=	\
		msrp/include \
		msrp/parse \
		msrp/port \
		msrp/dbg \
		msrp/db \
		msrp/chunk \
		msrp/packet \
		msrp/response \
		msrp/report \
		msrp/session \
		msrp/rmngr \
		msrp/connection \
		msrp/send \
		msrp/mem \
		msrp/api \
		mns \
		simple \
		../sapp
else
export MODULE_SUB_DIRS		+= 	\
		mns \
		../sapp
endif
	
export MODULE_PREFIX	= SAPP
export MODULE_OUTPUT	= rlib

ifeq ($(VPORT_OS),linux)
export MODULE_CDEFINES	= $(CDEFINES_ADD_FOR_USERLAND)
export MODULE_CFLAGS	= $(CFLAGS_ADD_FOR_USERLAND)
endif

ifeq ($(VPORT_OS),linux_pc)
export MODULE_CDEFINES	= $(CDEFINES_ADD_FOR_USERLAND)
export MODULE_CFLAGS	= $(CFLAGS_ADD_FOR_USERLAND)
endif

# END OF MAKEFILE

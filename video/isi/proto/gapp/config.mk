#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 17307 $ $Date: 2012-06-01 17:30:15 -0500 (Fri, 01 Jun 2012) $
#

#
# This file is for module level configuration
#
# -------- You may modify this file to include customizations
#

#export CDEBUG  = -g

#
# List dirs in order for making
ifeq ($(GAPP_GSM),y)
export MODULE_SUB_DIRS		+= 	\
		gsm 
endif

export MODULE_SUB_DIRS		+= 	\
		proxy/io

ifeq ($(PLATFORM),$(filter $(PLATFORM),ac501 ac502))
export MODULE_SUB_DIRS		+= 	\
		proxy/io/$(PLATFORM)
else
export MODULE_SUB_DIRS		+= 	\
		proxy/io/linux
endif

export MODULE_SUB_DIRS		+= 	\
		proxy \
		../gapp
		 		

export MODULE_PREFIX	= GAPP
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

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
		../main
		
export MODULE_PREFIX	= SAPPE
export MODULE_OUTPUT	= bare-exe

export MODULE_OTHER_LIBS		=	\
		$(LIB_DIR)/isi_proto_sapp.lib	\
		$(LIB_DIR)/extern.lib		\
		$(LIB_DIR)/osal_user.lib
	
ifeq ($(SR),y) 
export MODULE_OTHER_LIBS	+=		\
        $(LIB_DIR)/sr.lib 
endif

ifeq ($(VPAD),y) 
export MODULE_OTHER_LIBS	+=		\
        $(LIB_DIR)/modemDriver_vpad.lib 
endif

ifeq ($(SETTINGS),y)
export MODULE_OTHER_LIBS        +=    \
        $(LIB_DIR)/shared_settings.lib
endif

ifeq ($(SIMPLE),y)
export MODULE_OTHER_LIBS		+=	\
		$(LIB_DIR)/shared_http.lib		\
		$(LIB_DIR)/shared_xcap.lib


ifeq ($(EXTERN_ENABLE_HTTP_LIBCURL),y)
export OS_LIBS += \
         curl
endif
ifeq ($(EXTERN_ENABLE_HTTP_STATIC_CURL),y)
export MODULE_OTHER_LIBS    +=      \
        $(LIB_DIR)/extern_curl.lib
endif
ifeq ($(EXTERN_ENABLE_HTTP_LIBFETCH),y)
export OS_LIBS += \
         fetch
endif
ifeq ($(EXTERN_ENABLE_HTTP_VXWORKS),y)
export OS_LIBS += \
         curl
endif
ifeq ($(EXTERN_ENABLE_HTTP_LIBDOWNLOAD),y)
export OS_LIBS += \
         download
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

#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 18443 $ $Date: 2012-10-17 13:25:28 +0800 (Wed, 17 Oct 2012) $
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
		../test
		
export MODULE_PREFIX	= HTTP_TEST
export MODULE_OUTPUT	= bare-exe

export MODULE_OTHER_LIBS		=	\
		$(LIB_DIR)/extern.lib \
		$(LIB_DIR)/osal_user.lib   \
		$(LIB_DIR)/shared_http.lib

ifeq ($(EXTERN_ENABLE_HTTP_STATIC_CURL),y)
export MODULE_OTHER_LIBS    +=      \
        $(LIB_DIR)/extern_curl.lib
endif

ifeq ($(EXTERN_ENABLE_HTTP_LIBCURL),y)
export OS_LIBS += \
         curl
endif

export MODULE_CDEFINES	= $(CDEFINES_ADD_FOR_USERLAND)
export MODULE_CFLAGS	= $(CFLAGS_ADD_FOR_USERLAND)


# END OF MAKEFILE

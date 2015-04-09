# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
#

#
# This file is for module level configuration
#
# -------- You may modify this file to include customizations
#

# List dirs in order for making
MY_MODULE_SUB_DIRS		:=
MY_INCLUDE_LIBS			:=

ifeq ($(MY_EXTERN_ENABLE_AUTH),y)
MY_MODULE_SUB_DIRS		+= auth
MY_INCLUDE_LIBS += libextern_auth
endif
ifeq ($(MY_EXTERN_ENABLE_EZXML),y)
MY_MODULE_SUB_DIRS		+= ezxml
MY_INCLUDE_LIBS += libextern_ezxml
endif
ifeq ($(MY_EXTERN_ENABLE_MILENAGE),y)
MY_MODULE_SUB_DIRS		+= milenage
MY_INCLUDE_LIBS += libextern_milenage
endif
ifeq ($(MY_EXTERN_ENABLE_HTTP_PARSE),y)
MY_MODULE_SUB_DIRS		+= http
MY_INCLUDE_LIBS += libextern_http
endif
ifeq ($(MY_EXTERN_ENABLE_PDUCONV),y)
MY_MODULE_SUB_DIRS		+= pdu
MY_INCLUDE_LIBS += libextern_pdu
endif

#================disable==================
ifeq ($(MY_EXTERN_ENABLE_YAHOO),y)
export MY_MODULE_SUB_DIRS		+= \
		yahoo2
endif

ifeq ($(MY_EXTERN_ENABLE_RESOLV),y)
export MY_MODULE_SUB_DIRS      += \
		resolv
endif

ifeq ($(MY_EXTERN_ENABLE_JSMN),y)
export MY_MODULE_SUB_DIRS      += \
		jsmn
endif

ifeq ($(MY_EXTERN_ENABLE_LTE_LIB), wncmal)
export MY_MODULE_SUB_DIRS      += \
		modemvendor/wncmal
endif
#=========================================

MY_CFLAGS := \
	-DOSAL_PTHREADS \
	-DANDROID_ICS


# END OF MAKEFILE

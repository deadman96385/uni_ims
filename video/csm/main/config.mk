#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
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
export MODULE_SUB_DIRS        :=     \
        ../main

export MODULE_PREFIX    = MCE
export MODULE_OUTPUT    = bare-exe

export MODULE_OTHER_LIBS        +=    \
        $(LIB_DIR)/csm.lib    \
        $(LIB_DIR)/extern.lib    \
        $(LIB_DIR)/osal_user.lib   \
        $(LIB_DIR)/shared_pdu.lib

ifeq ($(4G_PLUS),y)
ifeq ($(VOER),y)
export MODULE_OTHER_LIBS        +=    \
		$(LIB_DIR)/voer.lib
endif
ifeq ($(VPAD),y)
export MODULE_OTHER_LIBS        +=    \
		$(LIB_DIR)/modemDriver_vpad_mux.lib \
		$(LIB_DIR)/modemDriver_vpad.lib
endif
ifeq ($(VPMD),y)
export MODULE_OTHER_LIBS        +=    \
		$(LIB_DIR)/modemDriver_vpmd_mux.lib \
		$(LIB_DIR)/modemDriver_vpmd.lib
endif
ifeq ($(VPR),y)
export MODULE_OTHER_LIBS        +=    \
        $(LIB_DIR)/vpr.lib
else
# sr.lib and vpr.lib are exclusive
ifeq ($(SR),y)
export MODULE_OTHER_LIBS        +=    \
        $(LIB_DIR)/sr.lib
endif
endif
ifeq ($(4G_PLUS_RCS),y)
export MODULE_OTHER_LIBS        +=    \
		$(LIB_DIR)/isi_rpc_xdr.lib \
		$(LIB_DIR)/isi_rpc_server.lib
endif
endif # 4G_PLUS

ifeq ($(MC),y)
export MODULE_OTHER_LIBS        +=    \
        $(LIB_DIR)/isi_proto_mc.lib
endif

ifeq ($(GAPP),y)
export MODULE_OTHER_LIBS        +=    \
        $(LIB_DIR)/isi_proto_gapp.lib
endif

ifeq ($(CSM_SAPP),y)
export MODULE_OTHER_LIBS        +=    \
        $(LIB_DIR)/isi_proto_sapp.lib
endif

ifeq ($(RIR),y)
export MODULE_OTHER_LIBS	+= \
		$(LIB_DIR)/rir.lib
endif

ifeq ($(GBA),y)
export MODULE_OTHER_LIBS        +=    \
        $(LIB_DIR)/shared_gba.lib
endif

ifeq ($(XCAP),y)
export MODULE_OTHER_LIBS		+= \
		$(LIB_DIR)/shared_xcap.lib    \
		$(LIB_DIR)/shared_http.lib
endif

ifeq ($(SETTINGS),y)
export MODULE_OTHER_LIBS        +=    \
        $(LIB_DIR)/shared_settings.lib
endif

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

ifeq ($(VPORT_OS),linux)
export MODULE_CDEFINES    = $(CDEFINES_ADD_FOR_USERLAND)
export MODULE_CFLAGS    = $(CFLAGS_ADD_FOR_USERLAND)
endif

ifeq ($(VPORT_OS),linux_pc)
export MODULE_CDEFINES    = $(CDEFINES_ADD_FOR_USERLAND)
export MODULE_CFLAGS    = $(CFLAGS_ADD_FOR_USERLAND)
endif

# END OF MAKEFILE

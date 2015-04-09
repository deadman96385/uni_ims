#!/bin/make
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

#export CDEBUG  = -g

#
# List dirs in order for making
		
export MODULE_PREFIX	= VIDEO

export MODULE_OTHER_LIBS		=	\
		$(LIB_DIR)/osal_user.lib	\
		$(LIB_DIR)/extern.lib	\
		$(LIB_DIR)/ve_rtp.lib	\


ifeq ($(VPORT_OS),linux) 
export MODULE_CDEFINES	= $(CDEFINES_ADD_FOR_USERLAND)
export MODULE_CFLAGS	= $(CFLAGS_ADD_FOR_USERLAND)
endif

ifeq ($(VPORT_OS),linux_pc) 
export MODULE_CDEFINES	= $(CDEFINES_ADD_FOR_USERLAND)
export MODULE_CFLAGS	= $(CFLAGS_ADD_FOR_USERLAND)
endif

export MODULE_SUB_DIRS		= 			\
		common							\
		jbv					\

ifeq (${PLATFORM},qsd01)

export MODULE_OUTPUT	= dynamic 

export MODULE_SUB_DIRS		+= 			\
		coders/common/omx 				\
		vdd/$(PLATFORM)/android 		\
		vcd/$(PLATFORM)/android			\
		ve								\
		os/android/froyo    			\


export MODULE_OTHER_LIBS		+=	\
		$(BSP_VPORT_LIB_ROOT)/../system/lib/libOmxCore.so

OS_LIBS += ui camera camera_client surfaceflinger surfaceflinger_client  binder android_runtime

export MODULE_CDEFINES	+= -I$(MODULE_DIR)/algs/$(PLATFORM) -DCODERS_QCOM_0
export MODULE_CFLAGS	+= -I$(MODULE_DIR)/algs/$(PLATFORM) -DCODERS_QCOM_0

ifeq ($(4G_PLUS),y)
export MODULE_OTHER_LIBS        +=              \
		$(LIB_DIR)/vier.lib				\
		$(LIB_DIR)/voer.lib
endif

ifeq ($(VPAD),y)
export MODULE_OTHER_LIBS        +=              \
		$(LIB_DIR)/modemDriver_vpad.lib
endif

#export MODULE_OTHER_LIB_DIR += $(BSP_VPORT_LIB_ROOT)/../system/lib
endif 

ifeq (${PLATFORM},ac903)

export MODULE_OUTPUT	= dynamic

export MODULE_SUB_DIRS		+= 			\
		dsp/arm							\
		dsp/$(PLATFORM)					\
		vdd/$(PLATFORM)/android	 		\
		coders/common/stagefright		\
		coders/$(PLATFORM)/stagefright	\
		vcd/$(PLATFORM)/android	 		\
		ve								\
		os/android/gingerbread			

export MODULE_OTHER_LIBS		+=	\
		$(MODULE_DIR)/algs/$(PLATFORM)/libomxIP.a \

export MODULE_CDEFINES	+= -I$(MODULE_DIR)/algs/$(PLATFORM)
export MODULE_CFLAGS	+= -I$(MODULE_DIR)/algs/$(PLATFORM)

OS_LIBS += ui cameraservice camera_client surfaceflinger surfaceflinger_client binder android_runtime stagefright stagefright_color_conversion dl OMX_Core

endif

ifeq (${PLATFORM},ac904)

export MODULE_OUTPUT	= dynamic

export MODULE_SUB_DIRS		+= 			\
		vdd/$(PLATFORM)/android	 		\
		coders/common/stagefright		\
		coders/$(PLATFORM)/stagefright	\
		vcd/$(PLATFORM)/android	 		\
		ve								\
		os/android/ics/			

OS_LIBS += ui gui cameraservice camera_client surfaceflinger surfaceflinger_client binder android_runtime stagefright android

endif

ifeq (${PLATFORM},ac905)

export MODULE_OUTPUT	= dynamic

export MODULE_SUB_DIRS		+= 			\
		vdd/$(PLATFORM)/android	 		\
		coders/common/stagefright		\
		coders/$(PLATFORM)/stagefright	\
		vcd/$(PLATFORM)/android	 		\
		ve								\
		os/android/ics/			

OS_LIBS += ui gui cameraservice camera_client surfaceflinger surfaceflinger_client binder android_runtime stagefright stagefright_foundation android

endif

ifeq (${PLATFORM},qsd06)

export MODULE_OUTPUT	= dynamic

export MODULE_SUB_DIRS		+= 			\
		vdd/$(PLATFORM)/android	 		\
		coders/common/stagefright		\
		coders/$(PLATFORM)/stagefright	\
		vcd/$(PLATFORM)/android	 		\
		ve								\
		os/android/ics/			

OS_LIBS += ui gui cameraservice camera_client surfaceflinger surfaceflinger_client binder android_runtime stagefright android

endif

ifeq (${PLATFORM},gal01)

export MODULE_OUTPUT	= dynamic

export MODULE_SUB_DIRS		+= 			\
		vdd/$(PLATFORM)/android	 		\
		coders/common/stagefright		\
		coders/$(PLATFORM)/stagefright	\
		vcd/$(PLATFORM)/android	 		\
		ve								\
		os/android/ics/			

OS_LIBS += ui gui cameraservice camera_client surfaceflinger surfaceflinger_client binder android_runtime stagefright stagefright_foundation android

endif

ifeq (${PLATFORM},gal03-aproc)

export MODULE_OUTPUT	= dynamic

export MODULE_SUB_DIRS		+= 			\
		vdd/gal03/android	 		\
		vcd/gal03/android	 		\
		coders/common/stagefright       \
		coders/gal03/stagefright	\
        ve					\
		os/android/${ANDROID_VERSION}/			

OS_LIBS += ui gui cameraservice camera_client surfaceflinger binder android_runtime stagefright stagefright_foundation android EGL GLESv2 

ifeq ($(4G_PLUS),y)
export MODULE_OTHER_LIBS        +=              \
		$(LIB_DIR)/vier.lib	
endif

ifeq ($(VPAD),y)
export MODULE_OTHER_LIBS        +=              \
		$(LIB_DIR)/modemDriver_vpad.lib
endif

endif

ifeq (${PLATFORM},gal03)

export MODULE_OUTPUT	= dynamic

export MODULE_SUB_DIRS		+= 			\
		vdd/$(PLATFORM)/android	 		\
		vcd/$(PLATFORM)/android	 		\
		coders/common/stagefright       \
		coders/$(PLATFORM)/stagefright	\
		ve					\
		os/android/${ANDROID_VERSION}/			

OS_LIBS += ui gui cameraservice camera_client surfaceflinger binder android_runtime stagefright stagefright_foundation android EGL GLESv2 

ifeq ($(4G_PLUS),y)
export MODULE_OTHER_LIBS        +=              \
		$(LIB_DIR)/vier.lib	
endif

ifeq ($(VPAD),y)
export MODULE_OTHER_LIBS        +=              \
		$(LIB_DIR)/modemDriver_vpad.lib
endif

endif

ifeq (${PLATFORM},i8605)

export MODULE_OUTPUT	= dynamic

export MODULE_SUB_DIRS		+= 			\
		vdd/$(PLATFORM)/android	 		\
		coders/common/stagefright		\
		coders/$(PLATFORM)/stagefright	\
		vcd/$(PLATFORM)/android	 		\
		ve								\
		os/android/ics/			

OS_LIBS += ui gui cameraservice camera_client surfaceflinger surfaceflinger_client binder android_runtime stagefright android stagefright_foundation

endif

ifeq (${PLATFORM},ac810)

export MODULE_OUTPUT	= dynamic

export MODULE_SUB_DIRS		+= 			\
		vdd/$(PLATFORM)/android	 		\
		coders/common/stagefright		\
		coders/$(PLATFORM)/stagefright	\
		vcd/$(PLATFORM)/android	 		\
		ve								\
		os/android/ics/			

OS_LIBS += ui gui cameraservice camera_client surfaceflinger surfaceflinger_client binder android_runtime stagefright android

endif

ifeq (${PLATFORM},pent0-aproc)

export MODULE_OUTPUT	= bare-exe

export MODULE_SUB_DIRS		+= 			\
		dsp/pent0					\
		vdd/pent0/x11		 		\
		vcd/pent0/v4l2	 		\
		coders/common/ffmpeg			\
		coders/common/x264				\
		ve								\
		os/linux    					\

OS_LIBS +=  X11

export MODULE_OTHER_LIBS		+=	\

ifeq ($(4G_PLUS),y) 
export MODULE_OTHER_LIBS	+=		\
		$(LIB_DIR)/vier.lib
endif

ifeq ($(VPAD),y)
export MODULE_OTHER_LIBS        +=              \
		$(LIB_DIR)/modemDriver_vpad.lib
endif

export MODULE_CDEFINES	+= -I$(MODULE_DIR)/algs/pent0
export MODULE_CFLAGS	+= -I$(MODULE_DIR)/algs/pent0

endif

ifeq (${PLATFORM},pent0)

export MODULE_OUTPUT	= bare-exe

export MODULE_SUB_DIRS		+= 			\
		dsp/$(PLATFORM)					\
		vdd/$(PLATFORM)/x11		 		\
		vcd/$(PLATFORM)/v4l2	 		\
		coders/common/ffmpeg			\
		coders/common/x264				\
		ve								\
		os/linux    					\

OS_LIBS +=  X11

export MODULE_OTHER_LIBS		+=	\

ifeq ($(4G_PLUS),y) 
export MODULE_OTHER_LIBS	+=		\
		$(LIB_DIR)/vier.lib
endif

ifeq ($(VPAD),y)
export MODULE_OTHER_LIBS        +=              \
		$(LIB_DIR)/modemDriver_vpad.lib
endif

export MODULE_CDEFINES	+= -I$(MODULE_DIR)/algs/$(PLATFORM)
export MODULE_CFLAGS	+= -I$(MODULE_DIR)/algs/$(PLATFORM)

endif

ifeq (${PLATFORM},ac814)

export MODULE_OUTPUT	= bare-exe

export MODULE_SUB_DIRS		+= 			\
		dsp/${PLATFORM}					\
		vdd/${PLATFORM}/dummy	 		\
		vcd/${PLATFORM}/dummy	 		\
		coders/common/gstreamer			\
		ve								\
		os/linux    					\

OS_LIBS +=  X11

#export MODULE_OTHER_LIBS		+=	\

ifeq ($(4G_PLUS),y) 
export MODULE_OTHER_LIBS	+=		\
        $(LIB_DIR)/modemDriver_vpad.lib \
		$(LIB_DIR)/vier.lib
endif

export MODULE_CDEFINES	+= -I$(MODULE_DIR)/algs/${PLATFORM}
export MODULE_CFLAGS	+= -I$(MODULE_DIR)/algs/${PLATFORM}

endif

# END OF MAKEFILE

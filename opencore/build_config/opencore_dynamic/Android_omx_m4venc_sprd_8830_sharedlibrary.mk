LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_WHOLE_STATIC_LIBRARIES := \
	libomx_m4venc_component_lib \
 	libsprdm4vencoder

LOCAL_MODULE := libomx_m4venc_sharedlibrary
LOCAL_MODULE_TAGS := optional


LOCAL_PRELINK_MODULE := false

#-include $(PV_TOP)/Android_system_extras.mk

LOCAL_SHARED_LIBRARIES +=   libomx_sharedlibrary libopencore_common libutils libbinder libui

include $(BUILD_SHARED_LIBRARY)
include   $(PV_TOP)/codecs_v2/omx/omx_m4venc_sprd_8830/Android.mk
include   $(PV_TOP)/codecs_v2/video/m4v_h263_sprd/sc8830/enc/Android.mk


LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_WHOLE_STATIC_LIBRARIES := \
	libomx_m4v_component_lib \
 	libsprdmp4decoder

LOCAL_MODULE := libomx_m4vdec_sharedlibrary
LOCAL_MODULE_TAGS := optional

LOCAL_PRELINK_MODULE := false

#-include $(PV_TOP)/Android_system_extras.mk

LOCAL_SHARED_LIBRARIES +=   libomx_sharedlibrary libopencore_common libutils libbinder libui

include $(BUILD_SHARED_LIBRARY)
include   $(PV_TOP)/codecs_v2/omx/omx_m4v_sprd_8800g/Android.mk
include   $(PV_TOP)/codecs_v2/video/m4v_h263_sprd/sc8810/dec/Android.mk



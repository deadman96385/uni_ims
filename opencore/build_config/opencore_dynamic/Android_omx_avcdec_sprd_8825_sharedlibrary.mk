LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_WHOLE_STATIC_LIBRARIES := \
	libomx_avc_component_lib \
 	libsprdavcdecoder

LOCAL_MODULE := libomx_avcdec_sharedlibrary
LOCAL_MODULE_TAGS := optional


LOCAL_PRELINK_MODULE := false

#-include $(PV_TOP)/Android_system_extras.mk

LOCAL_SHARED_LIBRARIES +=   libomx_sharedlibrary libopencore_common libutils libbinder  libui

include $(BUILD_SHARED_LIBRARY)
include   $(PV_TOP)/codecs_v2/omx/omx_h264_sprd_8825/Android.mk
include   $(PV_TOP)/codecs_v2/video/avc_h264_sprd/sc8825/dec/Android.mk


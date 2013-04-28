LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)


#LOCAL_WHOLE_STATIC_LIBRARIES := \
#	libomx_avcenc_component_lib \
# 	libsprdavcencoder

LOCAL_WHOLE_STATIC_LIBRARIES := \
 	libsprdavcencoder

LOCAL_MODULE := libomx_avcenc_sharedlibrary
LOCAL_MODULE_TAGS := optional


LOCAL_PRELINK_MODULE := false

#-include $(PV_TOP)/Android_system_extras.mk

LOCAL_SHARED_LIBRARIES +=   libomx_sharedlibrary libopencore_common libutils libbinder  libui

include $(BUILD_SHARED_LIBRARY)
#include   $(PV_TOP)/codecs_v2/omx/omx_h264enc/Android.mk
include   $(PV_TOP)/codecs_v2/video/avc_h264_sprd/sc8830/enc/Android.mk


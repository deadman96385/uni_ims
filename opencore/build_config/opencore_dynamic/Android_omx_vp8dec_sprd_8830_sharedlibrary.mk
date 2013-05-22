LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_WHOLE_STATIC_LIBRARIES := \
 	libsprdvp8decoder

LOCAL_MODULE := libomx_vp8dec_sharedlibrary
LOCAL_MODULE_TAGS := optional


LOCAL_PRELINK_MODULE := false

#-include $(PV_TOP)/Android_system_extras.mk

LOCAL_SHARED_LIBRARIES +=   libomx_sharedlibrary libopencore_common libutils libbinder  libui

include $(BUILD_SHARED_LIBRARY)
include   $(PV_TOP)/codecs_v2/video/vp8_sprd/sc8830/dec/Android.mk


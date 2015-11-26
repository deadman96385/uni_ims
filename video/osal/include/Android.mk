LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

include $(LOCAL_PATH)/config.mk

LOCAL_CFLAGS := \
		$(MY_SYSTEM_CFLAGS) \
		$(MY_CFLAGS)

LOCAL_32_BIT_ONLY := true

LOCAL_MODULE := libosal_include
LOCAL_MODULE_TAGS := optional
include $(BUILD_STATIC_LIBRARY)

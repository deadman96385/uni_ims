# Copyright 2006 The Android Open Source Project

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	h263.c \
	h264.c

LOCAL_CFLAGS := \
	$(MY_SYSTEM_CFLAGS) \
	$(MY_CFLAGS)

LOCAL_C_INCLUDES := $(MY_INCLUDE_DIR)

LOCAL_32_BIT_ONLY := true

LOCAL_MODULE := libvideo_2_0_coder_common
LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)


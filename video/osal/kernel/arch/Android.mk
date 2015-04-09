# Copyright 2006 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
		osal_arch_count.c

LOCAL_CFLAGS := \
	$(MY_SYSTEM_CFLAGS) \
	$(MY_CFLAGS)

LOCAL_MODULE:= libosal_kernel
LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)

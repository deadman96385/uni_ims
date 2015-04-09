# Copyright 2006 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
		  ac702/vpad_io.c \

LOCAL_CFLAGS := \
	$(MY_SYSTEM_CFLAGS) \
	$(MY_CFLAGS) \
	-DOSAL_PTHREADS

LOCAL_C_INCLUDES := $(MY_INCLUDE_DIR)

LOCAL_MODULE:= libvpad_io
LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)

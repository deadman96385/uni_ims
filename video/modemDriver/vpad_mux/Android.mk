# Copyright 2006 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	vpad_mux.c \
	_vpad_mux_helper.c

LOCAL_CFLAGS := \
	$(MY_SYSTEM_CFLAGS) \
	$(MY_CFLAGS) \
	-DOSAL_PTHREADS \
	-DOSAL_LOG_NDK

LOCAL_C_INCLUDES := $(MY_INCLUDE_DIR)

LOCAL_MODULE:= libvpad_mux
LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)

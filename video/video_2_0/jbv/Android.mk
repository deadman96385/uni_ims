# Copyright 2006 The Android Open Source Project

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	_jbv.c \
	jbv.c \
	_jbv_coder.c \
	_jbv_packet_loss_utils.c

LOCAL_CFLAGS := \
	 $(MY_SYSTEM_CFLAGS) \
	 $(MY_CFLAGS)

LOCAL_C_INCLUDES := $(MY_INCLUDE_DIR)

LOCAL_MODULE:= libvideo_2_0_jbv
LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)


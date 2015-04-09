# Copyright 2006 The Android Open Source Project

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(EXTERN_ENABLE_EZXML_DUMMY), y)
LOCAL_SRC_FILES := \
	ezxml_dummy.c
else
LOCAL_SRC_FILES := \
	_ezxml_list.c \
	ezxml_mem.c \
	ezxml.c
endif

LOCAL_CFLAGS := \
	 $(MY_SYSTEM_CFLAGS) \
	 $(MY_CFLAGS)

LOCAL_C_INCLUDES := $(MY_INCLUDE_DIR)

LOCAL_MODULE:= libextern_ezxml
LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)


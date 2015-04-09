# Copyright 2006 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	vhw.c \

LOCAL_CFLAGS := \
	$(MY_SYSTEM_CFLAGS) \
	$(MY_CFLAGS)

LOCAL_C_INCLUDES := \
	$(MY_INCLUDE_DIR) 

LOCAL_MODULE:= libve_vtsp_hw_vhw_dummy

LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)

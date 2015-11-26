# Copyright 2006 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	_vtsp_init.c \
	_vtsp_map.c \
	_vtsp_stream.c \
	_vtsp_control.c \
	_vtsp_cmd.c \
	_vtsp_flow.c \
	_vtsp_rtcp.c \
	_vtsp_stream_video.c\

LOCAL_CFLAGS := \
	$(MY_SYSTEM_CFLAGS) \
	$(MY_CFLAGS)

LOCAL_C_INCLUDES := \
	$(MY_INCLUDE_DIR)

LOCAL_32_BIT_ONLY := true

LOCAL_MODULE:= libve_vtsp_private

LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)

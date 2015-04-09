# Copyright 2006 The Android Open Source Project

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	vc.c \
	_vc_cmd.c \
	_vc_coder.c \
	_vc_default.c \
	vci.c \
	_vc_map.c \
	_vc_net.c \
	_vc_rtcp.c \
	_vc_rtcp_close.c \
	_vc_rtcp_init.c \
	_vc_rtcp_open.c \
	_vc_rtcp_recv.c \
	_vc_rtcp_send.c \
	_vc_rtp_close.c \
	_vc_rtp_init.c \
	_vc_rtp_open.c \
	_vc_rtp_recv.c \
	_vc_rtp_send.c \
	_vc_send.c \
	_vc_state.c \
	_vc_stream.c

LOCAL_CFLAGS := \
	 $(MY_SYSTEM_CFLAGS) \
	 $(MY_CFLAGS)

LOCAL_C_INCLUDES := $(MY_INCLUDE_DIR)

LOCAL_MODULE:= libvideo_2_0_vc
LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)


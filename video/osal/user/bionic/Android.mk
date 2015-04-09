# Copyright 2006 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
		osal_log.c \
		osal_mem.c \
		osal_task.c \
		osal_random.c \
		osal_net.c \
		osal_select.c \
		osal_string.c \
		osal_tmr.c \
		osal_sem.c \
		osal_cond.c \
		osal_msg.c \
		osal_time.c \
		osal_file.c \
		osal_sys.c \
		osal_ipsec.c \
		osal_crypto.c

LOCAL_CFLAGS := \
	$(MY_SYSTEM_CFLAGS) \
	$(MY_CFLAGS)

LOCAL_C_INCLUDES := $(MY_INCLUDE_DIR)

LOCAL_MODULE:= libosal_user_bionic
LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)

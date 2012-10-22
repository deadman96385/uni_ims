LOCAL_PATH := $(call my-dir)

common_flags := -DANDROID_CHANGES -DEBUG
common_libs := libcutils libc

include $(CLEAR_VARS)
LOCAL_SRC_FILES := sprd_monitor.c
LOCAL_CFLAGS := $(common_flags)
LOCAL_MODULE_TAGS := optional
LOCAL_STATIC_LIBRARIES := $(common_libs)

LOCAL_MODULE := sprd_monitor
include $(BUILD_EXECUTABLE)


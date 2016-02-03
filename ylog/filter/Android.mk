LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := filter_android_main.c
LOCAL_MODULE := libfilter_android_main
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	filter_android_system.cpp \
	logEntryBase.cpp

LOCAL_MODULE := libfilter_android_system
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES += libcutils

# include $(BUILD_SHARED_LIBRARY)
include $(BUILD_EXECUTABLE)
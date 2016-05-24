LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_ADDITIONAL_DEPENDENCIES := $(common_deps)
LOCAL_SRC_FILES := access_socket_fcnt_connect_close_test.c
LOCAL_MODULE := access_socket_fcnt_connect_close_test
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libcutils
include $(BUILD_EXECUTABLE)

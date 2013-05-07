LOCAL_PATH := $(call my-dir)

common_src := \
        channel_manager.c \
        at_tok.c \
        cmux.c \
        pty.c \
        send_thread.c\
        receive_thread.c\
        adapter.c\
        ps_service.c

common_flags := -DANDROID_CHANGES -DEBUG

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(common_src)
LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_SHARED_LIBRARIES := libcutils
LOCAL_CFLAGS := $(common_flags)
LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := phoneserver
include $(BUILD_EXECUTABLE)


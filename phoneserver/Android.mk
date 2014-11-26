LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := channel_manager.c \
        at_tok.c \
        cmux.c \
        pty.c \
        send_thread.c\
        receive_thread.c\
        adapter.c\
        ps_service.c

LOCAL_SHARED_LIBRARIES := libcutils libhardware_legacy
LOCAL_CFLAGS := -DANDROID_CHANGES -DEBUG
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS_arm := -marm

LOCAL_MODULE := phoneserver
LOCAL_MODULE_STEM_32 := phoneserver
LOCAL_MODULE_STEM_64 := phoneserver64
LOCAL_MULTILIB := both

include $(BUILD_EXECUTABLE)


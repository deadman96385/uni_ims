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
ifeq ($(BOARD_USE_VETH),true)
common_flags += -DCONFIG_VETH
endif

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(common_src)
LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_SHARED_LIBRARIES := libcutils
LOCAL_CFLAGS := $(common_flags)
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS += -DCONFIG_SINGLE_SIM
LOCAL_MODULE := phoneserver
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(common_src)
LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_SHARED_LIBRARIES := libcutils
LOCAL_CFLAGS := $(common_flags)
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS += -DCONFIG_DUAL_SIM
LOCAL_MODULE := phoneserver_2sim
include $(BUILD_EXECUTABLE)

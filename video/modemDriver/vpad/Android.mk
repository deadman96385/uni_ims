# Copyright 2006 The Android Open Source Project

##################VPAD_MAIN_LIB###########################
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	vpad.c \
	main/vpad_main.c \

LOCAL_CFLAGS := \
	$(MY_SYSTEM_CFLAGS) \
	$(MY_CFLAGS)

LOCAL_C_INCLUDES := $(MY_INCLUDE_DIR)

LOCAL_32_BIT_ONLY := true

LOCAL_MODULE:= libvpad
LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)

##################VPAD_TEST_LIB###########################

ifeq ($(MY_4G_PLUS_MODEM_TEST),y)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	vpad.c \
	test/test_txrx.c \


LOCAL_CFLAGS := \
	$(MY_SYSTEM_CFLAGS) \
	$(MY_CFLAGS)

LOCAL_C_INCLUDES := $(MY_INCLUDE_DIR)

LOCAL_32_BIT_ONLY := true

LOCAL_MODULE:= libvpad_test
LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)

endif

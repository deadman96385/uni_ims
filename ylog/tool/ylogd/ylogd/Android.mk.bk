LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_ADDITIONAL_DEPENDENCIES := $(common_deps)
LOCAL_SRC_FILES := ylogd.c
LOCAL_MODULE := ylogd
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libcutils
include $(BUILD_EXECUTABLE)

CUSTOM_MODULES += ylogd # no need appending this module in PRODUCT_PACKAGES

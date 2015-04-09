LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

include $(LOCAL_PATH)/config.mk
include $(call all-subdir-makefiles, $(MY_MODULE_SUB_DIRS))

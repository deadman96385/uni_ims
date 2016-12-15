LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

#PROVIDER := PROVIDER_CMCC

MY_VIDEO_DIR := video_2_0

include $(LOCAL_PATH)/system-malibu.mk
include $(LOCAL_PATH)/system-ac702.mk

MY_INCLUDE_DIR := \
        $(LOCAL_PATH)/include_ac702

include $(call all-named-subdir-makefiles, $(MY_MODULES))


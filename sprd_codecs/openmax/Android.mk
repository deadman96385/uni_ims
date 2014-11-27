LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include $(call all-makefiles-under,$(LOCAL_PATH))
include $(LOCAL_PATH)/h264dec_hw.mk
include $(LOCAL_PATH)/h264dec_sw.mk
include $(LOCAL_PATH)/h264enc_hw.mk
include $(LOCAL_PATH)/m4vh263dec_hw.mk
include $(LOCAL_PATH)/m4vh263dec_sw.mk
include $(LOCAL_PATH)/m4vh263enc_hw.mk
include $(LOCAL_PATH)/vpxdec_hw.mk


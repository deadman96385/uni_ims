LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/vp8dec_interface.c \
	../../../../vsp/sc8830/src/vsp_drv_sc8830.c


LOCAL_MODULE := libomx_vpxdec_hw_sprd
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS :=  -fno-strict-aliasing  -DVP8_DEC -D_VSP_LINUX_  -D_VSP_  -DCHIP_ENDIAN_LITTLE  -DCHIP_8830
#-DOR_CACHE_OFF

LOCAL_ARM_MODE := arm

LOCAL_SHARED_LIBRARIES := \
	libutils

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(TOP)/vendor/sprd/proprietories-source/sprd_codecs/openmax/vsp/sc8830/inc \

include $(BUILD_SHARED_LIBRARY)

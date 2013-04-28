LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/h264enc_interface.c \
	../../../vsp/sc8830/src/vsp_drv_sc8830.c


LOCAL_MODULE := libsprdavcencoder
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS :=  -fno-strict-aliasing -DH264_ENC -D_VSP_LINUX_  -D_VSP_  -DCHIP_ENDIAN_LITTLE  -DCHIP_8830 $(PV_CFLAGS)
LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES := 

LOCAL_SHARED_LIBRARIES := 

LOCAL_C_INCLUDES := \
	$(PV_TOP)/codecs_v2/video/vsp/sc8830/inc \
	$(PV_TOP)/codecs_v2/video/avc_h264_sprd/sc8830/enc/include \
	$(PV_TOP)/codecs_v2/video/avc_h264_sprd/sc8830/src	\
	$(PV_TOP)/codecs_v2/video/vsp/sc8830/src \
 	$(PV_INCLUDES)

LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)

include $(BUILD_STATIC_LIBRARY)

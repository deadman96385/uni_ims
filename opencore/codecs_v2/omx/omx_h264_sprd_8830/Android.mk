LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/omx_avc_component.cpp \
 	src/avc_dec.cpp


LOCAL_MODULE := libomx_avc_component_lib
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS :=  -fno-strict-aliasing -D_VSP_LINUX_  -DCHIP_ENDIAN_LITTLE  -DCHIP_8830 $(PV_CFLAGS)

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES := 

LOCAL_SHARED_LIBRARIES := 

LOCAL_C_INCLUDES := \
	$(PV_TOP)/codecs_v2/omx/omx_h264_sprd_8830/src \
 	$(PV_TOP)/codecs_v2/omx/omx_h264_sprd_8830/include \
 	$(PV_TOP)/extern_libs_v2/khronos/openmax/include \
 	$(PV_TOP)/codecs_v2/omx/omx_baseclass/include \
 	$(PV_TOP)/codecs_v2/video/avc_h264_sprd/sc8830/dec/include \
	$(PV_TOP)/codecs_v2/video/vsp/sc8830/inc \
 	$(PV_INCLUDES) \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video \
	$(TOP)/device/sprd/common/libs/gralloc \
	$(TOP)/device/sprd/common/libs/mali/src/ump/include \
	$(TOP)/frameworks/native/include/media/hardware


include $(BUILD_STATIC_LIBRARY)

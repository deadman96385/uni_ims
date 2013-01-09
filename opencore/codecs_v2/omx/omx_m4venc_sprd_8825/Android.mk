LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/omx_mpeg4enc_component.cpp \
 	src/mpeg4_enc.cpp


LOCAL_MODULE := libomx_m4venc_component_lib
LOCAL_MODULE_TAGS := optional


LOCAL_CFLAGS :=  -fno-strict-aliasing -D_VSP_LINUX_  -D_VSP_  -DCHIP_ENDIAN_LITTLE  -DCHIP_8825 $(PV_CFLAGS)


LOCAL_STATIC_LIBRARIES := 

LOCAL_SHARED_LIBRARIES := 

LOCAL_C_INCLUDES := \
	$(PV_TOP)/codecs_v2/omx/omx_m4venc_sprd_8825/src \
 	$(PV_TOP)/codecs_v2/omx/omx_m4venc_sprd_8825/include \
 	$(PV_TOP)/extern_libs_v2/khronos/openmax/include \
 	$(PV_TOP)/codecs_v2/video/m4v_h263_sprd/sc8825/enc/include \
 	$(PV_INCLUDES) \
	$(TOP)/frameworks/native/include/media/hardware


LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)

include $(BUILD_STATIC_LIBRARY)

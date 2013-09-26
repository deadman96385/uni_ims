LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/h264dec_biaridecod.c \
	src/h264dec_bitstream.c \
	src/h264dec_buffer.c \
	src/h264dec_context_init.c \
	src/h264dec_ctx_table.c \
	src/h264dec_global.c \
	src/h264dec_header.c \
	src/h264dec_image.c \
	src/h264dec_init.c \
	src/h264dec_interface.c \
	src/h264dec_malloc.c \
	src/h264dec_parset.c \
	src/h264dec_slice.c \
	src/h264dec_table.c \
	../../../../vsp/sc8830/src/vsp_drv_sc8830.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(TOP)/vendor/sprd/proprietories-source/sprd_codecs/openmax/vsp/sc8830/inc \

LOCAL_CFLAGS :=  -fno-strict-aliasing -DH264_DEC -D_VSP_LINUX_  -D_VSP_  -DCHIP_ENDIAN_LITTLE  -DCHIP_8830 
LOCAL_ARM_MODE := arm

LOCAL_SHARED_LIBRARIES := \
	libutils

LOCAL_MODULE := libomx_avcdec_hw_sprd
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

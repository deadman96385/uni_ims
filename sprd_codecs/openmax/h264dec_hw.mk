LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sc8830)
H264_PATH := avc_h264_mix/dec/hw/sc8830/
VSP_PATH := vsp/sc8830/
endif

LOCAL_SRC_FILES := \
	$(H264_PATH)/src/h264dec_biaridecod.c \
	$(H264_PATH)/src/h264dec_bitstream.c \
	$(H264_PATH)/src/h264dec_buffer.c \
	$(H264_PATH)/src/h264dec_context_init.c \
	$(H264_PATH)/src/h264dec_ctx_table.c \
	$(H264_PATH)/src/h264dec_global.c \
	$(H264_PATH)/src/h264dec_header.c \
	$(H264_PATH)/src/h264dec_image.c \
	$(H264_PATH)/src/h264dec_init.c \
	$(H264_PATH)/src/h264dec_interface.c \
	$(H264_PATH)/src/h264dec_malloc.c \
	$(H264_PATH)/src/h264dec_parset.c \
	$(H264_PATH)/src/h264dec_slice.c \
	$(H264_PATH)/src/h264dec_table.c \
	$(VSP_PATH)/src/vsp_drv.c \
	$(VSP_PATH)/src/osal_log.c 

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/$(H264_PATH)/include \
	$(LOCAL_PATH)/$(VSP_PATH)/inc
	
LOCAL_CFLAGS :=  -fno-strict-aliasing -DH264_DEC -D_VSP_LINUX_  -D_VSP_  -DCHIP_ENDIAN_LITTLE  -DCHIP_8830 
LOCAL_ARM_MODE := arm

LOCAL_SHARED_LIBRARIES := \
	libutils liblog libcutils

LOCAL_MODULE := libomx_avcdec_hw_sprd
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

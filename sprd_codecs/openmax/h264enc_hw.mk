LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sc8830)
H264_PATH := avc_h264_mix/enc/hw/sc8830/
VSP_PATH := vsp/sc8830/
endif

LOCAL_SRC_FILES := \
	$(H264_PATH)/src/h264enc_context_init.c \
	$(H264_PATH)/src/h264enc_ctx_table.c \
	$(H264_PATH)/src/h264enc_bitstrm.c \
	$(H264_PATH)/src/h264enc_frame.c \
	$(H264_PATH)/src/h264enc_global.c \
	$(H264_PATH)/src/h264enc_interface.c \
	$(H264_PATH)/src/h264enc_malloc.c \
	$(H264_PATH)/src/h264enc_rc.c \
	$(H264_PATH)/src/h264enc_init.c \
	$(H264_PATH)/src/h264enc_set.c \
	$(H264_PATH)/src/h264enc_slice.c \
	$(H264_PATH)/src/h264enc_table.c \
	$(VSP_PATH)/src/vsp_drv.c \
	$(VSP_PATH)/src/osal_log.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/$(H264_PATH)/include \
	$(LOCAL_PATH)/$(VSP_PATH)/inc

LOCAL_CFLAGS :=  -fno-strict-aliasing -DH264_ENC -D_VSP_LINUX_  -D_VSP_  -DCHIP_ENDIAN_LITTLE  -DCHIP_8830 
LOCAL_ARM_MODE := arm

LOCAL_SHARED_LIBRARIES := \
	libutils liblog

LOCAL_MODULE := libomx_avcenc_hw_sprd
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/h264enc_bitstrm.c \
	src/h264enc_frame.c \
	src/h264enc_global.c \
	src/h264enc_interface.c \
	src/h264enc_malloc.c \
	src/h264enc_rc.c \
	src/h264enc_reg.c \
	src/h264enc_set.c \
	src/h264enc_slice.c \
	src/h264enc_table.c \
	../../../../vsp/sc8830/src/vsp_drv_sc8830.c

LOCAL_MODULE := libomx_avcenc_hw_sprd
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS :=  -fno-strict-aliasing -DH264_ENC -D_VSP_LINUX_  -D_VSP_  -DCHIP_ENDIAN_LITTLE  -DCHIP_8830 
LOCAL_ARM_MODE := arm

LOCAL_SHARED_LIBRARIES := \
	libutils

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(TOP)/vendor/sprd/proprietories-source/sprd_codecs/openmax/vsp/sc8830/inc \

include $(BUILD_SHARED_LIBRARY)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/common/h264dec_biaridecod.c \
	src/common/h264dec_bitstream.c \
	src/common/h264dec_buffer.c \
	src/common/h264dec_cabac.c \
	src/common/h264dec_context_init.c \
	src/common/h264dec_ctx_table.c \
	src/common/h264dec_fmo.c \
	src/common/h264dec_global.c \
	src/common/h264dec_header.c \
	src/common/h264dec_image.c \
	src/common/h264dec_init.c \
	src/common/h264dec_interface.c \
	src/common/h264dec_malloc.c \
	src/common/h264dec_mb.c \
	src/common/h264dec_mv.c \
	src/common/h264dec_parset.c \
	src/common/h264dec_slice.c \
	src/common/h264dec_table.c \
	src/common/h264dec_vld.c	\
	src/hw/h264dec_command.c \
	src/hw/h264dec_deblock.c \
	src/hw/h264dec_mb_hw.c \
	src/hw/h264dec_mv_hw.c \
	src/hw/h264dec_slice_hw.c \
	src/hw/h264dec_vld_hw.c \
	src/common/h264dec_bitstream_neon.s \
	src/common/h264dec_mem_neon.s	\
	src/common/h264dec_vld_table.c 	\
	\
	../../../../vsp/sc8810/src/vsp_drv_sc8810.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/src/common \
	$(LOCAL_PATH)/src/hw	\
	$(TOP)/vendor/sprd/proprietories-source/sprd_codecs/openmax/vsp/sc8810/inc \

LOCAL_CFLAGS :=  -fno-strict-aliasing -D_VSP_LINUX_  -D_VSP_  -DCHIP_ENDIAN_LITTLE  -DCHIP_8810  -DH264_DEC 
#LOCAL_CFLAGS += -DYUV_THREE_PLANE
LOCAL_ARM_MODE := arm

LOCAL_SHARED_LIBRARIES := \
	libutils

LOCAL_MODULE := libomx_avcdec_hw_sprd
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

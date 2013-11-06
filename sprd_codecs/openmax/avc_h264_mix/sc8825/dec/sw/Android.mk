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
	src/sw/h264dec_ext_frame_neon.s	\
	src/sw/h264dec_ipred.c \
	src/sw/h264dec_ipred_neon.s \
	src/sw/h264dec_isqt.c \
	src/sw/h264dec_isqt_neon.s \
	src/sw/h264dec_mb_sw.c \
	src/sw/h264dec_mc.c \
	src/sw/h264dec_mc4_neon.s	\
	src/sw/h264dec_mc8_neon.s	\
	src/sw/h264dec_mc16_neon.s \
	src/sw/h264dec_mv_sw.c \
	src/sw/h264dec_slice_sw.c \
	src/sw/h264dec_vld_sw.c    \
	src/common/h264dec_bitstream_neon.s \
	src/common/h264dec_mem_neon.s	\
	src/common/h264dec_vld_table.c \
	src/sw/h264dec_wp_neon.s	\

LOCAL_MODULE := libomx_avcdec_sw_sprd
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS :=  -fno-strict-aliasing -D_VSP_LINUX_  -D_VSP_  -DCHIP_ENDIAN_LITTLE -DITRANS_ASSEMBLY -DCHIP_8825 
#LOCAL_CFLAGS += -DYUV_THREE_PLANE
LOCAL_ARM_MODE := arm

LOCAL_SHARED_LIBRARIES := \
	libutils liblog


LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/src/common \
	$(LOCAL_PATH)/src/sw 

include $(BUILD_SHARED_LIBRARY)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

H264_PATH := avc_h264_mix/dec/sw/
VSP_PATH := vsp/sc8830/

LOCAL_SRC_FILES := \
	$(H264_PATH)/src/h264dec_biaridecod.c \
	$(H264_PATH)/src/h264dec_bitstream.c \
	$(H264_PATH)/src/h264dec_buffer.c \
	$(H264_PATH)/src/h264dec_cabac.c \
	$(H264_PATH)/src/h264dec_context_init.c \
	$(H264_PATH)/src/h264dec_ctx_table.c \
	$(H264_PATH)/src/h264dec_deblock.c \
	$(H264_PATH)/src/h264dec_fmo.c \
	$(H264_PATH)/src/h264dec_global.c \
	$(H264_PATH)/src/h264dec_header.c \
	$(H264_PATH)/src/h264dec_image.c \
	$(H264_PATH)/src/h264dec_init.c \
	$(H264_PATH)/src/h264dec_interface.c \
	$(H264_PATH)/src/h264dec_ipred.c \
	$(H264_PATH)/src/h264dec_isqt.c \
	$(H264_PATH)/src/h264dec_malloc.c \
	$(H264_PATH)/src/h264dec_mb.c \
	$(H264_PATH)/src/h264dec_mc.c \
	$(H264_PATH)/src/h264dec_mv.c \
	$(H264_PATH)/src/h264dec_parset.c \
	$(H264_PATH)/src/h264dec_slice.c \
	$(H264_PATH)/src/h264dec_table.c \
	$(H264_PATH)/src/h264dec_vld.c	\
	$(H264_PATH)/src/h264dec_vld_table.c \
	$(H264_PATH)/src/h264dec_mc4xN.c \
	$(H264_PATH)/src/h264dec_mc8xN.c	\
	$(H264_PATH)/src/h264dec_mc16xN.c \
	$(VSP_PATH)/src/osal_log.c


#	$(H264_PATH)/src/gcc/h264dec_deblock_neon.s	\
#	$(H264_PATH)/src/gcc/h264dec_ext_frame_neon.s	\
#	$(H264_PATH)/src/gcc/h264dec_ipred_neon.s \
#	$(H264_PATH)/src/gcc/h264dec_isqt_neon.s \
#	$(H264_PATH)/src/gcc/h264dec_mc4_neon.s	\
#	$(H264_PATH)/src/gcc/h264dec_mc8_neon.s	\
#	$(H264_PATH)/src/gcc/h264dec_mc16_neon.s \
#	$(H264_PATH)/src/gcc/h264dec_mem_neon.s	\
#	$(H264_PATH)/src/gcc/h264dec_bitstream_neon.s \
#	$(H264_PATH)/src/gcc/h264dec_wp_neon.s

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/$(H264_PATH)/include \
	$(LOCAL_PATH)/$(VSP_PATH)/inc
	
LOCAL_CFLAGS :=  -fno-strict-aliasing -D_VSP_LINUX_ -DCHIP_ENDIAN_LITTLE -DITRANS_ASSEMBLY -DCHIP_ENDIAN_LITTLE
#LOCAL_LDFLAGS := -Wl,--no-warn-shared-textrel
LOCAL_ARM_MODE := arm

LOCAL_SHARED_LIBRARIES := \
        libutils liblog

LOCAL_MODULE := libomx_avcdec_sw_sprd
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

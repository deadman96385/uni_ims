LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/h264dec_biaridecod.c \
	src/h264dec_bitstream.c \
	src/h264dec_buffer.c \
	src/h264dec_cabac.c \
	src/h264dec_context_init.c \
	src/h264dec_ctx_table.c \
	src/h264dec_deblock.c \
	src/h264dec_fmo.c \
	src/h264dec_global.c \
	src/h264dec_header.c \
	src/h264dec_image.c \
	src/h264dec_init.c \
	src/h264dec_interface.c \
	src/h264dec_ipred.c \
	src/h264dec_isqt.c \
	src/h264dec_malloc.c \
	src/h264dec_mb.c \
	src/h264dec_mc.c \
	src/h264dec_mv.c \
	src/h264dec_parset.c \
	src/h264dec_slice.c \
	src/h264dec_table.c \
	src/h264dec_vld.c	\
	src/h264dec_vld_table.c \
	src/h264dec_mc4xN.c \
	src/h264dec_mc8xN.c	\
	src/h264dec_mc16xN.c \


#	src/gcc/h264dec_deblock_neon.s	\
#	src/gcc/h264dec_ext_frame_neon.s	\
#	src/gcc/h264dec_ipred_neon.s \
#	src/gcc/h264dec_isqt_neon.s \
#	src/gcc/h264dec_mc4_neon.s	\
#	src/gcc/h264dec_mc8_neon.s	\
#	src/gcc/h264dec_mc16_neon.s \
#	src/gcc/h264dec_mem_neon.s	\
#	src/gcc/h264dec_bitstream_neon.s \
#	src/gcc/h264dec_wp_neon.s	\

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/src \

LOCAL_CFLAGS :=  -fno-strict-aliasing -D_VSP_LINUX_ -DCHIP_ENDIAN_LITTLE -DITRANS_ASSEMBLY
LOCAL_LDFLAGS := -Wl,--no-warn-shared-textrel
LOCAL_ARM_MODE := arm

LOCAL_SHARED_LIBRARIES := \
        libutils liblog

LOCAL_MODULE := libomx_avcdec_sw_sprd
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

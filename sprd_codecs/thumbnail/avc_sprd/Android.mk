LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	SoftSPRDAVC.cpp \
	\
	src/h264dec_biaridecod.c \
	src/h264dec_bitstream.c \
	src/h264dec_buffer.c \
	src/h264dec_cabac.c \
	src/h264dec_context_init.c \
	src/h264dec_ctx_table.c \
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
	src/gcc/h264dec_ext_frame_neon.s	\
	src/gcc/h264dec_ipred_neon.s \
	src/gcc/h264dec_isqt_neon.s \
	src/gcc/h264dec_mc4_neon.s	\
	src/gcc/h264dec_mc8_neon.s	\
	src/gcc/h264dec_mc16_neon.s \
	src/gcc/h264dec_mem_neon.s	\
	src/gcc/h264dec_bitstream_neon.s \
	src/gcc/h264dec_wp_neon.s	\

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/src/common \
	$(LOCAL_PATH)/src/sw \
	\
        frameworks/av/media/libstagefright/include \
        frameworks/native/include/media/openmax

LOCAL_CFLAGS := -DOSCL_EXPORT_REF= -DOSCL_IMPORT_REF=
LOCAL_CFLAGS +=  -fno-strict-aliasing -D_VSP_LINUX_  -D_VSP_  -DCHIP_ENDIAN_LITTLE -DITRANS_ASSEMBLY 
LOCAL_ARM_MODE := arm

LOCAL_SHARED_LIBRARIES := \
        libstagefright libstagefright_omx libstagefright_foundation libutils

LOCAL_MODULE := libstagefright_sprd_soft_h264dec
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)


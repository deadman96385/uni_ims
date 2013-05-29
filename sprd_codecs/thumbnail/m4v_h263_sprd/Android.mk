LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
        SoftSPRDMPEG4.cpp	\
	\
	src/common/mp4_common_func.c \
	src/common/mp4_common_table.c \
	\
	src/common/mp4dec_bfrctrl.c \
	src/common/mp4dec_bitstream.c \
	src/common/mp4dec_datapartitioning.c	\
	src/common/mp4dec_global.c \
	src/common/mp4dec_header.c \
	src/common/mp4dec_interface.c \
	src/common/mp4dec_malloc.c \
	src/common/mp4dec_mb.c \
	src/common/mp4dec_mv.c \
	src/common/mp4dec_rvld.c	\
	src/common/mp4dec_session.c \
	src/common/mp4dec_table.c \
	src/common/mp4dec_vld.c \
	src/common/mp4dec_vop.c \
	\
	src/sw/mp4dec_block_sw.c \
	src/sw/mp4dec_FixPointDCT.c \
	src/sw/mp4dec_idct_neon.s \
	src/sw/mp4dec_mb_sw.c \
	src/sw/mp4dec_mc_neon.s \
	src/sw/mp4dec_mc_sw.c \
	src/sw/mp4dec_mv_sw.c \
	src/sw/mp4dec_vld_neon.s \
	src/sw/mp4dec_vld_sw.c \
	src/sw/mp4dec_vop_sw.c \
	src/sw/mp4dec_datapartitioning_vt.c \
	src/sw/mp4dec_error_handle_vt.c \
	src/sw/mp4dec_vop_vt.c \

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/src/common \
	$(LOCAL_PATH)/src/sw \
	\
        frameworks/av/media/libstagefright/include \
        frameworks/native/include/media/openmax

LOCAL_CFLAGS := -DOSCL_EXPORT_REF= -DOSCL_IMPORT_REF=
LOCAL_CFLAGS +=  -fno-strict-aliasing -D_VSP_LINUX_  -D_VSP_  -D_MP4CODEC_DATA_PARTITION_ -DCHIP_ENDIAN_LITTLE  -DCHIP_8810 

LOCAL_ARM_MODE := arm

LOCAL_SHARED_LIBRARIES := \
        libstagefright libstagefright_omx libstagefright_foundation libutils


LOCAL_MODULE := libstagefright_sprd_soft_mpeg4dec
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)


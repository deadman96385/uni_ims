LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
        SoftSPRDMPEG4.cpp	\
	\
	src/mp4dec_bfrctrl.c \
	src/mp4dec_bitstream.c \
	src/mp4dec_block.c \
	src/mp4dec_datapartitioning.c	\
	src/mp4dec_FixPointDCT.c \
	src/mp4dec_global.c \
	src/mp4dec_header.c \
	src/mp4dec_idct_neon.s \
	src/mp4dec_interface.c \
	src/mp4dec_malloc.c \
	src/mp4dec_mb.c \
	src/mp4dec_mc.c \
	src/mp4dec_mc_neon.s \
	src/mp4dec_mv.c \
	src/mp4dec_rvld.c	\
	src/mp4dec_session.c \
	src/mp4dec_table.c \
	src/mp4dec_vld.c \
	src/mp4dec_vld_neon.s \
	src/mp4dec_vop.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/src \
	\
        frameworks/av/media/libstagefright/include \
        frameworks/native/include/media/openmax

LOCAL_CFLAGS := -DOSCL_EXPORT_REF= -DOSCL_IMPORT_REF=
LOCAL_CFLAGS +=  -fno-strict-aliasing -D_VSP_LINUX_  -D_VSP_  -D_MP4CODEC_DATA_PARTITION_ -DCHIP_ENDIAN_LITTLE

LOCAL_ARM_MODE := arm

LOCAL_SHARED_LIBRARIES := \
        libstagefright libstagefright_omx libstagefright_foundation libutils


LOCAL_MODULE := libstagefright_sprd_soft_mpeg4dec
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)


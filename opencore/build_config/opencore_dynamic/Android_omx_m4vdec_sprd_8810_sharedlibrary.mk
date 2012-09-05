LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	../../codecs_v2/omx/omx_m4v_sprd_8800g/src/omx_mpeg4_component.cpp \
 	../../codecs_v2/omx/omx_m4v_sprd_8800g/src/mpeg4_dec.cpp \
	\
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/common/mp4_common_func.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/common/mp4_common_table.c \
	\
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder/common/mp4dec_bfrctrl.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder/common/mp4dec_bitstream.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder/common/mp4dec_global.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder/common/mp4dec_header.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder/common/mp4dec_interface.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder/common/mp4dec_malloc.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder/common/mp4dec_mb.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder/common/mp4dec_mv.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder/common/mp4dec_session.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder/common/mp4dec_table.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder/common/mp4dec_vld.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder/common/mp4dec_vop.c \
	\
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder/hw/mp4dec_block_hw.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder/hw/mp4dec_command.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder/hw/mp4dec_datapartitioning.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder/hw/mp4dec_error_handle.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder/hw/mp4dec_mb_hw.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder/hw/mp4dec_mc_hw.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder/hw/mp4dec_mv_hw.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder/hw/mp4dec_rvld.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder/hw/mp4dec_vld_hw.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder/hw/mp4dec_vop_hw.c \
	\
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder/sw/mp4dec_block_sw.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder/sw/mp4dec_FixPointDCT.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder/sw/mp4dec_idct_neon.s \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder/sw/mp4dec_mb_sw.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder/sw/mp4dec_mc_neon.s \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder/sw/mp4dec_mc_sw.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder/sw/mp4dec_mv_sw.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder/sw/mp4dec_vld_neon.s \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder/sw/mp4dec_vld_sw.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder/sw/mp4dec_vop_sw.c \
	\
	../../codecs_v2/video/video_sprd/vsp/sc8810/src/vsp_drv_sc8810.c


#LOCAL_MODULE := libomx_m4vdec_sprd_sharedlibrary
LOCAL_MODULE := libomx_m4vdec_sharedlibrary
LOCAL_MODULE_TAGS := optional

LOCAL_PRELINK_MODULE := false

LOCAL_CFLAGS :=  -fno-strict-aliasing -D_VSP_LINUX_  -D_VSP_  -D_MP4CODEC_DATA_PARTITION_ -DCHIP_ENDIAN_LITTLE  -DCHIP_8810 $(PV_CFLAGS)
#LOCAL_CFLAGS += -DYUV_THREE_PLANE
LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES :=

LOCAL_SHARED_LIBRARIES +=   libomx_sharedlibrary libopencore_common libutils libbinder libui

LOCAL_C_INCLUDES := \
	$(PV_TOP)/codecs_v2/omx/omx_m4v_sprd_8800g/src \
 	$(PV_TOP)/codecs_v2/omx/omx_m4v_sprd_8800g/include \
 	$(PV_TOP)/extern_libs_v2/khronos/openmax/include \
 	$(PV_TOP)/codecs_v2/omx/omx_baseclass/include \
	\
 	$(PV_TOP)/codecs_v2/video/video_sprd/vsp/sc8810/src \
 	$(PV_TOP)/codecs_v2/video/video_sprd/vsp/sc8810/inc \
	\
 	$(PV_TOP)/codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/export_inc \
 	$(PV_TOP)/codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/inc/common \
 	$(PV_TOP)/codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/common \
 	$(PV_TOP)/codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/inc/decoder \
 	$(PV_TOP)/codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/src/decoder \
	$(PV_TOP)/codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8810/inc/encoder \
 	$(PV_INCLUDES) \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video \
	$(TOP)/device/sprd/common/libs/gralloc \
	$(TOP)/device/sprd/common/libs/mali/src/ump/include \
	$(TOP)/frameworks/native/include/media/hardware

include $(BUILD_SHARED_LIBRARY)


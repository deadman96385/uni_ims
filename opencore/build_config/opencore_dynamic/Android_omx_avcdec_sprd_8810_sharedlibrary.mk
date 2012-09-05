LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	../../codecs_v2/video/video_sprd/vsp/sc8810/src/vsp_drv_sc8810.c \
	../../codecs_v2/omx/omx_h264_sprd_8800g/src/omx_avc_component.cpp \
 	../../codecs_v2/omx/omx_h264_sprd_8800g/src/avc_dec.cpp \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/common/h264dec_biaridecod.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/common/h264dec_bitstream.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/common/h264dec_buffer.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/common/h264dec_cabac.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/common/h264dec_context_init.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/common/h264dec_ctx_table.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/common/h264dec_fmo.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/common/h264dec_global.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/common/h264dec_header.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/common/h264dec_image.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/common/h264dec_init.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/common/h264dec_interface.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/common/h264dec_malloc.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/common/h264dec_mb.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/common/h264dec_mv.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/common/h264dec_parset.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/common/h264dec_slice.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/common/h264dec_table.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/common/h264dec_vld.c	\
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/hw/h264dec_command.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/hw/h264dec_deblock.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/hw/h264dec_mb_hw.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/hw/h264dec_mv_hw.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/hw/h264dec_slice_hw.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/hw/h264dec_vld_hw.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/sw/h264dec_ext_frame_neon.s	\
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/sw/h264dec_ipred.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/sw/h264dec_ipred_neon.s \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/sw/h264dec_isqt.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/sw/h264dec_isqt_neon.s \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/sw/h264dec_mb_sw.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/sw/h264dec_mc.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/sw/h264dec_mc4_neon.s	\
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/sw/h264dec_mc8_neon.s	\
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/sw/h264dec_mc16_neon.s \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/sw/h264dec_mem_neon.s	\
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/sw/h264dec_mv_sw.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/sw/h264dec_slice_sw.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/sw/h264dec_vld_sw.c    \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/common/h264dec_bitstream_neon.s \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/common/h264dec_vld_table.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/src/decoder/sw/h264dec_wp_neon.s

#LOCAL_MODULE := libomx_avcdec_sprd_sharedlibrary
LOCAL_MODULE := libomx_avcdec_sharedlibrary
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS :=  -fno-strict-aliasing -D_VSP_LINUX_  -D_VSP_  -DCHIP_ENDIAN_LITTLE -DITRANS_ASSEMBLY -DCHIP_8810 $(PV_CFLAGS)

#LOCAL_CFLAGS += -DYUV_THREE_PLANE

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES :=

LOCAL_SHARED_LIBRARIES +=   libomx_sharedlibrary libopencore_common libutils libbinder  libui

LOCAL_C_INCLUDES := \
	$(PV_TOP)/codecs_v2/omx/omx_h264_sprd_8800g/src \
 	$(PV_TOP)/codecs_v2/omx/omx_h264_sprd_8800g/include \
 	$(PV_TOP)/extern_libs_v2/khronos/openmax/include \
 	$(PV_TOP)/codecs_v2/omx/omx_baseclass/include \
 	$(PV_TOP)/codecs_v2/video/video_sprd/vsp/sc8810/src \
 	$(PV_TOP)/codecs_v2/video/video_sprd/vsp/sc8810/inc \
 	$(PV_TOP)/codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/inc/common \
 	$(PV_TOP)/codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8810/inc/decoder \
 	$(PV_INCLUDES) \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video \
	$(TOP)/device/sprd/common/libs/gralloc \
	$(TOP)/device/sprd/common/libs/mali/src/ump/include \
	$(TOP)/frameworks/native/include/media/hardware

include $(BUILD_SHARED_LIBRARY)

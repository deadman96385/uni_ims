LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	../../codecs_v2/omx/omx_h264_sprd_8800g/src/omx_avc_component.cpp \
 	../../codecs_v2/omx/omx_h264_sprd_8800g/src/avc_dec.cpp \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8800g/src/decoder/h264dec_bitstream.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8800g/src/decoder/h264dec_buffer.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8800g/src/decoder/h264dec_command.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8800g/src/decoder/h264dec_deblock.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8800g/src/decoder/h264dec_fmo.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8800g/src/decoder/h264dec_global.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8800g/src/decoder/h264dec_header.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8800g/src/decoder/h264dec_image.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8800g/src/decoder/h264dec_init.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8800g/src/decoder/h264dec_interface.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8800g/src/decoder/h264dec_malloc.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8800g/src/decoder/h264dec_mb.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8800g/src/decoder/h264dec_mv.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8800g/src/decoder/h264dec_parset.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8800g/src/decoder/h264dec_slice.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8800g/src/decoder/h264dec_table.c \
	../../codecs_v2/video/video_sprd/vsp/sc8800g/src/vsp_drv_sc8800g.c

#LOCAL_MODULE := libomx_avcdec_sprd_sharedlibrary
LOCAL_MODULE := libomx_avcdec_sharedlibrary
LOCAL_MODULE_TAGS := optional


LOCAL_CFLAGS :=  -fno-strict-aliasing -D_VSP_LINUX_  -D_VSP_  -DCHIP_ENDIAN_LITTLE $(PV_CFLAGS)
LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES :=

LOCAL_SHARED_LIBRARIES +=   libomx_sharedlibrary libopencore_common libutils libbinder

LOCAL_C_INCLUDES := \
	$(PV_TOP)/codecs_v2/omx/omx_h264_sprd_8800g/src \
 	$(PV_TOP)/codecs_v2/omx/omx_h264_sprd_8800g/include \
 	$(PV_TOP)/extern_libs_v2/khronos/openmax/include \
 	$(PV_TOP)/codecs_v2/omx/omx_baseclass/include \
 	$(PV_TOP)/codecs_v2/video/video_sprd/vsp/sc8800g/src \
 	$(PV_TOP)/codecs_v2/video/video_sprd/vsp/sc8800g/inc \
 	$(PV_TOP)/codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8800g/inc/common \
 	$(PV_TOP)/codecs_v2/video/video_sprd/h264_codec/h264_codec_sc8800g/inc/decoder \
 	$(PV_INCLUDES)

include $(BUILD_SHARED_LIBRARY)

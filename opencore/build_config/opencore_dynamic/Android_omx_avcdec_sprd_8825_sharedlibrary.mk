LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	../../codecs_v2/video/video_sprd/vsp/tiger/src/vsp_drv_tiger.c \
	../../codecs_v2/omx/omx_h264_sprd_tiger/src/omx_avc_component.cpp \
 	../../codecs_v2/omx/omx_h264_sprd_tiger/src/avc_dec.cpp \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_tiger/src/decoder/h264dec_bitstream.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_tiger/src/decoder/h264dec_buffer.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_tiger/src/decoder/h264dec_command.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_tiger/src/decoder/h264dec_deblock.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_tiger/src/decoder/h264dec_fmo.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_tiger/src/decoder/h264dec_global.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_tiger/src/decoder/h264dec_header.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_tiger/src/decoder/h264dec_image.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_tiger/src/decoder/h264dec_init.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_tiger/src/decoder/h264dec_interface.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_tiger/src/decoder/h264dec_malloc.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_tiger/src/decoder/h264dec_mb.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_tiger/src/decoder/h264dec_mv.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_tiger/src/decoder/h264dec_parset.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_tiger/src/decoder/h264dec_slice.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_tiger/src/decoder/h264dec_table.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_tiger/src/decoder/h264dec_biaridecod.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_tiger/src/decoder/h264dec_cabac.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_tiger/src/decoder/h264dec_context_init.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_tiger/src/decoder/h264dec_ctx_table.c \
	../../codecs_v2/video/video_sprd/h264_codec/h264_codec_tiger/src/decoder/h264dec_vld.c

#LOCAL_MODULE := libomx_avcdec_sprd_sharedlibrary
LOCAL_MODULE := libomx_avcdec_sharedlibrary
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS :=  -fno-strict-aliasing -DH264_DEC -D_VSP_LINUX_  -D_VSP_  -DCHIP_ENDIAN_LITTLE -DCHIP_TIGER $(PV_CFLAGS)
LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES :=

LOCAL_SHARED_LIBRARIES +=   libomx_sharedlibrary libopencore_common libutils libbinder  libui

LOCAL_C_INCLUDES := \
	$(PV_TOP)/codecs_v2/omx/omx_h264_sprd_tiger/src \
 	$(PV_TOP)/codecs_v2/omx/omx_h264_sprd_tiger/include \
 	$(PV_TOP)/extern_libs_v2/khronos/openmax/include \
 	$(PV_TOP)/codecs_v2/omx/omx_baseclass/include \
 	$(PV_TOP)/codecs_v2/video/video_sprd/vsp/tiger/src \
 	$(PV_TOP)/codecs_v2/video/video_sprd/vsp/tiger/inc \
 	$(PV_TOP)/codecs_v2/video/video_sprd/h264_codec/h264_codec_tiger/inc/common \
 	$(PV_TOP)/codecs_v2/video/video_sprd/h264_codec/h264_codec_tiger/inc/decoder \
 	$(PV_INCLUDES) \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video \
	$(TOP)/device/sprd/common/libs/gralloc \
	$(TOP)/device/sprd/common/libs/mali/src/ump/include \
	$(TOP)/frameworks/native/include/media/hardware


include $(BUILD_SHARED_LIBRARY)

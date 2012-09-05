LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)


LOCAL_SRC_FILES := \
	../../codecs_v2/omx/omx_m4venc_sprd_8800g/src/omx_mpeg4enc_component.cpp \
 	../../codecs_v2/omx/omx_m4venc_sprd_8800g/src/mpeg4_enc.cpp \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8800g/src/encoder/mp4enc_bfrctrl.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8800g/src/encoder/mp4enc_bitstrm.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8800g/src/encoder/mp4enc_command.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8800g/src/encoder/mp4enc_global.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8800g/src/encoder/mp4enc_header.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8800g/src/encoder/mp4enc_init.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8800g/src/encoder/mp4enc_interface.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8800g/src/encoder/mp4enc_malloc.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8800g/src/encoder/mp4enc_mb.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8800g/src/encoder/mp4enc_me.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8800g/src/encoder/mp4enc_mv.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8800g/src/encoder/mp4enc_ratecontrol.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8800g/src/encoder/mp4enc_table.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8800g/src/encoder/mp4enc_trace.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8800g/src/encoder/mp4enc_vlc.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8800g/src/encoder/mp4enc_vop.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8800g/src/common/mp4_common_func.c \
	../../codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8800g/src/common/mp4_common_table.c \
	../../codecs_v2/video/video_sprd/vsp/sc8800g/src/vsp_drv_sc8800g.c

#LOCAL_MODULE := libomx_m4venc_sprd_sharedlibrary
LOCAL_MODULE := libomx_m4venc_sharedlibrary
LOCAL_MODULE_TAGS := optional


LOCAL_PRELINK_MODULE := false

LOCAL_CFLAGS :=  -fno-strict-aliasing -D_VSP_LINUX_  -D_VSP_  -DCHIP_ENDIAN_LITTLE $(PV_CFLAGS)
LOCAL_ARM_MODE := arm

#-include $(PV_TOP)/Android_system_extras.mk

LOCAL_STATIC_LIBRARIES :=
LOCAL_SHARED_LIBRARIES +=   libomx_sharedlibrary libopencore_common libutils libbinder

LOCAL_C_INCLUDES := \
	$(PV_TOP)/codecs_v2/omx/omx_m4venc_sprd_8800g/src \
 	$(PV_TOP)/codecs_v2/omx/omx_m4venc_sprd_8800g/include \
 	$(PV_TOP)/extern_libs_v2/khronos/openmax/include \
 	$(PV_TOP)/codecs_v2/omx/omx_baseclass/include \
 	$(PV_TOP)/codecs_v2/video/video_sprd/vsp/sc8800g/src \
 	$(PV_TOP)/codecs_v2/video/video_sprd/vsp/sc8800g/inc \
 	$(PV_TOP)/codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8800g/export_inc \
 	$(PV_TOP)/codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8800g/inc/common \
 	$(PV_TOP)/codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8800g/src/common \
 	$(PV_TOP)/codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8800g/inc/encoder \
 	$(PV_TOP)/codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8800g/src/encoder \
 	$(PV_TOP)/codecs_v2/video/video_sprd/mpeg4_codec/mpeg4_codec_sc8800g/inc/decoder \
	$(PV_INCLUDES)

include $(BUILD_SHARED_LIBRARY)


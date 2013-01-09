LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/common/h264dec_biaridecod.c \
	src/common/h264dec_bitstream.c \
	src/common/h264dec_buffer.c \
	src/common/h264dec_cabac.c \
	src/common/h264dec_context_init.c \
	src/common/h264dec_ctx_table.c \
	src/common/h264dec_fmo.c \
	src/common/h264dec_global.c \
	src/common/h264dec_header.c \
	src/common/h264dec_image.c \
	src/common/h264dec_init.c \
	src/common/h264dec_interface.c \
	src/common/h264dec_malloc.c \
	src/common/h264dec_mb.c \
	src/common/h264dec_mv.c \
	src/common/h264dec_parset.c \
	src/common/h264dec_slice.c \
	src/common/h264dec_table.c \
	src/common/h264dec_vld.c	\
	src/hw/h264dec_command.c \
	src/hw/h264dec_deblock.c \
	src/hw/h264dec_mb_hw.c \
	src/hw/h264dec_mv_hw.c \
	src/hw/h264dec_slice_hw.c \
	src/hw/h264dec_vld_hw.c \
	src/sw/h264dec_ext_frame_neon.s	\
	src/sw/h264dec_ipred.c \
	src/sw/h264dec_ipred_neon.s \
	src/sw/h264dec_isqt.c \
	src/sw/h264dec_isqt_neon.s \
	src/sw/h264dec_mb_sw.c \
	src/sw/h264dec_mc.c \
	src/sw/h264dec_mc4_neon.s	\
	src/sw/h264dec_mc8_neon.s	\
	src/sw/h264dec_mc16_neon.s \
	src/sw/h264dec_mem_neon.s	\
	src/sw/h264dec_mv_sw.c \
	src/sw/h264dec_slice_sw.c \
	src/sw/h264dec_vld_sw.c    \
	src/common/h264dec_bitstream_neon.s \
	src/common/h264dec_vld_table.c \
	src/sw/h264dec_wp_neon.s	\
	\
	../../../vsp/sc8825/src/vsp_drv_sc8825.c

LOCAL_MODULE := libsprdavcdecoder
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS :=  -fno-strict-aliasing -D_VSP_LINUX_  -D_VSP_  -DCHIP_ENDIAN_LITTLE -DITRANS_ASSEMBLY -DCHIP_8825 -DH264_DEC $(PV_CFLAGS)
#LOCAL_CFLAGS += -DYUV_THREE_PLANE
ifeq ($(strip $(CODEC_ALLOC_FROM_PMEM)),false)
LOCAL_CFLAGS += -DH264CODEC_NO_PMEM
endif
LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES := 

LOCAL_SHARED_LIBRARIES := 

LOCAL_C_INCLUDES := \
	$(PV_TOP)/codecs_v2/video/avc_h264_sprd/sc8825/dec/include \
	$(PV_TOP)/codecs_v2/video/avc_h264_sprd/sc8825/src/common \
	$(PV_TOP)/codecs_v2/video/avc_h264_sprd/sc8825/src/hw \
	$(PV_TOP)/codecs_v2/video/avc_h264_sprd/sc8825/src/sw \
	$(PV_TOP)/codecs_v2/video/vsp/sc8825/src \
	$(PV_TOP)/codecs_v2/video/vsp/sc8825/inc \
 	$(PV_INCLUDES)

LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)

include $(BUILD_STATIC_LIBRARY)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/vp8_alloc_common.c \
	src/vp8_coef_update_probs.c \
	src/vp8dec_dboolhuff.c \
	src/vp8dec_demode.c \
	src/vp8dec_dequant.c \
	src/vp8dec_frame.c \
	src/vp8dec_global.c \
	src/vp8dec_interface.c \
	src/vp8dec_malloc.c \
	src/vp8dec_mv.c \
	src/vp8dec_reg.c \
	src/vp8dec_treereader.c \
	src/vp8_default_coef_counts.c \
	src/vp8_entropy.c \
	src/vp8_entropy_mode.c \
	src/vp8_entropy_mv.c \
	src/vp8_init.c \
	src/vp8_mode_context.c \
	src/vp8_mode_count.c \
	src/vp8_quant_common.c \
	src/vp8_setup_intra_recon.c \
	src/vp8_swap_yv12buffer.c \
	src/vp8_treecoder.c \
	src/vp8_yv12config.c \
	../../../../vsp/sc8830/src/vsp_drv_sc8830.c


LOCAL_MODULE := libomx_vpxdec_hw_sprd
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS :=  -fno-strict-aliasing  -DVP8_DEC -D_VSP_LINUX_  -D_VSP_  -DCHIP_ENDIAN_LITTLE  -DCHIP_8830
#-DOR_CACHE_OFF

LOCAL_ARM_MODE := arm

LOCAL_SHARED_LIBRARIES := \
	libutils

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(TOP)/vendor/sprd/proprietories-source/sprd_codecs/openmax/vsp/sc8830/inc \

include $(BUILD_SHARED_LIBRARY)

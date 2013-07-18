LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/common/mp4_common_func.c \
	src/common/mp4_common_table.c \
	\
	src/common/mp4dec_bfrctrl.c \
	src/common/mp4dec_bitstream.c \
	src/common/mp4dec_global.c \
	src/common/mp4dec_header.c \
	src/common/mp4dec_interface.c \
	src/common/mp4dec_malloc.c \
	src/common/mp4dec_mb.c \
	src/common/mp4dec_mv.c \
	src/common/mp4dec_session.c \
	src/common/mp4dec_table.c \
	src/common/mp4dec_vld.c \
	src/common/mp4dec_vop.c \
	\
	src/hw/mp4dec_block_hw.c \
	src/hw/mp4dec_command.c \
	src/hw/mp4dec_datapartitioning.c \
	src/hw/mp4dec_error_handle.c \
	src/hw/mp4dec_mb_hw.c \
	src/hw/mp4dec_mc_hw.c \
	src/hw/mp4dec_mv_hw.c \
	src/hw/mp4dec_rvld.c \
	src/hw/mp4dec_vld_hw.c \
	src/hw/mp4dec_vop_hw.c \
	\
	../../../../vsp/sc8810/src/vsp_drv_sc8810.c


LOCAL_MODULE := libomx_m4vh263dec_hw_sprd
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS :=  -fno-strict-aliasing -DMPEG4_DEC -D_VSP_LINUX_  -D_VSP_  -D_MP4CODEC_DATA_PARTITION_ -DCHIP_ENDIAN_LITTLE  -DCHIP_8810 
#LOCAL_CFLAGS += -DYUV_THREE_PLANE
LOCAL_ARM_MODE := arm

LOCAL_SHARED_LIBRARIES := \
	libutils

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/src/common \
	$(LOCAL_PATH)/src/hw 	\
	$(TOP)/vendor/sprd/proprietories-source/sprd_codecs/openmax/vsp/sc8810/inc \

include $(BUILD_SHARED_LIBRARY)

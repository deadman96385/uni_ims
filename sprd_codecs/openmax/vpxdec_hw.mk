LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sc8830)
VPX_PATH := vpx_mix/dec/hw/sc8830/
VSP_PATH := vsp/sc8830/
endif

LOCAL_SRC_FILES := \
	$(VPX_PATH)/src/vp8dec_bfrctrl.c \
	$(VPX_PATH)/src/vp8dec_dboolhuff.c \
	$(VPX_PATH)/src/vp8dec_dequant.c \
	$(VPX_PATH)/src/vp8dec_frame.c \
	$(VPX_PATH)/src/vp8dec_global.c \
	$(VPX_PATH)/src/vp8dec_init.c \
	$(VPX_PATH)/src/vp8dec_interface.c \
	$(VPX_PATH)/src/vp8dec_malloc.c \
	$(VPX_PATH)/src/vp8dec_table.c	\
	$(VPX_PATH)/src/vp8dec_vld.c \
	$(VSP_PATH)/src/vsp_drv.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/$(VPX_PATH)/include	\
	$(LOCAL_PATH)/$(VSP_PATH)/inc

LOCAL_CFLAGS :=  -fno-strict-aliasing  -DVP8_DEC -D_VSP_LINUX_  -D_VSP_  -DCHIP_ENDIAN_LITTLE  -DCHIP_8830
LOCAL_ARM_MODE := arm

LOCAL_SHARED_LIBRARIES := \
	libutils liblog

LOCAL_MODULE := libomx_vpxdec_hw_sprd
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

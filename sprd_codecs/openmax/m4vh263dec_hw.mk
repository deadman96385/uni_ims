LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sc8830)
MP4_PATH := m4v_h263_mix/dec/hw/sc8830/
VSP_PATH := vsp/sc8830/
endif

LOCAL_SRC_FILES := \
	$(MP4_PATH)/src/mp4dec_bfrctrl.c \
	$(MP4_PATH)/src/mp4dec_bitstream.c \
	$(MP4_PATH)/src/mp4dec_global.c \
	$(MP4_PATH)/src/mp4dec_header.c \
	$(MP4_PATH)/src/mp4dec_interface.c \
	$(MP4_PATH)/src/mp4dec_malloc.c \
	$(MP4_PATH)/src/mp4dec_session.c \
	$(MP4_PATH)/src/mp4dec_table.c \
	$(MP4_PATH)/src/mp4dec_vop.c \
	$(VSP_PATH)/src/vsp_drv.c \
	$(VSP_PATH)/src/osal_log.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/$(MP4_PATH)/include \
	$(LOCAL_PATH)/$(VSP_PATH)/inc

LOCAL_CFLAGS :=  -fno-strict-aliasing -DMPEG4_DEC -D_VSP_LINUX_  -D_VSP_  -DCHIP_ENDIAN_LITTLE  -DCHIP_8830 
LOCAL_ARM_MODE := arm

LOCAL_SHARED_LIBRARIES := \
	libutils liblog libcutils

LOCAL_MODULE := libomx_m4vh263dec_hw_sprd
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

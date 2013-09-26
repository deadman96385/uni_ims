LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/mp4dec_bfrctrl.c \
	src/mp4dec_bitstream.c \
	src/mp4dec_global.c \
	src/mp4dec_header.c \
	src/mp4dec_interface.c \
	src/mp4dec_malloc.c \
	src/mp4dec_session.c \
	src/mp4dec_table.c \
	src/mp4dec_vop.c \
	../../../../vsp/sc8830/src/vsp_drv_sc8830.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(TOP)/vendor/sprd/proprietories-source/sprd_codecs/openmax/vsp/sc8830/inc \

LOCAL_CFLAGS :=  -fno-strict-aliasing -DMPEG4_DEC -D_VSP_LINUX_  -D_VSP_  -DCHIP_ENDIAN_LITTLE  -DCHIP_8830 

LOCAL_ARM_MODE := arm

LOCAL_SHARED_LIBRARIES := \
	libutils

LOCAL_MODULE := libomx_m4vh263dec_hw_sprd
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

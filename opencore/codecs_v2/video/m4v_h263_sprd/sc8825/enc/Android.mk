LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/mp4_common_func.c \
	src/mp4_common_table.c \
	src/mp4enc_bfrctrl.c \
	src/mp4enc_bitstrm.c \
	src/mp4enc_command.c \
	src/mp4enc_global.c \
	src/mp4enc_header.c \
	src/mp4enc_init.c \
	src/mp4enc_interface.c \
	src/mp4enc_malloc.c \
	src/mp4enc_mb.c \
	src/mp4enc_me.c \
	src/mp4enc_mv.c \
	src/mp4enc_ratecontrol.c \
	src/mp4enc_table.c \
	src/mp4enc_trace.c \
	src/mp4enc_vlc.c \
	src/mp4enc_vop.c \
	../../../vsp/sc8825/src/vsp_drv_sc8825_enc.c


LOCAL_MODULE := libsprdm4vencoder
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS :=  -fno-strict-aliasing -DMPEG4_ENC -D_VSP_LINUX_  -D_VSP_  -DCHIP_ENDIAN_LITTLE  -DCHIP_8825 $(PV_CFLAGS)
#LOCAL_CFLAGS := -fno-strict-aliasing -D_VSP_LINUX_  -D_VSP_  -DCHIP_ENDIAN_LITTLE  -DCHIP_8810  $(PV_CFLAGS)
LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES := 

LOCAL_SHARED_LIBRARIES := 

LOCAL_C_INCLUDES := \
	$(PV_TOP)/codecs_v2/video/m4v_h263_sprd/sc8825/enc/include \
	$(PV_TOP)/codecs_v2/video/vsp/sc8825/src \
	$(PV_TOP)/codecs_v2/video/vsp/sc8825/inc \
 	$(PV_INCLUDES)

LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)

include $(BUILD_STATIC_LIBRARY)

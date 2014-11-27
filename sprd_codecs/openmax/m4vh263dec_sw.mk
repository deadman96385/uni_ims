LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

MP4_PATH := m4v_h263_mix/dec/sw/

LOCAL_SRC_FILES := \
	$(MP4_PATH)/src/mp4dec_bfrctrl.c \
	$(MP4_PATH)/src/mp4dec_bitstream.c \
	$(MP4_PATH)/src/mp4dec_block.c \
	$(MP4_PATH)/src/mp4dec_datapartitioning.c	\
	$(MP4_PATH)/src/mp4dec_FixPointDCT.c \
	$(MP4_PATH)/src/mp4dec_global.c \
	$(MP4_PATH)/src/mp4dec_header.c \
	$(MP4_PATH)/src/mp4dec_interface.c \
	$(MP4_PATH)/src/mp4dec_malloc.c \
	$(MP4_PATH)/src/mp4dec_mb.c \
	$(MP4_PATH)/src/mp4dec_mc.c \
	$(MP4_PATH)/src/mp4dec_mv.c \
	$(MP4_PATH)/src/mp4dec_rvld.c	\
	$(MP4_PATH)/src/mp4dec_session.c \
	$(MP4_PATH)/src/mp4dec_table.c \
	$(MP4_PATH)/src/mp4dec_vld.c \
	$(MP4_PATH)/src/mp4dec_vop.c \
	$(MP4_PATH)/src/mp4dec_error_handle.c	\
	$(MP4_PATH)/src/mp4dec_deblock.c


#	$(MP4_PATH)/src/mp4dec_dbk_neon.s   \
#	$(MP4_PATH)/src/mp4dec_idct_neon.s \
#	$(MP4_PATH)/src/mp4dec_mc_neon.s \
#	$(MP4_PATH)/src/mp4dec_vld_neon.s

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/$(MP4_PATH)/include

LOCAL_CFLAGS :=  -fno-strict-aliasing -D_VSP_LINUX_   -D_MP4CODEC_DATA_PARTITION_ -DCHIP_ENDIAN_LITTLE
#LOCAL_LDFLAGS := -Wl,--no-warn-shared-textrel

LOCAL_ARM_MODE := arm

LOCAL_SHARED_LIBRARIES := \
	libutils liblog

LOCAL_MODULE := libomx_m4vh263dec_sw_sprd
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)


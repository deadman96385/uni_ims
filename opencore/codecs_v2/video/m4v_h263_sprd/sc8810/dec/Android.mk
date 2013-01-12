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
	src/sw/mp4dec_block_sw.c \
	src/sw/mp4dec_FixPointDCT.c \
	src/sw/mp4dec_idct_neon.s \
	src/sw/mp4dec_mb_sw.c \
	src/sw/mp4dec_mc_neon.s \
	src/sw/mp4dec_mc_sw.c \
	src/sw/mp4dec_mv_sw.c \
	src/sw/mp4dec_vld_neon.s \
	src/sw/mp4dec_vld_sw.c \
	src/sw/mp4dec_vop_sw.c \
	src/sw/mp4dec_datapartitioning_vt.c \
	src/sw/mp4dec_error_handle_vt.c \
	src/sw/mp4dec_vop_vt.c \
	src/sw/mp4dec_deblock.c \
	src/sw/mp4dec_dbk_neon.s \
	\
	../../../vsp/sc8810/src/vsp_drv_sc8810.c


LOCAL_MODULE := libsprdmp4decoder
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS :=  -fno-strict-aliasing -D_VSP_LINUX_  -D_VSP_  -D_MP4CODEC_DATA_PARTITION_ -DCHIP_ENDIAN_LITTLE  -DCHIP_8810 $(PV_CFLAGS)
#LOCAL_CFLAGS += -DYUV_THREE_PLANE
ifeq ($(strip $(CODEC_ALLOC_FROM_PMEM)),false)
LOCAL_CFLAGS += -DMP4CODEC_NO_PMEM
endif
LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES := 

LOCAL_SHARED_LIBRARIES := 

LOCAL_C_INCLUDES := \
	$(PV_TOP)/codecs_v2/video/m4v_h263_sprd/sc8810/dec/include \
	$(PV_TOP)/codecs_v2/video/m4v_h263_sprd/sc8810/src/common \
	$(PV_TOP)/codecs_v2/video/m4v_h263_sprd/sc8810/src/hw \
	$(PV_TOP)/codecs_v2/video/m4v_h263_sprd/sc8810/src/sw \
	$(PV_TOP)/codecs_v2/video/vsp/sc8810/src \
	$(PV_TOP)/codecs_v2/video/vsp/sc8810/inc \
 	$(PV_INCLUDES)

LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)

include $(BUILD_STATIC_LIBRARY)

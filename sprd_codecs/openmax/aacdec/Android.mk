LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	decode_src/aac_asm_filterbank_tab.c \
	decode_src/aac_bits.c \
	decode_src/aac_cfft_tab.c \
	decode_src/aac_common.c \
	decode_src/aac_decoder.c \
	decode_src/aac_huffman.c \
	decode_src/aac_ifiltbank.c \
	decode_src/aac_is.c \
	decode_src/aac_ltp.c \
	decode_src/aac_ms.c \
	decode_src/aac_pns.c \
	decode_src/AAC_ps_dec.c \
	decode_src/AAC_ps_init.c \
	decode_src/AAC_ps_mix_phase.c \
	decode_src/AAC_ps_syntax.c \
	decode_src/AAC_ps_table.c \
	decode_src/aac_pulse.c \
	decode_src/AAC_sbr_common.c \
	decode_src/AAC_sbr_dec.c \
	decode_src/AAC_sbr_e_nf.c \
	decode_src/AAC_sbr_fbt.c \
	decode_src/AAC_sbr_hfadj.c \
	decode_src/AAC_sbr_hfgen.c \
	decode_src/AAC_sbr_huff.c \
	decode_src/AAC_sbr_qmf.c \
	decode_src/AAC_sbr_syntax.c \
	decode_src/AAC_sbr_table.c \
	decode_src/AAC_sbr_tf_grid.c \
	decode_src/aac_specrec.c \
	decode_src/aac_syntax.c \
	decode_src/aac_tns.c \
	decode_src/AAC_arm_math.s \
	decode_src/AAC_filterbank_output.s \
	decode_src/AAC_imdct_asm.s \
	decode_src/aac_lc_iq_tns_asm.s \
	decode_src/aac_ltp_asm.s \
	decode_src/AAC_ps_parse_arm.s \
	decode_src/aac_sbr_analysis_filter.s \
	decode_src/aac_sbr_analysis_post.s \
	decode_src/aac_sbr_synthesis_dct_post.s \
	decode_src/aac_sbr_synthesis_dct_pre.s \
	decode_src/aac_sbr_synthesis_filter.s \
	decode_src/AAC_stream_asm.s \
	decode_src/AAC_stream_iq_asm.s \
	decode_src/AAC_TNS_DecodeCoef.s \
	decode_src/aacplus_fft.s \
	decode_src/imdct128.s \
	decode_src/imdct_1024_pre.s \
	decode_src/long_block_only_window.s \
	decode_src/long_block_start_window.s \
	decode_src/long_block_window_end.s \
	decode_src/short_block_only_window.s


LOCAL_CFLAGS := -fno-strict-aliasing -D_AACARM_  -D_ARMNINEPLATFORM_  -DAAC_DEC_LITTLE_ENDIAN
LOCAL_STATIC_LIBRARIES := 
LOCAL_SHARED_LIBRARIES :=

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/decode_src \
        $(LOCAL_PATH)/decode_inc \


LOCAL_ARM_MODE := arm

LOCAL_MODULE := libaacdec_sprd

include $(BUILD_STATIC_LIBRARY)

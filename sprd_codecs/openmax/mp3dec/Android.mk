LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/mp3_bit.c \
	src/mp3_dec_api.c \
	src/mp3_fixed.c \
	src/mp3_frame.c \
        src/mp3_huffman.c \
        src/mp3_layer12.c \
        src/mp3_stream.c \
        src/mp3_synth.c \
        src/mp3_layer3.c \
        asm/mp3_layerIII_asm.s \
	asm/mp3_bit_operate_asm_1.s \
	asm/mp3_synth_asm.s


LOCAL_CFLAGS := -fno-strict-aliasing -DOPT_DCT_FIXED31 -DFPM_64BIT -D__ASO__
LOCAL_LDFLAGS += -Wl,--no-warn-shared-textrel

LOCAL_STATIC_LIBRARIES := 
LOCAL_SHARED_LIBRARIES :=

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/src \
        $(LOCAL_PATH)/inc \
        $(LOCAL_PATH)/asm \


LOCAL_ARM_MODE := arm

LOCAL_MODULE := libomx_mp3dec_sprd
LOCAL_MODULE_TAGS := optional
LOCAL_32_BIT_ONLY := true

include $(BUILD_SHARED_LIBRARY)

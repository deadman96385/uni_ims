ifeq (0,true)
LOCAL_PATH:= $(call my-dir)

################################################################################
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	decode_src/MONKEY_APE_ASM.s \
	decode_src/MONKEY_APEDecompress.c \
	decode_src/MONKEY_CircleBuffer.c \
	decode_src/MONKEY_NewPredictor.c \
	decode_src/MONKEY_NNFilter.c \
	decode_src/MONKEY_Prepare.c \
	decode_src/MONKEY_RollBuffer.c \
	decode_src/MONKEY_UnBitArray.c \
	decode_src/MONKEY_UnBitArrayBase.c


LOCAL_CFLAGS := -fno-strict-aliasing -D_AACARM_  -D_ARMNINEPLATFORM_  -DAAC_DEC_LITTLE_ENDIAN
LOCAL_STATIC_LIBRARIES := 
LOCAL_SHARED_LIBRARIES :=

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/decode_src \
        $(LOCAL_PATH)/decode_inc

LOCAL_ARM_MODE := arm

LOCAL_MODULE := libomx_apedec_sprd
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
endif

################################################################################



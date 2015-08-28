# Copyright 2006 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_C_INCLUDES :=$(LOCAL_PATH)/../include

LOCAL_SRC_FILES:= \
	sprd_rild.c


LOCAL_SHARED_LIBRARIES := \
	liblog \
	libcutils \
	libril_sp \
	libdl

# temporary hack for broken vendor rils
LOCAL_WHOLE_STATIC_LIBRARIES := \
	librilutils_static

LOCAL_CFLAGS := -DRIL_SHLIB

LOCAL_MODULE:= rild_sp
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/libril_sp
LOCAL_MODULE_STEM_32 := rild_sp
LOCAL_MODULE_STEM_64 := rild_sp64
LOCAL_MULTILIB := both
include $(BUILD_EXECUTABLE)

# For radiooptions binary
# =======================
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	radiooptions.c

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libcutils \

LOCAL_CFLAGS := \

LOCAL_MODULE:= radiooptions_sp
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE_STEM_32 := radiooptions_sp
LOCAL_MODULE_STEM_64 := radiooptions_sp64
LOCAL_MULTILIB := both

include $(BUILD_EXECUTABLE)

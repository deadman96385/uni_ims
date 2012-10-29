# Copyright 2006 The Android Open Source Project

# XXX using libutils for simulator build only...
#
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    sprd_ril.c \
    sprd_atchannel.c \
    misc.c \
    at_tok.c

LOCAL_SHARED_LIBRARIES := \
    libcutils libutils libril_sp

# for asprinf
LOCAL_CFLAGS := -D_GNU_SOURCE

ifeq ($(BOARD_SPRD_RIL),true)
LOCAL_CFLAGS += -DRIL_SPRD_EXTENSION
endif

ifeq ($(BOARD_SAMSUNG_RIL),true)
LOCAL_CFLAGS += -DGLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION
endif

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../include $(KERNEL_HEADERS)

ifeq (foo,foo)
#build shared library
LOCAL_SHARED_LIBRARIES += \
      libcutils libutils
LOCAL_LDLIBS += -lpthread
LOCAL_CFLAGS += -DRIL_SHLIB
LOCAL_MODULE:= libreference-ril_sp
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
else
#build executable
LOCAL_SHARED_LIBRARIES += \
      libril_sp
LOCAL_MODULE:= reference-ril_sp
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)
endif

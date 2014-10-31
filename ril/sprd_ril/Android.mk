# Copyright 2006 The Android Open Source Project

# XXX using libutils for simulator build only...
#
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    sprd_ril.c \
    sprd_atchannel.c \
    misc.c \
    at_tok.c

LOCAL_SHARED_LIBRARIES := \
    liblog libcutils libutils libril_sp librilutils

# for asprinf
LOCAL_CFLAGS := -D_GNU_SOURCE

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../include $(KERNEL_HEADERS)

ifeq ($(BOARD_SPRD_RIL),true)
LOCAL_CFLAGS += -DRIL_SPRD_EXTENSION
endif


LOCAL_CFLAGS += -DSIM_AUTO_POWERON


ifeq ($(BOARD_SAMSUNG_RIL),true)
LOCAL_CFLAGS += -DGLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION
#ifeq ($(ENABLE_HOOK_RAW),true)
LOCAL_SRC_FILES += sril/sril.c \
                sril/sril_svcmode.c \
                sril/sril_svcmode_version.c \
                sril/sril_sysdump.c \
                sril/sril_imei.c \
                sril/sril_gprs.c \
                sril/samsung_nv_flash.c \
                sril/sprd_ril_api.c
LOCAL_CFLAGS += -DCONFIG_SAMSUNG_HOOK_RAW
LOCAL_C_INCLUDES += $(LOCAL_PATH)/sril/ \
                $(TOP)/external/libpcap
#               $(TOP)/vendor/samsung/feature/CscFeature/libsecnativefeature
#LOCAL_SHARED_LIBRARIES += libsecnativefeature
LOCAL_STATIC_LIBRARIES += libpcap
#endif
endif

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
LOCAL_MODULE:= reference-ril_sp
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)
endif

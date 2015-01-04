LOCAL_PATH := $(call my-dir)

common_flags := -D_GNU_SOURCE
RPMB_FILE := false
ifeq (true, $(RPMB_FILE))
common_flags += -DTEST_FILE
endif
TKS_TEST := false
RPMB_TEST := false 

include $(CLEAR_VARS)

LOCAL_C_INCLUDES    +=  vendor/sprd/open-source/libs/libatchannel/
LOCAL_SRC_FILES := rpmb_api.c tks_lib.c
ifneq (true, $(RPMB_FILE))
LOCAL_SRC_FILES += rpmb_ops.c
endif
LOCAL_SHARED_LIBRARIES += libatchannel libcutils
LOCAL_CFLAGS := $(common_flags)
LOCAL_MODULE_TAGS := optional
#LOCAL_STATIC_LIBRARIES := $(common_libs)

LOCAL_MODULE := libtks
include $(BUILD_SHARED_LIBRARY)

ifeq (true,$(RPMB_TEST))
include $(CLEAR_VARS)

LOCAL_SRC_FILES := rpmb_api.c test_rpmb_api.c
ifneq (true, $(RPMB_FILE))
LOCAL_SRC_FILES += rpmb_ops.c
endif
LOCAL_SHARED_LIBRARIES += libcutils
LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := rpmb_test
include $(BUILD_EXECUTABLE)
endif


ifeq (true,$(TKS_TEST))
include $(CLEAR_VARS)

LOCAL_C_INCLUDES    +=  vendor/sprd/open-source/libs/libatchannel/
LOCAL_SRC_FILES := rpmb_api.c tks_lib.c test_tkslib.c
ifneq (true, $(RPMB_FILE))
LOCAL_SRC_FILES += rpmb_ops.c
endif
LOCAL_SHARED_LIBRARIES += libatchannel libcutils
LOCAL_CFLAGS := $(common_flags)
LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := tks_test
include $(BUILD_EXECUTABLE)
endif

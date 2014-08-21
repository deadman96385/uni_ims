LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := rilproxy.cpp

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
                    $(LOCAL_PATH)/../ril/include/telephony/

LOCAL_SHARED_LIBRARIES := \
                 libcutils \
                 liblog \
                 libbinder \
                 libutils 

ifeq ($(BOARD_SPRD_RIL),true)
LOCAL_CFLAGS += -DRIL_SPRD_EXTENSION
endif

LOCAL_MODULE:= librilproxy
LOCAL_MODULE_TAGS := optional
#LOCAL_LDLIBS += -lpthread
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)

LOCAL_SRC_FILES := rilproxyd.c

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
                    external/sqlite/dist

LOCAL_SHARED_LIBRARIES := \
                 libcutils \
                 liblog \
                 libsqlite \
                 librilproxy

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := rilproxyd

#LOCAL_LDLIBS += -lpthread

include $(BUILD_EXECUTABLE)

###########################BUILD_EXECUTABLE##########################################
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

MY_CFLAGS := \
	-DOSAL_PTHREADS \
	-DANDROID_ICS

include $(call all-makefiles-under,$(LOCAL_PATH))

include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libcutils


LOCAL_WHOLE_STATIC_LIBRARIES := \
	libvpad_mux\
	libvpad_io \
	libvpad \
	libosal_user_bionic

LOCAL_32_BIT_ONLY := true

LOCAL_MODULE:= modemDriver_vpad_main
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

#########################BUILD_SHARED_LIBRARY########################################
include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libcutils


LOCAL_WHOLE_STATIC_LIBRARIES := \
	libvpad_mux\
	libvpad_io \
	libvpad

LOCAL_32_BIT_ONLY := true

LOCAL_MODULE:= libmodemDriver_vpad
LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)
##############################BUILD_TEST_TXRX#######################################

ifeq ($(MY_4G_PLUS_MODEM_TEST),y)
include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libcutils


LOCAL_WHOLE_STATIC_LIBRARIES := \
	libvpad_mux\
	libvpad_io \
	libvpad_test\
	libosal_user_bionic

LOCAL_32_BIT_ONLY := true

LOCAL_MODULE:= libvpad_TXRXtest
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)
endif

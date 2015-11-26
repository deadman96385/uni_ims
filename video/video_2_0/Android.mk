LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

include $(LOCAL_PATH)/config.mk
include $(call all-named-subdir-makefiles, $(MY_MODULE_SUB_DIRS))

include $(CLEAR_VARS)

LOCAL_CFLAGS := \
	$(MY_SYSTEM_CFLAGS) \
	$(MY_CFLAGS)

LOCAL_WHOLE_STATIC_LIBRARIES := \
	$(MY_MODULE_OTHER_LIBS) \
	$(MY_INCLUDE_LIBS)

LOCAL_SHARED_LIBRARIES := \
	liblog

LOCAL_32_BIT_ONLY := true

LOCAL_MODULE := libvideo_2_0

include $(BUILD_SHARED_LIBRARY)



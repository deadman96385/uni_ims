LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    liblog \
    libnativehelper \
    libandroid_runtime \
    libgui \
    libui \
    libmedia \
    libbinder \
    libcameraservice \
    libcamera_client \
    libhardware \

LOCAL_SRC_FILES += VideoCallEngineCamera.cpp

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/ \
    vendor/sprd/proprietories-source/video/include_ac702 \
    $(JNI_H_INCLUDE) \
    system/media/camera/include \
    frameworks/base/include/binder \
    frameworks/base/include/gui \
    frameworks/native/include/binder \
    frameworks/av/include/camera \
    frameworks/base/include/media \
    frameworks/av/services/camera/libcameraservice \
    frameworks/av/services/camera/libcameraservice/device1 \
    hardware/libhardware/include/hardware \
    hardware/libhardware/modules/gralloc \

LOCAL_32_BIT_ONLY := true

LOCAL_MODULE:= libvcecamera

LOCAL_MODULE_TAGS:=optional

LOCAL_PRELINK_MODULE := false

LOCAL_PROPRIETARY_MODULE := false

include $(BUILD_SHARED_LIBRARY)

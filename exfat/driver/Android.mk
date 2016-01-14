
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := exfat_fs.ko
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(PRODUCT_OUT)/root/lib/modules
LOCAL_STRIP_MODULE := keep_symbols
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

ifeq ($(TARGET_BUILD_VARIANT),user)
  DEBUGMODE := BUILD=no
else
  DEBUGMODE := $(DEBUGMODE)
endif

ifeq ($(TARGET_ARCH),arm64)
ARCH_ := arm64
CROSS_COMPILE_ := aarch64-linux-android-
else
ARCH_ := arm
CROSS_COMPILE_ := arm-eabi-
endif

ifeq ($(strip $(CONFIG_64KERNEL_32FRAMEWORK)),true)
ARCH_ := arm64
CROSS_COMPILE_ := $(FIX_CROSS_COMPILE)
endif

STRIP_TARGET := $(LOCAL_PATH)/exfat_fs.ko

$(LOCAL_PATH)/exfat_fs.ko: $(TARGET_PREBUILT_KERNEL)
	$(MAKE) -C $(shell dirname $@) ARCH=$(ARCH_) CROSS_COMPILE=$(CROSS_COMPILE_) $(DEBUGMODE) KDIR=$(ANDROID_PRODUCT_OUT)/obj/KERNEL clean
	$(MAKE) -C $(shell dirname $@) ARCH=$(ARCH_) CROSS_COMPILE=$(CROSS_COMPILE_) $(DEBUGMODE) KDIR=$(ANDROID_PRODUCT_OUT)/obj/KERNEL
	$(TARGET_STRIP) --strip-debug  $(STRIP_TARGET)

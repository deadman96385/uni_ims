LOCAL_PATH := $(call my-dir)

# Prepare archs assuming
# TODO not support multi archs
my_archs := arm x86 arm64 x86_64
my_src_arch := $(call get-prebuilt-src-arch, $(my_archs))
ifeq ($(my_src_arch),arm)
    my_src_abi := armeabi-v7a
else ifeq ($(my_src_arch),x86)
    my_src_abi := x86
else ifeq ($(my_src_arch),arm64)
    my_src_abi := arm64-v8a
else ifeq ($(my_src_arch),x86_64)
    my_src_abi := x86_64
endif

# TODO asumme linux or macos, the unzip command may perform different
# TODO can not compatibility with LOCAL_MUTILIB
define get-prebuilt-jni
    $(strip $(if $(wildcard $(strip $(LOCAL_PATH))/$(strip $(1))), \
        $(foreach arch,$(2), \
            $(if $(shell unzip -l $(strip $(LOCAL_PATH))/$(strip $(1)) | grep $(arch)), \
                $(eval LOCAL_PREBUILT_JNI_LIBS += @lib/$(arch)/*) \
            ,) \
        ) \
    ,))
endef

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := BaiduNetworkLocation
LOCAL_MODULE_STEM := BaiduNetworkLocation.apk
LOCAL_MODULE_CLASS := APPS
LOCAL_CERTIFICATE := PRESIGNED
LOCAL_PRIVILEGED_MODULE := true
LOCAL_SRC_FILES := GlobalNetworkLocation_offline_https_Baidu_zhanxun_Build-645_20160708.apk
#$(call get-prebuilt-jni, $(LOCAL_SRC_FILES), $(my_src_abi))
LOCAL_PREBUILT_JNI_LIBS := lib/$(my_src_abi)/liblocSDK6c.so lib/$(my_src_abi)/libnetworklocation.so

include $(BUILD_PREBUILT)

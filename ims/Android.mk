LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_PACKAGE_NAME := ims
LOCAL_CERTIFICATE := platform
LOCAL_JAVA_LIBRARIES := telephony-common ims-common

LOCAL_DEX_PREOPT := nostripping

LOCAL_PROGUARD_ENABLED := disabled
LOCAL_PROTOC_OPTIMIZE_TYPE := micro
res_dirs := res
LOCAL_SRC_FILES := $(call all-subdir-java-files)
LOCAL_RESOURCE_DIR := $(addprefix $(LOCAL_PATH)/, $(res_dirs))
include $(BUILD_PACKAGE)
include $(call all-makefiles-under,$(LOCAL_PATH))


LOCAL_PATH:= $(call my-dir)
# ---------------------------------------------------
# For dynamic binary
# ---------------------------------------------------
include $(CLEAR_VARS)
LOCAL_ADDITIONAL_DEPENDENCIES := $(common_deps)
LOCAL_SRC_FILES := server/ylog.c
LOCAL_MODULE := ylog
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libcutils
include $(BUILD_EXECUTABLE)
CUSTOM_MODULES += ylog # no need appending this module in PRODUCT_PACKAGES

include $(CLEAR_VARS)
LOCAL_ADDITIONAL_DEPENDENCIES := $(common_deps)
LOCAL_SRC_FILES := client/ylog_cli.c
LOCAL_MODULE := ylog_cli
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libcutils liblog
include $(BUILD_EXECUTABLE)
CUSTOM_MODULES += ylog_cli # no need appending this module in PRODUCT_PACKAGES

include $(CLEAR_VARS)
LOCAL_ADDITIONAL_DEPENDENCIES := $(common_deps)
LOCAL_SRC_FILES := benchmark/ylog_benchmark.c
LOCAL_MODULE := ylog_benchmark
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libcutils
include $(BUILD_EXECUTABLE)
CUSTOM_MODULES += ylog_benchmark # no need appending this module in PRODUCT_PACKAGES

include $(CLEAR_VARS)
LOCAL_ADDITIONAL_DEPENDENCIES := $(common_deps)
LOCAL_SRC_FILES := benchmark/ylog_benchmark_socket_server.c
LOCAL_MODULE := ylog_benchmark_socket_server
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libcutils
include $(BUILD_EXECUTABLE)
CUSTOM_MODULES += ylog_benchmark_socket_server # no need appending this module in PRODUCT_PACKAGES
# ---------------------------------------------------
# For static binary
# ---------------------------------------------------
include $(CLEAR_VARS)
LOCAL_ADDITIONAL_DEPENDENCIES := $(common_deps)
LOCAL_SRC_FILES := server/ylog.c
LOCAL_MODULE := ylog-static
LOCAL_MODULE_TAGS := optional

LOCAL_FORCE_STATIC_EXECUTABLE:=true
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := ylog.static
LOCAL_MODULE_STEM_64 := ylog.static64

LOCAL_STATIC_LIBRARIES := libc libcutils liblog
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_ADDITIONAL_DEPENDENCIES := $(common_deps)
LOCAL_SRC_FILES := client/ylog_cli.c
LOCAL_MODULE := ylog_cli-static
LOCAL_MODULE_TAGS := optional

LOCAL_FORCE_STATIC_EXECUTABLE:=true
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := ylog_cli.static
LOCAL_MODULE_STEM_64 := ylog_cli.static64

LOCAL_STATIC_LIBRARIES := libc libcutils liblog
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_ADDITIONAL_DEPENDENCIES := $(common_deps)
LOCAL_SRC_FILES := benchmark/ylog_benchmark.c
LOCAL_MODULE := ylog_benchmark-static
LOCAL_MODULE_TAGS := optional

LOCAL_FORCE_STATIC_EXECUTABLE:=true
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := ylog_benchmark.static
LOCAL_MODULE_STEM_64 := ylog_benchmark.static64

LOCAL_STATIC_LIBRARIES := libc libcutils
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_ADDITIONAL_DEPENDENCIES := $(common_deps)
LOCAL_SRC_FILES := benchmark/ylog_benchmark_socket_server.c
LOCAL_MODULE := ylog_benchmark_socket_server-static
LOCAL_MODULE_TAGS := optional

LOCAL_FORCE_STATIC_EXECUTABLE:=true
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := ylog_benchmark_socket_server.static
LOCAL_MODULE_STEM_64 := ylog_benchmark_socket_server.static64

LOCAL_STATIC_LIBRARIES := libc libcutils
include $(BUILD_EXECUTABLE)

include $(LOCAL_PATH)/tool/logd_test/Android.mk \
        $(LOCAL_PATH)/tool/ycat/Android.mk \
        $(LOCAL_PATH)/tool/sgm.toolkits/Android.mk

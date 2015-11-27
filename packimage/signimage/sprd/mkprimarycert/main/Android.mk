LOCAL_PATH:=$(call my-dir)
    include $(CLEAR_VARS)

mysrc_path:=$(LOCAL_PATH)/../../common
static_lib:=$(LOCAL_PATH)/../../lib

file_list:=$(wildcard $(mysrc_path)/pk1/src/*.c)

file_list += $(wildcard $(mysrc_path)/rsa/src/*.c)
file_list += $(wildcard $(mysrc_path)/sha256/src/*.c)
file_list += $(wildcard $(mysrc_path)/verify/src/*.c)

LOCAL_C_INCLUDES := \
			$(mysrc_path)/pk1/inc \
			$(mysrc_path)/rsa/inc \
			$(mysrc_path)/sha256/inc \
			$(mysrc_path)/verify/inc \
			$(mysrc_path)/openssl-1.0.2a/include

LOCAL_SRC_FILES := mkprimarycert.c
LOCAL_SRC_FILES += $(file_list:$(LOCAL_PATH)/%=%)

LOCAL_LDFLAGS := -ldl

#LOCAL_STATIC_LIBRARIES += libcrypto_static
#LOCAL_SHARED_LIBRARIES += libssl-host

LOCAL_LDFLAGS += $(static_lib)/libcrypto_static.a \
				 $(static_lib)/libssl_static.a

LOCAL_MODULE := sprd_mkprimarycert

 LOCAL_MODULE_PATH := $(HOST_OUT_EXECUTABLES)

$(shell cp -rf $(HOST_OUT_EXECUTABLES)/sprd_mkprimarycert $(LOCAL_PATH)/../bin/mkprimarycert)
$(shell chmod +x $(LOCAL_PATH)/../bin/mkprimarycert)

include $(BUILD_HOST_EXECUTABLE)

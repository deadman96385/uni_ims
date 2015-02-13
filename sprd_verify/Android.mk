LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
USE_LEGENCY_RSA :=
ifeq ($(USE_LEGENCY_RSA), true)
LOCAL_SRC_FILES := \
	rsa/rsa.c \
	rsa/rsa_multidw.s \
	rsa/rsa_sub.s \
	sha1/sha1_32.c \
	sec_boot.c
else
LOCAL_SRC_FILES := \
	rsa_sprd/rsa.c \
	rsa_sprd/pk1.c \
	sha1/sha1_32.c \
	sec_boot.c
endif
ifeq ($(USE_LEGENCY_RSA), true)
LOCAL_CFLAGS := -DUSE_LEGENCY_RSA
else

LOCAL_CFLAGS := 
endif
LOCAL_LDFLAGS += 

LOCAL_STATIC_LIBRARIES := 
LOCAL_SHARED_LIBRARIES :=

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/rsa \
        $(LOCAL_PATH)/sha1

#LOCAL_ARM_MODE := arm

LOCAL_MODULE := libsprd_verify
#LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)


#########################################
# 
include $(CLEAR_VARS)

LOCAL_SRC_FILES := sprd_verify.c

LOCAL_CFLAGS := 

LOCAL_MODULE_TAGS := optional

LOCAL_STATIC_LIBRARIES := libsprd_verify

LOCAL_MODULE := sprd_verify

include $(BUILD_EXECUTABLE)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/ccrgb16toyuv420.cpp \
 	src/ccyuv422toyuv420.cpp \
 	src/cczoomrotation16.cpp \
 	src/cczoomrotationbase.cpp \
 	src/ccrgb24toyuv420.cpp \
 	src/ccrgb12toyuv420.cpp \
 	src/ccyuv420semiplnrtoyuv420plnr.cpp \
 	src/ccyuv420toyuv420semi.cpp \
 	src/ccyuv420toyuv422.cpp


LOCAL_MODULE := libcolorconvert
LOCAL_MODULE_TAGS := optional


LOCAL_CFLAGS := -DFALSE=false  $(PV_CFLAGS)
LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES := 

LOCAL_SHARED_LIBRARIES := 

LOCAL_C_INCLUDES := \
	$(PV_TOP)/codecs_v2/utilities/colorconvert/src \
 	$(PV_TOP)/codecs_v2/utilities/colorconvert/include \
 	$(PV_INCLUDES)

LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)

LOCAL_COPY_HEADERS := \
	include/cczoomrotationbase.h \
 	include/cczoomrotation16.h \
 	include/ccrgb16toyuv420.h \
 	include/ccyuv422toyuv420.h \
 	include/colorconv_config.h \
 	include/ccrgb24toyuv420.h \
 	include/ccrgb12toyuv420.h \
 	include/ccyuv420semitoyuv420.h \
 	include/ccyuv420toyuv420semi.h \
 	include/ccyuv420toyuv422.h

include $(BUILD_STATIC_LIBRARY)

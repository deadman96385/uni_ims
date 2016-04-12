LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= main.c
LOCAL_SHARED_LIBRARIES:=liblog libnl
LOCAL_MODULE := lmfs
include $(BUILD_EXECUTABLE)


# Get the current local path as the first operation
LOCAL_PATH := $(call get_makefile_dir)

# Clear out the variables used in the local makefiles
include $(MK)/clear.mk

TARGET := omx_baseclass_lib

XINCDIRS += ../../../../../extern_libs_v2/khronos/openmax/include

SRCDIR := ../../src
INCSRCDIR := ../../include

SRCS := pv_omxcomponent.cpp

HDRS := pv_omxcomponent.h

include $(MK)/library.mk

# Get the current local path as the first operation
LOCAL_PATH := $(call get_makefile_dir)

# Clear out the variables used in the local makefiles
include $(MK)/clear.mk

TARGET := pvaacparser



SRCDIR := ../../src
INCSRCDIR := ../../include

SRCS := aacfileparser.cpp \
	aacfileio.cpp

HDRS := aacfileparser.h \
	aacfileio.h

include $(MK)/library.mk

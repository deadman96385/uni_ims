# Get the current local path as the first operation
LOCAL_PATH := $(call get_makefile_dir)

# Clear out the variables used in the local makefiles
include $(MK)/clear.mk

TARGET := pvrtspunicaststreamingmanager
XCXXFLAGS := $(FLAG_COMPILE_WARNINGS_AS_ERRORS)

XINCDIRS += ../../../../../common/include ../../../common/include ../../../../../jitterbuffernode/jitterbuffer/common/include
XINCDIRS += ../../../../../../../protocols/rtsp_client_engine/inc ../../../../../../../protocols/rtsp_client_engine/src

SRCDIR := ../../src 
INCSRCDIR := ../../include

SRCS :=	pvmf_sm_fsp_rtsp_unicast.cpp \
	    pvmf_sm_rtsp_unicast_node_factory.cpp

HDRS := pvmf_sm_rtsp_unicast_node_factory.h	

include $(MK)/library.mk

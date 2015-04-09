#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 21510 $ $Date: 2013-07-18 15:48:08 -0700 (Thu, 18 Jul 2013) $
#

#
# Do not modify this Makefile
#

# Note always use forward slashes for pathnames other than executables

UNAME := $(shell uname)
HOST_MACHINE := $(shell uname -m)


ifeq ($(UNAME), Linux)
export PLATFORM_OS                       := linux
endif

ifeq ($(UNAME), Darwin)
export PLATFORM_OS                       := darwin
endif

# The host may require a 32-bit or 64-bit version of the toolchain
ifeq ($(HOST_MACHINE), i686)
export PLATFORM_ARCH             := x86
else
export PLATFORM_ARCH             := x86_64
endif


export TOOLBASE				:= $(BASE_DIR)/../android-ndk-r9
export TOOL_PREFIX          := arm-linux-androideabi
export TOOL_VERSION         := 4.6
export ANDROID_API			:= 9
export ANDROID_ARCH		 	:= arm


# The BSP_VPORT_ROOT variable can be set in the environment to override
# the default location for the BSP toolchain.
ifndef BSP_VPORT_TOOL_ROOT
export BSP_VPORT_TOOL_ROOT	:= \
	$(TOOLBASE)/toolchains/$(TOOL_PREFIX)-$(TOOL_VERSION)/prebuilt/$(PLATFORM_OS)-$(PLATFORM_ARCH)
endif
ifndef BSP_VPORT_TOOL_LIB
export BSP_VPORT_TOOL_LIB	:= \
	$(BSP_VPORT_TOOL_ROOT)/lib/gcc/$(TOOL_PREFIX)/$(TOOL_VERSION)
endif
export ANDROID_VERSION      := jb
export VIDEO_DIR			:= video_2_0
export VOICE_DIR            := ve

ifndef BSP_VPORT_LIB_ROOT
export BSP_VPORT_LIB_ROOT	:= \
	$(TOOLBASE)/platforms/android-$(ANDROID_API)/arch-$(ANDROID_ARCH)/usr
endif

ifndef BSP_VPORT_STD_INCLUDE
export BSP_VPORT_STD_INCLUDE	:= \
	$(TOOLBASE)/platforms/android-$(ANDROID_API)/arch-$(ANDROID_ARCH)/usr/include
endif


export OSAL_OS_DEFINE	:= OSAL
export OS_DIR			:= 
export OS_CLIB_DIR		:= bionic
export CORE_DIR		:= arch/$(PLATFORM)
export VPORT_OS			:= linux_pc
export VPORT_OS_VER		:= 26

export OS_LIBS = c gcc log

export OS_LIB_DIRS = $(BASE_DIR)/extern/openssl/arch-$(ANDROID_ARCH)/lib

export CCDEFINES = \
	-DCPU=ARM7 -DTOOL_FAMILY=GNU -DTOOL=$(PLATFORM) \
	-DEXPORT_SYMTAB \
	$(addprefix -D,$(OSAL_OS_DEFINE)) \
	$(addprefix -D,PLATFORM=ac702) \
	$(SYSTEM_CFLAGS)

export CC          := $(BSP_VPORT_TOOL_ROOT)/bin/$(TOOL_PREFIX)-gcc
export CPP         := $(BSP_VPORT_TOOL_ROOT)/bin/$(TOOL_PREFIX)-c++
export AS          := $(BSP_VPORT_TOOL_ROOT)/bin/$(TOOL_PREFIX)-as
export LD          := $(BSP_VPORT_TOOL_ROOT)/bin/$(TOOL_PREFIX)-ld
export CXX         := $(BSP_VPORT_TOOL_ROOT)/bin/$(TOOL_PREFIX)-g++
export AR          := $(BSP_VPORT_TOOL_ROOT)/bin/$(TOOL_PREFIX)-ar
export OBJCOPY := $(BSP_VPORT_TOOL_ROOT)/bin/$(TOOL_PREFIX)-objcopy
export STRIP   := $(BSP_VPORT_TOOL_ROOT)/bin/$(TOOL_PREFIX)-strip
export ENDIAN  := LITTLE_ENDIAN
export OBJDUMP := $(BSP_VPORT_TOOL_ROOT)/bin/$(TOOL_PREFIX)-objdump
export MIPS32TO2	:= touch

export LDFLAGS		:= $(LDFLAGS) -EL \
		-L$(BASE_DIR)/extern/openssl/lib \
		-L$(BSP_VPORT_LIB_ROOT)/lib \
		-nostdlib

export ALL_CPPFLAGS =  \
	-fno-rtti

export ALL_CFLAGS =  \
		$(DEFAULT_CFLAGS_GNU) \
		$(CCDEFINES) \
	-pthread \
	-fno-exceptions \
   	-Wno-multichar \
	-msoft-float \
	-fpic \
    -fPIE \
	-DD2_$(ENDIAN) \
	-ffunction-sections \
    -fdata-sections \
	-funwind-tables \
	-fstack-protector \
    -Wa,--noexecstack \
    -Werror=format-security \
	-fno-short-enums \
	-march=armv7-a \
	-mfloat-abi=softfp \
	-mfpu=neon \
    -fno-builtin-sin \
    -Wno-psabi \
	-mthumb-interwork \
	-DANDROID \
	-fmessage-length=0 \
	-W \
	-Wall \
	-Wno-unused \
	-Winit-self \
	-Wpointer-arith \
    -Wstrict-aliasing=2 \
	-UDEBUG \
	-fgcse-after-reload \
	-frerun-cse-after-loop \
	-frename-registers \
    -mthumb \
    -Os \
	-fno-strict-aliasing \
	-finline-functions \
	-fno-inline-functions-called-once \
	-O2 \
	-nodefaultlibs \
	-finline-limit=64  \
	-fsigned-char \
	-DSK_RELEASE \
	-DNDEBUG \
	-fomit-frame-pointer \
	-MD \
		-I$(BASE_DIR)/extern/arpa/include \
		-I$(BASE_DIR)/extern/openssl/include \
		-I$(BSP_VPORT_STD_INCLUDE) \

export CCDEFINES_ADD_FOR_LKM =

export CFLAGS_ADD_FOR_LKM =

export CDEFINES_ADD_FOR_USERLAND = \
		-DOSAL_PTHREADS	\
		-DANDROID_ICS

export CFLAGS_ADD_FOR_USERLAND =

# END OF MAKEFILE

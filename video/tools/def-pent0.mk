#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 26274 $ $Date: 2014-05-16 17:49:19 +0800 (Fri, 16 May 2014) $
#

#
# Do not modify this Makefile
#

# Note always use forward slashes for pathnames other than executables

# The BSP_VPORT_ROOT variable can be set in the environment to override
# the default location for the BSP toolchain.
ifndef BSP_VPORT_TOOL_ROOT
export BSP_VPORT_TOOL_ROOT	:= \
	/usr
endif
ifndef BSP_VPORT_KERNEL_ROOT
export BSP_VPORT_KERNEL_ROOT := \
	/usr/src
endif
ifndef BSP_VPORT_LIB_ROOT
export BSP_VPORT_LIB_ROOT	:= \
	$(BSP_VPORT_TOOL_ROOT)/lib
endif

export OSAL_OS_DEFINE	:= OSAL
export OS_DIR			:=
export OS_CLIB_DIR		:= glibc
export CORE_DIR	  		:= arch/pent0 
export VPORT_OS			:= linux_pc
export VPORT_OS_VER		:= 26

export OS_INCLUDE_DIRS 	=
export OS_LIB_DIRS 		=
export OS_OTHER_LIB_DIR	= ${BSP_VPORT_TOOL_ROOT}/lib
export OS_LIBS          = c pthread ssl crypto asound resolv 

export CCDEFINES 		= -m32 -g \
						-DCPU=I386 -DTOOL_FAMILY=GNU -DTOOL=$(PLATFORM) \
				  		-DEXPORT_SYMTAB \
						$(addprefix -D,$(OSAL_OS_DEFINE)) \
						$(addprefix -D,PLATFORM=pent0) \
						$(SYSTEM_CFLAGS)

export CC			:= $(BSP_VPORT_TOOL_ROOT)/bin/gcc
export CPP			:= $(BSP_VPORT_TOOL_ROOT)/bin/cpp
export AS			:= $(BSP_VPORT_TOOL_ROOT)/bin/as
export LD			:= $(BSP_VPORT_TOOL_ROOT)/bin/ld
export AR			:= $(BSP_VPORT_TOOL_ROOT)/bin/ar
export CXX			:= $(BSP_VPORT_TOOL_ROOT)/bin/g++
export OBJCOPY		:= $(BSP_VPORT_TOOL_ROOT)/bin/objcopy
export STRIP		:= $(BSP_VPORT_TOOL_ROOT)/bin/strip
export OBJDUMP		:= $(BSP_VPORT_TOOL_ROOT)/bin/objdump
export ENDIAN		:= LITTLE_ENDIAN

export LDFLAGS		:= $(LDFLAGS) \
		-L$(BSP_VPORT_TOOL_ROOT)/lib

export MIPS32TO2	:= touch

export ALL_CPPFLAGS =  \
	-fno-rtti

export ALL_CFLAGS = $(DEFAULT_CFLAGS_GNU) \
	-g \
	-Wall -fno-strict-aliasing -fsigned-char \
	-Werror \
	-DD2_$(ENDIAN) \
	$(CCDEFINES)

export CDEFINES_ADD_FOR_USERLAND = \
		-DOSAL_PTHREADS	\
		-I$(BSP_VPORT_TOOL_ROOT)/include \
		-I$(JDK)/include \
		-I$(JDK)/include/linux \

export CFLAGS_ADD_FOR_USERLAND = \

# END OF MAKEFILE


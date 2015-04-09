#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 18112 $ $Date: 2012-09-07 14:17:10 +0800 $
#

#
# Do not modify this Makefile
#

# Note always use forward slashes for pathnames other than executables

# The BSP_VPORT_ROOT variable can be set in the environment to override
# the default location for the BSP toolchain.
ifndef IDH_ROOT
export IDHBASE          := \
    /export/ac501/MOCORTM_IDH/MS_Code
endif
ifndef BSP_VPORT_TOOL_ROOT
export TOOLBASE			 := \
    /opt/ARM/RVDS
export BSP_VPORT_TOOL_ROOT	:= \
    $(TOOLBASE)/RVCT/Programs/4.1/713/linux-pentium
endif
ifndef BSP_VPORT_LIB_ROOT
export BSP_VPORT_LIB_ROOT	:= \
    $(TOOLBASE)/RVCT/Data/4.1/713/lib
endif

export OSAL_OS_DEFINES  := OSAL_THREADX
export OS_DIR			:= 
export CORE_DIR	  		:= arch/ac501
export VPORT_OS			:= threadx
export VPORT_OS_VER		:= 

export OS_LIBS = c 

export CCDEFINES = \
	-DCPU=ARMARCH7 -DTOOL_FAMILY=GNU -DTOOL=$(PLATFORM) \
	-DEXPORT_SYMTAB \
	$(addprefix -D,$(OSAL_OS_DEFINE)) \
	$(addprefix -D,PLATFORM=ac501) \
	$(SYSTEM_CFLAGS)

export CC	   := $(BSP_VPORT_TOOL_ROOT)/armcc
export LD	   := $(BSP_VPORT_TOOL_ROOT)/armlink
export AR	   := $(BSP_VPORT_TOOL_ROOT)/armar
export ENDIAN  := LITTLE_ENDIAN
export MIPS32TO2	:= touch

export LDFLAGS		:= $(LDFLAGS)  \
		$(BSP_VPORT_LIB_ROOT)/

export ALL_CPPFLAGS =  \
	-fno-rtti

export ALL_CFLAGS =  \
		-D$(PROVIDER) \
		-DOSAL_THREADX \
		-DOSA_DEBUG \
		-DPLATFORM_SC9600 \
		$(addprefix -I,$(INCLUDE_DIR) $(MODULE_DIR) .) \
	 	$(addprefix -I,$(OS_INCLUDE_DIRS)) \
	 	$(addprefix -I,$(SYSTEM_INCLUDE_DIRS))\
		$(CCDEFINES) \
		-I$(TOOLBASE)/RVCT/Data/4.1/713/include/unix \
        -I$(IDHBASE)/PS/sdi/export/inc/ \
        -I$(IDHBASE)/PS/stack/common/interfaces/common/include \
        -I$(IDHBASE)/PS/sdi/test/mta/inc \
        -I$(IDHBASE)/common/export/inc/ \
        -I$(IDHBASE)/PS/stack/common/nvm/include/ \
        -I$(IDHBASE)/PS/stack/common/config/stack/common/include/ \
        -I$(IDHBASE)/PS/stack/common/interfaces/common/include/ \
        -I$(IDHBASE)/PS/stack/ims/export/ \
        -I$(IDHBASE)/RTOS/export/inc \
        -I$(IDHBASE)/DAPS/export/inc        \
        -I$(IDHBASE)/MS_Ref/source/ppm/prot/mux/inc \
        -I$(IDHBASE)/chip_drv/export/inc \
        -I$(IDHBASE)/PS/build/obj/gen/debug/ \
        -I$(IDHBASE)/MS_Ref/export/inc	\
        -I$(IDHBASE)/MS_Ref/source/aud_config/inc_export/ \
        -I$(IDHBASE)/PS/build/obj/gen/production/ \
        -I$(IDHBASE)/MSL/l4/inc/ \
        -I$(IDHBASE)/MSL/sim/inc/ \
        -I$(IDHBASE)/MSL/gas/inc/ \
        -I$(IDHBASE)/MSL/dsm/inc/ \
		-DD2_$(ENDIAN) \
		-D_ROTS -D_DEBUG \
		--cpu cortex-a5 \
		-O2 \
		--enum_is_int \
		--loose_implicit_cast \
		--no_unaligned_access \
		--li \
		--thumb \
		--apcs /interwork/ \
		--via $(IDHBASE)/build/sc9620_refp_tcsfb_ca5_volte_builddir/dep/version_C_MACRO_INC.txt \


export CCDEFINES_ADD_FOR_LKM =

export CFLAGS_ADD_FOR_LKM =

export CDEFINES_ADD_FOR_USERLAND = 

export CFLAGS_ADD_FOR_USERLAND = 

# END OF MAKEFILE

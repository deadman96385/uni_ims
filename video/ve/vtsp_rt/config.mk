#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 30371 $ $Date: 2014-12-11 19:28:43 +0800 (Thu, 11 Dec 2014) $
#

#
# This file is for module level configuration
#
# -------- You may modify this file to include customizations
#

#export CDEBUG  = -g

#
# List dirs in order for making
export MODULE_SUB_DIRS		:= 	\
		vtspr dr

ifeq ($(VPORT_ENABLE_GATEWAY),y)
export MODULE_SUB_DIRS		:= 	\
		vtspr dr cid
endif

export MODULE_PREFIX	= VTSP_RT

ifeq ($(VPORT_OS),linux) 
export MODULE_SUB_DIRS		+= 	\
		vtspr_lkm
export MODULE_OUTPUT	= lkm
export MODULE_CDEFINES	= $(CDEFINES_ADD_FOR_LKM)
export MODULE_CFLAGS	= $(CFLAGS_ADD_FOR_LKM)
endif

ifeq ($(VPORT_OS),linux_pc)
export MODULE_OUTPUT	= rlib
export MODULE_CDEFINES	= $(CDEFINES_ADD_FOR_USERLAND)
export MODULE_CFLAGS	= $(CFLAGS_ADD_FOR_USERLAND)
endif

ifeq ($(VPORT_OS),vxworks)
export MODULE_OUTPUT	= rlib
export MODULE_CDEFINES	= $(CDEFINES_ADD_FOR_USERLAND)
export MODULE_CFLAGS	= $(CFLAGS_ADD_FOR_USERLAND)
endif

ifeq ($(VPORT_OS),threadx)
export MODULE_OUTPUT	= rlib
export MODULE_CDEFINES	= $(CDEFINES_ADD_FOR_USERLAND)
export MODULE_CFLAGS	= $(CFLAGS_ADD_FOR_USERLAND)
endif

export EXTERN_LIBS		= ve_rtp.lib

export MODULE_OTHER_INCLUDE_DIRS	:= $(BASE_DIR)/ve/vtsp_alginc/$(PLATFORM)
export MODULE_OTHER_LIB_DIR			:= $(BASE_DIR)/ve/vtsp_alglib/$(PLATFORM)

# Algorithm Libraries Options
# Includes: MP, G711P1, G722, G722P1, G723, G726, G729, ILBC, GAMRNB, GAMRWB 
export MODULE_ALG_MP		:= \
	libjb.a \
	libcomm.a \
	libdcrm.a \
	libg726.a \
	libnse.a \
	libaec.a \
	libbnd.a \
	libplc.a \
	libtone.a \
	libuds.a 

export MODULE_ALG_711_ONLY      := \
        libjb.a \
        libcomm.a \
        libplc.a \
        libtone.a \
        libdcrm.a \
        libnse.a \
        libaec.a \
        libbnd.a \
        libuds.a

export MODULE_ALG_FXS := \
	libjb.a   \
	libbnd.a  \
	libcomm.a \
	libdcrm.a \
	libdtmf.a \
	libecsr.a \
	libfmtd.a \
	liblms.a  \
	libfsks.a \
	libnfe.a  \
	libnse.a  \
	libplc.a  \
	libtone.a \
	libuds.a  

export MODULE_ALG_T38 := \
	libfr38v3.a

#
# Bug 14269.
# MODULE_ALG_MP_LITE is used for no D2 software DSP included.
#
export MODULE_ALG_MP_LITE := \
	libjb.a \
	libcomm.a

export MODULE_ALG_G711P1	=  \
	libg711p1.a

export MODULE_ALG_G722		=  \
	libg722.a

export MODULE_ALG_G723		=  \
	libg723a.a

export MODULE_ALG_G726		=  \
	libg726.a

export MODULE_ALG_G729		=  \
	libg729ab.a

export MODULE_ALG_ILBC		=  \
	libilbc.a

export MODULE_ALG_GAMRNB	=  \
	libgamrnb.a

export MODULE_ALG_GAMRWB	=  \
	libgamrwb.a

export MODULE_ALG_GAMRWB_MATH = \
    libmath.a

export MODULE_ALG_SILK   	=  \
	libsilk.a

export MODULE_ALG_VOLTE   	=  \
	libgamrwb.a \
	libgamrnb.a \
	libbasicop.a
	
# ALL PLATFORM export basic MP set MODULE_ALG_MP

ifeq ($(PLATFORM),qsd01)
export MODULE_OTHER_LIBS	:= \
	$(MODULE_ALG_MP)	\
	$(MODULE_ALG_VOLTE)
endif

ifeq ($(PLATFORM),ac903)
export MODULE_OTHER_LIBS	:= \
	$(MODULE_ALG_MP) 	 \
	$(MODULE_ALG_GAMRWB) \
    $(MODULE_ALG_GAMRWB_MATH) \
	$(MODULE_ALG_GAMRNB)
endif

ifeq ($(PLATFORM),ac904)
export MODULE_OTHER_LIBS	:= \
	$(MODULE_ALG_MP) 	 \
	$(MODULE_ALG_GAMRWB) \
    $(MODULE_ALG_GAMRWB_MATH) \
	$(MODULE_ALG_GAMRNB)
endif

ifeq ($(PLATFORM),ac905)
export MODULE_OTHER_LIBS	:= \
	$(MODULE_ALG_MP) 	 \
	$(MODULE_ALG_GAMRWB) \
    $(MODULE_ALG_GAMRWB_MATH) \
	$(MODULE_ALG_GAMRNB)
endif

ifeq ($(PLATFORM),qsd06)
export MODULE_OTHER_LIBS	:= \
	$(MODULE_ALG_MP)	\
	$(MODULE_ALG_VOLTE)
endif

ifeq ($(PLATFORM),qsd08)
export MODULE_OTHER_LIBS	:= \
	$(MODULE_ALG_MP)	\
	$(MODULE_ALG_VOLTE)
endif

ifeq ($(PLATFORM),gal01)
export MODULE_OTHER_LIBS	:= \
	$(MODULE_ALG_MP) 	 \
	$(MODULE_ALG_VOLTE)
endif

ifeq ($(PLATFORM),gal02)
export MODULE_OTHER_LIBS	:= \
	$(MODULE_ALG_MP) 	 \
	$(MODULE_ALG_VOLTE)
endif

ifeq ($(PLATFORM),gal03)
export MODULE_OTHER_LIBS	:= \
	$(MODULE_ALG_MP) 	 \
	$(MODULE_ALG_VOLTE)
endif

ifeq ($(PLATFORM),ac916)
export MODULE_OTHER_LIBS	:= \
	$(MODULE_ALG_MP) 	 \
	$(MODULE_ALG_VOLTE)
endif

ifeq ($(PLATFORM),ac810)
export MODULE_OTHER_LIBS	:= \
	$(MODULE_ALG_MP)	\
	$(MODULE_ALG_VOLTE)
endif

ifeq ($(PLATFORM),pent0)
export MODULE_OTHER_LIBS	:= \
	$(MODULE_ALG_711_ONLY)
endif 

ifeq ($(PLATFORM),pent1)
export MODULE_OTHER_LIBS	:= \
	$(MODULE_ALG_MP)	 \
	$(MODULE_ALG_ILBC)   \
	$(MODULE_ALG_GAMRWB) 
endif

ifeq ($(PLATFORM),ac812)
export MODULE_OTHER_LIBS	:= \
	$(MODULE_ALG_MP) 	 \
	$(MODULE_ALG_GAMRWB) \
    $(MODULE_ALG_GAMRWB_MATH) \
	$(MODULE_ALG_GAMRNB)
endif

ifeq ($(PLATFORM),i8605)
export MODULE_OTHER_LIBS	:= \
	$(MODULE_ALG_MP) 	 \
	$(MODULE_ALG_VOLTE)
endif

ifeq ($(PLATFORM),ac908)
export MODULE_OTHER_LIBS	:= \
	$(MODULE_ALG_MP_LITE)
endif

ifeq ($(PLATFORM),a1117)
export MODULE_OTHER_LIBS	:= \
	$(MODULE_ALG_MP) 	 \
	$(MODULE_ALG_VOLTE)
endif

ifeq ($(PLATFORM),m34k7)
export MODULE_OTHER_LIBS	:= \
	$(MODULE_ALG_FXS) 	 \
	$(MODULE_ALG_G729)
endif

ifeq ($(PLATFORM),ac501)
export MODULE_OTHER_LIBS	:= \
	$(MODULE_ALG_MP_LITE)
endif

ifeq ($(PLATFORM),ac502)
export MODULE_OTHER_LIBS	:= \
	$(MODULE_ALG_MP_LITE)
endif

ifeq ($(PLATFORM),m1004kc1)
export MODULE_OTHER_LIBS	:= \
	$(MODULE_ALG_FXS) 	 \
	$(MODULE_ALG_G729) \
	$(MODULE_ALG_G726) \
	$(MODULE_ALG_T38)
endif
# END OF MAKEFILE

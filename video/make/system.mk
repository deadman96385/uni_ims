#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004-2006 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 28988 $ $Date: 2014-09-24 10:26:57 +0800 (Wed, 24 Sep 2014) $
#

#
# This file is for user-defined software configuration
# 
# -------- You may modify this file to include customizations
#

ifndef BASE_DIR 
# Must use cygpath on windows to allow cross compile builds, 
#   because some chip vendor tools only support
#   c:/some/path type filenames.
export BASE_DIR    :=  $(shell if [ -x "c:/cygwin/bin/cygpath" ] ; \
			then "c:/cygwin/bin/cygpath" --mixed -a "$(CURDIR)" ;\
			fi )
ifeq ($(BASE_DIR),)
export BASE_DIR	   := $(CURDIR)
endif
endif

export TOOL_DIR    			:= $(BASE_DIR)/tools

# -------- Assign platform below.
#
#  If PLATFORM is not defined or commented out below, 
#  this variable can also be passed in from the environment, for example,
#    $ PLATFORM=m4kc1 make
#  or 
#    $ PLATFORM=m4kc1; export PLATFORM
#    $ make
#

ifndef PRODUCT
$(error !! PRODUCT must be defined in the environment.)
endif
ifeq ($(PRODUCT),)
$(error !! PRODUCT must be defined in the environment.)
endif

ifndef PROVIDER
export PROVIDER := PROVIDER_GENERIC
endif

# The following system-PRODUCT.mk defines the MODULES and flags to build
include $(BASE_DIR)/make/system-$(PRODUCT).mk

ifndef PLATFORM
$(error !! PLATFORM must be defined in the environment.)
endif
ifeq ($(PLATFORM),)
$(error !! PLATFORM must be defined in the environment.)
endif

# NOTE: Define VPORT_OS in tools/def-PLATFORM.mk

export SYSTEM_INCLUDE_DIRS += \
		$(BASE_DIR)/system/$(PLATFORM)/$(VPORT_OS)

# The following system-PLATFORM.mk defines the MODULES to build.
include $(BASE_DIR)/make/system-$(PLATFORM).mk

# Include other tool make defs
include $(TOOL_DIR)/def-default.mk
include $(TOOL_DIR)/def-$(PLATFORM).mk

export SVN_REV := $(shell svnversion -n)

export CUR_REL_TAG=VPORT_4G_R_11 $(SVN_REV)
export OSAL_KERNEL_REL_TAG=$(CUR_REL_TAG)
export OSAL_USER_REL_TAG=$(CUR_REL_TAG)
export OSAL_INCLUDE_REL_TAG=$(CUR_REL_TAG)
export EXTERN_REL_TAG=$(CUR_REL_TAG)
export RTP_REL_TAG=$(CUR_REL_TAG)
export PROFILER_REL_TAG=$(CUR_REL_TAG)
export PHC_REL_TAG=$(CUR_REL_TAG)
export VTSP_REL_TAG=$(CUR_REL_TAG)
export VTSP_HW_REL_TAG=$(CUR_REL_TAG)
export VTSP_HW_UT_REL_TAG=$(CUR_REL_TAG)
export VTSP_RT_REL_TAG=$(CUR_REL_TAG)
export SIPP_REL_TAG=$(CUR_REL_TAG)
export VTSP_UT_REL_TAG=$(CUR_REL_TAG)
export VPORT_REL_TAG=$(CUR_REL_TAG)
export ISI_REL_TAG=$(CUR_REL_TAG)
export ISI_JNI_REL_TAG=$(CUR_REL_TAG)
export ISI_UT_REL_TAG=$(CUR_REL_TAG)
export VCCT_REL_TAG=$(CUR_REL_TAG)
export AUDIO_JNI_REL_TAG=$(CUR_REL_TAG)
export MCM_REL_TAG=$(CUR_REL_TAG)
export VIDEO_REL_TAG=$(CUR_REL_TAG)
export IRIL_REL_TAG=$(CUR_REL_TAG)
export SIT_REL_TAG=$(CUR_REL_TAG)
export GAPP_REL_TAG=$(CUR_REL_TAG)
export MC_REL_TAG=$(CUR_REL_TAG)
export VCDN_REL_TAG=$(CUR_REL_TAG)
export VDDN_REL_TAG=$(CUR_REL_TAG)
export SAPP_REL_TAG=$(CUR_REL_TAG)
export SKAPP_REL_TAG=$(CUR_REL_TAG)
export XAPP_REL_TAG=$(CUR_REL_TAG)
export YAPP_REL_TAG=$(CUR_REL_TAG)
export MAPP_REL_TAG=$(CUR_REL_TAG)
export HTTP_REL_TAG=$(CUR_REL_TAG)
export XCAP_REL_TAG=$(CUR_REL_TAG)
export PDU_REL_TAG=$(CUR_REL_TAG)
export RPM_REL_TAG=$(CUR_REL_TAG)
export RIR_REL_TAG=$(CUR_REL_TAG)
export MCE_REL_TAG=$(CUR_REL_TAG)
export CSM_REL_TAG=$(CUR_REL_TAG)
export CSM_UT_CLI_REL_TAG=$(CUR_REL_TAG)
export CSM_UT_TAPP_REL_TAG=$(CUR_REL_TAG)

export VPR_INCLUDE_REL_TAG=$(CUR_REL_TAG)
export VPR_REL_TAG=$(CUR_REL_TAG)
export VOER_REL_TAG=$(CUR_REL_TAG)
export VIER_REL_TAG=$(CUR_REL_TAG)
export MODEM_DRVR_VPMD_REL_TAG=$(CUR_REL_TAG)
export MODEM_DRVR_VPAD_REL_TAG=$(CUR_REL_TAG)
export GBA_REL_TAG=$(CUR_REL_TAG)
export ISI_XDR_REL_TAG=$(CUR_REL_TAG)
export ISI_CLIENT_REL_TAG=$(CUR_REL_TAG)
export ISI_SERVER_REL_TAG=$(CUR_REL_TAG)

export EXTERN_CURL_REL_TAG=$(CUR_REL_TAG)
export EXTERN_CURL_SRC_REL_TAG=$(CUR_REL_TAG)

# Check that a valid platform is now configured
ifndef CC
$(error !! PLATFORM is not properly defined.  See make/system.mk)
endif

# Check that valid MODULES build is now configured
ifndef MODULES
$(error !! MODULES is not properly defined.  See make/system.mk)
endif

# END OF MAKEFILE


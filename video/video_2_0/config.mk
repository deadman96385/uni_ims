# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
#

#
# This file is for module level configuration
#
# -------- You may modify this file to include customizations
#

#
# List dirs in order for making
		

MY_MODULE_OTHER_LIBS := \
		libosal_user_bionic \
		libextern \
		libve_rtp_public

MY_CFLAGS := \
	-DOSAL_PTHREADS \
	-DANDROIDi_ICS

MY_MODULE_SUB_DIRS := \
		coder/common \
		common \
		jbv \


#ifeq (${MY_PLATFORM},ac702)
MY_MODULE_SUB_DIRS += \
	vc \
	os/android/jb

ifeq ($(MY_4G_PLUS),y)
MY_MODULE_OTHER_LIBS += \
	libvier
endif

ifeq ($(MY_VPAD),y)
MY_MODULE_OTHER_LIBS += \
	libmodemDriver_vpad
endif

MY_INCLUDE_LIBS := \
	libvideo_2_0_common \
	libvideo_2_0_jbv \
	libvideo_2_0_coder_common \
	libvideo_2_0_vc \
	libvideo_2_0_os
#endif

# END OF MAKEFILE

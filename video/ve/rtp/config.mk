# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004-2006 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 18860 $ $Date: 2012-11-20 06:36:40 +0800 (Tue, 20 Nov 2012) $
#

#
# This file is for module level configuration
# 
# -------- You may modify this file to include customizations
#

# List dirs in order for making
ifeq ($(MY_SRTP),y)
MY_MODULE_SUB_DIRS := \
		rtp_private/include \
        rtp_private/ae_xfm \
        rtp_private/cipher \
		rtp_private/hash \
        rtp_private/kernel \
        rtp_private/math \
        rtp_private/replay \
        rtp_private/rng \
        rtp_private/tables \
        rtp_private/srtp \
        rtp_public
else
MY_MODULE_SUB_DIRS := \
        rtp_public
endif

MY_CFLAGS := \
	-DOSAL_PTHREADS \
	-DANDROID_ICS

# END OF MAKEFILE

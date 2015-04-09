#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004-2006 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 5185 $ $Date: 2008-01-21 18:09:25 -0500 (Mon, 21 Jan 2008) $
#

#
# This file is for user-defined software configuration
# 
# -------- You may modify this file to include customizations
#

# This file defines product specific modules and flags

# Define this to include CSM in build
# For malibu in application processor, CSM is not required.
export MY_CSM = n

# Define this to include SUPSRV shared library in build
export MY_SUPSRV = n

# Define this to include MC in build. No need MC in Malibu.
export MY_MC = n

# Define this to include vPort 4G+ in build
# 4G_PLUS should be defined Malibu
export MY_4G_PLUS = y

# Define this to include MGT in build. No need in Malibu.
export MY_MGT = n

# Define this to include isim in build. No need in Malibu.
export MY_ISIM = n

# Define this to include gapp/at in build. No need in Malibu.
export MY_GAPP = n

# Define this to include VPR in build
export MY_VPR = n

# Define this to include VIER in build
export MY_VIER = y

# Define this to include VOER in build
export MY_VOER = n

# Define this to include VPAD in build
export MY_VPAD = y

# Define this to include VPMD in build
export MY_VPMD = n

# Define this to include ISI server.
# It's required for Seattle
export MY_ISI_SERVER = n

# Define this to include rpm in build. It's not required in Malibu.
export MY_RPM = n

# Define this to include rir in build. It's required in Malibu.
export MY_RIR = y

# Define this to use message queue between ISI server and client.
# Must set it to n for Malibu
export MY_ISI_RPC_MSG_Q = n

# END OF MAKEFILE


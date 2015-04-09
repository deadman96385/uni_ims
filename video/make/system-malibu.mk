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
export CSM = n

# Define this to include SUPSRV shared library in build
export SUPSRV = n

# Define this to include MC in build. No need MC in Malibu.
export MC = n

# Define this to include vPort 4G+ in build
# 4G_PLUS should be defined Malibu
export 4G_PLUS = y

# Define this to include MGT in build. No need in Malibu.
export MGT = n

# Define this to include isim in build. No need in Malibu.
export ISIM = n

# Define this to include gapp/at in build. No need in Malibu.
export GAPP = n

# Define this to include VPR in build
export VPR = n

# Define this to include VIER in build
export VIER = y

# Define this to include VOER in build
export VOER = n

# Define this to include VPAD in build
export VPAD = y

# Define this to include VPMD in build
export VPMD = n

# Define this to include ISI server.
# It's required for Seattle
export ISI_SERVER = n

# Define this to include rpm in build. It's not required in Malibu.
export RPM = n

# Define this to include rir in build. It's required in Malibu.
export RIR = y

# Define this to use message queue between ISI server and client.
# Must set it to n for Malibu
export ISI_RPC_MSG_Q = n

# END OF MAKEFILE


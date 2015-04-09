#!/bin/make
#
#  THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
#  AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
#  APPLIES:
#  "COPYRIGHT 2006 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 8688 $ $Date: 2009-01-10 09:34:42 +0800 (Sat, 10 Jan 2009) $
#

# C Source files
CSRC    = 	\
		utmem.c  \
		utsem.c  \
		utmsg.c  \
		uttimer.c  \
		uttask.c  \
		utstresstask.c  \
        utdns.c \
        utaton.c \
        utipsec.c \
        utcrypto.c \
        utfile.c \
        utfifo.c \
        _utlatch.c \
        utnet.c



# Assembly files
SSRC    = \

# Private Header files
PRIVATE_HEADERS = \

# Files to export to INCLUDE_DIR
PUBLIC_HEADERS  =  \
		osal_ut.h \

OUTPUT        = archive

include $(TOOL_DIR)/rules.mk

# Build Rule - add custom build commands below this rule
build: default_build

# Clean Rule - add custom clean commands below this rule
clean: default_clean


# END OF MAKEFILE


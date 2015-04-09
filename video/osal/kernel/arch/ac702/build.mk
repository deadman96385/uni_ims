#!/bin/make
#
#  THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
#  AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
#  APPLIES:  "COPYRIGHT 2005-2006 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 9022 $ $Date: 2009-02-27 19:25:54 -0600 (Fri, 27 Feb 2009) $
#

# C Source files
CSRC    = \
			osal_arch_count.c \
#			osal_arch_irq.c 

# Assembly files
SSRC    = \

# Private Header files
PRIVATE_HEADERS = \

# Files to export to INCLUDE_DIR
PUBLIC_HEADERS  =  \
			osal_types.h \
			osal_platform.h

OUTPUT        = archive

include $(TOOL_DIR)/rules.mk

# Build Rule - add custom build commands below this rule
build: default_build

# Clean Rule - add custom clean commands below this rule
clean: default_clean


# END OF MAKEFILE


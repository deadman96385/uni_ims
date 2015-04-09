#!/bin/make
#
#  THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
#  AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
#  APPLIES:
#  "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 20936 $ $Date: 2013-06-05 11:47:54 +0800 (Wed, 05 Jun 2013) $
#

# C Source files
CSRC    = \

# Assembly files
SSRC    = \

# Private Header files
PRIVATE_HEADERS = \

# Files to export to INCLUDE_DIR
PUBLIC_HEADERS  =  \
		osal.h \
		osal_constant.h \
		osal_log.h \
		osal_arch.h \
		osal_mem.h \
		osal_msg.h \
		osal_net.h \
		osal_random.h \
		osal_select.h \
		osal_sem.h \
		osal_sys.h \
		osal_string.h \
		osal_task.h \
		osal_cond.h \
		osal_tmr.h \
		osal_time.h \
		osal_file.h \
		osal_ipsec.h \
		osal_crypto.h

OUTPUT        = archive

include $(TOOL_DIR)/rules.mk

# Build Rule - add custom build commands below this rule
build: default_build

# Clean Rule - add custom clean commands below this rule
clean: default_clean


# END OF MAKEFILE


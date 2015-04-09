#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 988 $ $Date: 2006-11-03 07:47:08 +0800 (Fri, 03 Nov 2006) $
#

#
# This file is for definitions when building from within a module,
# where specific variables are not inherited from the top.
# 

# Find name of last directory in path, assume this is module name,
# ex: "h:/some/path/name/dir" -> "dir"
MODULE_PATH_WORDS	:= $(subst /, ,$(THIS_DIR))
export MODULE		:= \
	$(word $(words $(MODULE_PATH_WORDS)),$(MODULE_PATH_WORDS))

export MODULE_DIR	:= $(THIS_DIR)

# END OF MAKEFILE


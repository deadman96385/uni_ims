#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 9625 $ $Date: 2009-05-20 21:22:27 +0800 (Wed, 20 May 2009) $
#

#
# Do not modify this Makefile
#


ifndef MODULE_DIR
$(warn MODULE_DIR not defined, assuming current directory.)
export MODULE_DIR  :=  $(shell if [ -x "c:/cygwin/bin/cygpath" ] ; \
			then "c:/cygwin/bin/cygpath" --mixed -a "$(CURDIR)" ;\
			fi )
ifeq ($(MODULE_DIR),)
export MODULE_DIR  := $(CURDIR)
endif
endif

ifndef MODULE_PREFIX
$(error !! MODULE_PREFIX must be defined in $(MODULE_DIR)/config.mk)
endif


# Revision Source/Object 
export REVINFO_SRC	=$(OBJ_DIR)/$(subst /,_,$(MODULE))_revinfo.c 
export REVINFO_OBJ	=$(OBJ_DIR)/$(subst /,_,$(MODULE))_revinfo.o 
export REL_TAG		:= $($(MODULE_PREFIX)_REL_TAG)
ifndef REL_TAG
export REL_TAG		:= untagged_release
endif

# example: 
# MODULE_SUB_ARCHIVES=/my/obj/dir/mod1_sub1.a /my/obj/dir/mod1_sub2.a 
# Also, if MODULE_SUB_DIRS contains '.', this is changed to 'dir' 
export MODULE_SUB_ARCHIVE_NAMES=$(addsuffix .a, \
						 $(addprefix $(MODULE)/,$(MODULE_SUB_DIRS)))
export MODULE_SUB_ARCHIVES=$(addprefix $(OBJ_DIR)/, \
					$(subst /,_, $(MODULE_SUB_ARCHIVE_NAMES)))


# Files which require linking with other objects go into ./lib.PLATFORM
# Files which are directly loadable to target h/w go into
#  ->  ./build.PLATFORM  for linux 2.4
#  ->  ./obj.PLATFORM    for linux 2.6
ifeq ($(VPORT_OS_VER),24)
export MODULE_LKM 		=$(BUILD_DIR)/$(subst /,_,$(MODULE)).lkm
endif
ifeq ($(VPORT_OS_VER),26)
export MODULE_LKM 		=$(OBJ_DIR)/$(subst /,_,$(MODULE)).lkm
endif
export MODULE_RLIB		=$(LIB_DIR)/$(subst /,_,$(MODULE)).lib
export MODULE_KO  		=$(BUILD_DIR)/$(subst /,_,$(MODULE)).ko
export MODULE_EXE 		=$(BUILD_DIR)/$(subst /,_,$(MODULE)).exe
export MODULE_BAREEXE 	=$(BUILD_DIR)/$(subst /,_,$(MODULE))
export MODULE_DYNAMIC 	=$(BUILD_DIR)/$(subst /,_,$(MODULE)).so
export MODULE_MAP		=$(LIB_DIR)/$(subst /,_,$(MODULE)).map

# END OF MAKEFILE


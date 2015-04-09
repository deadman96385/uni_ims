#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 21853 $ $Date: 2013-08-15 05:36:26 +0800 (Thu, 15 Aug 2013) $
#

# DO NOT MODIFY THESE RULES

# This file contains per-file rules executed in the lowest level of Make.
# i.e., this file is only included by the deepest subdirectory build.mk. 


# How things work:
#
# All intermediate files created as part of the build are put in "obj.xxx"
# directory, where xxx is the name of the platform used.  This includes
# cdepend files, target objects, intermediate archives, etc.
#
# All files created for external linking are put in "lib.xxx" directory,
# where xxx is the name of the platform used.  These ".so", ".lib",
# ".lkm" files can be linked against for incorporation into applications.
#
# All files created for external execution or as binary images are put in
# "build.xxx" directory, where xxx is the name of the platform used.  These
# ".bin", ".exe", etc files can be directly run or burned.
#
# The 'clean' target will only remove files from the currently-defined 
# platform.  If the platform is changed then prior build objects/libraries
# will remain.
#
# --- Dependencies:
#  Dependencies are generated on a per-module basis for all subdirectories
#  within the module and put in _cdepend which is included at the lowest
#  level of Make. 


# generic target (unused)
all:
	$(error !! Makefile configuration error.)

# Define intermediate file locations 
MODULE_OBJ_DIR 				= $(OBJ_DIR)/$(MODULE)
ifneq ($(MODULE_SUB),.) 
MODULE_SUB_OBJ_DIR 			= $(OBJ_DIR)/$(MODULE)/$(MODULE_SUB)
else
MODULE_SUB_OBJ_DIR 			= $(OBJ_DIR)/$(MODULE)_dir
endif

EXPORT_HEADERS 				= $(MODULE_SUB_OBJ_DIR)/_exporth
CDEPEND 					= $(MODULE_SUB_OBJ_DIR)/_cdepend
HSRC						= $(PUBLIC_HEADERS) $(PRIVATE_HEADERS)

OBJS						= $(addprefix $(MODULE_SUB_OBJ_DIR)/,\
								$(CSRC:.c=.o) $(SSRC:.s=.o) $(CXXSRC:.cpp=.o))
								
export MODULE_ARCHIVE 		= $(LIB_DIR)/$(MODULE).a
ifneq ($(MODULE_SUB),.) 
export MODULE_SUB_ARCHIVE 	= $(OBJ_DIR)/$(subst /,_,$(MODULE)/$(MODULE_SUB).a)
else
export MODULE_SUB_ARCHIVE 	= $(OBJ_DIR)/$(subst /,_,$(MODULE)/$(MODULE).a)
endif
ifeq ($(OBJS),)
# No Objects in module (maybe it is includes only), so no archive for the objects
#export MODULE_SUB_ARCHIVE 	=
endif

# Add the module specific defines to CFLAGS
ifeq ($(MODULE_COMPILE_OPTS),Mips32toMips2)
MODULE_CFLAGS += -DMODULE_SUB=\"$(MODULE_SUB)\" \
				$(addprefix -I,$(MODULE_OTHER_INCLUDE_DIRS)) \
				$(addprefix -I,$(INCLUDE_DIR)) \
				$(addprefix -I,$(SYSTEM_INCLUDE_DIRS))
else
MODULE_CFLAGS += -DMODULE_SUB=\"$(MODULE_SUB)\" \
				$(addprefix -I,$(MODULE_OTHER_INCLUDE_DIRS)) \
				$(addprefix -I,$(SYSTEM_INCLUDE_DIRS))
endif

# The following flags are GNU gcc for dependency generation
#  -w inhibit warnings 
export CDEPEND_CFLAGS = -w \
					-DMODULE_SUB=\"$(MODULE_SUB)\" \
					$(addprefix -I,$(INCLUDE_DIR)) \
				 	$(addprefix -I,$(MODULE_DIR) $(MODULE_SUB_DIR)) \
					$(addprefix -I,$(OS_INCLUDE_DIRS)) \
					$(addprefix -I,$(SYSTEM_INCLUDE_DIRS)) \
				 	$(addprefix -I,$(MODULE_OTHER_INCLUDE_DIRS)) \
				 	-pass-exit-codes \
					$(CCDEFINES) \
					$(MODULE_CDEFINES)


ALL_CFLAGS += $(MODULE_CDEFINES) $(MODULE_CFLAGS)


# Target to generate dependencies for objects in OBJ_DIR from CSRC
# OBJ_DIR is parsed to a different tree than the source tree 
# This verifies all sources and header files (public & private) exist. 
#
# Modifying the system or module makefiles run dependency generation, which
# run header exportation, which cause the current module and any module
# depending on this module to be re-built.
#
# --------
ifneq ($(OBJS),)
$(CDEPEND): $(CSRC) $(CXXSRC) \
#			$(MAKEFILE) \
#			$(MODULE_DIR)/Makefile \
#			$(MODULE_DIR)/config.mk \
#			$(MODULE_SUB_DIR)/build.mk \
#			$(BASE_DIR)/make/system.mk
	$(ECHO) "=== Generating Dependencies"
	@if [ ! -d $(MODULE_SUB_OBJ_DIR) ]; \
		then mkdir -p $(MODULE_SUB_OBJ_DIR); \
	fi
	for f in $(HSRC) $(CSRC) $(SSRC) $(CXXSRC); do \
		if [ ! -f $$f ] ; then \
			echo "Can't find $$f specified in $(MODULE_SUB_DIR)/build.mk"; \
			exit 1; \
		fi ;\
	done
	$(RM) $(EXPORT_HEADERS)
	$(RM) $(CDEPEND)
	@if [ -n "$(CSRC)" ] ; \
		then $(GCC) $(CDEPEND_CFLAGS) -MM -MG -MP $(CSRC) | \
			perl -p -e 's!^(.*\.o)(:.*)!$(MODULE_SUB_OBJ_DIR)/$$1$$2!g' \
			>>$(CDEPEND) ; \
	fi
	@if [ -n "$(CXXSRC)" ] ; \
		then $(CXX) $(CDEPEND_CFLAGS) -MM -MG -MP $(CXXSRC) | \
			perl -p -e 's!^(.*\.o)(:.*)!$(MODULE_SUB_OBJ_DIR)/$$1$$2!g' \
			>>$(CDEPEND) ; \
	fi
else
# No CSRC or SSRC in this module.  No dependencies exist.
$(CDEPEND): $(CSRC) $(CXXSRC)
	@if [ ! -d $(MODULE_SUB_OBJ_DIR) ]; \
		then mkdir -p $(MODULE_SUB_OBJ_DIR); \
	fi
endif

# Assembly File Rules - specific to platform.  This file must exist.
# --------
-include $(TOOL_DIR)/rules-asm-$(PLATFORM).mk

# C Source Rules
# --------
#
$(addprefix $(MODULE_SUB_OBJ_DIR)/,%.o): $(addprefix $(MODULE_SUB_DIR)/,%.c)
ifeq ($(MODULE_COMPILE_OPTS),Mips32toMips2)
ifdef MIPS32TO2
# The following will:
#   remove -mips2 from CFLAGS 
#   add -mips32 to cc cmd line
#   run m32to2 tool on output object
#
	$(ECHO) "=== Compiling $*.c  $(CDEBUG)"
	$(CC) $(CDEBUG) $(subst -mips2,,$(ALL_CFLAGS)) -mips32 -o $@ -c $*.c 
	$(MIPS32TO2) $@
endif
else
	$(ECHO) "=== Compiling $*.c  $(CDEBUG)"
	$(CC) $(CDEBUG) $(ALL_CFLAGS) -o $@ -c $*.c 
endif

# CXX Source Rules
# --------
#
$(addprefix $(MODULE_SUB_OBJ_DIR)/,%.o): $(addprefix $(MODULE_SUB_DIR)/,%.cpp)
ifeq ($(MODULE_COMPILE_OPTS),Mips32toMips2)
ifdef MIPS32TO2
# The following will:
#   remove -mips2 from CFLAGS 
#   add -mips32 to cc cmd line
#   run m32to2 tool on output object
#
	$(ECHO) "=== Compiling $*.cpp  $(CDEBUG)"
	$(CXX) $(CDEBUG) $(ALL_CPPFLAGS) $(subst -mips2,,$(ALL_CFLAGS)) -mips32 -o $@ -c $*.cpp
	$(MIPS32TO2) $@
endif
else
	$(ECHO) "=== Compiling $*.cpp  $(CDEBUG)"
	$(CXX) $(CDEBUG) $(ALL_CPPFLAGS) $(ALL_CFLAGS) -o $@ -c $*.cpp
endif

# ASM Rules
# --------
#
$(addprefix $(MODULE_SUB_OBJ_DIR)/,%.o): $(addprefix $(MODULE_SUB_DIR)/,%.s)
	$(ECHO) "=== Compiling $*.s "
	$(AS) $(CDEBUG) $(ASM_CFLAGS) -o $@ $*.s 

# Archive Rule
# --------
$(MODULE_SUB_ARCHIVE): $(OBJS)
ifneq ($(OBJS),)
	$(ECHO) "=== Archiving Objects"
endif
	mkdir -p $(dir $@)
ifneq ($(OBJS),)
	$(AR) -rus \
			$@ \
			$^
endif
ifeq ($(OBJS),)
# If no objects, make a dummy archive
	$(ECHO) "/* dummy object */" >$@.dummy.c 
	$(CC) $(CDEBUG) $(ALL_CFLAGS) -o $@.dummy.o -c $@.dummy.c 
	$(AR) -rus \
			$@ \
			$@.dummy.o
endif


# 
# --------
%.exe: %.o
	$(ECHO) "Building Executable $@"
	$(CC) $(CDEBUG) $(ALL_CFLAGS) $^ -o $(addprefix $(LIB_DIR)/,$@)

# Export Header Rule
# --------
$(EXPORT_HEADERS): $(PUBLIC_HEADERS)
ifneq ($(PUBLIC_CURL_HEADERS),)
	$(ECHO) === Exporting Curl Headers
	mkdir $(INCLUDE_DIR)/curl
	$(CP) $(PUBLIC_CURL_HEADERS) $(INCLUDE_DIR)/curl/
endif
	$(TOUCH) $(EXPORT_HEADERS)
ifneq ($(PUBLIC_HEADERS),)
	$(ECHO) === Exporting Headers
	$(CP) $(PUBLIC_HEADERS) $(INCLUDE_DIR)
endif
	$(TOUCH) $(EXPORT_HEADERS)


# The default clean will remove all files under the obj.xx/module directory.
# Other files are removed on a per-file basis, removing only those files 
# that it knows about.
# --------
.PHONY: default_clean 
default_clean:
	$(RM) -fr $(MODULE_OBJ_DIR)
	$(RM) $(MODULE_SUB_ARCHIVE)
	$(RM) $(MODULE_ARCHIVE)
	$(RM) $(MODULE_RLIB) $(MODULE_MAP) $(MODULE_EXE) \
				$(MODULE_LKM) $(MODULE_KO) $(MODULE_BAREEXE) $(MODULE_DYNAMIC)
ifeq ($(MODULE_OUTPUT),lkm)
# Clean up many files for Linux 2.6 KO build process
	$(RM) $(OBJ_DIR)/.$(MODULE).ko.cmd $(OBJ_DIR)/.$(MODULE).o.cmd \
				$(OBJ_DIR)/.$(MODULE).mod.o.cmd \
				$(OBJ_DIR)/$(MODULE).mod.c $(OBJ_DIR)/$(MODULE).mod.o \
				$(OBJ_DIR)/$(MODULE).ko $(OBJ_DIR)/$(MODULE).o
endif
	$(RM) $(REVINFO_OBJ) $(REVINFO_SRC) $(REVINFO_SRC:.c=.d)
ifdef PUBLIC_CURL_HEADERS
	-@$(RM) -r $(addprefix $(INCLUDE_DIR)/,curl)
endif
ifdef PUBLIC_HEADERS
	-@$(RM) $(addprefix $(INCLUDE_DIR)/,$(PUBLIC_HEADERS))
endif
ifdef TAGFILE
	-@$(RM) $(TAGFILE)
endif

# Make Tags rule
# --------
.PHONY: default_tags
default_tags:
	$(TOUCH) $(TAGFILE)
	echo $(MODULE_SUB_DIR)..
	$(TAGGEN) -a -f $(TAGFILE) --tag-relative=yes $(CSRC) $(HSRC)
	
.PHONY: default_build
default_build: $(OUTPUT) $(EXPORT_HEADERS)

.PHONY: archive
archive: $(MODULE_SUB_ARCHIVE)

.PHONY: report
report:
	$(REPORT) $(MODULE_SUB_ARCHIVE)

.PHONY: default_clean build tags default_tags

# Include C dependency file for specific make targets
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),default_clean)
ifneq ($(MAKECMDGOALS),default_tags)
-include $(CDEPEND)
endif
endif
endif


# END OF MAKEFILE


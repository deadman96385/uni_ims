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



# -------- Default rule: release
.PHONY: release 
release: dircheck $(REVINFO_OBJ) build
	$(ECHO) "===   Release: $(REL_TAG)"
	

# -------- Secondary rule: build
.PHONY: build 
build: dircheck compile $(MODULE_OUTPUT)
	$(ECHO) "=== Done creating '$(MODULE_OUTPUT)' for $(MODULE)"

.PHONY: dummy
dummy:
	$(warn No targets defined.  Cant build.)


# -------- Prerequisites
.PHONY: dircheck 
dircheck: $(INCLUDE_DIR) $(BUILD_DIR) \
		$(LIB_DIR) $(OBJ_DIR) $(MODULE_OBJ_DIR)

$(INCLUDE_DIR):
	mkdir $(INCLUDE_DIR)

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(LIB_DIR):
	mkdir $(LIB_DIR)

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

$(MODULE_OBJ_DIR): 
	mkdir $(MODULE_OBJ_DIR)

# Build standard relocatable library
.PHONY: rlib 
rlib: dircheck compile $(MODULE_RLIB)

# Build lkm
.PHONY: lkm
lkm: dircheck compile $(MODULE_LKM)

# Build standard executable for unix (no extension)
.PHONY: bare-exe 
bare-exe: dircheck compile $(MODULE_BAREEXE)

# Build .so for unix
.PHONY: dynamic
dynamic: dircheck compile $(MODULE_DYNAMIC)

# Compile all subdirectories under module and the revision object
.PHONY: compile
compile: $(REVINFO_OBJ) $(MODULE_SUB_DIRS)

include $(TOOL_DIR)/link-$(PLATFORM).mk



# Target to create revision information object
# Note: ALL_CFLAGS does not yet include MODULE_* vars at this point.
comma=,
$(REVINFO_OBJ): $(REVINFO_SRC)
	$(CC) -c $(ALL_CFLAGS) $(MODULE_CDEFINES) $(MODULE_CFLAGS) \
			-o $@  $(REVINFO_SRC)
ifeq ($(MODULE_COMPILE_OPTS),Mips32toMips2)
ifdef MIPS32TO2
# The following will:
#   remove -mips2 from CFLAGS 
#   add -mips32 to cc cmd line
#   run m32to2 tool on output object
#
	$(MIPS32TO2) $@
endif
endif

# Target to create revision information source
#
.PHONY: $(REVINFO_SRC)
$(REVINFO_SRC):
ifeq ($(REL_TAG),untagged_release)
	$(ECHO) No Tag set for build/release,  \
	set environment var $(MODULE_PREFIX)_REL_TAG to define.
endif
	$(ECHO) Generating revision-info with tag $(REL_TAG)..
	$(D2RELTAG) \
			--output=$(REVINFO_SRC) --env=REL_TAG \
			--name=$(MODULE_PREFIX)

# -------- Clean target
# This target is only valid at Module level 
# Clean all layers by recursion
# After recursion, lib and obj dirs should be empty
#  and can be rmdir'd 
ifndef MODULE_SUB 
ifdef MODULE_SUB_DIRS
.PHONY: clean 
clean:
	$(foreach module_sub_dir,$(MODULE_SUB_DIRS), \
		echo "=== Cleaning $(MODULE)/$(module_sub_dir).."; \
	   	cd $(MODULE_DIR)/$(module_sub_dir) && \
		MODULE_SUB=$(module_sub_dir) $(MAKE) -f build.mk clean; \
	)
endif
endif

# -------- Module Sub directory recursive target
# This target is only valid at Module level
ifndef MODULE_SUB 
ifdef MODULE_SUB_DIRS
.PHONY: $(MODULE_SUB_DIRS) 
$(MODULE_SUB_DIRS): 
	cd $(MODULE_DIR)/$@ && \
		echo "=== Building $@ in $(MODULE_DIR)/$@" && \
		MODULE_SUB="$@" MODULE_SUB_DIR="$(MODULE_DIR)/$@" \
				   $(MAKE) -f build.mk build; 
endif
endif


# -------- Compile Target
.PHONY: compile 
compile: $(MODULE_SUB_DIRS)

# -------- Report target
.PHONY: report 
report:
	$(foreach module_sub_dir,$(MODULE_SUB_DIRS), \
		echo "=== Report for $(MODULE)/$(module_sub_dir).."; \
	   	cd $(MODULE_DIR)/$(module_sub_dir) && \
		MODULE_SUB=$(module_sub_dir) $(MAKE) -f build.mk report; \
	)

# END OF MAKEFILE


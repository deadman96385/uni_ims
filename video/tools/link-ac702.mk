#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 21097 $ $Date: 2013-06-18 18:15:21 -0700 (Tue, 18 Jun 2013) $
#

#
# Do not modify this Makefile
#

# -------- .lib generation

$(MODULE_RLIB): \
				$(MODULE_SUB_ARCHIVES) \
				$(addprefix $(LIB_DIR)/,$(EXTERN_LIBS)) \
				$(addprefix $(MODULE_OTHER_LIB_DIR)/,$(MODULE_OTHER_LIBS))
	$(ECHO) "=== Linking $(MODULE_RLIB)"
	$(LD) -r \
		$(LDFLAGS) \
		$(addprefix -L,$(LIB_DIR)) \
		$(addprefix -L,$(OS_LIB_DIRS)) \
		$(addprefix -L,$(EXTERN_LIB_DIRS)) \
		--whole-archive \
		$(MODULE_SUB_ARCHIVES) \
		$(REVINFO_OBJ) \
		--no-whole-archive \
		$(addprefix $(LIB_DIR)/,$(EXTERN_LIBS)) \
		$(addprefix $(MODULE_OTHER_LIB_DIR)/,$(MODULE_OTHER_LIBS)) \
		-Map $(MODULE_MAP) \
		--fix-cortex-a8 \
		-o $@

# -------- .lkm generation
$(MODULE_LKM): \
				$(MODULE_SUB_ARCHIVES) \
				$(addprefix $(LIB_DIR)/,$(EXTERN_LIBS)) \
				$(addprefix $(MODULE_OTHER_LIB_DIR)/,$(MODULE_OTHER_LIBS))
	$(ECHO) "=== Linking $(MODULE_LKM)"
	$(LD) -r \
		$(LDFLAGS) \
		$(addprefix -L,$(LIB_DIR)) \
		$(addprefix -L,$(OS_LIB_DIRS)) \
		$(addprefix -L,$(EXTERN_LIB_DIRS)) \
		--whole-archive \
		$(MODULE_SUB_ARCHIVES) \
		$(REVINFO_OBJ) \
		--no-whole-archive \
		$(addprefix $(LIB_DIR)/,$(EXTERN_LIBS)) \
		$(addprefix $(MODULE_OTHER_LIB_DIR)/,$(MODULE_OTHER_LIBS)) \
		-Map $(MODULE_MAP) \
		-o $@

# -------- .exe generation
$(MODULE_BAREEXE): \
				$(MODULE_SUB_ARCHIVES) \
				$(addprefix $(LIB_DIR)/,$(EXTERN_LIBS)) \
				$(addprefix $(MODULE_OTHER_LIB_DIR)/,$(MODULE_OTHER_LIBS))
	$(ECHO) "=== Linking $(MODULE_BAREEXE)"
# Touch the map file - avoids gnu ld bug
	$(TOUCH) $(MODULE_MAP)
	$(CXX) \
			-Wl,--gc-sections -Wl,-z,nocopyreloc \
			--sysroot=$(TOOLBASE)/platforms/android-$(ANDROID_API)/arch-$(ANDROID_ARCH) \
			$(MODULE_SUB_ARCHIVES) \
            $(REVINFO_OBJ) \
			-lgcc \
			-no-canonical-prefixes \
			-Wl,--no-undefined \
			-Wl,-z,noexecstack \
			-Wl,-z,relro \
			-Wl,-z,now \
			-lc -lm \
			$(addprefix -Wl$(comma), \
				-warn-common \
				-Map $(MODULE_MAP) \
				$(addprefix -L,$(LIB_DIR)) \
				$(addprefix -L,$(OS_LIB_DIRS)) \
				$(addprefix -L,$(EXTERN_LIB_DIRS)) \
				-rpath-link \
				-t \
				$(addprefix -l,$(OS_LIBS)) \
			) \
			$(addprefix $(LIB_DIR)/,$(EXTERN_LIBS)) \
			$(addprefix $(MODULE_OTHER_LIB_DIR)/,$(MODULE_OTHER_LIBS)) \
			-o $@ 
	$(STRIP) \
			--remove-section=.note \
			--remove-section=.comment \
			$@
	$(OBJDUMP) \
			-x \
			$@
	

# -------- no extension generation (executable on unix)
$(MODULE_EXE): 
	echo "I dont know how to make .exe yet."

# -------- .so generation
$(MODULE_DYNAMIC): \
				$(MODULE_SUB_ARCHIVES) \
				$(addprefix $(LIB_DIR)/,$(EXTERN_LIBS)) \
				$(addprefix $(MODULE_OTHER_LIB_DIR)/,$(MODULE_OTHER_LIBS))
	$(ECHO) "=== Linking Dynamic $(MODULE_DYNAMIC)"
# Touch the map file - avoids gnu ld bug
	$(TOUCH) $(MODULE_MAP)
	$(CXX) \
			-nostdlib \
			-Wl,--gc-sections \
			-Wl,-shared,-Bsymbolic \
			-L$(BSP_VPORT_LIB_ROOT)/lib \
			-Wl,--whole-archive \
			$(MODULE_SUB_ARCHIVES) \
			$(REVINFO_OBJ) \
			-Wl,--no-whole-archive \
			$(addprefix $(MODULE_OTHER_LIB_DIR)/,$(MODULE_OTHER_LIBS)) \
                        -l$(BSP_VPORT_TOOL_LIB)/libgcc.a \
			-L$(OS_LIB_DIRS) \
			$(addprefix -l,$(OS_LIBS)) \
			$(addprefix -Wl$(comma), \
				-Map $(MODULE_MAP) \
			) \
			$(addprefix $(LIB_DIR)/,$(EXTERN_LIBS)) \
			-o $@ \
			-static-libgcc \
			-Wl,--no-undefined \
			-Wl,--fix-cortex-a8 
	$(STRIP) \
			--remove-section=.note \
			--remove-section=.comment \
			$@
	$(OBJDUMP) \
			-x \
			$@
	


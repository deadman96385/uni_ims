#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 30369 $ $Date: 2014-12-11 19:09:13 +0800 (Thu, 11 Dec 2014) $
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
		-melf_i386 \
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

comma=,
# -------- .exe generation
$(MODULE_BAREEXE): \
				$(MODULE_SUB_ARCHIVES) \
				$(addprefix $(LIB_DIR)/,$(EXTERN_LIBS)) \
				$(addprefix $(MODULE_OTHER_LIB_DIR)/,$(MODULE_OTHER_LIBS))
	$(ECHO) "=== Linking $(MODULE_BAREEXE)"
# Touch the map file - avoids gnu ld bug
	$(TOUCH) $(MODULE_MAP)
#			-Wl,--strip-all   <-- removed by MS
	$(CXX) \
			-m32 \
			-o $@ \
			-Wl,--whole-archive \
			$(MODULE_SUB_ARCHIVES) \
			$(REVINFO_OBJ) \
			-Wl,--no-whole-archive \
			$(addprefix $(LIB_DIR)/,$(EXTERN_LIBS)) \
			$(addprefix ,$(MODULE_OTHER_LIBS)) \
			$(addprefix -Wl$(comma), \
				$(LDFLAGS) \
				-melf_i386 \
				-warn-common \
				-Map $(MODULE_MAP) \
				-Bdynamic \
				$(addprefix -L,$(LIB_DIR)) \
				$(addprefix -L,$(OS_LIB_DIRS)) \
				$(addprefix -L,$(EXTERN_LIB_DIRS)) \
				-rpath-link \
				-t \
				$(addprefix -l,$(OS_LIBS)) \
			)
#	$(STRIP) \
#			--remove-section=.note \
#			--remove-section=.comment \
#			$@
#	$(OBJDUMP) \
#			-x \
#			$@
	

# -------- no extension generation (executable on unix)
$(MODULE_EXE): \
				$(MODULE_SUB_ARCHIVES) \
				$(addprefix $(LIB_DIR)/,$(EXTERN_LIBS)) \
				$(addprefix $(MODULE_OTHER_LIB_DIR)/,$(MODULE_OTHER_LIBS))
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
			$(LDFLAGS) \
			-nostdlib \
			-Wl,-soname,$(MODULE_DYNAMIC) \
			-Wl,--gc-sections \
			-Wl,-shared,-melf_i386,-Bsymbolic \
			-L$(BSP_VPORT_LIB_ROOT)/lib \
			-L$(BSP_VPORT_LIB_ROOT)/../system/lib \
			-Wl,--whole-archive \
			$(MODULE_SUB_ARCHIVES) \
			$(REVINFO_OBJ) \
			-Wl,--no-whole-archive \
			$(addprefix $(MODULE_OTHER_LIB_DIR)/,$(MODULE_OTHER_LIBS)) \
			$(addprefix -l,$(OS_LIBS)) \
			$(addprefix -Wl$(comma), \
				-Map $(MODULE_MAP) \
            ) \
                        $(addprefix -L,$(LIB_DIR)) \
			$(addprefix $(LIB_DIR)/,$(EXTERN_LIBS)) \
			-o $@
#			-Wl,--no-undefined			
	


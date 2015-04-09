#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 17585 $ $Date: 2012-07-03 08:32:08 +0800 $
#

#
# Do not modify this Makefile
#

# -------- .lib generation

$(MODULE_RLIB): \
				$(MODULE_SUB_ARCHIVES) \
				$(addprefix $(LIB_DIR)/,$(EXTERN_LIBS)) \
				$(addprefix $(MODULE_OTHER_LIB_DIR)/,$(MODULE_OTHER_LIBS))
	$(ECHO) "=== Linking.lib,1.$(LDFLAGS),2.$(LIB_DIR))3.$(OS_LIB_DIRS)4.$(EXTERN_LIB_DIRS)5.$(MODULE_SUB_ARCHIVES)6.$(REVINFO_OBJ)7.$(EXTERN_LIBS)8.$(MODULE_OTHER_LIBS)9.$(MODULE_MAP)  $(MODULE_RLIB) 10.$@"
	$(AR) -r $@ \
	$(MODULE_SUB_ARCHIVES) \
	$(REVINFO_OBJ) \
	$(addprefix $(LIB_DIR)/,$(EXTERN_LIBS)) \
	$(addprefix $(MODULE_OTHER_LIB_DIR)/,$(MODULE_OTHER_LIBS))

# -------- .lkm generation
$(MODULE_LKM): \
				$(MODULE_SUB_ARCHIVES) \
				$(addprefix $(LIB_DIR)/,$(EXTERN_LIBS)) \
				$(addprefix $(MODULE_OTHER_LIB_DIR)/,$(MODULE_OTHER_LIBS))
	$(ECHO) "=== Linking.lkm $(MODULE_LKM)"
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
	$(ECHO) "=== Linking ac501 lib $(MODULE_BAREEXE).a 1.$(MODULE_SUB_ARCHIVES) 2.$(EXTERN_LIBS) 3.$(MODULE_OTHER_LIBS) 4.$(REVINFO_OBJ)"
# Touch the map file - avoids gnu ld bug
	$(AR) -r $@ \
	$(MODULE_SUB_ARCHIVES) \
	$(REVINFO_OBJ) \
	$(MODULE_OTHER_LIBS) \

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
			-WL,-soname,$(MODULE_DYNAMIC) \
			-Wl,-T,$(TOOLBASE)/build/core/armelf.x \
			-Wl,--gc-sections \
			-Wl,-shared,-Bsymbolic \
			-L$(BSP_VPORT_LIB_ROOT)/lib \
			-L$(BSP_VPORT_LIB_ROOT)/../system/lib \
			-Wl,--whole-archive \
			$(MODULE_SUB_ARCHIVES) \
			$(REVINFO_OBJ) \
			-Wl,--no-whole-archive \
			$(addprefix $(LIB_DIR)/,$(EXTERN_LIBS)) \
			$(addprefix $(MODULE_OTHER_LIB_DIR)/,$(MODULE_OTHER_LIBS)) \
			$(addprefix -l,$(OS_LIBS)) \
			-o $@ \
			-Wl,--no-undefined \
			-Wl,--fix-cortex-a8 \
			$(BSP_VPORT_TOOL_ROOT)/lib/gcc/arm-linux-androideabi/4.4.3/armv7-a/libgcc.a
#	$(STRIP) \
			--remove-section=.note \
			--remove-section=.comment \
			$@
	$(OBJDUMP) \
			-x \
			$@
	


LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/pvaetest.cpp \
 	src/test_pv_author_engine_testset1.cpp \
 	src/test_pv_author_engine_testset4.cpp \
 	src/./single_core/pvaetestinput.cpp \
 	src/test_pv_author_engine_testset5.cpp \
 	src/test_pv_author_engine_testset6.cpp \
 	src/test_pv_author_engine_testset7.cpp \
 	src/test_pv_mediainput_author_engine.cpp \
 	src/test_pv_author_engine_testset8.cpp


LOCAL_MODULE := test_pvauthorengine
LOCAL_MODULE_TAGS := optional


LOCAL_CFLAGS := -D_IMOTION_SPECIFIC_UT_DISABLE -D_UNICODE -DUNICODE  $(PV_CFLAGS)


LOCAL_STATIC_LIBRARIES := libunit_test_utils libunit_test libpvmioaviwavfileinput libpvavifileparser   

LOCAL_SHARED_LIBRARIES :=     libopencore_author libopencore_common libopencore_player

LOCAL_C_INCLUDES := \
	$(PV_TOP)/engines/author/test/src \
 	$(PV_TOP)/engines/author/test/src \
 	$(PV_TOP)/engines/common/include \
 	$(PV_TOP)/engines/author/test/src/single_core \
 	$(PV_TOP)/engines/author/test/config/android \
 	$(PV_TOP)/pvmi/pvmf/include \
 	$(PV_TOP)/nodes/common/include \
 	$(PV_TOP)/extern_libs_v2/khronos/openmax/include \
 	$(PV_TOP)/fileformats/mp4/parser/include \
 	$(PV_TOP)/fileformats/mp4/parser/config/opencore \
 	$(PV_TOP)/fileformats/mp4/parser/utils/mp4recognizer/include \
 	$(PV_TOP)/pvmi/content_policy_manager/include \
 	$(PV_TOP)/pvmi/content_policy_manager/plugins/common/include \
 	$(PV_TOP)/fileformats/id3parcom/include \
 	$(PV_INCLUDES)

LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)

LOCAL_COPY_HEADERS := \
 	

-include $(PV_TOP)/Android_system_extras.mk

include $(BUILD_EXECUTABLE)

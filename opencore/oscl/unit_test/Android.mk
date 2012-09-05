LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/test_case.cpp \
 	src/test_problem.cpp \
 	src/test_result.cpp \
 	src/text_test_interpreter.cpp \
 	src/xml_test_interpreter.cpp \
 	src/unit_test_main.cpp \
 	src/unit_test_args.cpp \
 	src/unit_test_xml_writer.cpp


LOCAL_MODULE := libunit_test
LOCAL_MODULE_TAGS := optional


LOCAL_CFLAGS :=   $(PV_CFLAGS)


LOCAL_STATIC_LIBRARIES := 

LOCAL_SHARED_LIBRARIES := 

LOCAL_C_INCLUDES := \
	$(PV_TOP)/oscl/unit_test/src \
 	$(PV_TOP)/oscl/unit_test/src \
 	$(PV_INCLUDES)

LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)

LOCAL_COPY_HEADERS := \
	src/stringable.h \
 	src/test_case.h \
 	src/test_problem.h \
 	src/test_result.h \
 	src/text_test_interpreter.h \
 	src/xml_test_interpreter.h \
 	src/unit_test_common.h \
 	src/unit_test_local_string.h \
 	src/unit_test_args.h \
 	src/unit_test_vector.h \
 	src/unit_test_xml_writer.h

include $(BUILD_STATIC_LIBRARY)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/pv_2way_datapath.cpp \
 	src/pv_2way_engine.cpp \
 	src/pv_2way_engine_test_ext.cpp \
 	src/pv_2way_data_channel_datapath.cpp \
 	src/pv_2way_cmd_control_datapath.cpp \
 	src/pv_2way_dec_data_channel_datapath.cpp \
 	src/pv_2way_enc_data_channel_datapath.cpp \
 	src/pv_2way_mux_datapath.cpp \
 	src/pv_2way_engine_factory.cpp \
 	src/pv_2way_proxy_adapter.cpp \
 	src/pv_2way_proxy_factory.cpp


LOCAL_MODULE := libpv2wayengine
LOCAL_MODULE_TAGS := optional


LOCAL_CFLAGS := -DSIP_VOIP_PROJECT=1 -DPV2WAY_USE_OMX  $(PV_CFLAGS)


LOCAL_STATIC_LIBRARIES := 

LOCAL_SHARED_LIBRARIES := 

LOCAL_C_INCLUDES := \
	$(PV_TOP)/engines/2way/src \
 	$(PV_TOP)/engines/2way/include \
 	$(PV_TOP)/engines/2way/src \
 	$(PV_TOP)/engines/2way/include \
 	$(PV_TOP)/engines/common/include \
 	$(PV_TOP)/protocols/systems/3g-324m_pvterminal/common/include \
 	$(PV_TOP)/protocols/systems/common/include \
 	$(PV_TOP)/protocols/systems/tools/general/common/include \
 	$(PV_TOP)/extern_libs_v2/khronos/openmax/include \
 	$(PV_INCLUDES)

LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)

LOCAL_COPY_HEADERS := \
	include/pv_2way_interface.h \
 	include/pv_2way_engine_factory.h \
 	include/pv_2way_test_extension_interface.h \
 	include/pv_2way_proxy_factory.h

include $(BUILD_STATIC_LIBRARY)

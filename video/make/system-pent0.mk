#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004-2006 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 5185 $ $Date: 2008-01-21 18:09:25 -0500 (Mon, 21 Jan 2008) $
#

#
# This file is for user-defined software configuration
# 
# -------- You may modify this file to include customizations
#

# Define the number of interfaces for Voice App
SYSTEM_CFLAGS += -DHARDWARE_NUM_INFC_1

# default provider is GENERIC
# export PROVIDER=PROVIDER_GENERIC

# -------- Select Hardware here
export HARDWARE	:= alsa

# Service Provider
#export PROVIDER = PROVIDER_LGUPLUS

# Extern library option flag, define this to include ezxml library
export EXTERN_ENABLE_EZXML = y

# Extern library option flags, define this to include AUTH library
export EXTERN_ENABLE_AUTH = y

# Extern library option flags, define this to include the milenage algorithm
export EXTERN_ENABLE_MILENAGE = y

# C lexer parser, define to use it, required for XCAP
export EXTERN_ENABLE_HTTP_PARSE = y

# PDU convertor for PDU SMS messages
export EXTERN_ENABLE_PDUCONV = y

# For input, what console device to use
SYSTEM_CFLAGS += -DCONSOLE="\"/dev/tty\""

# SSL library flag, define this to link with SSL library
SYSTEM_CFLAGS += -DOSAL_NET_ENABLE_SSL

# OSAL_NET option flags
SYSTEM_CFLAGS += -DOSAL_NET_ENABLE_DNS

# Enable ipsec support
SYSTEM_CFLAGS += -DOSAL_IPSEC_ENABLE

# Enable OSAL_CRYPTO support
SYSTEM_CFLAGS += -DOSAL_CRYPTO_ENABLE

#
# This enables running vTSP from user space
#
SYSTEM_CFLAGS += -DOSAL_KERNEL_EMULATION

# Define this to include MC in build
export MC = y

# Define this to include SIP in build
export SIP = y

# Define this to include SAPP in build
export SAPP = y

# Define this to include SIP SIMPLE in build
export SIMPLE = n

# Define this to include vTSP in build
export VTSP = y

# Define this to include vTSP in build
export VTSP_UT = n

# Define this to include video in build
export VIDEO = n

# Define this to include rpm in build
export RPM = y

# Define this to include rir in build
export RIR = y

# Define this to include gapp/at in build
export GAPP = y

# Define this to include gapp/proxy in build
export GAPP_PROXY = y

# Define this to include gapp/gsm in build
export GAPP_GSM = y

# Define this to include isim in build
export ISIM = y

# Define this to include MGT in build.
export MGT = n

# Define this to include CSM in build
export CSM = y

# Define this to include TAPP in build
# XXX Only define TAPP for running TAPP.
# TAPP requires 4G_PLUS and 4G_PLUS_RCS
# Define TAPP causes malfunction in SAPP
export TAPP = n

# Define this to include osal ut in build
export OSAL_UT = n

# Define this to include SAPP by CSM in build
export CSM_SAPP = y

# Define this to include SRTP in build
export SRTP = n

# Define this to include SIP shared library in build
export SIP = y

# Define this to include STUN/ICE/TURN shared library in build
export SIT = n

# Define this to include HTTP shared library in build
export HTTP = y

# Define this to include XCAP shared library in build
export XCAP = y

# Define this to include SUPSRV shared library in build
export SUPSRV = y

# Define this to include PDU shared library in build
export PDU = y

# Define this to include SETTINGS shared library in build
# Always enable it for CSM and SAPP
export SETTINGS = y

# Define this to include vPort 4G+ in build
export 4G_PLUS = y

# Define this to include vPort 4G+ modemDriver testing in build
export 4G_PLUS_MODEM_TEST = n

# Define this to have vPort 4G+ support RCS features
export 4G_PLUS_RCS = y

# Define this to have vPort 4G+ support wifi-offload features
export 4G_PLUS_WIFI_OFFLOAD = y

# Define this to include VPR in build, it's required in modem for 4G+
ifeq ($(4G_PLUS),y)
export VPR = y
endif

# Define this to include SR in build
# SR needs to be included for RCS or wifi-offload
ifeq ($(4G_PLUS_RCS),y)
export SR = y
else
ifeq ($(4G_PLUS_WIFI_OFFLOAD),y)
export SR = y
endif
endif

# Define this to include VIER in build
export VIER = n

# Define this to include VOER in build
export VOER = n

# Define this to include VPMD in build
export VPMD = y

# Define this to include VPAD in build
export VPAD = n

# Define this to include GBA/GAA in build
export GBA = n

# Define this to use message queue between ISI server and client.
# Must set it to n for build in modem processor
export ISI_RPC_MSG_Q = n

# Define this to include VAPP in build
export VAPP = n

# Define this to include Modem Management in build
export MM = n

#
# Define this for SIP and its log
#
ifeq ($(SAPP),y)
SYSTEM_CFLAGS += -DINCLUDE_SAPP
# Define this for debug logging in the protocol
SYSTEM_CFLAGS += -DSIP_DEBUG_LOG
# Define this for debug logging in the SAPP application
SYSTEM_CFLAGS += -DSAPP_DEBUG
SYSTEM_CFLAGS += -DMNS_DEBUG
ifeq ($(SIMPLE),y)
SYSTEM_CFLAGS += -DINCLUDE_SIMPLE
SYSTEM_CFLAGS += -DHTTP_DBG_LOG
#SYSTEM_CFLAGS += -DMSRP_DBG_LOG
SYSTEM_CFLAGS += -DINCLUDE_XCAP
#SYSTEM_CFLAGS += -DXCAP_DBG_LOG
endif
endif

ifeq ($(4G_PLUS),y)
SYSTEM_CFLAGS += -DINCLUDE_4G_PLUS
# Define this for debug logging in VIER
#SYSTEM_CFLAGS += -DVIER_DEBUG 
# Define this for debug logging in VOER
#SYSTEM_CFLAGS += -DVOER_DEBUG 
# Define this for debug logging in VPR
#SYSTEM_CFLAGS += -DVPR_DEBUG 
# Define this for debug logging in ISI RPC
SYSTEM_CFLAGS += -DISI_RPC_DEBUG
# Define this for debug logging in VPMD IO driver
#SYSTEM_CFLAGS += -DVPMD_IO_DEBUG
# Define this for debug logging in VPMD MUX
#SYSTEM_CFLAGS += -DVPMD_MUX_DEBUG
endif

ifeq ($(4G_PLUS_MODEM_TEST),y)
SYSTEM_CFLAGS += -DVP4G_PLUS_MODEM_TEST
endif

ifeq ($(4G_PLUS_RCS),y)
SYSTEM_CFLAGS += -DINCLUDE_4G_PLUS_RCS
endif

ifeq ($(VPR),y)
SYSTEM_CFLAGS += -DINCLUDE_VPR
endif

ifeq ($(VIER),y)
SYSTEM_CFLAGS += -DINCLUDE_VIER
endif

ifeq ($(VOER),y)
SYSTEM_CFLAGS += -DINCLUDE_VOER
endif

#
# Define this for vTSP and its log
#
ifeq ($(VTSP),y)
SYSTEM_CFLAGS += -DINCLUDE_VTSP
# Define this for debug log
SYSTEM_CFLAGS += -DVTSP_ENABLE_TRACE
# Define this for debug logging in the VE application
#SYSTEM_CFLAGS += -DMC_DEBUG
# Define this if the mCUE should handle the ringtones NOT vTSP
SYSTEM_CFLAGS += -DMC_NO_RING
endif

ifeq ($(VTSP_UT),y)
# No need SR for VTSP_UT
export SR = n
# Define this for running vtsp_ut in non-interactive way
# EX: ./vtsp_ut v2
#     The second parmaeter is the fix command
#SYSTEM_CFLAGS += -DVTSP_UT_FIXED_CMD
endif

ifeq ($(VAPP),y)
# VAPP Configuration file and PID file
SYSTEM_CFLAGS += -DVAPP_CFG_PROFILE_TMP_PATHNAME=\"/tmp/d2/vport_cfg.tmp\"
SYSTEM_CFLAGS += -DVAPP_CFG_PROFILE_XML_PATHNAME=\"/tmp/d2/vport_cfg.xml\"
SYSTEM_CFLAGS += -DVAPP_CFG_PID_PATHNAME=\"/tmp/d2/vapp_cfg.pid\"
SYSTEM_CFLAGS += -DVAPP_CFG_PORT_PATHNAME=\"/tmp/d2/vapp_cfg.port\"
SYSTEM_CFLAGS += -DVAPP_EVT_NOTIFY_IPC_NAME=\"vapp-mgmt-event-notify\"

# Define the following to start Voice App at system startup
SYSTEM_CFLAGS += -DVAPP_MAIN

# Define the following to use the vApp GUI and allow runtime reconfiguration
SYSTEM_CFLAGS += -DVAPP_MGMT
SYSTEM_CFLAGS += -DVAPP_MGMT_WORKAROUND
SYSTEM_CFLAGS += -DVAPP_MGMT_PRINT_TO_CONSOLE
SYSTEM_CFLAGS += -DVAPP_SIP_DEBUG_LOG_TO_CONSOLE
SYSTEM_CFLAGS += -DSIP_DEBUG_LOG
# Define the following to use the vApp mgmt event nofity
#SYSTEM_CFLAGS += -DVAPP_MGMT_EVENT

# Define the following to use the vApp mgmt config
#SYSTEM_CFLAGS += -DVAPP_MGMTCFG
#export VAPP_MGMTCFG := y

# Define the following to use the vApp test.
#SYSTEM_CFLAGS += -DVAPP_TEST
#export VAPP_TEST := y


# Define the following to use Fax/Modem tone detector during calls
# (Disabling this will eliminate certain call features while talking)
#SYSTEM_CFLAGS += -DVAPP_ENABLE_FMTD_DURING_CALLS

# Define the following to use DTMF tone detector during calls
# (Disabling this will eliminate certain call features while talking)
#SYSTEM_CFLAGS += -DVAPP_ENABLE_DTMF_DURING_CALLS
endif

#
# Define this for MC and its log
#
ifeq ($(MC),y)
SYSTEM_CFLAGS += -DINCLUDE_MC
# Define this for debug logging in the MC
#SYSTEM_CFLAGS += -DMC_DEBUG
endif

#
# Define this for GSM_AT and its log
#
ifeq ($(GAPP),y)
SYSTEM_CFLAGS += -DINCLUDE_GAPP
# Define this for debug logging in the GAPP application
SYSTEM_CFLAGS += -DGAPPD_DEBUG
SYSTEM_CFLAGS += -DPRXY_DEBUG
SYSTEM_CFLAGS += -DGSM_DEBUG
# Define this to disable supplementary service at command filtering
#SYSTEM_CFLAGS += -DGAPP_DISABLE_SUPPLEMENTARY_SERVICE_AT_CMD
# Define the gsm configuration file path
SYSTEM_CFLAGS += -DGSM_CONFIG_DEFAULT_PATH="\".\""
ifeq ($(GAPP_PROXY),n)
# Disable GAPP/RPXOY to run if GAPP_PROXY is set to n
SYSTEM_CFLAGS += -DGAPP_DISABLE_PROXY
endif
ifeq ($(GAPP_GSM),n)
# Disable GAPP/RPXOY to run if GAPP_PROXY is set to n
SYSTEM_CFLAGS += -DGAPP_DISABLE_GSM
export GAPP_GSM=n
endif

endif

#
# Define this for ISIM and its log
#
ifeq ($(ISIM),y)
# Define include ISIM
SYSTEM_CFLAGS += -DINCLUDE_ISIM
# Define this for debug logging in the CSM application
#SYSTEM_CFLAGS += -DISIM_DEBUG
# Define the CSM configuration file path
SYSTEM_CFLAGS += -DISIM_CONFIG_DEFAULT_PATH="\".\""
# Define this for sending CSM_SERVICE_REASON_IMS_ENABLE to register to IMS 
# after account initilization. Disable it to disable sending 
# CSM_SERVICE_REASON_IMS_ENABLE in isim task after account initialization.
# It will be used to manually enable ims in csm_ut_cli.
SYSTEM_CFLAGS += -DISIM_ENABLE_IMS_REGISTRATION
endif

ifeq ($(MGT),y)
# Define include MGT
SYSTEM_CFLAGS += -DINCLUDE_MGT
# Define this for debug logging in the CSM application
#SYSTEM_CFLAGS += -DMGT_DEBUG
# Define the CSM configuration file path
SYSTEM_CFLAGS += -DMGT_CONFIG_DEFAULT_PATH="\".\""
endif

#
# Define this for CSM and its log
#
ifeq ($(CSM),y)
# Define this for debug logging in the CSM application
SYSTEM_CFLAGS += -DCSM_DEBUG
# Define the CSM configuration file path
SYSTEM_CFLAGS += -DCSM_CONFIG_DEFAULT_PATH="\".\""
endif

#
# Define this for TAPP and its log
#
ifeq ($(TAPP),y)
# No need SR for TAPP
export SR = n
# Set dummy audio driver for TAPP
export HARDWARE	:= dummy
SYSTEM_CFLAGS += -DINCLUDE_TAPP
#SYSTEM_CFLAGS += -DTAPP_DEBUG
else
ifeq ($(SR),y)
# SR flag is needed for building 4G+ in modem processor.
SYSTEM_CFLAGS += -DINCLUDE_SR
endif
# Define this to disable AT device(GSM modem)
SYSTEM_CFLAGS += -DGAPP_DISABLE_AT_DEVICE
endif

#
# Define this for SUPSRV and its log
#
ifeq ($(SUPSRV),y)
SYSTEM_CFLAGS += -DINCLUDE_SUPSRV
#SYSTEM_CFLAGS += -DSUPSRV_DEBUG
SYSTEM_CFLAGS += -DINCLUDE_XCAP
#SYSTEM_CFLAGS += -DXCAP_DBG_LOG
endif

#
# Define this for GBA/GAA and its log
#
ifeq ($(GBA),y)
# Define  for debug logging in the GBA/GAA
#SYSTEM_CFLAGS += -DGBA_DEBUG
#SYSTEM_CFLAGS += -DGAA_DEBUG
#SYSTEM_CFLAGS += -DGBAM_DEBUG
SYSTEM_CFLAGS += -DINCLUDE_GBA
endif

#
# Define this for Modem Management and its log
#
ifeq ($(MM),y)
# Define  for debug logging in the MM
SYSTEM_CFLAGS += -DMM_DEBUG
SYSTEM_CFLAGS += -DINCLUDE_MM
#
# Define the LIB of LTE module for third party.
# currently, we have 
# dummy, Apply Dummy Library.
# wncmal, Apply WNC MAL Library.
export EXTERN_ENABLE_LTE_LIB := dummy
#export EXTERN_ENABLE_LTE_LIB := wncmal
endif

#
# Define to enable ISI debug printing
#
SYSTEM_CFLAGS += -DISI_DEBUG_LOG

#
# Define to enable ISI debug printing
#
#SYSTEM_CFLAGS += -DEZXML_DEBUG_LOG

#
# Define to determine the way about memory allocation.
#
#SYSTEM_CFLAGS += -DISI_DYNAMIC_MEMORY_ALLOC

#
# Define to enable video debug printing
#
#SYSTEM_CFLAGS += -DVIDEO_DEBUG_LOG

#
# Define to enable RIR debug printing
#
ifeq ($(RIR),y)
SYSTEM_CFLAGS += -DINCLUDE_RIR
#SYSTEM_CFLAGS += -DRIR_DEBUG_LOG
endif

#
# Define to enable RPM debug printing
#
#SYSTEM_CFLAGS += -DRPM_DEBUG

#
# Define to enable PDU debug printing
#
#SYSTEM_CFLAGS += -DPDU_DEBUG

#
#  !!!!!!!!!!!!!!!!!!!!!!  
#  !!! Important Note !!!
#  !!!!!!!!!!!!!!!!!!!!!!
#  The following constant defines how many buffers are required for time
#  allignment of Rin and Rout with Sin, because of delay in the VHW driver, from
#  tx_ptr to rx_ptr. Examples are tablulated below.
#  
#  Measured delay between tx_ptr and rx_ptr     VTSPR_NALLIGNMENT_BUFS
#  10 ms                                        (2)
#  20 ms                                        (3)
#  30 ms                                        (4)
# 
#  A unique setting is required for each PLATFORM, as set below.
#
SYSTEM_CFLAGS += -DVTSPR_NALLIGNMENT_BUFS=3

# !!! Important Note !!!
# Defines the number of samples in the audio interface processing
# At this time only 80 or 160 is supported for narrow band or wideband audio
# interface processing.
# Currently, there is no support for mixed bandwidth from different interface
# types
SYSTEM_CFLAGS += -DVTSPR_NSAMPLES_AUDIO=160
SYSTEM_CFLAGS += -DVTSPR_NSAMPLES_10MS_MAX=160
#SYSTEM_CFLAGS += -DVTSP_ENABLE_AUDIO_SRC

# VTSP OPTIONS: to enable the below options, put the definition in the
# VTSP_OPTIONS
#VTSP_ENABLE_G729
#VTSP_ENABLE_G726
#VTSP_ENABLE_G722
#VTSP_ENABLE_G722P1
#VTSP_ENABLE_G711P1
#VTSP_ENABLE_ILBC
#VTSP_ENABLE_G723
#VTSP_ENABLE_SILK
#VTSP_ENABLE_DTMFR
#VTSP_ENABLE_TRACE 
#VTSP_ENABLE_NETLOG
#VTSP_ENABLE_COUNT_REGISTER  
#VTSP_ENABLE_MEASURE_IMPULSE
#VTSP_ENABLE_RTP_REDUNDANT
#VTSP_ENABLE_AUDIO_TIMEOUT
# 
# Bug 14269.
# VTSP_ENABLE_MP_LITE is used for no D2 software DSP included.
# There is only included Jitter Buffer and comm liberary.
# Meanwhile, the MODULE_ALG_MP_LITE should be set in ve/vtsp_rt/config.mk.
#
VTSP_OPTIONS := \
    VTSP_ENABLE_DTMFR \
	VTSP_ENABLE_TIMEOUT_FIX \
    VTSP_ENABLE_TRACE

### VPORT WideBand Options ###
#
# !!! Important note !!!
#
# Two options available
# 1.) Narrowband mode: Compile with VTSP_ENABLE_STREAM_8K and no G722P1
# 2.) Wideband mode:   Compile w/ VTSP_ENABLE_STREAM_16K and VTSP_ENABLE_G722P1
#
#
#VTSP_ENABLE_STREAM_8K
#VTSP_ENABLE_STREAM_16K
#VTSP_ENABLE_G722P1
#VTSP_ENABLE_G722
#VTSP_ENABLE_SILK
VTSP_WB_OPTIONS :=         \
	VTSP_ENABLE_STREAM_16K \
#	VTSP_ENABLE_STREAM_8K

SYSTEM_CFLAGS += $(addprefix -D,$(VTSP_OPTIONS)) 
SYSTEM_CFLAGS += $(addprefix -D,$(VTSP_WB_OPTIONS))

#  Active Stream/Interface Limiting Constants
SYSTEM_CFLAGS += -DVTSP_MAX_NUM_ACTIVE_INFCS=1
SYSTEM_CFLAGS += -DVTSP_MAX_NUM_ACTIVE_STREAMS=3

# Revision String src file
export VPORT_REVINFO_FILE = $(BASE_DIR)/system/$(PLATFORM)/$(VPORT_OS)/vport_revinfo.c

# List components in order for making (directories to build)
export MODULES 		+=				\
		osal/include 				\
		osal/kernel					\
		extern						\
		osal/user					\
		isi/libisi					\
		vpr/include					\
		isi/rpc/include
ifeq ($(4G_PLUS_RCS),y)
export MODULES +=                   \
		isi/rpc/xdr
endif

ifeq ($(OSAL_UT),y)
export MODULES +=                   \
		osal_ut/user				\
		osal_ut/user/main
endif

# SIT: STUN, ICE, TRUN 
ifeq ($(SIT),y)
export MODULES += 					\
		shared/sit
endif

# HTTP
ifeq ($(HTTP),y)
export MODULES += 					\
		shared/http
# select which backend HTTP wrapper should use
export EXTERN_ENABLE_HTTP_LIBCURL = y
#export EXTERN_ENABLE_HTTP_LIBFETCH = y
#export EXTERN_ENABLE_HTTP_VXWORKS = y
#export EXTERN_ENABLE_HTTP_LIBDOWNLOAD = y
# added unit test only in pent0
export MODULES += 					\
		shared/http/test
#SYSTEM_CFLAGS += -DHTTP_DBG_LOG
endif

ifeq ($(GBA),y)
export MODULES += 					\
	    shared/gba

# added unit test only in pent0
export MODULES += 					\
		shared/gba/test
endif

# XCAP
ifeq ($(XCAP),y)
export MODULES += 					\
		shared/xcap
endif

# SUPSRV
ifeq ($(SUPSRV),y)
export MODULES += 					\
		shared/supsrv
endif

# PDU
ifeq ($(PDU),y)
export MODULES += 					\
		shared/pdu
endif

# SETTINGS 
ifeq ($(SETTINGS),y)
#SYSTEM_CFLAGS += -DSETTINGS_DEBUG
# Define the default configuration folder for all modules
SYSTEM_CFLAGS += -DSETTINGS_CONFIG_DEFAULT_FOLDER="\".\""
export CFG_METHOD := ezxml
export MODULES +=                   \
        shared/settings
endif

ifeq ($(SIP),y)
export MODULES +=                   \
        shared/sip
endif

# VTSP
ifeq ($(VTSP),y)
export MODULES 		+= 				\
		ve/vtsp_hw 					\
		ve/rtp 						\
		ve/vtsp 					\
		ve/vtsp_rt
endif	

# MC
ifeq ($(MC),y)
export MODULES +=                   \
        isi/proto/mc
endif

# SAPP
ifeq ($(SAPP),y)
export MODULES += 					\
		isi/proto/sapp
#		isi/proto/sapp/msrp/ut
endif

ifeq ($(RPM),y)
export MODULES += 					\
	rpm
endif

# vPort 4G Plus
ifeq ($(4G_PLUS),y)
export MODULES += 					\
		modemDriver/include			\
		vpr							\
		modemDriver/vpmd_mux		\
		modemDriver/vpmd
# vPort 4G+ with RCS feature
ifeq ($(4G_PLUS_RCS),y)
export MODULES +=                   \
		isi/rpc/server
endif

ifeq ($(4G_PLUS_MODEM_TEST),y)
# vpmd echo server for testing modemDriver
export MODULES += 					\
		modemDriver/vpmd/test
endif

endif


# VTSP  - test. Must be after 4G_PLUS
ifeq ($(VTSP_UT),y)
export MODULES 		+= 				\
		ve/vtsp_ut 
endif	

# VIDEO
ifeq ($(VIDEO),y)
export MODULES 		+= 				\
		video
endif

# RIR
ifeq ($(RIR),y)
export MODULES += 					\
	rir
endif

ifeq ($(CSM),y)
export MODULES += 					\
	csm
endif

# GAPP 
ifeq ($(GAPP),y)
export MODULES += 					\
	isi/proto/gapp
endif	

ifeq ($(CSM),y)
export MODULES += 					\
	csm/main						\
	csm/ut/cli                      \

export EXTERN_ENABLE_JSMN = y

# Build isi_ut or ipsec test 
export MODULES += 					\
#	isi/proto/sapp/main \
#	isi/ut

endif

ifeq ($(TAPP),y)
export MODULES +=                   \
    csm/ut/tapp
endif

ifeq ($(MM),y)
ifeq ($(EXTERN_ENABLE_LTE_LIB), wncmal)
export MODULES += 					\
    mm/wncmal
endif
ifeq ($(EXTERN_ENABLE_LTE_LIB), dummy)
export MODULES += 					\
    mm/dummy
endif
export MODULES += 					\
    mm \
	mm/ut
endif

ifeq ($(VAPP),y)
export MODULES += 					\
    vapp
endif


# END OF MAKEFILE


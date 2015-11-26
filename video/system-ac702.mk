#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
# AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
# APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 19429 $ $Date: 2012-12-21 13:07:07 -0600 (Fri, 21 Dec 2012) $
#

#
# This file is for user-defined software configuration
# 
# -------- You may modify this file to include customizations
#

# Define the number of interfaces for Voice App
MY_SYSTEM_CFLAGS += -DHARDWARE_NUM_INFC_1

# -------- Select Hardware here
MY_HARDWARE	:= dummy
MY_SYSTEM_CFLAGS += -DANDROID_JELLYBEAN

# Extern library option flag, define this to include ezxml library
MY_MY_EXTERN_ENABLE_EZXML = y

# Extern library option flags, define this to include AUTH library
MY_EXTERN_ENABLE_AUTH = y

# Extern library option flags, define this to include the milenage algorithm
MY_EXTERN_ENABLE_MILENAGE = y

# C lexer parser, define to use it, required for XCAP
MY_EXTERN_ENABLE_HTTP_PARSE = y

# PDU convertor for PDU SMS messages
MY_EXTERN_ENABLE_PDUCONV = y

# For input, what console device to use
MY_SYSTEM_CFLAGS += -DCONSOLE="\"/dev/tty\""

# SSL library flag, define this to link with SSL library
#MY_SYSTEM_CFLAGS += -DOSAL_NET_ENABLE_SSL

# OSAL_NET option flags
#MY_SYSTEM_CFLAGS += -DOSAL_NET_ENABLE_DNS

# Enable ipsec support
#MY_SYSTEM_CFLAGS += -DOSAL_IPSEC_ENABLE

# Enable OSAL_CRYPTO support
#MY_SYSTEM_CFLAGS += -DOSAL_CRYPTO_ENABLE

# NDK log
MY_SYSTEM_CFLAGS += -DOSAL_LOG_NDK

# Enable Regex Support
#MY_SYSTEM_CFLAGS += -DOSAL_ENABLE_REGEX

# OSAL_ARCH option flags
#MY_SYSTEM_CFLAGS += -DOSAL_ARCH_ENABLE_COUNT

# A/V sync
MY_SYSTEM_CFLAGS += -DLIP_SYNC_ENABLE

#
# This enables running vTSP from user space
#
MY_SYSTEM_CFLAGS += -DOSAL_KERNEL_EMULATION

# Define this to include SAPP in build
MY_SAPP = n

# Define this to include SIP SIMPLE in build
MY_SIMPLE = n

# Define this to include VIER in build
MY_VIER = y

# Define this to include VPR in build
MY_VPR = y

# Define this to include vTSP in build
MY_VTSP = y

# Define this to include video in build
MY_VIDEO = y

# Define this to include SRTP in build
MY_SRTP = n

# Define this to include SIP shared library in build
MY_SIP = n

# Define this to include STUN/ICE/TURN shared library in build
MY_SIT = n

# Define this to include HTTP shared library in build
MY_HTTP = n

# Define this to include XCAP shared library in build
MY_XCAP = n

# Define this to include PDU shared library in build
MY_PDU = n

# Define this to include SETTINGS shared library in build
# Always enable it for CSM and SAPP
MY_SETTINGS = y

# Define this to include vPort 4G+ modemDriver testing in build
MY_4G_PLUS_MODEM_TEST = y

# Define this to have vPort 4G+ support RCS features
MY_4G_PLUS_RCS = n

# Define this to have vPort 4G+
MY_4G_PLUS = y

# Define this to include SR in build
# SR needs to be included for RCS or wifi-offload
ifeq ($(MY_4G_PLUS_RCS),y)
MY_SR = y
else
ifeq ($(MY_4G_PLUS_WIFI_OFFLOAD),y)
MY_SR= y
endif
endif

# Define this to include rir in build.
MY_RIR = n

#
# Define this for SIP and its log
#
ifeq ($(MY_SIP),y)
MY_SYSTEM_CFLAGS += -DINCLUDE_SIP
# Define this for debug logging in the protocol
MY_SYSTEM_CFLAGS	+= -DSIP_DEBUG_LOG
# Define this for debug logging in the SAPP application
MY_SYSTEM_CFLAGS += -DSAPP_DEBUG
#MY_SYSTEM_CFLAGS += -DMNS_DEBUG
ifeq ($(MY_SIMPLE),y)
MY_SYSTEM_CFLAGS += -DINCLUDE_SIMPLE
#MY_SYSTEM_CFLAGS += -DHTTP_DBG_LOG
MY_SYSTEM_CFLAGS += -DMSRP_DBG_LOG
MY_SYSTEM_CFLAGS += -DINCLUDE_XCAP
#MY_SYSTEM_CFLAGS += -DXCAP_DBG_LOG
endif
endif

ifeq ($(MY_4G_PLUS),y)
MY_SYSTEM_CFLAGS += -DINCLUDE_4G_PLUS
# Define this for debug logging in VIER
MY_SYSTEM_CFLAGS += -DVIER_DEBUG 
# Define this for debug logging in VOER
MY_SYSTEM_CFLAGS += -DVOER_DEBUG 
# Define this for debug logging in SR
MY_SYSTEM_CFLAGS += -DSR_DEBUG 
# Define this for debug logging in ISI RPC
MY_SYSTEM_CFLAGS += -DISI_RPC_DEBUG
# Define this for bulding under application processor
MY_SYSTEM_CFLAGS += -DVPORT_4G_PLUS_APROC
# Define this for debug logging in VPAD IO driver
#MY_SYSTEM_CFLAGS += -DVPAD_IO_DEBUG
# Define this for debug logging in VPAD MUX
#MY_SYSTEM_CFLAGS += -DVPAD_MUX_DEBUG
MY_SYSTEM_CFLAGS += -DVPAD_MUX_STATS
endif

ifeq ($(MY_4G_PLUS_RCS),y)
MY_SYSTEM_CFLAGS += -DINCLUDE_4G_PLUS_RCS
endif

ifeq ($(MY_VPAD),y)
MY_SYSTEM_CFLAGS += -DINCLUDE_VPAD
endif

ifeq ($(MY_VPR),y)
MY_SYSTEM_CFLAGS += -DINCLUDE_VPR
endif

ifeq ($(MY_VIER),y)
MY_SYSTEM_CFLAGS += -DINCLUDE_VIER
endif

ifeq ($(MY_VOER),y)
MY_SYSTEM_CFLAGS += -DINCLUDE_VOER
endif

ifeq ($(MY_SR),y)
MY_SYSTEM_CFLAGS += -DINCLUDE_SR
endif

ifeq ($(MY_4G_PLUS_MODEM_TEST),y)
MY_SYSTEM_CFLAGS += -DVP4G_PLUS_MODEM_TEST
endif

#
# Define this for vTSP and its log
#
ifeq ($(MY_VTSP),y)
MY_SYSTEM_CFLAGS += -DINCLUDE_VTSP
# Define this for debug log
#MY_SYSTEM_CFLAGS += -DVTSP_ENABLE_TRACE
# Define this for debug logging in the VE application
#MY_SYSTEM_CFLAGS += -DMC_DEBUG
# Define this if the mCUE should handle the ringtones NOT vTSP
MY_SYSTEM_CFLAGS += -DMC_NO_RING
endif

#
# Define this for MC and its log
#
ifeq ($(MY_MC),y)
MY_SYSTEM_CFLAGS += -DINCLUDE_MC
# Define this for debug logging in the MC
#MY_SYSTEM_CFLAGS += -DMC_DEBUG
endif

#
# Define this for GSM_AT and its log
#
ifeq ($(MY_GAPP),y)
MY_SYSTEM_CFLAGS += -DINCLUDE_GAPP
# Define this for debug logging in the GAPP application
MY_SYSTEM_CFLAGS += -DGAPPD_DEBUG
MY_SYSTEM_CFLAGS += -DPRXY_DEBUG
MY_SYSTEM_CFLAGS += -DGSM_DEBUG
# Define this to disable supplementary service at command filtering
MY_SYSTEM_CFLAGS += -DGAPP_DISABLE_SUPPLEMENTARY_SERVICE_AT_CMD
# Define this to disable AT device(GSM modem)
#MY_SYSTEM_CFLAGS += -DGAPP_DISABLE_AT_DEVICE
# Define the gsm configuration file path
#MY_SYSTEM_CFLAGS += -DGSM_CONFIG_DEFAULT_PATH="\".\""
ifeq ($(MY_GAPP_PROXY),n)
# Disable GAPP/RPXOY to run if GAPP_PROXY is set to n
MY_SYSTEM_CFLAGS += -DGAPP_DISABLE_PROXY
endif
endif

#
# Define this for ISIM and its log
#
ifeq ($(MY_ISIM),y)
# Define include ISIM
MY_SYSTEM_CFLAGS += -DINCLUDE_ISIM
# Define this for debug logging in the CSM application
#MY_SYSTEM_CFLAGS += -DISIM_DEBUG
# Define the CSM configuration file path
#MY_SYSTEM_CFLAGS += -DISIM_CONFIG_DEFAULT_PATH="\".\""
# Define this for sending CSM_SERVICE_REASON_IMS_ENABLE to register to IMS 
# after account initilization. Disable it to disable sending 
# CSM_SERVICE_REASON_IMS_ENABLE in isim task after account initialization.
# It will be used to manually enable ims in csm_ut_cli.
MY_SYSTEM_CFLAGS += -DISIM_ENABLE_IMS_REGISTRATION
endif

ifeq ($(MY_MGT),y)
# Define include MGT
MY_SYSTEM_CFLAGS += -DINCLUDE_MGT
# Define this for debug logging in the CSM application
#MY_SYSTEM_CFLAGS += -DMGT_DEBUG
# Define the CSM configuration file path
#MY_SYSTEM_CFLAGS += -DMGT_CONFIG_DEFAULT_PATH="\".\""
endif

#
# Define this for SUPSRV and its log
#
ifeq ($(MY_SUPSRV),y)
MY_SYSTEM_CFLAGS += -DINCLUDE_SUPSRV
#MY_SYSTEM_CFLAGS += -DSUPSRV_DEBUG
MY_SYSTEM_CFLAGS += -DINCLUDE_XCAP
#MY_SYSTEM_CFLAGS += -DXCAP_DBG_LOG
endif

#
# Define this for CSM and its log
#
ifeq ($(MY_CSM),y)
ifeq ($(MY_CSM_SAPP),n)
MY_SYSTEM_CFLAGS += -DCSM_DISABLE_SAPP
endif
# Define this for debug logging in the GAPP application
#MY_SYSTEM_CFLAGS += -DCSM_CONFIG_DEFAULT_PATH="\".\""
MY_SYSTEM_CFLAGS += -DCSM_DEBUG
endif

#
# Define to enable ISI debug printing
#
MY_SYSTEM_CFLAGS += -DISI_DEBUG_LOG

#
# Define to enable video debug printing
#
#MY_SYSTEM_CFLAGS += -DVIDEO_DEBUG_LOG

#
# Define to enable RIR debug printing
#
ifeq ($(MY_RIR),y)
MY_SYSTEM_CFLAGS += -DINCLUDE_RIR
#MY_SYSTEM_CFLAGS += -DRIR_DEBUG_LOG
endif

#
# Define to enable RPM debug printing
#
#MY_SYSTEM_CFLAGS += -DRPM_DEBUG

#
# Define to enable PDU debug printing
#
#MY_SYSTEM_CFLAGS += -DPDU_DEBUG

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
MY_SYSTEM_CFLAGS += -DVTSPR_NALLIGNMENT_BUFS=3

# !!! Important Note !!!
# Defines the number of samples in the audio interface processing
# At this time only 80 or 160 is supported for narrow band or wideband audio
# interface processing.
# Currently, there is no support for mixed bandwidth from different interface
# types
MY_SYSTEM_CFLAGS += -DVTSPR_NSAMPLES_AUDIO=160
MY_SYSTEM_CFLAGS += -DVTSPR_NSAMPLES_10MS_MAX=160
#MY_SYSTEM_CFLAGS += -DVTSP_ENABLE_AUDIO_SRC

# VTSP OPTIONS: to enable the below options, put the definition in the
# VTSP_OPTIONS
#VTSP_ENABLE_GAMRNB
#VTSP_ENABLE_G729
#VTSP_ENABLE_G726
#VTSP_ENABLE_G722
#VTSP_ENABLE_G722P1
#VTSP_ENABLE_G711P1
#VTSP_ENABLE_GAMRNB
#VTSP_ENABLE_GAMRWB
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

MY_VTSP_OPTIONS := \
	VTSP_ENABLE_GAMRNB \
	VTSP_ENABLE_DTMFR \
#	VTSP_ENABLE_TRACE \
#	VTSP_ENABLE_SRTP \
	VTSP_ENABLE_RTP_REDUNDANT

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
#VTSP_ENABLE_GAMRWB
#VTSP_ENABLE_G722P1
#VTSP_ENABLE_G722
#VTSP_ENABLE_SILK
MY_VTSP_WB_OPTIONS :=         \
	VTSP_ENABLE_STREAM_16K \
	VTSP_ENABLE_GAMRWB
#	VTSP_ENABLE_STREAM_8K

MY_SYSTEM_CFLAGS += $(addprefix -D,$(VTSP_OPTIONS)) 
MY_SYSTEM_CFLAGS += $(addprefix -D,$(VTSP_WB_OPTIONS))

#  Active Stream/Interface Limiting Constants
MY_SYSTEM_CFLAGS += -DVTSP_MAX_NUM_ACTIVE_INFCS=1
MY_SYSTEM_CFLAGS += -DVTSP_MAX_NUM_ACTIVE_STREAMS=3

# Revision String src file
MY_VPORT_REVINFO_FILE = $(BASE_DIR)/system/$(MY_PLATFORM)/$(VPORT_OS)/vport_revinfo.c

# List components in order for making (directories to build)
MY_MODULES 		+=				\
		osal/include				\
		osal/kernel					\
		osal/user					\
		extern						\
		isi/libisi					\
		vpr/include                 \
		isi/rpc/include

# SIT: STUN, ICE, TRUN 
ifeq ($(MY_SIT),y)
MY_MODULES += 					\
		shared/sit
endif

# HTTP
ifeq ($(MY_HTTP),y)
MY_EXTERN_ENABLE_HTTP_LIBCURL = y
#export EXTERN_ENABLE_HTTP_STATIC_CURL= y
#export EXTERN_ENABLE_HTTP_LIBFETCH = y
#export EXTERN_ENABLE_HTTP_VXWORKS = y
#export EXTERN_ENABLE_HTTP_LIBDOWNLOAD = y

# If a static libcurl is desired then we must first build it before HTTP 
ifeq ($(MY_EXTERN_ENABLE_HTTP_STATIC_CURL),y)
MY_MODULES += 					\
		extern/curl
# only in pent0 to do curl testing, comment out in other platform
#export MY_MODULES += 					\
#		extern/curl/src
endif

MY_MODULES += 					\
		shared/http
endif

# XCAP
ifeq ($(MY_XCAP),y)
MY_MODULES += 					\
		shared/xcap
endif

# PDU
ifeq ($(MY_PDU),y)
MY_MODULES += 					\
		shared/pdu
endif

# SETTINGS 
ifeq ($(MY_SETTINGS),y)
MY_SYSTEM_CFLAGS += -DSETTINGS_DEBUG   
MY_MODULES +=                   \
        shared/settings
endif

# SUPSRV
ifeq ($(MY_SUPSRV),y)
MY_MODULES += 					\
		shared/supsrv
endif

ifeq ($(MY_SIP),y)
MY_MODULES +=                   \
        shared/sip
endif

# VTSP
ifeq ($(MY_VTSP),y)
MY_MODULES 		+=				\
		ve/vtsp_hw 					\
		ve/rtp 						\
		ve/vtsp 
endif	

# SAPP
ifeq ($(MY_SAPP),y)
MY_MODULES += 					\
		isi/proto/sapp 
#		isi/proto/sapp/msrp/ut
endif

# RPM 
ifeq ($(MY_RPM),y)
MY_MODULES += 					\
	rpm					
endif

# vPort 4G Plus
ifeq ($(MY_4G_PLUS),y)
MY_MODULES += 					\
		modemDriver/include	
ifeq ($(MY_4G_PLUS_RCS),y)
MY_MODULES +=                   \
		isi/rpc/xdr					\
		isi/rpc/client
endif

ifeq ($(MY_VPAD),y)
MY_MODULES += 					\
		modemDriver
endif

ifeq ($(MY_ISI_SERVER),y)
MY_MODULES +=                   \
		isi/rpc/server
endif

ifeq ($(MY_4G_PLUS_RCS),y)
MY_MODULES += 					\
		isi/libisi_jni
else
ifeq ($(MY_4G_PLUS_WIFI_OFFLOAD),y)
MY_MODULES += 					\
		isi/libisi_jni
endif
endif

ifeq ($(MY_SR),y)
MY_MODULES +=                   \
		sr
endif

ifeq ($(MY_VIER),y)
MY_MODULES +=                   \
		vier
endif

ifeq ($(MY_VOER),y)
MY_MODULES +=                   \
		voer						\
		voer/main
endif

ifeq ($(MY_4G_PLUS_MODEM_TEST),y)
# for testing modemDriver
MY_MODULES += 					\
		modemDriver/vpad/test
endif

endif # 4G_PLUS

# VTSP  - test. Must be after 4G_PLUS
#ifeq ($(VTSP),y)
#export MY_MODULES 		+= 				\
#		ve/vtsp_ut					
#		isi/proto/mc/main
#endif

# VIDEO
ifeq ($(MY_VIDEO),y)
MY_MODULES += 				\
	$(MY_VIDEO_DIR)
endif

# RIR 
ifeq ($(MY_RIR),y)
MY_MODULES += 					\
	rir					\
	rir/main
endif

ifeq ($(MY_CSM),y)
MY_MODULES += 					\
	csm								
endif

# GAPP 
ifeq ($(MY_GAPP),y)
MY_MODULES += 					\
	isi/proto/gapp					
endif	

ifeq ($(MY_CSM),y)
MY_MODULES += 					\
	csm/main					
#	csm/ut
endif

#export MY_MODULES +=                   \
#    isi/libproto_jni                \

# sapp main
MY_MODULES += 					\
#		isi/proto/sapp/main

# END OF MAKEFILE

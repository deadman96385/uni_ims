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

# -------- Select Hardware here
export HARDWARE	:= dummy

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

# JSMN
export EXTERN_ENABLE_JSMN = y

# For vce to enable camera & view.
SYSTEM_CFLAGS += -DENABLE_CAMERA_VIEW

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

# Define this to include SAPP in build
export SAPP = y

# Define this to include SIP SIMPLE in build
export SIMPLE = n

# Define this to include vTSP in build
export VTSP = y

# Define this to include video in build
export VIDEO = y

# Define this to include rir in build
#export RIR = y

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

# Define this to include PDU shared library in build
export PDU = y

# Define this to include SETTINGS shared library in build
# Always enable it for CSM and SAPP
export SETTINGS = y

# Define this to include vPort 4G+ modemDriver testing in build
export 4G_PLUS_MODEM_TEST = n

# Define this to have vPort 4G+ support RCS features
export 4G_PLUS_RCS = y

# Define this to include GBA/GAA in build
export GBA = n

# Define this to include SAPP by CSM in build
export CSM_SAPP = y

#
# Define this for SIP and its log
#
ifeq ($(SAPP),y)
SYSTEM_CFLAGS += -DINCLUDE_SAPP
# Define this for debug logging in the protocol
SYSTEM_CFLAGS	+= -DSIP_DEBUG_LOG
# Define this for debug logging in the SAPP application
SYSTEM_CFLAGS += -DSAPP_DEBUG
#SYSTEM_CFLAGS += -DMNS_DEBUG
ifeq ($(SIMPLE),y)
SYSTEM_CFLAGS += -DINCLUDE_SIMPLE
#SYSTEM_CFLAGS += -DHTTP_DBG_LOG
#SYSTEM_CFLAGS += -DMSRP_DBG_LOG
SYSTEM_CFLAGS += -DINCLUDE_XCAP
#SYSTEM_CFLAGS += -DXCAP_DBG_LOG
endif
endif

#
# Define this for MC and its log
#
ifeq ($(MC),y)
SYSTEM_CFLAGS += -DINCLUDE_MC
# Define this for debug logging in the MC
#SYSTEM_CFLAGS += -DMC_DEBUG
endif


ifeq ($(4G_PLUS),y)
SYSTEM_CFLAGS += -DINCLUDE_4G_PLUS
#SYSTEM_CFLAGS += -DINCLUDE_VPR 
# Define this for debug logging in VIER
#SYSTEM_CFLAGS += -DVIER_DEBUG 
# Define this for debug logging in VOER
#SYSTEM_CFLAGS += -DVOER_DEBUG 
# Define this for debug logging in SR
SYSTEM_CFLAGS += -DSR_DEBUG 
# Define this for debug logging in ISI RPC
SYSTEM_CFLAGS += -DISI_RPC_DEBUG
# Define this for bulding under application processor
SYSTEM_CFLAGS += -DVPORT_4G_PLUS_APROC
# Define this for debug logging in VPAD IO driver
#SYSTEM_CFLAGS += -DVPAD_IO_DEBUG
# Define this for debug logging in VPAD MUX
#SYSTEM_CFLAGS += -DVPAD_MUX_DEBUG
# Define this for vpad statistics report
#SYSTEM_CFLAGS += -DVPAD_MUX_STATS
# Define this for special unit testing RIR manual IP
#SYSTEM_CFLAGS += -DRIR_MANUAL_IP
endif

ifeq ($(4G_PLUS_RCS),y)
SYSTEM_CFLAGS += -DINCLUDE_4G_PLUS_RCS
endif

ifeq ($(VPAD),y)
SYSTEM_CFLAGS += -DINCLUDE_VPAD
endif

ifeq ($(VIER),y)
SYSTEM_CFLAGS += -DINCLUDE_VIER
endif

ifeq ($(VOER),y)
SYSTEM_CFLAGS += -DINCLUDE_VOER
endif

ifeq ($(SR),y)
SYSTEM_CFLAGS += -DINCLUDE_SR
endif

ifeq ($(4G_PLUS_MODEM_TEST),y)
SYSTEM_CFLAGS += -DVP4G_PLUS_MODEM_TEST
# Define this for skip delay in vpxd usb io
#SYSTEM_CFLAGS += -DVPXD_NO_DELAY
endif

#
# Define this for vTSP and its log
#
ifeq ($(VTSP),y)
SYSTEM_CFLAGS += -DINCLUDE_VTSP
# Define this for debug log
#SYSTEM_CFLAGS += -DVTSP_ENABLE_TRACE
# Define this for debug logging in the VE application
#SYSTEM_CFLAGS += -DMC_DEBUG
# Define this if the mCUE should handle the ringtones NOT vTSP
SYSTEM_CFLAGS += -DMC_NO_RING
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
# Define this to disable AT device(GSM modem)
#SYSTEM_CFLAGS += -DGAPP_DISABLE_AT_DEVICE
# Define the gsm configuration file path
SYSTEM_CFLAGS += -DGSM_CONFIG_DEFAULT_PATH="\".\""
ifeq ($(GAPP_PROXY),n)
# Disable GAPP/RPXOY to run if GAPP_PROXY is set to n
SYSTEM_CFLAGS += -DGAPP_DISABLE_PROXY
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
SYSTEM_CFLAGS += -DMGT_DEBUG
# Define the CSM configuration file path
SYSTEM_CFLAGS += -DMGT_CONFIG_DEFAULT_PATH="\".\""
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
# Define this for CSM and its log
#
ifeq ($(CSM),y)
# Define this for debug logging in the CSM application
SYSTEM_CFLAGS += -DCSM_DEBUG
# Define the CSM configuration file path
SYSTEM_CFLAGS += -DCSM_CONFIG_DEFAULT_PATH="\".\""
#SYSTEM_CFLAGS += -DTAPP_DEBUG

endif

#
# Define to enable ISI debug printing
#
SYSTEM_CFLAGS += -DISI_DEBUG_LOG

ifeq ($(VIDEO),y)
#
# Define to enable video debug printing
#
#SYSTEM_CFLAGS += -DVIDEO_DEBUG_LOG

#
# Define to not use any ffmpeg related stuff
#
SYSTEM_CFLAGS += -DEXCLUDE_FFMPEG_LIBRARY

endif

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

VTSP_OPTIONS := \
	VTSP_ENABLE_G729 \
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
#VTSP_ENABLE_GAMRWB
#VTSP_ENABLE_G722P1
#VTSP_ENABLE_G722
#VTSP_ENABLE_SILK
VTSP_WB_OPTIONS :=         \
	VTSP_ENABLE_STREAM_16K \
	VTSP_ENABLE_GAMRWB
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
		osal/user					\
		extern						\
		isi/libisi					\
		vpr/include                 \
		isi/rpc/include

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
#export EXTERN_ENABLE_HTTP_STATIC_CURL= y
#export EXTERN_ENABLE_HTTP_LIBFETCH = y
#export EXTERN_ENABLE_HTTP_VXWORKS = y
#export EXTERN_ENABLE_HTTP_LIBDOWNLOAD = y
# added unit test only in pent0
#export MODULES += 					\
#		shared/http/test
#SYSTEM_CFLAGS += -DHTTP_DBG_LOG

# If static libcurl is desired then build it before http
ifeq ($(EXTERN_ENABLE_HTTP_STATIC_CURL),y)
export MODULES += 					\
		extern/curl
# only in pent0 to do curl testing, comment out in other platform
export MODULES += 					\
		extern/curl/src
endif

endif

# XCAP
ifeq ($(XCAP),y)
export MODULES += 					\
		shared/xcap
endif

# PDU
ifeq ($(PDU),y)
export MODULES += 					\
		shared/pdu
endif

# SETTINGS 
ifeq ($(SETTINGS),y)
SYSTEM_CFLAGS += -DSETTINGS_DEBUG
# Define the default configuration folder for all modules
SYSTEM_CFLAGS += -DSETTINGS_CONFIG_DEFAULT_FOLDER="\".\""
export CFG_METHOD := ezxml
export MODULES +=                   \
        shared/settings
endif

# SUPSRV
ifeq ($(SUPSRV),y)
export MODULES += 					\
		shared/supsrv
endif

ifeq ($(SIP),y)
export MODULES +=                   \
        shared/sip
endif

# VTSP
ifeq ($(VTSP),y)
export MODULES 		+= 				\
		ve/vtsp_hw					\
		ve/rtp 						\
		ve/vtsp 					
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

# RPM 
ifeq ($(RPM),y)
export MODULES += 					\
	rpm					
endif

# vPort 4G Plus
ifeq ($(4G_PLUS),y)
export MODULES += 					\
		modemDriver/include	
ifeq ($(4G_PLUS_RCS),y)
export MODULES +=                   \
		isi/rpc/xdr					\
		isi/rpc/client
endif

ifeq ($(VPAD),y)
export MODULES += 					\
		modemDriver/vpad			\
		modemDriver/vpad_mux		\
		modemDriver/vpad/main
endif

ifeq ($(ISI_SERVER),y)
export MODULES +=                   \
		isi/rpc/server
endif

ifeq ($(SR),y)
export MODULES +=                   \
		sr
endif

ifeq ($(VIER),y)
export MODULES +=                   \
		vier
endif

ifeq ($(VOER),y)
export MODULES +=                   \
		voer						\
		voer/main
endif

ifeq ($(4G_PLUS_MODEM_TEST),y)
# for testing modemDriver
export MODULES += 					\
		modemDriver/vpad/test
endif

endif

# VTSP  - test. Must be after 4G_PLUS
#ifeq ($(VTSP),y)
#export MODULES 		+= 				\
#		ve/vtsp_ut					\
#		isi/proto/mc/main
#endif	

# VIDEO
ifeq ($(VIDEO),y)
export MODULES 		+= 				\
		video_2_0
endif

# RIR 
ifeq ($(RIR),y)
export MODULES += 					\
	rir								\
	rir/main
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
	csm/ut/cli 
endif

export MODULES += 					\
	isi/proto/sapp/main	\
	isi/ut


# END OF MAKEFILE


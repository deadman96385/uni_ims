#!/bin/make
#
#  THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
#  AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
#  APPLIES:
#  "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
#
# $D2Tech$ $Rev: 10917 $ $Date: 2009-12-03 07:39:31 +0800 (Thu, 03 Dec 2009) $
#

# C Source files
CSRC	= \
		vtspr.c				       \
		vtspr_recv.c		       \
		vtspr_send.c		       \
		vtspr_task.c    	       \
		_vtspr_audio.c 		       \
		_vtspr_audio_hs.c          \
		_vtspr_audio_tone.c        \
		_vtspr_benchmark.c         \
		_vtspr_cid.c		       \
		_vtspr_cmd.c		       \
		_vtspr_cmd_tone.c          \
		_vtspr_coder.c 		       \
		_vtspr_conf.c 		       \
		_vtspr_default.c	       \
		_vtspr_event.c		       \
		_vtspr_event_fxo.c	       \
		_vtspr_event_fxs.c         \
		_vtspr_event_hs.c          \
        _vtspr_flow.c              \
		_vtspr_map.c 		       \
		_vtspr_multi_coder_init.c  \
		_vtspr_multi_decode.c      \
		_vtspr_multi_decode_task.c \
		_vtspr_multi_encode.c      \
		_vtspr_multi_encode_task.c \
		_vtspr_net.c               \
		_vtspr_netlog.c            \
		_vtspr_rate.c              \
        _vtspr_rtp_init.c          \
        _vtspr_rtp_close.c         \
        _vtspr_rtp_open.c          \
        _vtspr_rtp_recv.c          \
        _vtspr_rtp_send.c          \
        _vtspr_rtp_shutdown.c      \
		_vtspr_rtcp_init.c         \
		_vtspr_rtcp_close.c        \
		_vtspr_rtcp.c              \
		_vtspr_rtcp_open.c         \
		_vtspr_rtcp_recv.c         \
		_vtspr_rtcp_send.c         \
		_vtspr_state.c		       \
		_vtspr_stream.c		       \
		_vtspr_stun.c		       \
		_vtspr_t38.c   	           \
		_vtspr_time.c		       
		
ifeq ($(PLATFORM),no_alglib)
CSRC += 	_vtspr_algstub.c	
endif
# Assembly file
SSRC	= \

# Private Header files
PRIVATE_HEADERS	= \
		_vtspr_private.h \
        _vtspr_net.h \
        _vtspr_rtp.h \
		_vtspr_rtcp.h \
		_vtspr_stun.h \
		vtspr.h \
		vtspr_struct.h \
		vtspr_const.h

# Files to export to INCLUDE_DIR
PUBLIC_HEADERS	=  \

# Files to export to OBJ_DIR
OUTPUT			= archive

include $(TOOL_DIR)/rules.mk

# Build Rule - add custom build commands below this rule
build: default_build

# Clean Rule - add custom clean commands below this rule
clean: default_clean


# END OF MAKEFILE


/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5550 $ $Date: 2008-03-25 06:40:56 -0400 (Tue, 25 Mar 2008) $
 *
 */

#include "osal.h"

#include "vtsp.h"

#include "vtsp_ut.h"

int32   UT_rval = 0;
uint32  UT_silComp = UT_STREAM_TEST_CN_MASK;
uint32  UT_dtmfRelay = UT_STREAM_TEST_DR_MASK;
uint32  UT_encoder = VTSP_CODER_G711U;

OSAL_NetAddress UT_sendAddr;
OSAL_NetAddress UT_recvAddr;
uint32          UT_sendPort = 0;
uint32          UT_recvPort = 0;
uint32          UT_confMask = 0;

uint64          UT_failures = 0;
vint            UT_run = 0;
vint            UT_testTimeSec = 0;
vint            UT_testVideoKeyIntSec = 0;
uvint           UT_newStream = 0;
VTSP_Context   *UT_vtsp_ptr = NULL;
uvint           UT_detect = 0;
uvint           UT_extension = 0;
vint            UT_startPort = 0;
vint            UT_endPort = 0;
vint            UT_streamDir = 0;
vint            UT_streamEC = 0;
vint            UT_streamJBFixed = 0;
uvint           UT_encodeTime = 10;
uvint           UT_encodeTime_cn = 1000;
int16           UT_dtmfThreshold = -45;
int16           UT_vadThreshold = -62;
vint           UT_rtpTos = -1;

VTSP_CIDData   UT_cidObj;
#ifdef UT_MHZ_NET_LOG
int            UT_sockFd = NULL;
#endif



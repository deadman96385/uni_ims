/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12523 $ $Date: 2010-07-13 18:57:44 -0400 (Tue, 13 Jul 2010) $
 */

#ifndef _VCE_H_
#define _VCE_H_

#include <osal.h>
#include <vci.h>

#ifndef VIDEO_DEBUG_LOG
#define VCE_dbgPrintf(fmt, args...)
#else
#define VCE_dbgPrintf(fmt, args...) \
         OSAL_logMsg("[%s:%d] " fmt, __FUNCTION__, __LINE__, ## args)
#endif

typedef enum {
    VCE_EVENT_NONE                 = 0,
    VCE_EVENT_INIT_COMPLETE        = 1,
    VCE_EVENT_START_ENC            = 2,
    VCE_EVENT_START_DEC            = 3,
    VCE_EVENT_STOP_ENC             = 4,
    VCE_EVENT_STOP_DEC             = 5,
    VCE_EVENT_SHUTDOWN             = 6,
    VCE_EVENT_REMOTE_RECV_BW_KBPS  = 7,
    VCE_EVENT_SEND_KEY_FRAME       = 8
} VCE_Event;

typedef struct VCE_Event *VCE_Event_Ptr;
/*
 * Video Codec Engine (VCE) is the application side engine to the Video Controller module.
 */

vint VCE_init(
    int w,
    int h);

void VCE_shutdown(
    void);

#endif

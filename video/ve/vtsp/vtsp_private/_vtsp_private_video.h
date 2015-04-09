/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12419 $ $Date: 2010-07-01 01:39:23 +0800 (Thu, 01 Jul 2010) $
 *
 */

#ifndef __VTSP_PRIVATE_VIDEO_H_
#define __VTSP_PRIVATE_VIDEO_H_

#include "_vtsp_private.h"
#include <vid.h>
#include <rtp.h>
#include <h264.h>
#include <h263.h>

/*
 * Private video structure.
 * XXX: Only one interface defined.
 */
typedef struct {
    H264_EncodeObj   h264EncObj[_VTSP_STREAM_PER_INFC];
    H264_DecodeObj   h264DecObj[_VTSP_STREAM_PER_INFC];
    H263_EncodeObj   h263EncObj[_VTSP_STREAM_PER_INFC];
    H263_DecodeObj   h263DecObj[_VTSP_STREAM_PER_INFC];
    VTSP_StreamVideo stream[_VTSP_STREAM_PER_INFC];
    OSAL_Boolean     encLoop[_VTSP_STREAM_PER_INFC];
    OSAL_Boolean     decLoop[_VTSP_STREAM_PER_INFC];
    OSAL_Boolean     encJoined[_VTSP_STREAM_PER_INFC];
    OSAL_Boolean     decJoined[_VTSP_STREAM_PER_INFC];
    OSAL_NetSockId   sock[_VTSP_STREAM_PER_INFC];
    RTP_Obj          decRtp[_VTSP_STREAM_PER_INFC];
    RTP_Obj          encRtp[_VTSP_STREAM_PER_INFC];
    uint8 scratchd[_VTSP_STREAM_PER_INFC][W * H * 2];
    uint8 scratche[_VTSP_STREAM_PER_INFC][W * H * 2];
} _VTSP_VideoObj;

#endif

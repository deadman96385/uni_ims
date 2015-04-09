/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 16581 $ $Date: 2012-02-15 03:38:57 -0800 (Wed, 15 Feb 2012) $
 */
#include <osal_types.h>
#include <osal.h>

#include "isi.h"
#include "isip.h"
#include "vtsp.h"
#include "_mc.h"

void _MC_LipSync_calculateAudioVideoSkew(
    MC_LipSync    *syncEngine_ptr)
{
    uint32 aRtpTs = syncEngine_ptr->aLatestRtp;
    uint32 aNtp = syncEngine_ptr->aNtp;
    uint32 aRtcpRtp = syncEngine_ptr->aRtcpRtp;

    uint32 vRtpTs = syncEngine_ptr->vLatestRtp;
    uint32 vNtp = syncEngine_ptr->vNtp;
    uint32 vRtcpRtp = syncEngine_ptr->vRtcpRtp;

    uint32 aLatency = syncEngine_ptr->aLatency;
    uint32 vLatency = syncEngine_ptr->vLatency;

    uvint  vInfc = syncEngine_ptr->vInfc;
    uint32 vStreamId = syncEngine_ptr->vStreamId;

    uint32 aTime, vTime;
    vint audioVideoSkew;

    MC_dbgPrintf("aRtpTs = %u, aNtp = %u, aRtcpRtp = %u, aLatency = %u",
            aRtpTs, aNtp, aRtcpRtp, aLatency);
    MC_dbgPrintf("vRtpTs = %u, vNtp = %u, vRtcpRtp = %u, vLatency = %u",
            vRtpTs, vNtp, vRtcpRtp, vLatency);

    if (aRtpTs == 0 || aNtp == 0 || aRtcpRtp == 0 ||
            vRtpTs == 0 || vNtp == 0 || vRtcpRtp == 0) {
        /*
         * To calculate audio video skew, we need atleast:
         * one audio RTP, one audio RTCP, one video RTP, video RTP RTCP packet.
         */
        return;
    }

    /*
     * Calculate the audio video presentation time in milliseconds.
     * Include the audio video decode latency.
     */
    aTime = aNtp + ((vint)(aRtpTs - aRtcpRtp)) + aLatency;
    vTime = vNtp + ((vint)(vRtpTs - vRtcpRtp)) + vLatency;
    /* calculate audio video skew. */
    audioVideoSkew = (aTime - vTime);
    MC_dbgPrintf("aTime = %u, vTime = %u, audioVideoSkew = %d",
            aTime, vTime, audioVideoSkew);

    /* Notify the Video Engine about the audioVideo Skew. */
    VTSP_streamVideoLipSync(vInfc, vStreamId, audioVideoSkew);
    return;
}

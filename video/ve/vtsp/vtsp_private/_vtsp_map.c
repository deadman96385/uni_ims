/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 13002 $ $Date: 2010-09-22 01:13:18 +0800 (Wed, 22 Sep 2010) $
 *
 */

#include "osal.h"

#include "vtsp.h"
#include "_vtsp_private.h"


/*
 * ======== _VTSP_isInfcFxs() ========
 * Check if the interface is an FXS
 * XXX Assumes linear interface numbering among interface types, FXS first
 */
VTSP_Return _VTSP_isInfcFxs(
    uvint     infc) 
{

    if ((vint)infc < (vint)_VTSP_object_ptr->config.hw.fxsFirst ||
            (vint)infc > (vint)_VTSP_object_ptr->config.hw.fxsLast) {
        return (VTSP_E_INFC_HW);
    }
    return (VTSP_OK);
}

/*
 * ======== _VTSP_isInfcFxo() ========
 * Check if the interface is an FXO
 * XXX Assumes linear interface numbering among interface types, FXO after FXS
 */
VTSP_Return _VTSP_isInfcFxo(
    uvint     infc) 
{

    if ((vint)infc < (vint)_VTSP_object_ptr->config.hw.fxoFirst ||
            (vint)infc > (vint)_VTSP_object_ptr->config.hw.fxoLast) {
        return (VTSP_E_INFC_HW);
    }
    return (VTSP_OK);
}

/*
 * ======== _VTSP_isInfcAudio() ========
 * Check if the interface is an AUDIO
 * XXX Assumes linear interface numbering among interface types, AUDIO first
 */
VTSP_Return _VTSP_isInfcAudio(
    uvint     infc) 
{

    if ((vint)infc < (vint)_VTSP_object_ptr->config.hw.audioFirst ||
            (vint)infc > (vint)_VTSP_object_ptr->config.hw.audioLast) {
        return (VTSP_E_INFC_HW);
    }
    return (VTSP_OK);
}

/*
 * ======== _VTSP_isInfcValid() ========
 * Check if the interface number is valid
 * XXX Assumes linear interface numbering among interface types
 * XXX Assumes interfaces are numbered ( FXS .. FXO .. AUDIO )
 */
VTSP_Return _VTSP_isInfcValid(
    uvint     infc) 
{

    if ((vint)infc < (vint)_VTSP_object_ptr->config.hw.fxsFirst ||
            (vint)infc >= (vint)_VTSP_object_ptr->config.hw.numInfc) {
        return (VTSP_E_INFC);
    }
    return (VTSP_OK);
}

/*
 * ========  _VTSP_streamIdToRtcpSktPtr() ========
 */
OSAL_INLINE _VTSP_RtcpSktObj* _VTSP_streamIdToRtcpSktPtr(
    _RTCP_SktTaskContext *sktTask_ptr,
    uvint                 infc, 
    uvint                 streamId)
{
    uvint  rtcpIndex;

    if (VTSP_INFC_VIDEO == infc) {
        rtcpIndex = streamId;
        return (sktTask_ptr->rtcpSktVideo_ptr[rtcpIndex]);
    }
    rtcpIndex = (streamId + (infc * _VTSP_STREAM_PER_INFC));

    return (sktTask_ptr->rtcpSkt_ptr[rtcpIndex]);
}


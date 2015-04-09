/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12772 $ $Date: 2010-08-20 05:05:49 +0800 (Fri, 20 Aug 2010) $
 *
 */

#include "osal.h"

#include "vtsp.h"
#include "_vtsp_private.h"

/*
 * ======== _VTSP_isStreamVideoIdValid() ========
 *
 *  check streamId variable is valid for the infc
 *
 *  return:
 *
 *  VTSP_OK if valid
 *  VTSP_E_STREAM_NUM if invalid
 */
VTSP_Return _VTSP_isStreamVideoIdValid(
    uvint infc, 
    uvint streamId)
{
    if (streamId < _VTSP_object_ptr->config.stream.numPerInfc) {
        return (VTSP_OK);
    }

    return (VTSP_E_STREAM_NUM);
}

/*
 * ======== _VTSP_isStreamVideoDirValid() ========
 *
 *  check stream direction variable is valid for the infc & streamId
 *
 *  return:
 *
 *  VTSP_OK if valid
 *  VTSP_E_STREAM_DATA if invalid
 */
VTSP_Return _VTSP_isStreamVideoDirValid(
    uvint infc, 
    uvint streamId, 
    uvint dir)
{
    if (VTSP_STREAM_DIR_SENDRECV == dir ||
            VTSP_STREAM_DIR_SENDONLY == dir ||
            VTSP_STREAM_DIR_RECVONLY == dir ||
            VTSP_STREAM_DIR_INACTIVE == dir) { 
        return (VTSP_OK);
    }

    return (VTSP_E_STREAM_DATA);
}

/*
 * ======== _VTSP_isStreamVideoConfMaskValid() ========
 *
 *  check stream confMask variable is valid for the infc & streamId
 *
 *  return:
 *
 *  VTSP_OK if valid
 *  VTSP_E_STREAM_DATA if invalid
 */
VTSP_Return _VTSP_isStreamVideoConfMaskValid(
    uvint  infc, 
    uvint  streamId, 
    uint32 confMask)
{
    uint32  bitMask;

    /* No conf bits set (common case)
     */
    if (0 == confMask) {  
        return (VTSP_OK);
    }

    /* create mask with all bits set for valid streamId range */
    bitMask =   ~((1 << _VTSP_object_ptr->config.stream.numPerInfc) - 1);

    confMask &= bitMask;
    if (confMask != 0) { 
        /* out-of-range bits are set */ 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (VTSP_E_STREAM_DATA);
    }

    /* 
     * there is no way to verify if confMask bit is an open/valid stream
     * without knowing the 'peer' in streamData
     */

    return (VTSP_OK);


}

/*
 * ======== _VTSP_isStreamVideoPeerValid ========
 *
 *
 * return: 
 *
 * VTSP_OK if peer valid 
 * VTSP_E_STREAM_DATA if peer is invalid
 *
 */
VTSP_Return _VTSP_isStreamVideoPeerValid(
    uvint             infc,
    VTSP_StreamVideo *stream_ptr) 
{
    /* Check peer parameter
     */
    if ((VTSP_STREAM_PEER_NETWORK != stream_ptr->peer) &&
            stream_ptr->peer >= _VTSP_object_ptr->config.hw.numInfc) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (VTSP_E_STREAM_DATA);
    }

    return (VTSP_OK);
}


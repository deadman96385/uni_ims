/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 17840 $ $Date: 2012-08-03 01:06:57 +0800 (Fri, 03 Aug 2012) $
 *
 */

#include "osal.h"
#include "vtsp.h"
#include "_vtsp_private.h"

/*
 * ======== _VTSP_flowGetFlowIndex() ========
 */
uvint _VTSP_flowGetIndex(
    vint flowId)
{
    uvint flowIndex;

    flowIndex = (_VTSP_flowGetInterface(flowId) * _VTSP_FLOW_PER_INFC) +
            _VTSP_flowGetStreamId(flowId);
    return (flowIndex);
}

/*
 * ======== _VTSP_flowGetInterface() ========
 *
 * Return the interface associated with the flow identifier.
 *
 * return:
 *
 * interface
 */
uvint _VTSP_flowGetInterface(
    vint flowId)
{
    return ((flowId >> 16) & 0xff);
}

/*
 * ======== _VTSP_flowGetKey() ========
 *
 * Return the key associated with the flow identifier.
 *
 * return:
 *
 * interface
 */
uvint _VTSP_flowGetKey(
    vint flowId)
{
    return ((flowId >> 24) & 0xff);
}

/*
 * ======== _VTSP_flowGetStreamId() ========
 *
 * Return the streamId associated with the flow identifier.
 *
 * return:
 *
 * streamId
 */
uvint _VTSP_flowGetStreamId(
    vint flowId)
{
    return (flowId & 0xff);
}

/*
 * ======== _VTSP_flowGetId() ========
 *
 *  Calculate the flow identifier based on the interface and stream ID.
 *
 *  return:
 *
 *  flow identifier.
 */
vint _VTSP_flowGetId(
    uvint infc, 
    uvint streamId,
    uvint key)
{
    vint flowId;

    /*
     * Compute a flow identifier based on the interface, stream ID, and key
     * number. Note this only works for 32 bit machines with less than 256
     * interfaces, streams, and keys.
     */
    flowId  = (key << 24) & 0xff000000;
    flowId |= ((infc << 16)   & 0x00ff0000);
    flowId |= (streamId & 0x000000ff);
    return (flowId);
}
/*
 * ======== _VTSP_isFlowBlockSizeValid() ========
 *
 *  Check that the blockSize does not exceed the maximum payload size for the
 *  flow queue.
 *
 *  return:
 *
 *  VTSP_OK if valid
 *  VTSP_E_BUSY if invalid
 */
VTSP_Return _VTSP_isFlowBlockSizeValid(
    uvint  blockSize) 
{
    if ((0 == blockSize) || (blockSize > _VTSP_Q_FLOW_PAYLOAD_SZ)) {
        return (VTSP_E_FLOW_DATA);
    }
    return (VTSP_OK);
}
/*
 * ======== _VTSP_isFlowControlValid() ========
 *
 *  Check to see that a valid flow control mask is present.
 *
 *  return:
 *
 *  VTSP_OK if valid
 *  VTSP_E_BUSY if invalid
 */
VTSP_Return _VTSP_isFlowControlValid(
    uint32  control) 
{
    if ((control & (~VTSP_FLOW_STOP_ANY)) != 0) {
        return (VTSP_E_CONTROL);
    }
    return (VTSP_OK);
}

/*
 * ======== _VTSP_isFlowDirValid() ========
 *
 *  check flow direction variable
 *
 *  return:
 *
 *  VTSP_OK if valid
 *  VTSP_E_FlOW_DIR if invalid
 */
VTSP_Return _VTSP_isFlowDirValid(
    uvint dir)
{
    if ((dir & (~(VTSP_FLOW_DIR_LOCAL_PLAY | VTSP_FLOW_DIR_PEER_PLAY
#ifdef VTSP_ENABLE_RECORD
            | VTSP_FLOW_DIR_LOCAL_RECORD | VTSP_FLOW_DIR_PEER_RECORD
#endif
            ))) == 0) {
        return (VTSP_OK);
    }

    return (VTSP_E_FLOW_DIR);
}

/*
 * ======== _VTSP_isFlowKeyValid() ========
 *
 *  check key parameter
 *
 *  return:
 *
 *  VTSP_OK if valid
 *  VTSP_E_FlOW_SESSION if invalid
 */
VTSP_Return _VTSP_isFlowKeyValid(
    uvint key)
{
    if (_VTSP_Q_FLOW_PUT_MASK < key) {
        return (VTSP_E_FLOW_SESSION);
    }
    return (VTSP_OK);
}

/*
 * ======== _VTSP_isFlowIdle() ========
 *
 *  check to see that the flow has been marked idle
 *
 *  return:
 *
 *  VTSP_OK if valid
 *  VTSP_E_BUSY if invalid
 */
VTSP_Return _VTSP_isFlowIdle(
    vint  flowId) 
{
    return (VTSP_OK);
}

/*
 * ======== _VTSP_isFlowOpen() ========
 *
 *  check to see that the flow has been marked open
 *
 *  return:
 *
 *  VTSP_OK if valid
 *  VTSP_E_BUSY if invalid
 */
VTSP_Return _VTSP_isFlowOpen(
    vint  flowId) 
{
    return (VTSP_OK);
}

/*
 * ======== _VTSP_isFlowValid() ========
 *
 * Check to see that the flow is valid.
 */
VTSP_Return _VTSP_isFlowValid(
    vint flowId)
{
    uvint infc;

    infc = _VTSP_flowGetInterface(flowId);

    if ((_VTSP_isInfcValid(infc) != VTSP_OK) ||
            (_VTSP_isStreamIdValid(infc, _VTSP_flowGetStreamId(flowId)) !=
            VTSP_OK)) {
        return (VTSP_E_FLOW_ID);
    }
    return (VTSP_OK);
}

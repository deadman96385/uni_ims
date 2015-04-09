/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 20759 $ $Date: 2013-05-22 00:13:21 +0800 (Wed, 22 May 2013) $
 *
 */

#include "osal.h"
#include "vtsp.h"
#include "../vtsp_private/_vtsp_private.h"

/*
 * ======== VTSP_flowOpen() ========
 */
vint VTSP_flowOpen(
    uvint infc,
    uvint streamId,
    uvint flowDir,
    uvint key,
    uvint recordCoder)
{
    vint         flowId;
    VTSP_Return  retVal;
    _VTSP_CmdMsg cmd;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;

    /*
     * First check for valid interface.
     */
    if (VTSP_OK != (retVal = _VTSP_isInfcValid(infc))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (retVal);
    }
    /*
     * Check for valid stream.
     */
    if (VTSP_OK != (retVal = _VTSP_isStreamIdValid(infc, streamId))) { 
        return (retVal);
    }
    /*
     * Check for a valid flow direction.
     */
    if (VTSP_OK != (retVal = _VTSP_isFlowDirValid(flowDir))) { 
        return (retVal);
    }
    /*
     * Check for a valid key number.
     */
    if (VTSP_OK != (retVal = _VTSP_isFlowKeyValid(key))) {
        return (retVal);
    }

    /*
     * Calculate a flow ID.
     */
    flowId = _VTSP_flowGetId(infc, streamId, key);

    /*
     * Check to see that the flow is idle.
     */
    if (VTSP_OK != (retVal = _VTSP_isFlowIdle(flowId))) { 
        return (retVal);
    }

    /*
     * Now tell vTSPR that the flow has been opened.
     */
    cmd.code = _VTSP_CMD_FLOW_OPEN;
    cmd.infc = infc;
    cmd.msg.flowOpen.flowIndex = _VTSP_flowGetIndex(flowId);
    cmd.msg.flowOpen.flowId = flowId;
    cmd.msg.flowOpen.streamId = streamId;
    cmd.msg.flowOpen.flowDir = flowDir;
    cmd.msg.flowOpen.key = key;
    cmd.msg.flowOpen.coder = recordCoder;

    _VTSP_putCmd(infc, &cmd, 0);

    /*
     * All checks have passed, return flow identifier.
     */
    return (flowId);
}


/*
 * ======== VTSP_flowPlay() ========
 */
VTSP_Return VTSP_flowPlay(
    vint   flowId,
    uvint  coder,
    uvint  blockSize,
    void  *data_ptr,
    uint32 control,
    uint32 timeout)
{
    uvint         infc;
    uvint         flowNum;
    VTSP_Return   retVal;
    OSAL_MsgQId   qId;
    OSAL_Boolean  timeo;
    vint          osalStatus;
    _VTSP_FlowMsg message;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;


    /*
     * Check to see that the flow indentifier is valid.
     */
    if (VTSP_OK != (retVal = _VTSP_isFlowValid(flowId))) { 
        return (retVal);
    }

    /*
     * Check to see that the flow has been opened.
     */
    if (VTSP_OK != (retVal = _VTSP_isFlowOpen(flowId))) { 
        return (retVal);
    }

    /*
     * Check to see that blockSize is valid.
     */
    if (VTSP_OK != (retVal = _VTSP_isFlowBlockSizeValid(blockSize))) { 
        return (retVal);
    }

    /*
     * Check to see that the control is valid.
     */
    if (VTSP_OK != (retVal = _VTSP_isFlowControlValid(control))) { 
        return (retVal);
    }

    /*
     * Check for invalid coder types
     */
    if ((coder != VTSP_CODER_G711U)
            && (coder != VTSP_CODER_G711A)
#ifdef VTSP_ENABLE_G729
            && (coder != VTSP_CODER_G729)
#endif
#ifdef VTSP_ENABLE_G726
            && (coder != VTSP_CODER_G726_32K)
#endif
#ifdef VTSP_ENABLE_ILBC
            && (coder != VTSP_CODER_ILBC_20MS)
            && (coder != VTSP_CODER_ILBC_30MS)
#endif
#ifdef VTSP_ENABLE_G723
            && (coder != VTSP_CODER_G723_30MS)
#endif
#ifdef VTSP_ENABLE_SILK
            && (coder != VTSP_CODER_SILK_20MS_8K)
            && (coder != VTSP_CODER_SILK_20MS_16K)
            && (coder != VTSP_CODER_SILK_20MS_24K)
#endif            
            ) {
        return (VTSP_E_CODER_TYPE);
    }

    /*
     * Now place the data in the proper queue.
     *
     * Because of load balancing flowNum and StreamId are identical.
     */
    infc = _VTSP_flowGetInterface(flowId);
    flowNum = _VTSP_flowGetStreamId(flowId);
    qId = _VTSP_object_ptr->infc[infc].flowDataFromAppQ[flowNum];

    message.coder     = coder;
    message.blockSize = blockSize;
    message.key       = _VTSP_flowGetKey(flowId);
    message.control   = control;
    message.duration  = 0;

    OSAL_memCpy(message.payload, data_ptr, blockSize);

    /*
     * This send is inefficient unless the blockSize is the max payload size.
     * This is anticipated to be the normal opperation as there is no reason to
     * parse data in smaller blocks unless CN or SID blocks exist or that the
     * last voice blocks do not fill an entire payload.
     */
    osalStatus = OSAL_msgQSend(qId, (char *)&message, sizeof(_VTSP_FlowMsg),
            timeout, &timeo);

    if (OSAL_SUCCESS != osalStatus) { 
        if (OSAL_TRUE == timeo) { 
            return (VTSP_E_TIMEOUT);
        }
        return (VTSP_E_RESOURCE);
    }

    return (VTSP_OK);
}


/*
 * ======== VTSP_flowPlaySil() ========
 */
VTSP_Return VTSP_flowPlaySil(
    vint   flowId,
    uvint  coder,
    uvint  blockSize,
    void  *data_ptr,
    uint32 duration,
    uint32 control,
    uint32 timeout)
{
    uvint         infc;
    uvint         flowNum;
    VTSP_Return   retVal;
    OSAL_MsgQId   qId;
    OSAL_Boolean  timeo;
    vint          osalStatus;
    _VTSP_FlowMsg message;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;

    /*
     * Check to see that the flow indentifier is valid.
     */
    if (VTSP_OK != (retVal = _VTSP_isFlowValid(flowId))) { 
        return (retVal);
    }

    /*
     * Check to see that the flow has been opened.
     */
    if (VTSP_OK != (retVal = _VTSP_isFlowOpen(flowId))) { 
        return (retVal);
    }

    /*
     * Check to see that the control word is valid.
     */
    if (VTSP_OK != (retVal = _VTSP_isFlowControlValid(control))) { 
        return (retVal);
    }

    /*
     * Check valid coder types. The coder can either be a CN block or a SID
     * block. Each type has a specified block size.
     */
    if ((coder != VTSP_CODER_CN)
#ifdef VTSP_ENABLE_G729
            && (coder != VTSP_CODER_G729)
#endif
            ) {
        return (VTSP_E_CODER_TYPE);
    }

    /*
     * Now place the data in the proper queue.
     */
    infc = _VTSP_flowGetInterface(flowId);
    flowNum = _VTSP_flowGetStreamId(flowId);
    qId = _VTSP_object_ptr->infc[infc].flowDataFromAppQ[flowNum];

    message.coder     = coder;
    message.blockSize = blockSize;
    message.key       = _VTSP_flowGetKey(flowId);
    message.control   = control;
    message.duration  = duration;

    OSAL_memCpy(message.payload, data_ptr, blockSize);

    osalStatus = OSAL_msgQSend(qId, (char *)&message, sizeof(_VTSP_FlowMsg),
            timeout, &timeo);

    if (OSAL_SUCCESS != osalStatus) { 
        if (OSAL_TRUE == timeo) { 
            return (VTSP_E_TIMEOUT);
        }
        return (VTSP_E_RESOURCE);
    }

    return (VTSP_OK);
}
/*
 * ======== VTSP_flowRecord() ========
 *
 * Copy at most blockSize bytes of data to the application. The function returns
 * the actual number of bytes copies. If the return value is less than zero an
 * error has been detected. The error is coded as a negative number.
 */
vint VTSP_flowRecord(
    vint    flowId,
    uvint   blockSize,
    uvint  *coder,
    void   *data_ptr,
    uint32 *duration,
    uint32  timeout)
{
    uvint         infc;
    uvint         flowNum;
    uvint         key;
    VTSP_Return   retVal;
    OSAL_MsgQId   qId;
    vint          readSize;
    _VTSP_FlowMsg message;
    OSAL_Boolean  timeo;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;

    /*
     * Check to see that the flow indentifier is valid.
     */
    if (VTSP_OK != (retVal = _VTSP_isFlowValid(flowId))) { 
        return (retVal);
    }

    /*
     * Check to see that the flow has been opened.
     */
    if (VTSP_OK != (retVal = _VTSP_isFlowOpen(flowId))) { 
        return (retVal);
    }

    /*
     * Now check the queue for data. If the key does not match the flow key,
     * discard the data and read again.
     */
    infc = _VTSP_flowGetInterface(flowId);
    flowNum = _VTSP_flowGetStreamId(flowId);
    qId = _VTSP_object_ptr->infc[infc].flowDataToAppQ[flowNum];
    key = _VTSP_flowGetKey(flowId);

    /*
     * The timeout for this section must be NO_WAIT because the application
     * writer may specify the wrong key with a timeout that will never be
     * exceeded. In this case, the application writer will think that the
     * function has hung. In reality, the loop would never be broken to report a
     * key mismatch.
     */
    while ((readSize = OSAL_msgQRecv(qId, (char *)&message,
            sizeof(_VTSP_FlowMsg), VTSP_TIMEOUT_NO_WAIT, &timeo)) > 0) {
        if (message.key == key) {
            break;
        }
    }
    if (readSize <= 0) {
        if (OSAL_FALSE == timeo) {
            return (VTSP_E_RESOURCE);
        }
        
        if ((readSize = OSAL_msgQRecv(qId, (char *)&message,
                sizeof(_VTSP_FlowMsg), timeout, &timeo)) <= 0) {
            if (OSAL_FALSE == timeo) {
                return (VTSP_E_RESOURCE);
            }
            _VTSP_TRACE(__FILE__, __LINE__);
            return (VTSP_E_RESOURCE);
        }
        /*
         * If got a payload with the wrong key at this point, report a data
         * error.
         */
        if (message.key != key) {
            return (VTSP_E_FLOW_DATA);
        }
    }

    readSize = message.blockSize;

    if (blockSize > _VTSP_Q_FLOW_PAYLOAD_SZ) {
        blockSize = _VTSP_Q_FLOW_PAYLOAD_SZ;
    }
    else if (blockSize > (uvint)readSize) {
        blockSize = readSize;
    }
        
    OSAL_memCpy(data_ptr, message.payload, blockSize);
    *duration = message.duration;
    *coder = message.coder;
    return (readSize);
}
/*
 * ======== VTSP_flowClose() ========
 */
VTSP_Return VTSP_flowClose(
    vint flowId)
{
    _VTSP_CmdMsg cmd;
    VTSP_Return  retVal;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;

    /*
     * Check to see that the flow indentifier is valid.
     */
    if (VTSP_OK != (retVal = _VTSP_isFlowValid(flowId))) { 
        return (retVal);
    }

    /*
     * Mark the flow as closing so that play functions will no longer accept
     * data. This status is sent to VTSPR so that play is terminated when the
     * queue does not have any more data available.
     */
    cmd.code = _VTSP_CMD_FLOW_CLOSE;
    cmd.infc = _VTSP_flowGetInterface(flowId);
    cmd.msg.flowOpen.flowIndex = _VTSP_flowGetIndex(flowId);
    cmd.msg.flowOpen.streamId = _VTSP_flowGetStreamId(flowId);

    _VTSP_putCmd(cmd.infc, &cmd, 0);

    return (VTSP_OK);
}

/*
 * ======== VTSP_flowAbort() ========
 */
VTSP_Return VTSP_flowAbort(
    vint   flowId,
    uint32 timeout)
{
    uvint         infc;
    uvint         flowNum;
    uvint         msgSize;
    OSAL_Boolean  timeo;
    vint          osalStatus;
    _VTSP_CmdMsg  cmd;
    _VTSP_FlowMsg message;
    VTSP_Return   retVal;
    OSAL_MsgQId   qId;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;

    /*
     * Check to see that the flow indentifier is valid.
     */
    if (VTSP_OK != (retVal = _VTSP_isFlowValid(flowId))) { 
        return (retVal);
    }

    /*
     * Put a "abort" payload on the payload queue. This marks the end of the
     * current flow.
     */
    infc = _VTSP_flowGetInterface(flowId);
    flowNum = _VTSP_flowGetStreamId(flowId);
    qId = _VTSP_object_ptr->infc[infc].flowDataFromAppQ[flowNum];

    message.coder     = VTSP_CODER_ABORT;
    message.blockSize = 0;
    message.key       = _VTSP_flowGetKey(flowId);
    message.control   = 0;
    message.duration  = 0;

    /*
     * Since there is no payload, the message size can be reduced to the
     * non-payload portion of the structure. This works as long as the payload
     * array is the last element in the structure.
     */
    msgSize = sizeof(_VTSP_FlowMsg) - _VTSP_Q_FLOW_PAYLOAD_SZ;
    osalStatus = OSAL_msgQSend(qId, (char *)&message, msgSize, timeout,
            &timeo);

    if (OSAL_SUCCESS != osalStatus) { 
        if ((OSAL_TRUE == timeo) || ((VTSP_TIMEOUT_NO_WAIT == timeout))) {
            return (VTSP_E_TIMEOUT);
        }
        _VTSP_TRACE(__FILE__, __LINE__);
        return (VTSP_E_RESOURCE);
    }    

    /*
     * Send message to VTSP that the flow must be closed and flushed.
     */
    cmd.code = _VTSP_CMD_FLOW_ABORT;
    cmd.infc = _VTSP_flowGetInterface(flowId);
    cmd.msg.flow.flowIndex = _VTSP_flowGetIndex(flowId);
    cmd.msg.flow.streamId = _VTSP_flowGetStreamId(flowId);

    _VTSP_putCmd(cmd.infc, &cmd, 0);

    return (VTSP_OK);
}

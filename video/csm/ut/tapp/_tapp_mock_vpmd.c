/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 19918 $ $Date: 2013-02-25 23:29:12 -0800 (Mon, 25 Feb 2013) $
 */

#include <osal.h>
#include <isi.h>
#include <isip.h>
#include <csm_event.h>
#include "_tapp.h"
#include <ezxml.h>
#include "_tapp_mock_vpmd.h"

extern TAPP_GlobalObj *global_ptr;

/*
 * ======== _TAPP_sappSetAvilableId() ========
 *
 * This function is used to set call id to avilable index of global_ptr->callId.
 * 
 * Return Values:
 *   TAPP_PASS: can get avilable id index.
 *   TAPP_FAIL: no avilable id index.
 */
static vint _TAPP_mockVpmdSetAvilableCid(
    TAPP_GlobalObj *global_ptr,
    vint          callId)
{
    vint index;
    vint ret = TAPP_FAIL;

    for (index = 0; index < TAPP_MAX_CALL_NUMS; index ++) {
        if (0 == global_ptr->isiCCallId[index]) {
            /* get empty and set call ID  */
            global_ptr->isiCCallId[index] = callId;
            ret = TAPP_PASS;
            break;
        }
    }
    return (ret);
}

/*
 * ======== _TAPP_mockVpmdSetServiceId() ========
 *
 * This function is used to set service id to global_ptr->isiCServiceId[]
 * 
 * Return Values:
 *   service id index
 *   -1 : failed.
 */
vint _TAPP_mockVpmdSetServiceId(
    TAPP_GlobalObj *global_ptr,
    vint            id)
{
    vint idx;

    for (idx =0; idx < TAPP_MAX_SERVICE_NUMS; idx++) {
        if (global_ptr->isiCServiceId[idx] == 0) {
            global_ptr->isiCServiceId[idx] = id;
            TAPP_dbgPrintf("%s isi client id:%d, idx:%d\n", __FUNCTION__,
                    id, idx);
            return (idx);
        }
    }

    return (-1);
}


/*
 * ======== TAPP_mockVpmdIssueIsiRpc() ========
 *
 * This function is used to issue an ISI server event.
 * 
 * Return Values:
 *   TAPP_PASS:send isip message sucessfully.
 *   TAPP_FAIL: send isip message failed.
 */
vint TAPP_mockVpmdIssueIsiRpc(
    TAPP_GlobalObj *global_ptr,
    TAPP_Action    *action_ptr)
{
    RPC_Message   *rpc_ptr;
    ISI_Xdr        xdrMsg;
    if (NULL == action_ptr || NULL == global_ptr) {
        return (TAPP_FAIL);
    }
    rpc_ptr = &action_ptr->msg.rpcMsg;
    /* Duplicate original one in case it's modifed */
    xdrMsg = rpc_ptr->isiXdr;
    if (TAPP_ID_TYPE_NONE != rpc_ptr->id.idType) {
        /* Get put integer index */
        xdrMsg.cur_ptr = &xdrMsg.data[4 * rpc_ptr->id.paramIdx];
        if (TAPP_ID_TYPE_CALL == rpc_ptr->id.idType) {
            ISI_xdrPutInteger(&xdrMsg,
                    global_ptr->isiCCallId[rpc_ptr->id.idIdx]);
        }
        else {
            ISI_xdrPutInteger(&xdrMsg,
                    global_ptr->isiCServiceId[rpc_ptr->id.idIdx]);
        }
    }
    
    if (OSAL_SUCCESS != OSAL_msgQSend(global_ptr->queue.rpcWrite,
            xdrMsg.data, ISI_DATA_SIZE, 
            OSAL_WAIT_FOREVER, NULL)) {
        OSAL_logMsg("%s:%d ERROR msgQ send FAILED qId=%p\n", __FUNCTION__,
                __LINE__, global_ptr->queue.rpcWrite);
        return (TAPP_FAIL);
    }
    return (TAPP_PASS);
}

/*
 * ======== TAPP_mockVpmdValidateIsiRpc() ========
 *
 * This function is used to validate isi server message.
 * 
 * Return Values:
 *   TAPP_PASS: the result is same the expected value.
 *   TAPP_FAIL: the result is different with the expected value.
 */
vint TAPP_mockVpmdValidateIsiRpc (
    TAPP_GlobalObj *global_ptr,
    TAPP_Action    *action_ptr) 
{
    TAPP_Event       *tappEvt_ptr;
    ISI_Xdr          *xdrMsg_ptr;   /* for getting input xdr message */
    RPC_Message       tappMsg;      /* for expect rpc message */
    vint              typeIdx;
    ISI_Id            serviceId;
    int               isiTmpInt;
    int               tappTmpInt;
    int               idFromEvt;
    int               idFromTapp;
    char              isiTmpStr[ISI_DATA_SIZE];
    char              tappTmpStr[ISI_DATA_SIZE];

    if (NULL == action_ptr || NULL == global_ptr) {
        return (TAPP_FAIL);
    }
    typeIdx = 0;
    tappEvt_ptr = &global_ptr->tappEvt;
    /* Duplicate original one in case it's modifed */
    tappMsg     = action_ptr->msg.rpcMsg;
    if (TAPP_PASS != TAPP_getInputEvent(global_ptr, action_ptr,
            tappEvt_ptr, action_ptr->u.timeout)) {
        return (TAPP_FAIL);

    }
    
    if (TAPP_EVENT_TYPE_ISI_RPC != tappEvt_ptr->type) {
        return (TAPP_FAIL);
    }
    xdrMsg_ptr = &tappEvt_ptr->msg.rpc.isiXdr;
    /* Because TAPP_getInputEvent() would cleanup isiXdr, init cur_ptr here */
    xdrMsg_ptr->cur_ptr = &xdrMsg_ptr->data[0];
    tappMsg.isiXdr.cur_ptr = &tappMsg.isiXdr.data[0];

    /* Get returned value and validate */
    ISI_xdrGetInteger(xdrMsg_ptr, &isiTmpInt);
    ISI_xdrGetInteger(&tappMsg.isiXdr, &tappTmpInt);     
    typeIdx++;
    if (tappTmpInt != isiTmpInt && OSAL_FALSE == tappMsg.isDontCare[0]) {
        return (TAPP_FAIL);
    }
    switch (tappMsg.funcType) {
        case ISI_GET_NEXT_SERVICE:
            /* get the serviceId  from vpmd*/
            ISI_xdrGetInteger(xdrMsg_ptr, (int *)&serviceId);
            /* get the protocol  from vpmd*/
            ISI_xdrGetInteger(xdrMsg_ptr, &isiTmpInt);
            if (ISI_PROTOCOL_SIP == isiTmpInt) {
                /* get emergencye */
                ISI_xdrGetInteger(xdrMsg_ptr, &isiTmpInt);
                /* isEmergency */
                if (OSAL_FALSE == isiTmpInt) {
                    _TAPP_mockVpmdSetServiceId(global_ptr, serviceId);
                }
            }
            break;
        case ISI_INITIATE_CALL:
            /* get call id*/
            ISI_xdrGetInteger(xdrMsg_ptr, &isiTmpInt);
            
            return (_TAPP_mockVpmdSetAvilableCid(global_ptr, isiTmpInt));
        case ISI_GET_EVENT:
            /* Get service Id */
            ISI_xdrGetInteger(xdrMsg_ptr, &isiTmpInt);
            ISI_xdrGetInteger(&tappMsg.isiXdr, &tappTmpInt);
            if (global_ptr->isiCServiceId[tappTmpInt] != isiTmpInt &&
                    OSAL_FALSE == tappMsg.isDontCare[1]) {
                return (TAPP_FAIL);
            }
            /* Get id */
            ISI_xdrGetInteger(xdrMsg_ptr, &idFromEvt);
            ISI_xdrGetInteger(&tappMsg.isiXdr, &idFromTapp);
 
            /* Get id Type*/
            ISI_xdrGetInteger(xdrMsg_ptr, &isiTmpInt);
            ISI_xdrGetInteger(&tappMsg.isiXdr, &tappTmpInt);
            /* Check id type */
            if (OSAL_FALSE == tappMsg.isDontCare[3] && 
                    isiTmpInt != tappTmpInt){
                return (TAPP_FAIL);
            }
            /* validate id */
            if (OSAL_FALSE == tappMsg.isDontCare[2]) {
                if (isiTmpInt == ISI_ID_TYPE_CALL) {
                    if (global_ptr->isiCCallId[idFromTapp] != idFromEvt) {
                        return (TAPP_FAIL);
                    }
                }
                else if (isiTmpInt == ISI_ID_TYPE_SERVICE) {
                    if (global_ptr->isiCServiceId[idFromTapp] != idFromEvt) {
                        return (TAPP_FAIL);
                    }
                }
            }
            /* get event id */
            ISI_xdrGetInteger(xdrMsg_ptr, &isiTmpInt);
            ISI_xdrGetInteger(&tappMsg.isiXdr, &tappTmpInt);

            if (OSAL_FALSE == tappMsg.isDontCare[4]  &&
                    isiTmpInt != tappTmpInt) {
                return (TAPP_FAIL);
            }
            /* If a new incoming call save call id */
            if (ISI_EVENT_CALL_INCOMING == isiTmpInt) {
                _TAPP_mockVpmdSetAvilableCid(global_ptr, idFromEvt);
            }
            /* if call finished, clear call id */
            else if ((ISI_EVENT_CALL_REJECTED == isiTmpInt) ||
                        (ISI_EVENT_CALL_FAILED == isiTmpInt) ||
                        (ISI_EVENT_NET_UNAVAILABLE == isiTmpInt) ||
                        (ISI_EVENT_CALL_DISCONNECTED == isiTmpInt)) {
                global_ptr->isiCCallId[idFromTapp] = 0;
            }
            break;          
        default:
            while (0 != tappMsg.dataType[typeIdx]) {
                if (TAPP_ISI_RPC_DATA_TYPE_INT == tappMsg.dataType[typeIdx]) {
                    ISI_xdrGetInteger(xdrMsg_ptr, &isiTmpInt);
                    ISI_xdrGetInteger(&tappMsg.isiXdr, &tappTmpInt);
                    if (OSAL_FALSE == tappMsg.isDontCare[typeIdx]) {
                        if (isiTmpInt != tappTmpInt) {
                            return (TAPP_FAIL);
                        }
                    }
                }
                /* Get and validate string from data */
                else if (TAPP_ISI_RPC_DATA_TYPE_STR == tappMsg.dataType[typeIdx]) {
                    ISI_xdrGetString(xdrMsg_ptr, isiTmpStr, &isiTmpInt);
                    ISI_xdrGetString(&tappMsg.isiXdr, tappTmpStr, &tappTmpInt);
                    if (OSAL_strncmp(isiTmpStr, tappTmpStr, tappTmpInt) &&
                            OSAL_FALSE == tappMsg.isDontCare[typeIdx]) {
                        return(OSAL_FAIL);
                    }
                }
                else {
                    return (TAPP_FAIL);
                }
                typeIdx ++;
            }
            break;
    }   
    return (TAPP_PASS);
}

/*
 *  ======== VPMD_writeIsiRpc() ========
 *  This function is mock vpmd used to write ISI function return to VPAD.
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to write.
 *  OSAL_FAIL: fail to write.
 */
OSAL_Status VPMD_writeIsiRpc(
    void *buf_ptr,
    vint  size)
{  
    if (OSAL_SUCCESS != OSAL_msgQSend(global_ptr->queue.rpcRead,
            (char *)buf_ptr, size,
            OSAL_WAIT_FOREVER, NULL)) {
        return (OSAL_FAIL);
    }
 
    return (OSAL_SUCCESS);
}

/*
 *  ======== VPMD_readIsiRpc() ========
 *  This function is mock vpmd used to read ISI function call from Q.
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read.
 *  OSAL_FAIL: fail to read.
 */
OSAL_Status VPMD_readIsiRpc(
    void *buf_ptr,
    vint  size,
    vint  timeout)
{
    if (0 >= OSAL_msgQRecv(global_ptr->queue.rpcWrite,
            (char *)buf_ptr,
            size,
            timeout < 0 ? OSAL_WAIT_FOREVER : timeout,
            NULL)) {
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 *  ======== VPMD_writeIsiEvtRpc() ========
 *  This function is  mock vpmd used to write ISI events and return to VPAD.
 *  This is for ISI_getEvent().
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to write.
 *  OSAL_FAIL: fail to write.
 */
OSAL_Status VPMD_writeIsiEvtRpc(
    void *buf_ptr,
    vint  size)
{
    if (OSAL_SUCCESS != OSAL_msgQSend(global_ptr->queue.rpcREvt,
            (char *)buf_ptr, size,
            OSAL_WAIT_FOREVER, NULL)) {
        return (OSAL_FAIL);
    }
 
    return (OSAL_SUCCESS);
}

/*
 *  ======== VPMD_readIsiEvtRpc() ========
 *  This function is mock vpmd used to read ISI function call and the function
 *  is, ISI_getEvent(), from VPAD.
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read.
 *  OSAL_FAIL: fail to read.
 */
OSAL_Status VPMD_readIsiEvtRpc(
    void *buf_ptr,
    vint  size,
    vint  timeout)
{
    if (0 >= OSAL_msgQRecv(global_ptr->queue.rpcWEvt,
            (char *)buf_ptr,
            size,
            timeout < 0 ? OSAL_WAIT_FOREVER : timeout,
            NULL)) {
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== TAPP_mockVpmdValidateIsiGetEvt() ========
 *
 * This function is used to get isi event and validate.
 * 
 * Return Values:
 *   TAPP_PASS: the result is same the expected value.
 *   TAPP_FAIL: the result is different with the expected value.
 */
vint TAPP_mockVpmdValidateIsiGetEvt (
    TAPP_GlobalObj *global_ptr,
    TAPP_Action    *action_ptr) 
{
    ISI_Xdr           xdrEvt;

    if (NULL == action_ptr || NULL == global_ptr) {
        return (TAPP_FAIL);
    }

     /* Encode XDR which is sent to VPAD.  */
    if (OSAL_SUCCESS != ISI_xdrEncodeInit(&xdrEvt)) {
        return (ISI_RETURN_FAILED);
    }

    if (OSAL_SUCCESS != ISI_xdrPutInteger(&xdrEvt, ISI_GET_EVENT)) {
        return (ISI_RETURN_FAILED);
    }
    /* timeout */
    if (OSAL_SUCCESS != ISI_xdrPutInteger(&xdrEvt, -1)) {
        return (ISI_RETURN_FAILED);
    }
    if (OSAL_SUCCESS != OSAL_msgQSend(global_ptr->queue.rpcWEvt,
            xdrEvt.data, ISI_DATA_SIZE, 
            OSAL_WAIT_FOREVER, NULL)) {
        OSAL_logMsg("%s:%d ERROR msgQ send FAILED qId=%p\n", __FUNCTION__,
                __LINE__, global_ptr->queue.rpcWEvt);
        return (TAPP_FAIL);
    }
    return (TAPP_mockVpmdValidateIsiRpc(global_ptr, action_ptr));
}

/*
 * ======== TAPP_mockVpmdCleanIsiEvt() ========
 *
 * This function is used to clean isi event.
 * 
 * Return Values:
 *   TAPP_PASS: the result is same the expected value.
 *   TAPP_FAIL: the result is different with the expected value.
 */
vint TAPP_mockVpmdCleanIsiEvt (
    TAPP_GlobalObj *global_ptr,
    TAPP_Action    *action_ptr) 
{
    ISI_Xdr  xdrEvt;
    int      ret;

    if (NULL == action_ptr || NULL == global_ptr) {
        return (TAPP_FAIL);
    }

     /* Encode XDR which is sent to VPAD.  */
    if (OSAL_SUCCESS != ISI_xdrEncodeInit(&xdrEvt)) {
        return (ISI_RETURN_FAILED);
    }

    if (OSAL_SUCCESS != ISI_xdrPutInteger(&xdrEvt, ISI_GET_EVENT)) {
        return (ISI_RETURN_FAILED);
    }
    /* timeout */
    if (OSAL_SUCCESS != ISI_xdrPutInteger(&xdrEvt, -1)) {
        return (ISI_RETURN_FAILED);
    }
    do {
        if (OSAL_SUCCESS != OSAL_msgQSend(global_ptr->queue.rpcWEvt,
                xdrEvt.data, ISI_DATA_SIZE, 
                OSAL_WAIT_FOREVER, NULL)) {
            OSAL_logMsg("%s:%d ERROR msgQ send FAILED qId=%p\n", __FUNCTION__,
                    __LINE__, global_ptr->queue.rpcWEvt);
            return (TAPP_FAIL);
        }
        ret = TAPP_getInputEvent(global_ptr, action_ptr,
                &global_ptr->tappEvt, action_ptr->u.timeout);
        
        if (TAPP_EVENT_TYPE_ISI_RPC != global_ptr->tappEvt.type) {
            return (TAPP_PASS);
        }
      /* if RETURN FAIL ,stop to clean isi server event  */  
    } while (TAPP_PASS == ret);
    return (TAPP_PASS);
}


/*
 * ======== VMPD_init() ========
 *
 * This function is Mock VPMD initial..
 * This function will call the mock sapp initial function
 * 
 * Return Values:
 *   0: initialize successfully.
 *   -1: initialize fialed.
 */
OSAL_Status VPMD_init(
    )
{
    /* Mock VPMD write queue */
    if (0 == (global_ptr->queue.rpcWrite = OSAL_msgQCreate(
            TAPP_MOCK_VPMD_WRITE_Q_NAME, 
            OSAL_MODULE_TAPP, OSAL_MODULE_TAPP, OSAL_DATA_STRUCT_ISI_DATA,
            16, ISI_DATA_SIZE, 0))) {
        TAPP_dbgPrintf("%s:%d Create vpmd WriteQ fail\n", __FUNCTION__,
                __LINE__);
        return (OSAL_FAIL);
    }
    /* mock VPMD read queue */
    if (0 == (global_ptr->queue.rpcRead = OSAL_msgQCreate(
            TAPP_MOCK_VPMD_READ_Q_NAME,
            OSAL_MODULE_TAPP, OSAL_MODULE_TAPP, OSAL_DATA_STRUCT_ISI_DATA,
            16, ISI_DATA_SIZE, 0))) {
        TAPP_dbgPrintf("%s:%d Create vpmd WriteQ fail\n", __FUNCTION__,
                __LINE__);
        OSAL_msgQDelete(global_ptr->queue.rpcWrite);
        return (OSAL_FAIL);
    }
    /* mock VPMD event queue */
    if (0 == (global_ptr->queue.rpcWEvt = OSAL_msgQCreate(
            TAPP_MOCK_VPMD_WEVT_Q_NAME,
            OSAL_MODULE_TAPP, OSAL_MODULE_TAPP, OSAL_DATA_STRUCT_ISI_DATA,
            16, ISI_DATA_SIZE, 0))) {
        TAPP_dbgPrintf("%s:%d Create vpmd EvtQ fail\n", __FUNCTION__,
                __LINE__);
        OSAL_msgQDelete(global_ptr->queue.rpcWEvt);
        return (OSAL_FAIL);
    }
     /* mock VPMD event queue */
    if (0 == (global_ptr->queue.rpcREvt = OSAL_msgQCreate(
            TAPP_MOCK_VPMD_REVT_Q_NAME,
            OSAL_MODULE_TAPP, OSAL_MODULE_TAPP, OSAL_DATA_STRUCT_ISI_DATA,
            16, ISI_DATA_SIZE, 0))) {
        TAPP_dbgPrintf("%s:%d Create vpmd EvtQ fail\n", __FUNCTION__,
                __LINE__);
        OSAL_msgQDelete(global_ptr->queue.rpcREvt);
        return (OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
}

/*
 * ======== VPMD_destroy() ========
 *
 * This function is to shutdown Mock VPMD .
 * 
 * Return Values:
 *   0: initialize successfully.
 *   -1: initialize fialed.
 */
int VPMD_destroy()
{
    if (OSAL_SUCCESS != OSAL_msgQDelete(global_ptr->queue.rpcWrite)) {
        return(-1);
    }
    if (OSAL_SUCCESS != OSAL_msgQDelete(global_ptr->queue.rpcRead)) {
        return(-1);
    }
    if (OSAL_SUCCESS != OSAL_msgQDelete(global_ptr->queue.rpcWEvt)) {
        return(-1);
    }
    if (OSAL_SUCCESS != OSAL_msgQDelete(global_ptr->queue.rpcREvt)) {
        return(-1);
    }
    return (0);
}

/*
 *  Below functions are the MOCK functions of VPMD
 *  They are not used in TAPP, just need empty function for link.
 */

/*
 * Empty function.
 */
OSAL_Status VPMD_writeVideoStream(
    void *buf_ptr,
    vint  size)
{
    return (OSAL_SUCCESS);
}

/*
 * Empty function.
 */
OSAL_Status VPMD_readVideoCmdEvt(
    void *buf_ptr,
    vint  size,
    vint  timeout)
{
    return (OSAL_SUCCESS);
}

/*
 * Empty function.
 */
OSAL_Status VPMD_writeVideoCmdEvt(
    void *buf_ptr,
    vint  size)
{
    return (OSAL_SUCCESS);
}

/*
 * Empty function.
 */
int VPMD_getVoiceStreamReadFd(
    void)
{
    return (1);
}   

/*
 * Empty function.
 */
int VPMD_getVideoCmdEvtReadFd(
    void)
{
    return (1);
}

/*
 * Empty function.
 */
int VPMD_getSipReadFd(
    void)
{
    return (1);
}

/*
 * Empty function.
 */
int VPMD_getIsipReadFd(
    void)
{
    return (1);
}

/*
 * Empty function.
 */
int VPMD_getVideoStreamReadFd(
    void)
{
    return (1);
}

/*
 * Empty function.
 */
int VPMD_getCsmEvtReadFd(
    void)
{
    return (1);
}

/*
 * Empty function.
 */
OSAL_Status VPMD_writeVoiceStream(
    void *buf_ptr,
    vint  size)
{
    return (OSAL_SUCCESS);
}

/*
 * Empty function.
 */
OSAL_Status VPMD_readVideoStream(
    void *buf_ptr,
    vint  size,
    vint  timeout)
{
    return (OSAL_SUCCESS);
}

/*
 * Empty function.
 */
OSAL_Status VPMD_readVoiceStream(
    void *buf_ptr,
    vint  size,
    vint  timeout)
{
    return (OSAL_SUCCESS);
}

/*
 * Empty function.
 */
OSAL_Status VPMD_readIsip(
    void *buf_ptr,
    vint  size,
    vint  timeout)
{
    return (OSAL_SUCCESS);
}

/*
 * Empty function.
 */
OSAL_Status VPMD_writeIsip(
    void *buf_ptr,
    vint  size)
{
    return (OSAL_SUCCESS);
}

/*
 * Empty function.
 */
OSAL_Status VPMD_readSip(
    void *buf_ptr,
    vint  size,
    vint  timeout)
{
    return (OSAL_SUCCESS);
}

/*
 * Empty function.
 */
OSAL_Status VPMD_writeSip(
    void *buf_ptr,
    vint  size)
{
    return (OSAL_SUCCESS);
}

/*
 * Empty function.
 */
OSAL_Status VPMD_readCsmEvt(
    void *buf_ptr,
    vint  size,
    vint  timeout)
{
    return (OSAL_SUCCESS);
}

/*
 * ======== VPMD_allocate() ========
 * VPMD allocation.
 */
OSAL_Status VPMD_allocate(void)
{
    return (VPMD_init());
}


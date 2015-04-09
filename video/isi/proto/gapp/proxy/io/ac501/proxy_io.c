/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30369 $ $Date: 2014-12-11 19:09:13 +0800 (Thu, 11 Dec 2014) $
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif


#include <stdio.h>
#include <stdlib.h>
#include <osal_types.h>
#include <osal.h>
#include <osal_msg.h>
#include <osal_log.h>
#include <csm_event.h>
#include "proxy_io.h"
#include "../../proxy.h"
#include "../../_proxy_ut.h"
#include "../../_cmd_mngr.h"
#include "mux_api.h"
#include "mux_all.h"

#define PRXY_MUX_LINK_ID         (30)
#define PRXY_IO_MESSAGE_SIZE_MAX (512)
#define PRXY_MAX_QUEUE_DEPTH     (16)
/* Global pointer to the PRXY queue  */
OSAL_MsgQId _PRXY_io_q = 0;


typedef struct {
    MUX_ENTITY_T        muxEntity;
    vint                muxLinkReady;
} PRXY_IoObj;

static PRXY_IoObj _PRXY_ioObj;
static PRXY_IoObj *_PRXY_ioObj_ptr = &_PRXY_ioObj;

/*
 * ======== _PRXY_ioDataHandler() ========*
 * This is the read callback, when mux have data to be read.
 * after read data from mux, the callback function, readHandler,
 * will dispatch the data to the corresponding component.
 */
static void _PRXY_ioDataHandler(
    uint8   link_id,     
    uint8  *data_ptr,
    uint32  dataSize)
{
    char data[PRXY_IO_MESSAGE_SIZE_MAX];
    
    OSAL_memSet(data, 0, PRXY_IO_MESSAGE_SIZE_MAX);

    if (1 == dataSize) {
        PRXY_dbgPrintf("%s:%d error !!!! size=1 data:0x%x skip it!\n",
                __FUNCTION__, __LINE__, (unsigned int)*data_ptr);
    }
    if (dataSize > PRXY_IO_MESSAGE_SIZE_MAX) {
        PRXY_dbgPrintf("data size > PRXY_IO_MESSAGE_SIZE_MAX.\n");
        return;
    }
    OSAL_strncpy(data, (char*)data_ptr, dataSize);

    PRXY_dbgPrintf(" Get Data == > %s\n", data);
    /* write to q for gapp task */
    if (OSAL_SUCCESS != OSAL_msgQSend(_PRXY_io_q, data,
            PRXY_IO_MESSAGE_SIZE_MAX, OSAL_NO_WAIT, NULL)) {
        PRXY_dbgPrintf("Failed to write GAPP PRXY IO Q.\n");
        return;
    }
    /* clean up driver i/o buffer for debugging */
    OSAL_memSet(data_ptr, 0, dataSize);
}

static void _PRXY_ioNotifyMuxLinkCreated(
        uint8 link_id)
{
   PRXY_dbgPrintf("Mux Link Established\n");
   if (OSAL_TRUE != _PRXY_ioObj_ptr->muxLinkReady) {
       _PRXY_ioObj_ptr->muxLinkReady = OSAL_TRUE;
   }
}

static void _PRXY_ioNotifyMuxLinkTerminated(
        uint8 link_id)
{
   _PRXY_ioObj_ptr->muxLinkReady = OSAL_FALSE;
    PRXY_dbgPrintf("Mux Link Terminated\n");
}


/*
 * ======== PRXY_init ========
 *
 * This public routine to initialize the PRXY sub module.  This function will
 * create MUX_IO channel and msgQ for sending msg to gapp. 
 *
 * RETURN:
 *     PRXY_Return
 */
OSAL_Status PRXY_ioInit(
    const char *proxyName_ptr)
{
    /* Init MUX_IO */
    OSAL_memSet(_PRXY_ioObj_ptr, 0, sizeof(PRXY_IoObj));
    
    _PRXY_ioObj_ptr->muxEntity.entity_recv_func = _PRXY_ioDataHandler;
    _PRXY_ioObj_ptr->muxEntity.notify           = _PRXY_ioNotifyMuxLinkCreated;
    _PRXY_ioObj_ptr->muxEntity.terminate        = _PRXY_ioNotifyMuxLinkTerminated;

    if (MUX_RET_SUCCESS != MUX_DTI_Create(PRXY_MUX_LINK_ID, _PRXY_ioObj_ptr->muxEntity)) {
        PRXY_dbgPrintf("%s:%d MUX Link Created Fail!!\n", __FILE__, __LINE__);
        return (PRXY_RETURN_FAILED);
    }
    /* Create q for gapp_task */
    if (0 == (_PRXY_io_q = OSAL_msgQCreate(PRXY_IO_MSG_Q_NAME, 
            OSAL_MODULE_PRXY, OSAL_MODULE_GAPP, OSAL_DATA_STRUCT_String,
            PRXY_MAX_QUEUE_DEPTH, PRXY_IO_MESSAGE_SIZE_MAX, 0))) {
        return (OSAL_FAIL);
    }
    
    return (OSAL_SUCCESS);
}


/*
 * ======== PRXY_getEventQueue ========
 *
 * This public routine to get the private msgQ Fd descriptor
 *
 * RETURN:
 *     PRXY_RETURN_OK
 */
PRXY_Return PRXY_getEventQueue(
    OSAL_MsgQId *queue_ptr)
{
    *queue_ptr = _PRXY_io_q;

    return (PRXY_RETURN_OK);
}

/*
 * ======== PRXY_destroy() ========
 *
 * This function closes the proxy file descriptor.
 *
 * Return Values:
 * Nothing
 */
void PRXY_destroy(
    void)
{
    if (0 != _PRXY_io_q) {
        OSAL_msgQDelete(_PRXY_io_q);
    }
}

/*
 *  ======== _PRXY_ioWriteDevice() ========
 *  This function is used to write data to the device.
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read.
 *  OSAL_FAIL: fail to read.
 */
static PRXY_Return _PRXY_ioWriteDevice(
    void *buf_ptr,
    vint size)
{
    MUX_RETURN_E     ret;

    ret = MUX_RET_FAILURE;
_PRXY_WAIT_LINK_READY:
    /* Wait someone to read/cat this device. */
    OSAL_taskDelay(200);
    while (OSAL_TRUE != _PRXY_ioObj_ptr->muxLinkReady) {
        OSAL_taskDelay(10);
    }
     /* check again to prevent the response back and Link terminated at the same time. */
    if (OSAL_TRUE == _PRXY_ioObj_ptr->muxLinkReady) {
        PRXY_dbgPrintf("%s %d write size=%d device =%s\n", __FILE__, __LINE__,
                size, buf_ptr);
        /* Ready to write */
        ret = MUX_Write((uint8 *)buf_ptr, (uint32)size, PRXY_MUX_LINK_ID);
    }
    else {
        goto _PRXY_WAIT_LINK_READY;
    }
    if (MUX_RET_SUCCESS != ret) {
        PRXY_dbgPrintf("%s:%d Fail to write buffer to Device ret=%d!\n",
                    __FUNCTION__, __LINE__, ret);
        return (PRXY_RETURN_FAILED);
    }

    return (PRXY_RETURN_OK);
}

/*
 * ======== PRXY_write() ===================
 * This function writes to the proxy slave interface.
 *
 * Returns:
 * Size of write, -1 for failure.
 */
vint PRXY_ioWrite(
    char    *buf_ptr,
    vint     size)
{
    vint ret = PRXY_RETURN_FAILED;

    if ((0 != size) && (_PRXY_io_q >= 0)) {
        PRXY_printAT("D2AtLog_Output", buf_ptr);
        ret = _PRXY_ioWriteDevice(buf_ptr, size);
    }
    return (ret);
}



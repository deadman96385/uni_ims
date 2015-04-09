/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2013 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision$ $Date$
 */

#include <osal.h>
#include "vpad_vpmd.h"
#include "vpmd_io.h"
#include "mux_api.h"
#include "mux_all.h"

#define VPMD_MUX_LINK_ID        (31)


typedef struct {
    VPMD_IoDataHandler  readHandler;
    VPMD_IoReadyHandler readyHandler;
    MUX_ENTITY_T        muxEntity;
    vint                muxLinkReady;
    vint                remainSize;
    vint                readSize;
    uint8               ioBuffer[VPMD_IO_MESSAGE_SIZE_MAX];
} VPMD_IoObj;

static VPMD_IoObj *_VPMD_ioObj_ptr;

/*
 * ======== _VPMD_ioDataHandler() ========*
 * This is the read callback, when mux have data to be read.
 * after read data from mux, the callback function, readHandler,
 * will dispatch the data to the corresponding component.
 */
void _VPMD_ioDataHandler(
    uint8   link_id,     
    uint8  *data_ptr,
    uint32  dataSize)
{
    if (1 == dataSize) {
        VPMD_ioDbgPrintf("%s:%d error !!!! size=1 data:0x%x skip it!\n",
                __FUNCTION__, __LINE__, (unsigned int)*data_ptr);
    }
    if (_VPMD_ioObj_ptr->remainSize < dataSize) {
        VPMD_ioDbgPrintf("%s:%d error !!!! remainsize:%d < size:%d, reset\n",
                __FUNCTION__, __LINE__, _VPMD_ioObj_ptr->remainSize, dataSize);
        /* heuristic: vpad restarted and inited new transaction. reset us too */
        _VPMD_ioObj_ptr->remainSize = VPMD_IO_MESSAGE_SIZE_MAX;
        _VPMD_ioObj_ptr->readSize = 0;
    }
    OSAL_memCpy(&_VPMD_ioObj_ptr->ioBuffer[_VPMD_ioObj_ptr->readSize],
            data_ptr, dataSize);
    _VPMD_ioObj_ptr->remainSize -= dataSize;
    _VPMD_ioObj_ptr->readSize += dataSize;

    if (0 == _VPMD_ioObj_ptr->remainSize) {
        VPMD_ioDbgPrintf("%s:%d read ok size:%d, push up\n",
                __FUNCTION__, __LINE__, _VPMD_ioObj_ptr->readSize);
        if (NULL != _VPMD_ioObj_ptr->readHandler) {
            _VPMD_ioObj_ptr->readHandler(
                _VPMD_ioObj_ptr->ioBuffer, _VPMD_ioObj_ptr->readSize);
        }
        else {
            VPMD_ioDbgPrintf("Error readHandler is NULL\n");
        }
        _VPMD_ioObj_ptr->remainSize = VPMD_IO_MESSAGE_SIZE_MAX;
        _VPMD_ioObj_ptr->readSize = 0;
    }
    /* clean up driver i/o buffer for debugging */
    OSAL_memSet(data_ptr, 0, dataSize);
}


/*
 *  ======== _VPMD_ioRegDeviceDataReadyHandler() ========
 *  This function is used to read data from device.
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read.
 *  OSAL_FAIL: fail to read.
 */
OSAL_Status _VPMD_ioRegDeviceDataReadyHandler(
        VPMD_IoDataHandler handler)
{
    _VPMD_ioObj_ptr->readHandler = handler;
    VPMD_ioDbgPrintf("%s:%d mux callback function Register.\n", __FILE__, __LINE__);
    return (OSAL_SUCCESS);
}

void _VPMD_ioNotifyMuxLinkCreated(
        uint8 link_id)
{
   VPMD_ioDbgPrintf("Mux Link Established\n");
   if (OSAL_TRUE != _VPMD_ioObj_ptr->muxLinkReady) {
       _VPMD_ioObj_ptr->muxLinkReady = OSAL_TRUE;
       /* emulated ready should be at last step as it may trigger sending */
       _VPMD_ioObj_ptr->readyHandler(OSAL_TRUE);
   }
}

void _VPMD_ioNotifyMuxLinkTerminated(
        uint8 link_id)
{
   _VPMD_ioObj_ptr->muxLinkReady = OSAL_FALSE;
}

/*
 *  ======== VPMD_ioInit() ========
 *  This function is used to init the io device and start the reading task
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read.
 *  OSAL_FAIL: fail to read.
 */
OSAL_Status VPMD_ioInit(
    VPMD_IoReadyHandler ioReadyHandler,
    VPMD_IoDataHandler  ioReadHandler)
{
    _VPMD_ioObj_ptr = OSAL_memCalloc(1, sizeof(VPMD_IoObj), 0);

    _VPMD_ioObj_ptr->muxEntity.entity_recv_func = _VPMD_ioDataHandler;
    _VPMD_ioObj_ptr->muxEntity.notify           = _VPMD_ioNotifyMuxLinkCreated;
    _VPMD_ioObj_ptr->muxEntity.terminate        = _VPMD_ioNotifyMuxLinkTerminated;

    _VPMD_ioObj_ptr->remainSize = VPMD_IO_MESSAGE_SIZE_MAX;
    _VPMD_ioObj_ptr->readSize = 0;
    
    /* Register callback function */
    _VPMD_ioObj_ptr->readyHandler = (VPMD_IoReadyHandler)ioReadyHandler;

    if (ioReadHandler == NULL) {
        VPMD_ioDbgPrintf("ioReadHandler is Null!!\n");
    }
    _VPMD_ioRegDeviceDataReadyHandler((VPMD_IoDataHandler)ioReadHandler);    

    if (MUX_RET_SUCCESS != MUX_DTI_Create(VPMD_MUX_LINK_ID, _VPMD_ioObj_ptr->muxEntity)) {
        VPMD_ioDbgPrintf("%s:%d MUX Link Created Fail!!\n", __FILE__, __LINE__);
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 *  ======== VPMD_ioDestroy() ========
 *  This function is used to close the io device.
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read.
 *  OSAL_FAIL: fail to read.
 */
void VPMD_ioDestroy(
        void)
{
    VPMD_ioDbgPrintf("ioDestroy!!!\n");
    _VPMD_ioRegDeviceDataReadyHandler(NULL);
}

/*
 *  ======== VPMD_ioWriteDevice() ========
 *  This function is used to write data to the device.
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read.
 *  OSAL_FAIL: fail to read.
 */
OSAL_Status VPMD_ioWriteDevice(
    void *buf_ptr,
    vint *size_ptr)
{
    MUX_RETURN_E     ret;

    ret = MUX_RET_FAILURE; 
    if (OSAL_TRUE == _VPMD_ioObj_ptr->muxLinkReady) {
        ret = MUX_Write((uint8 *)buf_ptr, (uint32)*size_ptr, VPMD_MUX_LINK_ID);
    }

    if (MUX_RET_SUCCESS != ret) {
        VPMD_ioDbgPrintf("%s:%d Fail to write buffer to Device!\n",
                    __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}


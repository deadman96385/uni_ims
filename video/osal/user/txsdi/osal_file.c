/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 *
 */

#include <osal_log.h>
#include <osal_mem.h>
#include <osal_file.h>
#include <osal_net.h>
#include <osal_sem.h>
#include <osal_string.h>

/*
 * File operations are not supported in this platform.
 * Only fifo operations are supported.
 * Fifo is implemented by local loopback socket.
 */

#define _OSAL_FILE_FIFO_NAME_MAX (128)
#define _OSAL_FILE_FIFO_SIZE_MAX (16)

typedef OSAL_NetSockId _OSAL_FileFifoId;

typedef struct {
    uint   inUse;
    void  *data_ptr;
} _OSAL_FifeFifoDataBuf;

typedef struct {
    char             pathname[_OSAL_FILE_FIFO_NAME_MAX];
    _OSAL_FileFifoId fid;
    OSAL_NetAddress  addr;
    OSAL_SemId       semCounter;
    OSAL_SemId       fifoAccessMutex;
    vint             numMsgs;
    vint             msgSize;
    void            *dataBuf_ptr;
} _OSAL_FileFifo;


/* Global fifo list */
static _OSAL_FileFifo fifoList[_OSAL_FILE_FIFO_SIZE_MAX] = {0};
static OSAL_SemId     fifoMutex = NULL;

/*
 * ======== _OSAL_fileFifoSearchByName() ========
 *
 * Private function search a fifo by path name.
 *
 * Returns:
 *  NULL: Not found.
 *  Otherwise: The fid.
 */
static _OSAL_FileFifo* _OSAL_fileFifoSearchByName(
    const char    *pathname_ptr)
{
    int i;

    OSAL_semAcquire(fifoMutex, OSAL_WAIT_FOREVER);
    /* Search fifo list. */
    for (i = 0; i < _OSAL_FILE_FIFO_SIZE_MAX; i++) {
        if (0 == OSAL_strcmp(fifoList[i].pathname, pathname_ptr)) {
            OSAL_semGive(fifoMutex);
            return (&fifoList[i]);
        }
    }
    OSAL_semGive(fifoMutex);
    return (NULL);
}

/*
 * ======== _OSAL_fileFifoSearchById() ========
 *
 * Private function search a fifo by fifo id.
 *
 * Returns:
 *  NULL: Not found.
 *  Otherwise: The fid.
 */
static _OSAL_FileFifo* _OSAL_fileFifoSearchById(
    _OSAL_FileFifoId fid)
{
    int i;

    OSAL_semAcquire(fifoMutex, OSAL_WAIT_FOREVER);
    /* Search fifo list. */
    for (i = 0; i < _OSAL_FILE_FIFO_SIZE_MAX; i++) {
        if (fifoList[i].fid == fid) {
            OSAL_semGive(fifoMutex);
            return (&fifoList[i]);
        }
    }
    OSAL_semGive(fifoMutex);
    return (NULL);
}

/*
 * ======== _OSAL_fileFifoGetData() ========
 *
 * Private function copy data from buffer of given index.
 *
 * Returns:
 *  OSAL_SUCCESS: Data copied and freed to pool.
 *  Otherwise: Failed.
 */
static OSAL_Status _OSAL_fileFifoGetData(
    _OSAL_FileFifo   *fifo_ptr,
    vint              index,
    void             *buf_ptr,
    vint             *size_ptr)
{
    _OSAL_FifeFifoDataBuf *data_ptr;

    if ((index < 0) || (index >= fifo_ptr->numMsgs)) {
        /* invalid index */
        OSAL_logMsg("Invalid index:%d fifo:%p\n", index, fifo_ptr);
        return (OSAL_FAIL);
    }

    data_ptr = (_OSAL_FifeFifoDataBuf *)((int)fifo_ptr->dataBuf_ptr + (index *
            (fifo_ptr->msgSize + sizeof(data_ptr->inUse))));

    if (OSAL_TRUE != data_ptr->inUse) {
        OSAL_logMsg("Data buffer not in use. data_ptr:%p Index:%d ERROR!!!\n",
                data_ptr, index);
    }

    /* do the time consuming part first */
    OSAL_memCpy(buf_ptr, &data_ptr->data_ptr, fifo_ptr->msgSize);

    OSAL_semAcquire(fifo_ptr->fifoAccessMutex, OSAL_WAIT_FOREVER);
    data_ptr->inUse = OSAL_FALSE;
    *size_ptr = fifo_ptr->msgSize;
    OSAL_semGive(fifo_ptr->fifoAccessMutex);

    /* safe to increase the counting semaphore now */
    OSAL_semGive(fifo_ptr->semCounter);

    return (OSAL_SUCCESS);
}

/*
 * ======== _OSAL_fileFifoSetData() ========
 *
 * Private function get an available buffer and copy data to buffer.
 *
 * Returns:
 *  -1: No avalable buffer.
 *  Otherwise :Data buffer index and data copied.
 */
static vint _OSAL_fileFifoSetData(
    _OSAL_FileFifo   *fifo_ptr,
    void             *buf_ptr,
    vint             *size_ptr)
{
    _OSAL_FifeFifoDataBuf *data_ptr;
    vint                   i;

    if (*size_ptr != fifo_ptr->msgSize) {
        OSAL_logMsg("Unexpected size:%d fifo msg size:%d\n",
                *size_ptr, fifo_ptr->msgSize);
        return (-1);
    }
    data_ptr = fifo_ptr->dataBuf_ptr;

    OSAL_semAcquire(fifo_ptr->semCounter, OSAL_WAIT_FOREVER);

    /* we reach here only if the buffer have free slot for us, try to lock it */
    OSAL_semAcquire(fifo_ptr->fifoAccessMutex, OSAL_WAIT_FOREVER);
    for (i = 0; i < fifo_ptr->numMsgs; i++) {
        if (OSAL_FALSE == data_ptr->inUse) {
            /* Found one. */
            break;
        }
        data_ptr = (_OSAL_FifeFifoDataBuf *)((int)data_ptr +
                fifo_ptr->msgSize + sizeof(data_ptr->inUse));
    }
    /* for debug, should not happen. */
    if (i == fifo_ptr->numMsgs) {
        /* No available. */
        OSAL_logMsg("%s:%d No available data buffer for in fifo %p\n",
                __FILE__, __LINE__, fifo_ptr);
        /* return the couting semaphore we just acquired since we can't get free buffer */
        OSAL_semGive(fifo_ptr->semCounter);
        OSAL_semGive(fifo_ptr->fifoAccessMutex);
        return (-1);
    }
    data_ptr->inUse = OSAL_TRUE;
    *size_ptr = fifo_ptr->msgSize;
    OSAL_semGive(fifo_ptr->fifoAccessMutex);

    /* now do the time consuming part */
    OSAL_memCpy(&data_ptr->data_ptr, buf_ptr, fifo_ptr->msgSize);

    return (i);
}
    
/*
 * ======== _OSAL_fileFifoCreate() ========
 *
 * Private function to create a named pipe (FIFO) with the
 * specified name (path name)
 *
 *
 * Returns:
 * OSAL_SUCCESS: The fifo node is created in the file system
 * OSAL_FAIL: failed to create the fifo
 */
static _OSAL_FileFifo* _OSAL_fileFifoCreate(
    const char    *pathname_ptr,
    uint32         numMsgs,
    uint32         numBytes)
{
    int i;

    /* Create semaphor counter to protect queue creating */
    if (NULL == fifoMutex) {
        fifoMutex = OSAL_semMutexCreate();
    }

    OSAL_semAcquire(fifoMutex, OSAL_WAIT_FOREVER);

    /* Search free fifo from fifo list. */
    for (i = 0; i < _OSAL_FILE_FIFO_SIZE_MAX; i++) {
        if (0 == fifoList[i].fid) {
            break;
        }
    }

    if (_OSAL_FILE_FIFO_SIZE_MAX == i) {
        /* No available entry. */
        OSAL_logMsg("No available fifo entry, increase fifo size!!!\n");
        OSAL_semGive(fifoMutex);
        return (NULL);
    }

    /* Create local loopback socket and bind it. */
    if (OSAL_SUCCESS != OSAL_netSocket(&fifoList[i].fid, OSAL_NET_SOCK_UDP)) {
        OSAL_logMsg("Failed to create socket for fifo!!!\n");
        OSAL_semGive(fifoMutex);
        return (NULL);
    }

    fifoList[i].addr.type = OSAL_NET_SOCK_UDP;
    fifoList[i].addr.ipv4 = OSAL_netHtonl(OSAL_NET_INADDR_LOOPBACK);
    /* Bind any */
    fifoList[i].addr.port = 0;
    if (OSAL_SUCCESS != OSAL_netBindSocket(&fifoList[i].fid, &fifoList[i].addr)) {
        OSAL_logMsg("Failed to bind socket for fifo!!!\n");
        OSAL_netCloseSocket(&fifoList[i].fid);
        fifoList[i].fid = 0;
        OSAL_semGive(fifoMutex);
        return (NULL);
    }

    /* Get the port. */
    if (OSAL_SUCCESS != OSAL_netGetSocketAddress(&fifoList[i].fid,
            &fifoList[i].addr)) {
        OSAL_logMsg("Failed to get bound address for fifo!!!\n");
        OSAL_netCloseSocket(&fifoList[i].fid);
        fifoList[i].fid = 0;
        OSAL_semGive(fifoMutex);
        return (NULL);
    }

    /* Everything is fine. */
    OSAL_strncpy(fifoList[i].pathname, pathname_ptr,
            sizeof(fifoList[i].pathname));

    fifoList[i].numMsgs = numMsgs;
    fifoList[i].msgSize = numBytes;
    
    /* Allocate memory for data pool. */
    if (NULL == (fifoList[i].dataBuf_ptr =
            OSAL_memAlloc(numMsgs * (numBytes + sizeof(uint)),
            OSAL_MEM_ARG_STATIC_ALLOC))) {
        OSAL_netCloseSocket(&fifoList[i].fid);
        fifoList[i].fid = 0;
        OSAL_semGive(fifoMutex);
        return (NULL);
    }
    /* Clear memory. */
    OSAL_memSet(fifoList[i].dataBuf_ptr, 0,
            numMsgs * (numBytes + sizeof(uint)));
    
    fifoList[i].fifoAccessMutex = OSAL_semMutexCreate();
    fifoList[i].semCounter = OSAL_semCountCreate(numMsgs);

    /* Done. */
    OSAL_logMsg("Fifo created. id:%d, port:%d numMsgs:%d numBytes:%d\n",
            fifoList[i].fid, fifoList[i].addr.port,
            numMsgs, numBytes);

    OSAL_semGive(fifoMutex);
    return (&fifoList[i]);
}

/*
 * ======== _OSAL_fileFifoDelete() ========
 *
 * Private function to delete the FIFO node specified by the path
 *
 *
 * Returns:
 * OSAL_SUCCESS: file node removed
 * OSAL_FAIL: invalid path? failed to remove file node.
 */
static OSAL_Status _OSAL_fileFifoDelete(
    _OSAL_FileFifo *fifo_ptr) 
{
    /* Close the socket */
    OSAL_logMsg("Fifo deleting. id:%d, port:%d\n", fifo_ptr->fid,
            fifo_ptr->addr.port);

    OSAL_semAcquire(fifoMutex, OSAL_WAIT_FOREVER);
    OSAL_netCloseSocket(&fifo_ptr->fid);
    /* Free buffer. */
    OSAL_memFree(fifo_ptr->dataBuf_ptr, OSAL_MEM_ARG_STATIC_ALLOC);
    fifo_ptr->fid = 0;
    fifo_ptr->pathname[0] = 0;
    OSAL_semDelete(fifo_ptr->semCounter);
    OSAL_semDelete(fifo_ptr->fifoAccessMutex);
    OSAL_semGive(fifoMutex);

    return (OSAL_SUCCESS);
}

/*
 * ======== OSAL_fileOpen() ========
 *
 * Creates a new open file ID.
 * This function searches database for speficed file pathname and
 * return the socket id if found.
 *
 * flags and mode are not used.
 *
 * Returns:
 * OSAL_SUCCESS: A file was successfully opened.
 * OSAL_FAIL: The file failed to open or be created.
 */
OSAL_Status OSAL_fileOpen(
    OSAL_FileId   *fileId_ptr,
    const char    *pathname_ptr,
    OSAL_FileFlags flags,
    uint32         mode)
{
    _OSAL_FileFifo   *fifo_ptr;

    if (NULL == pathname_ptr) {
        return (OSAL_FAIL);
    }

    /* Look up if the fifo is already there. */
    if (NULL == (fifo_ptr = _OSAL_fileFifoSearchByName(pathname_ptr))) {
        /* Fifo doesn't exist, return fail. */
        return (OSAL_FAIL);
    }

    *fileId_ptr = (OSAL_FileId)(fifo_ptr->fid);

    return (OSAL_SUCCESS);
}

/*
 * ======== OSAL_fileClose() ========
 *
 * Closes an open file
 *
 * Returns:
 * OSAL_SUCCESS: The file was successfully closed.
 * OSAL_FAIL: The file failed to close or was already closed.
 */
OSAL_Status OSAL_fileClose(OSAL_FileId *fileId_ptr)
{
    /* Do nothing here. */
    return (OSAL_SUCCESS);
}

/*
 * ======== OSAL_fileRead() ========
 *
 * File is not supported, this function only supports to read data from fifo.
 *
 * Returns:
 * OSAL_SUCCESS: The file was successfully read.
 * OSAL_FAIL: The read failed to read data.
 */
OSAL_Status OSAL_fileRead(
    OSAL_FileId *fileId_ptr,
    void        *buf_ptr,
    vint        *size_ptr)
{
    OSAL_NetAddress     addr;
    vint                size;
    _OSAL_FileFifo     *fifo_ptr;
    vint                dataBufIndex;
    OSAL_Status         result;

    /* Search fifo id. */
    if (NULL == (fifo_ptr = _OSAL_fileFifoSearchById(
            (_OSAL_FileFifoId)*fileId_ptr))) {
        /* Fifo doesn't exist, return fail. */
        return (OSAL_FAIL);
    }

    size = sizeof(dataBufIndex);
    /* Read it. */
    if (OSAL_SUCCESS != OSAL_netSocketReceiveFrom((OSAL_NetSockId *)fileId_ptr,
            (void *)(&dataBufIndex), &size, &addr)) {
        return (OSAL_FAIL);
    }

    /* Check read size. */
    if (size != sizeof(dataBufIndex)) {
        /* Should not be here. */
        OSAL_logMsg("Unexpected read size %d!!!\n", size);
        return (OSAL_FAIL);
    }

    /* Copy and free memory buffer */
    result = _OSAL_fileFifoGetData(fifo_ptr, dataBufIndex, buf_ptr, size_ptr);

    return (result);
}

/*
 * ======== OSAL_fileWrite() ========
 *
 * File is not supported, this function only supports to write data to fifo.
 *
 * Returns:
 * OSAL_SUCCESS: The file was successfully written.
 * OSAL_FAIL: The write failed to write data.
 */
OSAL_Status OSAL_fileWrite(
    OSAL_FileId *fileId_ptr,
    void        *buf_ptr,
    vint        *size_ptr)
{
    _OSAL_FileFifo     *fifo_ptr;
    vint                dataBufIndex;  
    vint                size;

    /* Search fifo id. */
    if (NULL == (fifo_ptr = _OSAL_fileFifoSearchById(
            (_OSAL_FileFifoId)*fileId_ptr))) {
        /* Fifo doesn't exist, return fail. */
        return (OSAL_FAIL);
    }
    if (-1 == (dataBufIndex = _OSAL_fileFifoSetData(fifo_ptr, buf_ptr,
            size_ptr))) {
        return (OSAL_FAIL);
    }
    size = sizeof(dataBufIndex);
    /* Write it. */
    return (OSAL_netSocketSendTo((OSAL_NetSockId *)fileId_ptr,
            (void *)(&dataBufIndex), &size,
            &(fifo_ptr->addr)));
}

/*
 * ======== OSAL_fileSeek() ========
 *
 * Repositions the offset of the open file associated with the file ID to the
 * value at the *offset_ptr parameter according to the type parameter:
 * OSAL_FILE_SEEK_SET: The offset is set to offset bytes.
 * OSAL_FILE_SEEK_CUR: The offset is set to its current location plus offset
 *                     bytes.
 * OSAL_FILE_SEEK_END: The offset is set to the size of the file plus offset
 *                     bytes.
 *
 * If successful the offset into the file from byte 0 is copied back to
 * *offset_ptr.
 *
 * Returns:
 * OSAL_SUCCESS: The file was repositioned.
 * OSAL_FAIL: Failed to reposition the file.
 */
OSAL_Status OSAL_fileSeek(
    OSAL_FileId      *fileId_ptr,
    vint             *offset_ptr,
    OSAL_FileSeekType type)
{
    /* No need to support. */
    return (OSAL_FAIL);
}

/*
 * ======== OSAL_fileGetSize() ========
 *
 * Returns the size of the file associated with the file ID.
  *
 * If successful the size of the file is copied to *size_ptr.
 *
 * Returns:
 * OSAL_SUCCESS: The size is returned. The size was written to size_ptr.
 * OSAL_FAIL: Failed to get the size of the file.
 */
OSAL_Status OSAL_fileGetSize(OSAL_FileId *fileId_ptr, vint *size_ptr)
{

    /* No need to support. */
    return (OSAL_FAIL);
}

/*
 * ======== OSAL_fileExists() ========
 *
 * Returns whether a file exists or not.
 *
 * Returns:
 * OSAL_TRUE: The file at the path exists.
 * OSAL_FALSE: The file does not exist.
 */
OSAL_Boolean OSAL_fileExists(const char *pathname_ptr)
{

    /* No need to support. */
    return (OSAL_FALSE);
}

/*
 * ======== OSAL_fileDelete() ========
 *
 * Delete the file node specified by the path
 *
 *
 * Returns:
 * OSAL_SUCCESS: file node removed
 * OSAL_FAIL: invalid path? failed to remove file node.
 */
OSAL_Status OSAL_fileDelete(
    const char    *pathname_ptr)
{
    /* No need to support. */
    return (OSAL_FAIL);
}

/*
 * ======== OSAL_fileFifoCreate() ========
 *
 * Create a named pipe (FIFO) with the specified name (path name)
 *
 *
 * Returns:
 * OSAL_SUCCESS: The fifo node is created in the file system
 * OSAL_FAIL: failed to create the fifo
 */
OSAL_Status OSAL_fileFifoCreate(
    const char    *pathname_ptr,
    uint32         numMsgs,
    uint32         numBytes)
{
    if (NULL == pathname_ptr) {
        return (OSAL_FAIL);
    }

    /* Look up if the fifo is already there. */
    if (NULL != _OSAL_fileFifoSearchByName(pathname_ptr)) {
        /* Fifo already exists, return fail. */
        return (OSAL_FAIL);
    }

    /* Create a fifo. */
    if (NULL == _OSAL_fileFifoCreate(pathname_ptr, numMsgs, numBytes)) {
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== OSAL_fileFifoDelete() ========
 *
 * Delete the FIFO node specified by the path
 *
 *
 * Returns:
 * OSAL_SUCCESS: file node removed
 * OSAL_FAIL: invalid path? failed to remove file node.
 */
OSAL_Status OSAL_fileFifoDelete(
    const char    *pathname_ptr)
{
    _OSAL_FileFifo   *fifo_ptr;

    if (NULL == pathname_ptr) {
        return (OSAL_FAIL);
    }

    /* Look up if the fifo is already there. */
    if (NULL == (fifo_ptr = _OSAL_fileFifoSearchByName(pathname_ptr))) {
        /* Fifo doesn't exist, return fail. */
        return (OSAL_FAIL);
    }

    /* Delete fifo. */
    return (_OSAL_fileFifoDelete(fifo_ptr));
}



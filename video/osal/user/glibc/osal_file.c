/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 23244 $ $Date: 2013-12-02 16:27:24 +0800 (Mon, 02 Dec 2013) $
 *
 */

#include <osal_file.h>

/*
 * ======== OSAL_fileOpen() ========
 *
 * Creates a new open file ID.
 * The flags parameter must include one of the following values:
 * OSAL_FILE_O_RDONLY, OSAL_FILE_O_WRONLY, or OSAL_FILE_O_RDWR.
 * These request opening the file read-only, write-only, or read/write,
 * respectively.
 * In addition, zero or more file creation flags and file status flags
 * can be bitwise-or'd in flags. The file creation flags are:
 * OSAL_FILE_O_CREATE, OSAL_FILE_O_APPEND. The file status flags.
 *
 * Note, the mode parameter specifies the permissions to use in case
 * a new file is created. This argument must be supplied when
 * OSAL_FILE_O_CREATE is specified in flags; if OSAL_FILE_O_CREATE is not
 * specified, then mode is ignored.  The mode value is a
 * is the linux permission value.  for possible values see the
 * chmod manpage.  An example is 00755.
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
    OSAL_FileId fileId;
    vint flgs = 0;
    
    if (NULL == fileId_ptr) {
        return (OSAL_FAIL);
    }
    
    if (0 != (flags & OSAL_FILE_O_CREATE)) {
        flgs |= O_CREAT;
    }
    if (0 != (flags & OSAL_FILE_O_APPEND)) {
        flgs |= O_APPEND;
    }
    if (0 != (flags & OSAL_FILE_O_RDONLY)) {
        flgs |= O_RDONLY;
    }
    if (0 != (flags & OSAL_FILE_O_WRONLY)) {
        flgs |= O_WRONLY;
    }
    if (0 != (flags & OSAL_FILE_O_RDWR)) {
        flgs |= O_RDWR;
    }
    if (0 != (flags & OSAL_FILE_O_TRUNC)) {
        flgs |= O_TRUNC;
    }
    if (0 != (flags & OSAL_FILE_O_NDELAY)) {
        flgs |= O_NDELAY;
    }

    fileId = open(pathname_ptr, flgs, mode);
    if (-1 == fileId) {
        return (OSAL_FAIL);
    }
    *fileId_ptr = fileId;
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
    if (NULL == fileId_ptr) {
        return (OSAL_FAIL);
    }
    
    if (0 == close(*fileId_ptr)) {
        *fileId_ptr = -1;
        return (OSAL_SUCCESS);
    }
    return (OSAL_FAIL);
}

/*
 * ======== OSAL_fileRead() ========
 *
 * Read up to *size_ptr bytes from file ID into the buffer pointed to by
 * buf_ptr.  If successful the number of bytes read is written back to the
 * memory pointed to by size_ptr and the file position is advanced by this
 * value.
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
    ssize_t retval;

    if ((NULL == fileId_ptr) || (NULL == size_ptr)) {
        return (OSAL_FAIL);
    }
    
    retval = read(*fileId_ptr, buf_ptr, *size_ptr);

    if (0 <= retval) {
        *size_ptr = retval;
        return(OSAL_SUCCESS);
    }
    return (OSAL_FAIL);
}

/*
 * ======== OSAL_fileWrite() ========
 *
 * Write *size_ptr number of bytes in the buffer pointed to by buf_ptr
 * to the file ID. If successful the number of bytes written is copied back to
 * the memory pointed to by size_ptr and the file position is advanced by this
 * value.
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
    ssize_t retval;

    if ((NULL == fileId_ptr) || (NULL == size_ptr)) {
        return (OSAL_FAIL);
    }
    
    retval = write(*fileId_ptr, buf_ptr, *size_ptr);

    if (0 <= retval) {
        *size_ptr = retval;
        return(OSAL_SUCCESS);
    }
    return (OSAL_FAIL);
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
    int whence;
    off_t retval;

    if ((NULL == fileId_ptr) || (NULL == offset_ptr)) {
        return (OSAL_FAIL);
    }
    
    switch (type) {
        case OSAL_FILE_SEEK_SET:
            whence = SEEK_SET;
            break;
        case OSAL_FILE_SEEK_CUR:
            whence = SEEK_CUR;
            break;
        case OSAL_FILE_SEEK_END:
            whence = SEEK_END;
            break;
        default:
            return (OSAL_FAIL);
    }

    retval = lseek(*fileId_ptr, *offset_ptr, whence);
    if (-1 != retval) {
        *offset_ptr = retval;
        return(OSAL_SUCCESS);
    }
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
    struct stat   stat;
    int retval;

    if ((NULL == fileId_ptr) || (NULL == size_ptr)) {
        return (OSAL_FAIL);
    }
    
    retval = fstat(*fileId_ptr, &stat);
    if (0 == retval) {
        *size_ptr = stat.st_size;
        return(OSAL_SUCCESS);
    }
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
    struct stat fileStat;
    int retval;
    retval = stat(pathname_ptr, &fileStat);
    if (0 == retval) {
        return(OSAL_TRUE);
    }
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
    if (0 == remove(pathname_ptr)) {
        return OSAL_SUCCESS;
    } else {
        return OSAL_FAIL;
    }
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
    if (0 == mkfifo(pathname_ptr, S_IRWXU | S_IRWXG | S_IRWXO)) {
        return OSAL_SUCCESS;
    } else {
        return OSAL_FAIL;
    }
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
    if (0 == remove(pathname_ptr)) {
        return OSAL_SUCCESS;
    } else {
        return OSAL_FAIL;
    }
}


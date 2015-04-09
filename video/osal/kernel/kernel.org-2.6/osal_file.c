/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004,2005 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 7595 $ $Date: 2008-09-11 09:53:05 -0400 (Thu, 11 Sep 2008) $
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
    OSAL_FileId     fileId;
    vint            flgs = 0;
    mm_segment_t    fs;

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

    fs = get_fs();
    set_fs(KERNEL_DS);

    /* open it */
    fileId = OSAL_syscall3(__NR_open, (long)pathname_ptr, (long)flgs,
            (long)mode);

    set_fs(fs);

    if (0 > fileId) {
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
OSAL_Status OSAL_fileClose(
    OSAL_FileId    *fileId_ptr)
{
    mm_segment_t    fs;

    if (NULL == fileId_ptr) {
        return (OSAL_FAIL);
    }

    fs = get_fs();
    set_fs(KERNEL_DS);

    if (0 != OSAL_syscall3(__NR_close, (long)*fileId_ptr, (long)0, (long)0)) {
        set_fs(fs);
        return(OSAL_FAIL);
    }

    set_fs(fs);
    return (OSAL_SUCCESS);
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
    ssize_t         retval;
    mm_segment_t    fs;

    if ((NULL == fileId_ptr) || (NULL == size_ptr)) {
        return (OSAL_FAIL);
    }

    fs = get_fs();
    set_fs(KERNEL_DS);

    retval = OSAL_syscall3(__NR_read, (long)*fileId_ptr, (long)buf_ptr,
            (long)*size_ptr);

    set_fs(fs);
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
    ssize_t         retval;
    mm_segment_t    fs;


    if ((NULL == fileId_ptr) || (NULL == size_ptr)) {
        return (OSAL_FAIL);
    }

    fs = get_fs();
    set_fs(KERNEL_DS);

    retval = OSAL_syscall3(__NR_write, (long)*fileId_ptr, (long)buf_ptr,
            (long)*size_ptr);

    set_fs(fs);
    if (0 <= retval) {
        *size_ptr = retval;
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
OSAL_Boolean OSAL_fileExists(
    const char *pathname_ptr)
{
    mm_segment_t    fs;

    if (NULL == pathname_ptr) {
        return (OSAL_FALSE);
    }

    fs = get_fs();
    set_fs(KERNEL_DS);

    if (0 > OSAL_syscall3(__NR_access, (long)pathname_ptr, (long)0, (long)0)) {
        set_fs(fs);
        return (OSAL_FALSE);
    }

    set_fs(fs);
    return(OSAL_TRUE);
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
    mm_segment_t    fs;

    fs = get_fs();
    set_fs(KERNEL_DS);

    if (0 == OSAL_syscall3(__NR_unlink, (long)pathname_ptr, (long)0, (long)0)) {
        set_fs(fs);
        return (OSAL_SUCCESS);
    }
    set_fs(fs);
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
    mm_segment_t    fs;

    fs = get_fs();
    set_fs(KERNEL_DS);

    if (0 > OSAL_syscall3(__NR_access, (long)pathname_ptr, (long)0, (long)0)) {
        if (0 > OSAL_syscall3(__NR_mknod,
                (long)pathname_ptr,
                (long)(S_IFIFO | 0666),
                (long)0)) {
            set_fs(fs);
            return (OSAL_FAIL);
        }
    }

    set_fs(fs);
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
    mm_segment_t    fs;

    fs = get_fs();
    set_fs(KERNEL_DS);

    if (0 == OSAL_syscall3(__NR_unlink, (long)pathname_ptr, (long)0, (long)0)) {
        set_fs(fs);
        return (OSAL_SUCCESS);
    }
    set_fs(fs);
    return (OSAL_FAIL);
}

EXPORT_SYMBOL(OSAL_fileOpen);
EXPORT_SYMBOL(OSAL_fileClose);
EXPORT_SYMBOL(OSAL_fileRead);
EXPORT_SYMBOL(OSAL_fileWrite);
EXPORT_SYMBOL(OSAL_fileExists);
EXPORT_SYMBOL(OSAL_fileDelete);
EXPORT_SYMBOL(OSAL_fileFifoCreate);
EXPORT_SYMBOL(OSAL_fileFifoDelete);

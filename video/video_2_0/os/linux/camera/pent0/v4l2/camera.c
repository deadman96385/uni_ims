/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
 */

#include <sys/time.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <video.h>
#include "camera.h"

static pthread_mutex_t _CAM_apiLock = PTHREAD_MUTEX_INITIALIZER;

/*
 * ======== _CAM_Task() ========
 *
 * Camera task: To send captured picture to ENC directly.
 *
 * Return Values:
 * None.
 */
static void _CAM_Task(
    void *arg_ptr)
{
    /*
    * Big D2 Banner with version information.
    */
    OSAL_logMsg("<<<<<<<<<<<<<<< CAM_TASK >>>>>>>>>>>>>>>>>>\n");
    OSAL_logMsg("\n"
        "             D2 Technologies, Inc.\n"
        "      _   _  __                           \n"
        "     / | '/  /_  _  /_ _  _  /_  _  ._   _\n"
        "    /_.'/_  //_'/_ / // //_///_//_///_'_\\ \n"
        "                                _/        \n"
        "\n"
        "        Unified Communication Products\n");
    OSAL_logMsg("               www.d2tech.com\n");

    OSAL_logMsg("%s:%d RUNNING\n", __FILE__, __LINE__);

    CAMERA *cam_ptr = (CAMERA *)arg_ptr;

_CAM_TASK_LOOP:

    /* Send captured frame to ENC directly. */
    if(cam_ptr->encDataObj->codecInited) {
        if(0 == CAM_videoIn(cam_ptr)) {
            cam_ptr->toEncCodecQInputBuffer(cam_ptr->encDataObj);
        }
    }
    /* Capture video frame every 20ms. */
    usleep(20000);
    /* Loop as long as the encoder is initialized. */
    if (OSAL_TRUE == cam_ptr->_cam_task_obj.thread_started) {
        goto _CAM_TASK_LOOP;
    }
    else {
        OSAL_logMsg("%s:%d _CAM_Task exited\n", __FILE__, __LINE__);
    }
}

/*
 * ======== CAM_init() ========
 * This function is called by video engine to init video capture driver.
 * Returns:
 *  0: Success.
 *  -1: Failed.
 */
int CAM_init(
    CAMERA     *cam_ptr,
    int         width,
    int         height,
    int         id,
    int         flags)
{
    struct v4l2_capability      cap;
    struct v4l2_format          fmt;
    struct v4l2_requestbuffers  req;
    struct v4l2_buffer          buf;
    int                         bufs;
    enum v4l2_buf_type          type;

    if ((0 == width) || (0 == height)) {
        DBG("");
        return (-1);
    }

    DBG("");
    pthread_mutex_lock(&_CAM_apiLock);

    if (cam_ptr->_CAM_inited) {
        DBG("");
        pthread_mutex_unlock(&_CAM_apiLock);
        return (-1);
    }

    OSAL_snprintf(cam_ptr->_CAM_obj.devname,  sizeof(cam_ptr->_CAM_obj.devname),
            "/dev/video%d", id);
    DBG("Devname is %s\n", _CAM_obj.devname);

    /*
     * Using video4linux2 for video capture.
     * For Linux this would be fine on all platforms.
     */

    /*
     * Open device, check capabilities.
     * - capture
     * - stream
     */
    cam_ptr->_CAM_obj.vi.fd = open(cam_ptr->_CAM_obj.devname, O_RDWR | O_NONBLOCK, 0);
    if (cam_ptr->_CAM_obj.vi.fd < 0) {
        DBG("");
        pthread_mutex_unlock(&_CAM_apiLock);
        return (-1);
    }
    if (-1 == ioctl(cam_ptr->_CAM_obj.vi.fd,
            VIDIOC_QUERYCAP,
            &cap)) {
        close(cam_ptr->_CAM_obj.vi.fd);
        DBG("");
        pthread_mutex_unlock(&_CAM_apiLock);
        return (-1);
    }
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        close(cam_ptr->_CAM_obj.vi.fd);
        DBG("");
        pthread_mutex_unlock(&_CAM_apiLock);
        return (-1);
    }
    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        close(cam_ptr->_CAM_obj.vi.fd);
        DBG("");
        pthread_mutex_unlock(&_CAM_apiLock);
        return (-1);
    }
   
    /*
     * Set input format. Most cameras support 
     * Packed mode: Y0+U0+Y1+V0 (1 plane)
     * This is U downsample by 2 in horizontal direction, and V downsample by 2
     * in horizontal direction.
     */
    memset(&fmt, 0, sizeof(fmt));
    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = width;
    fmt.fmt.pix.height      = height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field       = V4L2_FIELD_NONE;
    if(-1 == ioctl (cam_ptr->_CAM_obj.vi.fd,
            VIDIOC_S_FMT,
            &fmt)) {
        close(cam_ptr->_CAM_obj.vi.fd);
        DBG("");
        pthread_mutex_unlock(&_CAM_apiLock);
        return (-1);
    }

    /*
     * Request buffers A/B
     * Buffers be mmap
     */
    memset(&req, 0, sizeof(req));
    req.count   = _CAMERA_VI_BUFS;
    req.type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory  = V4L2_MEMORY_MMAP;
    if(-1 == ioctl(cam_ptr->_CAM_obj.vi.fd,
            VIDIOC_REQBUFS,
            &req)) {
        close(cam_ptr->_CAM_obj.vi.fd);
        DBG("");
        pthread_mutex_unlock(&_CAM_apiLock);
        return (-1);
    }
    if (_CAMERA_VI_BUFS != req.count) {
        close(cam_ptr->_CAM_obj.vi.fd);
        DBG("");
        pthread_mutex_unlock(&_CAM_apiLock);
        return (-1);
    }

    /*
     * Memory map buffers
     */
    for (bufs = 0; bufs < _CAMERA_VI_BUFS; bufs++) {

        memset(&buf, 0, sizeof(buf));
        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = bufs;

        /*
         * Find buffer size then mmap it.
         */
        if (-1 == ioctl (cam_ptr->_CAM_obj.vi.fd,
                VIDIOC_QUERYBUF,
                &buf)) {
            close(cam_ptr->_CAM_obj.vi.fd);
            DBG("");
            pthread_mutex_unlock(&_CAM_apiLock);
            return (-1);
        }

        cam_ptr->_CAM_obj.vi.buf[bufs].length = buf.length;
        cam_ptr->_CAM_obj.vi.buf[bufs].start_ptr = mmap(NULL,
              buf.length,
              PROT_READ | PROT_WRITE,
              MAP_SHARED,
              cam_ptr->_CAM_obj.vi.fd,
              buf.m.offset);
        if (MAP_FAILED == cam_ptr->_CAM_obj.vi.buf[bufs].start_ptr) {
            close(cam_ptr->_CAM_obj.vi.fd);
            cam_ptr->_CAM_obj.vi.buf[bufs].start_ptr = NULL;
            DBG("");
            pthread_mutex_unlock(&_CAM_apiLock);
            return (-1);
        }
    }

    cam_ptr->_CAM_obj.muted = 0;
    cam_ptr->_CAM_obj.vi.width = width;
    cam_ptr->_CAM_obj.vi.height = height;
    gettimeofday(&cam_ptr->_CAM_obj.inittime, NULL);

    /*
     * Start video in.
     * Enqueue empty buffers in video capture.
     */
    for (bufs = 0; bufs < _CAMERA_VI_BUFS; bufs++) {
        memset(&buf, 0, sizeof(buf));
        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = bufs;
        if (-1 == ioctl (cam_ptr->_CAM_obj.vi.fd,
                VIDIOC_QBUF,
                &buf)) {
            DBG("");
            pthread_mutex_unlock(&_CAM_apiLock);
            CAM_shutdown(cam_ptr);
            return (-1);
        }
    }

    /*
     * Start streaming
     */
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl (cam_ptr->_CAM_obj.vi.fd,
            VIDIOC_STREAMON,
            &type)) {
        DBG("");
        pthread_mutex_unlock(&_CAM_apiLock);
        CAM_shutdown(cam_ptr);
        return (-1);
    }

    cam_ptr->_CAM_obj.vi.lastbuf.index = -1;

    cam_ptr->_CAM_inited = 1;

    pthread_mutex_unlock(&_CAM_apiLock);

    cam_ptr->_cam_task_obj.thread_started = OSAL_TRUE;
    /* Init the task used to feed encoder. */
    cam_ptr->_cam_task_obj.taskId = 0;
    cam_ptr->_cam_task_obj.stackSize = _CAM_TASK_STACK_SZ;
    cam_ptr->_cam_task_obj.taskPriority = _CAM_TASK_PRIORITY;
    cam_ptr->_cam_task_obj.func_ptr = _CAM_Task;

    OSAL_snprintf(cam_ptr->_cam_task_obj.name, sizeof(cam_ptr->_cam_task_obj.name), "%s",
            CAM_TASK_NAME);

    cam_ptr->_cam_task_obj.arg_ptr = (void *)cam_ptr;

    if (0 == (cam_ptr->_cam_task_obj.taskId = OSAL_taskCreate(
        cam_ptr->_cam_task_obj.name,
        cam_ptr->_cam_task_obj.taskPriority,
        cam_ptr->_cam_task_obj.stackSize,
        cam_ptr->_cam_task_obj.func_ptr,
        cam_ptr->_cam_task_obj.arg_ptr))){
        OSAL_logMsg("%s:%d _CAM_Task Stopped\n", __FILE__, __LINE__);
        return (1);
    }

    DBG("Camera Started\n");
    return (0);
}

/*
 * ======== CAM_shutdown() ========
 * This function is called by video engine to shutdown video capture driver.
 * Returns:
 *  0: Success.
 *  -1: Failed.
 */
int CAM_shutdown(
    CAMERA    *cam_ptr)
{
    enum v4l2_buf_type type;
    int                bufs;
    DBG("");

    cam_ptr->_cam_task_obj.thread_started = OSAL_FALSE;

    pthread_mutex_lock(&_CAM_apiLock);
    if (!cam_ptr->_CAM_inited) {
        DBG("");
        pthread_mutex_unlock(&_CAM_apiLock);
        return (-1);
    }

    /*
     * Stop video input.
     */
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(cam_ptr->_CAM_obj.vi.fd, VIDIOC_STREAMOFF, &type);

    /*
     * Unmap input buffers.
     */
    for (bufs = 0; bufs < _CAMERA_VI_BUFS; bufs++) {
        if (NULL != cam_ptr->_CAM_obj.vi.buf[bufs].start_ptr) {
            munmap(cam_ptr->_CAM_obj.vi.buf[bufs].start_ptr,
                    cam_ptr->_CAM_obj.vi.buf[bufs].length);
            cam_ptr->_CAM_obj.vi.buf[bufs].start_ptr = NULL;
        }
        else {
            DBG("");
        }
    }

    if (cam_ptr->_CAM_obj.vi.fd > -1) {
        close(cam_ptr->_CAM_obj.vi.fd);
        cam_ptr->_CAM_obj.vi.fd = -1;
    }
    cam_ptr->_CAM_obj.vi.lastbuf.index = -1;
    cam_ptr->_CAM_inited = 0;
    pthread_mutex_unlock(&_CAM_apiLock);
    DBG("Camera stopped");
    return (0);
}

/*
 * ======== CAM_videoIn() ========
 * This function is called by video engine to get an image from capture device.
 * Returns:
 *  0: Success.
 *  -1: Failed.
 */
int CAM_videoIn(
    CAMERA        *cam_ptr)
{
    fd_set  fds;
    int fd;
    struct timeval time;
    uint64_t t;
    pthread_mutex_lock(&_CAM_apiLock);

    cam_ptr->encDataObj->buf_ptr = NULL;

    if (!cam_ptr->_CAM_inited) {
        pthread_mutex_unlock(&_CAM_apiLock);
        usleep(100000);
        DBG("");
        return (-1);
    }

   /*
    * Enqueue last used buffer. Give it back to driver.
    */
    if (-1 != (int)cam_ptr->_CAM_obj.vi.lastbuf.index) {
        ioctl (cam_ptr->_CAM_obj.vi.fd,
                VIDIOC_QBUF,
                &cam_ptr->_CAM_obj.vi.lastbuf);
        cam_ptr->_CAM_obj.vi.lastbuf.index = -1;
    }

    /*
     * Wait for video to become available.
     */
    FD_ZERO(&fds);
    FD_SET(cam_ptr->_CAM_obj.vi.fd, &fds);
    fd = cam_ptr->_CAM_obj.vi.fd;
    time.tv_sec = 1;
    time.tv_usec = 0;

    pthread_mutex_unlock(&_CAM_apiLock);
    if (select(fd + 1, &fds, NULL, NULL, &time) <= 0) {
        DBG("%s %d\n", __FILE__, __LINE__);
        return (-1);
    }
    /*
     * Dequeue a buffer
     */
    pthread_mutex_lock(&_CAM_apiLock);
    cam_ptr->_CAM_obj.vi.lastbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    cam_ptr->_CAM_obj.vi.lastbuf.memory = V4L2_MEMORY_MMAP;
    if (-1 == ioctl(cam_ptr->_CAM_obj.vi.fd,
            VIDIOC_DQBUF,
            &cam_ptr->_CAM_obj.vi.lastbuf)) {
        cam_ptr->_CAM_obj.vi.lastbuf.index = -1;
        DBG("");
        pthread_mutex_unlock(&_CAM_apiLock);
        usleep(100000);
        return (-1);
    }

    if (NULL == cam_ptr->_CAM_obj.vi.buf[cam_ptr->_CAM_obj.vi.lastbuf.index].start_ptr) {
        DBG("");
        pthread_mutex_unlock(&_CAM_apiLock);
        return (-1);
    }
    if (cam_ptr->_CAM_obj.vi.buf[cam_ptr->_CAM_obj.vi.lastbuf.index].length <
            (cam_ptr->_CAM_obj.vi.width * cam_ptr->_CAM_obj.vi.height * 2)) {
        DBG("");
        pthread_mutex_unlock(&_CAM_apiLock);
        return (-1);
    }

    cam_ptr->encDataObj->buf_ptr = cam_ptr->_CAM_obj.vi.buf[cam_ptr->_CAM_obj.vi.lastbuf.index].start_ptr;
    cam_ptr->encDataObj->width = cam_ptr->_CAM_obj.vi.width;
    cam_ptr->encDataObj->height = cam_ptr->_CAM_obj.vi.height;

    gettimeofday(&time, NULL);
    time.tv_sec -= cam_ptr->_CAM_obj.inittime.tv_sec;
    t = (uint64_t)time.tv_sec;
    t *= (uint64_t)1000000 ;
    t += (uint64_t)time.tv_usec;

    cam_ptr->encDataObj->format = VIDEO_FORMAT_YUYV;

    cam_ptr->encDataObj->id = VIDEO_CALLID_CAMERA;
    cam_ptr->encDataObj->tsMs = t;
    pthread_mutex_unlock(&_CAM_apiLock);
    return (0);
}

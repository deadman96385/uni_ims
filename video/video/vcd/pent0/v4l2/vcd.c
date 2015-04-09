/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
 */

#include <sys/time.h>
#include <linux/videodev2.h>
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
#include "vcd.h"

#define DBG(fmt, args...) \
        printf("%s %d:" fmt, __FILE__, __LINE__, ## args)

#define _VCD_VI_BUFS (2)


/*
 * A structure to hold video input data and info.
 */
typedef struct {
    struct {
        int                  fd;
        struct v4l2_buffer   lastbuf;
        struct {
            void *start_ptr;
            int   length;
        } buf[_VCD_VI_BUFS];
        int width;
        int height;
    } vi;
    int muted;
    struct timeval inittime;
    char devname[128];
} _VCD_Obj;

_VCD_Obj _VCD_obj;
static vint _VCD_inited = 0;
static pthread_mutex_t _VCD_apiLock = PTHREAD_MUTEX_INITIALIZER;


/*
 * ======== VCD_init() ========
 * This function is called by video engine to init video capture driver.
 * Returns:
 *  0: Success.
 *  -1: Failed.
 */
int VCD_init(
    int width,
    int height,
    int id,
    int flags)
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
    pthread_mutex_lock(&_VCD_apiLock);

    if (_VCD_inited) {
        DBG("");
        pthread_mutex_unlock(&_VCD_apiLock);
        return (-1);
    }

    OSAL_snprintf(_VCD_obj.devname,  sizeof(_VCD_obj.devname),
            "/dev/video%d", id);
    DBG("Devname is %s\n", _VCD_obj.devname);

    /*
     * Using video4linux2 for video capture.
     * For Linux this would be fine on all platforms.
     */

    /*
     * Open device, check capabilities.
     * - capture
     * - stream
     */
    _VCD_obj.vi.fd = open(_VCD_obj.devname, O_RDWR | O_NONBLOCK, 0);
    if (_VCD_obj.vi.fd < 0) {
        DBG("");
        pthread_mutex_unlock(&_VCD_apiLock);
        return (-1);
    }
    if (-1 == ioctl(_VCD_obj.vi.fd,
            VIDIOC_QUERYCAP,
            &cap)) {
        close(_VCD_obj.vi.fd);
        DBG("");
        pthread_mutex_unlock(&_VCD_apiLock);
        return (-1);
    }
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        close(_VCD_obj.vi.fd);
        DBG("");
        pthread_mutex_unlock(&_VCD_apiLock);
        return (-1);
    }
    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        close(_VCD_obj.vi.fd);
        DBG("");
        pthread_mutex_unlock(&_VCD_apiLock);
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
    if(-1 == ioctl (_VCD_obj.vi.fd,
            VIDIOC_S_FMT,
            &fmt)) {
        close(_VCD_obj.vi.fd);
        DBG("");
        pthread_mutex_unlock(&_VCD_apiLock);
        return (-1);
    }

    /*
     * Request buffers A/B
     * Buffers be mmap
     */
    memset(&req, 0, sizeof(req));
    req.count   = _VCD_VI_BUFS;
    req.type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory  = V4L2_MEMORY_MMAP;
    if(-1 == ioctl(_VCD_obj.vi.fd,
            VIDIOC_REQBUFS,
            &req)) {
        close(_VCD_obj.vi.fd);
        DBG("");
        pthread_mutex_unlock(&_VCD_apiLock);
        return (-1);
    }
    if (_VCD_VI_BUFS != req.count) {
        close(_VCD_obj.vi.fd);
        DBG("");
        pthread_mutex_unlock(&_VCD_apiLock);
        return (-1);
    }

    /*
     * Memory map buffers
     */
    for (bufs = 0; bufs < _VCD_VI_BUFS; bufs++) {

        memset(&buf, 0, sizeof(buf));
        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = bufs;

        /*
         * Find buffer size then mmap it.
         */
        if (-1 == ioctl (_VCD_obj.vi.fd,
                VIDIOC_QUERYBUF,
                &buf)) {
            close(_VCD_obj.vi.fd);
            DBG("");
            pthread_mutex_unlock(&_VCD_apiLock);
            return (-1);
        }

        _VCD_obj.vi.buf[bufs].length = buf.length;
        _VCD_obj.vi.buf[bufs].start_ptr = mmap(NULL,
              buf.length,
              PROT_READ | PROT_WRITE,
              MAP_SHARED,
              _VCD_obj.vi.fd,
              buf.m.offset);
        if (MAP_FAILED == _VCD_obj.vi.buf[bufs].start_ptr) {
            close(_VCD_obj.vi.fd);
            _VCD_obj.vi.buf[bufs].start_ptr = NULL;
            DBG("");
            pthread_mutex_unlock(&_VCD_apiLock);
            return (-1);
        }
    }

    _VCD_obj.muted = 0;
    _VCD_obj.vi.width = width;
    _VCD_obj.vi.height = height;
    gettimeofday(&_VCD_obj.inittime, NULL);

    /*
     * Start video in.
     * Enqueue empty buffers in video capture.
     */
    for (bufs = 0; bufs < _VCD_VI_BUFS; bufs++) {
        memset(&buf, 0, sizeof(buf));
        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = bufs;
        if (-1 == ioctl (_VCD_obj.vi.fd,
                VIDIOC_QBUF,
                &buf)) {
            DBG("");
            pthread_mutex_unlock(&_VCD_apiLock);
            VCD_shutdown();
            return (-1);
        }
    }

    /*
     * Start streaming
     */
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl (_VCD_obj.vi.fd,
            VIDIOC_STREAMON,
            &type)) {
        DBG("");
        pthread_mutex_unlock(&_VCD_apiLock);
        VCD_shutdown();
        return (-1);
    }

    _VCD_obj.vi.lastbuf.index = -1;

    _VCD_inited = 1;

    pthread_mutex_unlock(&_VCD_apiLock);
    DBG("Camera Started\n");
    return (0);
}

/*
 * ======== VCD_shutdown() ========
 * This function is called by video engine to shutdown video capture driver.
 * Returns:
 *  0: Success.
 *  -1: Failed.
 */
int VCD_shutdown(
    void)
{
    enum v4l2_buf_type type;
    int                bufs;
    DBG("");

    pthread_mutex_lock(&_VCD_apiLock);
    if (!_VCD_inited) {
        DBG("");
        pthread_mutex_unlock(&_VCD_apiLock);
        return (-1);
    }

    /*
     * Stop video input.
     */
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(_VCD_obj.vi.fd, VIDIOC_STREAMOFF, &type);

    /*
     * Unmap input buffers.
     */
    for (bufs = 0; bufs < _VCD_VI_BUFS; bufs++) {
        if (NULL != _VCD_obj.vi.buf[bufs].start_ptr) {
            munmap(_VCD_obj.vi.buf[bufs].start_ptr,
                    _VCD_obj.vi.buf[bufs].length);
            _VCD_obj.vi.buf[bufs].start_ptr = NULL;
        }
        else {
            DBG("");
        }
    }

    if (_VCD_obj.vi.fd > -1) {
        close(_VCD_obj.vi.fd);
        _VCD_obj.vi.fd = -1;
    }
    _VCD_obj.vi.lastbuf.index = -1;
    _VCD_inited = 0;
    pthread_mutex_unlock(&_VCD_apiLock);
    DBG("Camera stopped");
    return (0);
}

/*
 * ======== VCD_mute() ========
 * Mutes VCD so that VCD sets a mute flag in output picture.
 * Returns:
 * -1 : Error
 *  0 : Success
 */
int VCD_mute(
    void)
{
    pthread_mutex_lock(&_VCD_apiLock);
    if (!_VCD_inited) {
        DBG("");
        pthread_mutex_unlock(&_VCD_apiLock);
        return (-1);
    }

    if (_VCD_obj.muted) {
        pthread_mutex_unlock(&_VCD_apiLock);
        DBG("");
        return (-1);
    }
    _VCD_obj.muted = 1;
    pthread_mutex_unlock(&_VCD_apiLock);
    return (0);
}

/*
 * ======== VCD_unmute() ========
 * Mutes VCD so that VCD unsets a mute flag in output picture.
 * Returns:
 * -1 : Error
 *  0 : Success
 */
int VCD_unmute(
    void)
{
    pthread_mutex_lock(&_VCD_apiLock);
    if (!_VCD_inited) {
        DBG("");
        pthread_mutex_unlock(&_VCD_apiLock);
        return (-1);
    }

    if (!_VCD_obj.muted) {
        DBG("");
        pthread_mutex_unlock(&_VCD_apiLock);
        return (-1);
    }
    _VCD_obj.muted = 0;
    pthread_mutex_unlock(&_VCD_apiLock);
    return (0);
}

/*
 * ======== VCD_videoIn() ========
 * This function is called by video engine to get an image from capture device.
 * Returns:
 *  0: Success.
 *  -1: Failed.
 */
int VCD_videoIn(
    Video_Picture *pic_ptr)
{
    fd_set  fds;
    int fd;
    struct timeval time;
    uint64_t t;
    pthread_mutex_lock(&_VCD_apiLock);

    pic_ptr->base_ptr = NULL;
    pic_ptr->size = 0;

    if (NULL == pic_ptr) {
        pthread_mutex_unlock(&_VCD_apiLock);
        DBG("");
        return (-1);
    }

    if (!_VCD_inited) {
        pthread_mutex_unlock(&_VCD_apiLock);
        usleep(100000);
        DBG("");
        return (-1);
    }

   /*
    * Enqueue last used buffer. Give it back to driver.
    */
    if (-1 != (int)_VCD_obj.vi.lastbuf.index) {
        ioctl (_VCD_obj.vi.fd,
                VIDIOC_QBUF,
                &_VCD_obj.vi.lastbuf);
        _VCD_obj.vi.lastbuf.index = -1;
    }

    /*
     * Wait for video to become available.
     */
    FD_ZERO(&fds);
    FD_SET(_VCD_obj.vi.fd, &fds);
    fd = _VCD_obj.vi.fd;
    time.tv_sec = 1;
    time.tv_usec = 0;

    pthread_mutex_unlock(&_VCD_apiLock);
    if (select(fd + 1, &fds, NULL, NULL, &time) <= 0) {
        DBG("%s %d\n", __FILE__, __LINE__);
        return (-1);
    }

    /*
     * Dequeue a buffer
     */
    pthread_mutex_lock(&_VCD_apiLock);
    _VCD_obj.vi.lastbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    _VCD_obj.vi.lastbuf.memory = V4L2_MEMORY_MMAP;
    if (-1 == ioctl(_VCD_obj.vi.fd,
            VIDIOC_DQBUF,
            &_VCD_obj.vi.lastbuf)) {
        _VCD_obj.vi.lastbuf.index = -1;
        DBG("");
        pthread_mutex_unlock(&_VCD_apiLock);
        usleep(100000);
        return (-1);
    }

    if (NULL == _VCD_obj.vi.buf[_VCD_obj.vi.lastbuf.index].start_ptr) {
        DBG("");
        pthread_mutex_unlock(&_VCD_apiLock);
        return (-1);
    }
    if (_VCD_obj.vi.buf[_VCD_obj.vi.lastbuf.index].length <
            (_VCD_obj.vi.width * _VCD_obj.vi.height * 2)) {
        DBG("");
        pthread_mutex_unlock(&_VCD_apiLock);
        return (-1);
    }

    pic_ptr->base_ptr = _VCD_obj.vi.buf[_VCD_obj.vi.lastbuf.index].start_ptr;
    pic_ptr->size = _VCD_obj.vi.buf[_VCD_obj.vi.lastbuf.index].length;;
    pic_ptr->width = _VCD_obj.vi.width;
    pic_ptr->height = _VCD_obj.vi.height;

    gettimeofday(&time, NULL);
    time.tv_sec -= _VCD_obj.inittime.tv_sec;
    t = (uint64_t)time.tv_sec;
    t *= (uint64_t)1000000 ;
    t += (uint64_t)time.tv_usec;

    pic_ptr->format = VIDEO_FORMAT_YUYV;
    pic_ptr->bpp = 16;
    pic_ptr->stride = pic_ptr->width;
    pic_ptr->id = VIDEO_CALLID_CAMERA;
    pic_ptr->ts = t;
    pic_ptr->muted = _VCD_obj.muted;
    pthread_mutex_unlock(&_VCD_apiLock);
    return (0);
}

/*
 * ======== VCD_requestResolution() ========
 * Request change in resolution for the VCD device.
 * Returns:
 * -1 : Error
 *  0 : Success
 */
extern int OS_changeCameraResolutionRequest(int width, int height);
int VCD_requestResolution(
    int width,
    int height)
{

    /*
     * This should ask application to change resolution.
     */
    return(OS_changeCameraResolutionRequest(width, height));
}

/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
 */

#include <X11/Xlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <video.h>
#include <dsp_scale.h>
#include "vdd.h"

#define _VDD_BPP (24)
#define _VDD_MAX_CALLS (4)

typedef struct {
    struct {
        Display *dpy_ptr;
        Visual  *visual_ptr;
        XImage  *image_ptr;
        Window   w;
        GC       gc;
        char    *mem_ptr;
        int      bypp;  
        int      width;
        int      height;
        int      flags;
    } vo;
    char *scratch_ptr;
} _VDD_Obj;

_VDD_Obj _VDD_obj[_VDD_MAX_CALLS + 1];
static pthread_mutex_t _VDD_apiLock = PTHREAD_MUTEX_INITIALIZER;

/*
 * ======== VDD_init() ========
 * This function is called by video engine to init video display driver.
 * Returns:
 *  0: Success.
 *  -1: Failed.
 */
int VDD_init(
    int               callId,
    int               flags)
{
    pthread_mutex_lock(&_VDD_apiLock);

    if (VIDEO_CALLID_CAMERA == callId) {
        callId = _VDD_MAX_CALLS;
    }

    if ((callId < 0) || (callId > _VDD_MAX_CALLS)) {
        printf("%s:%d\n", __FILE__, __LINE__);
        pthread_mutex_unlock(&_VDD_apiLock);
        return (-1);
    }

    if(NULL != _VDD_obj[callId].vo.dpy_ptr) {
        printf("%s:%d\n", __FILE__, __LINE__);
        pthread_mutex_unlock(&_VDD_apiLock);
        return (-1);
    }

    _VDD_obj[callId].vo.dpy_ptr = XOpenDisplay(0);
    if (NULL == _VDD_obj[callId].vo.dpy_ptr) {
        printf("%s:%d\n", __FILE__, __LINE__);
        pthread_mutex_unlock(&_VDD_apiLock);
        return (-1);
    }

    _VDD_obj[callId].vo.width = 0;
    _VDD_obj[callId].vo.height = 0;
    pthread_mutex_unlock(&_VDD_apiLock);
    return (0);
}

/*
 * ======== VDD_shutdown() ========
 * This function is called by video engine to shutdown video display driver.
 * Returns:
 *  0: Success.
 *  -1: Failed.
 */
int VDD_shutdown(
    int callId)
{
    pthread_mutex_lock(&_VDD_apiLock);

    if (VIDEO_CALLID_CAMERA == callId) {
        callId = _VDD_MAX_CALLS;
    }

    if ((callId < 0) || (callId > _VDD_MAX_CALLS)) {
        printf("%s:%d\n", __FILE__, __LINE__);
        pthread_mutex_unlock(&_VDD_apiLock);
        return (-1);
    }

    if (NULL == _VDD_obj[callId].vo.dpy_ptr) {
        printf("%s:%d\n", __FILE__, __LINE__);
        pthread_mutex_unlock(&_VDD_apiLock);
        return (-1);
    }
    if (NULL != _VDD_obj[callId].vo.mem_ptr) {
        free(_VDD_obj[callId].vo.mem_ptr);
        _VDD_obj[callId].vo.mem_ptr = NULL;
    }

    if (NULL != _VDD_obj[callId].scratch_ptr) {
        free(_VDD_obj[callId].scratch_ptr);
        _VDD_obj[callId].scratch_ptr = NULL;
    }
    if (NULL != _VDD_obj[callId].vo.gc) {
        XFreeGC(_VDD_obj[callId].vo.dpy_ptr, _VDD_obj[callId].vo.gc);
        _VDD_obj[callId].vo.gc = NULL;
    }

    if (_VDD_obj[callId].vo.w > 0) {
        XDestroyWindow(_VDD_obj[callId].vo.dpy_ptr, _VDD_obj[callId].vo.w);
        _VDD_obj[callId].vo.w = 0;
    }
    XCloseDisplay(_VDD_obj[callId].vo.dpy_ptr);
    _VDD_obj[callId].vo.dpy_ptr = NULL;
    _VDD_obj[callId].vo.width = 0;
    _VDD_obj[callId].vo.height = 0;

    pthread_mutex_unlock(&_VDD_apiLock);
    return (0);
}

/*
 * ======== VDD_shutdown() ========
 * This function is called by video engine to clear the display.
 * Returns:
 *  0: Success.
 *  -1: Failed.
 */
int VDD_clear(
    int  callId)
{
    pthread_mutex_lock(&_VDD_apiLock);

    if (VIDEO_CALLID_CAMERA == callId) {
        callId = _VDD_MAX_CALLS;
    }

    if ((callId < 0) || (callId > _VDD_MAX_CALLS)) {
        printf("%s:%d\n", __FILE__, __LINE__);
        pthread_mutex_unlock(&_VDD_apiLock);
        return (-1);
    }

    if (NULL == _VDD_obj[callId].vo.mem_ptr) {
        printf("%s:%d\n", __FILE__, __LINE__);
        pthread_mutex_unlock(&_VDD_apiLock);
        return (-1);
    }

    if ((0 == _VDD_obj[callId].vo.width) || (0 == _VDD_obj[callId].vo.height)) {
        printf("%s:%d\n", __FILE__, __LINE__);
        pthread_mutex_unlock(&_VDD_apiLock);
        return (-1);
    }

    memset(_VDD_obj[callId].vo.mem_ptr, 0, _VDD_obj[callId].vo.width *
            _VDD_obj[callId].vo.height * _VDD_obj[callId].vo.bypp);

    XPutImage(_VDD_obj[callId].vo.dpy_ptr,
            _VDD_obj[callId].vo.w, _VDD_obj[callId].vo.gc,
            _VDD_obj[callId].vo.image_ptr, 0, 0, 0, 0,
            _VDD_obj[callId].vo.width, _VDD_obj[callId].vo.height);
    pthread_mutex_unlock(&_VDD_apiLock);
    return (0);
}

/*
 * ======== VDD_videoOut() ========
 * This function is called by video engine to send an image to display screen.
 * Returns:
 *  0: Success.
 *  -1: Failed.
 */
int VDD_videoOut(
    Video_Picture *pic_ptr)
{
    vint callId;
    pthread_mutex_lock(&_VDD_apiLock);

    if (NULL == pic_ptr) {
        printf("%s:%d\n", __FILE__, __LINE__);
        pthread_mutex_unlock(&_VDD_apiLock);
        return (-1);
    }

    if (VIDEO_CALLID_CAMERA == pic_ptr->id) {
        callId = _VDD_MAX_CALLS;
    }
    else {
        callId = pic_ptr->id;
    }
    if (NULL == pic_ptr->base_ptr) {
        printf("%s:%d\n", __FILE__, __LINE__);
        pthread_mutex_unlock(&_VDD_apiLock);
        return (-1);
    }
    
    if ((callId < 0) || (callId > _VDD_MAX_CALLS)) {
        printf("%s:%d\n", __FILE__, __LINE__);
        pthread_mutex_unlock(&_VDD_apiLock);
        return (-1);
    }

    if (NULL == _VDD_obj[callId].vo.dpy_ptr) {
        printf("%s:%d\n", __FILE__, __LINE__);
        pthread_mutex_unlock(&_VDD_apiLock);
        return (-1);
    }

    if ((0 == pic_ptr->width) || (0 == pic_ptr->height)) {
        printf("%s:%d\n", __FILE__, __LINE__);
        pthread_mutex_unlock(&_VDD_apiLock);
        return (-1);
    }

    if ((_VDD_obj[callId].vo.width != pic_ptr->width) ||
            (_VDD_obj[callId].vo.height != pic_ptr->height)) {

        /*
         * Realloc
         */
        if (NULL != _VDD_obj[callId].vo.mem_ptr) {
            free(_VDD_obj[callId].vo.mem_ptr);
            _VDD_obj[callId].vo.mem_ptr = NULL;
        }

        if (NULL != _VDD_obj[callId].scratch_ptr) {
            free(_VDD_obj[callId].scratch_ptr);
            _VDD_obj[callId].scratch_ptr = NULL;
        }
        if (NULL != _VDD_obj[callId].vo.gc) {
            XFreeGC(_VDD_obj[callId].vo.dpy_ptr, _VDD_obj[callId].vo.gc);
            _VDD_obj[callId].vo.gc = NULL;
        }

        if (_VDD_obj[callId].vo.w > 0) {
            XDestroyWindow(_VDD_obj[callId].vo.dpy_ptr, _VDD_obj[callId].vo.w);
            _VDD_obj[callId].vo.w = 0;
        }

        _VDD_obj[callId].scratch_ptr = malloc(pic_ptr->width * pic_ptr->height * 4);
        if (NULL == _VDD_obj[callId].scratch_ptr) {
            printf("%s:%d\n", __FILE__, __LINE__);
            pthread_mutex_unlock(&_VDD_apiLock);
            return (-1);
        }

        _VDD_obj[callId].vo.w = XCreateWindow(_VDD_obj[callId].vo.dpy_ptr,
                DefaultRootWindow(_VDD_obj[callId].vo.dpy_ptr), 0, 0,
                pic_ptr->width, pic_ptr->height, 0, _VDD_BPP, CopyFromParent,
                _VDD_obj[callId].vo.visual_ptr,
                0, NULL);

        XSelectInput(_VDD_obj[callId].vo.dpy_ptr,
                _VDD_obj[callId].vo.w, StructureNotifyMask);
        XMapWindow(_VDD_obj[callId].vo.dpy_ptr,
                _VDD_obj[callId].vo.w);

        for(;;) {
            XEvent e;
            XNextEvent(_VDD_obj[callId].vo.dpy_ptr, &e);
            if (e.type == MapNotify) {
                break;
            }
        }

        _VDD_obj[callId].vo.gc = XCreateGC(_VDD_obj[callId].vo.dpy_ptr,
                _VDD_obj[callId].vo.w, 0, 0);

        switch(_VDD_BPP) {
            case 16:
                _VDD_obj[callId].vo.bypp = 2;
                break;
            default:
                _VDD_obj[callId].vo.bypp = 4;
                break;
        }
        _VDD_obj[callId].vo.mem_ptr = (char *)malloc(pic_ptr->width * pic_ptr->height *
                _VDD_obj[callId].vo.bypp);

        if(NULL == _VDD_obj[callId].vo.mem_ptr) {
            printf("%s:%d\n", __FILE__, __LINE__);
            free(_VDD_obj[callId].scratch_ptr);
            pthread_mutex_unlock(&_VDD_apiLock);
            return (-1);
        }

        _VDD_obj[callId].vo.image_ptr =
                XCreateImage(_VDD_obj[callId].vo.dpy_ptr,
                _VDD_obj[callId].vo.visual_ptr, 24, ZPixmap, 0,
                _VDD_obj[callId].vo.mem_ptr,
                pic_ptr->width, pic_ptr->height, 32,
                _VDD_obj[callId].vo.bypp * pic_ptr->width);
        if (NULL == _VDD_obj[callId].vo.image_ptr) {
            printf("%s:%d\n", __FILE__, __LINE__);
            free(_VDD_obj[callId].scratch_ptr);
            pthread_mutex_unlock(&_VDD_apiLock);
            return (-1);
        }

        _VDD_obj[callId].vo.width = pic_ptr->width;
        _VDD_obj[callId].vo.height = pic_ptr->height;
    }

    Dsp_scale((void *)_VDD_obj[callId].vo.mem_ptr,
            pic_ptr->width, pic_ptr->height, VIDEO_FORMAT_RGB_32,
            (void *)pic_ptr->base_ptr,
            pic_ptr->width, pic_ptr->height, pic_ptr->format);

    XPutImage(_VDD_obj[callId].vo.dpy_ptr,
            _VDD_obj[callId].vo.w, _VDD_obj[callId].vo.gc,
            _VDD_obj[callId].vo.image_ptr, 0, 0, 0, 0,
            _VDD_obj[callId].vo.width, _VDD_obj[callId].vo.height);
    pthread_mutex_unlock(&_VDD_apiLock);
    return (0);
}


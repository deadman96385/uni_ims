/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
 */

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <video.h>
#include <img_convert.h>
#include "view.h"

#define _VIEW_BPP (24)

static pthread_mutex_t _VIEW_apiLock = PTHREAD_MUTEX_INITIALIZER;

/*
 * ======== _VIEW_Task() ========
 *
 * Camera task: To get received raw video frame & captured video image to display on screen.
 * TODO: To get captured video image.
 *
 * Return Values:
 * None.
 */
static void _VIEW_Task(
    void *arg_ptr)
{
    /*
    * Big D2 Banner with version information.
    */
    OSAL_logMsg("<<<<<<<<<<<<<<< VIEW_TASK >>>>>>>>>>>>>>>>>>\n");
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
    VIEW *view_ptr = (VIEW *)arg_ptr;

_VIEW_TASK_LOOP:

        /* TODO: Get Captured video frame from Camera */

        /* Get Raw Video frame from DECODER. */
        if(view_ptr->decDataObj->codecInited) {
            if(view_ptr->getDecCodecQOutputBuffer(view_ptr->decDataObj)) {
                /* Show on the screen. */
                VIEW_videoOut(view_ptr);
            }
        }

        /* Show, Change view every 30m */
        usleep(30000);

        /* Loop as long as the encoder is initialized. */
        if (OSAL_TRUE == view_ptr->task_obj.thread_started) {
            goto _VIEW_TASK_LOOP;
        }
        else {
            OSAL_logMsg("%s:%d _VIEW_Task exited\n", __FILE__, __LINE__);
        }
}

/*
 * ======== VIEW_init() ========
 * This function is called by video engine to initialize video display driver.
 * Returns:
 *  0: Success.
 *  -1: Failed.
 */
int VIEW_init(
    VIEW      *view_ptr,
    int        callId,
    int        flags)
{
    pthread_mutex_lock(&_VIEW_apiLock);

    if (VIDEO_CALLID_CAMERA == callId) {
        callId = _VIEW_MAX_CALLS;
    }

    if ((callId < 0) || (callId > _VIEW_MAX_CALLS)) {
        printf("%s:%d\n", __FILE__, __LINE__);
        pthread_mutex_unlock(&_VIEW_apiLock);
        return (-1);
    }

    if(NULL != view_ptr->_VIEW_obj[callId].vo.dpy_ptr) {
        printf("%s:%d\n", __FILE__, __LINE__);
        pthread_mutex_unlock(&_VIEW_apiLock);
        return (-1);
    }

    view_ptr->_VIEW_obj[callId].vo.dpy_ptr = XOpenDisplay(0);
    if (NULL == view_ptr->_VIEW_obj[callId].vo.dpy_ptr) {
        printf("%s:%d\n", __FILE__, __LINE__);
        pthread_mutex_unlock(&_VIEW_apiLock);
        return (-1);
    }

    view_ptr->_VIEW_obj[callId].vo.width = 0;
    view_ptr->_VIEW_obj[callId].vo.height = 0;
    pthread_mutex_unlock(&_VIEW_apiLock);

    view_ptr->task_obj.thread_started = OSAL_TRUE;
    /* Init the task used to feed encoder. */
    view_ptr->task_obj.taskId = 0;
    view_ptr->task_obj.stackSize = _VIEW_TASK_STACK_SZ;
    view_ptr->task_obj.taskPriority = _VIEW_TASK_PRIORITY;
    view_ptr->task_obj.func_ptr = _VIEW_Task;

    OSAL_snprintf(view_ptr->task_obj.name, sizeof(view_ptr->task_obj.name), "%s",
            VIEW_TASK_NAME);

    view_ptr->task_obj.arg_ptr = (void *)view_ptr;

    if (0 == (view_ptr->task_obj.taskId = OSAL_taskCreate(
        view_ptr->task_obj.name,
        view_ptr->task_obj.taskPriority,
        view_ptr->task_obj.stackSize,
        view_ptr->task_obj.func_ptr,
        view_ptr->task_obj.arg_ptr))){
        OSAL_logMsg("%s:%d _VIEW_Task Stopped\n", __FILE__, __LINE__);
        return (1);
    }

    return (0);
}

/*
 * ======== VIEW_shutdown() ========
 * This function is called by video engine to shutdown video display driver.
 * Returns:
 *  0: Success.
 *  -1: Failed.
 */
int VIEW_shutdown(
    VIEW *view_ptr,
    int   callId)
{
    view_ptr->task_obj.thread_started = OSAL_FALSE;

    pthread_mutex_lock(&_VIEW_apiLock);

    if (VIDEO_CALLID_CAMERA == callId) {
        callId = _VIEW_MAX_CALLS;
    }

    if ((callId < 0) || (callId > _VIEW_MAX_CALLS)) {
        printf("%s:%d\n", __FILE__, __LINE__);
        pthread_mutex_unlock(&_VIEW_apiLock);
        return (-1);
    }

    if (NULL == view_ptr->_VIEW_obj[callId].vo.dpy_ptr) {
        printf("%s:%d\n", __FILE__, __LINE__);
        pthread_mutex_unlock(&_VIEW_apiLock);
        return (-1);
    }
    if (NULL != view_ptr->_VIEW_obj[callId].vo.mem_ptr) {
        free(view_ptr->_VIEW_obj[callId].vo.mem_ptr);
        view_ptr->_VIEW_obj[callId].vo.mem_ptr = NULL;
    }

    if (NULL != view_ptr->_VIEW_obj[callId].scratch_ptr) {
        free(view_ptr->_VIEW_obj[callId].scratch_ptr);
        view_ptr->_VIEW_obj[callId].scratch_ptr = NULL;
    }
    if (NULL != view_ptr->_VIEW_obj[callId].vo.gc) {
        XFreeGC(view_ptr->_VIEW_obj[callId].vo.dpy_ptr, view_ptr->_VIEW_obj[callId].vo.gc);
        view_ptr->_VIEW_obj[callId].vo.gc = NULL;
    }

    if (view_ptr->_VIEW_obj[callId].vo.w > 0) {
        XDestroyWindow(view_ptr->_VIEW_obj[callId].vo.dpy_ptr, view_ptr->_VIEW_obj[callId].vo.w);
        view_ptr->_VIEW_obj[callId].vo.w = 0;
    }
    XCloseDisplay(view_ptr->_VIEW_obj[callId].vo.dpy_ptr);
    view_ptr->_VIEW_obj[callId].vo.dpy_ptr = NULL;
    view_ptr->_VIEW_obj[callId].vo.width = 0;
    view_ptr->_VIEW_obj[callId].vo.height = 0;

    pthread_mutex_unlock(&_VIEW_apiLock);
    return (0);
}

/*
 * ======== VIEW_shutdown() ========
 * This function is called by video engine to clear the display.
 * Returns:
 *  0: Success.
 *  -1: Failed.
 */
int VIEW_clear(
    VIEW *view_ptr,
    int   callId)
{
    pthread_mutex_lock(&_VIEW_apiLock);

    if (VIDEO_CALLID_CAMERA == callId) {
        callId = _VIEW_MAX_CALLS;
    }

    if ((callId < 0) || (callId > _VIEW_MAX_CALLS)) {
        printf("%s:%d\n", __FILE__, __LINE__);
        pthread_mutex_unlock(&_VIEW_apiLock);
        return (-1);
    }

    if (NULL == view_ptr->_VIEW_obj[callId].vo.mem_ptr) {
        printf("%s:%d\n", __FILE__, __LINE__);
        pthread_mutex_unlock(&_VIEW_apiLock);
        return (-1);
    }

    if ((0 == view_ptr->_VIEW_obj[callId].vo.width) || (0 == view_ptr->_VIEW_obj[callId].vo.height)) {
        printf("%s:%d\n", __FILE__, __LINE__);
        pthread_mutex_unlock(&_VIEW_apiLock);
        return (-1);
    }

    memset(view_ptr->_VIEW_obj[callId].vo.mem_ptr, 0, view_ptr->_VIEW_obj[callId].vo.width *
            view_ptr->_VIEW_obj[callId].vo.height * view_ptr->_VIEW_obj[callId].vo.bypp);

    XPutImage(view_ptr->_VIEW_obj[callId].vo.dpy_ptr,
            view_ptr->_VIEW_obj[callId].vo.w, view_ptr->_VIEW_obj[callId].vo.gc,
            view_ptr->_VIEW_obj[callId].vo.image_ptr, 0, 0, 0, 0,
            view_ptr->_VIEW_obj[callId].vo.width, view_ptr->_VIEW_obj[callId].vo.height);
    pthread_mutex_unlock(&_VIEW_apiLock);
    return (0);
}

/*
 * ======== VIEW_videoOut() ========
 * This function is called by video engine to send an image to display screen.
 * Returns:
 *  0: Success.
 *  -1: Failed.
 */
int VIEW_videoOut(
    VIEW          *view_ptr)
{
    vint callId;
    pthread_mutex_lock(&_VIEW_apiLock);

    if (VIDEO_CALLID_CAMERA == view_ptr->decDataObj->id) {
        callId = _VIEW_MAX_CALLS;
    }
    else {
        callId = view_ptr->decDataObj->id;
    }
    if (NULL == view_ptr->decDataObj->outbuf) {
        printf("%s:%d\n", __FILE__, __LINE__);
        pthread_mutex_unlock(&_VIEW_apiLock);
        return (-1);
    }
    
    if ((callId < 0) || (callId > _VIEW_MAX_CALLS)) {
        printf("%s:%d\n", __FILE__, __LINE__);
        pthread_mutex_unlock(&_VIEW_apiLock);
        return (-1);
    }

    if (NULL == view_ptr->_VIEW_obj[callId].vo.dpy_ptr) {
        printf("%s:%d\n", __FILE__, __LINE__);
        pthread_mutex_unlock(&_VIEW_apiLock);
        return (-1);
    }

    if ((0 == view_ptr->decDataObj->width) || (0 == view_ptr->decDataObj->height)) {
        printf("%s:%d\n", __FILE__, __LINE__);
        pthread_mutex_unlock(&_VIEW_apiLock);
        return (-1);
    }

    if ((view_ptr->_VIEW_obj[callId].vo.width != view_ptr->decDataObj->width) ||
            (view_ptr->_VIEW_obj[callId].vo.height != view_ptr->decDataObj->height)) {

        /*
         * Realloc
         */
        if (NULL != view_ptr->_VIEW_obj[callId].vo.mem_ptr) {
            free(view_ptr->_VIEW_obj[callId].vo.mem_ptr);
            view_ptr->_VIEW_obj[callId].vo.mem_ptr = NULL;
        }

        if (NULL != view_ptr->_VIEW_obj[callId].scratch_ptr) {
            free(view_ptr->_VIEW_obj[callId].scratch_ptr);
            view_ptr->_VIEW_obj[callId].scratch_ptr = NULL;
        }
        if (NULL != view_ptr->_VIEW_obj[callId].vo.gc) {
            XFreeGC(view_ptr->_VIEW_obj[callId].vo.dpy_ptr, view_ptr->_VIEW_obj[callId].vo.gc);
            view_ptr->_VIEW_obj[callId].vo.gc = NULL;
        }

        if (view_ptr->_VIEW_obj[callId].vo.w > 0) {
            XDestroyWindow(view_ptr->_VIEW_obj[callId].vo.dpy_ptr, view_ptr->_VIEW_obj[callId].vo.w);
            view_ptr->_VIEW_obj[callId].vo.w = 0;
        }
        view_ptr->_VIEW_obj[callId].scratch_ptr = malloc(view_ptr->decDataObj->width * view_ptr->decDataObj->height * 4);
        if (NULL == view_ptr->_VIEW_obj[callId].scratch_ptr) {
            printf("%s:%d\n", __FILE__, __LINE__);
            pthread_mutex_unlock(&_VIEW_apiLock);
            return (-1);
        }

        view_ptr->_VIEW_obj[callId].vo.w = XCreateWindow(view_ptr->_VIEW_obj[callId].vo.dpy_ptr,
                DefaultRootWindow(view_ptr->_VIEW_obj[callId].vo.dpy_ptr), 0, 0,
                view_ptr->decDataObj->width, view_ptr->decDataObj->height, 0, _VIEW_BPP, CopyFromParent,
                view_ptr->_VIEW_obj[callId].vo.visual_ptr,
                0, NULL);

        XSelectInput(view_ptr->_VIEW_obj[callId].vo.dpy_ptr,
                view_ptr->_VIEW_obj[callId].vo.w, StructureNotifyMask);
        XMapWindow(view_ptr->_VIEW_obj[callId].vo.dpy_ptr,
                view_ptr->_VIEW_obj[callId].vo.w);

        for(;;) {
            XEvent e;
            XNextEvent(view_ptr->_VIEW_obj[callId].vo.dpy_ptr, &e);
            if (e.type == MapNotify) {
                break;
            }
        }

        view_ptr->_VIEW_obj[callId].vo.gc = XCreateGC(view_ptr->_VIEW_obj[callId].vo.dpy_ptr,
                view_ptr->_VIEW_obj[callId].vo.w, 0, 0);

        switch(_VIEW_BPP) {
            case 16:
                view_ptr->_VIEW_obj[callId].vo.bypp = 2;
                break;
            default:
                view_ptr->_VIEW_obj[callId].vo.bypp = 4;
                break;
        }
        view_ptr->_VIEW_obj[callId].vo.mem_ptr = (char *)malloc(view_ptr->decDataObj->width * view_ptr->decDataObj->height *
                view_ptr->_VIEW_obj[callId].vo.bypp);

        if(NULL == view_ptr->_VIEW_obj[callId].vo.mem_ptr) {
            printf("%s:%d\n", __FILE__, __LINE__);
            free(view_ptr->_VIEW_obj[callId].scratch_ptr);
            pthread_mutex_unlock(&_VIEW_apiLock);
            return (-1);
        }

        view_ptr->_VIEW_obj[callId].vo.image_ptr =
                XCreateImage(view_ptr->_VIEW_obj[callId].vo.dpy_ptr,
                view_ptr->_VIEW_obj[callId].vo.visual_ptr, 24, ZPixmap, 0,
                view_ptr->_VIEW_obj[callId].vo.mem_ptr,
                view_ptr->decDataObj->width, view_ptr->decDataObj->height, 32,
                view_ptr->_VIEW_obj[callId].vo.bypp * view_ptr->decDataObj->width);
        if (NULL == view_ptr->_VIEW_obj[callId].vo.image_ptr) {
            printf("%s:%d\n", __FILE__, __LINE__);
            free(view_ptr->_VIEW_obj[callId].scratch_ptr);
            pthread_mutex_unlock(&_VIEW_apiLock);
            return (-1);
        }

        view_ptr->_VIEW_obj[callId].vo.width = view_ptr->decDataObj->width;
        view_ptr->_VIEW_obj[callId].vo.height = view_ptr->decDataObj->height;
    }

    IMG_formatConvert((void *)view_ptr->_VIEW_obj[callId].vo.mem_ptr,
            view_ptr->decDataObj->width, view_ptr->decDataObj->height, VIDEO_FORMAT_RGB_32,
            (void *)view_ptr->decDataObj->outbuf,
            view_ptr->decDataObj->width, view_ptr->decDataObj->height, view_ptr->decDataObj->format);

    XPutImage(view_ptr->_VIEW_obj[callId].vo.dpy_ptr,
            view_ptr->_VIEW_obj[callId].vo.w, view_ptr->_VIEW_obj[callId].vo.gc,
            view_ptr->_VIEW_obj[callId].vo.image_ptr, 0, 0, 0, 0,
            view_ptr->_VIEW_obj[callId].vo.width, view_ptr->_VIEW_obj[callId].vo.height);
    pthread_mutex_unlock(&_VIEW_apiLock);
    return (0);
}

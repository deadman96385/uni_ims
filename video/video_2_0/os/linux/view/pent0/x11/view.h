/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
 */

#ifndef _VIEW_H_
#define _VIEW_H_

#include <X11/Xlib.h>
#include <video.h>
#include "codec_object.h"

/* Task related settings. */
#define VIEW_TASK_NAME             "View-Task"
#define _VIEW_TASK_PRIORITY        (OSAL_TASK_PRIO_VDEC)
#define _VIEW_TASK_STACK_SZ        (OSAL_STACK_SZ_LARGE)
#define _VIEW_MAX_CALLS            (4)

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
} _VIEW_Obj;

/*
 * Define View task object(thread).
 */
typedef struct {
    OSAL_TaskId      taskId;
    uvint            stackSize;
    uvint            taskPriority;
    void            *func_ptr;
    int8             name[16];
    void            *arg_ptr;
    OSAL_Boolean     thread_started;
} _View_TaskObj;

/*
 * Define View object to interact with DEC.
 */
typedef struct {
    _VIEW_Obj                   _VIEW_obj[_VIEW_MAX_CALLS + 1];
    _View_TaskObj               task_obj;
    DataObj                    *decDataObj;
    Codec_DequeueOutputBuffer   getDecCodecQOutputBuffer;
} VIEW;



/*
 * To init the show for a particular call.
 * Note that surface size is unrelated to width, height arguments.
 * width and height arguments are the dimensions of the picture that will be
 * sent to the surface.
 */
int VIEW_init(
    VIEW      *view_ptr,
    int        callId,
    int        flags);

int VIEW_shutdown(
    VIEW      *view_ptr,
    int        callId);

/*
 * Note: Use callId as
 * 0 ... camera, 1 ... stream 0, 2 ... stream 1
 */

int VIEW_videoOut(
    VIEW      *view_ptr);

int VIEW_clear(
    VIEW      *view_ptr,
    int        callId);

#endif

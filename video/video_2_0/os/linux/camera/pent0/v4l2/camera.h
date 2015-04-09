/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
 */

#ifndef _CAMERA_H_
#define _CAMERA_H_

#include <linux/videodev2.h>
#include <video.h>
#include "codec_object.h"

/* Task related settings. */
#define CAM_TASK_NAME             "Camera-Task"
#define _CAM_TASK_PRIORITY        (OSAL_TASK_PRIO_VDEC)
#define _CAM_TASK_STACK_SZ        (OSAL_STACK_SZ_LARGE)
#define _CAMERA_VI_BUFS           (2)

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
        } buf[_CAMERA_VI_BUFS];
        int width;
        int height;
    } vi;
    int muted;
    struct timeval inittime;
    char devname[128];
} _CAM_Obj;

/*
 * Define Camera task object(thread).
 */
typedef struct {
    OSAL_TaskId      taskId;
    uvint            stackSize;
    uvint            taskPriority;
    void            *func_ptr;
    int8             name[16];
    void            *arg_ptr;
    OSAL_Boolean     thread_started;
} _CAM_TaskObj;

/*
 * Define Camera object to interact with ENC.
 */
typedef struct {
    _CAM_Obj                _CAM_obj;
    _CAM_TaskObj            _cam_task_obj;
    vint                    _CAM_inited;
    DataObj                *encDataObj;
    Codec_QueueInputBuffer  toEncCodecQInputBuffer;
} CAMERA;

int CAM_init(
    CAMERA    *cam_ptr,
    int        width,
    int        height,
    int        id,
    int        flags);

int CAM_shutdown(
    CAMERA    *cam_ptr);

int CAM_videoIn(
    CAMERA    *cam_ptr);

#endif

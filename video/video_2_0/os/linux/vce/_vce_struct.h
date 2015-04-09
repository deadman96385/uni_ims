/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12523 $ $Date: 2010-07-13 18:57:44 -0400 (Tue, 13 Jul 2010) $
 */

#ifndef _VCE_STRUCT_H_
#define _VCE_STRUCT_H_

#include <osal.h>
#include <vce.h>

#include "_vce_const.h"
#include "fsm.h"
#include <codec.h>

#ifdef ENABLE_CAMERA_VIEW
#include "camera.h"
#include "view.h"
#endif


typedef struct {
    VCE_Event   event;
    char        eventDesc[VCI_EVENT_DESC_STRING_SZ + 1];
} _VCE_AppEventMsg;

typedef struct {
    OSAL_TaskId      taskId;
    uvint            stackSize;
    uvint            taskPriority;
    void            *func_ptr;
    int8             name[16];
    void            *arg_ptr;
} _VCE_TaskObj;

/*
 * Main object for VCE
 */
typedef struct {
    /* VCE main task */
    _VCE_TaskObj     taskMain;

    /* RTP Encoder and Decoder bitrates. */
    uint32           bitrateEnc;
    uint32           bitrateDec;

    /* Video Controller Init flag. */
    vint             vceInit;

    FSM_Context      fsm;    /* The state machine context */
    CODEC            codec;  /* The codec interface */
#ifdef ENABLE_CAMERA_VIEW
    CAMERA           camera; /* The camera interface */
    VIEW             view;   /* The view(display) interface */
#endif

} _VCE_Obj;

#endif


/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12523 $ $Date: 2010-07-13 18:57:44 -0400 (Tue, 13 Jul 2010) $
 *
 */
#include "_vce_private.h"
#include <vci.h>
#include <vier.h>
#include "fsm.h"


#ifdef ENABLE_CAMERA_VIEW
#include "codecSem.h"
#endif
/* 
 * Video Controller structure, private to VCE
 */
_VCE_Obj _VCE_obj;

/*
 * ======== _VCE_selfInit() ========
 *
 * Initializes the internal objects of _VCE_Obj
 *
 */
static vint _VCE_selfInit(
    int w,
    int h)
{
    _VCE_Obj    *vce_ptr;

    vce_ptr = &_VCE_obj;

    vce_ptr->vceInit = 1;
    vce_ptr->codec.enc.data.width = w;
    vce_ptr->codec.enc.data.height = h;

    /* Init the codec interface */
    Codec_init(&(vce_ptr->codec));

    /* Init the FSM */
    FSM_init(&(vce_ptr->fsm), &(vce_ptr->codec));

#ifdef ENABLE_CAMERA_VIEW
    /* Initialize codec's semaphore. */
    if (0 != Codec_SemInit()) {
        return (0);
    }
    /* Initialize camera
     * Parameter: vce_ptr->codec is for getting raw video frame from DEC.
     */
    if(_VCE_display_init(&(vce_ptr->codec)) != 0) {
        return (_VCE_ERROR_INIT);
    }
    /* Initialize camera
     * Parameter: vce_ptr->codec is for sending captured video frame to ENC.
     */
    if(_VCE_camera_init(&(vce_ptr->codec), w, h) != 0) {
        return (_VCE_ERROR_INIT);
    }
#endif
    vce_ptr->fsm.active         = OSAL_TRUE;

    return (_VCE_OK);

    /*
     * Critical error, shutdown
     */
//critical_error:
//    /* VCE Task must stop itself. */
//    VCE_shutdown();
//    return (_VCE_ERROR_INIT);
}

/*
 * ======== _VCE_vtspCommandTask() ========
 *
 * This task is to read commands from VTSP.  It loops forever
 * waiting for commands on the IPC that VTSP wrote.
 * Commands read from the IPC are then sent to an OSAL
 * queue processed by another task.
 *
 * Return Values:
 * None.
 */
static void _VCE_VciEventTask(
    void *arg_ptr)
{
    _VCE_Obj       *vce_ptr;
    VC_Event        event;
    vint            codecType;
    char            eventDesc[VCI_EVENT_DESC_STRING_SZ];

    vce_ptr = (_VCE_Obj *)arg_ptr;

     /*
     * Big D2 Banner with version information.
     */
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

_VCE_VTSP_COMMAND_TASK_LOOP:
    /* Block on event message from VCI. */
    while (_VCE_OK != VCI_getEvent(&event, eventDesc, &codecType, OSAL_WAIT_FOREVER));

    if (OSAL_TRUE == vce_ptr->fsm.active) {
        FSM_processEvent(&(vce_ptr->fsm), event, codecType, eventDesc);
    }

    _VCE_processEvt(vce_ptr, event, codecType, eventDesc);

    /* Loop as long as the VCE is initialized. */
    if (1 == vce_ptr->vceInit) {
        goto _VCE_VTSP_COMMAND_TASK_LOOP;
    }
    else {
        OSAL_logMsg("%s:%d _VCE_vtspCommandTask exited\n", __FILE__, __LINE__);
    }
}

/*
 * ======== VCE_init() ========
 * Initializes the Video Codec Engine Module.
 * This will create the main VCE task.
 *
 * Returns:
 * _VCE_OK             All resources were successfully initialized
 * _VCE_ERROR_INIT     Failed to init one of the resources needed to read commands
 */
vint VCE_init(
    int w,
    int h)
{
    _VCE_Obj     *vce_ptr;
    _VCE_TaskObj *task_ptr;

    /* Initialize the Video Controller private data object. */
    vce_ptr = &_VCE_obj;
    OSAL_memSet(vce_ptr, 0, sizeof(_VCE_obj));
    task_ptr = &vce_ptr->taskMain;

    /*
     * Initialize VIER.
     */
    if (OSAL_FAIL == VIER_init()) {
        OSAL_logMsg("%s:%d Init vier failed.\n", __FUNCTION__, __LINE__);
        return (0);
    }

    DBG("Video Controller starting\n");
    VCI_init();
    DBG("Video Controller started\n");

    /* Initialize internal objects and queues. */
    if (_VCE_ERROR_INIT == _VCE_selfInit(w, h)) {
        return _VCE_ERROR_INIT;
    }

    /* Init the task used to read and process commands from VTSP. */
    task_ptr->taskId = 0;
    task_ptr->stackSize = _VCE_TASK_STACK_SZ;
    task_ptr->taskPriority = _VCE_TASK_MAIN_PRIORITY;
    task_ptr->func_ptr = _VCE_VciEventTask;
    OSAL_snprintf(task_ptr->name, sizeof(task_ptr->name), "%s",
            VCE_VCI_INFC_TASK_NAME);
    task_ptr->arg_ptr = (void *)vce_ptr;

    if (0 == (task_ptr->taskId = OSAL_taskCreate(
        task_ptr->name,
        task_ptr->taskPriority,
        task_ptr->stackSize,
        task_ptr->func_ptr,
        task_ptr->arg_ptr))){
        VCE_shutdown();
        return _VCE_ERROR_INIT;
    }

    OSAL_logMsg("%s:%d Done\n", __FUNCTION__, __LINE__);

    /* Return success. */
    return (_VCE_OK);
}

/*
 * ======== VCE_shutdown() =======
 * Stop task, wait for task to stop.
 * Free all resources if they exist
 */
void VCE_shutdown(
    void)
{
    _VCE_Obj          *vce_ptr;

    _VCE_TRACE(__FUNCTION__, __LINE__);

    vce_ptr = &_VCE_obj;

    /* This will exit cause the _VCE_vtspCommandTask to stop looping. */
    vce_ptr->vceInit = 0;
#ifdef ENABLE_CAMERA_VIEW
    Codec_SemShutdown();

    VIEW_shutdown(&(vce_ptr->view), 0);
    VIEW_shutdown(&(vce_ptr->view), VIDEO_CALLID_CAMERA);
    CAM_shutdown(&(vce_ptr->camera));
#endif
    /* Shutdown VIER */
    VIER_shutdown();

    OSAL_logMsg("%s:%d Exit complete\n", __FUNCTION__, __LINE__);

    return;
}
vint _VCE_display_init(
    CODEC_Ptr  codec_ptr)
{
#ifdef ENABLE_CAMERA_VIEW
    _VCE_Obj    *vce_ptr;
    vce_ptr = &_VCE_obj;
    /*
     * Camera view
     */

    /* To get DEC's decoder obj. */
    vce_ptr->view.decDataObj = &(codec_ptr->dec.data);

    if (0 != VIEW_init(&(vce_ptr->view), VIDEO_CALLID_CAMERA, 0)) {
        VCE_shutdown();
        OSAL_logMsg("%s %d\n", __FILE__, __LINE__);
        return (-1);
    }

    /*
     * Stream 0 view
     */
    if (0 != VIEW_init(&(vce_ptr->view), 0, 0)) {
        VCE_shutdown();
        VIEW_shutdown(&(vce_ptr->view), 0);
        OSAL_logMsg("%s %d\n", __FILE__, __LINE__);
        return (-1);
    }
#endif
    return 0;
}
vint _VCE_camera_init(
    CODEC_Ptr  codec_ptr,
    int        width,
    int        height)
{
#ifdef ENABLE_CAMERA_VIEW
    _VCE_Obj    *vce_ptr;
    vce_ptr = &_VCE_obj;
    /*
     * Camera
     */

    /* To get ENC's encoder obj. */
    vce_ptr->camera.encDataObj = &(codec_ptr->enc.data);

    if (0 != CAM_init(&(vce_ptr->camera), width, height, 0, 0)) {
        VCE_shutdown();
        OSAL_logMsg("%s %d\n", __FILE__, __LINE__);
        return (-1);
    }
#endif
    return 0;
}

/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12523 $ $Date: 2010-07-13 18:57:44 -0400 (Tue, 13 Jul 2010) $
 *
 */
#include "vci.h"
#include "_vc_private.h"

/*
 * ======== VCI_init() ========
 * Call to VC_init.

 * Returns:
 * _VC_OK          All resources were successfully initialized
 * _VC_ERROR_INIT  Failed to init one of the resources needed to read commands
 *          from VTSP.
 */
vint VCI_init(
    void)
{
    return VC_init();
}

/*
 * ======== VCI_shutdown() =======
 * Call to VC_shutdown.
 */
void VCI_shutdown(
    void)
{
    VC_shutdown();
    return;
}

/*
 * ======== VCI_sendFIR() =======
 * Trigger an FIR. This will not send an FIR if currently in the process
 * of requesting or receiving a key frame.
 */
void VCI_sendFIR(
    void)
{
    VC_sendFIR();
    return;
}

/*
 * ======== VCI_getAppEvent() =======
 * Call to VC_getAppEvent.
 */
vint VCI_getEvent(
    VC_Event *event_ptr,
    char     *eventDesc_ptr,
    vint     *codecType_ptr,
    vint      timeout)
{
    return VC_getAppEvent(event_ptr, eventDesc_ptr, codecType_ptr, timeout);
}

/*
 * ======== VCI_sendEncodedFrame() =======
 * Call to VC_sendEncodedFrame.
 */
vint VCI_sendEncodedFrame(
    VC_EncodedFrame *frame_ptr)
{
    return VC_sendEncodedFrame(frame_ptr->data_ptr, frame_ptr->length, frame_ptr->tsMs, frame_ptr->rcsRtpExtnPayload);
}
/*
 * ======== VCI_getEncodedFrame() =======
 * Call to VC_getEncodedFrame.
 */
vint VCI_getEncodedFrame(
    VC_EncodedFrame *frame_ptr)
{
    return VC_getEncodedFrame(&frame_ptr->data_ptr, &frame_ptr->length, &frame_ptr->tsMs, &frame_ptr->flags, &frame_ptr->rcsRtpExtnPayload);
}

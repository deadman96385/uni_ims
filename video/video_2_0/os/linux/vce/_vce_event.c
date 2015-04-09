/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12730 $ $Date: 2010-08-09 20:55:01 -0400 (Mon, 09 Aug 2010) $
 *
 */

/*
 * This process the VCI event.
 */
#include "_vce_private.h"

void _VCE_videoPrintEvent(
    VCE_Event code) 
{
    switch (code) {
        case VCE_EVENT_NONE:
            OSAL_logMsg("event VCE_EVENT_NONE\n"); 
            break;
        case VCE_EVENT_INIT_COMPLETE:
            OSAL_logMsg("event VCE_EVENT_INIT_COMPLETE\n");
            break;
        case VCE_EVENT_START_ENC:
            OSAL_logMsg("event VCE_EVENT_START_ENC\n"); 
            break;
        case VCE_EVENT_START_DEC:
            OSAL_logMsg("event VCE_EVENT_START_DEC\n"); 
            break;
        case VCE_EVENT_STOP_ENC:
            OSAL_logMsg("event VCE_EVENT_STOP_ENC\n");
            break;
        case VCE_EVENT_STOP_DEC:
            OSAL_logMsg("event VCE_EVENT_STOP_DEC\n");
            break;
        case VCE_EVENT_SHUTDOWN:
            OSAL_logMsg("event VCE_EVENT_SHUTDOWN\n"); 
            break;
        case VCE_EVENT_REMOTE_RECV_BW_KBPS:
            OSAL_logMsg("event VCE_EVENT_REMOTE_RECV_BW_KBPS\n");
            break;
        case VCE_EVENT_SEND_KEY_FRAME:
            OSAL_logMsg("event VCE_EVENT_SEND_KEY_FRAME\n");
            break;
        default:
            OSAL_logMsg("event <UNKNOWN> ???\n");
    }
}

/*
 * ======== _VCE_processEvt() ========
 *
 * Process the VCI Event.
 */
void _VCE_processEvt(
    _VCE_Obj       *vce_ptr,
    VCE_Event       event,
    vint            codecType,
    char           *eventDesc_ptr)
{
    _VCE_videoPrintEvent(event);

    switch (event) {
        case VCE_EVENT_INIT_COMPLETE:
            _VCE_TRACE(__FILE__, __LINE__);
            break;
        case VCE_EVENT_START_ENC:
            _VCE_TRACE(__FILE__, __LINE__);
#ifdef ENABLE_CAMERA_VIEW
            /* Set ENC's function pointer, we know the codec type now. */
            vce_ptr->camera.toEncCodecQInputBuffer =
                    vce_ptr->codec.codecEncoder_ptr->codecQueueInputBuffer;
#endif
            break;
        case VCE_EVENT_START_DEC:
            _VCE_TRACE(__FILE__, __LINE__);
#ifdef ENABLE_CAMERA_VIEW
            /* Set DEC's function pointer, we know the codec type now. */
            vce_ptr->view.getDecCodecQOutputBuffer =
                    vce_ptr->codec.codecDecoder_ptr->codecDequeueOutputBuffer;
#endif
            break;
        case VCE_EVENT_STOP_ENC:
            _VCE_TRACE(__FILE__, __LINE__);
            break;
        case VCE_EVENT_STOP_DEC:
            _VCE_TRACE(__FILE__, __LINE__);
            break;
        case VCE_EVENT_SHUTDOWN:
            _VCE_TRACE(__FILE__, __LINE__);
            break;
        case VCE_EVENT_REMOTE_RECV_BW_KBPS:
            _VCE_TRACE(__FILE__, __LINE__);
            break;
        case VCE_EVENT_SEND_KEY_FRAME:
            _VCE_TRACE(__FILE__, __LINE__);
            break;
        case VCE_EVENT_NONE:
        default:
            _VCE_TRACE(__FILE__, __LINE__);
            break;
    }

    OSAL_logMsg("%s:%d END\n", __FUNCTION__, __LINE__);
}


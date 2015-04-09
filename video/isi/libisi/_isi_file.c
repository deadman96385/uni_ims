/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12486 $ $Date: 2010-07-07 17:10:49 -0500 (Wed, 07 Jul 2010) $
 *
 */
#include "_isi_port.h"
#include "isi.h"
#include "isip.h"
#include "_isi_db.h"
#include "_isi_queue.h"
#include "_isi_msg.h"
#include "_isi_file.h"

/* 
 * ======== ISIFT_protoMsg() ========
 * This function is the FSM entry point of commands related to 
 * 'file transfers'.
 *
 * Returns: 
 *  nothing
 */
void ISIFT_protoMsg(
    ISI_Id        fileId,
    ISIP_Message *msg_ptr)
{
    /* We just received a progress report for a file transfer
     * or we have a new file downloading
     */

    ISIP_FileReason  reason;
    ISID_ServiceId   *service_ptr;
    ISI_EventMessage  event;
    ISIP_File        *m_ptr;
    ISID_FileId      *f_ptr;
    ISI_Return        ret;
    
    m_ptr           = &msg_ptr->msg.file;
    reason          = m_ptr->reason;

    
    if (ISID_lockFiles() != ISI_RETURN_OK) {
        OSAL_logMsg("%s:%d silently ignoring due to failure locking database\n", __FUNCTION__, __LINE__);
        return;
    }

    /*
     * ISIP_FILE_REASON_NEW may occur as a message from the protocol when an
     * incoming file transfer request is received.
     * When it occurs as a message from the application, it is a result of
     * initiating a file transfer request but it is not handled by this function
     */
    if (reason == ISIP_FILE_REASON_NEW) {
        /* Then this is a new file download, so create it */
        ret = ISID_serviceGet(m_ptr->serviceId, &service_ptr);
        if (ret == ISI_RETURN_FAILED || ret == ISI_RETURN_SERVICE_NOT_ACTIVE) {
            /* This service ID does not exist or is not active */
            ISID_unlockFiles();
            return;
        }
        /* All is well so create the file entry to store in the database */
        if ((f_ptr = ISID_fileCreate()) == NULL) {
            /* No resource to deliver so just return */
            ISID_unlockFiles();
            return;
        }

        /*
         * the message contains information about the file transfer that
         * we'll need later, so store it in the database so it can be
         * referenced later by the fileId
         */
        f_ptr->service_ptr = service_ptr;
        /* Set whether or not there is a chat associated with this message */

        f_ptr->isiMsg.msg.file.chatId = m_ptr->chatId;

        OSAL_memCpy(&f_ptr->isiMsg.msg.file, m_ptr, sizeof(ISIP_File));
        /* initialize file transfer progress to 0 for new incoming requests */
        f_ptr->isiMsg.msg.file.progress = 0;

        /*
         * must be the recipient of the file transfer since the NEW reason
         * arrived from the lower protocol layer
         */
        f_ptr->state = ISID_FILE_STATE_RECV;

        /* Store the file entry in the database */
        if (ISID_fileAdd(f_ptr, &fileId) != ISI_RETURN_OK) {
            /* exceeded all available resources */
            ISI_free(f_ptr, ISI_OBJECT_FILE_ID);
            ISID_unlockFiles();
            return;
        }
    }
    else {
        /* find the file in the database */
        if (ISID_fileGet(fileId, &f_ptr) != ISI_RETURN_OK) {
            OSAL_logMsg("%s:%d silently ignoring due to failure getting fileId %d in database\n", __FUNCTION__, __LINE__, fileId);
            /* Then this non-zero fileId is invalid */
            ISID_unlockFiles();
            return;
        }
    }

    /* Update the progress */
    f_ptr->isiMsg.msg.file.progress = m_ptr->progress;

    event.serviceId = f_ptr->service_ptr->e.desc;
    event.id = fileId;
    event.idType = ISI_ID_TYPE_FILE;
    event.event = ISI_EVENT_NONE;
    event.eventDesc[0] = 0;
    if (0 != m_ptr->reasonDesc[0]) {
        OSAL_strncpy(event.eventDesc, m_ptr->reasonDesc,
                ISI_EVENT_DESC_STRING_SZ);
    }

    service_ptr = f_ptr->service_ptr;
    
    /* Run the state machine */
    switch (reason) {
        case ISIP_FILE_REASON_NEW:
            event.event = ISI_EVENT_FILE_REQUEST;

            break;
        case ISIP_FILE_REASON_PROGRESS:
            event.event = ISI_EVENT_FILE_PROGRESS;
            break;
        case ISIP_FILE_REASON_COMPLETE:
            event.event = ISI_EVENT_FILE_PROGRESS;
            /*
             * now that the transfer is complete, let's shut things down
             * when this is processed by SAPP, we'll get a response
             * and cleanup the fileId entry in the database
             */
            ISIM_shutdownFileTransfer(f_ptr);
            ISIQ_writeProtocolQueue(&f_ptr->isiMsg);

            break;
        case ISIP_FILE_REASON_ACCEPT:
            /*
             * other end accepted our file transfer request
             * or the file transfer that we've accepted has
             * connected so notify ISI that the request has been accepted
             */
            event.event = ISI_EVENT_FILE_ACCEPTED;

            if (ISID_FILE_STATE_SEND == f_ptr->state) {
                /*
                 * now that the other end has accepted and the MSRP session
                 * is active, let's begin the transfer
                 */
                ISIM_beginSendingFileTransfer(f_ptr);
                ISIQ_writeProtocolQueue(&f_ptr->isiMsg);
            }
            break;
        case ISIP_FILE_REASON_ACKNOWLEDGE:
            event.event = ISI_EVENT_FILE_ACKNOWLEDGED;
            break;
        case ISIP_FILE_REASON_REJECT:
            /*
             * the remote party rejected our file transfer request
             * notify ISI that the request has been rejected
             */
            event.event = ISI_EVENT_FILE_REJECTED;
            ISID_fileDestroy(fileId);

            break;
        case ISIP_FILE_REASON_CANCEL:
            /*
             * we rejected the file transfer request
             * notify ISI that the request has been cancelled
             */
            event.event = ISI_EVENT_FILE_CANCELLED;
            ISID_fileDestroy(fileId);
            break;
        case ISIP_FILE_REASON_ERROR:
            event.event = ISI_EVENT_FILE_FAILED;
            ISID_fileDestroy(fileId);
            break;
        case ISIP_FILE_REASON_SHUTDOWN:
            event.event = ISI_EVENT_FILE_COMPLETED;
            ISID_fileDestroy(fileId);
            break;
        case ISIP_FILE_REASON_TRYING:
            event.event = ISI_EVENT_FILE_TRYING;
            break;
        default:
            /* do nothing */
            break;
    }
    if (event.event != ISI_EVENT_NONE) {
        /* Then send it to the app queue */
        ISIQ_writeAppQueue((char*)&event, sizeof(ISI_EventMessage));
    }
    /* **************************************
     * NOTE: You don't have to free msg_ptr, 
     * because it came from .bss and not heap
     **************************************** 
     */
    
    ISID_unlockFiles();
}

/* 
 * ======== ISIFT_appMsg() ========
 * This function is the FSM entry point of commands related to 'file transfer'.
 * These commands come as a result of an API call.
 *
 * Returns: 
 *  ISI_RETURN_OK       : The command successfully passed through the FSM
 *                        and was sent to the underlying protocol.
 *  ISI_RETURN_FAILED   : The command failed, the command and was NOT sent to
 *                        the underlying protocol.
 */
void ISIFT_appMsg(
    ISID_FileId  *file_ptr,
    ISIP_Message *msg_ptr)
{
    /* This is currently unused */
    (void)file_ptr;
    /* There are no active messages, so send to the protocol */
    ISIQ_writeProtocolQueue(msg_ptr);
    return;
}


/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 27616 $ $Date: 2014-07-18 11:02:53 +0800 (Fri, 18 Jul 2014) $
 *
 */

#include <osal_log.h>
#include <osal_msg.h>
#include <osal_mem.h>
#include <osal_string.h>

#ifdef OSAL_LOG_NDK
#include <android/log.h>
#else
#include <utils/Log.h>
#endif

#ifndef OSAL_LOG_CONSOLE
#define LOG_TAG "OSAL"
#endif

/*
 * ======== OSAL_logToDiag() ========
 * This function is using to send the OSAL log to diagnostic message queue.
 *
 * log_ptr : The message to send.
 *
 * Returns
 * OSAL_SUCCESS : Message send to diagnostic queue is OK.
 * -1 : Message send to diagnostic queue is failed.
 */
OSAL_Status OSAL_logToDiag(
    char *log_ptr)
{
    OSAL_MsgQId     diagId;

    if (0 == (diagId = OSAL_msgQCreate(OSAL_DIAG_EVENT_QUEUE_NAME,
            0, 0, 0,
            OSAL_DIAG_EVENT_QUEUE_DEPTH, sizeof(OSAL_diagMsg), 0))) {
        OSAL_logMsg("OSAL_logToDiag : OSAL_msgQCreate is Failed\n");
        return (-1);
    }

    OSAL_diagMsg diagMsg;

    OSAL_memSet(diagMsg.message, 0, sizeof(diagMsg.message));
    diagMsg.msgType = 10;
    OSAL_strncpy(diagMsg.message, log_ptr, sizeof(diagMsg.message));
    if (OSAL_SUCCESS != OSAL_msgQSend(diagId, &diagMsg, sizeof(OSAL_diagMsg),
            OSAL_WAIT_FOREVER, NULL)) {
        OSAL_logMsg("OSAL_logToDiag : OSAL_msgQSend is Failed\n");
        OSAL_msgQDelete(diagId);
        return (-1);
    }
    OSAL_msgQDelete(diagId);

    return(OSAL_SUCCESS);
}

/*
 * ======== OSAL_logMsg() ========
 *
 * Used for logging.
 *
 * Returns
 *  OSAL_SUCCESS
 */
OSAL_Status OSAL_logMsg(
    const char *format_ptr,
    ...)
{
    va_list args;

    va_start(args,
            format_ptr);

#ifdef OSAL_LOG_CONSOLE 
    vprintf(format_ptr,
            args);
#else
    {
        static char _OSAL_logBuffer[1024];
        _OSAL_logBuffer[0] = 0;
        vsnprintf(_OSAL_logBuffer, sizeof(_OSAL_logBuffer),
                format_ptr, args);
#ifdef OSAL_LOG_NDK
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "%s", 
                _OSAL_logBuffer);
#else
        LOGE(_OSAL_logBuffer);
#endif
#ifdef OSAL_DIAGNOSTIC_LOG
        /* Send the message to diag queue. */
        OSAL_logToDiag(_OSAL_logBuffer);
#endif
    }
#endif

    va_end(args);

    return(OSAL_SUCCESS);
}

/*
 * ======== OSAL_logString() ========
 *
 * Used for logging.  Will NOT attempt to 'construct' a debug log string
 * using a static buffer and printf'esque type routines like OSAL_logMsg().
 * It will simply print the NULL terminated string provided in 'string_ptr'.
 * Any limitations regarding the length of the string printed will be enforced
 * by the underlying OS and not OSAL.
 *
 * Returns
 *  OSAL_SUCCESS
 */
OSAL_Status OSAL_logString(
    char *string_ptr)
{
#ifdef OSAL_LOG_CONSOLE
    printf("%s", string_ptr);
#else
    {
#ifdef OSAL_LOG_NDK
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "%s", string_ptr);
#else
        static char _OSAL_logBuffer[1024];
        int len = strlen(string_ptr);
        int offset = 0;
        while (offset < len) {
            snprintf(_OSAL_logBuffer, 1024, "%s", string_ptr + offset);
            LOGE(_OSAL_logBuffer);
            offset += 1023;
        }
#endif
#ifdef OSAL_DIAGNOSTIC_LOG
        /* Send the message to diag queue. */
        static char _OSAL_diagBuffer[1024];
        OSAL_memSet(_OSAL_diagBuffer, 0, 1024);
        OSAL_strncpy(_OSAL_diagBuffer, string_ptr, 1024);
        char *split_ptr;
        split_ptr = strtok(_OSAL_diagBuffer, "\n");
        while (split_ptr != NULL) {
            OSAL_logToDiag(split_ptr);
            split_ptr = strtok(NULL, "\n");
        }
#endif
    }
#endif
    return(OSAL_SUCCESS);
}


/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 17840 $ $Date: 2012-08-03 01:06:57 +0800 (Fri, 03 Aug 2012) $
 *
 */
#ifndef __ISI_DBG_H__
#define __ISI_DBG_H__

#define ISI_DEBUG_MAX_STRING_SIZE (128)

#ifndef ISI_DEBUG_LOG

#define ISIG_log(arg, arg1, arg2, arg3, arg4)

#define ISIG_logCall(arg, arg1, arg2, arg3)

#define ISIG_logSystem(arg)

#define ISIG_logService(arg, arg1)

#define ISIG_logAudio(arg)

#define ISIG_logCoder(arg, arg1)
    
#define ISIG_logStream(arg)

#define ISIG_logChat(arg, arg1, arg2, arg3)

#define ISIG_logDiag(arg)

#else
void ISIG_log(
    const char *funct_ptr,
    char       *str_ptr,
    uint32      arg1,
    uint32      arg2,
    uint32      arg3);

void ISIG_logCall(
    uint32  callId,
    vint    state, 
    vint    reason, 
    uint32  serviceId);

void ISIG_logSystem(
    vint    reason);

void ISIG_logService(
    uint32  serviceId,
    vint    reason);

void ISIG_logAudio(
    vint    reason);

void ISIG_logCoder(
    ISIP_Coder coders[],
    vint       numEntries);

void ISIG_logStream(ISIP_Stream *s_ptr);

void ISIG_logChat(
    uint32  chatId,
    uint32  state,
    uint32  reason,
    uint32  serviceId);

void ISIG_logDiag(
    vint    reason);

#endif

#endif


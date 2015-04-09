/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 25157 $ $Date: 2014-03-17 00:59:26 +0800 (Mon, 17 Mar 2014) $
 */

#ifndef _IUT_PRT_H_
#define _IUT_PRT_H_

#define IUT_PRT_OUTPUT_BUFFER_MAX_SIZE (4095)

char* IUT_prtReturnString(
    ISI_Return r);

void IUT_prtEvent(
    ISI_Id     serviceId,
    ISI_Id     id,
    ISI_IdType idType,
    ISI_Event  event,
    char      *eventDesc_ptr);

void IUT_prtTelEvt(
    ISI_TelEvent event,
    char        *from_ptr,
    char        *dateTime_ptr,
    uint32       arg0,
    uint32       arg1);

void IUT_prtRoster(
    char *contact_ptr,
    char *group_ptr,
    char *name_ptr,
    char *substate_ptr);

void IUT_prtRosterList(
    IUT_Roster aRoster[]);

void IUT_prtIm(
    char  *from_ptr,
    ISI_Id chatId,
    char  *subject_ptr,
    char  *message_ptr,
    char  *dateTime_ptr,
    vint   reports,
    char  *reportId_ptr);

void IUT_prtImReport(
    char  *from_ptr,
    ISI_Id chatId,
    char  *dateTime_ptr,
    int    report,
    char  *reportId_ptr);

void IUT_prtImComposing(
    ISI_Id chatId,
    vint   event);

void IUT_prtFile(
    char            *from_ptr,
    ISI_Id           chatId,
    char            *filePath_ptr,
    ISI_FileType     fileType,
    vint             progress,
    char            *subject_ptr);

void IUT_prtFileProgress(
    char               *from_ptr,
    ISI_Id              chatId,
    char               *filePath_ptr,
    ISI_FileType        fileType,
    ISI_FileAttribute   fileAttr,
    vint                fileSize,
    vint                progress,
    char               *subject_ptr);

void IUT_prtCallerId(
    char *from_ptr,
    char *subject_ptr);

void IUT_prtChatPresence(
    ISI_Id chatId,
    char  *roomName_ptr,
    char  *from_ptr,
    char  *subject_ptr,
    char  *presence_ptr);

void IUT_prtGroupChat(
    ISI_Id  chatId,
    char   *subject_ptr,
    char   *from_ptr);

void IUT_prtCallModify(
    IUT_HandSetObj *hs_ptr,
    vint            id);

void IUT_prtFeature(
    vint features);

void IUT_prtCallModifyResult(
    IUT_HandSetObj *hs_ptr,
    ISI_Event       event, 
    vint            id);
#endif

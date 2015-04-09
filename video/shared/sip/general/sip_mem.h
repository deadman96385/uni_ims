/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 24709 $ $Date: 2014-02-20 17:27:12 +0800 (Thu, 20 Feb 2014) $
 */


#ifndef _SIP_MEM_H_
#define _SIP_MEM_H_

void *SIP_malloc(
    int size);

void SIP_free(
    void *pMem);

tSipIntMsg *SIP_allocMsg(
    void);

void SIP_freeMsg(
    tSipIntMsg *pMsg);

tSipIntMsg* SIP_copyMsg(
    tSipIntMsg *pMsg);

char *SIP_allocString(
    uint32 size);

vint SIP_copyString(
    char *pSrc,
    char **ppDest);

vint SIP_copySipText(
    tSipText  *pSrc,
    tSipText **ppDest);

vint SIP_copySipMsgBody(
    tSipMsgBody *pSrc,
    tSipMsgBody **ppDest);


vint SIP_copyStringToSipText(
    char     *pSrc,
    tSipText **ppDest);
#endif

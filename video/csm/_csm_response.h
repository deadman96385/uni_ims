/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30168 $ $Date: 2014-12-02 16:40:06 +0800 (Tue, 02 Dec 2014) $
 *
 */

#ifndef _CSM_OUTPUT_H_
#define _CSM_OUTPUT_H_

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * CSM Response API public methods 
 */
void CSM_sendOutputEvent(
    CSM_OutputEvent *response_ptr);

void CSM_sendError(
    int              errorCode,
    char            *description_ptr,
    CSM_OutputEvent *csmOutput_ptr);

void CSM_sendOk(
    char            *description_ptr,
    CSM_OutputEvent *csmOutput_ptr);

void CSM_sendRemoteDisconnect(
    char            *description_ptr,
    CSM_OutputEvent *csmOutput_ptr);

void CSM_sendSmsError(
    int              errorCode,
    char            *description_ptr,
    CSM_OutputEvent *csmOutput_ptr);

void CSM_sendAkaChallenge(
    char            *rand_ptr,
    char            *autn_ptr,
    CSM_OutputEvent *csmOutput_ptr);

void CSM_sendIpsecInfo(
    int   status,
    int   portUc,
    int   portUs,
    int   portPc,
    int   portPs,
    int   spiUc,
    int   spiUs,
    int   spiPc,
    int   spiPs);

void CSM_sendSupSrvError(
    int              errorCode,
    char            *description_ptr,
    CSM_OutputEvent *csmOutput_ptr);

void CSM_sendSrvccResult(
    int              result,
    int              callId,
    char            *description_ptr,
    CSM_OutputEvent *csmOutput_ptr);

void CSM_sendIpsecEvent(
    int              reason,
    int              portUc,
    int              portUs,
    int              portPc,
    int              portPs,
    int              spiUc,
    int              spiUs,
    int              spiPc,
    int              spiPs,
    CSM_OutputEvent *csmOutput_ptr);
    
void CSM_sendUssdError(
    int   errorCode,
    char *description_ptr,
    CSM_OutputEvent *csmOutput_ptr);

void CSM_sendServiceError(
    int   errorCode,
    char *description_ptr,
    CSM_OutputEvent *csmOutput_ptr);

void CSM_sendEarlyMedia(
    CSM_OutputEvent *csmOutput_ptr);

#ifdef __cplusplus
}
#endif
#endif /* _CSM_OUTPUT_H_ */

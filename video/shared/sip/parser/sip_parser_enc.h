/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 24709 $ $Date: 2014-02-20 17:27:12 +0800 (Thu, 20 Feb 2014) $
 */

#ifndef _SIP_PARSER_ENC_H__
#define _SIP_PARSER_ENC_H__

vint ENC_Msg(
    tSipIntMsg *pMsg,
    tL4Packet  *pBuff,
    vint        useCompactForm);

vint ENC_Uri(
    tUri    *pUri,
    char    *pTarget,
    uint32  *pLength,
    vint     useEscChars);

vint ENC_ReferTo(
    tReferToHF *pReferTo,
    char       *pTarget,
    uint32     *pLength);

vint ENC_Event(
    tEventHF   *pEvent,
    char       *pTarget,
    uint32     *pLength);

vint ENC_Route(
    tDLList    *pRouteList,
    char       *pTarget,
    uint32     *pLength);

#endif

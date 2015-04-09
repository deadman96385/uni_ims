/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 24709 $ $Date: 2014-02-20 17:27:12 +0800 (Thu, 20 Feb 2014) $
 */

#ifndef _SIP_PARSER_DEC_H_
#define _SIP_PARSER_DEC_H_

#define MAX_URI_ARGS (2)
typedef struct sDecodeArgs
{
    char  *s;
    uint16 l;
}tDecodeArgs;

void DEC_Init(void);

vint DEC_Msg(
    tL4Packet  *pBuff, 
    tSipIntMsg *pMsg, 
    vint       *pIsUsingCompact);

vint DEC_Uri(
    char   *str, 
    uint16  size, 
    tUri   *pUri);

vint DEC_UriNoScheme(
    char   *str, 
    uint16  size, 
    tUri   *pUri);

vint DEC_ReferTo2Replaces(
    char        *str, 
    uint16       size,
    tReplacesHF *pReplaces);

vint DEC_HeaderFields(
    tSipIntMsg *pMsg, 
    char       *pHdrFld);

#endif

/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 24709 $ $Date: 2014-02-20 17:27:12 +0800 (Thu, 20 Feb 2014) $
 */

#ifndef _SIP_TOKEN_H_
#define _SIP_TOKEN_H_

#define SIP_CRLF "\r\n"
#define MAX_TOKEN_LENGTH (0xffff)


typedef struct sFSM tFSM;
typedef struct sL4Packet tL4Packet;

typedef  int (*tpfTokenHndlr)  (tFSM*, tL4Packet*, tSipHandle);
typedef  int (*tpfGetToken)    (tFSM*, tL4Packet*, const char*);

typedef struct sTokenizer
{
   const char   *pExt;      /* external id of the attribute */
   uint32        Int;       /* internal id of the attribute */
   tpfTokenHndlr pfHandler;
} tTokenizer;

/* this is the OSI layer 4 packet data */
struct sL4Packet
{
   tSipHandle  frame;
   char       *pStart;
   char       *pCurr;
   uint16      length;
   vint        isOutOfRoom;
};

typedef struct sToken
{
   char     *pStart;     /* points to start of token */
   uint32    length;     /* token's length (except delimiter) */
   char     *pDmtr;      /* points to start of delimiter */
} tToken;

struct sFSM
{
   tpfTokenHndlr     pfHandler;
   tpfGetToken       pfGetToken;

   tToken            CurrToken;
   tSipHandle        hCurrBlock;
   int               Status;
   uint16            ErrorCode;
   uint32            CurrLine;
   vint              isEndOfPacket;
};

/* macros */
#define EXTBUF_LEN(pB) ((int)((pB)->pCurr - (pB)->pStart))

#define CALC_MIN(x,y) ((x < y)?x:y)

#define EXTBUF_END(pB)            ( (char *)((pB)->pStart + (pB)->length) )


#define RETURN_ERROR(pFSM,Err)   {                               \
                                    if ( (pFSM)->Status == SIP_OK )   \
                                       (pFSM)->Status = Err;     \
                                    return SIP_FAILED;             \
                                 }

#define SET_ERROR(pFSM,Err)   {                               \
                                    if ( (pFSM)->Status == SIP_OK )   \
                                       (pFSM)->Status = Err;     \
                                 }

#define SIP_MSG_CODE(pMsg,Err)   \
{                                \
    (pMsg)->internalErrorCode = Err;          \
}

#ifndef UNUSED
#define UNUSED(_param) ((void)(_param))
#endif

/* prototypes */

vint TOKEN_Get(
    tFSM       *pFSM, 
    tL4Packet  *pB, 
    const char *EscString);

vint TOKEN_GetBlock(
    tFSM       *pFSM,
    tL4Packet  *pB,
    const char *EscSeqeunce);

vint TOKEN_Put(
    const char *String,
    const char *EscString,
    tL4Packet  *pBuff);

vint TOKEN_PutSpecChar(
    const char *String,
    const char *EscString,
    tL4Packet  *pBuff);
    
vint TOKEN_iCmpToken(
    tToken     *pToken, 
    const char *Str);

vint TOKEN_ExtLookup(
    tToken           *pToken, 
    const tTokenizer *Table, 
    uint32            Size, 
    uint32           *pIndex);

vint TOKEN_IntLookup(
     uint32           Int, 
    const tTokenizer *Table, 
    uint32            Size, 
    uint32           *pIndex);

vint TOKEN_SkipUntilEsc(
    tFSM       *pFSM, 
    tL4Packet  *pPacket, 
    const char *pEscString);

vint TOKEN_SkipWhileEsc(
    tFSM       *pFSM, 
    tL4Packet  *pPacket, 
    const char *pEscString);

void TOKEN_SkipToEnd(tL4Packet *pPacket);

vint TOKEN_bCheckPgbk(tL4Packet *pPacket);

vint TOKEN_bCheckWsTail(char *pStr);

void TOKEN_SkipEmptyLines(
    tL4Packet *pPacket, 
    vint      *pIsEndOfMsg);

void TOKEN_SkipWS(tL4Packet *pPacket);

vint TOKEN_copyToBuffer(
   char        *target_ptr,
   vint         maxTargetSize,
   tToken      *token_ptr);

#endif  

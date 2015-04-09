/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28728 $ $Date: 2014-09-05 17:01:31 +0800 (Fri, 05 Sep 2014) $
 */

#include "sip_types.h"
#include "sip_sip_const.h"
#include "sip_abnfcore.h"
#include "sip_clib.h"
#include "sip_debug.h"
#include "sip_token.h"


/* 
 ******************************************************************************
 * ================TOKEN_Get()===================
 *
 * This function is used to find a token in a SIP message.  
 * The pFSM is a pointer to an instance of an object used to keep the state of
 * the search for the token.  pBuff should be a pointer to a buffer containing 
 * the SIP message (The payload of the SIP TCP or UDP packet).  pToken is the 
 * token or string to search for.
 *
 * pFSM    = A pointer to a FSM (Finite State Machine) object.
 *
 * pB = A Pointer to a buffer containing a SIP message.
 *
 * pToken = A pointer to a string containing the string of characters 
 *          to search for.
 *         
 * RETURNS: 
 *         SIP_FAILED: The end of the buffer was reached and there is no token
 *                             OR
 *                     The MAX_TOKEN length was reached.
 *         SIP_OK: A token exists, and if the end of the buffer was reached
 *                 then 'pFSM->isEndOfPacket' will be set to TRUE  
 *                 
 ******************************************************************************
 */
vint TOKEN_Get(
    tFSM       *pFSM, 
    tL4Packet  *pB, 
    const char *EscString)
{
    tToken   *pT = &pFSM->CurrToken;
    char         *bufEnd  = EXTBUF_END(pB);
    uint16        holeLen = 0, eolLen;
    register char currc;

    SIP_DebugLog(SIP_DB_GENERAL_LVL_3, "GetToken", 0, 0, 0);
    
    OSAL_memSet(pT, 0, sizeof(tToken));

    for (; pB->pCurr < bufEnd ; pB->pCurr++) {
        if ( !ABNF_ISWSP(*pB->pCurr) )
            break;
    }
   
    if (pB->pCurr >= bufEnd) {
        pFSM->isEndOfPacket = TRUE;
        return SIP_FAILED;
    }
   
    pT->pStart = pB->pCurr;

    while (pB->pCurr < bufEnd) { 
        uint16 inc = 1;

        currc = *pB->pCurr;

        /* if we are on the next line */
        if (0 != (eolLen = (uint16)ABNF_ISEOL(pB->pCurr))) {
           /* if we've got the CRLF then inc = 2; 
            * increment the lines counter 
            */
            inc = eolLen;
            pFSM->CurrLine++;         
        }

        /* if the character is a delimiter then break the loop */
        if (OSAL_strchr(EscString, currc)) {
            pT->pDmtr  = pB->pCurr;
            pB->pCurr += inc;
            break;
        }

        /* if the character is not a delimiter but whitespace */
        if (ABNF_ISWSP(currc))
            holeLen++;
        else {
           /* increase token length by accumulated number of 
            * whitespaces and by inc 
            */
            pT->length += holeLen + inc;
            holeLen     = 0;

            if ( pT->length > MAX_TOKEN_LENGTH )
                return SIP_FAILED;
        }
      
        pB->pCurr += inc; 
    }

    if ((pB->pCurr >= bufEnd) || TOKEN_bCheckPgbk(pB))
        pFSM->isEndOfPacket = TRUE;

    return SIP_OK;
}

vint TOKEN_GetBlock(
    tFSM       *pFSM,
    tL4Packet  *pB,
    const char *EscSeqeunce)
{
    tToken   *pT = &pFSM->CurrToken;
    char         *bufEnd  = EXTBUF_END(pB);
    char         *end;

    SIP_DebugLog(SIP_DB_GENERAL_LVL_3, "TOKEN_GetBlock", 0, 0, 0);
    OSAL_memSet(pT, 0, sizeof(tToken));

    if (pB->pCurr >= bufEnd) {
        pFSM->isEndOfPacket = TRUE;
        return SIP_FAILED;
    }

    end = OSAL_strnscan(pB->pCurr, bufEnd - pB->pCurr, EscSeqeunce);
    if (NULL == end) {
        return (SIP_FAILED);
    }

    pT->pStart = pB->pCurr;
    pT->pDmtr  = end;
    pT->length = end - pB->pCurr;

    pB->pCurr  = end +  OSAL_strlen(EscSeqeunce);

    if ((pB->pCurr >= bufEnd) || TOKEN_bCheckPgbk(pB))
            pFSM->isEndOfPacket = TRUE;

    return (SIP_OK);
}

/* 
 ******************************************************************************
 * ================TOKEN_Put()===================
 *
 * This function will place pString into the target buffer (pBuff), and then 
 * it will write pEscSeq.  If the maximum length of pBuff has been reached 
 * before it can write pString and pEscSeq then SIP_NO_ROOM will be returned.
 * If successful, SIP_OK is returned
 *
 * pString = A pointer to a string to place in pBuff.
 *
 * pEscSeq = A pointer to a string that will be placed after pString 
 *           (the escape sequence).
 *
 * pBuff = A pointer to a tL4Packet, or the target buffer to place 
 *         pString and pEscSeq.
 *
 *         
 * RETURNS: 
 *         SIP_OK:  The pString and pEscSeq were successfully placed in pBuff
 *    SIP_NO_ROOM:  The end of the pBuff was reached and no string were written
 *                 
 ******************************************************************************
 */
vint TOKEN_Put(
    const char *String,
    const char *EscString,
    tL4Packet  *pBuff)
{  
   char   *bufEnd = EXTBUF_END(pBuff);
   uint32  i;
   
   SIP_DebugLog(SIP_DB_GENERAL_LVL_3, "Putting Token", 0, 0, 0);
   
   /* check if buffer length are not expired */
   if ((pBuff->pCurr + OSAL_strlen(String) + OSAL_strlen(EscString)) >= bufEnd) {
      pBuff->isOutOfRoom = TRUE;
      return SIP_NO_ROOM;
   }

   /* add token to the output buffer */
   for ( i = 0; String[i] != '\0'; i++ )
      *pBuff->pCurr++ = String[i];

   /* add token's delimiter to the output buffer */
   for ( i = 0; EscString[i] != '\0'; i++ )
      *pBuff->pCurr++ = EscString[i];

   return SIP_OK;
}

/* 
 ******************************************************************************
 * ================TOKEN_PutSpecChar()===================
 *
 * This function will place a String into the target buffer (pBuff), and then 
 * it will write EscString.  If the String contains "special chars" i.e. 
 * spaces, at signs, equals etc...) then they are replaced with their 
 * hexidecimal equivalant.  For example a 'space' is replaced with '%20' 
 *
 * String = A pointer to a string to place in pBuff.
 *
 * EscString = A pointer to a string that will be placed after String 
 *           (the escape sequence).
 *
 * pBuff = A pointer to a tL4Packet, or the target buffer to place 
 *         String and EscString.
 *
 *         
 * RETURNS: 
 *         SIP_OK:  The String and Escstring were successfully placed in pBuff
 *    SIP_NO_ROOM:  The end of the pBuff was reached and no string was written
 *                 
 ******************************************************************************
 */
vint TOKEN_PutSpecChar(
    const char *String,
    const char *EscString,
    tL4Packet  *pBuff)
{  
    char   *bufEnd = EXTBUF_END(pBuff);
    uint32  i;
    char    a;
    char   *pStr;
    vint    x;
    vint    maxTargetLen;

    SIP_DebugLog(SIP_DB_GENERAL_LVL_3, "Putting Token w-special char", 0, 0, 0);
   
    /* We must account for worst case, 
     * which would be with special chars
     */
    maxTargetLen = (bufEnd - pBuff->pCurr) - (OSAL_strlen(EscString) + 3);

    if (maxTargetLen <= 0) {
        /* then there is not enough room */
        pBuff->isOutOfRoom = TRUE;
        return SIP_NO_ROOM;
    }
   
    x = 0;
    i = 0;
    while (String[i] != '\0' && x < maxTargetLen) {
        a = String[i];
        switch (a) {
        case '@':
            pStr = "%40";
            break;
        case ';':
            pStr = "%3B";
            break;
        case ' ':
            pStr = "%20";
            break;
        case '=':
            /*
             * Commented this out for YTL specs.
             * They do not understand an equal sign using special chars.
             */
            /* pStr = "%3D"; */
            pStr = NULL;
            break;
        case '#':
            pStr = "%23";
            break;
        default: 
            pStr = NULL;
            break;
        }

        if (pStr) {
            OSAL_memCpy(pBuff->pCurr, pStr, 3);
            pBuff->pCurr += 3;
            x += 3;
        }
        else {
            *pBuff->pCurr++ = a;
            x++;
        }
        i++;
    }

    /* add token's delimiter to the output buffer */
    for ( i = 0; EscString[i] != '\0'; i++ )
        *pBuff->pCurr++ = EscString[i];

    return SIP_OK;
}

/* 
 ******************************************************************************
 * ================TOKEN_iCmpToken()===================
 *
 * This function is used to compare a token to a string.  The comparison is 
 * CASE INSENSITIVE.  If the token and string are the same, then TRUE is 
 * retuned, otherwise FALSE is returned.
 *
 * pToken = A pointer to a token object.
 * pString = A string to compare the token to.
 *
 *         
 * TRUE:  The string is the same as the token.
 * FALSE: The token and the string are NOT the same.
 *                 
 ******************************************************************************
 */
vint TOKEN_iCmpToken(
    tToken     *pToken, 
    const char *Str)
{
   uint32 i;

   SIP_DebugLog(SIP_DB_GENERAL_LVL_3, "Comparing Token", 0, 0, 0);
   
   if (!pToken || !Str)
      return SIP_BADPARM;

   for ( i = 0; (i < pToken->length)&&(Str[i] != '\0'); i++ ) {
      if ( pToken->pStart[i] != Str[i] ) {
         if ( LOW2UP_CASE(pToken->pStart[i]) != LOW2UP_CASE(Str[i]) )
            return FALSE;
      }
   }
   if ( ((i >= pToken->length)&&(Str[i] != '\0')) || ((i != pToken->length)&&(Str[i] == '\0')) )
      return FALSE;

   return TRUE;
}

/* 
 ******************************************************************************
 * ================TOKEN_ExtLookup()===================
 *
 * This function will search a table of tTokenizer objects pointer to by Table 
 * for the token pointed to by pToken.  The function will search every element 
 * in the table until the number of elements searched reaches Size.  
 * If successful, SIP_OK will be returned and the value in pIndex will be the 
 * index of the entry in the table that contains the token.  
 * If SIP_FAILED s retuned, that means that the table was searched up to Size 
 * and the token was not found.
 *
 * pToken = A pointer to a token object
 *
 * Table = A pointer to a table containing tTokenizer objects
 *
 * Size = The number of entries in the table pointed by pTable
 *
 * pIndex = A pointer to an unsigned integer
 *         
 * SIP_OK: Successfull and the value in pIndex will be the index of the entry 
 *         in the table that contains the token.  
 * SIP_FAILED: This means that the table was searched up to Size 
 *             and the token was not found.
 *                 
 ******************************************************************************
 */
vint TOKEN_ExtLookup(
    tToken           *pToken, 
    const tTokenizer *Table, 
    uint32            Size, 
    uint32           *pIndex)
{
   uint32 i;

   SIP_DebugLog(SIP_DB_GENERAL_LVL_3, "Quering External Look up Table",
           0, 0, 0);
   
   if ( !Table || !Size ||!pIndex )
      return SIP_BADPARM;
   
   for ( i = 0; i < Size; i++ ) {
      if ( TOKEN_iCmpToken(pToken, Table[i].pExt) ) {
         *pIndex = i;
         break;
      }
   }
   if ( i >= Size )
      return SIP_NOT_FOUND;

   return SIP_OK;
}

/* 
 ******************************************************************************
 * ================TOKEN_IntLookup()===================
 *
 * This function will search a table of tTokenizer objects looking for an 
 * internal enumerated value specified by Int.  The table will be searched up 
 * to Size number of entries.  If successful, a value of SIP_OK will be 
 * returned and pIndex will contain the index in the table that the entry can 
 * be found.  If the table doesn't contain the internal enumerated value then 
 * SIP_FAILED is returned
 *
 * Int    An enumerated value representing the internal value of a token (string)
 *
 * Table    A pointer to a table containing tTokenizer objects
 *
 * Size    The number of entries in the table pointed by Table
 *
 * pIndex    A pointer to an unsigned integer
 *
 * RETURNS:
 *        SIP_OK: The internal value was found in the table
 *    SIP_FAILED: Could not find the internal value in the table
 *
 ******************************************************************************
 */
vint TOKEN_IntLookup(
     uint32            Int, 
     const tTokenizer *Table, 
     uint32            Size, 
    uint32           *pIndex)
{
   uint32 i;
   
   SIP_DebugLog(SIP_DB_GENERAL_LVL_3, "Quering Internal Look up Table",
           0, 0, 0);
   
   if ( !Table || !Size ||!pIndex )
      return SIP_BADPARM;
   
   for ( i = 0; i < Size; i++ )
   {
      if ( Table[i].Int == Int )
      {
         *pIndex = i;
         break;
      }
   }
   if ( i >= Size )
      return SIP_NOT_FOUND;

   return SIP_OK;
}

/* 
 ******************************************************************************
 * ================TOKEN_SkipUntilEsc()===================
 *
 * This function will advance the FSM (Finite State Machine) and the SIP packet 
 * forward until the Escape Sequence pointed to by pEscString is reached.  If 
 * the FSM was successfully advanced to the escape sequence, a value of SIP_OK 
 * is returned.  If the escape sequence could not be found in the SIP message 
 * pointed to by pEscString then a value of SIP_FAILED
 *
 * pFSM = A Pointer to a FSM object (Finite State Machine)
 *
 * pPacket = A pointer to a tL4Packet (OSI Level 4 packet)
 * 
 * pEscString = A pointer to a string containing an escape sequence
 *
 *  RETURNS:
 *      SIP_OK: The Finite State Machine was successfully advanced
 *  SIP_FAILED: The escape sequence could not be found
 *
 ******************************************************************************
 */
vint TOKEN_SkipUntilEsc(
    tFSM       *pFSM, 
    tL4Packet  *pPacket, 
        const char *pEscString)
{
   char *bufEnd = EXTBUF_END(pPacket); 

   SIP_DebugLog(SIP_DB_GENERAL_LVL_3, "Skiping until chars", 0, 0, 0);
   
   /* do for each character in the buffer */
   for ( ; pPacket->pCurr < bufEnd; pPacket->pCurr++ )
   {
      /* increment the lines counter */
      if ( ABNF_ISEOL(pPacket->pCurr) )
         pFSM->CurrLine++;

      /* if the character is an escCharacter then */
      if ( OSAL_strchr(pEscString, *pPacket->pCurr) )
      {
         pPacket->pCurr += ABNF_ISCRLF(pPacket->pCurr)? 2: 1;         
         break;
      }      
   }
   if ( pPacket->pCurr >= bufEnd )
      return SIP_FAILED;

   return SIP_OK;
}

/* 
 ******************************************************************************
 * ================TOKEN_SkipWhileEsc()===================
 *
 * This function will advance the FSM (Finite State Machine) and the SIP packet 
 * forward will the Escape Sequence pointed to by pEscString is observed.  If 
 * the FSM was successfully advanced over the escape sequence, a value of SIP_OK 
 * is returned.  If the end of the packet was reached then a value of SIP_FAILED
 * is returned.
 *
 * pFSM = A Pointer to a FSM object (Finite State Machine)
 *
 * pPacket = A pointer to a tL4Packet (OSI Level 4 packet)
 * 
 * pEscString = A pointer to a string containing an escape sequence
 *
 *  RETURNS:
 *      SIP_OK: The FSM was successfully advanced over the escape sequence
 *      SIP_FAILED: The end of the packet was reached.
 *
 ******************************************************************************
 */
vint TOKEN_SkipWhileEsc(
    tFSM       *pFSM, 
    tL4Packet  *pPacket, 
        const char *pEscString)
{
   char *bufEnd = EXTBUF_END(pPacket);

   SIP_DebugLog(SIP_DB_GENERAL_LVL_3, "Skipping until escape sequence", 0, 0, 0);
   
   /* do for each character in the buffer */
   for ( ; pPacket->pCurr < bufEnd; pPacket->pCurr++ )
   {
      /* increment the lines counter */
      if ( ABNF_ISEOL(pPacket->pCurr) )
         pFSM->CurrLine++;

      /* if the character is not an escCharacter then */
      if ( !OSAL_strchr(pEscString, *pPacket->pCurr) )
         break;
      
      if ( ABNF_ISCRLF(pPacket->pCurr) )
         pPacket->pCurr++;      
   }
   if ( pPacket->pCurr >= bufEnd )
      return SIP_FAILED;

   return SIP_OK;
}

/* 
 ******************************************************************************
 * ================TOKEN_SkipToEnd()===================
 *
 * This function will advance the packet to the end.
 *
 * pPacket = A pointer to a tL4Packet (OSI Level 4 packet)
 * 
 *  RETURNS:
 *        Nothing
 *
 ******************************************************************************
 */
void TOKEN_SkipToEnd(tL4Packet *pPacket)
{
   char *bufEnd = EXTBUF_END(pPacket);

   SIP_DebugLog(SIP_DB_GENERAL_LVL_3, "Skipping until the end", 0, 0, 0);
   
   for ( ; pPacket->pCurr < bufEnd; pPacket->pCurr++ )
   {
      if ( TOKEN_bCheckPgbk(pPacket) )
         break;
   }
}

/* 
 ******************************************************************************
 * ================TOKEN_bCheckPgbk()===================
 *
 *
 * pPacket = A pointer to a tL4Packet (OSI Level 4 packet)
 * 
 *  RETURNS:
 *        TRUE: 
 *        FALSE:
 ******************************************************************************
 */
vint TOKEN_bCheckPgbk(tL4Packet *pPacket)
{
   char   *bufEnd = EXTBUF_END(pPacket);
   uint32  eolLen;

   SIP_DebugLog(SIP_DB_GENERAL_LVL_3, "TOKEN_bCheckPgbk", 0, 0, 0);

   if ( !ABNF_ISPGBCK(pPacket->pCurr) )
      return FALSE;

   pPacket->pCurr++;

   /* to skip following empty lines if any */
   for ( ; pPacket->pCurr < bufEnd; pPacket->pCurr += eolLen )
   {
      if (0 == (eolLen = ABNF_ISEOL(pPacket->pCurr)) )
         break;
   }

   return TRUE;
}

/* 
 ******************************************************************************
 * ================TOKEN_bCheckWsTail()===================
 * This function checks if there as any white space before an EOL char set.
 *
 * pStr = A pointer to the string in question
 * 
 *  RETURNS:
 *        TRUE: There was white space before an EOL
 *        FALSE: Nop there wasn't
 ******************************************************************************
 */
vint TOKEN_bCheckWsTail(char *pStr)
{
   uint32 i = 0;

   SIP_DebugLog(SIP_DB_GENERAL_LVL_3, "TOKEN_bCheckWsTail", 0, 0, 0);

   while (ABNF_ISWSP(pStr[i]))
      i++;
   
   if (!ABNF_ISEOL((pStr + i)))
      return FALSE;

   return TRUE;
}

/* 
 ******************************************************************************
 * ================TOKEN_SkipEmptyLines()===================
 * 
 * This function advances over empty lines
 *
 * pPacket = A pointer to the packet.
 *
 * pIsEndMsg = a pointer to a BOLL that will set to TRUE if the end 
 *             of the message was reached.
 * 
 *  RETURNS:
 *        Nothing
 *
 ******************************************************************************
 */
void TOKEN_SkipEmptyLines(
    tL4Packet *pPacket, 
    vint      *pIsEndOfMsg)
{
    char   *bufEnd = EXTBUF_END(pPacket);
    uint32  eolLen;

    SIP_DebugLog(SIP_DB_GENERAL_LVL_3, "Skipping empty lines", 0, 0, 0);

    for ( ; pPacket->pCurr < bufEnd; pPacket->pCurr += eolLen )
    {
        if ( (0 == (eolLen = ABNF_ISEOL(pPacket->pCurr))) &&
                (0 == (eolLen = ABNF_ISCR(*((char *)pPacket->pCurr))))) {
            break;
        }

    }
   
    if ( pPacket->pCurr >= bufEnd || TOKEN_bCheckPgbk(pPacket) )
       *pIsEndOfMsg = TRUE;
}

/* 
 ******************************************************************************
 * ================TOKEN_SkipWS()===================
 * 
 * This function advances over white space
 *
 * pPacket = A pointer to the packet.
 *
 *  RETURNS:
 *        Nothing
 *
 ******************************************************************************
 */
void TOKEN_SkipWS(tL4Packet *pPacket)
{
    char *bufEnd = EXTBUF_END(pPacket);

    SIP_DebugLog(SIP_DB_GENERAL_LVL_3, "Skipping white space", 0, 0, 0);

    /* to skip the multiple whitespaces */
    for ( ; pPacket->pCurr < bufEnd ; pPacket->pCurr++ )
    {
        if ( !ABNF_ISWSP(*pPacket->pCurr) )
            break;
    }
}

/*
 * ======== TOKEN_copyToBuffer() ========
 * This function is used to copy a token value to a target buffer and then
 * NULL terminate the buffer.
 *
 * Return Values:
 * Nothing
 */
vint TOKEN_copyToBuffer(
   char        *target_ptr,
   vint         maxTargetSize,
   tToken      *token_ptr)
{
    maxTargetSize--;
    maxTargetSize = ((vint)token_ptr->length > maxTargetSize) ?
            maxTargetSize : (vint)token_ptr->length;
    OSAL_memCpy(target_ptr, token_ptr->pStart, maxTargetSize);
    target_ptr[maxTargetSize] = 0;
    return (maxTargetSize);
}


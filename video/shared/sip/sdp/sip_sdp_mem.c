/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 24709 $ $Date: 2014-02-20 17:27:12 +0800 (Thu, 20 Feb 2014) $
 */

#include "sip_list.h"
#include "sip_sip.h"
#include "sip_sdp_msg.h"
#include "sip_mem_pool.h"

static vint _SDP_CopyConn(tSdpConnInfo *pConn, tSdpConnInfo *pCopy);
static vint _SDP_CopyAttr(tAttribute *pAttr, tAttribute *pCopy);
static vint _SDP_CopyMedia(tSdpMedia *pMedia, tSdpMedia *pMediaCopy);

#if 1
/* Defined only for static payload types */
static tRtpAvp aAudioAlgEnum[] = {
        { "PCMU",     8000,   1, },
        { "1016",     8000,   1, },
        { "G726-32",  8000,   1, },
        { "GSM",      8000,   1, },
        { "G723",     8000,   1, },
        { "DVI4",     8000,   1, },
        { "DVI4",     16000,  1, },
        { "LPC",      8000,   1, },
        { "PCMA",     8000,   1, },
        { "G722",     8000,   1, },
        { "L16",      44100,  2, },
        { "L16",      44100,  1, },
        { "QCELP",    8000,   1, },
        { "CN",       8000,   1, },
        { "MPA",      90000,  1, },
        { "G728",     8000,   1, },
        { "DVI4",     11025,  1, },
        { "DVI4",     22050,  1, },
        { "G729",     8000,   1, },
        { "UNUSED19", 8000,   1, },
        { "UNUSED20", 8000,   1, },
        { "UNUSED21", 8000,   1, },
        { "UNUSED22", 8000,   1, },
        { "UNUSED23", 8000,   1, },
        { "UNUSED24", 8000,   1, },
        { "CELB",     90000,  1, },
        { "JPEG",     90000,  1, },
        { "UNUSED27", 8000,   1, },
        { "NV",       90000,  1, },
        { "UNUSED29", 8000,   1, },
        { "UNUSED30", 8000,   1, },
        { "H261",     90000,  1, },
        { "MPV",      90000,  1, },
        { "MP2T",     90000,  1, },
        { "H263",     90000,  0, },
};


static vint _Enum2Val(const char *pExtVal, uint16 tableSize,
                     tRtpAvp profile[],  uint16 *pt_ptr)
{
   uint16 i;
   for( i=0; i<tableSize; i++ )
   {
      if ( !OSAL_strcasecmp( profile[i].encodingName, pExtVal ) )
      {
         *pt_ptr = i;
         return SIP_OK;
      }
   }
   return SIP_FAILED;
}

static tRtpAvp *_Val2Enum(uint16 index, uint16 tableSize, tRtpAvp profile[] )
{
   if ( index >= tableSize )
      return NULL;
   return &profile[index];
}

/*******************************************************************
 * SDP_audioEncodingNameToStaticPT
 *
 * Convert audio encoding name into static payload type
 *
 * RETURNS: SIP_OK
 *          SIP_FAILED-not found
 *******************************************************************/
vint SDP_audioEncodingNameToStaticPT( const char    *name,         /* encoding name*/
                        uint16 **profile_ptr)       /* Payload Type is returned here */
{
    if ( _Enum2Val( name, sizeof(aAudioAlgEnum)/sizeof(aAudioAlgEnum[0]),
            aAudioAlgEnum, *profile_ptr) != SIP_OK )
        return SIP_FAILED;
    return SIP_OK;
}




/*******************************************************************
 * SDP_staticAudioPTtoRtpAvp
 *
 * get RTP AVP for this static audio payload type
 *
 * RETURNS: SIP_OK
 *          SIP_FAILED -not found
 *******************************************************************/
tRtpAvp *SDP_staticAudioPTtoRtpAvp(uint16 pt)
{
    return _Val2Enum( pt, sizeof(aAudioAlgEnum)/sizeof(aAudioAlgEnum[0]), aAudioAlgEnum );
}

#endif

tSdpMsg* SDP_AllocMsg()
{
    return (tSdpMsg*)SIP_memPoolAlloc(eSIP_OBJECT_SDP_MSG);
}

void SDP_DeallocMsg(tSdpMsg *pMsg)
{
    tSdpMsg *pDel;
    while (pMsg) {
        /* Clear static fields */
        pMsg->parmPresenceMask = 0 ;

        /* Clear the caution flag */
        pMsg->careFlag = FALSE;

        /* Connection info: c= */
        if ( pMsg->connInfo.next != NULL ) {
            tSdpConnInfo  *cur = pMsg->connInfo.next;
            do {
                tSdpConnInfo  *next = cur->next;
                /* Free memory to memory poll */
                SIP_memPoolFree(eSIP_OBJECT_SDP_CONN_INFO, (tDLListEntry *)cur);
                cur = next;
            } while( cur );
            pMsg->connInfo.next = NULL;
        }

        /* Attributes: a= */
        if ( pMsg->pAttr ) {
            tAttribute *cur = pMsg->pAttr;
            do {
                tAttribute  *next = cur->next;
                /* Free memory to memory poll */
                SIP_memPoolFree(eSIP_OBJECT_ATTRIBUTE, (tDLListEntry *)cur);
                cur = next;
            } while( cur );
            pMsg->pAttr = NULL;
        }

        /* Media stream(s): m= */
        SDP_DeallocMedia( &pMsg->media );
        if ( pMsg->media.next ) {
            tSdpMedia *cur = pMsg->media.next;
            do {
                tSdpMedia  *next = cur->next;
                SDP_DeallocMedia( cur );
                /* Free memory to memory poll */
                SIP_memPoolFree(eSIP_OBJECT_SDP_MEDIA, (tDLListEntry *)cur);
                cur = next;
            } while(cur);
            pMsg->media.next = NULL;
        }

        /* Other message parameters: o=,s=,i=,u=,e=,p=,t=,r=,z=,k= */
        if ( pMsg->pParmAttr ) {
            tAttribute *cur = pMsg->pParmAttr;
            do {
                tAttribute  *next = cur->next;
                /* Free memory to memory poll */
                SIP_memPoolFree(eSIP_OBJECT_ATTRIBUTE, (tDLListEntry *)cur);
                cur = next;
            } while(cur);
            pMsg->pParmAttr = NULL;
        }

        pDel = pMsg;
        pMsg = pMsg->next;
        /* Release message block */
        SIP_memPoolFree(eSIP_OBJECT_SDP_MSG, (tDLListEntry *)pDel);
    }
}

tSdpMsg* SDP_CopyMsg(tSdpMsg *pMsg)
{
   tSdpMsg   *pCopy = NULL;
   tSdpMedia *pSdpM;
   tSdpMedia *pCurrM;
   tSdpMedia *pNextM;

   if ((pCopy = SDP_AllocMsg()) == NULL)
       return NULL;

   pCopy->version = pMsg->version;
   pCopy->parmPresenceMask = pMsg->parmPresenceMask;
   pCopy->careFlag = pMsg->careFlag;

   if (_SDP_CopyConn(&pMsg->connInfo, &pCopy->connInfo) != SIP_OK) {
       SDP_DeallocMsg(pCopy);
       return (NULL);
   }

   if (pMsg->pAttr) {
       pCopy->pAttr = (tAttribute*)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE);
       if (_SDP_CopyAttr(pMsg->pAttr, pCopy->pAttr) != SIP_OK) {
           SDP_DeallocMsg(pCopy);
           return (NULL);
       }
   }

   if (pMsg->pParmAttr) {
        pCopy->pParmAttr = (tAttribute*)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE);
        if (_SDP_CopyAttr(pMsg->pParmAttr, pCopy->pParmAttr) != SIP_OK) {
            SDP_DeallocMsg(pCopy);
            return (NULL);
        }
   }


    /* initialize the SDP media pointer */
    pSdpM = &pMsg->media;
    pCurrM = pNextM = &pCopy->media;

    while (pSdpM) {
        if (!pNextM) {
            /* then allocate a new one */
            pNextM = (tSdpMedia*)SIP_memPoolAlloc(eSIP_OBJECT_SDP_MEDIA);
            if (!pNextM) {
                SDP_DeallocMsg(pCopy);
                return (NULL);
            }
            else {
                /* attach it */
                pCurrM->next = pNextM;
                pCurrM = pNextM;
            }
        }
        _SDP_CopyMedia(pSdpM, pCurrM);
        pSdpM = pSdpM->next;
        pNextM = NULL;
    }

    return pCopy;
}


void SDP_DeallocMedia(tSdpMedia *pMedia)
{
   /* Clear static fields */
   pMedia->parmPresenceMask = 0 ;
   pMedia->nFormats = 0;

   /* Media attributes: a= */
   if ( pMedia->pAttr ) {
      tAttribute *cur = pMedia->pAttr;
      do {
         tAttribute  *next = cur->next;
         /* Free memory to memory poll */
         SIP_memPoolFree(eSIP_OBJECT_ATTRIBUTE, (tDLListEntry *)cur);
         cur = next;
      } while( cur );
      pMedia->pAttr = NULL;
   }

   /* Connection info: c= */
   if ( pMedia->connInfo.next != NULL ) {
      tSdpConnInfo  *cur = pMedia->connInfo.next;
      do {
         tSdpConnInfo  *next = cur->next;
         /* Free memory to memory poll */
         SIP_memPoolFree(eSIP_OBJECT_SDP_CONN_INFO, (tDLListEntry *)cur);
         cur = next;
      } while( cur );
      pMedia->connInfo.next = NULL;
   }

   /* Media parameters: i=, k= */
   if ( pMedia->pParmAttr ) {
      tAttribute *cur = pMedia->pParmAttr;
      do {
         tAttribute  *next = cur->next;
         /* Free memory to memory poll */
         SIP_memPoolFree(eSIP_OBJECT_ATTRIBUTE, (tDLListEntry *)cur);
         cur = next;
      } while( cur );
      pMedia->pParmAttr = NULL;
   }
}

static vint _SDP_CopyMedia(tSdpMedia *pMedia, tSdpMedia *pMediaCopy)
{

   *pMediaCopy = *pMedia;
   pMediaCopy->pAttr = NULL;
   pMediaCopy->pParmAttr = NULL;


   if (pMedia->pAttr) {
       pMediaCopy->pAttr = (tAttribute*)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE);
       if (_SDP_CopyAttr(pMedia->pAttr, pMediaCopy->pAttr) != SIP_OK) {
           SDP_DeallocMedia(pMediaCopy);
           return (SIP_FAILED);
       }
   }

   if (pMedia->pParmAttr) {
        pMediaCopy->pParmAttr = (tAttribute*)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE);
        if (_SDP_CopyAttr(pMedia->pParmAttr, pMediaCopy->pParmAttr) != SIP_OK) {
           SDP_DeallocMedia(pMediaCopy);
           return (SIP_FAILED);
       }
   }

   if (_SDP_CopyConn(&pMedia->connInfo, &pMediaCopy->connInfo) != SIP_OK) {
        SDP_DeallocMedia(pMediaCopy);
        return (SIP_FAILED);
   }
   return (SIP_OK);
}

static vint _SDP_CopyConn(tSdpConnInfo *pConn, tSdpConnInfo *pCopy)
{

   *pCopy = *pConn;
   pCopy->next = NULL;
   /* Connection info: c= */
   if (pConn->next != NULL)
   {
       tSdpConnInfo *pNext = pConn->next;
       tSdpConnInfo *pCopyCurr = pCopy;
       tSdpConnInfo *pConnCopy;
       while (pNext)
       {
            pConnCopy = (tSdpConnInfo*)SIP_memPoolAlloc(
                    eSIP_OBJECT_SDP_CONN_INFO);
            if (pConnCopy == NULL)
                return (SIP_NO_MEM);
            *pConnCopy = *pNext;
            pConnCopy->next = NULL;
            pCopyCurr->next = pConnCopy;
            pCopyCurr = pConnCopy;
            pNext = pNext->next;
       }
   }
   return (SIP_OK);
}

static vint _SDP_CopyAttr(tAttribute *pAttr, tAttribute *pCopy)
{

    tAttribute *pNextAttr;
    tAttribute *pCurrAttr;

    if ((NULL == pAttr) || (NULL == pCopy)) {
        return (SIP_NO_MEM);
    }

    *pCopy = *pAttr;
    pCopy->next = NULL;

    pNextAttr = pAttr->next;
    pCurrAttr = pCopy;
    while (pNextAttr)
    {
       tAttribute *pNewAttr = (tAttribute*)SIP_memPoolAlloc(eSIP_OBJECT_ATTRIBUTE);
       if (pNewAttr == NULL)
           return (SIP_NO_MEM);
       *pNewAttr = *pNextAttr;
       pNewAttr->next = NULL;
       pCurrAttr->next = pNewAttr;
       pCurrAttr = pNewAttr;
       pNextAttr = pNextAttr->next;
    }
    return (SIP_OK);
}

vint SDP_InsertAttr(tAttribute **ppAttr, tAttribute *pAttr)
{
    if (*ppAttr == NULL) {
        *ppAttr = pAttr;
    }
    else {
        tAttribute *pLastAttr = *ppAttr;
        while (pLastAttr->next) {
            pLastAttr = pLastAttr->next;
        }
        pLastAttr->next = pAttr;
    }
    return (SIP_OK);
}

tAttribute* SDP_FindAttr(tAttribute *pAttrList, tSdpAttrType attribute)
{
    while (pAttrList) {
        if (pAttrList->id == (uint32)attribute)
            return pAttrList;
        pAttrList = pAttrList->next;
    }
    return (NULL);
}


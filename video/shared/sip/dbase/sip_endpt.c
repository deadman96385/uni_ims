/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 24709 $ $Date: 2014-02-20 17:27:12 +0800 (Thu, 20 Feb 2014) $
 */
#include "sip_types.h"
#include "sip_sip_const.h"
#include "sip_sip.h"
#include "sip_hdrflds.h"
#include "sip_clib.h"
#include "sip_mem.h"
#include "sip_auth.h"
#include "sip_dbase_endpt.h"
#include "sip_mem_pool.h"

/* 
 *****************************************************************************
 * ================EPDB_Set()===================
 * 
 * This function sets an entry in the endpoint database (EPDB).  The EPDB
 * is a small database created for each UA and is used to store UA 
 * specific attributes.
 *
 * entry = The enumerated value of the entry to set
 *
 * pValue = A pointer to the new value.
 *
 * aTable = A pointer to the entire database
 * 
 * RETURNS:
 *      SIP_OK:     Setting the entry was successfull.
 *      SIP_NOT_FOUND: The entry parameter was not understood.
 *
 ******************************************************************************
 */
vint  EPDB_Set(
    tEPDB_Type  entry, 
    void       *pValue, 
    tEPDB_Table aTable) 
{
    char *pMemory;
    uint32   size;

    tEPDB_Entry *pE;
    
    if (entry >= eEPDB_LAST_ENTRY) return (SIP_NOT_FOUND);

    pE = &aTable[entry];
    if (pE->dataType & SIP_VAL_ALLOC_DATA) {
        if (pE->dataType & SIP_VAL_DLL) {
            if (eEPDB_CREDENTIALS == entry) {
                /* Free to memory pool */
                DLLIST_Empty(&pE->x.dll, eSIP_OBJECT_AUTH_CRED);
            }
            else if ((eEPDB_CONTACTS == entry) ||
                    (eEPDB_ADDR_OF_REC == entry)) {
                /* Free to memory pool */
                DLLIST_Empty(&pE->x.dll, eSIP_OBJECT_CONTACT_HF);
            }
        }
        else {
            /* Must be tUri */
            if (NULL != pE->x.pUri) {
                /* Free to memory pool */
                SIP_memPoolFree(eSIP_OBJECT_URI, (tDLListEntry *)pE->x.pUri);
            }
        }
    }
    OSAL_memSet(pE, 0, sizeof(tEPDB_Entry));
    pE->dataType = SIP_VAL_NONE;

    if (!pValue) {
        /* then it means we just wanted 
         * to clear this value, so just return */
        return (SIP_OK);
    }

    switch (entry) {
        case eEPDB_REG_PROXY_URI:
        case eEPDB_OUTBOUND_PROXY_URI:
        case eEPDB_PROXY_URI:
        case eEPDB_WIMAX_PROXY_URI:
            size = sizeof(tUri);
            pE->type = entry;
            pE->size = size;
            pE->dataType = SIP_VAL_ALLOC_DATA;
            pMemory = (char *)SIP_memPoolAlloc(eSIP_OBJECT_URI);
            OSAL_memCpy(pMemory, pValue, size);
            pE->x.pUri = (tUri *)pMemory;
            break;
        case eEPDB_CREDENTIALS:
            pE->type = entry;
            pE->dataType |= (SIP_VAL_ALLOC_DATA | SIP_VAL_DLL);
            DLLIST_Copy((tDLList*)pValue, &pE->x.dll, eDLLIST_AUTH_CREDENTIALS);
            break;
        case eEPDB_PACKET_RATE:
            size = sizeof(tPacketRate);
            pE->type = entry;
            pE->size = size;
            pE->dataType = SIP_VAL_DATA;
            pE->x.packetRate = *((tPacketRate*)pValue);
            break;
        case eEPDB_CONTACTS:
        case eEPDB_ADDR_OF_REC:
            pE->type = entry;
            pE->dataType |= (SIP_VAL_ALLOC_DATA | SIP_VAL_DLL);
            DLLIST_Copy((tDLList*)pValue, &pE->x.dll, eDLLIST_CONTACT_HF);
            break;
        case eEPDB_CODER_TYPES:
        {
            char *pBuff = (char*)pValue;
            size = pBuff[0] + 1;
            pE->type = entry;
            pE->size = size;
            pE->dataType = SIP_VAL_DATA;
            OSAL_memCpy(pE->x.cparm, pValue, size);
            break;
        }
        case eEPDB_LAST_ENTRY:
        default:
            return (SIP_NOT_FOUND);
    } /* end of switch */

    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================EPDB_Get()===================
 * 
 * This function returns an entry in the endpoint database (EPDB).
 *
 * entry = The enumerated value of the entry to return
 *
 * aTable = A pointer to the entire database
 * 
 * RETURNS:
 *      NULL: The entry value exists
 *      tEPDB_Entry*: A pointer to the entry's value(s).
 *
 ******************************************************************************
 */
tEPDB_Entry* EPDB_Get(
    tEPDB_Type  entry, 
    tEPDB_Table aTable)
{
    if (entry >= eEPDB_LAST_ENTRY) return (NULL);
    if (aTable[entry].dataType != SIP_VAL_NONE)
        return &aTable[entry];
    else
        return (NULL);
}

/* 
 *****************************************************************************
 * ================EPDB_Empty()===================
 * 
 * This function empites out the endpoint database (EPDB).  The EPDB
 * is a small database created for each UA and is used to store UA 
 * specific attributes.
 *
 * aTable = A pointer to the entire database
 * 
 * RETURNS:
 *    Nothing
 *
 ******************************************************************************
 */
void EPDB_Empty(tEPDB_Table aTable) 
{
    tEPDB_Entry *pE;
    tEPDB_Type   entry;
    
    /* set the entry to the first one in the database */
    entry = eEPDB_ADDR_OF_REC;
    while (entry < eEPDB_LAST_ENTRY) {
        pE = &aTable[entry];
        if (pE->dataType & SIP_VAL_ALLOC_DATA) {
            if (pE->dataType & SIP_VAL_DLL) {
                if (eEPDB_CREDENTIALS == entry) {
                    /* Free to memory pool */
                    DLLIST_Empty(&pE->x.dll, eSIP_OBJECT_AUTH_CRED);
                }
                else if ((eEPDB_CONTACTS == entry) ||
                        (eEPDB_ADDR_OF_REC == entry)) {
                    /* Free to memory pool */
                    DLLIST_Empty(&pE->x.dll, eSIP_OBJECT_CONTACT_HF);
                }
            }
            else {
                /* Must be tUri */
                if (NULL != pE->x.pUri) {
                    /* Free to memory pool */
                    SIP_memPoolFree(eSIP_OBJECT_URI,
                            (tDLListEntry *)pE->x.pUri);
                }
            }
        }
        OSAL_memSet(pE, 0, sizeof(tEPDB_Entry));
        entry++;
    }
}


/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28728 $ $Date: 2014-09-05 17:01:31 +0800 (Fri, 05 Sep 2014) $
 */

#include "sip_sip.h"
#include "sip_mem.h"
#include "sip_clib.h"
#include "sip_parser_dec.h"
#include "sip_abnfcore.h"
#include "sip_dbase_sys.h"
#include "sip_mem_pool.h"

/* local prototypes */
static int  _SYSDB_getIntIdx(unsigned int  Internal, tSYSDB_Te *pTable, int tableSize, int *pIdx);
static int  _SYSDB_uriLoad          (tSYSDB_Entry *pE, char *pStr);
static int  _SYSDB_stringLoad       (tSYSDB_Entry *pE, char *pStr);
static vint _SYSDB_examineUriScheme (tSipIntMsg *pMsg);

static tSYSDB_Te _configHFTable[] =
{
    { SIP_ACCEPT_HF_STR,          eSIP_ACCEPT_HF,         {eSIP_VALUE_TYPE_NONE,{0}}, _SYSDB_stringLoad, SYSDB_ACCEPT_DFLT          },
    { SIP_ALLOW_HF_STR,           eSIP_ALLOW_HF,          {eSIP_VALUE_TYPE_NONE,{0}}, _SYSDB_stringLoad, SYSDB_ALLOW_DFLT           },
    { SIP_ALLOW_EVENTS_HF_STR,    eSIP_ALLOW_EVENTS_HF,   {eSIP_VALUE_TYPE_NONE,{0}}, _SYSDB_stringLoad, SYSDB_ALLOW_EVENTS_DFLT    },
    { SIP_MAX_FORWARDS_HF_STR,    eSIP_MAX_FORWARDS_HF,   {eSIP_VALUE_TYPE_NONE,{0}}, _SYSDB_stringLoad, SYSDB_MAX_FORWARDS_DFLT    },
    { SIP_ORGANIZATION_HF_STR,    eSIP_ORGANIZATION_HF,   {eSIP_VALUE_TYPE_NONE,{0}}, _SYSDB_stringLoad, SYSDB_ORGANIZATION_DFLT    },
    { SIP_SERVER_HF_STR,          eSIP_SERVER_HF,         {eSIP_VALUE_TYPE_NONE,{0}}, _SYSDB_stringLoad, SYSDB_SERVER_DFLT          },
    { SIP_SUPPORTED_HF_STR,       eSIP_SUPPORTED_HF,      {eSIP_VALUE_TYPE_NONE,{0}}, _SYSDB_stringLoad, SYSDB_SUPPORTED_DFLT       },
    { SIP_USER_AGENT_HF_STR,      eSIP_USER_AGENT_HF,     {eSIP_VALUE_TYPE_NONE,{0}}, _SYSDB_stringLoad, SYSDB_USER_AGENT_DFLT      },
    { SIP_ACCEPT_ENCODING_HF_STR, eSIP_ACCEPT_ENCODING_HF,{eSIP_VALUE_TYPE_NONE,{0}}, _SYSDB_stringLoad, SYSDB_ACCEPT_ENCODING_DFLT },
    { SIP_ACCEPT_LANGUAGE_HF_STR, eSIP_ACCEPT_LANGUAGE_HF,{eSIP_VALUE_TYPE_NONE,{0}}, _SYSDB_stringLoad, SYSDB_ACCEPT_LANGUAGE_DFLT },
    { SIP_ROUTE_HF_STR,           eSIP_ROUTE_HF,          {eSIP_VALUE_TYPE_NONE,{0}}, _SYSDB_uriLoad,    SYSDB_ROUTE_DFLT           },
    { SIP_CONTENT_DISP_HF_STR,    eSIP_CONTENT_DISP_HF,   {eSIP_VALUE_TYPE_NONE,{0}}, _SYSDB_stringLoad, SYSDB_CONTENT_DISP_DFLT    },
    { SIP_REQUIRE_HF_STR,         eSIP_REQUIRE_HF,        {eSIP_VALUE_TYPE_NONE,{0}}, _SYSDB_stringLoad, SYSDB_REQUIRE_DFLT         },
};

/* 
 ******************************************************************************
 * ================SYSDB_Init()===================
 *
 * This function will load SYSTEM DATABASE with the default values 
 * listed in the table above.
 *
 * RETURNS: 
 *         Nothing
 ******************************************************************************
 */
void SYSDB_Init(void)
{
    int idx;
    int tableSize = (sizeof(_configHFTable)/sizeof(_configHFTable[0]) );
    
    /* load up the header field default table */
    for (idx = 0 ; idx < tableSize ; idx++)
    {
        if ((_configHFTable[idx].pDflt != NULL) && (_configHFTable[idx].pDflt[0] != 0))
            _configHFTable[idx].pfExtInt(&_configHFTable[idx].E, _configHFTable[idx].pDflt);
    }
}

/* 
 ******************************************************************************
 * ================SYSDB_KillModule()===================
 *
 * This function will free any heap memory used in the SYSDB table. 
 * This function should only be called when the SIP stack is being destroyed.
 *
 * RETURNS: 
 *         Nothing
 ******************************************************************************
 */
void SYSDB_KillModule(void)
{
    int idx;
    int tableSize = (sizeof(_configHFTable)/sizeof(_configHFTable[0]) );
    tSYSDB_Entry *pE;
    
    /* load up the header field default table */
    for (idx = 0 ; idx < tableSize ; idx++) {
        pE = &_configHFTable[idx].E;
        if (pE->type == eSIP_VALUE_TYPE_STR && pE->u.pStr != NULL) {
            SIP_free(pE->u.pStr);
            pE->u.pStr = NULL;
        }
        else if (pE->type == eSIP_VALUE_TYPE_DLL) {
            DLLIST_Empty(&pE->u.dll, eSIP_OBJECT_NONE);
        }
    }
}

/* 
 ******************************************************************************
 * ================SYSDB_HF_Load()===================
 *
 * This function will load the default values of header fields that the config
 * module knows about.  The logic is as follows.  If, a header field exists
 * in pMap and the header field is not already populated, then populate the 
 * header field with the default value found in the HFDB datbase table
 *
 * pMap = A pointer to the bitmap that indictates which header feilds to 
 *        populate.
 *
 * pMsg = A pointer to the SIP mesasge to populate with default values.
 *           
 * RETURNS: 
 *         SIP_OK: always
 ******************************************************************************
 */
vint SYSDB_HF_Load(
    tPres64Bits *pMap, 
    tSipIntMsg  *pMsg)
{
    int idx;
    int tableSize = (sizeof (_configHFTable)/sizeof(_configHFTable[0]));
    for (idx = 0; idx < tableSize; idx++) {
        vint check = FALSE;
        
        /* get the internal headerfield value for each table entry */
        tHdrFld hf = (tHdrFld)_configHFTable[idx].Int;

        if (!pMap) {
            check = TRUE;
            HF_SetPresence(&pMsg->x.ECPresenceMasks, hf);
        }
        else if (HF_PresenceExists(pMap, hf))
            check = TRUE;

        if (check) {
            tSYSDB_Entry *pE = &_configHFTable[idx].E;
            /* 
             * Then check if the hf is empty, if so, 
             * attempt to populate it with defualt value 
             */
            switch(pE->type)
            {
            case eSIP_VALUE_TYPE_STR:
                if (*pE->u.pStr != 0) {
                    /* if it's not already in there then insert it
                     * otherwise do not replace the value in there 
                     */
                    if (!(HF_Find(&pMsg->pHFList, hf))) {
                        HF_CopyInsert(&pMsg->pHFList, hf, pE->u.pStr, 0);
                    }
                }
                break;
            case eSIP_VALUE_TYPE_INT:
                break;
            case eSIP_VALUE_TYPE_DLL:
                if (hf == eSIP_ROUTE_HF) {
                    if (DLLIST_IsEmpty(&pMsg->RouteList))
                        DLLIST_Copy(&pE->u.dll, &pMsg->RouteList, eDLLIST_ROUTE_HF);
                }
                break;
            
            case eSIP_VALUE_TYPE_NONE:
            default:
                break;

            }
        }
    } /* end of for loop */
    return (SIP_OK);
}

/* 
 ******************************************************************************
 * ================SYSDB_HF_Get()===================
 *
 * This function returns a default header field value from the HFDB database 
 * table and populates ppE with the entry from the database table.
 *
 * hfld = The enumerated value of the desired header field.
 *
 * ppE = An address to a pointer of a tSYSDB_Entry to place the value into
 *         
 *
 * RETURNS: 
 *         SIP_OK: Entry was found and returned
 *         SIP_FAILED: Could not find the header field int he database table
 ******************************************************************************
 */
vint SYSDB_HF_Get(
     tHdrFld         hfld, 
    tSYSDB_Entry  **ppE)
{
    int idx = 0;
    
    if (_SYSDB_getIntIdx(hfld, _configHFTable, sizeof (_configHFTable)/sizeof(_configHFTable[0]), &idx) == SIP_OK) {
        if (_configHFTable[idx].E.type != eSIP_VALUE_TYPE_NONE) {
            *ppE =  &(_configHFTable[idx].E);
            SIP_DebugLog(SIP_DB_CONFIG_LVL_2,"SYSDB_HF_Get: found config value for hf:%d at index %d", hfld, idx, 0);
            return (SIP_OK);
        }
    }
    SIP_DebugLog(SIP_DB_CONFIG_LVL_2,"SYSDB_HF_Get: could not find config value for hf:%d, idx:%d", hfld, idx, 0);
    return (SIP_FAILED);
}

/* 
 ******************************************************************************
 * ================SYSDB_HF_Set()===================
 *
 * This function sets a default header field value in the HFDB database. 
 *
 * hfld = The enumerated value of the desired header field.
 *
 * pE = A a pointer to a tSYSDB_Entry containing the new value
 *         
 *
 * RETURNS: 
 *         SIP_OK: Entry was found and set
 *         SIP_FAILED: Could not find the header field in the database table
 ******************************************************************************
 */
vint SYSDB_HF_Set(
    tHdrFld       hfld, 
    tSYSDB_Entry *pE)
{
    int idx;

    SIP_DebugLog(SIP_DB_CONFIG_LVL_2,"SYSDB_HF_Set HF:%d", hfld, 0, 0);
    if (_SYSDB_getIntIdx(hfld, _configHFTable, sizeof (_configHFTable)/sizeof(_configHFTable[0]), &idx) == SIP_OK) {
        _configHFTable[idx].E = *pE;
        SIP_DebugLog(SIP_DB_CONFIG_LVL_2,"SYSDB_HF_Set changing config value", 0, 0, 0);
        return (SIP_OK);
    }
    SIP_DebugLog(SIP_DB_CONFIG_LVL_2,"SYSDB_HF_Set coundnt change value", 0, 0, 0);
    return (SIP_FAILED);
}

/* This is THE API function to validate the incoming REQUEST */
vint SYSDB_ExamineRequest(tSipIntMsg *pMsg)
{
    /* first check for any error already from the parser */
    if (pMsg->internalErrorCode != 0) {
        return (SIP_FAILED);
    }

    return _SYSDB_examineUriScheme(pMsg);
}

static vint _SYSDB_examineUriScheme(tSipIntMsg *pMsg)
{
    if (pMsg->requestUri.scheme == eURI_SCHEME_DUMMY) {
        /*MSGCODE_Create(pMsg, NULL, eSIP_RSP_UNSUPP_URI_SCHEME);*/
        SIP_MSG_CODE(pMsg, eSIP_RSP_UNSUPP_URI_SCHEME);
        return (SIP_NOT_SUPPORTED);
    }
    return (SIP_OK);
}

static int _SYSDB_stringLoad(tSYSDB_Entry *pE, char *pStr)
{
    int strSize;
    
    if (pStr == NULL) {
        /* clean (delete) the entry */
        if (pE->u.pStr)
            SIP_free(pE->u.pStr);
        pE->type = eSIP_VALUE_TYPE_NONE;
        return (SIP_OK);
    }

    strSize = OSAL_strlen(pStr);
    if (!strSize) {
        pE->type = eSIP_VALUE_TYPE_NONE;
        return (SIP_FAILED);
    }
    if (pE->u.pStr)
        SIP_free(pE->u.pStr);

    pE->u.pStr = (char*)SIP_malloc(strSize + 1);
    if (pE->u.pStr) {
        pE->type = eSIP_VALUE_TYPE_STR;
        OSAL_strcpy(pE->u.pStr, pStr);
    }
    else
        return (SIP_FAILED);
    
    return (SIP_OK);
}

static int _SYSDB_getIntIdx(
    unsigned int    Internal, 
    tSYSDB_Te      *pTable, 
    int             tableSize, 
    int            *pIdx)
{
    int x;
    for(x = 0 ; x < tableSize ; x++)
    {
        if (pTable->Int == Internal)
        {
            *pIdx = x;
            return (SIP_OK);
        }
        pTable++;
    }
    return (SIP_FAILED);
}

static int _SYSDB_uriLoad(tSYSDB_Entry *pE, char *pStr)
{
    tFSM FSM;
    tL4Packet Buff;

    if (pStr == NULL || *pStr == 0) {
        /* clean (delete) the entry */
        DLLIST_Empty(&pE->u.dll, eSIP_OBJECT_NONE);
        pE->type = eSIP_VALUE_TYPE_NONE;
        return (SIP_OK);
    }

    OSAL_memSet(&FSM, 0, sizeof (tFSM));
    OSAL_memSet(&Buff, 0, sizeof (tL4Packet));
    Buff.frame = pStr;
    Buff.length = (uint16)OSAL_strlen(pStr);
    Buff.pCurr = pStr;
    Buff.pStart = pStr;

    while (TOKEN_Get(&FSM, &Buff,"<>,") == SIP_OK) {
        vint status = SIP_FAILED;
        tUri *pUri;
        
        if (FSM.isEndOfPacket) {
            if (!FSM.CurrToken.length)
                return (SIP_OK);
        }
        else {
            if (ABNF_ISCRLF(FSM.CurrToken.pDmtr) && !FSM.CurrToken.length)
                return (SIP_OK);
        }
        if (FSM.CurrToken.length > 1) { /* we want to blow off the delimiters */
            if (NULL == (pUri = (tUri *)SIP_malloc(sizeof(tUri))))
                return (SIP_NO_MEM);
            status = DEC_Uri(FSM.CurrToken.pStart, FSM.CurrToken.length, pUri);
            if (status == SIP_OK) {
                DLLIST_InitEntry(&pUri->dll);
                DLLIST_Enqueue(&pE->u.dll, &pUri->dll);
                pE->type = eSIP_VALUE_TYPE_DLL;
            }
            else
                SIP_free(pUri);
        }
        
        TOKEN_SkipWS(&Buff);
    }
    return (SIP_OK);
}




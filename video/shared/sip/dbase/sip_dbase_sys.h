/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 26773 $ $Date: 2014-06-06 11:24:11 +0800 (Fri, 06 Jun 2014) $
 */

#ifndef _SIP_DBASE_SYS_H_
#define _SIP_DBASE_SYS_H_

#include "sip_sip.h"
#include "sip_list.h"
#include "sip_voipnet.h"

/* these are default Header Field values */
#define SYSDB_ACCEPT_DFLT           "application/sdp"
#define SYSDB_ALLOW_DFLT            "INVITE, CANCEL, BYE, ACK, REGISTER, OPTIONS, REFER, SUBSCRIBE, NOTIFY, MESSAGE, INFO, PRACK, UPDATE, PUBLISH"
#define SYSDB_ALLOW_EVENTS_DFLT     "dialog,message-summary,presence"
#define SYSDB_MAX_FORWARDS_DFLT     "70"
#define SYSDB_ORGANIZATION_DFLT     "D2 Technologies"
#define SYSDB_SERVER_DFLT           ""
#define SYSDB_SUPPORTED_DFLT        "replaces"
#define SYSDB_USER_AGENT_DFLT       "vport"
#define SYSDB_ACCEPT_ENCODING_DFLT  "identity"
#define SYSDB_ACCEPT_LANGUAGE_DFLT  "*"
#define SYSDB_ROUTE_DFLT            "" 
#define SYSDB_CONTENT_DISP_DFLT     "session"
#define SYSDB_REQUIRE_DFLT          "100rel"

typedef enum eSYSDB_ValueTypes
{
    eSIP_VALUE_TYPE_NONE,
    eSIP_VALUE_TYPE_INT,
    eSIP_VALUE_TYPE_STR,
    eSIP_VALUE_TYPE_DLL,
    eSIP_VALUE_TYPE_CODERS,
}tSYSDB_ValueTypes;

typedef struct sSYSDB_Entry
{
    tSYSDB_ValueTypes  type;
    union
    {
        char          *pStr;
        unsigned int   integer;
        tDLList        dll;
        tIPAddr        ipAddr;
    }u;
}tSYSDB_Entry;

typedef  int (*tpfSYSDB_Ext2Int) (tSYSDB_Entry *pE, char* str);

typedef struct sSYSDB_Te
{
    char              *pExt;
    unsigned int       Int;
    tSYSDB_Entry       E;
    tpfSYSDB_Ext2Int   pfExtInt;
    char              *pDflt;
}tSYSDB_Te;

vint SYSDB_HF_Load(
    tPres64Bits *pMap, 
    tSipIntMsg  *pMsg);

vint SYSDB_HF_Get(
    tHdrFld        hfld, 
    tSYSDB_Entry **ppE);

vint SYSDB_HF_Set(
    tHdrFld       hfld, 
    tSYSDB_Entry *pE);

void SYSDB_Init(void);

void SYSDB_KillModule(void);

vint SYSDB_ExamineRequest(tSipIntMsg *pMsg);


#endif

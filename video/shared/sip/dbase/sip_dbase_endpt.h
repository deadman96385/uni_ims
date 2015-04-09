/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 24709 $ $Date: 2014-02-20 17:27:12 +0800 (Thu, 20 Feb 2014) $
 */

#ifndef _SIP_DBASE_ENDPT_
#define _SIP_DBASE_ENDPT_

typedef enum eEPDB_Type
{
    eEPDB_ADDR_OF_REC,
    eEPDB_PROXY_URI,
    eEPDB_OUTBOUND_PROXY_URI,
    eEPDB_REG_PROXY_URI,
    eEPDB_CODER_TYPES,
    eEPDB_PACKET_RATE,
    eEPDB_CREDENTIALS,
    eEPDB_CONTACTS,
    eEPDB_WIMAX_PROXY_URI,
    eEPDB_LAST_ENTRY,
}tEPDB_Type;

typedef struct sEPDB_Entry
{
    tEPDB_Type     type;
    uint32         size;
    uint32         dataType;
    union {
      char         cparm[SYSDB_MAX_NUM_CODERS + 1];
      tUri        *pUri;
      tPacketRate  packetRate;
      tDLList      dll;
   } x;
}tEPDB_Entry;

typedef tEPDB_Entry tEPDB_Table[]; 

vint  EPDB_Set(
    tEPDB_Type  entry, 
    void       *pValue, 
    tEPDB_Table aTable);

tEPDB_Entry* EPDB_Get(
    tEPDB_Type  entry, 
    tEPDB_Table aTable);

void EPDB_Empty(tEPDB_Table aTable);


#endif

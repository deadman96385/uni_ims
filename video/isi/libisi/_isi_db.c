/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30028 $ $Date: 2014-11-21 19:05:32 +0800 (Fri, 21 Nov 2014) $
 *
 */
#include "_isi_port.h"
#include "isi.h"
#include "isip.h"
#include "_isi_db.h"
#include "_isi_xml.h"
#include "_isi_mem.h"

/* 0 = not ready, 1 = ready */
static vint _ISI_DB_State = 0;

/* A bitmap containing all supported underlying protocols */
static uint32 _ISI_DB_Protocols = 0;
static uint32 _ISI_DB_RequestedProtocols = 0;
static uint32 _ISI_DB_FailedProtocols = 0;

static char _ISID_Country[ISI_COUNTRY_STRING_SZ];

static ISID_List _ISID_Services;
static ISID_List _ISID_Calls;
static ISID_List _ISID_Texts;
static ISID_List _ISID_Evts;
static ISID_List _ISID_Confs;
static ISID_List _ISID_Presence;
static ISID_List _ISID_GChats;
static ISID_List _ISID_Files;
static ISID_List _ISID_Ussds;


static ISID_Stream _ISID_Streams;


/*
 * Supported coders.  ISI validates the codecs that
 * applications attempt to use.  The ISI will not allow
 * codecs that are not defined in this table
 */
static ISID_CoderE _ISID_CoderTable[] = {
    { "PCMU",                NULL, ISI_SESSION_TYPE_AUDIO },
    { "PCMA",                NULL, ISI_SESSION_TYPE_AUDIO },
    { "G726-32",             NULL, ISI_SESSION_TYPE_AUDIO },
    { "G729",                NULL, ISI_SESSION_TYPE_AUDIO },
    { "CN",                  NULL, ISI_SESSION_TYPE_AUDIO },
    { "iLBC",                NULL, ISI_SESSION_TYPE_AUDIO },
    { "SILK-24k",            NULL, ISI_SESSION_TYPE_AUDIO },
    { "SILK-16k",            NULL, ISI_SESSION_TYPE_AUDIO },
    { "SILK-8k",             NULL, ISI_SESSION_TYPE_AUDIO },
    { "G722",                NULL, ISI_SESSION_TYPE_AUDIO },
    { "G7221",               NULL, ISI_SESSION_TYPE_AUDIO },
    { "telephone-event",     NULL, ISI_SESSION_TYPE_AUDIO },
    { "telephone-event-16k", NULL, ISI_SESSION_TYPE_AUDIO },
    { "AMR",     "octet-align=1", ISI_SESSION_TYPE_AUDIO },
    { "AMR-WB",  "octet-align=1", ISI_SESSION_TYPE_AUDIO },
    { "H264",                NULL, ISI_SESSION_TYPE_VIDEO },
    { "H263",                NULL, ISI_SESSION_TYPE_VIDEO },
    { "H263-1998",           NULL, ISI_SESSION_TYPE_VIDEO },
    { "H263-2000",           NULL, ISI_SESSION_TYPE_VIDEO }

};

/*
 * ======== _ISID_allocStream() ========
 * This function checks if there is an available stream resource available and
 * if so marks it as "in use" and returns the streamId in "streamId_ptr".
 *
 * Returns:
 *  ISI_RETURN_OK: An available streamId was found and is returned
 *  ISI_RETURN_FAILED: No available streamId's were found
 */
static ISI_Return _ISID_allocStream(
    uint8  *streamId_ptr)
{
    vint  x;
    uint8 map;
    uint8 mask;

    if (_ISID_Streams.inUse >= ISID_MAX_NUM_CALLS) {
        /* Then there are no more available streams */
        return (ISI_RETURN_FAILED);
    }
    /* Find an available */
    map = _ISID_Streams.bitmask;
    mask = 0x01;
    for (x = 0 ; x < ISID_MAX_NUM_CALLS ; x++) {
        if (!(map & mask)) {
            /* Found an available stream id, which is the same as the index */
            _ISID_Streams.bitmask |= mask;
            *streamId_ptr = (uint8)x;
            _ISID_Streams.inUse++;
            return (ISI_RETURN_OK);
        }
        mask <<= 1;
    }
    return (ISI_RETURN_FAILED);
}

/*
 * ======== _ISID_freeStream() ========
 * This function frees (releases) a stream ID that had been previously
 * allocated.  This typically happens at the end of a call.
 *
 * Returns:
 *  Nothing
 */
static void _ISID_freeStream(
    uint8  streamId)
{
    uint8 mask;

    mask = 1;
    mask <<= streamId;
    /* Invert */
    mask = (~mask);
    _ISID_Streams.bitmask &= mask;
    /* Decrement the 'inUse' counter */
    if (_ISID_Streams.inUse) {
        _ISID_Streams.inUse--;
    }
    return;
}

/*
 * ======== _ISID_entryClean() ========
 * This function cleans (zero's) a ISID_Entry object
 *
 * Returns:
 *  Nothing
 */
static void _ISID_entryClean(
    ISID_Entry *e_ptr)
{
    e_ptr->desc = 0;
    e_ptr->desc2 = 0;
    e_ptr->next_ptr = 0;
}

/*
 * ======== _ISID_entryAdd() ========
 * This function will insert an entry into a linked list
 * (referred to as a database).  Entries are inserted at the tail
 * of the list.  Also, when an entry is inserted the list will assign it
 * a unique number (referred to as a 'desc' or 'id' -which is short for
 * identification descriptor. Developers can use these descriptors to
 * uniquely identify the entry being inserted. After the function generates
 * a unique id, it will write to the data pointed to by id_ptr.
 * Note, that if the data pointed to by id_ptr is NOT '0', then the unique
 * id will not be generated, rather that value provided in id_ptr will be used.
 *
 * Returns:
 *  ISI_RETURN_OK     : Entry was successfully inserted.
 *  ISI_RETURN_FAILED : The entry could not be inserted, the max length of the
 *                      linked list was reached.
 */
static ISI_Return _ISID_entryAdd(
    ISID_Entry  *e_ptr,
    ISID_List   *l_ptr,
    ISI_Id      *id_ptr,
    vint         maxListSize)
{
    ISID_Entry *curr_ptr;

    if (l_ptr->numEntries < maxListSize) {
        curr_ptr = l_ptr->head_ptr;
        if (curr_ptr == NULL) {
            /* Then the list is empty */
            l_ptr->head_ptr = e_ptr;
            l_ptr->tail_ptr = e_ptr;
        }
        else {
            l_ptr->tail_ptr->next_ptr = e_ptr;
            l_ptr->tail_ptr = e_ptr;
        }
        l_ptr->numEntries++;
        if (*id_ptr == 0) {
            /*
             * If it's zero then let the DB determine the
             * descriptor otherwise use the value provided
             */
            l_ptr->descCnt++;
            /* Handle rollover. */
            if (l_ptr->descCnt > ISID_MAX_ID) {
                l_ptr->descCnt = 1;
            }
            e_ptr->desc = l_ptr->descCnt;
            *id_ptr =  e_ptr->desc;
        }
        else {
            e_ptr->desc = *id_ptr;
        }

        return (ISI_RETURN_OK);
    }
    else {
        *id_ptr = 0;
        return (ISI_RETURN_FAILED);
    }
}

/*
 * ======== _ISID_entryAddFirst() ========
 * This function will insert an entry into a linked list
 * (referred to as a database).  Entries are inserted at the head
 * of the list.  Also, when an entry is inserted the list will assign it
 * a unique number (referred to as a 'desc' or 'id' -which is short for
 * identification descriptor. Developers can use these descriptors to
 * uniquely identify the entry being inserted. After the function generates
 * a unique id, it will write to the data pointed to by id_ptr.
 * Note, that if the data pointed to by id_ptr is NOT '0', then the unique
 * id will not be generated, rather that value provided in id_ptr will be used.
 *
 * Returns:
 *  ISI_RETURN_OK     : Entry was successfully inserted.
 *  ISI_RETURN_FAILED : The entry could not be inserted, the max length of the
 *                      linked list was reached.
 */
static ISI_Return _ISID_entryAddFirst(
    ISID_Entry  *e_ptr,
    ISID_List   *l_ptr,
    ISI_Id      *id_ptr,
    vint         maxListSize)
{
    ISID_Entry *curr_ptr;

    if (l_ptr->numEntries < maxListSize) {
        curr_ptr = l_ptr->head_ptr;
        if (curr_ptr == NULL) {
            /* Then the list is empty */
            l_ptr->head_ptr = e_ptr;
            l_ptr->tail_ptr = e_ptr;
        }
        else {
            e_ptr->next_ptr = l_ptr->head_ptr;
            l_ptr->head_ptr = e_ptr;
        }
        l_ptr->numEntries++;
        if (*id_ptr == 0) {
            /*
             * If it's zero then let the DB determine the
             * descriptor otherwise use the value provided
             */
            l_ptr->descCnt++;
            /* Handle rollover. */
            if (l_ptr->descCnt > ISID_MAX_ID) {
                l_ptr->descCnt = 1;
            }
            e_ptr->desc = l_ptr->descCnt;
            *id_ptr =  e_ptr->desc;
        }
        else {
            e_ptr->desc = *id_ptr;
        }

        return (ISI_RETURN_OK);
    }
    else {
        *id_ptr = 0;
        return (ISI_RETURN_FAILED);
    }
}

/*
 * ======== _ISID_entryDestroy ========
 * This function will search for an entry in a linked list and then remove it
 * and free it back to heap memory.
 *
 * id : The id number of the entry that the developer wishes to remove.
 *      This was the value that was generated when the entry was inserted
 *      via _ISID_entryAdd().
 *
 * l_ptr :  A pointer to the list to search.
 *
 * Returns:
 *  ISI_RETURN_OK     : Entry was successfully removed.
 *  ISI_RETURN_FAILED : The entry could not be found, hence nothing removed.
 */
static ISI_Return _ISID_entryDestroy(
    ISI_Id     id,
    ISID_List *l_ptr,
    ISI_objType objType)
{
    ISID_Entry *curr_ptr;
    ISID_Entry *prev_ptr;

    prev_ptr = NULL;
    curr_ptr = l_ptr->head_ptr;

    while (curr_ptr) {
        if (curr_ptr->desc == id) {
            if (prev_ptr) {
                prev_ptr->next_ptr = curr_ptr->next_ptr;
                /* Check if we are the tail */
                if (curr_ptr == l_ptr->tail_ptr) {
                    l_ptr->tail_ptr = prev_ptr;
                }
            }
            else {
                /* Then we are removing the head */
                l_ptr->head_ptr = curr_ptr->next_ptr;
            }
            /* now free the object */
            ISI_free(curr_ptr, objType);
            l_ptr->numEntries--;
            return (ISI_RETURN_OK);
        }
        prev_ptr = curr_ptr;
        curr_ptr = curr_ptr->next_ptr;
    }
    return (ISI_RETURN_FAILED);
}

/*
 * ======== _ISID_entryGet ========
 * This function will search for and return an entry in a linked list.
 *
 * id : The id number of the entry that the developer wishes to search for.
 *      This was the value that was generated when the entry was inserted
 *      via _ISID_entryAdd().
 *
 * byPrimaryDesc : '0' will try to match the 'id' value to the 'desc2'
 *                  value inside each list entry. '1' will try to match
 *                  the 'id' to 'desc'.
 *
 * l_ptr : A pointer to the list to search.
 *
 * entry_ptr : An address to a pointer to a ISID_Entry object where the
 *             function will write a pointer to the found entry.
 *
 * Returns:
 *  ISI_RETURN_OK     : Entry was successfully found and entry_ptr has the
 *                      pointer value.
 *  ISI_RETURN_FAILED : The entry could not be found.  entry_ptr will be
 *                      invalid.
 */
static ISI_Return _ISID_entryGet(
    ISI_Id       id,
    vint         byPrimaryDesc,
    ISID_List   *l_ptr,
    ISID_Entry **entry_ptr)
{
    ISI_Id      d;
    ISID_Entry *e_ptr;

    e_ptr = l_ptr->head_ptr;
    while (e_ptr) {
        if (byPrimaryDesc) {
            d = e_ptr->desc;
        }
        else {
            d = e_ptr->desc2;
        }
        if (d == id) {
            *entry_ptr = e_ptr;
            return (ISI_RETURN_OK);
        }
        e_ptr = e_ptr->next_ptr;
    }
    return (ISI_RETURN_FAILED);
}

/*
 * ======== ISID_accountClean ========
 * This function initializes a ISID_Account object.
 *
 * Returns:
 *  Nothing
 */
void ISID_accountClean(
    ISID_Account *acct_ptr)
{
    acct_ptr->next_ptr = NULL;
    acct_ptr->szRealm[0] = 0;
    acct_ptr->szPassword[0] = 0;
    acct_ptr->szUsername[0] = 0;
}


/*
 * ======== ISID_coderClean ========
 * This function initializes a ISID_Coder object.
 *
 * Returns:
 *  Nothing
 */
void ISID_coderClean(
    ISIP_Coder *coder_ptr)
{
    coder_ptr->relates = ISI_SESSION_TYPE_NONE;
    coder_ptr->szCoderName[0] = 0;
    coder_ptr->description[0] = 0;
}

/*
 * ======== _ISID_serviceClean ========
 * This function will initialize a ISID_ServiceId object.  It is typically
 * called when a service is first being created.
 *
 * Returns:
 *  Nothing.
 */
static void _ISID_serviceClean(
    ISID_ServiceId *service_ptr)
{
    vint x;

    _ISID_entryClean(&service_ptr->e);
    service_ptr->protocol = 0;
    service_ptr->szUri[0] = 0;
    service_ptr->szProxy[0] = 0;
    service_ptr->szRegistrar[0] = 0;
    service_ptr->szOutboundProxy[0] = 0;
    service_ptr->szStunServer[0] = 0;
    service_ptr->accounts_ptr = NULL;
    service_ptr->numAccounts = 0;
    service_ptr->privateCid = 0;
    service_ptr->isActivated = 0;
    service_ptr->vccEnabled = 0;
    service_ptr->szInfcName[0] = 0;
    service_ptr->bsId.szBsId[0] = 0;
    service_ptr->szRelayServer[0] = 0;
    service_ptr->szStorageServer[0] = 0;
    service_ptr->szChatServer[0] = 0;
    service_ptr->szNickname[0] = 0;
    OSAL_memSet(&service_ptr->infcAddress, 0, sizeof(OSAL_NetAddress));
    /* Set the country to what the ISI module knows globally */
    OSAL_strncpy(service_ptr->szCountry, _ISID_Country, ISI_COUNTRY_STRING_SZ);
    for (x = 0; x < ISI_CODER_NUM; x++) {
        ISID_coderClean(&service_ptr->coders[x]);
    }
    service_ptr->features = 0;
}

/*
 * ======== _ISID_callClean ========
 * This function will initialize a ISID_CallId object.  It is typically
 * called when a call is first being initialized.
 *
 * Returns:
 *  Nothing.
 */
static void _ISID_callClean(
    ISID_CallId *call_ptr)
{
    vint x;

    _ISID_entryClean(&call_ptr->e);
    call_ptr->type = ISI_SESSION_TYPE_NONE;
    call_ptr->service_ptr = NULL;
    call_ptr->audioDir = ISI_SESSION_DIR_SEND_RECV;
    call_ptr->videoDir = ISI_SESSION_DIR_SEND_RECV;
    call_ptr->state = ISI_CALL_STATE_INVALID;
    call_ptr->previousState = ISI_CALL_STATE_INVALID;
    call_ptr->transId = 0;
    call_ptr->szRemoteUri[0] = 0;
    call_ptr->szTargetUri[0] = 0;
    call_ptr->szSubject[0] = 0;
    call_ptr->confMask = 0;
    call_ptr->streamId = 0;
    call_ptr->isRinging = 0;
    call_ptr->ringTemplate = 0;
    call_ptr->cidType = ISI_SESSION_CID_TYPE_NONE;
    call_ptr->rsrcStatus = ISI_RESOURCE_STATUS_NOT_READY;
    call_ptr->srvccStatus = ISI_SRVCC_STATUS_NONE;
    call_ptr->supsrvHfExist = 0;
    call_ptr->historyInfo[0] = 0;
    for (x = 0; x < ISI_CODER_NUM; x++) {
        ISID_coderClean(&call_ptr->coders[x]);
    }
    /* init RTP interface (candidate) stuff */
    OSAL_memSet(&call_ptr->rtpAudioLcl, 0, sizeof(call_ptr->rtpAudioLcl));
    OSAL_memSet(&call_ptr->rtpAudioRmt, 0, sizeof(call_ptr->rtpAudioRmt));
    OSAL_memSet(&call_ptr->rtpVideoLcl, 0, sizeof(call_ptr->rtpVideoLcl));
    OSAL_memSet(&call_ptr->rtpVideoRmt, 0, sizeof(call_ptr->rtpVideoRmt));
    OSAL_memSet(&call_ptr->audioKeys, 0, sizeof(call_ptr->audioKeys));
    OSAL_memSet(&call_ptr->videoKeys, 0, sizeof(call_ptr->videoKeys));
}

/*
 * ======== _ISID_gchatClean ========
 * This function will initialize a ISID_GChatId object.  It is typically
 * called when a group chat room is first being initialized.
 *
 * Returns:
 *  Nothing.
 */
static void _ISID_gchatClean(
    ISID_GChatId *chat_ptr)
{
    _ISID_entryClean(&chat_ptr->e);
    chat_ptr->service_ptr = NULL;
    chat_ptr->state = ISI_CHAT_STATE_INVALID;
    chat_ptr->requiresPassword = 0;
    chat_ptr->szLocalIdentity[0] = 0;
    chat_ptr->szInvitor[0] = 0;
    chat_ptr->szRemoteAddress[0] = 0;
    chat_ptr->szParticipants[0] = 0;
    chat_ptr->szSubject[0] = 0;
    chat_ptr->szContributionId[0] = 0;
    chat_ptr->isConference = 0;
    chat_ptr->firstMessageId = 0;
    return;
}

/*
 * ======== _ISID_evtClean ========
 * This function will initialize a ISID_EvtId object.  It is typically
 * called when an event (such as a telephone events) is first sent or
 * received.
 *
 * Returns:
 *  Nothing.
 */
static void _ISID_evtClean(
    ISID_EvtId *evt_ptr)
{
    _ISID_entryClean(&evt_ptr->e);
    evt_ptr->call_ptr = NULL;
    evt_ptr->service_ptr = NULL;
    evt_ptr->state = ISID_EVENT_STATE_NONE;
    OSAL_memSet(&evt_ptr->isiMsg, 0, sizeof(ISIP_Message));
}

/*
 * ======== _ISID_presClean ========
 * This function will initialize a ISID_PresId object.  It is typically
 * called when a presence event is first sent or
 * received.
 *
 * Returns:
 *  Nothing.
 */
static void _ISID_presClean(
    ISID_PresId *pres_ptr)
{
    _ISID_entryClean(&pres_ptr->e);
    pres_ptr->presLen = 0;
    pres_ptr->presOffset = 0;
    pres_ptr->service_ptr = NULL;
    pres_ptr->chatId = 0;
    pres_ptr->state = ISID_PRES_STATE_NONE;
    pres_ptr->reason = ISIP_PRES_REASON_INVALID;
    OSAL_memSet(&pres_ptr->isiMsg, 0, sizeof(ISIP_Message));
}

/*
 * ======== _ISID_confClean ========
 * This function will initialize a ISID_ConfId object.  It is typically
 * called when a Conference call is first being created.
 *
 * Returns:
 *  Nothing.
 */
static void _ISID_confClean(
    ISID_ConfId *conf_ptr)
{
    vint x;
    _ISID_entryClean(&conf_ptr->e);
    for (x = 0 ; x < ISI_CONF_USERS_NUM ; x++) {
        conf_ptr->aCall[x] = 0;
        conf_ptr->aConfMask[x] = 0;
    }
}

/*
 * ======== _ISID_callRemoveFromAllConf ========
 * This function is typically called when a call is being terminated.
 * This function will search all active call conferences and remove
 * the call if it exists.
 *
 * Returns:
 *  Nothing.
 */
static void _ISID_callRemoveFromAllConf(
    ISID_CallId *call_ptr)
{
    ISID_ConfId *conf_ptr;
    ISID_Entry  *e_ptr;
    ISI_Return   ret;

    e_ptr = _ISID_Confs.head_ptr;
    while (e_ptr) {
        conf_ptr = (ISID_ConfId*)e_ptr;
        ret = ISID_confRemoveCall(conf_ptr, call_ptr, 1);
        if (ret == ISI_RETURN_OK) {
            /* Then the call was found and removed */
            return;
        }
        else if (ret == ISI_RETURN_DONE) {
            /* Call was found and it was the
             * last one in the conference.
             * So destroy the conference
             */
            ISID_confDestroy(conf_ptr->e.desc);
            return;
        }
        e_ptr = e_ptr->next_ptr;
    }
    return;
}

/*
 * ======== ISID_textClean ========
 * This function will initialize a ISID_TextId object.  It is typically
 * called when a text message is first sent or received.
 *
 * Returns:
 *  Nothing.
 */
void ISID_textClean(
    ISID_TextId *text_ptr)
{
    _ISID_entryClean(&text_ptr->e);
    text_ptr->msgLen = 0;
    text_ptr->msgOffset = 0;
    text_ptr->chatId = 0;
    text_ptr->service_ptr = NULL;
    text_ptr->state = ISID_TEXT_STATE_NONE;
    OSAL_memSet(&text_ptr->isiMsg, 0, sizeof(ISIP_Message));
    return;
}

/*
 * ======== ISID_fileClean ========
 * This function will initialize a ISID_FileId object.  It is typically
 * used when a file is sent or received.
 *
 * Returns:
 *  Nothing.
 */
void ISID_fileClean(
    ISID_FileId *file_ptr)
{
    _ISID_entryClean(&file_ptr->e);
    file_ptr->service_ptr = NULL;
    file_ptr->state = ISID_FILE_STATE_NONE;
    OSAL_memSet(&file_ptr->isiMsg, 0, sizeof(ISIP_Message));
    return;
}

/*
 * ======== ISID_getState ========
 * This function will return the state of the database (ISID) module.
 * this function is used to determine whether or not the ISI module
 * has been initialized or not.
 *
 * Returns:
 *  0 : ISI database is not initialized.
 *  1 : ISI database is initialized and ready to go`.
 */
vint ISID_getState(void)
{
    return (_ISI_DB_State);
}

/*
 * ======== ISID_setState ========
 * This function is used to set the state of the ISI module.
 * It is called by the isi.c file when the isi module is successfully
 * initialized.
 *
 * state : '1' the ISI module & ISID sub-module are currently initialized
 *          and ready to go.  '0', the ISI module is NOT initialized.
 *
 * Returns:
 *  0 : ISI database is not initialized.
 *  1 : ISI database is initialized and ready to go`.
 */
void ISID_setState(vint state)
{
    _ISI_DB_State = state;
}

/*
 * ======== ISID_checkProtocol ========
 * This function will return OK if the protocol parameter is supported
 * by ISI.  Otherwise it will return a failure, meaning that the ISI module
 * does not support this protocol.
 *
 * protocol : An enumerated value representing the protocol to return.
 *
 * Returns:
 *  ISI_RETURN_FAILED : ISI module DOES NOT supports the protocol specified
 *                      in 'protocol'
 *  ISI_RETURN_OK     : ISI module supports the protocol specified in
 *                      'protocol'
 */
ISI_Return ISID_checkProtocol(vint protocol)
{
    uint32 mask;
    mask = (1 << protocol);
    if (mask & _ISI_DB_Protocols) {
        return (ISI_RETURN_OK);
    }
    else {
        return (ISI_RETURN_FAILED);
    }
}

/*
 * ======== ISID_setProtocol ========
 * This function will set a bitmask used to track supported protocols in ISI
 *
 * protocol : An enumerated value representing the protocol to set
 *
 * enable : '1' enable the protocol specified in protocol, '0' disable that
 *          protocol.
 *
 * Returns:
 *  uint32 : the bitmask defined by 'maskType'.
 */
uint32 ISID_setProtocol(
    ISID_ProtocolMask maskType,
    vint              protocol,
    vint              enable)
{
    uint32  mask;
    uint32 *mask_ptr;

    switch (maskType) {
    case ISID_PROTOCOLS_ACTIVE:
        mask_ptr = &_ISI_DB_Protocols;
        break;
    case ISID_PROTOCOLS_FAILED:
        mask_ptr = &_ISI_DB_FailedProtocols;
        break;
    case ISID_PROTOCOLS_REQUESTED:
        mask_ptr = &_ISI_DB_RequestedProtocols;
        break;
    default:
        return (0);
    }

    mask = (1 << protocol);

    if (enable) {
        /* Set the bit */
        *mask_ptr |= mask;
    }
    else {
        mask = ~mask;
        *mask_ptr &= mask;
    }
    return (*mask_ptr);
}

/*
 * ======== ISID_getProtocol ========
 * This function will return the bitmask defined by maskType
 *
 * maskType : An enumerated value representing the bitmask to get
 *
 * Returns:
 *  uint32 : the bitmask defined by 'maskType'.
 */
uint32 ISID_getProtocol(
    ISID_ProtocolMask maskType)
{
    switch (maskType) {
    case ISID_PROTOCOLS_ACTIVE:
        return (_ISI_DB_Protocols);
    case ISID_PROTOCOLS_FAILED:
        return (_ISI_DB_FailedProtocols);
    case ISID_PROTOCOLS_REQUESTED:
        return (_ISI_DB_RequestedProtocols);
    default:
        return (0);
    }
}

/*
 * ======== ISID_setCountry ========
 * This function is used to set the "country" for which the ISI module
 * will be used within. Currently this function does not stimulate any
 * special behavior based on the country.  However, in the future, it
 * may automatically set up ring & tone types.
 *
 * country_ptr : A pointer to a NULL terminated string.
 *
 * Returns:
 *  Nothing
 */
void ISID_setCountry(char *country_ptr)
{
    if (country_ptr) {
        OSAL_strncpy(_ISID_Country, country_ptr, ISI_COUNTRY_STRING_SZ);
    }
}

/*
 * ======== ISID_init ========
 * This function is used to initialized the internals used by the ISID.
 * It is called when the ISI module is initialized.  It will init
 * global data and initiate mutexes used by the linked lists (databases).
 *
 *
 * Returns:
 *  Nothing
 */
ISI_Return ISID_init(void)
{
    vint x;

    _ISID_Country[0] = 0;
    _ISI_DB_State = 0;
    _ISI_DB_Protocols = 0;
    _ISI_DB_FailedProtocols = 0;
    _ISI_DB_RequestedProtocols = 0;
    /* These are used to track stream id's that are in use */
    _ISID_Streams.bitmask = 0;
    _ISID_Streams.inUse = 0;

    for (x = 0 ; x < ISID_MAX_NUM_PROTOCOLS ; x++) {
        _ISI_DB_RequestedProtocols |= (1 << x);
    }

    /* Init all the lists used in the database */
    _ISID_Services.head_ptr = NULL;
    _ISID_Services.tail_ptr = NULL;
    _ISID_Services.numEntries = 0;
    _ISID_Services.descCnt = 0;
    if (0 == ISI_semMutexCreate(&_ISID_Services.mutex)) {
        return (ISI_RETURN_FAILED);
    }

    _ISID_Calls.head_ptr = NULL;
    _ISID_Calls.tail_ptr = NULL;
    _ISID_Calls.numEntries = 0;
    _ISID_Calls.descCnt = 0;
    if (0 == ISI_semMutexCreate(&_ISID_Calls.mutex)) {
        ISI_semDelete(_ISID_Services.mutex);
        return (ISI_RETURN_FAILED);
    }

    _ISID_Texts.head_ptr = NULL;
    _ISID_Texts.tail_ptr = NULL;
    _ISID_Texts.numEntries = 0;
    _ISID_Texts.descCnt = 0;
    if (0 == ISI_semMutexCreate(&_ISID_Texts.mutex)) {
        ISI_semDelete(_ISID_Services.mutex);
        ISI_semDelete(_ISID_Calls.mutex);
        return (ISI_RETURN_FAILED);
    }
    _ISID_Evts.head_ptr = NULL;
    _ISID_Evts.tail_ptr = NULL;
    _ISID_Evts.numEntries = 0;
    _ISID_Evts.descCnt = 0;
    if (0 == ISI_semMutexCreate(&_ISID_Evts.mutex)) {
        ISI_semDelete(_ISID_Texts.mutex);
        ISI_semDelete(_ISID_Services.mutex);
        ISI_semDelete(_ISID_Calls.mutex);
        return (ISI_RETURN_FAILED);
    }
    _ISID_Confs.head_ptr = NULL;
    _ISID_Confs.tail_ptr = NULL;
    _ISID_Confs.numEntries = 0;
    _ISID_Confs.descCnt = 0;
    if (0 == ISI_semMutexCreate(&_ISID_Confs.mutex)) {
        ISI_semDelete(_ISID_Evts.mutex);
        ISI_semDelete(_ISID_Texts.mutex);
        ISI_semDelete(_ISID_Services.mutex);
        ISI_semDelete(_ISID_Calls.mutex);
        return (ISI_RETURN_FAILED);
    }
    _ISID_Presence.head_ptr = NULL;
    _ISID_Presence.tail_ptr = NULL;
    _ISID_Presence.numEntries = 0;
    _ISID_Presence.descCnt = 0;
    if (0 == ISI_semMutexCreate(&_ISID_Presence.mutex)) {
        ISI_semDelete(_ISID_Confs.mutex);
        ISI_semDelete(_ISID_Evts.mutex);
        ISI_semDelete(_ISID_Texts.mutex);
        ISI_semDelete(_ISID_Services.mutex);
        ISI_semDelete(_ISID_Calls.mutex);
        return (ISI_RETURN_FAILED);
    }
    _ISID_GChats.head_ptr = NULL;
    _ISID_GChats.tail_ptr = NULL;
    _ISID_GChats.numEntries = 0;
    _ISID_GChats.descCnt = 0;
    if (0 == ISI_semMutexCreate(&_ISID_GChats.mutex)) {
        ISI_semDelete(_ISID_Presence.mutex);
        ISI_semDelete(_ISID_Confs.mutex);
        ISI_semDelete(_ISID_Evts.mutex);
        ISI_semDelete(_ISID_Texts.mutex);
        ISI_semDelete(_ISID_Services.mutex);
        ISI_semDelete(_ISID_Calls.mutex);
        return (ISI_RETURN_FAILED);
    }
    _ISID_Files.head_ptr = NULL;
    _ISID_Files.tail_ptr = NULL;
    _ISID_Files.numEntries = 0;
    _ISID_Files.descCnt = 0;
    if (0 == ISI_semMutexCreate(&_ISID_Files.mutex)) {
        ISI_semDelete(_ISID_Presence.mutex);
        ISI_semDelete(_ISID_Confs.mutex);
        ISI_semDelete(_ISID_Evts.mutex);
        ISI_semDelete(_ISID_Texts.mutex);
        ISI_semDelete(_ISID_Services.mutex);
        ISI_semDelete(_ISID_Calls.mutex);
        ISI_semDelete(_ISID_GChats.mutex);
        return (ISI_RETURN_FAILED);
    }
    _ISID_Ussds.head_ptr = NULL;
    _ISID_Ussds.tail_ptr = NULL;
    _ISID_Ussds.numEntries = 0;
    _ISID_Ussds.descCnt = 0;
    if (0 == ISI_semMutexCreate(&_ISID_Ussds.mutex)) {
        ISI_semDelete(_ISID_Presence.mutex);
        ISI_semDelete(_ISID_Confs.mutex);
        ISI_semDelete(_ISID_Evts.mutex);
        ISI_semDelete(_ISID_Texts.mutex);
        ISI_semDelete(_ISID_Services.mutex);
        ISI_semDelete(_ISID_Calls.mutex);
        ISI_semDelete(_ISID_GChats.mutex);
        ISI_semDelete(_ISID_Files.mutex);
        return (ISI_RETURN_FAILED);
    }
    return (ISI_RETURN_OK);
}

/*
 * ======== ISID_destroy ========
 * This function is used to destroy internals used by the ISID.
 * It is called when the ISI module is destroyed or deactivated.
 *
 * Returns:
 *  Nothing
 */
void ISID_destroy(void)
{
    ISI_semDelete(_ISID_Services.mutex);
    ISI_semDelete(_ISID_Calls.mutex);
    ISI_semDelete(_ISID_Texts.mutex);
    ISI_semDelete(_ISID_Evts.mutex);
    ISI_semDelete(_ISID_Confs.mutex);
    ISI_semDelete(_ISID_Presence.mutex);
    ISI_semDelete(_ISID_GChats.mutex);
    ISI_semDelete(_ISID_Files.mutex);
    ISI_semDelete(_ISID_Ussds.mutex);
    _ISID_Services.mutex = 0;
    _ISID_Calls.mutex = 0;
    _ISID_Texts.mutex = 0;
    _ISID_Evts.mutex = 0;
    _ISID_Confs.mutex = 0;
    _ISID_Presence.mutex = 0;
    _ISID_GChats.mutex = 0;
    _ISID_Files.mutex = 0;
    _ISID_Ussds.mutex = 0;
}

/*
 * ======== ISID_lockServices ========
 * This function is used to take a semaphore against the
 * linked list (database) used to manage "services".
 *
 * Returns:
 *  ISI_RETURN_OK       : The semaphore was successfully taken.
 *  ISI_RETURN_FAILED   : The semaphore could not be taken.
 */
ISI_Return ISID_lockServices(void)
{
    if (0 == ISI_semAcquire(_ISID_Services.mutex)) {
        return (ISI_RETURN_FAILED);
    }
    return (ISI_RETURN_OK);
}

/*
 * ======== ISID_unlockServices ========
 * This function is used to Give a semaphore back to the
 * linked list (database) used to manage "services".
 *
 * Returns:
 *  Nothing
 */
void ISID_unlockServices(void)
{
    ISI_semGive(_ISID_Services.mutex);
}

/*
 * ======== ISID_lockCalls ========
 * This function is used to take a semaphore against the
 * linked list (database) used to manage "calls".
 * Note that to successfully use a mutex on "calls"
 * the "services" mutex must also be taken.
 *
 * Returns:
 *  ISI_RETURN_OK       : The semaphore was successfully taken.
 *  ISI_RETURN_FAILED   : The semaphore could not be taken.
 */
ISI_Return ISID_lockCalls(void)
{
    /* Lock both the services and calls database */
    if (0 == ISI_semAcquire(_ISID_Services.mutex)) {
        return (ISI_RETURN_FAILED);
    }
    if (0 == ISI_semAcquire(_ISID_Calls.mutex)) {
        ISI_semGive(_ISID_Services.mutex);
        return (ISI_RETURN_FAILED);
    }
    /* Lock the confs as well since the state of a
     * call affects the state of a conference
     */
    if (0 == ISI_semAcquire(_ISID_Confs.mutex)) {
        ISI_semGive(_ISID_Calls.mutex);
        ISI_semGive(_ISID_Services.mutex);
        return (ISI_RETURN_FAILED);
    }
    return (ISI_RETURN_OK);
}

/*
 * ======== ISID_lockGChats ========
 * This function is used to take a semaphore against the
 * linked list (database) used to manage group chat rooms.
 * Note that to successfully use a mutex on group chats
 * the "services" mutex must also be taken.
 *
 * Returns:
 *  ISI_RETURN_OK       : The semaphore was successfully taken.
 *  ISI_RETURN_FAILED   : The semaphore could not be taken.
 */
ISI_Return ISID_lockGChats(void)
{
    /* Lock both the services and calls database */
    if (0 == ISI_semAcquire(_ISID_Services.mutex)) {
        return (ISI_RETURN_FAILED);
    }
    if (0 == ISI_semAcquire(_ISID_GChats.mutex)) {
        ISI_semGive(_ISID_Services.mutex);
        return (ISI_RETURN_FAILED);
    }
    return (ISI_RETURN_OK);
}

/*
 * ======== ISID_unlockCalls ========
 * This function is used to Give a semaphore back to the
 * linked list (database) used to manage "calls".
 * Note, how the "services" semaphore is also given back.
 *
 * Returns:
 *  Nothing
 */
void ISID_unlockCalls(void)
{
    ISI_semGive(_ISID_Services.mutex);
    ISI_semGive(_ISID_Calls.mutex);
    ISI_semGive(_ISID_Confs.mutex);
}

/*
 * ======== ISID_unlockGChats ========
 * This function is used to Give a semaphore back to the
 * linked list (database) used to manage group chat rooms.
 * Note, how the "services" semaphore is also given back.
 *
 * Returns:
 *  Nothing
 */
void ISID_unlockGChats(void)
{
    ISI_semGive(_ISID_Services.mutex);
    ISI_semGive(_ISID_GChats.mutex);
}

/*
 * ======== ISID_lockTexts ========
 * This function is used to take a semaphore against the
 * linked list (database) used to manage text messages (instant messages).
 * Note that to successfully use a mutex on "text messages" the "chats" &
 * "services" mutexes must also be taken.
 *
 * Returns:
 *  ISI_RETURN_OK       : The semaphore was successfully taken.
 *  ISI_RETURN_FAILED   : The semaphore could not be taken.
 */
ISI_Return ISID_lockTexts(void)
{
    /* Lock the services, calls, and texts database */
    if (0 == ISI_semAcquire(_ISID_Services.mutex)) {
        return (ISI_RETURN_FAILED);
    }
    if (0 == ISI_semAcquire(_ISID_GChats.mutex)) {
        ISI_semGive(_ISID_Services.mutex);
        return (ISI_RETURN_FAILED);
    }
    if (0 == ISI_semAcquire(_ISID_Texts.mutex)) {
        ISI_semGive(_ISID_GChats.mutex);
        ISI_semGive(_ISID_Services.mutex);
        return (ISI_RETURN_FAILED);
    }
    return (ISI_RETURN_OK);
}

/*
 * ======== ISID_lockTexts ========
 * This function is used to take a semaphore against the
 * linked list (database) used to manage file transfers.
 * Note that to successfully use a mutex on "files" the "chats" &
 * "services" mutexes must also be taken.
 *
 * Returns:
 *  ISI_RETURN_OK       : The semaphore was successfully taken.
 *  ISI_RETURN_FAILED   : The semaphore could not be taken.
 */
ISI_Return ISID_lockFiles(void)
{
    /* Lock the services, calls, and texts database */
    if (0 == ISI_semAcquire(_ISID_Services.mutex)) {
        return (ISI_RETURN_FAILED);
    }
    if (0 == ISI_semAcquire(_ISID_GChats.mutex)) {
        ISI_semGive(_ISID_Services.mutex);
        return (ISI_RETURN_FAILED);
    }
    if (0 == ISI_semAcquire(_ISID_Files.mutex)) {
        ISI_semGive(_ISID_GChats.mutex);
        ISI_semGive(_ISID_Services.mutex);
        return (ISI_RETURN_FAILED);
    }
    return (ISI_RETURN_OK);
}

/*
 * ======== ISID_unlockTexts ========
 * This function is used to Give a semaphore back to the linked list
 * (database) used to manage text messages. Note, how the "services"
 * and "calls" semaphores are also given back.
 *
 * Returns:
 *  Nothing
 */
void ISID_unlockTexts(void)
{
    ISI_semGive(_ISID_Services.mutex);
    ISI_semGive(_ISID_GChats.mutex);
    ISI_semGive(_ISID_Texts.mutex);
}

/*
 * ======== ISID_unlockFiles ========
 * This function is used to Give a semaphore back to the linked list
 * (database) used to manage file transfers. Note, how the "services"
 * and "GChats" semaphores are also given back.
 *
 * Returns:
 *  Nothing
 */
void ISID_unlockFiles(void)
{
    ISI_semGive(_ISID_Services.mutex);
    ISI_semGive(_ISID_GChats.mutex);
    ISI_semGive(_ISID_Files.mutex);
}

/*
 * ======== ISID_lockConfs ========
 * This function is used to take a semaphore against the
 * linked list (database) used to manage conferenced phone calls.
 * Note that to successfully use a mutex on a conferences call the "calls" &
 * "services" mutexes must also be taken.
 *
 * Returns:
 *  ISI_RETURN_OK       : The semaphore was successfully taken.
 *  ISI_RETURN_FAILED   : The semaphore could not be taken.
 */
ISI_Return ISID_lockConfs(void)
{
    /* Lock the services, calls, and texts database */
    if (0 == ISI_semAcquire(_ISID_Services.mutex)) {
        return (ISI_RETURN_FAILED);
    }
    if (0 == ISI_semAcquire(_ISID_Calls.mutex)) {
        ISI_semGive(_ISID_Services.mutex);
        return (ISI_RETURN_FAILED);
    }
    if (0 == ISI_semAcquire(_ISID_Confs.mutex)) {
        ISI_semGive(_ISID_Calls.mutex);
        ISI_semGive(_ISID_Services.mutex);
        return (ISI_RETURN_FAILED);
    }
    return (ISI_RETURN_OK);
}

/*
 * ======== ISID_unlockConfs ========
 * This function is used to Give a semaphore back to the linked list
 * (database) used to manage conferenced calls. Note, how the "services"
 * and "calls" semaphores are also given back.
 *
 * Returns:
 *  Nothing
 */
void ISID_unlockConfs(void)
{
    ISI_semGive(_ISID_Services.mutex);
    ISI_semGive(_ISID_Calls.mutex);
    ISI_semGive(_ISID_Confs.mutex);
}

/*
 * ======== ISID_lockPres ========
 * This function is used to take a semaphore against the linked list
 * (database) used to manage presence transactions. Note that to successfully
 * use a mutex on a presence transactions the "services" mutex must also be
 * taken as well as the "chats" mtuex.
 *
 * Returns:
 *  ISI_RETURN_OK       : The semaphore was successfully taken.
 *  ISI_RETURN_FAILED   : The semaphore could not be taken.
 */
ISI_Return ISID_lockPres(void)
{
    /* Lock the services, calls, and texts database */
    if (0 == ISI_semAcquire(_ISID_Services.mutex)) {
        return (ISI_RETURN_FAILED);
    }
    if (0 == ISI_semAcquire(_ISID_GChats.mutex)) {
        ISI_semGive(_ISID_Services.mutex);
        return (ISI_RETURN_FAILED);
    }
    if (0 == ISI_semAcquire(_ISID_Presence.mutex)) {
        ISI_semGive(_ISID_GChats.mutex);
        ISI_semGive(_ISID_Services.mutex);
        return (ISI_RETURN_FAILED);
    }
    return (ISI_RETURN_OK);
}

/*
 * ======== ISID_unlockPres ========
 * This function is used to Give a semaphore back to the linked list
 * (database) used to manage presence transactions. Note, how the "services"
 * semaphore is also given back.
 *
 * Returns:
 *  Nothing
 */
void ISID_unlockPres(void)
{
    ISI_semGive(_ISID_Services.mutex);
    ISI_semGive(_ISID_GChats.mutex);
    ISI_semGive(_ISID_Presence.mutex);
}

/*
 * ======== ISID_lockEvts ========
 * This function is used to take a semaphore against the linked list
 * (database) used to manage event transactions such as those used
 * to signal DTMF digits. Note that to successfully use a mutex on a
 * an event the "calls" & "services" mutexes must also be taken.
 *
 * Returns:
 *  ISI_RETURN_OK       : The semaphore was successfully taken.
 *  ISI_RETURN_FAILED   : The semaphore could not be taken.
 */
ISI_Return ISID_lockEvts(void)
{
    /* Lock the services, calls, and texts database */
    if (0 == ISI_semAcquire(_ISID_Services.mutex)) {
        return (ISI_RETURN_FAILED);
    }
    if (0 == ISI_semAcquire(_ISID_Calls.mutex)) {
        ISI_semGive(_ISID_Services.mutex);
        return (ISI_RETURN_FAILED);
    }
    if (0 == ISI_semAcquire(_ISID_Evts.mutex)) {
        ISI_semGive(_ISID_Calls.mutex);
        ISI_semGive(_ISID_Services.mutex);
        return (ISI_RETURN_FAILED);
    }
    return (ISI_RETURN_OK);
}

/*
 * ======== ISID_unlockEvts ========
 * This function is used to Give a semaphore back to the linked list
 * (database) used to manage generic events. Note, how the "services"
 * and "calls" semaphores are also given back.
 *
 * Returns:
 *  Nothing
 */
void ISID_unlockEvts(void)
{
    ISI_semGive(_ISID_Services.mutex);
    ISI_semGive(_ISID_Calls.mutex);
    ISI_semGive(_ISID_Evts.mutex);
}

/*
 * ======== ISID_textCreate ========
 * This function is used to create/initialize as object used to manage
 * a text message.  The object returned is then typically inserted into
 * the text message linked list (database).
 *
 * Returns:
 *  ISID_TextId*    : A pointer to a new ISID_TextId object
 *  NULL            : No object could be allocated or initialized.
 */
ISID_TextId *ISID_textCreate(void)
{
    ISID_TextId *t_ptr;
    t_ptr = ISI_alloc(sizeof(ISID_TextId), ISI_OBJECT_TEXT_ID);
    if (t_ptr == NULL) {
        return (NULL);
    }
    else {
        ISID_textClean(t_ptr);
        return (t_ptr);
    }
}

/*
 * ======== ISID_fileCreate ========
 * This function is used to create/initialize as object used to manage
 * a file transfer.  The object returned is then typically inserted into
 * the file linked list (database).
 *
 * Returns:
 *  ISID_TextId*    : A pointer to a new ISID_FileId object
 *  NULL            : No object could be allocated or initialized.
 */
ISID_FileId *ISID_fileCreate(void)
{
    ISID_FileId *f_ptr;
    f_ptr = ISI_alloc(sizeof(ISID_FileId), ISI_OBJECT_FILE_ID);
    if (f_ptr == NULL) {
        return (NULL);
    }
    else {
        ISID_fileClean(f_ptr);
        return (f_ptr);
    }
}

/*
 * ======== ISID_textAdd ========
 * This function is used to add (insert) a ISID_TextId object into the
 * linked list (database) used to manage text messages.
 *
 * text_ptr : A pointer to the object to add (insert)
 *
 * id_ptr : A pointer to an id to use as a unique identifier for this
 *          particular entry.  If the de-reference value is '0', then the
 *          ISID will generate the unique identifier and write it to id_ptr.
 *
 * Returns:
 *  ISI_RETURN_OK       : The entry was successfully inserted into the
 *                        linked list.
 *  ISI_RETURN_FAILED   : Could not insert the object, the list is full.
 */
ISI_Return ISID_textAdd(
    ISID_TextId *text_ptr,
    ISI_Id      *id_ptr)
{
    return _ISID_entryAdd(&text_ptr->e, &_ISID_Texts, id_ptr, ISID_MAX_NUM_MSGS);
}

/*
 * ======== ISID_fileAdd ========
 * This function is used to add (insert) a ISID_FileId object into the
 * linked list (database) used to manage file transfers.
 *
 * file_ptr : A pointer to the object to add (insert)
 *
 * id_ptr : A pointer to an id to use as a unique identifier for this
 *          particular entry.  If the de-reference value is '0', then the
 *          ISID will generate the unique identifier and write it to id_ptr.
 *
 * Returns:
 *  ISI_RETURN_OK       : The entry was successfully inserted into the
 *                        linked list.
 *  ISI_RETURN_FAILED   : Could not insert the object, the list is full.
 */
ISI_Return ISID_fileAdd(
    ISID_FileId *file_ptr,
    ISI_Id      *id_ptr)
{
    return _ISID_entryAdd(&file_ptr->e, &_ISID_Files, id_ptr, ISID_MAX_NUM_FILES);
}

/*
 * ======== ISID_textDestroy ========
 * This function is used to remove and free a ISID_TextId object from the
 * linked list (database) used to manage text messages. First the object
 * is searched for via the textId, then it is removed from the list and
 * then the object is "freed" back to the heap.
 *
 * textId : The identifier used to id the particular entry to remove from
 *          the linked list (database).
 *
 * Returns:
 *  ISI_RETURN_OK     : The object was successfully removed and freed
 *  ISI_RETURN_FAILED : Could not remove the object, the object was not found.
 */
ISI_Return ISID_textDestroy(
    ISI_Id textId)
{
    return _ISID_entryDestroy(textId, &_ISID_Texts, ISI_OBJECT_TEXT_ID);
}

/*
 * ======== ISID_fileDestroy ========
 * This function is used to remove and free a ISID_FileId object from the
 * linked list (database) used to manage file transfers. First the object
 * is searched for via the fileId, then it is removed from the list and
 * then the object is "freed" back to the heap.
 *
 * textId : The identifier used to id the particular entry to remove from
 *          the linked list (database).
 *
 * Returns:
 *  ISI_RETURN_OK     : The object was successfully removed and freed
 *  ISI_RETURN_FAILED : Could not remove the object, the object was not found.
 */
ISI_Return ISID_fileDestroy(
    ISI_Id fileId)
{
    return _ISID_entryDestroy(fileId, &_ISID_Files, ISI_OBJECT_FILE_ID);
}

/*
 * ======== ISID_textGet ========
 * This function is used to get (or retrieve) a ISID_TextId object from the
 * linked list (database) used to manage text messages.
 *
 * textId : An identifier representing the id of the particular ISID_TextId
 *          object to retrieve.
 *
 * text_ptr : An address to a pointer to a ISID_TextId object.  If this
 *           function successfully finds the ISID_TextId object specified by
 *           the 'textId' parameter, the function will write the pointer to
 *           the corresponding ISID_TextId object here.
 *
 * Returns:
 *  ISI_RETURN_OK       : The entry was successfully found.
 *  ISI_RETURN_FAILED   : Could not find the object.
 */
ISI_Return ISID_textGet(
    ISI_Id        textId,
    ISID_TextId **text_ptr)
{
    ISID_Entry *e_ptr;

    if (_ISID_entryGet(textId, 1, &_ISID_Texts, &e_ptr) != ISI_RETURN_FAILED) {
        *text_ptr = (ISID_TextId*)e_ptr;
        return (ISI_RETURN_OK);
    }
    return (ISI_RETURN_FAILED);
}

/*
 * ======== ISID_fileGet ========
 * This function is used to get (or retrieve) a ISID_FileId object from the
 * linked list (database) used to manage file transfers.
 *
 * textId : An identifier representing the id of the particular ISID_FileId
 *          object to retrieve.
 *
 * file_ptr : An address to a pointer to a ISID_FileId object.  If this
 *           function successfully finds the ISID_FileId object specified by
 *           the 'fileId' parameter, the function will write the pointer to
 *           the corresponding ISID_FileId object here.
 *
 * Returns:
 *  ISI_RETURN_OK       : The entry was successfully found.
 *  ISI_RETURN_FAILED   : Could not find the object.
 */
ISI_Return ISID_fileGet(
    ISI_Id        fileId,
    ISID_FileId **file_ptr)
{
    ISID_Entry *e_ptr;

    if (_ISID_entryGet(fileId, 1, &_ISID_Files, &e_ptr) != ISI_RETURN_FAILED) {
        *file_ptr = (ISID_FileId*)e_ptr;
        return (ISI_RETURN_OK);
    }
    return (ISI_RETURN_FAILED);
}

ISI_Id ISID_fileGetFirstByServiceId(
    ISID_ServiceId   *service_ptr,
    ISID_FileId     **file_ptr)
{
    ISID_FileId *f_ptr;
    f_ptr = (ISID_FileId*)_ISID_Files.head_ptr;
    while (f_ptr) {
        if (f_ptr->service_ptr && f_ptr->service_ptr == service_ptr) {
            *file_ptr = f_ptr;
            return (ISI_RETURN_OK);
        }
        f_ptr = (ISID_FileId*)f_ptr->e.next_ptr;
    }
    return (ISI_RETURN_FAILED);
}

ISI_Return ISID_fileGetFirst(
    ISID_FileId **file_ptr)
{
    ISID_Entry *e_ptr;
    e_ptr = _ISID_Files.head_ptr;
    if (e_ptr) {
        *file_ptr = (ISID_FileId*)e_ptr;
        return (ISI_RETURN_OK);
    }
    return (ISI_RETURN_FAILED);
}

/*
 * ======== ISID_textGetFirst ========
 * This function is used to get (or retrieve) the first ISID_TextId object in
 * the linked list (database) used to manage text messages.  In other words,
 * it pops the "head" of the list.
 *
 * text_ptr : An address to a pointer to a ISID_TextId object. The function
 *            will write the pointer to the "head" entry here it is
 *            successful.
 *
 * Returns:
 *  ISI_RETURN_OK       : The "head" entry was returned.
 *  ISI_RETURN_FAILED   : The list is empty.
 */
ISI_Return ISID_textGetFirst(
    ISID_TextId **text_ptr)
{
    ISID_Entry *e_ptr;
    e_ptr = _ISID_Texts.head_ptr;
    if (e_ptr) {
        *text_ptr = (ISID_TextId*)e_ptr;
        return (ISI_RETURN_OK);
    }
    return (ISI_RETURN_FAILED);
}

ISI_Id ISID_textGetFirstByServiceId(
    ISID_ServiceId   *service_ptr,
    ISID_TextId     **text_ptr)
{
    ISID_TextId *t_ptr;
    t_ptr = (ISID_TextId*)_ISID_Texts.head_ptr;
    while (t_ptr) {
        if (t_ptr->service_ptr && t_ptr->service_ptr == service_ptr) {
            *text_ptr = t_ptr;
            return (ISI_RETURN_OK);
        }
        t_ptr = (ISID_TextId*)t_ptr->e.next_ptr;
    }
    return (ISI_RETURN_FAILED);
}

/*
 * ======== ISID_textGetViaServiceIdAndState ========
 * This function is used to get (or retrieve) text messages that belong
 * to a particular "service" and are in a particular "state". This function
 * will search the linked list (database) used to manage text messages and
 * find the next text message that belongs to the service specified in
 * service_ptr.  Additionally the function will only look for text messages
 * that are in the state specified in "state".
 *
 * service_ptr :  A pointer to a ISID_ServiceId object that represents the
 *                service to look within for the next text message.
 *
 * text_ptr : An address to a pointer to a ISID_TextId object. The function
 *            will write the pointer to the text message entry here it is
 *            successful.
 *
 * state : The state of a text message to search for.  Possible values are:
 *         CAD_TEXT_STATE_ACTIVE, ISID_TEXT_STATE_WAITING.
 *
 * Returns:
 *  ISI_RETURN_OK       : A text message entry was found and returned.
 *  ISI_RETURN_FAILED   : Could not find any text messages for this service
 *                        and state.
 */
ISI_Return ISID_textGetViaServiceIdAndState(
    ISID_ServiceId   *service_ptr,
    ISID_TextId     **text_ptr,
    ISID_TextState    state)
{
    ISID_TextId *t_ptr;

    t_ptr = (ISID_TextId*)_ISID_Texts.head_ptr;
    while (t_ptr) {
        if (t_ptr->service_ptr && t_ptr->service_ptr == service_ptr) {
            /* found it */
            if (t_ptr->state == state) {
                *text_ptr = t_ptr;
                return (ISI_RETURN_OK);
            }
        }
        t_ptr = (ISID_TextId*)t_ptr->e.next_ptr;
    }
    return (ISI_RETURN_FAILED);
}

/*
 * ======== ISID_evtCreate ========
 * This function is used to create/initialize as object used to manage
 * generic events.  The object returned is then typically inserted into
 * the event linked list (database).
 *
 * Returns:
 *  ISID_EvtId*  : A pointer to a new ISID_EvtId object
 *  NULL         : No object could be allocated or initialized.
 */
ISID_EvtId *ISID_evtCreate(void)
{
    ISID_EvtId *e_ptr;
    e_ptr = ISI_alloc(sizeof(ISID_EvtId), ISI_OBJECT_EVT_ID);
    if (e_ptr == NULL) {
        return (NULL);
    }
    else {
        _ISID_evtClean(e_ptr);
        return (e_ptr);
    }
}

/*
 * ======== ISID_evtAdd ========
 * This function is used to add (insert) a ISID_EvtId object into the
 * linked list (database) used to manage events.
 *
 * evt_ptr : A pointer to the object to add (insert)
 *
 * id_ptr : A pointer to an id to use as a unique identifier for this
 *          particular entry.  If the de-referenced value is '0', then the
 *          ISID will generate the unique identifier and write it to id_ptr.
 *
 * Returns:
 *  ISI_RETURN_OK       : The entry was successfully inserted into the
 *                        linked list.
 *  ISI_RETURN_FAILED   : Could not insert the object, the list is full.
 */
ISI_Return ISID_evtAdd(
    ISID_EvtId *evt_ptr,
    ISI_Id     *id_ptr)
{
    return _ISID_entryAdd(&evt_ptr->e, &_ISID_Evts, id_ptr, ISID_MAX_NUM_EVTS);
}

/*
 * ======== ISID_evtDestroy ========
 * This function is used to remove and free a ISID_EvtId object from the
 * linked list (database) used to manage events. First the object
 * is searched for via the evtId, then it is removed from the list and
 * then the object is "freed" back to the heap.
 *
 * evtId : The identifier used to id the particular entry to remove from
 *         the linked list (database).
 *
 * Returns:
 *  ISI_RETURN_OK     : The object was successfully removed and freed
 *  ISI_RETURN_FAILED : Could not remove the object, the object was not found.
 */
ISI_Return ISID_evtDestroy(
    ISI_Id evtId)
{
    return _ISID_entryDestroy(evtId, &_ISID_Evts, ISI_OBJECT_EVT_ID);
}

/*
 * ======== ISID_evtGet ========
 * This function is used to get (or retrieve) a ISID_EvtId object from the
 * linked list (database) used to manage events.
 *
 * evtId : An identifier representing the id of the particular ISID_EvtId
 *          object to retrieve.
 *
 * evt_ptr : An address to a pointer to a ISID_EvtId object.  If this
 *           function successfully finds the ISID_EvtId object specified by
 *           the 'evtId' parameter, the function will write the pointer to
 *           the corresponding ISID_EvtId object here.
 *
 * Returns:
 *  ISI_RETURN_OK       : The entry was successfully found.
 *  ISI_RETURN_FAILED   : Could not find the object.
 */
ISI_Return ISID_evtGet(
    ISI_Id       evtId,
    ISID_EvtId **evt_ptr)
{
    ISID_Entry *e_ptr;

    if (_ISID_entryGet(evtId, 1, &_ISID_Evts, &e_ptr) != ISI_RETURN_FAILED) {
        *evt_ptr = (ISID_EvtId*)e_ptr;
        return (ISI_RETURN_OK);
    }
    return (ISI_RETURN_FAILED);
}

/*
 * ======== ISID_evtGetFirst ========
 * This function is used to get (or retrieve) the first ISID_EvtId object
 * in the linked list (database) used to manage events.
 * In other words, it pops the "head" of the list.
 *
 * evt_ptr : An address to a pointer to a ISID_EvtId object. The function
 *          will write the pointer to the "head" entry here if it is
 *          successful.
 *
 * Returns:
 *  ISI_RETURN_OK       : The "head" entry was returned.
 *  ISI_RETURN_FAILED   : The list is empty.
 */
ISI_Return ISID_evtGetFirst(
    ISID_EvtId **evt_ptr)
{
    ISID_Entry *e_ptr;
    e_ptr = _ISID_Evts.head_ptr;
    if (e_ptr) {
        *evt_ptr = (ISID_EvtId*)e_ptr;
        return (ISI_RETURN_OK);
    }
    return (ISI_RETURN_FAILED);
}

ISI_Return ISID_evtGetFirstByServiceId(
    ISID_ServiceId   *service_ptr,
    ISID_EvtId      **evt_ptr)
{
    ISID_EvtId *e_ptr;
    e_ptr = (ISID_EvtId*)_ISID_Evts.head_ptr;
    while (e_ptr) {
        if (e_ptr->service_ptr && e_ptr->service_ptr == service_ptr) {
            *evt_ptr = e_ptr;
            return (ISI_RETURN_OK);
        }
        e_ptr = (ISID_EvtId*)e_ptr->e.next_ptr;
    }
    return (ISI_RETURN_FAILED);
}

/*
 * ======== ISID_evtGetViaCallIdAndState ========
 * This function is used to get (or retrieve) events that belong to a
 * particular "call" and are in a particular "state". This function will search
 * the linked list (database) used to manage events and find the next event that
 * belongs to the call specified in call_ptr.  Additionally the function will
 * only look for events that are in the state specified in "state".
 *
 * call_ptr :  A pointer to a ISID_CallId object that represents the
 *             service to look within for the next telephone event.
 *
 * evt_ptr : An address to a pointer to a ISID_EvtId object. The function
 *            will write the pointer to the event entry here it is
 *            successful.
 *
 * state : The state of an event to search for.  Possible values are:
 *         ISID_EVENT_STATE_ACTIVE.
 *
 * Returns:
 *  ISI_RETURN_OK       : An event entry was found and returned.
 *  ISI_RETURN_FAILED   : Could not find any events for this call and state.
 */
ISI_Return ISID_evtGetViaCallIdAndState(
    ISID_CallId  *call_ptr,
    ISID_EvtId  **evt_ptr,
    ISID_EvtState state)
{
    ISID_EvtId *e_ptr;

    e_ptr = (ISID_EvtId*)_ISID_Evts.head_ptr;
    while (e_ptr) {
        if (e_ptr->call_ptr == call_ptr) {
            /* found it */
            if (e_ptr->state == state) {
                *evt_ptr = e_ptr;
                return (ISI_RETURN_OK);
            }
        }
        e_ptr = (ISID_EvtId*)e_ptr->e.next_ptr;
    }
    return (ISI_RETURN_FAILED);
}

/*
 * ======== ISID_evtGetViaServiceIdAndState ========
 * This function is used to get (or retrieve) events that belong to a particular
 * "service" and are in a particular "state". This function will search the
 * linked list (database) used to manage events and find the next event that b
 * elongs to the service specified in service_ptr.  Additionally the function
 * will only look for events that are in the state specified in "state".
 *
 * service_ptr :  A pointer to a ISID_ServiceId object that represents the
 *                service to look within for the next event.
 *
 * evt_ptr : An address to a pointer to a ISID_EvtId object. The function
 *           will write the pointer to the event entry here if it is successful.
 *
 * state : The state of a event to search for.  Possible values are:
 *         ISID_EVENT_STATE_ACTIVE.
 *
 * Returns:
 *  ISI_RETURN_OK       : An event entry was found and returned.
 *  ISI_RETURN_FAILED   : Could not find any events for this service and state.
 */
ISI_Return ISID_evtGetViaServiceIdAndState(
    ISID_ServiceId   *service_ptr,
    ISID_EvtId      **evt_ptr,
    ISID_EvtState     state)
{
    ISID_EvtId *e_ptr;

    e_ptr = (ISID_EvtId*)_ISID_Evts.head_ptr;
    while (e_ptr) {
        if (NULL == e_ptr->call_ptr && e_ptr->service_ptr == service_ptr) {
            /* found it */
            if (e_ptr->state == state) {
                *evt_ptr = e_ptr;
                return (ISI_RETURN_OK);
            }
        }
        e_ptr = (ISID_EvtId*)e_ptr->e.next_ptr;
    }
    return (ISI_RETURN_FAILED);
}

/*
 * ======== ISID_presCreate ========
 * This function is used to create/initialize as object used to manage
 * a presence transaction.  The object returned is then typically inserted into
 * the presence linked list (database).
 *
 * Returns:
 *  ISID_PresId*  : A pointer to a new ISID_PresId object
 *  NULL          : No object could be allocated or initialized.
 */
ISID_PresId *ISID_presCreate(void)
{
    ISID_PresId *p_ptr;
    p_ptr = ISI_alloc(sizeof(ISID_PresId), ISI_OBJECT_PRES_ID);
    if (p_ptr == NULL) {
        return (NULL);
    }
    else {
        _ISID_presClean(p_ptr);
        return (p_ptr);
    }
}

/*
 * ======== ISID_PresDestroy ========
 * This function is used to remove and free a ISID_PresId object from the
 * linked list (database) used to manage telephone events. First the object
 * is searched for via the presId, then it is removed from the list and
 * then the object is "freed" back to the heap.
 *
 * presId : The identifier used to id the particular entry to remove from
 *         the linked list (database).
 *
 * Returns:
 *  ISI_RETURN_OK     : The object was successfully removed and freed
 *  ISI_RETURN_FAILED : Could not remove the object, the object was not found.
 */
ISI_Return ISID_presDestroy(
    ISI_Id presId)
{
    return _ISID_entryDestroy(presId, &_ISID_Presence, ISI_OBJECT_PRES_ID);
}

/*
 * ======== ISID_presGet ========
 * This function is used to get (or retrieve) a ISID_PresId object from the
 * linked list (database) used to manage presence transactions.
 *
 * presId : An identifier representing the id of the particular ISID_PresId
 *          object to retrieve.
 *
 * text_ptr : An address to a pointer to a ISID_PresId object.  If this
 *           function successfully finds the ISID_PresId object specified by
 *           the 'presId' parameter, the function will write the pointer to
 *           the corresponding ISID_PresId object here.
 *
 * Returns:
 *  ISI_RETURN_OK       : The entry was successfully found.
 *  ISI_RETURN_FAILED   : Could not find the object.
 */
ISI_Return ISID_presGet(
    ISI_Id        presId,
    ISID_PresId **pres_ptr)
{
    ISID_Entry *e_ptr;

    if (_ISID_entryGet(presId, 1, &_ISID_Presence, &e_ptr) !=
                ISI_RETURN_FAILED) {
        *pres_ptr = (ISID_PresId*)e_ptr;
        return (ISI_RETURN_OK);
    }
    return (ISI_RETURN_FAILED);
}

ISI_Return ISID_presGetFirstByServiceId(
    ISID_ServiceId   *service_ptr,
    ISID_PresId     **pres_ptr)
{
    ISID_PresId *p_ptr;

    p_ptr = (ISID_PresId*)_ISID_Presence.head_ptr;
    while (p_ptr) {
        if (p_ptr->service_ptr && p_ptr->service_ptr == service_ptr) {
            *pres_ptr = p_ptr;
            return (ISI_RETURN_OK);
        }
        p_ptr = (ISID_PresId*)p_ptr->e.next_ptr;
    }
    return (ISI_RETURN_FAILED);
}

/*
 * ======== ISID_presGetViaServiceIdAndState ========
 * This function is used to get (or retrieve) presence transactions that
 * belong to a particular "service" and are in a particular "state". This
 * function will search the linked list (database) used to manage presence
 * transactions and find the next presence transaction that belongs to the
 * service specified in service_ptr.  Additionally the function will only look
 * for presence transactions that are in the state specified in "state".
 *
 * service_ptr :  A pointer to a ISID_ServiceId object that represents the
 *                service to look within for the next presence transaction.
 *
 * pres_ptr : An address to a pointer to a ISID_PresId object. The function
 *            will write the pointer to the presence entry here if it is
 *            successful.
 *
 * state : The state of a presence transaction to search for.  Possible values
 *         are:  ISID_PRES_STATE_ACTIVE, ISID_PRES_STATE_WAITING.
 *
 * Returns:
 *  ISI_RETURN_OK       : A presence entry was found and returned.
 *  ISI_RETURN_FAILED   : Could not find any presence transactions for this
 *                        service and state.
 */
ISI_Return ISID_presGetViaServiceIdAndState(
    ISID_ServiceId   *service_ptr,
    ISID_PresId     **pres_ptr,
    ISID_PresState    state)
{
    ISID_PresId *p_ptr;

    p_ptr = (ISID_PresId*)_ISID_Presence.head_ptr;
    while (p_ptr) {
        if (p_ptr->service_ptr && p_ptr->service_ptr == service_ptr) {
            /* found it */
            if (p_ptr->state == state) {
                *pres_ptr = p_ptr;
                return (ISI_RETURN_OK);
            }
        }
        p_ptr = (ISID_PresId*)p_ptr->e.next_ptr;
    }
    return (ISI_RETURN_FAILED);
}

/*
 * ======== ISID_presAdd ========
 * This function is used to add (insert) a ISID_PresId object into the
 * linked list (database) used to manage presence transactions.
 *
 * pres_ptr : A pointer to the object to add (insert)
 *
 * id_ptr : A pointer to an id to use as a unique identifier for this
 *          particular entry.  If the de-referenced value is '0', then the
 *          ISID will generate the unique identifier and write it to id_ptr.
 *
 * Returns:
 *  ISI_RETURN_OK       : The entry was successfully inserted into the
 *                        linked list.
 *  ISI_RETURN_FAILED   : Could not insert the object, the list is full.
 */
ISI_Return ISID_presAdd(
    ISID_PresId *pres_ptr,
    ISI_Id      *id_ptr)
{
    return _ISID_entryAdd(&pres_ptr->e, &_ISID_Presence, id_ptr,
            ISID_MAX_NUM_PRESENCE);
}

/*
 * ======== ISID_presAddFirst ========
 * This function is used to add (insert) a ISID_PresId object into the
 * linked list (database) used to manage presence transactions.
 *
 * pres_ptr : A pointer to the object to add (insert)
 *
 * id_ptr : A pointer to an id to use as a unique identifier for this
 *          particular entry.  If the de-referenced value is '0', then the
 *          ISID will generate the unique identifier and write it to id_ptr.
 *
 * Returns:
 *  ISI_RETURN_OK       : The entry was successfully inserted into the
 *                        linked list.
 *  ISI_RETURN_FAILED   : Could not insert the object, the list is full.
 */
ISI_Return ISID_presAddFirst(
    ISID_PresId *pres_ptr,
    ISI_Id      *id_ptr)
{
    return _ISID_entryAddFirst(&pres_ptr->e, &_ISID_Presence, id_ptr,
            ISID_MAX_NUM_PRESENCE);
}

/*
 * ======== ISID_presGetFirst ========
 * This function is used to get (or retrieve) the first ISID_PresId object
 * in the linked list (database) used to manage presence status events.
 * In other words, it pops the "head" of the list.
 *
 * pres_ptr : An address to a pointer to a ISID_PresId object. The function
 *          will write the pointer to the "head" entry here if it is
 *          successful.
 *
 * Returns:
 *  ISI_RETURN_OK       : The "head" entry was returned.
 *  ISI_RETURN_FAILED   : The list is empty.
 */
ISI_Return ISID_presGetFirst(
    ISID_PresId **pres_ptr)
{
    ISID_Entry *e_ptr;
    e_ptr = _ISID_Presence.head_ptr;
    if (e_ptr) {
        *pres_ptr = (ISID_PresId*)e_ptr;
        return (ISI_RETURN_OK);
    }
    return (ISI_RETURN_FAILED);
}

/*
 * ======== ISID_confCreate ========
 * This function is used to create/initialize as object used to manage
 * conferenced calls.  The object returned is then typically inserted into
 * the conference linked list (database).
 *
 * Returns:
 *  ISID_ConfId*  : A pointer to a new ISID_ConfId object
 *  NULL          : No object could be allocated or initialized.
 */
ISID_ConfId *ISID_confCreate(void)
{
    ISID_ConfId *c_ptr;
    c_ptr = ISI_alloc(sizeof(ISID_ConfId), ISI_OBJECT_CONF_ID);
    if (c_ptr == NULL) {
        return (NULL);
    }
    else {
        _ISID_confClean(c_ptr);
        return (c_ptr);
    }
}

/*
 * ======== ISID_confAdd ========
 * This function is used to add (insert) a ISID_ConfId object into the
 * linked list (database) used to manage conferenced phone calls.
 *
 * conf_ptr : A pointer to the object to add (insert)
 *
 * id_ptr : A pointer to an id to use as a unique identifier for this
 *          particular entry.  If the de-referenced value is '0', then the
 *          ISID will generate the unique identifier and write it to id_ptr.
 *
 * Returns:
 *  ISI_RETURN_OK       : The entry was successfully inserted into the
 *                        linked list.
 *  ISI_RETURN_FAILED   : Could not insert the object, the list is full.
 */
ISI_Return ISID_confAdd(
    ISID_ConfId *conf_ptr,
    ISI_Id      *id_ptr)
{
    return _ISID_entryAdd(&conf_ptr->e, &_ISID_Confs, id_ptr,
            ISID_MAX_NUM_CONFS);
}

/*
 * ======== ISID_confDestroy ========
 * This function is used to remove and free a ISID_confId object from the
 * linked list (database) used to manage conferenced calls. First the object
 * is searched for via the confId, then it is removed from the list and
 * then the object is "freed" back to the heap.
 *
 * presId : The identifier used to id the particular entry to remove from
 *         the linked list (database).
 *
 * Returns:
 *  ISI_RETURN_OK     : The object was successfully removed and freed
 *  ISI_RETURN_FAILED : Could not remove the object, the object was not found.
 */
ISI_Return ISID_confDestroy(
    ISI_Id confId)
{
    return _ISID_entryDestroy(confId, &_ISID_Confs, ISI_OBJECT_CONF_ID);
}

/*
 * ======== ISID_confGet ========
 * This function is used to get (or retrieve) a ISID_ConfId object from the
 * linked list (database) used to manage conferenced calls.
 *
 * confId : An identifier representing the id of the particular ISID_ConfId
 *          object to retrieve.
 *
 * conf_ptr : An address to a pointer to a ISID_ConfId object.  If this
 *           function successfully finds the ISID_ConfId object specified by
 *           the 'confId' parameter, the function will write the pointer to
 *           the corresponding ISID_ConfId object here.
 *
 * Returns:
 *  ISI_RETURN_OK       : The entry was successfully found.
 *  ISI_RETURN_FAILED   : Could not find the object.
 */
ISI_Return ISID_confGet(
    ISI_Id       confId,
    ISID_ConfId **conf_ptr)
{
    ISID_Entry *e_ptr;

    if (_ISID_entryGet(confId, 1, &_ISID_Confs, &e_ptr) != ISI_RETURN_FAILED) {
        *conf_ptr = (ISID_ConfId*)e_ptr;
        return (ISI_RETURN_OK);
    }
    return (ISI_RETURN_FAILED);
}

/*
 * ======== ISID_confAddCall ========
 * This function is used to add a call to a "conference". ISID_ConfId objects
 * are used to manage groups of calls (groups of ISID_CallId objects) that
 * are involved in a conference.
 *
 * conf_ptr : A pointer to a ISID_ConfId object. This is the object that
 *            manages the calls involved in a call conference and is created
 *            when a conferenced call is first created.  The call specified in
 *            call_ptr is added to this object.
 *
 * call_ptr : A pointer to a ISID_CallId object.  This is the call that is
 *           being added to the conference.
 *
 * Returns:
 *  ISI_RETURN_OK       : The call was successfully added to the conference.
 *  ISI_RETURN_FAILED   : Could not add the call.  The call conference is
 *                        full and can not accept any more calls.
 */
ISI_Return ISID_confAddCall(
    ISID_ConfId *conf_ptr,
    ISID_CallId *call_ptr)
{
    vint x;
    for (x = 0 ; x < ISI_CONF_USERS_NUM ; x++) {
        if (conf_ptr->aCall[x] == NULL) {
            conf_ptr->aCall[x] = call_ptr;
            /* Currently only support a conf call up to 2 calls */
            if (call_ptr->streamId == 0) {
                conf_ptr->aConfMask[x] = 2;
                call_ptr->confMask = 2;
            }
            else {
                conf_ptr->aConfMask[x] = 1;
                call_ptr->confMask = 1;
            }
            return (ISI_RETURN_OK);
        }
    }
    return (ISI_RETURN_FAILED);
}

/*
 * ======== ISID_confRemoveCall ========
 * This function is used to remove a call from a "conference". ISID_ConfId
 * objects are used to manage groups of calls (groups of ISID_CallId objects)
 * that are involved in a conference. This function will remove any reference
 * to the specified call from the conference.
 *
 * conf_ptr : A pointer to a ISID_ConfId object. This is the object that
 *            manages the calls involved in a call conference and is created
 *            when a conferenced call is first created.  The call specified in
 *            call_ptr will be removed from this object.
 *
 * call_ptr : A pointer to a ISID_CallId object.  This is the call that is
 *           being removed from the conference.
 *
 * Returns:
 *  ISI_RETURN_OK       : The call was successfully removed from the conference.
 *  ISI_RETURN_FAILED   : The call did not exist in the conference.
 *  ISI_RETURN_DONE     : The call was removed and there are now no more calls
 *                        involved in the conference.
 */
ISI_Return ISID_confRemoveCall(
    ISID_ConfId *conf_ptr,
    ISID_CallId *call_ptr,
    vint         clearAll)
{
    ISI_Return ret;
    vint       x;

    ret = ISI_RETURN_OK;
    for (x = 0 ; x < ISI_CONF_USERS_NUM ; x++) {
        if (conf_ptr->aCall[x] == call_ptr) {
            /* Clear the mask */
            conf_ptr->aConfMask[x] = 0;
            call_ptr->confMask = 0;
            if (clearAll) {
                /* Then also clear the call Pointer */
                conf_ptr->aCall[x] = NULL;
            }
            break;
        }
    }
    if (x == ISI_CONF_USERS_NUM) {
        /* Never found it */
        return (ISI_RETURN_FAILED);
    }
    /* If there are no calls left in the conference then return 'done' */
    for (x = 0 ; x < ISI_CONF_USERS_NUM ; x++) {
        if (conf_ptr->aConfMask[x] != 0) {
            break;
        }
    }
    if (x == ISI_CONF_USERS_NUM) {
        /* Then there was no active calls so this conference is done */
        ret = ISI_RETURN_DONE;
    }
    return (ret);
}

/*
 * ======== ISID_serviceCreate ========
 * This function is used to create/initialize as object used to manage
 * "services".  The object returned is then typically inserted into
 * the Service linked list (database).
 *
 * Returns:
 *  ISID_ServiceId* : A pointer to a new ISID_ServiceId object
 *  NULL            : No object could be allocated or initialized.
 */
ISID_ServiceId *ISID_serviceCreate(void)
{
    ISID_ServiceId *service_ptr;
    service_ptr = ISI_alloc(sizeof(ISID_ServiceId), ISI_OBJECT_SERVICE_ID);
    if (service_ptr == NULL) {
        return (NULL);
    }
    else {
        _ISID_serviceClean(service_ptr);
        return (service_ptr);
    }
}

/*
 * ======== ISID_serviceAdd ========
 * This function is used to add (insert) a ISID_ServiceId object into the
 * linked list (database) used to manage "services".
 *
 * service_ptr : A pointer to the object to add (insert)
 *
 * id_ptr : A pointer to an id to use as a unique identifier for this
 *          particular entry.  If the de-referenced value is '0', then the
 *          ISID will generate the unique identifier and write it to id_ptr.
 *
 * Returns:
 *  ISI_RETURN_OK       : The entry was successfully inserted into the
 *                        linked list.
 *  ISI_RETURN_FAILED   : Could not insert the object, the list is full.
 */
ISI_Return ISID_serviceAdd(
    ISID_ServiceId *service_ptr,
    ISI_Id         *id_ptr)
{
    return _ISID_entryAdd(&service_ptr->e, &_ISID_Services, id_ptr, ISID_MAX_NUM_SERVICES);
}

/*
 * ======== ISID_serviceDestroy ========
 * This function is used to remove and free a ISID_serviceId object from the
 * linked list (database) used to manage "services". First the object
 * is searched for via the serviceId, then it is removed from the list and
 * then the object is "freed" back to the heap.  Note, that this function
 * will also free any internally used objects associated with this service
 * that were originally allocated from he heap when the service was first
 * created.
 *
 * serviceId : The identifier used to id the particular entry to remove from
 *         the linked list (database).
 *
 * Returns:
 *  ISI_RETURN_OK     : The object was successfully removed and freed
 *  ISI_RETURN_FAILED : Could not remove the object, the object was not found.
 */
ISI_Return ISID_serviceDestroy(
    ISI_Id serviceId)
{
    ISID_ServiceId *service_ptr;
    ISID_Account   *acct_ptr;
    ISID_Account   *next_ptr;
    ISI_Return      ret;

    /* First free the accounts in the service object */
    ret = ISID_serviceGet(serviceId, &service_ptr);
    if ((ISI_RETURN_OK == ret) || (ISI_RETURN_SERVICE_NOT_ACTIVE == ret)) {
        next_ptr = service_ptr->accounts_ptr;
        while (next_ptr) {
            acct_ptr = next_ptr;
            next_ptr = next_ptr->next_ptr;
            ISI_free(acct_ptr, ISI_OBJECT_ACCOUNT);
        }
    }
    return _ISID_entryDestroy(serviceId, &_ISID_Services, ISI_OBJECT_SERVICE_ID);
}

/*
 * ======== ISID_serviceGet ========
 * This function is used to get (or retrieve) a ISID_ServiceId object from the
 * linked list (database) used to manage "services".
 *
 * serviceId : An identifier representing the id of the particular "service"
 *             to retrieve.
 *
 * service_ptr : An address to a pointer to a ISID_ServiceId object.  If this
 *           function successfully finds the ISID_ServiceId object specified by
 *           the 'serviceId' parameter, the function will write the pointer to
 *           the corresponding ISID_serviceId object here.
 *
 * Return:
 *   ISI_RETURN_FAILED : The service "id" is invalid
 *   ISI_RETURN_SERVICE_NOT_ACTIVE : The service exists but is deactivated
 *   ISI_RETURN_OK : The service exists AND is activated
 */
ISI_Return ISID_serviceGet(
    ISI_Id           serviceId,
    ISID_ServiceId **service_ptr)
{
    ISI_Return      ret;
    ISID_Entry     *e_ptr;
    ISID_ServiceId *s_ptr;

    ret = _ISID_entryGet(serviceId, 1, &_ISID_Services, &e_ptr);
    if (ret == ISI_RETURN_FAILED) {
        return (ISI_RETURN_FAILED);
    }
    else {
        /* Then the entry exists */
        s_ptr = (ISID_ServiceId*)e_ptr;
        *service_ptr = s_ptr;
        if (s_ptr->isActivated) {
            return (ISI_RETURN_OK);
        }
        return (ISI_RETURN_SERVICE_NOT_ACTIVE);
    }
}

/*
 * ======== ISID_serviceGetByProtocol ========
 * Marco
 */
ISI_Return ISID_serviceGetByProtocol(
    vint             protocolId,
    ISID_ServiceId **service_ptr)
{
    ISID_Entry     *e_ptr;
    ISID_ServiceId *s_ptr;

    e_ptr = _ISID_Services.head_ptr;
    while(e_ptr) {
        s_ptr = (ISID_ServiceId*)e_ptr;
        if(s_ptr->protocol == protocolId && s_ptr->isActivated ) {
            *service_ptr = (ISID_ServiceId*)s_ptr;
            return (ISI_RETURN_OK);
        }
        e_ptr = e_ptr->next_ptr;
    }
    return (ISI_RETURN_FAILED);

}




/*
 * ======== ISID_serviceGetFirst ========
 * This function is used to get (or retrieve) the first ISID_ServiceId object
 * in the linked list (database) used to manage "services".
 * In other words, it pops the "head" of the list.
 *
 * service_ptr : An address to a pointer to a ISID_ServiceId object.
 *               The function will write the pointer to the "head"
 *               entry here if it is successful.
 *
 * Returns:
 *  ISI_RETURN_OK       : The "head" entry was returned.
 *  ISI_RETURN_FAILED   : The list is empty.
 */
ISI_Return ISID_serviceGetFirst(
    ISID_ServiceId **service_ptr)
{
    ISID_Entry *e_ptr;
    e_ptr = _ISID_Services.head_ptr;
    if (e_ptr) {
        *service_ptr = (ISID_ServiceId*)e_ptr;
        return (ISI_RETURN_OK);
    }
    return (ISI_RETURN_FAILED);
}

/*
 * ======== ISID_serviceCheck4Calls ========
 * This function is used to check a service to see if there are any active
 * calls.  If the serviceId is zero ('0'), then the function will return
 * 'OK' if there are any active calls at all for the whole system.
 *
 * serviceId : An identifier presenting the service to check for active calls.
 *             If '0', then we will look for any active call at all in the
 *             whole system.
 *
 * ignoreCall_ptr : A pointer to ISID_CallId object (a call).  If NULL,
 *                 then this function will look for any active calls.  If NOT
 *                 NULL, then this function will look for any active calls
 *                 OTHER THAN the call specified here in ignoreCall_ptr.
 *
 * Returns:
 *  ISI_RETURN_OK       : Indicates there are active calls.
 *  ISI_RETURN_FAILED   : Indicates that there are NO active call for this
 *                        service.
 */
ISI_Return ISID_serviceCheck4Calls(
    ISI_Id       serviceId,
    ISID_CallId *ignoreCall_ptr)
{
    /*
     * If ignoreCall_ptr is NULL then look for any call that belongs to this
     * service.  Otherwise, look for any other call beside the one indicated
     * in ignoreCall_ptr.
     */
    ISID_CallId *curr_ptr;

    curr_ptr = (ISID_CallId*)_ISID_Calls.head_ptr;

    while(curr_ptr) {
        if (ignoreCall_ptr != NULL &&
                ignoreCall_ptr == curr_ptr) {
            /* Ignore this one */
            curr_ptr = (ISID_CallId*)curr_ptr->e.next_ptr;
            continue;
        }
        if (serviceId == 0) {
            /* Then we are looking for any active calls at all in the system */
            return (ISI_RETURN_OK);
        }
        else {
            if (curr_ptr->service_ptr->e.desc == serviceId) {
                /* Found it so there is an active call */
                return (ISI_RETURN_OK);
            }
        }
        curr_ptr = (ISID_CallId*)curr_ptr->e.next_ptr;
    }
    return (ISI_RETURN_FAILED);
}

/*
 * ======== ISID_callCreate ========
 * This function is used to create/initialize an object used to manage
 * a "call".  The object returned is then typically inserted into
 * the Call linked list (database).
 *
 * Please also refer to ISID_callCreateFromXml method which does same thing however has
 * a slightly different api and usually used when initiating outgoing call sessions.
 *
 * Returns:
 *  ISID_CallId* : A pointer to a new ISID_ServiceId object
 *  NULL         : No object could be allocated or initialized.
 */
ISID_CallId *ISID_callCreate(
    ISID_ServiceId       *service_ptr,
    char                 *to_ptr,
    char                 *subject_ptr,
    uint16                callType,
    ISI_SessionCidType    cidType,
    ISI_SessionDirection  audioDirection,
    ISI_SessionDirection  videoDirection)
{
    ISID_CallId *call_ptr;

    call_ptr = ISI_alloc(sizeof(ISID_CallId), ISI_OBJECT_CALL_ID);
    if (call_ptr != NULL) {
        _ISID_callClean(call_ptr);
        /* Load the coder from the service */
        OSAL_memCpy(call_ptr->coders, service_ptr->coders,
                sizeof(ISIP_Coder) * ISI_CODER_NUM);

        /* Now set the call type */
        if ((callType & ISI_SESSION_TYPE_AUDIO) ||
                (callType & ISI_SESSION_TYPE_VIDEO) ||
                (callType & ISI_SESSION_TYPE_CHAT) ||
                (callType & ISI_SESSION_TYPE_EMERGENCY) ||
                (callType & ISI_SESSION_TYPE_CONFERENCE) ||
                (callType & ISI_SESSION_TYPE_SECURITY_AUDIO) ||
                (callType & ISI_SESSION_TYPE_SECURITY_VIDEO)) {
            call_ptr->type = callType;
            call_ptr->audioDir = audioDirection;
            call_ptr->videoDir = videoDirection;
            call_ptr->service_ptr = service_ptr;
            call_ptr->cidType = cidType;
            if (to_ptr) {
                OSAL_strncpy(call_ptr->szRemoteUri, to_ptr, 
                        ISI_ADDRESS_STRING_SZ);
            }
            if (subject_ptr) {
                OSAL_strncpy(call_ptr->szSubject, subject_ptr, 
                        ISI_SUBJECT_STRING_SZ);
            }
            if (_ISID_allocStream(&call_ptr->streamId) == ISI_RETURN_OK) {
                /* Then there was an available stream resource */
                return (call_ptr);
            }
        }
        /* Free the call */
        ISI_free(call_ptr, ISI_OBJECT_CALL_ID);
    }
    return (NULL);
}

/*
 * ======== ISID_callCreateFromXml ========
 * This function is used to create/initialize an object used to manage a "call"
 * using Media Attributes XML doc. The object returned is then typically
 * inserted into the Call linked list (database).
 *
 * Please also refer to ISID_callCreate method which does same thing however has
 * a slightly different api and usually used during incoming call sessions.
 *
 * Returns:
 *  ISID_CallId* : A pointer to a new ISID_ServiceId object
 *  NULL         : No object could be allocated or initialized.
 */
ISID_CallId *ISID_callCreateFromXml(
    ISID_ServiceId       *service_ptr,
    char                 *to_ptr,
    char                 *subject_ptr,
    ISI_SessionCidType    cidType,
    char                 *mediaAttribute_ptr)
{
    ISID_CallId *call_ptr;

    call_ptr = ISI_alloc(sizeof(ISID_CallId), ISI_OBJECT_CALL_ID);
    if (call_ptr != NULL) {
        _ISID_callClean(call_ptr);
        /*
         * Parse the XML pointer and populate the values
         * into a ISID_CallId structure.
         */
        if (ISI_RETURN_OK != ISI_decodeMediaAttributeXMLDoc(
               mediaAttribute_ptr, call_ptr)) {
            /* Free the call */
            ISI_free(call_ptr, ISI_OBJECT_CALL_ID);
            /* Failed to parse the XML string. Return NULL */
            return (NULL);
        }

        /* Load the coder from the service */
        OSAL_memCpy(call_ptr->coders, service_ptr->coders,
                sizeof(ISIP_Coder) * ISI_CODER_NUM);
        /* Validate the call type and set other parameters. */
        if ((call_ptr->type & ISI_SESSION_TYPE_AUDIO) ||
                (call_ptr->type & ISI_SESSION_TYPE_VIDEO) ||
                (call_ptr->type & ISI_SESSION_TYPE_CHAT) ||
                (call_ptr->type & ISI_SESSION_TYPE_EMERGENCY) ||
                (call_ptr->type & ISI_SESSION_TYPE_CONFERENCE) ||
                (call_ptr->type & ISI_SESSION_TYPE_SECURITY_AUDIO) ||
                (call_ptr->type & ISI_SESSION_TYPE_SECURITY_VIDEO)) {
            call_ptr->service_ptr = service_ptr;
            call_ptr->cidType = cidType;
            if (to_ptr) {
                OSAL_strncpy(call_ptr->szRemoteUri, to_ptr, 
                        ISI_ADDRESS_STRING_SZ);
            }
            if (subject_ptr) {
                OSAL_strncpy(call_ptr->szSubject, subject_ptr, 
                        ISI_SUBJECT_STRING_SZ);
            }
            if (_ISID_allocStream(&call_ptr->streamId) == ISI_RETURN_OK) {
                /* Then there was an available stream resource */
                return (call_ptr);
            }
        }
        /* Free the call */
        ISI_free(call_ptr, ISI_OBJECT_CALL_ID);
    }
    return (NULL);
}

/*
 * ======== ISID_gchatCreate ========
 * This function is used to create/initialize an object used to manage
 * a group chat room.  The object returned is then typically inserted into
 * the GChat linked list (database).
 *
 * Returns:
 *  ISID_GChatId* : A pointer to a new ISID_GChatId object
 *  NULL         : No object could be allocated or initialized.
 */
ISID_GChatId *ISID_gchatCreate(
    ISID_ServiceId  *service_ptr,
    char            *remoteAddress_ptr,
    char            *invitor_ptr,
    char            *localIdentity_ptr,
    char            *subject_ptr,
    char            *participants_ptr,
    int              passwordRequired)
{
    ISID_GChatId *chat_ptr;

    // The contributionId may be in the remoteAddress_ptr as a URI parameter
    char *contributionId_ptr = NULL;

    chat_ptr = ISI_alloc(sizeof(ISID_GChatId), ISI_OBJECT_GCHAT_ID);
    if (chat_ptr != NULL) {
        _ISID_gchatClean(chat_ptr);
        chat_ptr->service_ptr = service_ptr;
        if (localIdentity_ptr) {
            OSAL_strncpy(chat_ptr->szLocalIdentity, localIdentity_ptr, ISI_ADDRESS_STRING_SZ);
        }
        if (remoteAddress_ptr) {
            /* Find the contribution ID parameter in the string, if it exists.
             * Look for the position of the semi-colon that precedes the parameter name. */
            contributionId_ptr = OSAL_strnscan(remoteAddress_ptr, ISI_LONG_ADDRESS_STRING_SZ,
                    ";" ISI_CONTRIBUTION_ID_PARAM_STR);

            OSAL_strncpy(chat_ptr->szRemoteAddress, remoteAddress_ptr, ISI_ADDRESS_STRING_SZ);
            if (contributionId_ptr) {
                /* found it, so we'll remove this contribution ID portion
                 * from the remote address by setting the semi-colon to NULL
                 * to terminate the remote address string */
                chat_ptr->szRemoteAddress[(contributionId_ptr - remoteAddress_ptr)] = 0;
            }
        }
        if (contributionId_ptr) {
            /* if a contribution ID exists, get its value by finding the equals sign
             * the value will begin at the location of the pointer +2 (to skip the
             * semi-colon and equals sign) + skip the length of the parameter name */
            OSAL_strncpy(chat_ptr->szContributionId, 
                  contributionId_ptr + 2 + OSAL_strlen(ISI_CONTRIBUTION_ID_PARAM_STR), 
                  ISI_ADDRESS_STRING_SZ);
        }
        else {
            /* initialize to NULL */
            chat_ptr->szContributionId[0] = 0;
        }

        if (invitor_ptr) {
            OSAL_strncpy(chat_ptr->szInvitor, invitor_ptr, ISI_ADDRESS_STRING_SZ);
        }
        if (subject_ptr) {
            OSAL_strncpy(chat_ptr->szSubject, subject_ptr, ISI_SUBJECT_STRING_SZ);
        }
        if (participants_ptr) {
            OSAL_strncpy(chat_ptr->szParticipants, participants_ptr, ISI_LONG_ADDRESS_STRING_SZ);
        }
        chat_ptr->requiresPassword = passwordRequired;
        return (chat_ptr);
    }
    return (NULL);
}

/*
 * ======== ISID_callDestroy ========
 * This function is used to remove and free a ISID_callId object from the
 * linked list (database) used to manage "calls". This is typically called
 * when a phone call is being disconnected. First the object is searched for
 * via the callId, then it is removed from the list and then the object is
 * "freed" back to the heap.  Note, that this function will also check all
 * the conferenced calls and remove that call automatically from the call
 * conference to eliminate any future references to this call.
 *
 * callId : The identifier used to id the particular entry to remove from
 *         the linked list (database).
 *
 * Returns:
 *  ISI_RETURN_OK     : The object was successfully removed and freed
 *  ISI_RETURN_FAILED : Could not remove the object, the object was not found.
 */
ISI_Return ISID_callDestroy(
    ISI_Id callId)
{
    ISID_CallId *call_ptr;

    /* Find the call */
    if (ISID_callGet(callId, &call_ptr) == ISI_RETURN_OK) {
        /* mark the stream as now available */
        _ISID_freeStream (call_ptr->streamId);
        /* remove it from any conferences that may exist */
        _ISID_callRemoveFromAllConf(call_ptr);
    }
    return _ISID_entryDestroy(callId, &_ISID_Calls, ISI_OBJECT_CALL_ID);
}

/*
 * ======== ISID_gchatDestroy ========
 * This function is used to remove and free a ISID_gchatId object from the
 * linked list (database) used to manage group chat rooms. This is typically
 * called when a room is destroyed or this entity is 'leaving' the room.
 * First the object is searched for via the chatId, then it is removed from the
 * list and then the object is "freed" back to the heap.  Note, that this
 * function will also check all the conferenced calls and remove that call
 * automatically from the call conference to eliminate any future references to
 * this call.
 *
 * chatId : The identifier used to id the particular entry to remove from
 *         the linked list (database).
 *
 * Returns:
 *  ISI_RETURN_OK     : The object was successfully removed and freed
 *  ISI_RETURN_FAILED : Could not remove the object, the object was not found.
 */
ISI_Return ISID_gchatDestroy(
    ISI_Id chatId)
{
    return _ISID_entryDestroy(chatId, &_ISID_GChats, ISI_OBJECT_GCHAT_ID);
}

/*
 * ======== ISID_callAdd ========
 * This function is used to add (insert) a ISID_CallId object into the
 * linked list (database) used to manage phone "calls".
 *
 * call_ptr : A pointer to the object to add (insert)
 *
 * id_ptr : A pointer to an id to use as a unique identifier for this
 *          particular entry.  If the de-referenced value is '0', then the
 *          ISID will generate the unique identifier and write it to id_ptr.
 *
 * Returns:
 *  ISI_RETURN_OK       : The entry was successfully inserted into the
 *                        linked list.
 *  ISI_RETURN_FAILED   : Could not insert the object, the list is full.
 */
ISI_Return ISID_callAdd(
    ISID_CallId    *call_ptr,
    ISI_Id         *id_ptr)
{
    return _ISID_entryAdd(&call_ptr->e, &_ISID_Calls, id_ptr, ISID_MAX_NUM_CALLS);
}

/*
 * ======== ISID_gchatAdd ========
 * This function is used to add (insert) a ISID_GChatId object into the
 * linked list (database) used to manage group chat rooms.
 *
 * chat_ptr : A pointer to the object to add (insert)
 *
 * id_ptr : A pointer to an id to use as a unique identifier for this
 *          particular entry.  If the de-referenced value is '0', then the
 *          ISID will generate the unique identifier and write it to id_ptr.
 *
 * Returns:
 *  ISI_RETURN_OK       : The entry was successfully inserted into the
 *                        linked list.
 *  ISI_RETURN_FAILED   : Could not insert the object, the list is full.
 */
ISI_Return ISID_gchatAdd(
    ISID_GChatId   *chat_ptr,
    ISI_Id         *id_ptr)
{
    return _ISID_entryAdd(&chat_ptr->e, &_ISID_GChats, id_ptr, ISID_MAX_NUM_GCHATS);
}

/*
 * ======== ISID_callGet ========
 * This function is used to get (or retrieve) a ISID_CallId object from the
 * linked list (database) used to manage "calls".
 *
 * calId : An identifier representing the id of the particular "call"
 *             to retrieve.
 *
 * call_ptr : An address to a pointer to a ISID_CallId object.  If this
 *           function successfully finds the ISID_callId object specified by
 *           the 'callId' parameter, the function will write the pointer to
 *           the corresponding ISID_callId object here.
 *
 * Return:
 *   ISI_RETURN_FAILED  : The callId (the call) was not found.
 *   ISI_RETURN_OK      : The call was found and was returned in call_ptr.
 */
ISI_Return ISID_callGet(
    ISI_Id        callId,
    ISID_CallId **call_ptr)
{
    ISID_Entry *e_ptr;
    if (_ISID_entryGet(callId, 1, &_ISID_Calls, &e_ptr) != ISI_RETURN_FAILED) {
        *call_ptr = (ISID_CallId*)e_ptr;
        return (ISI_RETURN_OK);
    }
    /* If we are then we didn't find it. If the callId is non-zero
     * then look for the callId under the secondary
     * set of descriptors but ignore zero i.e. "e.desc2"
     */
    if (callId) {
        if (_ISID_entryGet(callId, 0, &_ISID_Calls, &e_ptr) !=
                ISI_RETURN_FAILED) {
            *call_ptr = (ISID_CallId*)e_ptr;
            return (ISI_RETURN_OK);
        }
    }
    return (ISI_RETURN_FAILED);
}

/*
 * ======== ISID_gchatGet ========
 * This function is used to get (or retrieve) a ISID_GChatId object from the
 * linked list (database) used to manage group chat rooms.
 *
 * chatId : An identifier representing the id of the particular group chat room.
 *
 * chat_ptr : An address to a pointer to a ISID_GChatId object.  If this
 *           function successfully finds the ISID_chatId object specified by
 *           the 'chatId' parameter, the function will write the pointer to
 *           the corresponding ISID_chatId object here.
 *
 * Return:
 *   ISI_RETURN_FAILED  : The chatId (the chat) was not found.
 *   ISI_RETURN_OK      : The chatId was found and was returned in chat_ptr.
 */
ISI_Return ISID_gchatGet(
    ISI_Id         chatId,
    ISID_GChatId **chat_ptr)
{
    ISID_Entry *e_ptr;
    if (_ISID_entryGet(chatId, 1, &_ISID_GChats, &e_ptr) != ISI_RETURN_FAILED) {
        *chat_ptr = (ISID_GChatId*)e_ptr;
        return (ISI_RETURN_OK);
    }
    return (ISI_RETURN_FAILED);
}

ISI_Return ISID_gchatGetFirst(
    ISID_GChatId **chat_ptr)
{
    ISID_Entry *e_ptr;
    e_ptr = _ISID_GChats.head_ptr;
    if (e_ptr) {
        *chat_ptr = (ISID_GChatId*)e_ptr;
        return (ISI_RETURN_OK);
    }
    return (ISI_RETURN_FAILED);
}

ISI_Return ISID_gchatGetFirstByServiceId(
    ISID_ServiceId *service_ptr,
    ISID_GChatId  **chat_ptr)
{
    ISID_GChatId *g_ptr;
    g_ptr = (ISID_GChatId*)_ISID_GChats.head_ptr;
    while  (g_ptr) {
        if (g_ptr->service_ptr && g_ptr->service_ptr == service_ptr) {
            *chat_ptr = g_ptr;
            return (ISI_RETURN_OK);
        }
        g_ptr = (ISID_GChatId*)g_ptr->e.next_ptr;
    }
    return (ISI_RETURN_FAILED);
}

/*
 * ======== ISID_callGetFirst ========
 * This function is used to get (or retrieve) the first ISID_CallId object
 * in the linked list (database) used to manage "calls".
 * In other words, it pops the "head" of the list.
 *
 * call_ptr : An address to a pointer to a ISID_CallId object.
 *            The function will write the pointer to the "head"
 *            entry here if it is successful.
 *
 * Returns:
 *  ISI_RETURN_OK       : The "head" entry was returned.
 *  ISI_RETURN_FAILED   : The list is empty.
 */
ISI_Return ISID_callGetFirst(
    ISID_CallId **call_ptr)
{
    ISID_Entry *e_ptr;
    e_ptr = _ISID_Calls.head_ptr;
    if (e_ptr) {
        *call_ptr = (ISID_CallId*)e_ptr;
        return (ISI_RETURN_OK);
    }
    return (ISI_RETURN_FAILED);
}

ISI_Return ISID_callGetFirstByServiceId(
    ISID_ServiceId *service_ptr,
    ISID_CallId   **call_ptr)
{
    ISID_CallId *c_ptr;
    c_ptr = (ISID_CallId*)_ISID_Calls.head_ptr;
    while  (c_ptr) {
        if (c_ptr->service_ptr && c_ptr->service_ptr == service_ptr) {
            *call_ptr = c_ptr;
            return (ISI_RETURN_OK);
        }
        c_ptr = (ISID_CallId*)c_ptr->e.next_ptr;
    }
    return (ISI_RETURN_FAILED);
}

/*
 * ======== ISID_accountFind ========
 * This function searches a service for account information based on the name
 * of the account provided in the "str_ptr" parameter.
 *
 * Returns:
 *  ISID_Account*  : The account entry was found and the pointer was returned.
 *  NULL           : No account information could be found for the name
 *                   provided in str_ptr.
 */
ISID_Account* ISID_accountFind(
    ISID_ServiceId *service_ptr,
    char           *str_ptr,
    vint            isStrRealm)
{
    ISID_Account *acct_ptr;
    acct_ptr = service_ptr->accounts_ptr;
    while (acct_ptr) {
        if (isStrRealm) {
            /* Then look for the user using the realm */
            if (OSAL_strcasecmp(acct_ptr->szRealm, str_ptr) == 0) {
                return acct_ptr;
            }
        }
        else {
            /* Then look for the user using the username */
            if (OSAL_strcasecmp(acct_ptr->szUsername, str_ptr) == 0) {
                return acct_ptr;
            }
        }
        acct_ptr = acct_ptr->next_ptr;
    }
    return (NULL);
}

/*
 * ======== ISID_accountGet ========
 * This function returns the first account in the list of accounts known
 * to the service specified in service_ptr.
 *
 * Returns:
 *  ISID_Account*  : A pointer to the account
 *  NULL           : No account information is known.
 */
ISID_Account* ISID_accountGet(
    ISID_ServiceId *service_ptr)
{
    return (service_ptr->accounts_ptr);
}

/*
 * ======== ISID_accountRemove ========
 * This function removes account information from a service based on the
 * "realm" provided in the realm_ptr parameter.
 *
 * Returns:
 *  ISI_RETURN_OK     :   The account entry was found and removed.
 *  ISI_RETURN_FAILED :   No account information could be found under the
 *                        "realm" provided.
 */
ISI_Return ISID_accountRemove(
    ISID_ServiceId *service_ptr,
    char           *realm_ptr)
{
   ISID_Account *curr_ptr;
   ISID_Account *prev_ptr;
   curr_ptr = service_ptr->accounts_ptr;
   prev_ptr = NULL;

   while (curr_ptr) {
        if (OSAL_strcasecmp(curr_ptr->szRealm, realm_ptr) == 0) {
            /* Found it! */
            if (!prev_ptr) {
                /* Then it was the first one in the list */
                service_ptr->accounts_ptr = curr_ptr->next_ptr;
                /* Free up this one */
                service_ptr->numAccounts--;
                ISI_free(curr_ptr, ISI_OBJECT_ACCOUNT);
                return (ISI_RETURN_OK);
            }
            else {
                prev_ptr->next_ptr = curr_ptr->next_ptr;
                service_ptr->numAccounts--;
                ISI_free(curr_ptr, ISI_OBJECT_ACCOUNT);
                return (ISI_RETURN_OK);
            }
        }
        prev_ptr = curr_ptr;
        curr_ptr = curr_ptr->next_ptr;
    }
    return (ISI_RETURN_FAILED);
}

/*
 * ======== ISID_accountAdd ========
 * This function adds account information to a service
 *
 * Returns:
 *  ISI_RETURN_OK     :   The account entry was added.
 *  ISI_RETURN_FAILED :   No account was added, the max number of accounts
 *                        was reached.
 */
ISI_Return ISID_accountAdd(
    ISID_ServiceId *service_ptr,
    ISID_Account   *acct_ptr)
{
   ISID_Account *curr_ptr;
   curr_ptr = service_ptr->accounts_ptr;

   if (service_ptr->numAccounts > ISID_MAX_NUM_ACCOUNTS) {
       return (ISI_RETURN_FAILED);
   }

   if (!curr_ptr) {
       /* Then this is the first entry */
       service_ptr->accounts_ptr = acct_ptr;
       service_ptr->numAccounts++;
   }
   else {
       while (curr_ptr) {
            if (!curr_ptr->next_ptr) {
                /* then we are at the end of the list */
                curr_ptr->next_ptr = acct_ptr;
                service_ptr->numAccounts++;
                break;
            }
            curr_ptr = curr_ptr->next_ptr;
       }
   }
   return (ISI_RETURN_OK);
}

/*
 * ======== ISID_coderAdd ========
 * This function adds a coder description to an array of coders
 *
 * Returns:
 *  ISI_RETURN_OK     :   The coder entry was added.
 *  ISI_RETURN_FAILED :   No coder was added, the max number of coders
 *                        was reached.
 */
ISI_Return ISID_coderAdd(
    ISIP_Coder      coder[],
    vint            coderMaxSize,
    ISIP_Coder     *cdr_ptr)
{
    vint x;
    for (x = 0 ; x < coderMaxSize ; x++) {
        if (coder[x].szCoderName[0] == 0) {
            /* then we found an empty cell */
            coder[x] = *cdr_ptr;
            return (ISI_RETURN_OK);
        }
    }
    return (ISI_RETURN_FAILED);
}

/*
 * ======== ISID_coderFind ========
 * This function finds a coder description in an array of coders
 * based on a "coder name" (i.e. "PCMU").
 *
 * Returns:
 *  ISIP_Coder*   :   The coder entry was found and returned.
 *  NULL          :   The coder could not be found for the name provided.
 */
ISIP_Coder* ISID_coderFind(
    ISIP_Coder      coder[],
    vint            coderMaxSize,
    char           *coderName_ptr,
    char           *coderDescription_ptr)
{
    vint x;
    ISID_CoderE *coder_ptr;

    for (x = 0 ; x < coderMaxSize ; x++) {
        if (OSAL_strcasecmp(coder[x].szCoderName, coderName_ptr) == 0) {
            /* If it's AMR or AMR-WB then compare octet-align format. */
            coder_ptr = ISID_coderFindIsi(coderName_ptr);
            if ((NULL != coder_ptr) && (NULL != coder_ptr->description_ptr)) {
                /*
                 * Description is not null, compare it.
                 * See if both has "octet-align=1" or not.
                 */
                if ((0 == OSAL_strncasescan(coderDescription_ptr,
                        ISI_CODER_DESCRIPTION_STRING_SZ,
                        coder_ptr->description_ptr)) ==
                        (0 == OSAL_strncasescan(coder[x].description,
                        ISI_CODER_DESCRIPTION_STRING_SZ,
                        coder_ptr->description_ptr))) {
                    /* Match the description */
                    return &coder[x];
                }
            }
            else {
                /* Description is null, coder name match. Found it */
                return &coder[x];
            }
        }
    }
    return (NULL);
}

/*
 * ======== ISID_coderRemove ========
 * This function finds a coder description in an array of coders
 * based on a "coder name" (i.e. "PCMU") and then removes it from the
 * list (or array ) of coders provided in the coder parameter.
 *
 * Returns:
 *  ISI_RETURN_OK     : The coder entry was found and removed.
 *  ISI_RETURN_FAILED : The coder could not be found and hence NOT removed.
 */
ISI_Return ISID_coderRemove(
    ISIP_Coder coder[],
    vint       coderMaxSize,
    char      *coderName_ptr)
{
    vint x;
    for (x = 0 ; x < coderMaxSize ; x++) {
        if (OSAL_strcasecmp(coder[x].szCoderName, coderName_ptr) == 0) {
            /* then we found it */
            ISID_coderClean(&coder[x]);
            return (ISI_RETURN_OK);
        }
    }
    return (ISI_RETURN_FAILED);
}

/*
 * ======== ISID_coderRemoveByPayloadType ========
 * This function finds a coder description in an array of coders
 * based on a "Payload Type" and then removes it from the
 * list (or array ) of coders provided in the coder parameter.
 *
 * Returns:
 *  ISI_RETURN_OK     : The coder entry was found and removed.
 *  ISI_RETURN_FAILED : The coder could not be found and hence NOT removed.
 */
ISI_Return ISID_coderRemoveByPayloadType(
    ISIP_Coder coder[],
    vint       coderMaxSize,
    vint       payloadType)
{
    vint x;
    char payloadTypeBuf[32];
    OSAL_snprintf(payloadTypeBuf, 32, "enum=%d;", payloadType);
    for (x = 0 ; x < coderMaxSize ; x++) {
        if (OSAL_strscan(coder[x].description, payloadTypeBuf)) {
            /* then we found it */
            ISID_coderClean(&coder[x]);
            return (ISI_RETURN_OK);
        }
    }
    return (ISI_RETURN_FAILED);
}

/*
 * ======== ISID_coderFindIsi ========
 * When users attempt to add coders to a service or a call via the appropriate
 * ISI API interfaces, the ISI module will verify that it is aware of such
 * coders by comparing the coder description provided by the application to a
 * coder table defined at the top of this file. This function returns entries
 * from that table based on the coder name provided in the coderName_ptr.
 *
 * Returns:
 *  ISID_CoderE*  : The coder entry was found and returned.
 *  NULL          : No coder entry was found for the name provided in
 *                  coderName_ptr.
 */
ISID_CoderE *ISID_coderFindIsi(
    char *coderName_ptr)
{
    vint x, y;
    x = (sizeof(_ISID_CoderTable)) / (sizeof(ISID_CoderE));
    for (y = 0 ; y < x ; y++) {
        if (OSAL_strcasecmp(_ISID_CoderTable[y].name_ptr, coderName_ptr) == 0) {
            return &_ISID_CoderTable[y];
        }
    }
    return (NULL);
}

/*
 * ======== ISID_akaChallengeGet ========
 * This function is used to retrive rand and autn of AKA challenge.
 *
 * rand_ptr : A pointer to an rand for AKA authentication.
 * autn_ptr : A pointer to an autn for AKA authentication.
 *
 * Returns:
 *  ISI_RETURN_OK       : Challenge get successfully.
 */
ISI_Return ISID_akaChallengeGet(
    ISID_ServiceId *service_ptr,
    char           *rand_ptr,
    char           *autn_ptr)
{
    OSAL_memCpy(rand_ptr, service_ptr->rand, sizeof(service_ptr->rand));
    OSAL_memCpy(autn_ptr, service_ptr->autn, sizeof(service_ptr->autn));

    /* The challenge should not be used second times, clear them */
    OSAL_memSet(service_ptr->rand, 0, sizeof(service_ptr->rand));
    OSAL_memSet(service_ptr->autn, 0, sizeof(service_ptr->autn));
    return (ISI_RETURN_OK);
}

/*
* ======== ISID_ussdClean ========
* This function will initialize a ISID_UssdId object.  It is typically
* called when a ussd message is first sent or received.
*
* Returns:
*  Nothing.
*/

void ISID_ussdClean(
    ISID_UssdId *ussd_ptr)
{
    _ISID_entryClean(&ussd_ptr->e);
    ussd_ptr->service_ptr = NULL;
    ussd_ptr->state = ISID_USSD_STATE_NONE;
    OSAL_memSet(&ussd_ptr->isiMsg, 0, sizeof(ISIP_Message));
    return;
}

/*
 * ======== ISID_ussdCreate ========
 * This function is used to create/initialize as object used to manage
 * a ussd message.  The object returned is then typically inserted into
 * the ussd message linked list (database).
 *
 * Returns:
 *  ISID_UssdId*    : A pointer to a new ISID_UssdId object
 *  NULL            : No object could be allocated or initialized.
 */

ISID_UssdId *ISID_ussdCreate(ISID_ServiceId *service_ptr)
{
    ISID_UssdId *t_ptr;
    t_ptr = ISI_alloc(sizeof(ISID_UssdId), ISI_OBJECT_USSD_ID);
    if (t_ptr == NULL) {
        return (NULL);
    }
    else {
        ISID_ussdClean(t_ptr);
        /* Load the coder from the service */
        OSAL_memCpy(t_ptr->isiMsg.msg.ussd.coders, service_ptr->coders,
                sizeof(ISIP_Coder) * ISI_CODER_NUM);
        return (t_ptr);
    }
}

/*
 * ======== ISID_ussdAdd ========
 * This function is used to add (insert) a ISID_UssdId object into the
 * linked list (database) used to manage ussd messages.
 *
 * ussd_ptr : A pointer to the object to add (insert)
 *
 * id_ptr : A pointer to an id to use as a unique identifier for this
 *          particular entry.  If the de-reference value is '0', then the
 *          ISID will generate the unique identifier and write it to id_ptr.
 *
 * Returns:
 *  ISI_RETURN_OK       : The entry was successfully inserted into the
 *                        linked list.
 *  ISI_RETURN_FAILED   : Could not insert the object, the list is full.
 */

ISI_Return ISID_ussdAdd(
    ISID_UssdId *ussd_ptr,
    ISI_Id      *id_ptr)
{
    return _ISID_entryAdd(&ussd_ptr->e, &_ISID_Ussds, id_ptr, ISID_MAX_NUM_MSGS);
}

/*
 * ======== ISID_ussdDestroy ========
 * This function is used to remove and free a ISID_UssdId object from the
 * linked list (database) used to manage ussd messages. First the object
 * is searched for via the ussdId, then it is removed from the list and
 * then the object is "freed" back to the heap.
 *
 * ussdId : The identifier used to id the particular entry to remove from
 *          the linked list (database).
 *
 * Returns:
 *  ISI_RETURN_OK     : The object was successfully removed and freed
 *  ISI_RETURN_FAILED : Could not remove the object, the object was not found.
 */
ISI_Return ISID_ussdDestroy(
    ISI_Id ussdId)
{
    return _ISID_entryDestroy(ussdId, &_ISID_Ussds, ISI_OBJECT_USSD_ID);
}

/*
 * ======== ISID_ussdGet ========
 * This function is used to get (or retrieve) a ISID_UssdId object from the
 * linked list (database) used to manage ussd messages.
 *
 * ussdId : An identifier representing the id of the particular ISID_UssdId
 *          object to retrieve.
 *
 * ussd_ptr : An address to a pointer to a ISID_UssdId object.  If this
 *           function successfully finds the ISID_UssdId object specified by
 *           the 'ussdId' parameter, the function will write the pointer to
 *           the corresponding ISID_UssdId object here.
 *
 * Returns:
 *  ISI_RETURN_OK       : The entry was successfully found.
 *  ISI_RETURN_FAILED   : Could not find the object.
 */

ISI_Return ISID_ussdGet(
    ISI_Id        ussdId,
    ISID_UssdId **ussd_ptr)
{
    ISID_Entry *e_ptr;

    if (_ISID_entryGet(ussdId, 1, &_ISID_Ussds, &e_ptr) != ISI_RETURN_FAILED) {
        *ussd_ptr = (ISID_UssdId*)e_ptr;
        return (ISI_RETURN_OK);
    }
    return (ISI_RETURN_FAILED);
}

/*
 * ======== ISID_ussdGetFirst ========
 * This function is used to get (or retrieve) the first ISID_UssdId object in
 * the linked list (database) used to manage ussd messages.  In other words,
 * it pops the "head" of the list.
 *
 * ussd_ptr : An address to a pointer to a ISID_UssdId object. The function
 *            will write the pointer to the "head" entry here it is
 *            successful.
 *
 * Returns:
 *  ISI_RETURN_OK       : The "head" entry was returned.
 *  ISI_RETURN_FAILED   : The list is empty.
 */
ISI_Return ISID_ussdGetFirst(
    ISID_UssdId **ussd_ptr)
{
    ISID_Entry *e_ptr;
    e_ptr = _ISID_Ussds.head_ptr;
    if (e_ptr) {
        *ussd_ptr = (ISID_UssdId*)e_ptr;
        return (ISI_RETURN_OK);
    }
    return (ISI_RETURN_FAILED);
}

/*
 * ======== ISID_ussdGetFirstByServiceId ========
 * This function is used to get (or retrieve) ussd messages that belong
 * to a particular "service". This function will search the linked list (database) 
 * used to manage ussd messages and find the next ussd message that belongs
 * to the service specified in service_ptr.
 *
 * service_ptr :  A pointer to a ISID_ServiceId object that represents the
 *                service to look within for the next ussd message.
 *
 * ussd_ptr : An address to a pointer to a ISID_UssdId object. The function
 *            will write the pointer to the ussd message entry here it is
 *            successful.
 *
 * Returns:
 *  ISI_RETURN_OK       : A ussd message entry was found and returned.
 *  ISI_RETURN_FAILED   : Could not find any ussd messages for this service
 *                        and state.
 */
ISI_Id ISID_ussdGetFirstByServiceId(
    ISID_ServiceId   *service_ptr,
    ISID_UssdId     **ussd_ptr)
{
    ISID_UssdId *t_ptr;
    t_ptr = (ISID_UssdId*)_ISID_Ussds.head_ptr;
    while (t_ptr) {
        if (t_ptr->service_ptr && t_ptr->service_ptr == service_ptr) {
            *ussd_ptr = t_ptr;
            return (ISI_RETURN_OK);
        }
        t_ptr = (ISID_UssdId*)t_ptr->e.next_ptr;
    }
    return (ISI_RETURN_FAILED);
}

/*
 * ======== ISID_ussdGetViaServiceIdAndState ========
 * This function is used to get (or retrieve) ussd messages that belong
 * to a particular "service" and are in a particular "state". This function
 * will search the linked list (database) used to manage ussd messages and
 * find the next ussd message that belongs to the service specified in
 * service_ptr.  Additionally the function will only look for ussd messages
 * that are in the state specified in "state".
 *
 * service_ptr :  A pointer to a ISID_ServiceId object that represents the
 *                service to look within for the next ussd message.
 *
 * ussd_ptr : An address to a pointer to a ISID_UssdId object. The function
 *            will write the pointer to the ussd message entry here it is
 *            successful.
 *
 * state : The state of a ussd message to search for.  Possible values are:
 *         ISID_USSD_STATE_NONE, ISID_USSD_STATE_ACTIVE.
 *
 * Returns:
 *  ISI_RETURN_OK       : A ussd message entry was found and returned.
 *  ISI_RETURN_FAILED   : Could not find any ussd messages for this service
 *                        and state.
 */

ISI_Return ISID_ussdGetViaServiceIdAndState(
    ISID_ServiceId   *service_ptr,
    ISID_UssdId     **ussd_ptr,
    ISID_UssdState    state)
{
    ISID_UssdId *t_ptr;

    t_ptr = (ISID_UssdId*)_ISID_Ussds.head_ptr;
    while (t_ptr) {
        if (t_ptr->service_ptr && t_ptr->service_ptr == service_ptr) {
            /* found it */
            if (t_ptr->state == state) {
                *ussd_ptr = t_ptr;
                return (ISI_RETURN_OK);
            }
        }
        t_ptr = (ISID_UssdId*)t_ptr->e.next_ptr;
    }
    return (ISI_RETURN_FAILED);
}

/*
 * ======== ISID_lockUssds ========
 * This function is used to take a semaphore against the
 * linked list (database) used to manage ussd messages.
 * Note that to successfully use a mutex on "Ussds" &
 * "services" mutexes must also be taken.
 *
 * Returns:
 *  ISI_RETURN_OK       : The semaphore was successfully taken.
 *  ISI_RETURN_FAILED   : The semaphore could not be taken.
 */
ISI_Return ISID_lockUssds(void)
{
    /* Lock the services, Ussds database */
    if (0 == ISI_semAcquire(_ISID_Services.mutex)) {
        return (ISI_RETURN_FAILED);
    }
    if (0 == ISI_semAcquire(_ISID_Ussds.mutex)) {
        ISI_semGive(_ISID_Services.mutex);
        return (ISI_RETURN_FAILED);
    }
    return (ISI_RETURN_OK);
}

/*
 * ======== ISID_unlockUssds ========
 * This function is used to Give a semaphore back to the linked list
 * (database) used to manage ussd messages. Note, how the "services"
 * semaphores are also given back.
 *
 * Returns:
 *  Nothing
 */
void ISID_unlockUssds(void)
{
    ISI_semGive(_ISID_Services.mutex);
    ISI_semGive(_ISID_Ussds.mutex);
}



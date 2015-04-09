/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29644 $ $Date: 2014-11-03 19:15:40 +0800 (Mon, 03 Nov 2014) $
 */

#include <osal.h>
#include <isi.h>
#include <isip.h>
#include "_iut_app.h"
#include "_iut_prt.h"
#include "_iut_cfg.h"
#include "_iut.h"


static vint           _IUT_peer;
static ISI_Id         _IUT_serviceId;
static IUT_HandSetObj _IUT_Ep[IUT_CFG_MAX_SERVICE];
static IUT_ServiceObj _IUT_Services[IUT_CFG_MAX_SERVICE];
static vint           _IUT_init = 0;

/*
 * ======== _IUT_appGetFileSize ========
 * This function is to get file size of given file name.
 *
 * Returns the file size of the fileName specified.
 * Value will be less than 0 to indicate error.
 */
static vint _IUT_appGetFileSize(const char* fileName)
{
    OSAL_FileId fileId;
    vint size = 0;

    if (OSAL_SUCCESS != OSAL_fileOpen(&fileId, fileName, OSAL_FILE_O_RDONLY,
            0)) {
        return -1;
    }

    if (OSAL_SUCCESS != OSAL_fileGetSize(&fileId, &size)) {
        OSAL_fileClose(&fileId);
        return -1;
    }

    OSAL_fileClose(&fileId);
    return (size);
}

/*
 * ======== _IUT_appBareJid() ========
 *
 * Takes a FULL jid as a NULL terminated string and replaces the
 * slash with a NULL termination. This means that the string in jid_ptr
 * get re-written
 *
 *
 * Returns:
 *   Nothing.
 *
 */
static void _IUT_appBareJid(
    char *jid_ptr)
{
    char *str_ptr;
    /* Convert string to bare JID */
    str_ptr = OSAL_strchr(jid_ptr, '/');
    if (str_ptr != NULL) {
        /* NULL terminate */
        *str_ptr = 0;
    }
    return;
}

/*
 * ======== _IUT_appRosterInsert() ========
 *
 * This function will read roster contact info from ISI when it signals
 * that a contact has been received and then insert it into a database
 * (array of roster entries).  This function can be considered the ISI event
 * handler for ISI_EVENT_CONTACT_RECEIVED event.
 *
 *
 * Returns:
 *   Nothing.
 *
 */
static void _IUT_appRosterInsert(
    IUT_HandSetObj *hs_ptr,
    ISI_Id          id)
{
    IUT_Roster roster;
    vint        x;

    /* Init the parts of the roster entry that won't be set be a ISI call */
    roster.jid[0] = 0;
    roster.statusDesc[0] = 0;
    if (ISI_readContact(id, roster.contact, roster.group,
            roster.name, roster.state, IUT_STR_SIZE) == ISI_RETURN_DONE) {
        OSAL_logMsg("Got a contact for :%s\n", roster.contact);
        for (x = 0 ; x < IUT_MAX_NUM_ROSTERS ; x++) {
            if (OSAL_strncmp(roster.contact, hs_ptr->roster[x].contact,
                    IUT_STR_SIZE) == 0) {
                /* Found a duplicate */
                OSAL_snprintf(hs_ptr->roster[x].group, IUT_STR_SIZE,
                        "%s", roster.group);
                OSAL_snprintf(hs_ptr->roster[x].name, IUT_STR_SIZE,
                        "%s", roster.name);
                OSAL_snprintf(hs_ptr->roster[x].state, IUT_STR_SIZE,
                        "%s", roster.state);
                break;
            }
            else if (hs_ptr->roster[x].contact[0] == 0) {
                /* Then here's an empty cell */
                hs_ptr->roster[x] = roster;
                break;
            }
        }
    }
}

/*
 * ======== _IUT_appRosterFindJid() ========
 *
 * This function will search the roster list for a JID
 *
 * Returns:
 *   IUT_OK  : The JID was found inthe roster list.
 *   IUT_ERR : The JID could not be found in the roster list.
 *
 */
static vint  _IUT_appRosterFindJid(
    IUT_HandSetObj *hs_ptr,
    char           *to_ptr,
    char           *target_ptr,
    vint            targetLen)
{
    vint        x;
    char       *begin_ptr;
    char       *end_ptr;
    vint        len;

    for (x = 0 ; x < IUT_MAX_NUM_ROSTERS ; x++) {
        if (OSAL_strcmp(hs_ptr->roster[x].contact, to_ptr) == 0) {
            /* Found the entry, try to find the jid in the status string */
            begin_ptr = OSAL_strscan(hs_ptr->roster[x].statusDesc, to_ptr);
            if (begin_ptr) {
                /* We have the beginning of the full Jid, find the end */
                end_ptr = OSAL_strscan(begin_ptr, "<");
                if (end_ptr) {
                    /* Copy the string and NULL terminate */
                    len = end_ptr - begin_ptr;
                    if (len > targetLen) {
                        len = targetLen;
                    }
                    OSAL_memCpy(target_ptr, begin_ptr, len);
                    target_ptr[len] = 0;
                    return (IUT_OK);
                }
            }
        }
    }
    return (IUT_ERR);
}

/*
 * ======== _IUT_appPresenceUpdate() ========
 *
 * This function will read presence state info from ISI when it signals
 * that a new presence info has been received it then updated the roster
 * database (array of roster entries).  This function can be considered
 * the ISI event handler for ISI_EVENT_PRES_RECEIVED event.
 *
 *
 * Returns:
 *   Nothing.
 *
 */
static void _IUT_appPresenceUpdate(
    IUT_HandSetObj *hs_ptr,
    ISI_Id          id)
{
    ISI_Return r;
    char       contact[IUT_STR_SIZE + 1];
    char       presence[IUT_STR_SIZE + 1];
    vint       x;
    ISI_Id     chatId;

    r= ISI_readPresence(id, &chatId, contact, presence, IUT_STR_SIZE);

    if (r == ISI_RETURN_DONE) {

        OSAL_logMsg("PRES EVT: chatId:%d contact:%s presence:%s\n",
                chatId, contact, presence);

        _IUT_appBareJid(contact);
        /* Now find the roster entry based on the contact info */
        for (x = 0 ; x < IUT_MAX_NUM_ROSTERS ; x++) {
            if (OSAL_strncmp(contact, hs_ptr->roster[x].contact,
                         IUT_STR_SIZE) == 0) {
                /* Found the roster entry, now update the presence state */
                OSAL_snprintf(hs_ptr->roster[x].statusDesc, IUT_STR_SIZE,
                        "%s", presence);
                OSAL_logMsg("PRESENCE chatId:%d status string: %s\n", chatId,
                        hs_ptr->roster[x].statusDesc);
                return;
            }
        }
        /*
         * If we are here then the roster entry didn't exist,
         * so insert it anyway
         */
        for (x = 0 ; x < IUT_MAX_NUM_ROSTERS ; x++) {
            if (hs_ptr->roster[x].contact[0] == 0) {
                /* Found an empty cell, so insert */
                OSAL_snprintf(hs_ptr->roster[x].contact, IUT_STR_SIZE,
                        "%s", contact);
                OSAL_snprintf(hs_ptr->roster[x].statusDesc, IUT_STR_SIZE,
                        "%s", presence);
                OSAL_logMsg("PRESENCE status string: %s\nPRESENCE Contact :%s\n",
                        hs_ptr->roster[x].statusDesc, hs_ptr->roster[x].contact);
                hs_ptr->roster[x].group[0] = 0;
                hs_ptr->roster[x].name[0] = 0;
                hs_ptr->roster[x].jid[0] = 0;
                hs_ptr->roster[x].state[0] = 0;
                return;
            }
        }
    }
    else {
        OSAL_logMsg("Error updating presence for %s. Return code:%s\n",
                contact, IUT_prtReturnString(r));
    }
}

/*
 * ======== _IUT_appSubAddRequest() ========
 *
 * This function will read a request from a remote entity to subscribe
 * to this entity's presence state.  This function can be considered
 * the ISI event handler for ISI_EVENT_SUB_TO_PRES_RECEIVED event.
 *
 *
 * Returns:
 *   Nothing.
 *
 */
static void _IUT_appSubAddRequest(
    IUT_HandSetObj *hs_ptr,
    ISI_Id          id)
{
    ISI_Return r;
    vint       x;
    char       contact[IUT_STR_SIZE + 1];

    r = ISI_readSubscribeToPresenceRequest(id, contact);
    if (r == ISI_RETURN_OK) {
        /* Now find or add the roster entry based on the contact info */
        for (x = 0 ; x < IUT_MAX_NUM_ROSTERS ; x++) {
            if ((OSAL_strncmp(contact, hs_ptr->roster[x].contact,
                    IUT_STR_SIZE) == 0) || hs_ptr->roster[x].contact[0] == 0) {
                /*
                 * Found a duplicate or an empty spot write something
                 * informative to the statusDesc to inform user of the 'invite'
                 */
                OSAL_snprintf(hs_ptr->roster[x].contact, IUT_STR_SIZE,
                        "%s", contact);
                hs_ptr->roster[x].group[0] = 0;
                hs_ptr->roster[x].jid[0] = 0;
                hs_ptr->roster[x].name[0] = 0;
                hs_ptr->roster[x].state[0] = 0;
                OSAL_snprintf(hs_ptr->roster[x].statusDesc, IUT_STR_SIZE,
                        "%s", "You are Invited");
                break;
            }
        }
    }
    else {
        OSAL_logMsg("Error adding subscription request %s. Return code:%s\n",
                contact, IUT_prtReturnString(r));
    }
}

/*
 * ======== _IUT_appSubAdd() ========
 *
 * This function will add a contact to the roster.
 *
 * Returns:
 *   Nothing.
 *
 */
static void _IUT_appSubAdd(
    IUT_HandSetObj *hs_ptr,
    char           *to_ptr,
    char           *status_ptr)
{
    vint x;

    /* Now find or add the roster entry based on the contact info */
    for (x = 0 ; x < IUT_MAX_NUM_ROSTERS ; x++) {
        if ((OSAL_strncmp(to_ptr, hs_ptr->roster[x].contact,
                IUT_STR_SIZE) == 0) || hs_ptr->roster[x].contact[0] == 0) {
            /*
             * Found a duplicate or an empty spot write something
             * informative to the statusDesc to inform user of the 'invitation'
             */
            OSAL_snprintf(hs_ptr->roster[x].contact, IUT_STR_SIZE, "%s", to_ptr);
            hs_ptr->roster[x].group[0] = 0;
            hs_ptr->roster[x].jid[0] = 0;
            hs_ptr->roster[x].name[0] = 0;
            hs_ptr->roster[x].state[0] = 0;
            OSAL_snprintf(hs_ptr->roster[x].statusDesc, IUT_STR_SIZE, "%s",
                    status_ptr);
            break;
        }
    }
}

/*
 * ======== _IUT_appSubUpdate() ========
 *
 * This function will read a response to a request from a remote
 * entity regarding our request to subscribe to their presence.
 * This function can be considered the ISI event handler for
 * ISI_EVENT_SUBSCRIPTION_RESP_RECEIVED event.
 *
 *
 * Returns:
 *   Nothing.
 *
 */
static void _IUT_appSubUpdate(
    IUT_HandSetObj *hs_ptr,
    ISI_Id          id)
{
    ISI_Return r;
    vint       x;
    int        allowed;
    char       contact[IUT_STR_SIZE];

    r = ISI_readSubscriptionToPresenceResponse(id, contact, &allowed);
    if (r == ISI_RETURN_OK) {
        /* Now find the roster entry based on the contact info */
        for (x = 0 ; x < IUT_MAX_NUM_ROSTERS ; x++) {
            if (OSAL_strncmp(contact, hs_ptr->roster[x].contact,
                        IUT_STR_SIZE) == 0) {
                /* Found the roster entry, now update the presence state */
                if (allowed == 1) {
                    OSAL_snprintf(hs_ptr->roster[x].statusDesc, IUT_STR_SIZE, "%s",
                            "Subscription Accepted");
                }
                else {
                    OSAL_snprintf(hs_ptr->roster[x].statusDesc, IUT_STR_SIZE, "%s",
                            "Subscription Denied");
                }
                break;
            }
        }
    }
    else {
        OSAL_logMsg("Error reading subscription status for %s. Return code:%s\n",
                contact, IUT_prtReturnString(r));
    }
}

/*
 * ======== _IUT_appHandoff() ========
 *
 * This function is command handler for performing a VCC
 * ("Voice Call Continuity") handoff.  It will set the VDN/VDI
 * for the target URI/phone number that will accept the handoff.
 *
 * Returns:
 *   Nothing.
 *
 */
static void _IUT_appHandoff(
    IUT_HandSetObj *hs_ptr,
    ISI_Id          callId)
{
    ISI_Id handoffServiceId;
    char   handoff[IUT_STR_SIZE + 1];

    if (hs_ptr->proto == ISI_PROTOCOL_SIP) {
        if (ISI_getCallHandoff(callId, handoff) == ISI_RETURN_OK) {
            /*
             * Then we are handing off to GSM AT.
             * The 'handoff' will be populated with the VDN for the
             * handoff. Set the 'to' URI of the next service (GSM AT)
             * to this value.  Now, if the GSM AT service decides to
             * call the handoff number.
             */
            if (0 != (handoffServiceId =
                    IUT_appServiceNext(hs_ptr->serviceId))) {
                /* Then set the 'to' URI for this new service */
                IUT_cfgSetField(handoffServiceId, IUT_CFG_TO_URI_F, handoff);
            }
        }
    }
    else if (hs_ptr->proto == IUT_GSM_AT_SERVICE_TYPE) {
        /*
         * GSM AT to WiFi handoff, if there is no VDI from ISI, then try to
         * handoff the call via the URI of the SIP service. So set the 'to'
         * URI of the SIP service to the URI of itself if ISI has no VDI.
         */
        if (ISI_getCallHandoff(callId, handoff) == ISI_RETURN_OK) {
            if (0 != (handoffServiceId =
                    IUT_appServiceNext(hs_ptr->serviceId))) {
                /* Then set the 'to' URI for this new service */
                IUT_cfgSetField(handoffServiceId, IUT_CFG_TO_URI_F, handoff);
            }
        }
        else {
            if (0 != (handoffServiceId = IUT_appServiceNext(hs_ptr->serviceId))) {
                /* Then set the 'to' URI for this new service */
                OSAL_snprintf(handoff, IUT_STR_SIZE, "sip:%s@%s",
                        IUT_cfgGetStrField(handoffServiceId, IUT_CFG_USERNAME_F),
                        IUT_cfgGetStrField(handoffServiceId, IUT_CFG_REALM_F));
                IUT_cfgSetField(handoffServiceId, IUT_CFG_TO_URI_F, handoff);
            }
        }
    }
    return;
}

/*
 * ======== IUT_appRosterAdd() ========
 *
 * This function is command handler for adding a contact entry to
 * the roster database. It will also call the ISI API to
 * add a Contact to a roster list residing on a remote server.
 *
 * Returns:
 *   IUT_OK : Contact was successfully added.
 *   IUT_ERR: Contact could not be added.
 *
 */
vint IUT_appRosterAdd(
    IUT_HandSetObj *hs_ptr,
    char           *contact_ptr,
    char           *group_ptr)
{
    ISI_Return r;

    r = ISI_addContact(&hs_ptr->presId, hs_ptr->serviceId, contact_ptr,
            group_ptr, NULL);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not add:%s grp:%s to the roster ERROR:%s\n",
                contact_ptr, group_ptr, IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

/*
 * ======== IUT_appRosterRemove() ========
 *
 * This function is command handler for removing a contact from
 * the roster database. It will also call the ISI API to
 * remove a Contact from a roster list residing on a remote server.
 *
 * Returns:
 *   IUT_OK : Contact was successfully added.
 *   IUT_ERR: Contact could not be added.
 *
 */
vint IUT_appRosterRemove(
    IUT_HandSetObj *hs_ptr,
    char           *contact_ptr,
    char           *group_ptr)
{
    ISI_Return r;

    r = ISI_removeContact(&hs_ptr->presId, hs_ptr->serviceId, contact_ptr,
            group_ptr, NULL);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not remove:%s grp:%sfrom the roster ERROR:%s\n",
                contact_ptr, group_ptr, IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

/*
 * ======== IUT_appCallHandoff() ========
 *
 * This function is command handler for performing a VCC
 * ("Voice Call Continuity") "Handoff".  The call will be handed
 * off to the next available service.
 *
 * Note, if this function fails it will NOT teardown the existing call
 *
 * Returns:
 *   IUT_OK : The call is successfully being handed off
 *   IUT_ERR: the call could not be handed off.
 *
 */
vint IUT_appCallHandoff(
    IUT_HandSetObj *hs_ptr,
    vint            peer)
{
    ISI_Id          handoffServiceId;
    IUT_HandSetObj *handoffHs_ptr;

    /*
     * The following call will set up the VDN, or VDI
     * (i.e. the 'to' field).
     */

    _IUT_appHandoff(hs_ptr, hs_ptr->callId[peer]);

    /* Set the active service to the next service for the handoff */
    handoffServiceId = IUT_appServiceNext(hs_ptr->serviceId);
    if (handoffServiceId == 0) {
        return (IUT_ERR);
    }

    /* Set it as the active service */
    IUT_appServiceSet(handoffServiceId);
    /* Change the hs_ptr to point to the new service */
    handoffHs_ptr = IUT_appServiceGet();
    if (NULL == handoffHs_ptr) {
        return (IUT_ERR);
    }
    /*
     * Now try to place a new call to accomodate the handoff
     * on the next service
     */
    if (IUT_appCall(handoffHs_ptr, peer,
            IUT_cfgGetStrField(handoffServiceId, IUT_CFG_TO_URI_F),
            ISI_SESSION_TYPE_AUDIO, ISI_SESSION_DIR_SEND_RECV, 
            ISI_SESSION_DIR_NONE) != 0) {
        return (IUT_OK);
    }
    return (IUT_ERR);
}




/*
 * ======== IUT_appProcessEvent() ========
 *
 * This function is the entry point for all events coming from ISI.
 * It can be considered the top layer to processing ISI Events received
 * by this application.
 *
 * Returns:
 *   Nothing.
 *
 */
void IUT_appProcessEvent(
    ISI_Id          serviceId,
    ISI_Id          id,
    ISI_IdType      idType,
    ISI_Event       event)
{
    /* First print the id type */
    ISI_Id          callId;
    ISI_Id          chatId;
    ISI_Id          presId;
    ISI_Id          isiId;
    ISI_Return      ret;
    char            message[IUT_STR_SIZE + 1];
    char            from[IUT_STR_SIZE + 1];
    //char            room[IUT_STR_SIZE + 1];
    char            filePath[IUT_STR_SIZE + 1];
    char            subject[IUT_STR_SIZE + 1];
    char            dateTime[IUT_STR_SIZE + 1];
    char            reportId[IUT_STR_SIZE + 1];
    ISI_MessageReport reports;
    vint            x;
    vint            arg0;
    vint            arg1;
    vint            fileSize;
    vint            fileProgress;
    ISI_TelEvent    telEvt;
    IUT_HandSetObj *hs_ptr;
    ISI_MessageType type;
    vint            bytes;
    char            rand[ISI_AKA_AUTH_RAND_STRING_SZ];
    char            autn[ISI_AKA_AUTH_RAND_STRING_SZ];
    vint            features;

    if (serviceId == 0) {
        if (idType == ISI_ID_TYPE_NONE && event == ISI_EVENT_PROTOCOL_FAILED) {
            for (x = 0 ; x < IUT_CFG_MAX_SERVICE ; x++) {
                if (_IUT_Services[x].proto == id) {
                    _IUT_Services[x].isDead = 1;
                    break;
                }
            }
            for (x = 0 ; x < IUT_CFG_MAX_SERVICE ; x++) {
                if (_IUT_Ep[x].proto == id) {
                    _IUT_Ep[x].isDead = 1;
                }
            }
        }
        /* Set a flag to determine if we updated */
        arg0 = 0;
        if (idType == ISI_ID_TYPE_NONE && event == ISI_EVENT_PROTOCOL_READY) {
            /* First try to find an existing entry in the protocol array */
            for (x = 0 ; x < IUT_CFG_MAX_SERVICE ; x++) {
                if (_IUT_Services[x].proto == id) {
                    _IUT_Services[x].isDead = 0;
                    arg0 = 1;
                    break;
                }
            }

            if (0 == arg0) {
                /*
                 * If we are here, then there was no existing entry found so add
                 * this one.
                 */
                for (x = 0 ; x < IUT_CFG_MAX_SERVICE ; x++) {
                    if (_IUT_Services[x].proto == 0) {
                        _IUT_Services[x].proto = id;
                        _IUT_Services[x].isDead = 0;
                        break;
                    }
                }
            }
            /* Now process services in the ep array */
            for (x = 0 ; x < IUT_CFG_MAX_SERVICE ; x++) {
                if (_IUT_Ep[x].proto == id) {
                    _IUT_Ep[x].isDead = 0;
                    _IUT_Ep[x].isActive = 0;
                }
            }
        }
        return;
    }

    hs_ptr = IUT_appServiceFind(serviceId);
    if (hs_ptr == NULL) {
        /* Don't have an object for this event, ignore it */
        return;
    }

    if (idType == ISI_ID_TYPE_SERVICE) {
        if (event == ISI_EVENT_SERVICE_ACTIVE) {
            hs_ptr->isActive = 1;
            /* Features */
            ISI_serviceGetFeature(serviceId, &features);
            IUT_prtFeature(features);
            /* Then send our presence and get some roster entries if any */
            ISI_sendPresence(&presId, id, NULL,
                    "<priority>0</priority><show>available</show><status>Now Available</status>");
        }
        else if (event == ISI_EVENT_SERVICE_INACTIVE) {
            hs_ptr->isActive = 0;
        }
        else if (event == ISI_EVENT_AKA_AUTH_REQUIRED) {
            /* AKA authentication required, get rand and autn */
            ISI_getAkaAuthChallenge(serviceId, rand, autn);
            OSAL_logMsg("AKA challenge rand:%s, autn:%s\n", rand, autn);
            /* Set empty response for authentication failed */
            ISI_setAkaAuthResp(serviceId, ISI_SERVICE_AKA_RESPONSE_SUCCESS,
                    "", 0, "", "", "");
        }
    }
    else if (idType == ISI_ID_TYPE_MESSAGE) {
        if (event == ISI_EVENT_MESSAGE_RECEIVED) {
            /* then we need to get the message and print it */
            if (ISI_getMessageHeader(id, &type, subject, from, dateTime, &reports,
                    reportId) == ISI_RETURN_OK) {
                ret = ISI_RETURN_OK;
                while (ret != ISI_RETURN_DONE && ret != ISI_RETURN_FAILED) {
                    bytes = IUT_STR_SIZE;
                    ret = ISI_readMessage(id, &chatId, message, &bytes);
                    IUT_prtIm(from, chatId, subject, message, dateTime, reports, reportId);
                }
                /* Send back a delivery report if need be */
                if (0 != (reports & ISI_MSG_RPT_DELIVERY_SUCCESS)) {
                    if (0 != chatId) {
                        ISI_sendChatMessageReport(&isiId, chatId,
                                ISI_MSG_RPT_DELIVERY_SUCCESS, reportId);
                    }
                    else {
                        ISI_sendMessageReport(&isiId, serviceId, from,
                                ISI_MSG_RPT_DELIVERY_SUCCESS, reportId);
                    }
                }
                if (0 != (reports & ISI_MSG_RPT_DISPLAY_SUCCESS)) {
                    if (0 != chatId) {
                        ISI_sendChatMessageReport(&isiId, chatId,
                                ISI_MSG_RPT_DISPLAY_SUCCESS, reportId);
                    }
                    else {
                        ISI_sendMessageReport(&isiId, serviceId, from,
                                ISI_MSG_RPT_DISPLAY_SUCCESS, reportId);
                    }
                }
            }
        }
        if (event == ISI_EVENT_MESSAGE_REPORT_RECEIVED) {
            /* then we need to get the message and print it */
            if (ISI_RETURN_OK == ISI_readMessageReport(id, &chatId, from,
                    dateTime, &reports, reportId)) {
                IUT_prtImReport(from, chatId, dateTime, reports, reportId);
            }
        }
        if ((event == ISI_EVENT_MESSAGE_COMPOSING_ACTIVE) ||
                (event == ISI_EVENT_MESSAGE_COMPOSING_IDLE)) {
            if (ISI_RETURN_DONE ==
                    ISI_readMessage(id, &chatId, message, &bytes)) {
                IUT_prtImComposing(chatId, event);
            }
        }

    }
    else if (idType == ISI_ID_TYPE_FILE) {
        switch (event) {
            case ISI_EVENT_FILE_SEND_PROGRESS:
            case ISI_EVENT_FILE_SEND_PROGRESS_COMPLETED:
            case ISI_EVENT_FILE_SEND_PROGRESS_FAILED:
            case ISI_EVENT_FILE_RECV_PROGRESS:
            case ISI_EVENT_FILE_RECV_PROGRESS_COMPLETED:
            case ISI_EVENT_FILE_RECV_PROGRESS_FAILED:
                OSAL_logMsg("%s:%d Event %d is no longer supported.  New Event IDs are used.\n", __FUNCTION__, __LINE__, event);
                /* then we need to get the message and print it */
//                if (ISI_RETURN_OK == ISI_getFileHeader(id, subject, from)) {
//                    if (ISI_RETURN_OK == ISI_readFileProgress(
//                            id, &callId, message, (ISI_FileType*)&arg0, &arg1)) {
//                        IUT_prtFile(from, callId, message, arg0, arg1, subject);
//                    }
//                }
                break;
            /* New RCS-e events include ISI_EVENT_FILE_REQUEST */
            case ISI_EVENT_FILE_REQUEST:
                /* cache the fileId */
                hs_ptr->fileId = id;

                if (ISI_RETURN_OK != ISI_acknowledgeFile(id)) {
                    OSAL_logMsg("%s:%d ISI_acknowledgeFile() failed\n", __FUNCTION__, __LINE__);
                }
                /* then we need to get the message and print it */
                if (ISI_RETURN_OK == ISI_getFileHeader(id, subject, from)) {
                    OSAL_logMsg("%s:%d ISI_getFileHeader() returned subject: %s from: %s\n", __FUNCTION__, __LINE__, subject, from);
                }
                else {
                    OSAL_logMsg("%s:%d ISI_getFileHeader() failed\n", __FUNCTION__, __LINE__);
                }
                /* then we can also get the file progress info and print it */
                ret = ISI_readFileProgress(
                        id, filePath, (ISI_FileType *)&arg0, (ISI_FileAttribute *)&arg1, &fileSize, &fileProgress);
                if (ret == ISI_RETURN_OK) {
                    IUT_prtFileProgress(from, 0, filePath, arg0, arg1, fileSize, fileProgress, subject);
                }
                else if (ret == ISI_RETURN_DONE) {
                    IUT_prtFileProgress(from, 0, filePath, arg0, arg1, fileSize, fileProgress, subject);
                }
                break;
            case ISI_EVENT_FILE_PROGRESS:
                /* get the message header again so that we have the "from" and "subject" fields to print */
                if (ISI_RETURN_OK == ISI_getFileHeader(id, subject, from)) {
                    OSAL_logMsg("%s:%d ISI_getFileHeader() returned subject: %s from: %s\n", __FUNCTION__, __LINE__, subject, from);
                }
                else {
                    OSAL_logMsg("%s:%d ISI_getFileHeader() failed\n", __FUNCTION__, __LINE__);
                }

                ret = ISI_readFileProgress(
                        id, filePath, (ISI_FileType *)&arg0, (ISI_FileAttribute *)&arg1, &fileSize, &fileProgress);
                if (ret == ISI_RETURN_OK) {
                    IUT_prtFileProgress(from, 0, filePath, arg0, arg1, fileSize, fileProgress, subject);
                }
                else if (ret == ISI_RETURN_DONE) {
                    IUT_prtFileProgress(from, 0, filePath, arg0, arg1, fileSize, fileProgress, subject);
                }
                break;
            case ISI_EVENT_FILE_COMPLETED:
                OSAL_logMsg("%s:%d ISI_EVENT_FILE_COMPLETED event\n", __FUNCTION__, __LINE__);
                break;
            case ISI_EVENT_FILE_ACKNOWLEDGED:
                OSAL_logMsg("%s:%d ISI_EVENT_FILE_ACKNOWLEDGED event\n", __FUNCTION__, __LINE__);
                break;
            case ISI_EVENT_FILE_ACCEPTED:
                OSAL_logMsg("%s:%d ISI_EVENT_FILE_ACCEPTED event\n", __FUNCTION__, __LINE__);
                break;
            case ISI_EVENT_FILE_REJECTED:
                OSAL_logMsg("%s:%d ISI_EVENT_FILE_REJECTED event\n", __FUNCTION__, __LINE__);
                break;
            case ISI_EVENT_FILE_FAILED:
                OSAL_logMsg("%s:%d ISI_EVENT_FILE_FAILED event\n", __FUNCTION__, __LINE__);
                break;
            case ISI_EVENT_FILE_CANCELLED:
                OSAL_logMsg("%s:%d ISI_EVENT_FILE_CANCELLED event\n", __FUNCTION__, __LINE__);
                break;
            case ISI_EVENT_FILE_TRYING:
                OSAL_logMsg("%s:%d ISI_EVENT_FILE_TRYING event\n", __FUNCTION__, __LINE__);
                break;
            default:
                break;
        }
    }
    else if (idType == ISI_ID_TYPE_PRESENCE) {
        if (event == ISI_EVENT_CONTACT_RECEIVED) {
            /* Then we got a roster entry */
            _IUT_appRosterInsert(hs_ptr, id);
        }
        else if (event == ISI_EVENT_PRES_RECEIVED) {
            /* Then we got a presence state update */
            _IUT_appPresenceUpdate(hs_ptr, id);
        }
        else if (event == ISI_EVENT_SUB_TO_PRES_RECEIVED) {
            /* Then we got a request to subscribe to our presence */
            _IUT_appSubAddRequest(hs_ptr, id);
        }
        else if (event == ISI_EVENT_SUBSCRIPTION_RESP_RECEIVED) {
            /* Then we got a response to a previous presence request we sent */
            _IUT_appSubUpdate(hs_ptr, id);
        }
    }
    else if(idType == ISI_ID_TYPE_TEL_EVENT) {
        if (event == ISI_EVENT_TEL_EVENT_RECEIVED) {
            /* Then we got a new tel event.  Get it and print it */
            ISI_getTelEventFromRemote(id, &callId, &telEvt, &arg0, &arg1,
                    message, dateTime);
            IUT_prtTelEvt(telEvt, message, dateTime, arg0, arg1);
        }
    }
    else if (idType == ISI_ID_TYPE_CALL) {
        switch (event) {
        case ISI_EVENT_CALL_INCOMING:
            /* new call so show caller ID and cache the callId */
            if (ISI_RETURN_OK == ISI_getCallHeader(id, subject, from)) {
                IUT_prtCallerId(from, subject);
            }
            /* Find a place for it */
            for(x = 0 ; x < IUT_MAX_LINES ; x++) {
                if (hs_ptr->callId[x] == 0) {
                    hs_ptr->callId[x] = id;
                    break;
                }
            }
            break;
        case ISI_EVENT_CALL_REJECTED:
        case ISI_EVENT_CALL_FAILED:
        case ISI_EVENT_NET_UNAVAILABLE:
        case ISI_EVENT_CALL_DISCONNECTED:
            /* Then the call is over so clear the ID */
            for(x = 0 ; x < IUT_MAX_LINES ; x++) {
                if (hs_ptr->callId[x] == id) {
                    hs_ptr->callId[x] = 0;
                    break;
                }
            }
            break;
        case ISI_EVENT_CALL_HANDOFF:
            IUT_appCallHandoff(hs_ptr, id);
            break;
        case ISI_EVENT_CALL_MODIFY:
            IUT_prtCallModify(hs_ptr, id);
            break;
        case ISI_EVENT_CALL_MODIFY_FAILED:
        case ISI_EVENT_CALL_MODIFY_COMPLETED:
            IUT_prtCallModifyResult(hs_ptr, event, id);
            break;
        default:
            break;
        } /* End if switch */
    }
    else if (idType == ISI_ID_TYPE_CHAT) {
        
        //if (ISI_RETURN_OK == ISI_getGroupChatHeader(id, subject, from, room)) {
        //    IUT_prtGroupChat(id, subject, from);
        //}
        if (ISI_RETURN_OK == ISI_getChatHeader(id, subject, from, &isiId)) {
            IUT_prtGroupChat(id, subject, from);
        }

        if (event == ISI_EVENT_GROUP_CHAT_INCOMING || event == ISI_EVENT_CHAT_INCOMING) {
            /* Find a place for it */
            for(x = 0 ; x < IUT_MAX_LINES ; x++) {
                if (hs_ptr->chatRoomId[x] == 0) {
                    hs_ptr->chatRoomId[x] = id;
                    break;
                }
            }

            if (0 != isiId) {
                if (ISI_getMessageHeader(isiId, &type, subject, from, dateTime, &reports, reportId) == ISI_RETURN_OK) {
                    ret = ISI_RETURN_OK;
                    while (ret != ISI_RETURN_DONE && ret != ISI_RETURN_FAILED) {
                        bytes = IUT_STR_SIZE;
                        ret = ISI_readMessage(isiId, &chatId, message, &bytes);
                        IUT_prtIm(from, chatId, subject, message, dateTime, reports, reportId);
                    }
                    /* Send back a delivery report if need be */
                    if (0 != (reports & ISI_MSG_RPT_DELIVERY_SUCCESS)) {
                        ISI_sendMessageReport(&isiId, serviceId, from,
                                ISI_MSG_RPT_DELIVERY_SUCCESS, reportId);
                    }
                    if (0 != (reports & ISI_MSG_RPT_DISPLAY_SUCCESS)) {
                        ISI_sendMessageReport(&isiId, serviceId, from,
                                ISI_MSG_RPT_DISPLAY_SUCCESS, reportId);
                    }
                }
            }

        }
        else if (event == ISI_EVENT_CHAT_ACCEPTED) {
            // Nothing to do, do whatever you want
        }
        else if (event == ISI_EVENT_CHAT_DISCONNECTED || event == ISI_EVENT_CHAT_DISCONNECTED) {
            /* Then the chat is over so clear the ID */
            for(x = 0 ; x < IUT_MAX_LINES ; x++) {
                if (hs_ptr->chatRoomId[x] == id) {
                    hs_ptr->chatRoomId[x] = 0;
                    break;
                }
            }
        }
        else if (event == ISI_EVENT_CHAT_FAILED || event == ISI_EVENT_CHAT_FAILED) {
            /* Then the call is over so clear the ID */
            for(x = 0 ; x < IUT_MAX_LINES ; x++) {
                if (hs_ptr->chatRoomId[x] == id) {
                    hs_ptr->chatRoomId[x] = 0;
                    break;
                }
            }
        }

    }
    return;
}

/*
 * ======== IUT_appServiceAlloc() ========
 *
 * This function will initialize locally defined objects used to manage ISI
 * service and call the appropriate ISI API to alloc a service.
 *
 * Returns:
 *   IUT_OK  : The service was successfully created/initialized
 *   IUT_ERR : The service could not be allocated/initialized.
 *             The maximum number of available services configured
 *             in ISI has probably been exceeded.
 *
 */
vint IUT_appServiceAlloc(
    IUT_HandSetObj *hs_ptr,
    vint            proto)
{
    ISI_Return r;

    /* Now init a service */

    OSAL_memSet(hs_ptr, 0, sizeof(IUT_HandSetObj));
    /* Set a subject */
    OSAL_strncpy(hs_ptr->subject, "D2 Rocks", IUT_STR_SIZE);
    /* Set the default tone duration */
    hs_ptr->toneDuration = IUT_TONE_DURATION_MS;

    hs_ptr->proto = proto;
    r = ISI_allocService(&hs_ptr->serviceId, proto);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not allocate service ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    OSAL_logMsg("A ISI service was allocated\n");
    return (IUT_OK);
}

/*
 * ======== IUT_appServiceFree() ========
 *
 * This function frees any resource previously allocated and initialized to
 * manage an ISI service
 *
 * Returns:
 *   IUT_OK  : The service was successfully "freed".
 *   IUT_ERR : The service could not be "freed". The service was more
 *             than likely already freed or never initialized in the
 *             first place.
 */
vint IUT_appServiceFree(
    IUT_HandSetObj *hs_ptr)
{
    ISI_Return r;

    r = ISI_freeService(hs_ptr->serviceId);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not free service ERROR:%s\n", IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    OSAL_logMsg("A ISI service was freed\n");
    hs_ptr->serviceId = 0;
    return (IUT_OK);
}

/*
 * ======== IUT_appInit() ========
 *
 * This function is a wrapper for initializing the ISI module.
 *
 * Returns:
 *   IUT_OK  : ISI has been initialized
 *   IUT_ERR : Could not initialize ISI
 *
 */
vint IUT_appInit(void)
{
    ISI_Return r;

    if(_IUT_init) {
        OSAL_logMsg("ISI already initialized\n");
        return(IUT_OK);
    }
    r = ISI_init(NULL);
    if ((ISI_RETURN_OK != r) || (ISI_RETURN_OK_4G_PLUS != r)) {
        OSAL_logMsg("The ISI module could not be initialized ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    _IUT_init = 1;
    OSAL_logMsg("The ISI module was successfully initialized\n");

    return (IUT_OK);
}

/*
 * ======== IUT_appShutdown() ========
 *
 * This function is a wrapper for shutting down the ISI module.
 *
 * Returns:
 *   IUT_OK  : ISI has been shutdown.
 *   IUT_ERR : ISI could not be shutdown.
 *
 */
vint IUT_appShutdown(void)
{
    ISI_Return r;

    if (!_IUT_init) {
        OSAL_logMsg("ISI already shutdown\n");
        return (IUT_OK);
    }
    OSAL_logMsg("ISI being shutdown\n");
    r = ISI_shutdown();
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("ISI COULD NOT be shutdown\n");
        return (IUT_ERR);
    }
    OSAL_logMsg("ISI shutdown complete\n");
    _IUT_init = 0;
    return (IUT_OK);
}

/*
 * ======== IUT_appConfAdd() ========
 *
 * This function will command the ISI to conference 2 calls together.
 *
 * Returns:
 *   IUT_OK  : The ISI API call to start a conference was successful
 *   IUT_ERR : The ISI API call failed and the conference didn't get initiated.
 *
 */
vint IUT_appConfAdd(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    IUT_HandSetObj *hs2Add_ptr,
    vint            peer2Add)
{
    ISI_Return r;

    r = ISI_startConfCall(&hs_ptr->confId, hs_ptr->callId[peer],
            hs2Add_ptr->callId[peer2Add]);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not conference in a call ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

/*
 * ======== IUT_appConfRemove() ========
 *
 * This function will command the ISI to remove calls from a conference.
 * This function will call the ISI API to remove all participants for
 * a call conference.
 *
 * Returns:
 *   IUT_OK  : Always.
 *
 */
vint IUT_appConfRemove(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    IUT_HandSetObj *hs2Rm_ptr,
    vint            peer2Rm)
{
    ISI_Return r;
    //vint       peer2;
    //peer2 = (peer == 0) ? 1 : 0;

    /* Remove all calls from the conference */
    r = ISI_removeCallFromConf(hs_ptr->confId, hs_ptr->callId[peer]);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not remove a call in a conference ERROR:%s\n",
                IUT_prtReturnString(r));
    }
    r = ISI_removeCallFromConf(hs_ptr->confId, hs2Rm_ptr->callId[peer2Rm]);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not remove a call in a conference ERROR:%s\n",
                IUT_prtReturnString(r));
    }
    /* All is good so the call conference resource should be destroyed */
    hs_ptr->confId = 0;
    return (IUT_OK);
}

/*
 * ======== IUT_appCallBlindTransfer() ========
 *
 * This function will command the ISI to "blindly" transfer a call.
 * A blind call transfer is one where the transferor does not
 * care about the outcome or status of a call that was transferred.
 * Rather, it simply request the call to be transferred and then
 * disconnects.
 *
 * Returns:
 *   IUT_OK  : The ISI API call to perform the call transfer was successful.
 *   IUT_ERR : The ISI API failed, no call transfer was performed.
 *
 */
vint IUT_appCallBlindTransfer(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    char           *to_ptr)
{
    ISI_Return r;
    r = ISI_blindTransferCall(hs_ptr->callId[peer], to_ptr);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not transfer call ERROR:%s\n", IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

/*
 * ======== IUT_appCallAttendTransfer() ========
 *
 * This function will command the ISI to perform an "attended call transfer".
 * An attended call transfer is one where the transferor is kept aware
 * of the outcome or status of a call that was transferred.  Meaning, the
 * transferor will be alerted whether or not the call transfer was
 * successful.
 *
 * Returns:
 *   IUT_OK  : The ISI API call to perform the call transfer was successful.
 *   IUT_ERR : The ISI API failed, no call transfer was performed.
 *
 */
vint IUT_appCallAttendTransfer(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    char           *to_ptr)
{
    ISI_Return r;
    r = ISI_attendedTransferCall(hs_ptr->callId[peer], to_ptr);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not transfer call ERROR:%s\n", IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

vint IUT_appCallConsultativeTransfer(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    vint            targetPeer)
{
    ISI_Return r;
    r = ISI_consultativeTransferCall(hs_ptr->callId[peer], hs_ptr->callId[targetPeer]);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not transfer call ERROR:%s\n", IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

/*
 * ======== IUT_appCallForward() ========
 *
 * This function will command the ISI to forward an existing call attempt
 * to another remote entity specified in to_ptr.
 * This function is called when the user wants to "forward a call".
 *
 * Returns:
 *   IUT_OK  : The ISI API call to perform the call forward was successful.
 *   IUT_ERR : The ISI API failed, no call forward was performed.
 *
 */
vint IUT_appCallForward(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    char           *to_ptr)
{
    ISI_Return r;
    r = ISI_forwardCall(hs_ptr->callId[peer], to_ptr);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not forward call ERROR:%s\n", IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    hs_ptr->callId[peer] = 0;
    return (IUT_OK);
}

static vint IUT_addToXmlAudioEnabledSendRecv(
        char *xml_ptr,
        char *line_ptr,
        vint *len_ptr)
{
    vint            len = *len_ptr;

    /* Create the audio media. enabled true and direction sendrecv. */
    OSAL_strncpy(line_ptr, "<audio enabled=\"true\" direction=\"sendrecv\"/>\r\n",
            IUT_STR_SIZE);
    OSAL_strncpy(xml_ptr + len, line_ptr, IUT_MAX_IM_MSG_SIZE);
    len += OSAL_strlen(line_ptr);

    /* Update the length. */
    *len_ptr = len;

    return ISI_RETURN_OK;
}

static vint IUT_addToXmlAudioEnabled(
        char *xml_ptr,
        char *line_ptr,
        vint *len_ptr)
{
    vint            len = *len_ptr;

    /* Create the audio media. enabled true. */
    OSAL_strncpy(line_ptr, "<audio enabled=\"true\"/>\r\n",
            IUT_STR_SIZE);
    OSAL_strncpy(xml_ptr + len, line_ptr, IUT_MAX_IM_MSG_SIZE);
    len += OSAL_strlen(line_ptr);

    /* Update the length. */
    *len_ptr = len;

    return ISI_RETURN_OK;
}

static vint IUT_addToXmlAudioDisabled(
        char *xml_ptr,
        char *line_ptr,
        vint *len_ptr)
{
    vint            len = *len_ptr;

    /* Create the audio media. enabled false. */
    OSAL_strncpy(line_ptr, "<audio enabled=\"false\"/>\r\n",
            IUT_STR_SIZE);
    OSAL_strncpy(xml_ptr + len, line_ptr, IUT_MAX_IM_MSG_SIZE);
    len += OSAL_strlen(line_ptr);

    /* Update the length. */
    *len_ptr = len;

    return ISI_RETURN_OK;
}

static vint IUT_addToXmlVideoEnabledSendRecv(
        char *xml_ptr,
        char *line_ptr,
        vint *len_ptr)
{
    vint            len = *len_ptr;

    /* Create the video media. enabled true and direction sendrecv. */
    OSAL_strncpy(line_ptr, "<video enabled=\"true\" direction=\"sendrecv\"/>\r\n",
            IUT_STR_SIZE);
    OSAL_strncpy(xml_ptr + len, line_ptr, IUT_MAX_IM_MSG_SIZE);
    len += OSAL_strlen(line_ptr);

    /* Update the length. */
    *len_ptr = len;

    return ISI_RETURN_OK;
}

static vint IUT_addToXmlVideoEnabledRecvOnly(
        char *xml_ptr,
        char *line_ptr,
        vint *len_ptr)
{
    vint            len = *len_ptr;

    /* Create the video media. enabled true and direction recvonly. */
    OSAL_strncpy(line_ptr, "<video enabled=\"true\" direction=\"recvonly\"/>\r\n",
            IUT_STR_SIZE);
    OSAL_strncpy(xml_ptr + len, line_ptr, IUT_MAX_IM_MSG_SIZE);
    len += OSAL_strlen(line_ptr);

    /* Update the length. */
    *len_ptr = len;

    return ISI_RETURN_OK;
}

static vint IUT_addToXmlVideoDisabled(
        char *xml_ptr,
        char *line_ptr,
        vint *len_ptr)
{
    vint            len = *len_ptr;

    /* Create the audio media. enabled false. */
   OSAL_strncpy(line_ptr, "<video enabled=\"false\"/>\r\n",
           IUT_STR_SIZE);
   OSAL_strncpy(xml_ptr + len, line_ptr, IUT_MAX_IM_MSG_SIZE);
   len += OSAL_strlen(line_ptr);

    /* Update the length. */
    *len_ptr = len;

    return ISI_RETURN_OK;
}

static vint IUT_buildCallInitiateXml(
        char *xml_ptr,
        ISI_SessionType         type,
        ISI_SessionDirection    audioDirection,
        ISI_SessionDirection    videoDirection)
{
    char            line[IUT_STR_SIZE + 1];
    char            dirStr[IUT_ARG_SIZE + 1];
    vint            len = 0;

    /* Set the memory to zero. */
    OSAL_memSet(line, 0, IUT_STR_SIZE + 1);
    OSAL_memSet(xml_ptr, 0, IUT_MAX_IM_MSG_SIZE + 1);
    len = 0;

    /* Build the media root tag. */
    OSAL_strncpy(line, "<media>\r\n", IUT_STR_SIZE);
    OSAL_strncpy(xml_ptr + len, line, IUT_MAX_IM_MSG_SIZE);
    len += OSAL_strlen(line);

    /* Check if session type has audio bit mask set. */
    if (ISI_SESSION_TYPE_AUDIO & type) {
        /* Convert the audio direction vint to string. */
        switch (audioDirection) {
            case ISI_SESSION_DIR_INACTIVE:
                OSAL_strncpy(dirStr, "inactive", IUT_ARG_SIZE);
                break;
            case ISI_SESSION_DIR_SEND_ONLY:
                OSAL_strncpy(dirStr, "sendonly", IUT_ARG_SIZE);
                break;
            case ISI_SESSION_DIR_RECV_ONLY:
                OSAL_strncpy(dirStr, "recvonly", IUT_ARG_SIZE);
                break;
            case ISI_SESSION_DIR_SEND_RECV:
                OSAL_strncpy(dirStr, "sendrecv", IUT_ARG_SIZE);
                break;
            default:
                OSAL_strncpy(dirStr, "inactive", IUT_ARG_SIZE);
                break;
        }

        /* Create the audio media. */
        OSAL_snprintf(line, IUT_STR_SIZE, "%s%s%s",
                "<audio enabled=\"true\" direction=\"", dirStr, "\"/>\r\n");
        OSAL_strncpy(xml_ptr + len, line, IUT_MAX_IM_MSG_SIZE);
        len += OSAL_strlen(line);
    }

    /* Check if session type has video bit mask set. */
    if (ISI_SESSION_TYPE_VIDEO & type) {
        /* Convert the video direction vint to string. */
        switch (videoDirection) {
            case ISI_SESSION_DIR_INACTIVE:
                OSAL_strncpy(dirStr, "inactive", IUT_ARG_SIZE);
                break;
            case ISI_SESSION_DIR_SEND_ONLY:
                OSAL_strncpy(dirStr, "sendonly", IUT_ARG_SIZE);
                break;
            case ISI_SESSION_DIR_RECV_ONLY:
                OSAL_strncpy(dirStr, "recvonly", IUT_ARG_SIZE);
                break;
            case ISI_SESSION_DIR_SEND_RECV:
                OSAL_strncpy(dirStr, "sendrecv", IUT_ARG_SIZE);
                break;
            default:
                OSAL_strncpy(dirStr, "inactive", IUT_ARG_SIZE);
                break;
        }

        /* Create the video media. */
        OSAL_snprintf(line, IUT_STR_SIZE, "%s%s%s",
                "<video enabled=\"true\" direction=\"", dirStr, "\"/>\r\n");
        OSAL_strncpy(xml_ptr + len, line, IUT_MAX_IM_MSG_SIZE);
        len += OSAL_strlen(line);
    }

    /* Close the media root tag. */
    OSAL_strncpy(line, "</media>\r\n", IUT_STR_SIZE);
    OSAL_strncpy(xml_ptr + len, line, IUT_MAX_IM_MSG_SIZE);
    len += OSAL_strlen(line);

    return ISI_RETURN_OK;
}

/*
 * ======== IUT_appCall() ========
 *
 * This function will retrieve the "active contact" set by the user
 * and then place a call via the ISI PAI to that active contact.
 *
 * Returns:
 *   CallId  : The call ID of the new call.
 *   '0' (zero) : The call could not be placed.
 *
 */
ISI_Id IUT_appCall(
    IUT_HandSetObj         *hs_ptr,
    vint                    peer,
    char                   *to_ptr,
    ISI_SessionType         type,
    ISI_SessionDirection    audioDirection,
    ISI_SessionDirection    videoDirection)
{
    ISI_Return r;
    char       jid[IUT_STR_SIZE + 1];
    char       xml[IUT_MAX_IM_MSG_SIZE + 1];

    /* Search for the full jid in the roster */
    if (_IUT_appRosterFindJid(hs_ptr, to_ptr, jid, IUT_STR_SIZE) ==
            IUT_OK) {
        /* Then we have a full jid, try to make a call using that */
        to_ptr = jid;
    }
    /* Otherwise we couldn't find a full jid, so try the to_ptr anyway */

    if (hs_ptr->callId[peer] == 0) {
        /* Build call initiate xml. */
        IUT_buildCallInitiateXml(xml, type, audioDirection, videoDirection);

        /*
         * Note: set session type to AV because selection of coders will
         * dictate if video or audio session or both are required.
         * However, user can disable for example a video call even if video
         * coder is enabled a video call by not oring VIDEO session type.
         */
        r = ISI_initiateCall(&hs_ptr->callId[peer], hs_ptr->serviceId,
                to_ptr, hs_ptr->subject, ISI_SESSION_CID_TYPE_NONE, xml);
        if (r != ISI_RETURN_OK) {
            OSAL_logMsg("Could not initiate a call ERROR:%s\n",
                    IUT_prtReturnString(r));
            return (0);
        }
        return (hs_ptr->callId[peer]);
    }
    OSAL_logMsg("Call currently exists on this peer\n");
    return (0);
}

static vint IUT_buildCallModifyVideoXml(
        char *xml_ptr)
{
    char            line[IUT_STR_SIZE + 1];
    vint            len = 0;

    /* Set the memory to zero. */
    OSAL_memSet(line, 0, IUT_STR_SIZE + 1);
    OSAL_memSet(xml_ptr, 0, IUT_MAX_IM_MSG_SIZE + 1);
    len = 0;

    /* Build the media root tag. */
    OSAL_strncpy(line, "<media>\r\n", IUT_STR_SIZE);
    OSAL_strncpy(xml_ptr + len, line, IUT_MAX_IM_MSG_SIZE);
    len += OSAL_strlen(line);

    /* Add Audio enabled, sendrecv. */
    IUT_addToXmlAudioEnabledSendRecv(xml_ptr, line, &len);

    /* Add Video enabled, sendrecv. */
    IUT_addToXmlVideoEnabledSendRecv(xml_ptr, line, &len);

    /* Close the media root tag. */
    OSAL_strncpy(line, "</media>\r\n", IUT_STR_SIZE);
    OSAL_strncpy(xml_ptr + len, line, IUT_MAX_IM_MSG_SIZE);
    len += OSAL_strlen(line);

    return ISI_RETURN_OK;
}

/*
 * ======== IUT_appCallModifyVideo() ========
 *
 * This function will command the ISI to modify an existing call to audio+video
 *
 * Returns:
 *   IUT_OK  : The ISI API call to perform the call modify was successful.
 *   IUT_ERR : The ISI API failed, no call modification was performed.
 *
 */
vint IUT_appCallModifyVideo(
    IUT_HandSetObj *hs_ptr,
    vint            peer)
{
    ISI_Return r;
    char            xml[IUT_MAX_IM_MSG_SIZE + 1];

    /* Build xml that contains both audio and video enabled and direction sendrecv. */
    IUT_buildCallModifyVideoXml(xml);

    r = ISI_updateCallSession(hs_ptr->callId[peer], xml);

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not update call session ERROR:%s\n", IUT_prtReturnString(r));
        return (IUT_ERR);
    }

    return (IUT_appCallModify(hs_ptr, peer));
}

static vint IUT_buildCallModifyAudioOnlyXml(
        char *xml_ptr)
{
    char            line[IUT_STR_SIZE + 1];
    vint            len = 0;

    /* Set the memory to zero. */
    OSAL_memSet(line, 0, IUT_STR_SIZE + 1);
    OSAL_memSet(xml_ptr, 0, IUT_MAX_IM_MSG_SIZE + 1);
    len = 0;

    /* Build the media root tag. */
    OSAL_strncpy(line, "<media>\r\n", IUT_STR_SIZE);
    OSAL_strncpy(xml_ptr + len, line, IUT_MAX_IM_MSG_SIZE);
    len += OSAL_strlen(line);

    /* Add Audio enabled, sendrecv. */
    IUT_addToXmlAudioEnabledSendRecv(xml_ptr, line, &len);

    /* Add Video disabled. */
    IUT_addToXmlVideoDisabled(xml_ptr, line, &len);

    /* Close the media root tag. */
    OSAL_strncpy(line, "</media>\r\n", IUT_STR_SIZE);
    OSAL_strncpy(xml_ptr + len, line, IUT_MAX_IM_MSG_SIZE);
    len += OSAL_strlen(line);

    return ISI_RETURN_OK;
}

/*
 * ======== IUT_appCallModifyAudio() ========
 *
 * This function will command the ISI to modify an existing call to audio only
 *
 * Returns:
 *   IUT_OK  : The ISI API call to perform the call modify was successful.
 *   IUT_ERR : The ISI API failed, no call modification was performed.
 *
 */
vint IUT_appCallModifyAudio(
    IUT_HandSetObj *hs_ptr,
    vint            peer)
{
    ISI_Return r;
    char            xml[IUT_MAX_IM_MSG_SIZE + 1];

    /* Build xml that contains audio enabled and direction sendrecv. */
    IUT_buildCallModifyAudioOnlyXml(xml);

    r = ISI_updateCallSession(hs_ptr->callId[peer], xml);

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not update call session ERROR:%s\n", IUT_prtReturnString(r));
        return (IUT_ERR);
    }

    return (IUT_appCallModify(hs_ptr, peer));
}

/*
 * ======== IUT_appCallModifyAccept() ========
 *
 * This function will command the ISI to accept the call modify
 *
 * Returns:
 *   IUT_OK  : The ISI API call to perform the call modify was successful.
 *   IUT_ERR : The ISI API failed, no call modification was performed.
 *
 */
vint IUT_appCallModifyAccept(
    IUT_HandSetObj *hs_ptr,
    vint            peer)
{
    ISI_Return r;

    r = ISI_acceptCallModify(hs_ptr->callId[peer]);

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not accept call modify ERROR:%s\n", IUT_prtReturnString(r));
        return (IUT_ERR);
    }

    return (IUT_OK);
}

static vint IUT_buildCallModifyRejectXml(
        char *xml_ptr)
{
    char            line[IUT_STR_SIZE + 1];
    vint            len = 0;

    /* Set the memory to zero. */
    OSAL_memSet(line, 0, IUT_STR_SIZE + 1);
    OSAL_memSet(xml_ptr, 0, IUT_MAX_IM_MSG_SIZE + 1);
    len = 0;

    /* Build the media root tag. */
    OSAL_strncpy(line, "<media>\r\n", IUT_STR_SIZE);
    OSAL_strncpy(xml_ptr + len, line, IUT_MAX_IM_MSG_SIZE);
    len += OSAL_strlen(line);

    /* Enable Audio media. */
    IUT_addToXmlAudioEnabled(xml_ptr, line, &len);

    /* Disable Video media. */
    IUT_addToXmlVideoDisabled(xml_ptr, line, &len);

    /* Close the media root tag. */
    OSAL_strncpy(line, "</media>\r\n", IUT_STR_SIZE);
    OSAL_strncpy(xml_ptr + len, line, IUT_MAX_IM_MSG_SIZE);
    len += OSAL_strlen(line);

    return ISI_RETURN_OK;
}

/*
 * ======== IUT_appCallModifyReject() ========
 *
 * This function will command the ISI to reject the call modify
 *
 * Returns:
 *   IUT_OK  : The ISI API call to perform the call modify was successful.
 *   IUT_ERR : The ISI API failed, no call modification was performed.
 *
 */
vint IUT_appCallModifyReject(
    IUT_HandSetObj *hs_ptr,
    vint            peer)
{
    ISI_Return r;
    char            xml[IUT_MAX_IM_MSG_SIZE + 1];

    /* Build xml that contains audio enabled. */
    IUT_buildCallModifyRejectXml(xml);

    r = ISI_updateCallSession(hs_ptr->callId[peer], xml);

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not update call session ERROR:%s\n", IUT_prtReturnString(r));
        return (IUT_ERR);
    }

    r = ISI_rejectCallModify(hs_ptr->callId[peer], "User reject call modify");

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not reject call modify ERROR:%s\n", IUT_prtReturnString(r));
        return (IUT_ERR);
    }

    return (IUT_OK);
}

static vint IUT_buildCallModifyDirectionXml(
        char *xml_ptr,
        vint type,
        vint dir)
{
    char            line[IUT_STR_SIZE + 1];
    char            dirStr[IUT_ARG_SIZE + 1];
    vint            len = 0;

    /* Set the memory to zero. */
    OSAL_memSet(line, 0, IUT_STR_SIZE + 1);
    OSAL_memSet(xml_ptr, 0, IUT_MAX_IM_MSG_SIZE + 1);
    len = 0;

    /* Build the media root tag. */
    OSAL_strncpy(line, "<media>\r\n", IUT_STR_SIZE);
    OSAL_strncpy(xml_ptr + len, line, IUT_MAX_IM_MSG_SIZE);
    len += OSAL_strlen(line);

    /* Convert the dir vint to string. */
    switch (dir) {
        case ISI_SESSION_DIR_INACTIVE:
            OSAL_strncpy(dirStr, "inactive", IUT_ARG_SIZE);
            break;
        case ISI_SESSION_DIR_SEND_ONLY:
            OSAL_strncpy(dirStr, "sendonly", IUT_ARG_SIZE);
            break;
        case ISI_SESSION_DIR_RECV_ONLY:
            OSAL_strncpy(dirStr, "recvonly", IUT_ARG_SIZE);
            break;
        case ISI_SESSION_DIR_SEND_RECV:
            OSAL_strncpy(dirStr, "sendrecv", IUT_ARG_SIZE);
            break;
        default:
            OSAL_strncpy(dirStr, "inactive", IUT_ARG_SIZE);
            break;
    }

    if (type == ISI_SESSION_TYPE_AUDIO) {
        /* Create the audio media. */
        OSAL_snprintf(line, IUT_STR_SIZE, "%s%s%s",
                "<audio direction=\"", dirStr, "\"/>\r\n");
        OSAL_strncpy(xml_ptr + len, line, IUT_MAX_IM_MSG_SIZE);
        len += OSAL_strlen(line);
    }
    else {
        /* Create the video media. */
       OSAL_snprintf(line, IUT_STR_SIZE, "%s%s%s",
               "<video direction=\"", dirStr, "\"/>\r\n");
       OSAL_strncpy(xml_ptr + len, line, IUT_MAX_IM_MSG_SIZE);
       len += OSAL_strlen(line);
    }

    /* Close the media root tag. */
    OSAL_strncpy(line, "</media>\r\n", IUT_STR_SIZE);
    OSAL_strncpy(xml_ptr + len, line, IUT_MAX_IM_MSG_SIZE);
    len += OSAL_strlen(line);

    return ISI_RETURN_OK;
}

/*
 * ======== IUT_appCallModifyDir() ========
 *
 * This function will command the ISI to modify the direction an existing call
 *
 * Returns:
 *   IUT_OK  : The ISI API call to perform the call modify was successful.
 *   IUT_ERR : The ISI API failed, no call modification was performed.
 *
 */
vint IUT_appCallModifyDir(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    vint            type,
    vint            dir)
{
    ISI_Return r;
    char            xml[IUT_MAX_IM_MSG_SIZE + 1];

    /* Build xml that Modifies appropriate media dir. */
    IUT_buildCallModifyDirectionXml(xml, type, dir);

    r = ISI_updateCallSession(hs_ptr->callId[peer], xml);

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not update call session ERROR:%s\n", IUT_prtReturnString(r));
        return (IUT_ERR);
    }

    r = ISI_modifyCall(hs_ptr->callId[peer], NULL);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not modify call ERROR:%s\n", IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

/*
 * ======== IUT_appCallModify() ========
 *
 * This function will command the ISI to modify an existing call.
 *
 * Returns:
 *   IUT_OK  : The ISI API call to perform the call modify was successful.
 *   IUT_ERR : The ISI API failed, no call modification was performed.
 *
 */
vint IUT_appCallModify(
    IUT_HandSetObj *hs_ptr,
    vint            peer)
{
    ISI_Return r;
    r = ISI_modifyCall(hs_ptr->callId[peer], NULL);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not modify call ERROR:%s\n", IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

/*
 * ======== IUT_appCallTerminate() ========
 *
 * This function will command the ISI to terminate an existing call.
 *
 * Returns:
 *   IUT_OK  : The ISI API call to perform the termination  was successful.
 *   IUT_ERR : The ISI API failed, the call was not terminated
 *
 */
vint IUT_appCallTerminate(
    IUT_HandSetObj *hs_ptr,
    vint            peer)
{
    ISI_Return r;
    r = ISI_terminateCall(hs_ptr->callId[peer], NULL);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not terminate call ERROR:%s\n", IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    hs_ptr->callId[peer] = 0;
    return (IUT_OK);
}

/*
 * ======== IUT_appCallAck() ========
 *
 * This function will command the ISI to acknowledge a call.
 * "acknowledging" a call is like saying, "The endpoint is ringing".
 *
 * Returns:
 *   IUT_OK  : The ISI API call to perform the ack  was successful.
 *   IUT_ERR : The ISI API failed, the call was not ack'ed
 *
 */
vint IUT_appCallAck(
    IUT_HandSetObj *hs_ptr,
    vint           peer)
{
    ISI_Return r;
    r = ISI_acknowledgeCall(hs_ptr->callId[peer]);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not acknowledge the call ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

/*
 * ======== IUT_appCallAcceptAudioOnly() ========
 *
 * This function will command the ISI to accept (answer) a 
 * audio only call.
 *
 * Returns:
 *   IUT_OK  : The ISI API call to answer the call was successful.
 *   IUT_ERR : The ISI API failed, the call was not answered.
 *
 */
vint IUT_appCallAcceptAudioOnly(
    IUT_HandSetObj *hs_ptr,
    vint            peer)
{
    ISI_Return r;
    char            xml[IUT_MAX_IM_MSG_SIZE + 1];

    /* Build xml that has audio only and video is disabled. */
    IUT_buildCallModifyAudioOnlyXml(xml);

    r = ISI_updateCallSession(hs_ptr->callId[peer], xml);

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not update call session ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }

    r = ISI_acceptCall(hs_ptr->callId[peer]);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not accept the call ERROR:%s\n", IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

static vint IUT_buildCallAcceptVideoSendOnlyXml(
        char *xml_ptr)
{
    char            line[IUT_STR_SIZE + 1];
    vint            len = 0;

    /* Set the memory to zero. */
    OSAL_memSet(line, 0, IUT_STR_SIZE + 1);
    OSAL_memSet(xml_ptr, 0, IUT_MAX_IM_MSG_SIZE + 1);
    len = 0;

    /* Build the media root tag. */
    OSAL_strncpy(line, "<media>\r\n", IUT_STR_SIZE);
    OSAL_strncpy(xml_ptr + len, line, IUT_MAX_IM_MSG_SIZE);
    len += OSAL_strlen(line);

    /* Disable Audio media. */
    IUT_addToXmlAudioDisabled(xml_ptr, line, &len);

    /* Enable Video media. Recv Only. */
    IUT_addToXmlVideoEnabledRecvOnly(xml_ptr, line, &len);

    /* Close the media root tag. */
    OSAL_strncpy(line, "</media>\r\n", IUT_STR_SIZE);
    OSAL_strncpy(xml_ptr + len, line, IUT_MAX_IM_MSG_SIZE);
    len += OSAL_strlen(line);

    return ISI_RETURN_OK;
}

/*
 * ======== IUT_appCallAcceptVideoSendOnly() ========
 *
 * This function will command the ISI to accept (answer) a 
 * video send only call.
 *
 * Returns:
 *   IUT_OK  : The ISI API call to answer the call was successful.
 *   IUT_ERR : The ISI API failed, the call was not answered.
 *
 */
vint IUT_appCallAcceptVideoSendOnly(
    IUT_HandSetObj *hs_ptr,
    vint            peer)
{
    ISI_Return r;
    char            xml[IUT_MAX_IM_MSG_SIZE + 1];

    /* Build xml that has video send only. */
    IUT_buildCallAcceptVideoSendOnlyXml(xml);

    r = ISI_updateCallSession(hs_ptr->callId[peer], xml);

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not update call session ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }

    r = ISI_acceptCall(hs_ptr->callId[peer]);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not accept the call ERROR:%s\n", IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

/*
 * ======== IUT_appCallAccept() ========
 *
 * This function will command the ISI to accept (answer) a call.
 *
 * Returns:
 *   IUT_OK  : The ISI API call to answer the call was successful.
 *   IUT_ERR : The ISI API failed, the call was not answered.
 *
 */
vint IUT_appCallAccept(
    IUT_HandSetObj *hs_ptr,
    vint            peer)
{
    ISI_Return r;
    char            xml[IUT_MAX_IM_MSG_SIZE + 1];

    /* Build xml that Modifies appropriate media dir. */
    IUT_buildCallModifyDirectionXml(xml, ISI_SESSION_TYPE_AUDIO, ISI_SESSION_DIR_SEND_RECV);
    r = ISI_updateCallSession(hs_ptr->callId[peer], xml);

    /* Build xml that Modifies appropriate media dir. */
    IUT_buildCallModifyDirectionXml(xml, ISI_SESSION_TYPE_VIDEO, ISI_SESSION_DIR_SEND_RECV);
    r = ISI_updateCallSession(hs_ptr->callId[peer], xml);

    r = ISI_acceptCall(hs_ptr->callId[peer]);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not accept the call ERROR:%s\n", IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

/*
 * ======== IUT_appCallReject() ========
 *
 * This function will command the ISI to reject an incoming call.
 *
 * Returns:
 *   IUT_OK  : The ISI API call to reject the call was successful.
 *   IUT_ERR : The ISI API failed, the call was not rejected.
 *
 */
vint IUT_appCallReject(
    IUT_HandSetObj *hs_ptr,
    vint            peer)
{
    ISI_Return r;
    r = ISI_rejectCall(hs_ptr->callId[peer], NULL);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not reject the call ERROR:%s\n", IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    hs_ptr->callId[peer] = 0;
    return (IUT_OK);
}

/*
 * ======== IUT_appCallHold() ========
 *
 * This function will command the ISI to "hold" a call.
 *
 * Returns:
 *   IUT_OK  : The ISI API call to hold a call was successful.
 *   IUT_ERR : The ISI API failed, the call was not held.
 *
 */
vint IUT_appCallHold(
    IUT_HandSetObj *hs_ptr,
    vint            peer)
{
    ISI_Return r;
    r = ISI_holdCall(hs_ptr->callId[peer]);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not place the call on hold ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

/*
 * ======== IUT_appCallResume() ========
 *
 * This function will command the ISI to take a call off "hold".
 *
 * Returns:
 *   IUT_OK  : The ISI API call to resume a call after holding was successful.
 *   IUT_ERR : The ISI API failed, resuming the call was NOT successful.
 *
 */
vint IUT_appCallResume(
    IUT_HandSetObj *hs_ptr,
    vint            peer)
{
    ISI_Return r;
    r = ISI_resumeCall(hs_ptr->callId[peer]);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not take the call off hold ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

/*
 * ======== IUT_appDigit() ========
 *
 * This function will command the ISI to send a DTMF digit within a call.
 *
 * Returns:
 *   IUT_OK  : The ISI API call to send a dtmf digit was successful.
 *   IUT_ERR : The ISI API failed, did not send a dtmf digit.
 *
 */
vint IUT_appDigit(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    char            digit,
    vint            isOob,
    vint            duration)
{
    ISI_Return   r;
    ISI_TelEvent t;

    t = (0 != isOob) ? ISI_TEL_EVENT_DTMF_OOB : ISI_TEL_EVENT_DTMF;

    r = ISI_sendTelEventToRemote(&hs_ptr->evtId, hs_ptr->callId[peer],
                t, (uint32)digit, duration);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not send a 'digit' tel event ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

vint IUT_appFlashhook(
    IUT_HandSetObj *hs_ptr,
    vint            peer)
{
    ISI_Return   r;
    ISI_TelEvent t;

    t = ISI_TEL_EVENT_FLASHHOOK;

    r = ISI_sendTelEventToRemote(&hs_ptr->evtId, hs_ptr->callId[peer],
                t, 0, 0);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not send a 'flash hook' tel event ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

/*
 * ======== IUT_appTone() ========
 *
 * This function will command the ISI to generate a local tone.
 *
 * Returns:
 *   IUT_OK  : The ISI API call to generate a tone was successful.
 *   IUT_ERR : The ISI API failed.
 *
 */
vint IUT_appTone(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    vint            tone,
    vint            duration)
{
    ISI_Return r;

    if (tone == ISI_TONE_LAST) {
        /* Then stop the tone */
        r = ISI_stopTone(hs_ptr->callId[peer]);
        if (r != ISI_RETURN_OK) {
            OSAL_logMsg("Could not stop tone. ERROR:%s\n", IUT_prtReturnString(r));
            return (IUT_ERR);
        }
    }
    else {
        /* Then generate the tone */
        r = ISI_generateTone(hs_ptr->callId[peer], tone, duration);
        if (r != ISI_RETURN_OK) {
            OSAL_logMsg("Could not generate tone. ERROR:%s\n",
                    IUT_prtReturnString(r));
            return (IUT_ERR);
        }
    }
    return (IUT_OK);
}

static void _IUT_generateRandomString(char *target_ptr, vint length) {
    static char _randomNumberCharTable[]=
            "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz.-";
    vint  x;
    uint8 c;
    OSAL_randomGetOctets(target_ptr, length);
    /* Now convert each byte to a character */
    for (x = 0 ; x <  length ; x++) {
        c = (uint8) target_ptr[x];
        c &= 0x3F; // Make the value 63 or less
        // Rewrite the value with a ascii char
        target_ptr[x] = _randomNumberCharTable[c];
    }
    /* NULL Terminate */
    target_ptr[x] = 0;
}

/*
 * ======== IUT_appMessageSend() ========
 *
 * This function will command the ISI to send a text message (a.k.a. IM)
 * outside the context of a call.
 *
 * Returns:
 *   IUT_OK  : The ISI API call to send the text message was successful.
 *   IUT_ERR : The ISI API failed, did not send the text message.
 *
 */
vint IUT_appMessageSend(
    IUT_HandSetObj *hs_ptr,
    char           *to_ptr,
    char           *msg_ptr,
    vint            deliveryReport,
    vint            displayReport)
{
    ISI_Return r;
    int report = 0;
    int msgLen = OSAL_strlen(msg_ptr);
    char reportId[IUT_STR_SIZE + 1] = { 0 };

    if (0 != deliveryReport) {
        report = ISI_MSG_RPT_DELIVERY_SUCCESS | ISI_MSG_RPT_DELIVERY_FAILED |
            ISI_MSG_RPT_DELIVERY_FORBIDDEN | ISI_MSG_RPT_DELIVERY_ERROR;
        _IUT_generateRandomString(reportId, 20);
    }

    if (0 != displayReport) {
        report |= ISI_MSG_RPT_DISPLAY_SUCCESS;
        if (0 == deliveryReport) {
            _IUT_generateRandomString(reportId, 20);
        }
    }

    r = ISI_sendMessage(&hs_ptr->textId,
            hs_ptr->serviceId,
            ISI_MSG_TYPE_TEXT, /* XXX Text only for now */
            to_ptr, hs_ptr->subject,
            msg_ptr, msgLen,
            report, reportId);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not send a text message ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

/*
 * ======== IUT_appCapabilitiesSet() ========
 *
 * This function will command the ISI to set capabilities xml value
 *
 * Returns:
 *   IUT_OK  : The ISI API call to send the text message was successful.
 *   IUT_ERR : The ISI API failed, did not send the text message.
 *
 */
vint IUT_appCapabilitiesSet(
    IUT_HandSetObj *hs_ptr,
    char           *xml_ptr)
{
    ISI_Return r;
    r = ISI_serviceSetCapabilities(hs_ptr->serviceId, xml_ptr);

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not set capabilities exchange ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

/*
 * ======== IUT_appCapabilitiesSend() ========
 *
 * This function will command the ISI to send capabilities 
 *
 * Returns:
 *   IUT_OK  : The ISI API call to send the text message was successful.
 *   IUT_ERR : The ISI API failed, did not send the text message.
 *
 */
vint IUT_appCapabilitiesSend(
    IUT_HandSetObj *hs_ptr,
    char           *to_ptr,
    vint            peer,
    char           *capabilities_ptr)
{
    ISI_Return r;
    r = ISI_sendCapabilities(&hs_ptr->capabilityId[peer], 
            hs_ptr->serviceId, to_ptr, 
            capabilities_ptr, 1);

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not send capabilities exchange ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

/*
 * ======== IUT_appSubscribe() ========
 *
 * This function will command the ISI to attempt to subscribe to a
 * remote entity's presence state.  If the attempt is successful it will
 * then  add the contact to the "roster" list.
 *
 * Returns:
 *   IUT_OK  : The ISI API call to subscribe to presence was successful.
 *             and the contact specified in the to_ptr was added to the Roster
 *             list.
 *   IUT_ERR : The ISI API failed, No subscription to presence was sent and the
 *             contact specified in the to_ptr was not added to the Roster list.
 *
 */
vint IUT_appSubscribe(
    IUT_HandSetObj *hs_ptr,
    char           *to_ptr)
{
    ISI_Return r;
    r = ISI_subscribeToPresence(&hs_ptr->presId, hs_ptr->serviceId, to_ptr);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not subscribe to %s ERROR:%s\n", to_ptr,
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    /* Add the entry to the roster list */
    _IUT_appSubAdd(hs_ptr, to_ptr, "Awaiting response to invitation");
    return (IUT_OK);
}

/*
 * ======== IUT_appUnsubscribe() ========
 *
 * This function will command the ISI to attempt to un-subscribe from a
 * remote entity's presence state.  If the attempt is successful it will
 * then update the status of the contact in the "roster" list to indicate
 * that we are unsubscribed from the contact.
 *
 * Returns:
 *   IUT_OK  : The ISI API call to subscribe to presence was successful.
 *             and the contact specified in the to_ptr was added to the Roster
 *             list.
 *   IUT_ERR : The ISI API failed, No subscription to presence was sent and the
 *             contact specified in the to_ptr was not added to the Roster
 *             list.
 *
 */
vint IUT_appUnsubscribe(
    IUT_HandSetObj *hs_ptr,
    char           *to_ptr)
{
    ISI_Return r;
    r = ISI_unsubscribeFromPresence(&hs_ptr->presId,
            hs_ptr->serviceId, to_ptr);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not unsubscribe to %s ERROR:%s\n", to_ptr,
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    _IUT_appSubAdd(hs_ptr, to_ptr, "We are unsubscribed from this contact");
    return (IUT_OK);
}

/*
 * ======== IUT_appSubscribeAllow() ========
 *
 * When a remote entity requests to us to see our presence state information
 * we can allow or deny that remote entity permission. This function is used
 * to command the ISI to allow or deny a request from a remote entity to see
 * our presence state.  If allow is '1' then we will allow that request, else
 * '0' means to deny.
 *
 * Returns:
 *   IUT_OK  : The ISI API call to allow/deny a subscription to presence
 *             was successful.
 *   IUT_ERR : The ISI API failed, could not allow or deny the request for
 *             subscription to presence
 *
 */
vint IUT_appSubscribeAllow(
    IUT_HandSetObj *hs_ptr,
    char           *to_ptr,
    vint            allow)
{
    ISI_Return r;
    if (allow) {
        r = ISI_allowSubscriptionToPresence(&hs_ptr->presId,
            hs_ptr->serviceId, to_ptr);
    }
    else {
        r= ISI_denySubscriptionToPresence(&hs_ptr->presId,
            hs_ptr->serviceId, to_ptr);
    }
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not allow/deny this subscription to %s ERROR:%s\n",
                to_ptr, IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

/*
 * ======== IUT_appPresenceSend() ========
 *
 * This function will construct and send a signalling messages used to update
 * a presence server with the presence state for this entity.
 *
 * Returns:
 *   IUT_OK  : The ISI API call to send presence state was successful.
 *   IUT_ERR : The ISI API failed, could not send our presence state.
 *
 */
vint IUT_appPresenceSend(
    IUT_HandSetObj *hs_ptr,
    vint            state,
    char           *status_ptr)
{
    ISI_Return r;
    char presence[256];

    if (state == 0) {
        OSAL_snprintf(presence, 256, "%s",
                "<show>away</show><priority>0</priority>");
    }
    else if (state == 1) {
        if (status_ptr) {
            OSAL_snprintf(presence, 256,
            "<show>available</show><priority>0</priority><status>%s</status>",
            status_ptr);
        }
        else {
            OSAL_snprintf(presence, 256, "%s",
                    "<show>available</show><priority>0</priority>");
        }
    }
    else {
        /* Then it's 'dnd' */
        if (status_ptr) {
            OSAL_snprintf(presence, 256,
            "<show>dnd</show><priority>0</priority><status>%s</status>",
            status_ptr);
        }
        else {
            OSAL_snprintf(presence, 256, "%s",
                    "<show>dnd</show><priority>0</priority>");
        }
    }

    r = ISI_sendPresence(&hs_ptr->presId, hs_ptr->serviceId, NULL, presence);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not send a presence state update ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

/*
 * ======== IUT_appSetCoders() ========
 *
 * This function is used to issue all the ISI commands to set the available
 * voice coders on a "service" bases, or "call" basis.
 *
 * Returns:
 *   Nothing.
 *
 */
void IUT_appSetCoders(
    IUT_HandSetObj *hs_ptr,
    vint            id,
    vint            pcmu,
    vint            ppcmu,
    vint            pcma,
    vint            ppcma,
    vint            g726,
    vint            pg726,
    vint            g729,
    vint            pg729,
    vint            iLBC,
    vint            piLBC,
    vint            silk,
    vint            psilk,
    vint            g722,
    vint            pg722,
    vint            dtmfr,
    vint            pdtmfr,
    vint            cn,
    vint            pcn,
    vint            h264,
    vint            ph264,
    vint            h263,
    vint            ph263,
    vint            amrnb,
    vint            pamrnb,
    vint            amrwb,
    vint            pamrwb)
{
    ISI_Id isiId;
    char desc[ISI_CODER_DESCRIPTION_STRING_SZ];
    int dynamicCoderValue = 98;

    /* The id is unused for now */
    if (id >= IUT_MAX_LINES) {
        /* Then set the coders in the service */
        isiId = hs_ptr->serviceId;
        if (pcmu) {
            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d", 0, 20);
            ISI_addCoderToService(isiId, "PCMU", desc);
        }
        else {
            ISI_removeCoderFromService(isiId, "PCMU");
        }
        if (pcma) {
            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d", 8, 20);
            ISI_addCoderToService(isiId, "PCMA", desc);
        }
        else {
            ISI_removeCoderFromService(isiId, "PCMA");
        }
        if (g729) {
            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d%s",
                    18, 20, cn ? ";annexb=yes" : "");
            ISI_addCoderToService(isiId, "G729", desc);
        }
        else {
            ISI_removeCoderFromService(isiId, "G729");
        }
        if (g726) {
            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d", 2, 20);
            ISI_addCoderToService(isiId, "G726-32", desc);
        }
        else {
            ISI_removeCoderFromService(isiId, "G726-32");
        }
        if (iLBC) {
            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d", dynamicCoderValue++, 20);
            ISI_addCoderToService(isiId, "ILBC", desc);
        }
        else {
            ISI_removeCoderFromService(isiId, "ILBC");
        }
        if (silk) {
            /* XXX 24kHz SILK not supported yet */
//            OSAL_snprintf(desc, sizeof(desc),
//                    "enum=%d;rate=%d", dynamicCoderValue++, 20);
//            ISI_addCoderToService(isiId, "silk-24k", desc);
            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d", dynamicCoderValue++, 20);
            ISI_addCoderToService(isiId, "silk-16k", desc);
            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d", dynamicCoderValue++, 20);
            ISI_addCoderToService(isiId, "silk-8k", desc);
        }
        else {
//            ISI_removeCoderFromService(isiId, "silk-24k");
            ISI_removeCoderFromService(isiId, "silk-16k");
            ISI_removeCoderFromService(isiId, "silk-8k");
        }
        if (g722) {
            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d", 9, 20);
            ISI_addCoderToService(isiId, "G722", desc);
        }
        else {
            ISI_removeCoderFromService(isiId, "G722");
        }
        if (cn) {
            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d", 13, 200);
            ISI_addCoderToService(isiId, "CN", desc);
        }
        else {
            ISI_removeCoderFromService(isiId, "CN");
        }
        if (h264) {
            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d;"
                    "profile-level-id=42e020;packetization-mode=1",
                    dynamicCoderValue++, 0);
            ISI_addCoderToService(isiId, "H264", desc);
        }
        else {
            ISI_removeCoderFromService(isiId, "H264");
        }
        if (h263) {
            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d;CIF=1", 34, 0);
            ISI_addCoderToService(isiId, "H263", desc);
        }
        else {
            ISI_removeCoderFromService(isiId, "H263");
        }
        if (amrnb) {
            /* Add both bandwidth-efficient and octet-align format. */
            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d", dynamicCoderValue++, 0);
            ISI_addCoderToService(isiId, "AMR", desc);
            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d;octet-align=1", dynamicCoderValue++, 0);
            ISI_addCoderToService(isiId, "AMR", desc);
        }
        else {
            ISI_removeCoderFromService(isiId, "AMR");
        }
        if (amrwb) {
            /* Add both bandwidth-efficient and octet-align format. */
            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d", dynamicCoderValue++, 0);
            ISI_addCoderToService(isiId, "AMR-WB", desc);
            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d;octet-align=1", dynamicCoderValue++, 0);
            ISI_addCoderToService(isiId, "AMR-WB", desc);
        }
        else {
            ISI_removeCoderFromService(isiId, "AMR-WB");
        }
        if (dtmfr) {
            OSAL_snprintf(desc, sizeof(desc),
                   "enum=%d;rate=%d", dynamicCoderValue++, 20);
            ISI_addCoderToService(isiId, "telephone-event-16k", desc);

            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d", dynamicCoderValue++, 20);
            ISI_addCoderToService(isiId, "telephone-event", desc);
        }
        else {
            ISI_removeCoderFromService(isiId, "telephone-event-16k");
            ISI_removeCoderFromService(isiId, "telephone-event");
        }
    }
    else {
        /* then it's for a particular call */
        isiId = hs_ptr->callId[id];
        if (pcmu) {
            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d", 0, 20);
            ISI_addCoderToCall(isiId, "PCMU", desc);
        }
        else {
            ISI_removeCoderFromCall(isiId, "PCMU");
        }
        if (pcma) {
            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d", 8, 20);
            ISI_addCoderToCall(isiId, "PCMA", desc);
        }
        else {
            ISI_removeCoderFromCall(isiId, "PCMA");
        }
        if (g729) {
            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d%s",
                    18, 20, cn ? ";annexb=yes" : "");
            ISI_addCoderToCall(isiId, "G729", desc);
        }
        else {
            ISI_removeCoderFromCall(isiId, "G729");
        }
        if (g726) {
            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d", 2, 20);
            ISI_addCoderToCall(isiId, "g726-32", desc);
        }
        else {
            ISI_removeCoderFromCall(isiId, "G726-32");
        }
        if (iLBC) {
            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d", dynamicCoderValue++, 20);
            ISI_addCoderToCall(isiId, "ILBC", desc);
        }
        else {
            ISI_removeCoderFromCall(isiId, "ILBC");
        }
        if (silk) {
            /* XXX 24kHz SILK not supported yet */
//            OSAL_snprintf(desc, sizeof(desc),
//                    "enum=%d;rate=%d", dynamicCoderValue++, 20);
//            ISI_addCoderToCall(isiId, "silk-24k", desc);
            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d", dynamicCoderValue++, 20);
            ISI_addCoderToCall(isiId, "silk-16k", desc);
            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d", dynamicCoderValue++, 20);
            ISI_addCoderToCall(isiId, "silk-8k", desc);
        }
        else {
//            ISI_removeCoderFromCall(isiId, "silk-24k");
            ISI_removeCoderFromCall(isiId, "silk-16k");
            ISI_removeCoderFromCall(isiId, "silk-8k");
        }
        if (g722) {
            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d", 9, 20);
            ISI_addCoderToCall(isiId, "G722", desc);
        }
        else {
            ISI_removeCoderFromCall(isiId, "silk-8k");
        }
        if (cn) {
            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d", 13, 200);
            ISI_addCoderToCall(isiId, "CN", desc);
        }
        else {
            ISI_removeCoderFromCall(isiId, "CN");
        }
        if (h264) {
            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d;"
                    "profile-level-id=42e020;packetization-mode=1",
                    dynamicCoderValue++, 0);
            ISI_addCoderToCall(isiId, "H264", desc);
        }
        else {
            ISI_removeCoderFromCall(isiId, "H264");
        }
        if (h263) {
            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d;CIF=1;QCIF=1", 34, 0);
            ISI_addCoderToCall(isiId, "H263", desc);
        }
        else {
            ISI_removeCoderFromCall(isiId, "H263");
        }
        if (amrnb) {
            /* Add both bandwidth-efficient and octet-align format. */
            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d", dynamicCoderValue++, 0);
            ISI_addCoderToCall(isiId, "AMR", desc);
            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d;octet-align=1", dynamicCoderValue++, 0);
            ISI_addCoderToCall(isiId, "AMR", desc);
        }
        else {
            ISI_removeCoderFromCall(isiId, "AMR");
        }
        if (amrwb) {
            /* Add both bandwidth-efficient and octet-align format. */
            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d", dynamicCoderValue++, 0);
            ISI_addCoderToCall(isiId, "AMR-WB", desc);
            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d;octet-align=1", dynamicCoderValue++, 0);
            ISI_addCoderToCall(isiId, "AMR-WB", desc);
        }
        else {
            ISI_removeCoderFromCall(isiId, "AMR-WB");
        }
        if (dtmfr) {
            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d", dynamicCoderValue++, 20);
            ISI_addCoderToCall(isiId, "telephone-event-16k", desc);
            OSAL_snprintf(desc, sizeof(desc),
                    "enum=%d;rate=%d", dynamicCoderValue++, 20);
            ISI_addCoderToCall(isiId, "telephone-event", desc);
        }
        else {
            ISI_removeCoderFromCall(isiId, "telephone-event-16k");
            ISI_removeCoderFromCall(isiId, "telephone-event");
        }
    }
}

/*
 * ======== IUT_appSetFields() ========
 *
 * This function is used to issue all the ISI commands to set various
 * fields for a service. The parameter list is self explanatory as to
 * which fields get set.
 *
 * Returns:
 *   Nothing.
 *
 */
void IUT_appSetFields(
    IUT_HandSetObj *hs_ptr,
    char const     *proxy_ptr,
    char const     *stun_ptr,
    char const     *relay_ptr,
    char const     *xcap_ptr,
    char const     *chat_ptr,
    char const     *outbound_ptr,
    char const     *username_ptr,
    char const     *password_ptr,
    char const     *realm_ptr,
    char const     *uri_ptr,
    char const     *imei_ptr,
    vint            isEmergency,
    vint            audioPortNumber,
    vint            videoPortNumber,
    char const     *interfaceAddress,
    vint            cidPrivate)
{
    ISI_Return r;

    if (uri_ptr) {
        hs_ptr->uri[0] = 0;
        OSAL_strncpy(hs_ptr->uri, uri_ptr, IUT_STR_SIZE);
        r = ISI_serviceSetUri(hs_ptr->serviceId, hs_ptr->uri);
        if (r != ISI_RETURN_OK) {
            OSAL_logMsg("Could not set URI ERROR:%s\n", IUT_prtReturnString(r));
        }
    }

    hs_ptr->cidPrivate = cidPrivate;
    r = ISI_serviceMakeCidPrivate(hs_ptr->serviceId, hs_ptr->cidPrivate);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not set Cid Private ERROR:%s\n", IUT_prtReturnString(r));
    }

    hs_ptr->isEmergency = isEmergency;
    r = ISI_serviceSetEmergency(hs_ptr->serviceId, hs_ptr->isEmergency);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not set isEmergency ERROR:%s\n", IUT_prtReturnString(r));
    }

    if (imei_ptr) {
        hs_ptr->imeiUri[0] = 0;
        OSAL_strncpy(hs_ptr->imeiUri, imei_ptr, IUT_STR_SIZE);
        r = ISI_serviceSetImeiUri(hs_ptr->serviceId, hs_ptr->imeiUri);
        if (r != ISI_RETURN_OK) {
            OSAL_logMsg("Could not set imeiUri ERROR:%s\n", IUT_prtReturnString(r));
        }
    }
    
    hs_ptr->audioPortNumber = audioPortNumber;
    r = ISI_serviceSetPort(hs_ptr->serviceId, hs_ptr->audioPortNumber, 20, ISI_PORT_TYPE_AUDIO);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not set audioPortNumber ERROR:%s\n", IUT_prtReturnString(r));
    }
    
    hs_ptr->videoPortNumber = videoPortNumber;
    r = ISI_serviceSetPort(hs_ptr->serviceId, hs_ptr->audioPortNumber, 20, ISI_PORT_TYPE_VIDEO);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not set videoPortNumber ERROR:%s\n", IUT_prtReturnString(r));
    }
    
    if (interfaceAddress) {
        hs_ptr->interfaceAddress[0] = 0;
        OSAL_strncpy(hs_ptr->interfaceAddress, interfaceAddress, IUT_STR_SIZE);
        r = ISI_serviceSetInterface(hs_ptr->serviceId, "lte", hs_ptr->interfaceAddress);
        if (r != ISI_RETURN_OK) {
            OSAL_logMsg("Could not set interface %s\n", IUT_prtReturnString(r));
        }
    }
    /*
     * If 'outbound proxy' is set then issue that command
     * but NOT the 'proxy' command.  IF 'outbound proxy is NOT set,
     * then don't issue the command; rather, issue the 'proxy' command
     */

    if (proxy_ptr) {
        hs_ptr->proxy[0] = 0;
        OSAL_strncpy(hs_ptr->proxy, proxy_ptr, IUT_STR_SIZE);
        r = ISI_serviceSetServer(hs_ptr->serviceId, hs_ptr->proxy,
                ISI_SERVER_TYPE_REGISTRAR);
        if (r != ISI_RETURN_OK) {
            OSAL_logMsg("Could not set Registrar ERROR:%s\n",
                   IUT_prtReturnString(r));
        }

        if (outbound_ptr && outbound_ptr[0] != 0) {
            hs_ptr->outProxy[0] = 0;
            OSAL_strncpy(hs_ptr->outProxy, outbound_ptr, IUT_STR_SIZE);
            r = ISI_serviceSetServer(hs_ptr->serviceId, hs_ptr->outProxy,
                ISI_SERVER_TYPE_OUTBOUND_PROXY);
            if (r != ISI_RETURN_OK) {
                OSAL_logMsg("Could not set Proxy ERROR:%s\n", IUT_prtReturnString(r));
            }
        }
        else {
            r = ISI_serviceSetServer(hs_ptr->serviceId, hs_ptr->proxy,
                ISI_SERVER_TYPE_PROXY);
            if (r != ISI_RETURN_OK) {
                OSAL_logMsg("Could not set Registrar ERROR:%s\n",
                       IUT_prtReturnString(r));
            }
        }
    }

    if (stun_ptr) {
        hs_ptr->stun[0] = 0;
        OSAL_strncpy(hs_ptr->stun, stun_ptr, IUT_STR_SIZE);
        r = ISI_serviceSetServer(hs_ptr->serviceId, hs_ptr->stun,
                ISI_SERVER_TYPE_STUN);
        if (r != ISI_RETURN_OK) {
            OSAL_logMsg("Could not set the stun server ERROR:%s\n",
                    IUT_prtReturnString(r));
        }
    }

    if (relay_ptr) {
        hs_ptr->relay[0] = 0;
        OSAL_strncpy(hs_ptr->relay, relay_ptr, IUT_STR_SIZE);
        r = ISI_serviceSetServer(hs_ptr->serviceId, hs_ptr->relay,
                ISI_SERVER_TYPE_RELAY);
        if (r != ISI_RETURN_OK) {
            OSAL_logMsg("Could not set the relay server ERROR:%s\n",
                    IUT_prtReturnString(r));
        }
    }

    if (xcap_ptr) {
        hs_ptr->xcap[0] = 0;
        OSAL_strncpy(hs_ptr->xcap, xcap_ptr, IUT_STR_SIZE);
        r = ISI_serviceSetServer(hs_ptr->serviceId, hs_ptr->xcap,
                ISI_SERVER_TYPE_STORAGE);
        if (r != ISI_RETURN_OK) {
            OSAL_logMsg("Could not set the xcap root ERROR:%s\n",
                    IUT_prtReturnString(r));
        }
    }

    if (chat_ptr) {
        hs_ptr->chat[0] = 0;
        OSAL_strncpy(hs_ptr->chat, chat_ptr, IUT_STR_SIZE);
        r = ISI_serviceSetServer(hs_ptr->serviceId, hs_ptr->chat,
                ISI_SERVER_TYPE_CHAT);
        if (r != ISI_RETURN_OK) {
            OSAL_logMsg("Could not set the chat server ERROR:%s\n",
                    IUT_prtReturnString(r));
        }
    }

    if (username_ptr && password_ptr && realm_ptr) {
        hs_ptr->username[0] = 0;
        hs_ptr->password[0] = 0;
        hs_ptr->realm[0] = 0;
        OSAL_strncpy(hs_ptr->username, username_ptr, IUT_STR_SIZE);
        OSAL_strncpy(hs_ptr->password, password_ptr, IUT_STR_SIZE);
        OSAL_strncpy(hs_ptr->realm, realm_ptr, IUT_STR_SIZE);
        r = ISI_serviceSetCredentials(hs_ptr->serviceId, hs_ptr->username,
                hs_ptr->password, hs_ptr->realm);
        if (r != ISI_RETURN_OK) {
            OSAL_logMsg("Could not set Credentials ERROR:%s\n",
                    IUT_prtReturnString(r));
        }
    }
}

/*
 * ======== IUT_appSetActivation() ========
 *
 * This function wraps the ISI commands that will activate/de-activate
 * service based on the "on" parameter. '1' = activate, '0' = de-activate.
 *
 * Returns:
 *   Nothing.
 *
 */
void IUT_appSetActivation(
    IUT_HandSetObj *hs_ptr,
    vint            on)
{
    ISI_Return r;
    if (on) {
        r = ISI_activateService(hs_ptr->serviceId);
        if (r != ISI_RETURN_OK) {
            OSAL_logMsg("Could not activate the service ERROR:%s\n",
                    IUT_prtReturnString(r));
        }
    }
    else {
        r = ISI_deactivateService(hs_ptr->serviceId);
        if (r != ISI_RETURN_OK) {
            OSAL_logMsg("Could not de-activate the service ERROR:%s\n",
                    IUT_prtReturnString(r));
        }
    }
}

/*
 * ======== IUT_appBlockUser() ========
 *
 * This function wraps the ISI commands that will block/unblock users from
 * being able to call into this entity.
 *
 * blockUser: '1' = block, '0' = unblock.
 *
 * Returns:
 *   Nothing.
 *
 */
void IUT_appBlockUser(
    IUT_HandSetObj *hs_ptr,
    char           *user_ptr,
    vint            blockUser)
{
    ISI_Return r;
    if (blockUser) {
        r = ISI_serviceBlockUser(hs_ptr->serviceId, user_ptr);
        if (r != ISI_RETURN_OK) {
            OSAL_logMsg("Could not block the user %s\n",
                    IUT_prtReturnString(r));
        }
    }
    else {
        r = ISI_serviceUnblockUser(hs_ptr->serviceId, user_ptr);
        if (r != ISI_RETURN_OK) {
            OSAL_logMsg("Could not unblock the user %s\n",
                    IUT_prtReturnString(r));
        }
    }
}

/*
 * ======== IUT_appForward() ========
 *
 * This function wraps the ISI commands that will set conditional call
 * forwarding
 *
 * Returns:
 *   Nothing.
 *
 */
void IUT_appForward(
    IUT_HandSetObj *hs_ptr,
    char           *target_ptr,
    vint            condition,
    vint            enable,
    vint            timeout)
{
    ISI_Return r;
    ISI_Id     id;
    r = ISI_serviceForwardCalls(&id, hs_ptr->serviceId, condition,
            enable, target_ptr, timeout);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not set conditional call forwarding %s\n",
                IUT_prtReturnString(r));
    }
    return;
}

/*
 * ======== IUT_appForward() ========
 *
 * This function wraps the ISI commands that will set conditional call
 * forwarding
 *
 * Returns:
 *   Nothing.
 *
 */
void IUT_appSetInterface(
    IUT_HandSetObj *hs_ptr,
    char           *name_ptr,
    char           *addr_ptr)
{
    ISI_Return r;

    r = ISI_serviceSetInterface(hs_ptr->serviceId, name_ptr, addr_ptr);

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not set interface %s\n",
                IUT_prtReturnString(r));
    }
    return;
}

/*
 * ======== IUT_appSetPort() ========
 *
 * This function wraps the ISI commands that will set port
 *
 * Returns:
 *   Nothing.
 *
 */
void IUT_appSetPort(
    IUT_HandSetObj *hs_ptr,
    int            port,
    int            poolSize,
    int            type)
{
    ISI_Return r;
    
    r = ISI_serviceSetPort(hs_ptr->serviceId, port, poolSize, type);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not set port %s\n", IUT_prtReturnString(r));
    }
    return;
}

/*
 * ======== IUT_appSetIpsec() ========
 *
 * This function wraps the ISI commands that will set ipsec related
 * parameters.
 *
 * Returns:
 *   Nothing.
 *
 */
void IUT_appSetIpsec(
    IUT_HandSetObj *hs_ptr,
    int            port,
    int            poolSize,
    int            spi,
    int            spiPoolSize)
{
    ISI_Return r;
    
    r = ISI_serviceSetIpsec(hs_ptr->serviceId, port, poolSize,
            spi, spiPoolSize);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not set port %s\n", IUT_prtReturnString(r));
    }
    return;
}
/*
 * ======== IUT_appModuleInit() ========
 *
 * This function initializes (zeros) the locally (statically) defined
 * objects in this file.  It is called around system startup time.
 *
 * Returns:
 *   Nothing.
 *
 */
void IUT_appModuleInit(void)
{
    _IUT_peer = 0;
    _IUT_serviceId = 0;
    OSAL_memSet(_IUT_Ep, 0, sizeof(IUT_HandSetObj) * IUT_CFG_MAX_SERVICE);
    OSAL_memSet(_IUT_Services, 0, sizeof(IUT_ServiceObj) * IUT_CFG_MAX_SERVICE);
    return;
}

/*
 * ======== IUT_appServiceFind() ========
 *
 * This function will search the array of handset objects locally defined
 * in this file and try to find one that has the same service Identifier
 * associated with it as the "serviceId" parameter.
 *
 * Returns:
 *   IUT_HandSetObj* : A handle to the handset object with a matching serviceId
 *   NULL : No handset object could be found that has the same serviceId as the
 *         "serviceId" parameter.
 *
 */
IUT_HandSetObj *IUT_appServiceFind(ISI_Id serviceId)
{
    vint x;
    for (x = 0 ; x < IUT_CFG_MAX_SERVICE ; x++) {
        if (serviceId == _IUT_Ep[x].serviceId) {
            return (&_IUT_Ep[x]);
        }
    }
    return (NULL);
}

/*
 * ======== IUT_appServiceSet() ========
 *
 * This function will place a particular service in "focus".
 * It will search for a service based on the serviceId parameter
 * provided.
 *
 * Returns:
 *   Nothing.
 *
 */
void IUT_appServiceSet(ISI_Id serviceId)
{
    IUT_HandSetObj *hs_ptr;
    /* Make sure the service ID exists */
    if (serviceId == 0) {
        OSAL_logMsg("!!The service you requested does not exist!!\n");
        XXGETCH();
        return;
    }
    hs_ptr = IUT_appServiceFind(serviceId);
    if (hs_ptr == NULL) {
        /* Doesn't exist */
        OSAL_logMsg("!!The service you requested does not exist!!\n");
        XXGETCH();
        return;
    }
    _IUT_serviceId = serviceId;
}

/*
 * ======== IUT_appServiceGet() ========
 *
 * This function will return a pointer to the handset object
 * that is owned by the service that is in "focus".
 *
 * Returns:
 *   IUT_HandSetObj * : A pointer to the handset object of the service
 *                      in "focus".
 *   NULL : There is no service currently in focus.  In fact there is
 *          probably no services at all.
 *
 */
IUT_HandSetObj *IUT_appServiceGet(void)
{
    if (_IUT_serviceId == 0) {
        return (NULL);
    }
    return (IUT_appServiceFind(_IUT_serviceId));
}

/*
 * ======== IUT_appServiceNext() ========
 *
 * This function will take the serviceId parameter and then search
 * for that service in the array of known services.  Once it is found
 * this function will then get THE NEXT service in the array of
 * services.  Note that this function will automatically account
 * for roll overs in the array when getting the next service.
 *
 * This function is used during a VCC handoff.  The isi_ut
 * will handoff calls from one service to the very next available service
 *
 * Returns:
 *   ISI_Id : The service ID of the next service
 *   0 : There is no "next" service.
 *
 */
ISI_Id IUT_appServiceNext(ISI_Id serviceId)
{
    vint x;
    vint count;
    for (x = 0 ; x < IUT_CFG_MAX_SERVICE ; x++) {
        if (serviceId == _IUT_Ep[x].serviceId) {
            /* Keep advancing the index and looking for the next valid serviceId */
            for (count = 0; count < IUT_CFG_MAX_SERVICE; count++) {
                x++;
                if (x == IUT_CFG_MAX_SERVICE) {
                    x = 0;
                }
                if (_IUT_Ep[x].serviceId != 0) {
                    return (_IUT_Ep[x].serviceId);
                }
            }
            return (0);
        }
    }
    return (0);
}

/*
 * ======== IUT_appServicePrint() ========
 *
 * This function will print all available services
 *
 * Returns:
 *   Nothing.
 *
 */
void IUT_appServicePrint(void)
{
    vint  x;
    vint  cnt;
    char *name_ptr;
    char *state_ptr;

    OSAL_logMsg("PROTOCOLS:\n");
    for (x = 0, cnt = 0; x < IUT_CFG_MAX_SERVICE ; x++) {
        switch (_IUT_Services[x].proto) {
            case 1:
                name_ptr = "SIP";
                break;
            case 2:
                name_ptr = "XMPP";
                break;
            case 3:
                name_ptr = "GSM";
                break;
            case 4:
                name_ptr = "Yahoo";
                break;
            case 5:
                name_ptr = "Sametime";
                break;
            default:
                continue;
        }

        if (_IUT_Services[x].isDead == 1) {
            state_ptr = "De-registered";
        }
        else {
            state_ptr = "Registered";
        }

        OSAL_logMsg("   %s Protocol is %s\n", name_ptr, state_ptr);
    }

    OSAL_logMsg("SERVICES:\n");
    for (x = 0, cnt = 0; x < IUT_CFG_MAX_SERVICE ; x++) {
        switch (_IUT_Ep[x].proto) {
            case 1:
                name_ptr = "SIP";
                break;
            case 2:
                name_ptr = "XMPP";
                break;
            case 3:
                name_ptr = "GSM";
                break;
            case 4:
                name_ptr = "Yahoo";
                break;
            case 5:
                name_ptr = "Sametime";
                break;
            default:
                continue;
        }

        if (_IUT_Ep[x].isDead == 1) {
            state_ptr = "Dead";
        }
        else {
            if (_IUT_Ep[x].isActive == 1) {
                state_ptr = "Active";
            }
            else {
                state_ptr = "Not Active";
            }
        }

        OSAL_logMsg("   ID#:%d %s Service is %s\n",
                _IUT_Ep[x].serviceId, name_ptr, state_ptr);
        cnt++;
        /* XXGETCH(); */
    }
    OSAL_logMsg("Total Number of Services:%d\n", cnt);
    XXGETCH();
    return;
}

/*
 * ======== IUT_appPeerSet() ========
 *
 * This function will set the focus of the peer
 *
 * Returns:
 *   Nothing.
 *
 */
void IUT_appPeerSet(
    vint peer)
{
    _IUT_peer = peer;
}

/*
 * ======== IUT_appPeerGet() ========
 *
 * This function returns the peer that is in focus.
 *
 * Returns:
 *   vint : The peer is focus. a.k.a. the index of the call that is in focus.
 *
 */
vint IUT_appPeerGet()
{
    return (_IUT_peer);
}

vint IUT_appCreateGroupChat(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    char           *roomName_ptr,
    char           *subject_ptr)
{
    ISI_Return r;
    vint       type;

    type = ISI_SESSION_TYPE_CHAT;
    /* if would like to use TLS, add following */
    //type |= ISI_SESSION_TYPE_SECURITY_CHAT;

    r = ISI_initiateGroupChat(&hs_ptr->chatRoomId[peer], hs_ptr->serviceId,
        roomName_ptr, subject_ptr, NULL, type);

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not create a chat room. ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

vint IUT_appCreateAdhocGroupChat(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    char           *to_ptr,
    char           *conferenceUri_ptr,
    char           *subject_ptr)
{
    ISI_Return r;
    vint       type;

    type = ISI_SESSION_TYPE_CHAT;
    /* if would like to use TLS, add following */
    //type |= ISI_SESSION_TYPE_SECURITY_CHAT;

    if (hs_ptr->chatRoomId[peer] == 0) {
        r = ISI_initiateGroupChatAdhoc(&hs_ptr->chatRoomId[peer], hs_ptr->serviceId,
                to_ptr, conferenceUri_ptr, subject_ptr, type);

        if (r != ISI_RETURN_OK) {
            OSAL_logMsg("Could not create an adhoc chat room. ERROR:%s\n",
                    IUT_prtReturnString(r));
            return (IUT_ERR);
        }
        return (IUT_OK);
    }

    OSAL_logMsg("There is already a chat session for peer:%d\n", peer);
    return (IUT_ERR);
}

vint IUT_appInitiateChat(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    char           *to_ptr,
    char           *subject_ptr,
    char           *message_ptr,
    uint8           requestDeliveryReports,
    uint8           requestDisplayReports)
{
    ISI_Return r;
    ISI_MessageReport report = ISI_MSG_RPT_NONE;
    char reportId[IUT_STR_SIZE + 1] = { 0 };
    vint       type;

    if (0 != requestDeliveryReports) {
        report = ISI_MSG_RPT_DELIVERY_SUCCESS | ISI_MSG_RPT_DELIVERY_FAILED |
            ISI_MSG_RPT_DELIVERY_FORBIDDEN | ISI_MSG_RPT_DELIVERY_ERROR;
        _IUT_generateRandomString(reportId, 20);
    }

    if (0 != requestDisplayReports) {
        report |= ISI_MSG_RPT_DISPLAY_SUCCESS;
        if (0 == requestDeliveryReports) {
            _IUT_generateRandomString(reportId, 20);
        }
    }

    if (hs_ptr->chatRoomId[peer] == 0) {
        type = ISI_SESSION_TYPE_CHAT;
        /* if would like to use TLS, add following */
        //type |= ISI_SESSION_TYPE_SECURITY_CHAT;

        r = ISI_initiateChat(&hs_ptr->chatRoomId[peer], hs_ptr->serviceId,
                to_ptr, subject_ptr, message_ptr, report, reportId, type);

        if (r != ISI_RETURN_OK) {
            OSAL_logMsg("Could not initiate a chat. ERROR:%s\n",
                    IUT_prtReturnString(r));
            return (IUT_ERR);
        }
        return (IUT_OK);
    }
    OSAL_logMsg("There is already a chat session for peer:%d\n", peer);
    return (IUT_ERR);
}

vint IUT_appAcceptChat(
    IUT_HandSetObj *hs_ptr,
    vint            peer)
{
    ISI_Return r;

    r = ISI_acceptChat(hs_ptr->chatRoomId[peer]);

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not accept a chat. ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

vint IUT_appAcknowledgeChat(
    IUT_HandSetObj *hs_ptr,
    vint            peer)
{
    ISI_Return r;

    r = ISI_acknowledgeChat(hs_ptr->chatRoomId[peer]);

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not acknowledge a chat. ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

vint IUT_appRejectChat(
    IUT_HandSetObj *hs_ptr,
    vint            peer)
{
    ISI_Return r;

    r = ISI_rejectChat(hs_ptr->chatRoomId[peer], "Disconnect");

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not reject a chat. ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

vint IUT_appDisconnectChat(
    IUT_HandSetObj *hs_ptr,
    vint            peer)
{
    ISI_Return r;

    r = ISI_disconnectChat(hs_ptr->chatRoomId[peer]);

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not disconnect a chat. ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

vint IUT_appJoinGroupChat(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    char           *roomName_ptr,
    char           *password_ptr)
{
    ISI_Return r;
    vint       type;

    type = ISI_SESSION_TYPE_CHAT;
    /* if would like to use TLS, add following */
    //type |= ISI_SESSION_TYPE_SECURITY_CHAT;

    r = ISI_joinGroupChat(&hs_ptr->chatRoomId[peer], hs_ptr->serviceId,
        roomName_ptr, password_ptr, type);

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not create a chat room. ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

vint IUT_appDestroyGroupChat(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    char           *reason_ptr)
{
    ISI_Return r;

    r = ISI_destroyGroupChat(hs_ptr->chatRoomId[peer], reason_ptr);

    hs_ptr->chatRoomId[peer] = 0;

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not destroy a chat room. ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

vint IUT_appInviteGroupChat(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    char           *participant_ptr,
    char           *reason_ptr)
{
    ISI_Return r;

    r = ISI_inviteGroupChat(hs_ptr->chatRoomId[peer], participant_ptr, reason_ptr);

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not invite someone to a chat room. ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

vint IUT_appKickGroupChat(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    char           *participant_ptr,
    char           *reason_ptr)
{
    ISI_Return r;

    r = ISI_kickGroupChat(hs_ptr->chatRoomId[peer], participant_ptr, reason_ptr);

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not kick %s from a chat room. ERROR:%s\n",
                participant_ptr, IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

vint IUT_appSendMessageChat(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    char           *msg_ptr,
    uint8           requestDeliveryReports,
    uint8           requestDisplayReports)
{
    ISI_Return r;
    ISI_MessageReport report = ISI_MSG_RPT_NONE;
    char reportId[IUT_STR_SIZE + 1] = { 0 };

    if (0 != requestDeliveryReports) {
        report = ISI_MSG_RPT_DELIVERY_SUCCESS | ISI_MSG_RPT_DELIVERY_FAILED |
            ISI_MSG_RPT_DELIVERY_FORBIDDEN | ISI_MSG_RPT_DELIVERY_ERROR;
        _IUT_generateRandomString(reportId, 20);
    }

    if (0 != requestDisplayReports) {
        report |= ISI_MSG_RPT_DISPLAY_SUCCESS;
        if (0 == requestDeliveryReports) {
            _IUT_generateRandomString(reportId, 20);
        }
    }

    r = ISI_sendChatMessage(&hs_ptr->textId, hs_ptr->chatRoomId[peer],
            msg_ptr, report, reportId);

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not send a message to a chat room. ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

vint IUT_appComposingMessageChat(
    IUT_HandSetObj *hs_ptr,
    vint            peer)
{
    ISI_Return r;

    r = ISI_composingChatMessage(hs_ptr->chatRoomId[peer]);

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not send composingMessage to a chat room. ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

vint IUT_appSendFileChat(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    char           *filePath_ptr,
    int             fileType)
{
    ISI_Return r;

    r = ISI_sendChatFile(&hs_ptr->fileId, hs_ptr->chatRoomId[peer], "",
            filePath_ptr, (ISI_FileType) fileType);

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not send a file to a chat room. ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

vint IUT_appSendPresenceGroupChat(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    vint            state,
    char           *status_ptr)
{
    ISI_Return r;
    char presence[256];

    if (state == 0) {
        OSAL_snprintf(presence, 256, "%s",
                "<show>away</show><priority>0</priority>");
    }
    else if (state == 1) {
        if (status_ptr) {
            OSAL_snprintf(presence, 256,
            "<show>available</show><priority>0</priority><status>%s</status>",
            status_ptr);
        }
        else {
            OSAL_snprintf(presence, 256, "%s",
                    "<show>available</show><priority>0</priority>");
        }
    }
    else {
        /* Then it's 'dnd' */
        if (status_ptr) {
            OSAL_snprintf(presence, 256,
            "<show>dnd</show><priority>0</priority><status>%s</status>",
            status_ptr);
        }
        else {
            OSAL_snprintf(presence, 256, "%s",
                    "<show>dnd</show><priority>0</priority>");
        }
    }

    r = ISI_sendGroupChatPresence(&hs_ptr->presId, hs_ptr->chatRoomId[peer],
            presence);

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not send presence to a group chat room ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}



/*
 * ======== IUT_appContentShareSendFile() ========
 *
 * This function wraps the ISI commands that will send a file
 * without the need for an established session.  This supports
 * Content Sharing as part of RCS-e.
 *
 * Returns:
 *   Nothing.
 *
 */
vint IUT_appContentShareSendFile(
    IUT_HandSetObj *hs_ptr,
    char           *to_ptr,
    char           *subject_ptr,
    char           *filePath_ptr,
    int             fileType,
    int             fileAttribute,
    int             report)
{
    ISI_Return r;

    r = ISI_sendFile(&hs_ptr->fileId, hs_ptr->serviceId,
            to_ptr, subject_ptr, filePath_ptr, (ISI_FileType) fileType,
            (ISI_FileAttribute) fileAttribute, _IUT_appGetFileSize(filePath_ptr),
            (ISI_FileReport) report);

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not perform content sharing.  Sending file outside session failed. ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}


/*
 * ======== IUT_appAcknowledgeFileTransfer() ========
 *
 * This function wraps the ISI commands that will acknowledge an incoming file
 * transfer request.
 * This supports Content Sharing as part of RCS-e.
 *
 * Returns:
 *   Nothing.
 *
 */
vint IUT_appAcknowledgeFileTransfer(
    IUT_HandSetObj *hs_ptr,
    ISI_Id          id)
{
    ISI_Return r;

    r = ISI_acknowledgeFile(id);

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not acknowledge content sharing.  Acknowledgment failed. ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

/*
 * ======== IUT_appAcceptFileTransfer() ========
 *
 * This function wraps the ISI commands that will accept an incoming file
 * transfer request.
 * This supports Content Sharing as part of RCS-e.
 *
 * Returns:
 *   Nothing.
 *
 */
vint IUT_appAcceptFileTransfer(
    IUT_HandSetObj *hs_ptr,
    ISI_Id          id)
{
    ISI_Return r;

    r = ISI_acceptFile(id);

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not accept content sharing.  Accept failed. ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

/*
 * ======== IUT_appRejectFileTransfer() ========
 *
 * This function wraps the ISI commands that will reject an incoming file
 * transfer request.
 * This supports Content Sharing as part of RCS-e.
 *
 * Returns:
 *   Nothing.
 *
 */
vint IUT_appRejectFileTransfer(
    IUT_HandSetObj *hs_ptr,
    ISI_Id          id,
    const char     *rejectReason_ptr)
{
    ISI_Return r;

    r = ISI_rejectFile(id, rejectReason_ptr);

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg(
                "Could not reject content sharing.  Reject failed. ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

/*
 * ======== IUT_appCancelFileTransfer() ========
 *
 * This function wraps the ISI commands that will cancel a file
 * transfer request.
 * This supports Content Sharing as part of RCS-e.
 *
 * Returns:
 *   Nothing.
 *
 */
vint IUT_appCancelFileTransfer(
    IUT_HandSetObj *hs_ptr,
    ISI_Id          id,
    const char     *cancelReason_ptr)
{
    ISI_Return r;

    r = ISI_cancelFile(id, cancelReason_ptr);

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg(
                "Could not cancel content sharing.  Cancel failed. ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

/*
 * ======== IUT_appSetFeature() ========
 *
 * This function wraps the ISI commands that will set feature to ISI server.
 *
 * Returns:
 *   Nothing.
 */
vint IUT_appSetFeature(
    ISI_FeatureType features)
{
    ISI_Return r;

    r = ISI_setFeature(features);

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not set feature. ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    return (IUT_OK);
}

/*
 * ======== IUT_appGetNextService() ========
 *
 * This function wraps the ISI commands to get next service and set the 
 * IUT service focus.
 *
 * Returns:
 *   IUT_HandSetObj* : A handle to the handset object which associated to
                       the next service get from ISI.
 *   NULL : No next service get.
 */
IUT_HandSetObj* IUT_appGetNextService(
    IUT_HandSetObj *hs_ptr)
{
    ISI_Return      r;
    ISI_Id          serviceId;
    int             protocol;
    int             isEmergency;
    int             features;
    int             isActivated;

    /* Get next service from current service */
    if (NULL != hs_ptr) {
        serviceId = hs_ptr->serviceId;
    }
    else {
        serviceId = 0;
    }

    r = ISI_getNextService(&serviceId, &protocol, &isEmergency, &features,
            &isActivated);

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not get next service. ERROR:%s\n",
                IUT_prtReturnString(r));
        return (NULL);
    }

    OSAL_logMsg("SeriveId=%d, Protocol=%d, Emergency=%d features:0x%02X."
            "isActivated:%d\n", serviceId, protocol, isEmergency, features,
            isActivated);
    /* Try to find an available resource */
    hs_ptr = IUT_appServiceFind((ISI_Id)0);
    if (hs_ptr == NULL) {
        OSAL_logMsg("Failed! No more resources"
                " to accomodate this service\n");
        return (NULL);
    }
    /* Now init a service */
    OSAL_memSet(hs_ptr, 0, sizeof(IUT_HandSetObj));
    /* Set a subject */
    OSAL_strncpy(hs_ptr->subject, "D2 Rocks", IUT_STR_SIZE);
    /* Set the default tone duration */
    hs_ptr->toneDuration = IUT_TONE_DURATION_MS;
    hs_ptr->serviceId    = serviceId;
    hs_ptr->proto        = protocol;
    IUT_appServiceSet(hs_ptr->serviceId);
    if (IUT_cfgSetServiceId(hs_ptr->serviceId) != IUT_OK) {
        /* No configuration object to match */
        OSAL_logMsg("Failed! No more config resources"
                " to accomodate this service\n");
        IUT_appServiceFree(hs_ptr);
        return (NULL);
    }

    return (hs_ptr);
}

/*
 * ======== IUT_appSetProvisioningData() ========
 *
 * This function wraps the ISI commands to set parameters to a specific service
 * from a xml doc.
 *
 * Returns:
 *   IUT_OK: Paramters set ok.
 *   IUT_ERR: Failed to set paramters.
 */
vint IUT_appSetProvisioningData(
    IUT_HandSetObj *hs_ptr,
    char           *filename_ptr)
{
    ISI_Return  r;
    OSAL_FileId fid;
    char        xmlDoc[ISI_PROVISIONING_DATA_STRING_SZ + 1];
    vint        size;

    /* Open file */
    if (OSAL_SUCCESS != OSAL_fileOpen(&fid, filename_ptr, OSAL_FILE_O_RDONLY,
            00755)) {
        return (IUT_ERR);
    }
    /* Read file */
    size = ISI_PROVISIONING_DATA_STRING_SZ;
    if (OSAL_SUCCESS != OSAL_fileRead(&fid, xmlDoc, &size)) {
        OSAL_fileClose(&fid);
        return (IUT_ERR);
    }
    /* Close file */
    OSAL_fileClose(&fid);

    /* Call ISI api to set parameters */
    r = ISI_setProvisioningData(hs_ptr->serviceId, xmlDoc);

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not set provisioning data. ERROR:%s\n",
                IUT_prtReturnString(r));
        return (IUT_ERR);
    }
    
    return (IUT_OK);
}

/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 27379 $ $Date: 2014-07-10 17:07:59 +0800 (Thu, 10 Jul 2014) $
 */

#include <osal.h>
#include <isi.h>
#include <isip.h>
#include "_iut_app.h"
#include "_iut_cfg.h"
#include "_iut.h"

static void _IUT_apiLoadState(
    IUT_HandSetObj *hs_ptr)
 {
    OSAL_strncpy(hs_ptr->uri, 
            IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_URI_F),
            IUT_STR_SIZE);

    OSAL_strncpy(hs_ptr->proxy, 
            IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_PROXY_F),
            IUT_STR_SIZE);

    OSAL_strncpy(hs_ptr->outProxy, 
            IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_OUTBOUND_F),
            IUT_STR_SIZE);

    OSAL_strncpy(hs_ptr->stun, 
            IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_STUN_F),
            IUT_STR_SIZE);

    OSAL_strncpy(hs_ptr->relay, 
            IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_RELAY_F),
            IUT_STR_SIZE);
    
    OSAL_strncpy(hs_ptr->xcap, 
            IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_XCAP_F),
            IUT_STR_SIZE);

    OSAL_strncpy(hs_ptr->chat,
            IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_CHAT_F),
            IUT_STR_SIZE);

    OSAL_strncpy(hs_ptr->username, 
            IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_USERNAME_F),
            IUT_STR_SIZE);

    OSAL_strncpy(hs_ptr->password,
            IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_PASSWORD_F),
            IUT_STR_SIZE);

    OSAL_strncpy(hs_ptr->realm,
            IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_REALM_F),
            IUT_STR_SIZE);

    hs_ptr->cidPrivate = 
            IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_CID_PRIVATE_F);
    hs_ptr->isEmergency = 
            IUT_cfgGetIntField(hs_ptr->serviceId, IUT_CFG_EMERGENCY_F);
    
    OSAL_strncpy(hs_ptr->imeiUri,
            IUT_cfgGetStrField(hs_ptr->serviceId, IUT_CFG_IMEI_F),
            IUT_STR_SIZE);
    return;
}

static void _IUT_apiService(
    IUT_HandSetObj *hs_ptr)
{
    vint       step;
    
    OSAL_logMsg("Testing service related interfaces\n");

    step = 0;
    
    /* Check for NULL parameter */
    step++;
    /* The '1' forces the test to SIP */
    if (ISI_allocService(NULL, 1) != ISI_RETURN_FAILED) {
        goto _IUT_apiService_LABEL;
    }
    /* Test for invalid protocol */
    step++;
    if (ISI_allocService(&hs_ptr->serviceId, 10) != 
            ISI_RETURN_INVALID_PROTOCOL) {
        goto _IUT_apiService_LABEL;
    }

    step++;
    /* Correctly load service info */
    _IUT_apiLoadState(hs_ptr);
    /* Correctly init the service */
    if (ISI_allocService(&hs_ptr->serviceId, 1) != ISI_RETURN_OK) {
        goto _IUT_apiService_LABEL;
    }

    
    /* ISI_serviceMakeCidPrivate */
    step++;
    /* Check for an invalid service Id's */
    if (ISI_serviceMakeCidPrivate(0, 1) != ISI_RETURN_INVALID_SERVICE_ID) {
        goto _IUT_apiService_LABEL;
    }

    /* ISI_serviceSetUri */
    step++;
    /* Check for an invalid service Id's */
    if (ISI_serviceSetUri(0, hs_ptr->uri) != ISI_RETURN_INVALID_SERVICE_ID) {
        goto _IUT_apiService_LABEL;
    }
    step++;
    /* Check for an invalid URI */
    if (ISI_serviceSetUri(hs_ptr->serviceId, "") != ISI_RETURN_FAILED) {
        goto _IUT_apiService_LABEL;
    }
    step++;
    /* Check for an invalid URI */
    if (ISI_serviceSetUri(hs_ptr->serviceId, NULL) != ISI_RETURN_FAILED) {
        goto _IUT_apiService_LABEL;
    }

    /* ISI_serviceSetServer */
    step++;
    /* Check for an invalid service Id's */
    if (ISI_serviceSetServer(0, hs_ptr->proxy, ISI_SERVER_TYPE_PROXY) != 
            ISI_RETURN_INVALID_SERVICE_ID) {
        goto _IUT_apiService_LABEL;
    }
    step++;
    /* Check for an invalid server arg */
    if (ISI_serviceSetServer(hs_ptr->serviceId, NULL, ISI_SERVER_TYPE_PROXY) !=
            ISI_RETURN_FAILED) {
        goto _IUT_apiService_LABEL;
    }
    step++;
    if (ISI_serviceSetServer(hs_ptr->serviceId, hs_ptr->proxy, 
            ISI_SERVER_TYPE_INVALID) != ISI_RETURN_INVALID_SERVER_TYPE) {
        goto _IUT_apiService_LABEL;
    }
    
    step++;
    if (ISI_serviceSetCredentials(0, hs_ptr->username, hs_ptr->password, 
            hs_ptr->realm) != ISI_RETURN_INVALID_SERVICE_ID) {
        goto _IUT_apiService_LABEL;
    }

    step++;
    if (ISI_serviceSetCredentials(hs_ptr->serviceId, NULL, hs_ptr->password,
            hs_ptr->realm) != ISI_RETURN_INVALID_CREDENTIALS) {
        goto _IUT_apiService_LABEL;
    }
    step++;
    if (ISI_serviceSetCredentials(hs_ptr->serviceId, hs_ptr->username, NULL, 
            hs_ptr->realm) != ISI_RETURN_INVALID_CREDENTIALS) {
        goto _IUT_apiService_LABEL;
    }
    step++;
    if (ISI_serviceSetCredentials(hs_ptr->serviceId, hs_ptr->username, 
            hs_ptr->password, NULL) != ISI_RETURN_INVALID_CREDENTIALS) {
        goto _IUT_apiService_LABEL;
    }
    step++;
    if (ISI_serviceSetCredentials(hs_ptr->serviceId, "", "", "") != 
            ISI_RETURN_INVALID_CREDENTIALS) {
        goto _IUT_apiService_LABEL;
    }
    
    step++;

    /* ISI_serviceBlockUser */
    
    step++;
    if (ISI_serviceBlockUser(0, hs_ptr->uri) != 
            ISI_RETURN_INVALID_SERVICE_ID) {
        goto _IUT_apiService_LABEL;
    }

    step++;
    if (ISI_serviceBlockUser(hs_ptr->serviceId, NULL) != ISI_RETURN_FAILED) {
        goto _IUT_apiService_LABEL;
    }

    step++;
    if (ISI_serviceBlockUser(hs_ptr->serviceId, "") != ISI_RETURN_FAILED) {
        goto _IUT_apiService_LABEL;
    }

    /* ISI_serviceUnblockUser */
    
    step++;
    if (ISI_serviceUnblockUser(0, hs_ptr->uri) != 
            ISI_RETURN_INVALID_SERVICE_ID) {
        goto _IUT_apiService_LABEL;
    }

    step++;
    if (ISI_serviceUnblockUser(hs_ptr->serviceId, NULL) != ISI_RETURN_FAILED) {
        goto _IUT_apiService_LABEL;
    }

    step++;
    if (ISI_serviceUnblockUser(hs_ptr->serviceId, "") != ISI_RETURN_FAILED) {
        goto _IUT_apiService_LABEL;
    }

    /* ISI_addCoderToService */

    step++;
    if (ISI_addCoderToService(0, "PCMU", "enum=0,prio=1,rate=20") != 
            ISI_RETURN_INVALID_SERVICE_ID) {
        goto _IUT_apiService_LABEL;
    }

    step++;
    if (ISI_addCoderToService(hs_ptr->serviceId, "BAD-CODER",
                "enum=0,prio=1,rate=20") != 
            ISI_RETURN_INVALID_CODER) {
        goto _IUT_apiService_LABEL;
    }

    step++;
    if (ISI_addCoderToService(hs_ptr->serviceId, NULL,
                "enum=0,prio=1,rate=20") != 
            ISI_RETURN_INVALID_CODER) {
        goto _IUT_apiService_LABEL;
    }

    /* ISI_removeCoderFromService */

    step++;
    if (ISI_removeCoderFromService(0, "PCMU") != 
            ISI_RETURN_INVALID_SERVICE_ID) {
        goto _IUT_apiService_LABEL;
    }

    step++;
    if (ISI_removeCoderFromService(hs_ptr->serviceId, NULL) != 
            ISI_RETURN_INVALID_CODER) {
        goto _IUT_apiService_LABEL;
    }

    step++;
    if (ISI_removeCoderFromService(hs_ptr->serviceId, "BAD-CODER") != 
            ISI_RETURN_INVALID_CODER) {
        goto _IUT_apiService_LABEL;
    }

    
    OSAL_logMsg("++++++++++++++++++++++++++++++++++++\n"
            "The service API test has SUCCESSFULLY completed\n"
            "++++++++++++++++++++++++++++++++++++\n");
    return;
    
_IUT_apiService_LABEL:
    OSAL_logMsg("++++++++++++++++++++++++++++++++++++\n"
            "Service Test failed at step:%d\n"
            "++++++++++++++++++++++++++++++++++++\n", step);
    return;
}

static void _IUT_apiIm(IUT_HandSetObj *hs_ptr)
{
    vint   step;
    ISI_Id msgId;
    ISI_Id callId;
    char   buffer[IUT_STR_SIZE + 1];
    vint   msgLen;

    OSAL_logMsg("Testing IM related interfaces\n");

    step = 0;

    if (hs_ptr->serviceId == 0) {
        OSAL_logMsg("IM test failed.  You must call the service test first \n");
        return;
    }
    
    step++;
    if (ISI_sendMessage(NULL, hs_ptr->serviceId, ISI_MSG_TYPE_TEXT,
            "sip:38839@fwd.pulver.com", 
            "D2 Rocks", 
            "Performing Unit Test",
            OSAL_strlen("Performing Unit Test"),
            ISI_MSG_RPT_NONE, NULL) != ISI_RETURN_FAILED) {
        goto _IUT_apiIm_LABEL;
    }

    step++;
    if (ISI_sendMessage(&msgId, 0, ISI_MSG_TYPE_TEXT,
            "sip:38839@fwd.pulver.com", 
            "D2 Rocks", 
            "Performing Unit Test",
            OSAL_strlen("Performing Unit Test"),
            ISI_MSG_RPT_NONE, NULL) != ISI_RETURN_INVALID_SERVICE_ID) {
        goto _IUT_apiIm_LABEL;
    }

    step++;
    if (ISI_sendMessage(&msgId, hs_ptr->serviceId, ISI_MSG_TYPE_TEXT,
            NULL, 
            "D2 Rocks", 
            "Performing Unit Test",
            OSAL_strlen("Performing Unit Test"),
            ISI_MSG_RPT_NONE, NULL) != ISI_RETURN_FAILED) {
        goto _IUT_apiIm_LABEL;
    }

    step++;
    if (ISI_sendMessage(&msgId, hs_ptr->serviceId, ISI_MSG_TYPE_TEXT,
            "sip:38839@fwd.pulver.com", 
            "D2 Rocks", NULL, OSAL_strlen("NULL"),
            ISI_MSG_RPT_NONE, NULL) != ISI_RETURN_FAILED) {
        goto _IUT_apiIm_LABEL;
    }

    step++;
    if (ISI_sendMessage(&msgId, hs_ptr->serviceId, ISI_MSG_TYPE_TEXT,
            "", 
            "D2 Rocks", 
            "Performing Unit Test",
            OSAL_strlen("Performing Unit Test"),
            ISI_MSG_RPT_NONE, NULL) != ISI_RETURN_FAILED) {
        goto _IUT_apiIm_LABEL;
    }

    /* ISI_readMessage */
    step++;
    msgLen = IUT_STR_SIZE;
    if (ISI_readMessage(99999, NULL, buffer, &msgLen) !=
            ISI_RETURN_FAILED) {
        goto _IUT_apiIm_LABEL;
    }

    step++;
    msgLen = IUT_STR_SIZE;
    if (ISI_readMessage(99999, &callId, buffer, &msgLen) !=
            ISI_RETURN_INVALID_MESSAGE_ID) {
        goto _IUT_apiIm_LABEL;
    }

    OSAL_logMsg("++++++++++++++++++++++++++++++++++++\n"
            "The IM API test has SUCCESSFULLY completed\n"
            "++++++++++++++++++++++++++++++++++++\n");
    return;
    
_IUT_apiIm_LABEL:
    OSAL_logMsg("++++++++++++++++++++++++++++++++++++\n"
            "IM Test failed at step:%d\n"
            "++++++++++++++++++++++++++++++++++++\n", step);
    return;
}

static void _IUT_apiPresence(IUT_HandSetObj *hs_ptr)
{
    vint   step;
    ISI_Id presId;
    char   buffer[IUT_STR_SIZE + 1];
    
    OSAL_logMsg("Testing Presence related interfaces\n");

    step = 0;

    if (hs_ptr->serviceId == 0) {
        OSAL_logMsg("IM test failed.  You must call the service test first \n");
        return;
    }

    /* ISI_addContact */
    step++;
    if (ISI_addContact(NULL, hs_ptr->serviceId, "dude@gmail.com", NULL,
            NULL) != ISI_RETURN_FAILED) {
        goto _IUT_apiPres_LABEL;
    }

    step++;
    if (ISI_addContact(&presId, 99999, "dude@gmail.com", NULL, NULL) != 
            ISI_RETURN_INVALID_SERVICE_ID) {
        goto _IUT_apiPres_LABEL;
    }

    step++;
    if (ISI_addContact(&presId, hs_ptr->serviceId, NULL, NULL, NULL) != 
            ISI_RETURN_FAILED) {
        goto _IUT_apiPres_LABEL;
    }

    step++;
    if (ISI_addContact(&presId, hs_ptr->serviceId, "", NULL, NULL) != 
            ISI_RETURN_FAILED) {
        goto _IUT_apiPres_LABEL;
    }

    /* ISI_removeContact */

    step++;
    if (ISI_removeContact(NULL, hs_ptr->serviceId, "dude@gmail.com", NULL, NULL) !=
            ISI_RETURN_FAILED) {
        goto _IUT_apiPres_LABEL;
    }

    step++;
    if (ISI_removeContact(&presId, 99999, "dude@gmail.com", NULL, NULL) !=
            ISI_RETURN_INVALID_SERVICE_ID) {
        goto _IUT_apiPres_LABEL;
    }

    step++;
    if (ISI_removeContact(&presId, hs_ptr->serviceId, NULL, NULL, NULL) !=
            ISI_RETURN_FAILED) {
        goto _IUT_apiPres_LABEL;
    }

    step++;
    if (ISI_removeContact(&presId, hs_ptr->serviceId, "", NULL, NULL) !=
            ISI_RETURN_FAILED) {
        goto _IUT_apiPres_LABEL;
    }

    /* ISI_readContact */
    step++;
    if (ISI_readContact(9999999, buffer, buffer, buffer, buffer, 
            IUT_STR_SIZE) != ISI_RETURN_INVALID_PRESENCE_ID) {
        goto _IUT_apiPres_LABEL;
    }
    
    OSAL_logMsg("++++++++++++++++++++++++++++++++++++\n"
            "The Presence API test has SUCCESSFULLY completed\n"
            "++++++++++++++++++++++++++++++++++++\n");
    return;
    
_IUT_apiPres_LABEL:
    OSAL_logMsg("++++++++++++++++++++++++++++++++++++\n"
            "Presence Test failed at step:%d\n"
            "++++++++++++++++++++++++++++++++++++\n", step);
    return;
}

static void _IUT_apiCall(IUT_HandSetObj *hs_ptr)
{
    hs_ptr = 0;
    return;
}

static void _IUT_apiTelEvt(IUT_HandSetObj *hs_ptr)
{
    hs_ptr = 0;
    return;
}

void IUT_apiMainMenu(
    IUT_HandSetObj *hs_ptr)
{
    char ch;
    vint x;
    
    while (1) {
        OSAL_logMsg( "<><><><> API TEST main <><><><>\n"
                "0=service test\n"
                "1=presence\n"
                "2=Call test \n"
                "3=IM or text message test \n"
                "4=Telephone Event test \n"
                "q=quit\n");
        ch=XXGETCH();

        switch(ch) {
            case '0':
                _IUT_apiService(hs_ptr);
                break;
            case '1':
                _IUT_apiPresence(hs_ptr);
                break;
            case '2':
                _IUT_apiCall(hs_ptr);
                break;
            case '3':
                _IUT_apiIm(hs_ptr);
                break;
            case '4':
                _IUT_apiTelEvt(hs_ptr);
                break;
            case 'q':
                /* Hangup all calls and destroy the service */
                for (x = 0 ; x < IUT_MAX_LINES ; x++) {
                    ISI_terminateCall(hs_ptr->callId[x], NULL);
                }
                ISI_deactivateService(hs_ptr->serviceId);
                ISI_freeService(hs_ptr->serviceId);
                hs_ptr->serviceId = 0;
                return;
            default:
                break;
        }
    }
}



/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 18777 $ $Date: 2012-11-09 01:47:11 -0800 (Fri, 09 Nov 2012) $
 *
 */

#include <osal.h>
#include <csm_event.h>
#include "_csm_ut.h"
#include "_csm_ut_utils.h"

vint CSM_UT_serviceRun = 1;

/*
 * ======== _CSM_UT_service_enable()  ========
 *
 * The CLI for user to enable IMS.
 *
 */
UT_Return _CSM_UT_service_enable(
    vint arg)
{
    CSM_InputEvent    event;
    CSM_InputService *service_ptr;

    OSAL_logMsg("%s:%d\n", __FUNCTION__, __LINE__);

    /* Construct CSM Event */
    event.type = CSM_EVENT_TYPE_SERVICE;
    service_ptr = &event.evt.service;
    service_ptr->reason = CSM_SERVICE_REASON_IMS_ENABLE;
    /* Set up reason */
    OSAL_strncpy(service_ptr->reasonDesc, "CSM_SERVICE_REASON_IMS_ENABLE", 
            sizeof(service_ptr->reasonDesc));

    /* Send to CSM */
    CSM_UT_writeCsmEvent(&event);

    return (UT_PASS);
}

/*
 * ======== _CSM_UT_service_disable()  ========
 *
 * The CLI for user to disable IMS.
 *
 */
UT_Return _CSM_UT_service_disable(
    vint arg)
{
    CSM_InputEvent    event;
    CSM_InputService *service_ptr;

    OSAL_logMsg("%s:%d\n", __FUNCTION__, __LINE__);

    /* Construct CSM Event */
    event.type = CSM_EVENT_TYPE_SERVICE;
    service_ptr = &event.evt.service;
    service_ptr->reason = CSM_SERVICE_REASON_IMS_DISABLE;
    /* Set up reason */
    OSAL_strncpy(service_ptr->reasonDesc, "CSM_SERVICE_REASON_IMS_DISABLE", 
            sizeof(service_ptr->reasonDesc)); 

    /* Send to CSM */
    CSM_UT_writeCsmEvent(&event);

    return (UT_PASS);
}

/*
 * ======== _CSM_UT_service_localDereg()  ========
 *
 * The CLI for user to local de-register.
 *
 */
UT_Return _CSM_UT_service_localDereg(
    vint arg)
{
    CSM_InputEvent    event;
    char              ipAddr[8] = "0.0.0.0";
    CSM_InputService *service_ptr;

    OSAL_logMsg("%s:%d\n", __FUNCTION__, __LINE__);

    /* send the ip change event to change ip as zero. */
    CSMUT_ipEventSend(ipAddr, 0);

    /* Construct CSM Event */ 
    event.type = CSM_EVENT_TYPE_SERVICE;
    service_ptr = &event.evt.service;
    service_ptr->reason = CSM_SERVICE_REASON_IMS_DISABLE; 
    /* Set up reason */ 
    OSAL_strncpy(service_ptr->reasonDesc, "CSM_SERVICE_REASON_IMS_DISABLE", 
            sizeof(service_ptr->reasonDesc)); 

    /* Send to CSM */
    CSM_UT_writeCsmEvent(&event); 

    return (UT_PASS);
}

CSM_UT_TestTableItem CSM_UT_cgiTypeTable[] =
{
    { "1",    NULL,     "CSM_SERVICE_ACCESS_TYPE_3GPP_GERAN"},
    { "2",    NULL,     "CSM_SERVICE_ACCESS_TYPE_3GPP_UTRAN_FDD"},
    { "3",    NULL,     "CSM_SERVICE_ACCESS_TYPE_3GPP_UTRAN_TDD"},
    { "4",    NULL,     "CSM_SERVICE_ACCESS_TYPE_3GPP_E_UTRAN_FDD"},
    { "5",    NULL,     "CSM_SERVICE_ACCESS_TYPE_3GPP_E_UTRAN_TDD"},    
};

/*
 * ======== _CSM_UT_service_setCgi()  ========
 *
 * The CLI for user to set CGI.
 *
 */
UT_Return _CSM_UT_service_setCgi(
    vint arg)
{
    CSM_InputEvent        event;
    CSM_InputService     *service_ptr;
    vint                  typeNum;
    vint                  x;
    CSM_UT_TestTableItem *item_ptr;
    char                  id[CSM_CGI_STRING_SZ + 1];
    vint                  type;

    OSAL_logMsg("%s:%d\n", __FUNCTION__, __LINE__);

    OSAL_logMsg("\n"
            "===========================\n"
            "D2 CSM CGI type list\n"
            "===========================\n"
            "Command  Description\n"
            "-------  -----------------\n");
    typeNum = sizeof(CSM_UT_cgiTypeTable)/sizeof(CSM_UT_cgiTypeTable[0]);
    for (x = 0; x < typeNum; x++) {
        item_ptr = &CSM_UT_cgiTypeTable[x];
        OSAL_logMsg("%-9s%s\n", item_ptr->cmd, item_ptr->desc);
    }
    OSAL_logMsg("type: ");
    _CSM_UT_getLine(id, CSM_CGI_STRING_SZ);
    type = OSAL_atoi(id);

    OSAL_logMsg("id: ");
    _CSM_UT_getLine(id, CSM_CGI_STRING_SZ);

    /* Construct CSM Event */ 
    event.type = CSM_EVENT_TYPE_SERVICE;
    service_ptr = &event.evt.service;
    service_ptr->reason = CSM_SERVICE_REASON_UPDATE_CGI; 
    service_ptr->u.cgi.type = type; 
    OSAL_strncpy(service_ptr->u.cgi.id, id, CSM_CGI_STRING_SZ);
    /* Set up reason */ 
    OSAL_strncpy(service_ptr->reasonDesc, "CSM_SERVICE_REASON_UPDATE_CGI",
            sizeof(service_ptr->reasonDesc));

    /* Send to CSM */
    CSM_UT_writeCsmEvent(&event);

    return (UT_PASS);
}

/*
 * ======== _CSM_UT_service_setPcscf()  ========
 *
 * The CLI for user to setup P-CSCF.
 *
 */
UT_Return _CSM_UT_service_setPcscf(
    vint arg)
{
    CSM_InputEvent    event;
    CSM_InputService *service_ptr;
    char              pcscf[CSM_EVENT_LONG_STRING_SZ + 1];

    OSAL_logMsg("P-CSCF: ");
    _CSM_UT_getLine(pcscf, CSM_EVENT_LONG_STRING_SZ + 1);
    OSAL_logMsg("Get P-CSCF : %s\n", pcscf);

    /* Construct CSM Event */
    event.type = CSM_EVENT_TYPE_SERVICE;
    service_ptr = &event.evt.service;
    service_ptr->reason = CSM_SERVICE_REASON_SET_PCSCF;
    OSAL_strncpy(service_ptr->reasonDesc, "CSM_SERVICE_REASON_SET_PCSCF", 
            sizeof(service_ptr->reasonDesc));
    OSAL_strncpy(service_ptr->u.pcscf, pcscf,
            sizeof(service_ptr->u.pcscf));

    /* Send to CSM */
    CSM_UT_writeCsmEvent(&event);

    return (UT_PASS);
}

/*
 * ======== _CSM_UT_service_quit ========
 *
 * Quit the service menu and return to main menu
 */
static UT_Return _CSM_UT_service_quit(
    int arg0)
{
    CSM_UT_serviceRun = 0;
    return UT_PASS;
}

CSM_UT_TestTableItem CSM_UT_serviceTable[] =
{
    { "imsEn",      _CSM_UT_service_enable,     "Enable IMS" },
    { "imsDis",     _CSM_UT_service_disable,    "Disable IMS" },
    { "localDereg", _CSM_UT_service_localDereg, "Local de-register" },
    { "setCgi",     _CSM_UT_service_setCgi,     "Setup CGI" },
    { "setPcscf",   _CSM_UT_service_setPcscf,   "Setup P-CSCF" },
    { "q",          _CSM_UT_service_quit,       "Quit (exit service menu"}
};

/*
 * ======== CSM_UT_service ========
 *
 * The main menu for the service opertion
 */
UT_Return CSM_UT_service (
)
{
    CSM_UT_serviceRun = 1;
    vint              item;
    vint              itemMax;
    CSM_UT_TestTableItem *item_ptr;
    CSM_UT_TestFunc      *func_ptr;
    int32             arg;
    char              buf[10];
    vint              printMenu;

    printMenu = 1;
    OSAL_taskDelay(500);    /* cleans up printing */
    itemMax = sizeof(CSM_UT_serviceTable)/sizeof(CSM_UT_serviceTable[0]);
    while (CSM_UT_serviceRun) {
        if (printMenu > 0) {
            printMenu = 0;
            OSAL_logMsg("\n"
                    "===========================\n"
                    "D2 CSM service test suite\n"
                    "================================================\n"
                    "Command        Description\n"
                    "-------        ---------------------------------------\n");            

            for (item = 0; item < itemMax; item++) {
                item_ptr = &CSM_UT_serviceTable[item];
                OSAL_logMsg("%-9s%s\n", item_ptr->cmd, item_ptr->desc);
            }
        }

        OSAL_logMsg("\nCmd: ");
        _CSM_UT_getLine(buf, 2 * sizeof(CSM_UT_serviceTable[0].cmd));
        for (item = 0, func_ptr = NULL; item < itemMax; item++) {
            item_ptr = &CSM_UT_serviceTable[item];
            if (0 == OSAL_strncmp(buf, (char *)item_ptr->cmd, sizeof(buf))) {
                func_ptr = item_ptr->func_ptr;
                break;
            }
        }

        if (NULL != func_ptr) {
            OSAL_logMsg("\n"
                    "==============================================\n"
                    "%s\n"
                    "==============================================\n",
                    item_ptr->desc);
            arg = 0;
            func_ptr(arg);
            printMenu = 0;
        }
        else {
            /* Unknown cmd, print help */
            printMenu = 1;
        }        
    }
    OSAL_logMsg("\n%s:%d  UT_menu return\n", __FILE__, __LINE__);

    return UT_PASS;
}


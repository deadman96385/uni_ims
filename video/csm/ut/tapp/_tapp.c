/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29252 $ $Date: 2014-10-10 08:15:06 +0800 (Fri, 10 Oct 2014) $
 *
 */

#include <osal.h>
#include <isi.h>
#include <isip.h>
#include <csm_event.h>
#include <xcap.h>
#include "_tapp.h"
#include <settings.h>
#include <csm_main.h>
#include "../../_csm_response.h"
#include "_tapp_mock_sapp.h"
#include "_tapp_mock_gsm.h"
#include "_tapp_mock_vpmd.h"
#include "_tapp_at_infc.h"
#include "_tapp_xml.h"
#include "_tapp_report.h"
#include <ezxml_mem.h>

extern TAPP_EnumObj _TAPP_IsipCodeTable[];
extern TAPP_EnumObj _TAPP_IsipCallReasonTable[];
extern TAPP_EnumObj _TAPP_IsipServiceReasonTable[];
extern TAPP_EnumObj _TAPP_IsipSystemReasonTable[];
extern TAPP_EnumObj _TAPP_IsipStatusTable[];
extern TAPP_EnumObj _TAPP_IsipSessionDirTable[];
extern TAPP_EnumObj _TAPP_IsipServerTypeTable[];
extern TAPP_EnumObj _TAPP_IsipTextReasonTable[];
extern TAPP_EnumObj _TAPP_IsipMessageTypeTable[];
extern TAPP_EnumObj _TAPP_IsipSessionCidTypeTable[];
extern TAPP_EnumObj _TAPP_RpcIsiFuncTypeTable[];
extern TAPP_EnumObj _TAPP_NetworkAccessTypeTable[];
extern TAPP_EnumObj _TAPP_IsipUssdReasonTable[];
extern TAPP_EnumObj _TAPP_IsipPresReasonTable[];

char *_TAPP_ActionTypeStrings[TAPP_ACTION_TYPE_LAST + 1] = {
    "TAPP_ACTION_TYPE_ISSUE_AT",
    "TAPP_ACTION_TYPE_VALIDATE_AT",
    "TAPP_ACTION_TYPE_ISSUE_ISIP",
    "TAPP_ACTION_TYPE_VALIDATE_ISIP",
    "TAPP_ACTION_TYPE_ISSUE_GSM_AT",
    "TAPP_ACTION_TYPE_VALIDATE_GSM_AT",
    "TAPP_ACTION_TYPE_ISSUE_CSM",
    "TAPP_ACTION_TYPE_ISSUE_XCAP",
    "TAPP_ACTION_TYPE_VALIDATE_XCAP",
    "TAPP_ACTION_TYPE_ISSUE_ISI_RPC",
    "TAPP_ACTION_TYPE_VALIDATE_ISI_RPC",
    "TAPP_ACTION_TYPE_VALIDATE_ISI_RPC_RETURN",
    "TAPP_ACTION_TYPE_VALIDATE_ISI_GET_EVT",
    "TAPP_ACTION_TYPE_PAUSE",
    "TAPP_ACTION_TYPE_CLEAN_ISIP",
    "TAPP_ACTION_TYPE_REPEAT_START",
    "TAPP_ACTION_TYPE_REPEAT_END",
    "TAPP_ACTION_TYPE_LAST",
};

char *_TAPP_EventTypeStrings[TAPP_EVENT_TYPE_LAST + 1] = {
    "TAPP_EVENT_TYPE_ISIP",
    "TAPP_EVENT_TYPE_AT_INFC",
    "TAPP_EVENT_TYPE_GSM_DEV",
    "TAPP_EVENT_TYPE_TIMEOUT",
    "TAPP_EVENT_TYPE_XCAP",
    "TAPP_EVENT_TYPE_ISI_RPC",
    "TAPP_EVENT_TYPE_LAST",
};

/*
 * Private Q structure to mimic OSAL message Q for retriving fd inside
 * the OSAL message Q.
 */
typedef struct {
    char    name[128];
    int     fid;
    uint32  sz;
} _TAPP_MsgQParams;

/*
 * ======== _TAPP_initFdSet() ========
 *
 * Private function to initialized osal select set.
 * This function adds fd of gsm dev, at interface and fd inside
 * isiCmd message Q to the fdSet in tapp global object.
 *
 * Returns:
 *   TAPP_PASS: Select set initialized successfully.
 *   TAPP_FAIL: Failed to initialize select set.
 */
TAPP_Return _TAPP_initFdSet(
    TAPP_GlobalObj *global_ptr)
{
    /*
     * XXX Currently TAPP doesn't do group select on at, gsm and isiCmd.
     * This function and related data field could be remove in the future.
     */
    OSAL_SelectSet      *fdSet_ptr;
    _TAPP_MsgQParams    *q_ptr;

    /* Get the fd inside the message queue */
    q_ptr = (_TAPP_MsgQParams *)global_ptr->queue.isiCmd;

    /* Get fd inside the isiCmd msg Q */
    global_ptr->fd.isiCmd = q_ptr->fid;
    
    /* do the same for xcap queue */
    q_ptr = (_TAPP_MsgQParams *)global_ptr->queue.xcapCmd;
    global_ptr->fd.xcapCmd = q_ptr->fid;

    /* Get the fd inside the message queue */
    q_ptr = (_TAPP_MsgQParams *)global_ptr->queue.rpcRead;
    /* Get fd inside the rpc msg Q */
    global_ptr->fd.rpcCmd = q_ptr->fid;

    /* Get the fd inside the message queue */
    q_ptr = (_TAPP_MsgQParams *)global_ptr->queue.rpcREvt;
    /* Get fd inside the rpc Event cmd Q */
    global_ptr->fd.rpcEvtCmd = q_ptr->fid;

    /* Get fd set pointer for later use */
    fdSet_ptr = &global_ptr->fd.set;

    /* Create a select set */
    OSAL_memSet(fdSet_ptr, 0, sizeof(OSAL_SelectSet));

    if (OSAL_SUCCESS != OSAL_selectSetInit(fdSet_ptr)) {
        return (TAPP_FAIL);
    }
    /* Add gsm to select set */
    if (OSAL_SUCCESS != OSAL_selectAddId(&global_ptr->fd.gsm, fdSet_ptr)) {
        return (TAPP_FAIL);
    }
    /* Add at interface to select */
    if (OSAL_SUCCESS != OSAL_selectAddId(&global_ptr->fd.atInfc, fdSet_ptr)) {
        return (TAPP_FAIL);
    }
    /* Add isi commandQ to select */
    if (OSAL_SUCCESS != OSAL_selectAddId(&global_ptr->fd.isiCmd, fdSet_ptr)) {
        return (TAPP_FAIL);
    }
    return (TAPP_PASS);
}

/*
 * ======== _TAPP_getFileNameAndFolder() ========
 *
 * Private function to get file name and which folder file in it.
 *
 * Returns:
 *   TAPP_PASS: Get file's folder and file name successfully.
 *   TAPP_FAIL: The input, file name path, is not correct.
 */
TAPP_Return _TAPP_getFileNameAndFolder(
    TAPP_GlobalObj *global_ptr,
    char           *currFolderPath,
    char           *fileName_ptr)
{
    char *s_ptr;
    char *delimiter_ptr   = "/";
    char  folderPath[TAPP_SCRATCH_STRING_SZ] = "\0";
    char  fileName[TAPP_SCRATCH_STRING_SZ]   = "\0";

    /* If the length of fileName_ptr is 0, return FAIL. */
    if (0 == OSAL_strlen(fileName_ptr)) {
        OSAL_logMsg("%s, %d: The file path is empty.\n", 
                __FUNCTION__, __LINE__);
        return (TAPP_FAIL);
    }

    /* If last char is '/', return FAIL. It is not a file. */
    if (0 == OSAL_strcmp(delimiter_ptr, 
            &fileName_ptr[OSAL_strlen(fileName_ptr) - 1])) {
        OSAL_logMsg("%s, %d: The path is not a file. The path is %s\n", 
                __FUNCTION__, __LINE__, fileName_ptr);
        return (TAPP_FAIL);
    }

    /* get folder path and file name of fileName_ptr. */
    if (NULL != OSAL_strscan(fileName_ptr, delimiter_ptr)) {
        if ('/' == fileName_ptr[0]) {
            OSAL_strcpy(folderPath, "/");
        }
        s_ptr = OSAL_strtok(fileName_ptr, delimiter_ptr);
        while (NULL != s_ptr ) {
            OSAL_strncpy(fileName, s_ptr, TAPP_SCRATCH_STRING_SZ);
            OSAL_snprintf(folderPath + OSAL_strlen(folderPath), 
                    sizeof(folderPath), "%s/", s_ptr);
            s_ptr = strtok(NULL, delimiter_ptr);
        }
        folderPath[OSAL_strlen(folderPath) - (OSAL_strlen(fileName)+1)] = '\0';
    }
    else {
        OSAL_strcpy(fileName, fileName_ptr);
        OSAL_snprintf(folderPath, sizeof(folderPath), "./%s", fileName_ptr);
        folderPath[OSAL_strlen(folderPath) - OSAL_strlen(fileName)] = '\0';
    }

    OSAL_strncpy(currFolderPath, folderPath, TAPP_SCRATCH_STRING_SZ);
    OSAL_strncpy(fileName_ptr, fileName, TAPP_SCRATCH_STRING_SZ);

    return (TAPP_PASS);
}

/*
 * ======== _TAPP_addAction() ========
 *
 * Private function to add an action to global object's action list
 *
 * Returns:
 *  TAPP_PASS: Action added.
 *  TAPP_FAIL: Failed to add action.
 */
TAPP_Return _TAPP_addAction(
    TAPP_GlobalObj *global_ptr,
    TAPP_Action    *action_ptr,
    vint           *idx_ptr)
{
    if (TAPP_MAX_ACTION_SZ <= global_ptr->actionList.size) {
        /* No more space to add */
        return (TAPP_FAIL);
    }

    /*
     * Process the end of test case.
     * Check if this action the begin of a test case
     * Always the current case to the end of test case.
     */
    action_ptr->endOfTestCase = OSAL_TRUE;
    if (0 == action_ptr->testCaseName[0]) {
        /* Not beginning the a test case, previous action is not the end. */
        global_ptr-> actionList.actions[global_ptr->actionList.size - 1].
                endOfTestCase = OSAL_FALSE;
    }

    *idx_ptr = global_ptr->actionList.size;
    /* Copy the action and increase size */
    global_ptr->actionList.actions[global_ptr->actionList.size++] =
            *action_ptr;
    
    return (TAPP_PASS);
}

/*
 * ======== TAPP_readTestCase() ========
 *
 * This function is to read test cases from the arguments of tapp_main
 * and store all actions the action list in the global object
 *
 * Returns:
 *  TAPP_PASS: Read and store the actions from test cases to action list.
 *  TAPP_FAIL: Failed to read test cases from test case files.
 */
TAPP_Return TAPP_readTestCase(
    TAPP_GlobalObj *global_ptr,
    char           *filename_ptr)
{
    ezxml_t     xml_ptr;
    char       *xmlDoc_ptr;
    int         xmlDocLen;
    char        currFolderPath[TAPP_SCRATCH_STRING_SZ];

    if (TAPP_PASS != _TAPP_getFileNameAndFolder(global_ptr, currFolderPath, 
            filename_ptr)) {
        return (TAPP_FAIL);
    }
    OSAL_snprintf(global_ptr->scratch, sizeof(global_ptr->scratch), "%s%s",
            currFolderPath, filename_ptr);

    /* Read test case xml file */
    if (0 != ezxml_alloc_str(global_ptr->scratch, &xmlDoc_ptr, &xmlDocLen)) {
        TAPP_rptOutput("Read test case file failed: %s\n",
            (int)global_ptr->scratch, 0, 0);
        return (TAPP_FAIL);
    }

    /* Parse the xml file */
    xml_ptr = ezxml_parse_str(xmlDoc_ptr, xmlDocLen);
    if (NULL == xml_ptr) {
        EZXML_memFree(xmlDoc_ptr, OSAL_MEM_ARG_DYNAMIC_ALLOC);
        return (TAPP_FAIL);
    }

    if (TAPP_PASS != _TAPP_xmlParseTestCase(global_ptr, currFolderPath, 
            xml_ptr)) {
        OSAL_logMsg("%s:%d Parse test case filed\n", __FUNCTION__, __LINE__);
        ezxml_free(xml_ptr);
        EZXML_memFree(xmlDoc_ptr, OSAL_MEM_ARG_DYNAMIC_ALLOC);
        return (TAPP_FAIL);
    }

    ezxml_free(xml_ptr);
    EZXML_memFree(xmlDoc_ptr, OSAL_MEM_ARG_DYNAMIC_ALLOC);
    return (TAPP_PASS);
}

/*
 * ======== TAPP_csmInit() ========
 *
 * This function is to initialized CSM
 *
 * Returns:
 *  TAPP_PASS: CSM initialized successfully.
 *  TAPP_FAIL: Failed to initialize CSM.
 */
TAPP_Return TAPP_csmInit(
    TAPP_GlobalObj *global_ptr)
{
    void *cfg_ptr = SETTINGS_cfgMemAlloc(SETTINGS_TYPE_CSM);

    /* Get the XML init info */
    if (SETTINGS_RETURN_OK != SETTINGS_getContainer(SETTINGS_TYPE_CSM,
            global_ptr->csmXmlFileName, cfg_ptr)) {
        return (TAPP_FAIL);
    }

    /* 
     * Initialize & start CSM 
     */
    if (0 != CSM_init(cfg_ptr)) {
        SETTINGS_memFreeDoc(SETTINGS_TYPE_CSM, cfg_ptr);
        return (TAPP_FAIL);
    }

    /* Free memory */
    SETTINGS_memFreeDoc(SETTINGS_TYPE_CSM, cfg_ptr);

    return (TAPP_PASS);
}

void TAPP_csmShutdown(void)
{
    CSM_shutdown();
    return;
}

/*
 * ======== TAPP_getXmlNestedTagText() ========
 *
 * This function retrieves the text of the XML tag nested underneath
 * a parent tag.
 *
 * Returns:
 *  A pointer to a string containing the text to the nested tag or
 *  NULL if the tag could not be found or if there is no text.
 *
 */
char* TAPP_getXmlNestedTagText(
    ezxml_t     xml_ptr,
    const char *parentTag_ptr,
    const char *childTag_ptr)
{
    ezxml_t   child_ptr;

    /* Get the parent tag.  If its exists then search for the child tag */
    if (NULL == (child_ptr = ezxml_child(xml_ptr, parentTag_ptr))) {
        return (NULL);
    }
    /* Get the child */
    if (NULL == (child_ptr = ezxml_child(child_ptr, childTag_ptr))) {
        return (NULL);
    }
    return ezxml_txt(child_ptr);
}

/*
 * ======== TAPP_readConfiguration() ========
 *
 * This function is to read the configuration from configuration xml file.
 *
 * Returns:
 *  TAPP_PASS: Configuration read.
 *  TAPP_FAIL: Failed to read configuration.
 */
static TAPP_Return TAPP_readConfiguration(
    TAPP_GlobalObj *global_ptr,
    char           *xmlDoc_ptr,
    int             xmlDocLen)
{
    ezxml_t xml_ptr;
    char   *value_ptr;

    /* Parse the init file */
    xml_ptr = ezxml_parse_str(xmlDoc_ptr, xmlDocLen);
    if (NULL == xml_ptr) {
        return (-1);
    }

    /* Get testsuit name */
    value_ptr = TAPP_getXmlNestedTagText(xml_ptr, "testsuite", "name");
    if (NULL != value_ptr) {
        OSAL_strncpy(global_ptr->testsuiteName, value_ptr,
                sizeof(global_ptr->testsuiteName));
    }

    /* Get report file name */
    value_ptr = TAPP_getXmlNestedTagText(xml_ptr, "report", "filename");
    if (NULL != value_ptr) {
        OSAL_strncpy(global_ptr->report.fileName, value_ptr,
                sizeof(global_ptr->report.fileName));
    }

    /* Get xml report file name */
    value_ptr = TAPP_getXmlNestedTagText(xml_ptr, "xmlReport", "filename");
    if (NULL != value_ptr) {
        OSAL_strncpy(global_ptr->xmlReport.fileName, value_ptr,
                sizeof(global_ptr->xmlReport.fileName));
    }

    /* Get GSM device name */
    value_ptr = TAPP_getXmlNestedTagText(xml_ptr, "gsm", "device");
    if (NULL == value_ptr) {
        OSAL_logMsg("%s:%d Failed to get GSM device name\n", __FILE__, __LINE__);
        ezxml_free(xml_ptr);
        return (TAPP_FAIL);
    }

    OSAL_strncpy(global_ptr->gsmDevName, value_ptr,
            sizeof(global_ptr->gsmDevName));

    /* Get csm xml file name */
    value_ptr = TAPP_getXmlNestedTagText(xml_ptr, "csm", "xml");
    if (NULL == value_ptr) {
        OSAL_logMsg("%s:%d Failed to get csm xml file name\n", __FILE__, __LINE__);
        ezxml_free(xml_ptr);
        return (TAPP_FAIL);
    }

    OSAL_strncpy(global_ptr->csmXmlFileName, value_ptr,
            sizeof(global_ptr->csmXmlFileName));
    ezxml_free(xml_ptr);
    return (TAPP_PASS);
}

/*
 * ======== TAPP_init() ========
 *
 * This function is to initialize all sub-modules' init routine.
 *
 * Returns:
 *  TAPP_PASS: Initialized successfully.
 *  TAPP_FAIL: Failed to initialized tapp.
 */
TAPP_Return TAPP_init(
    TAPP_GlobalObj **pGlobal_ptr,
    char            *filename_ptr)
{
    char   *xmlDoc_ptr;
    int     xmlDocLen;

    /* Allocate memory for global object */
    if (NULL != *pGlobal_ptr) {
        /* Initialized already */
        return (TAPP_FAIL);
    }

    /* 
     * Init and clear the global object
     */
    *pGlobal_ptr = OSAL_memCalloc(1, sizeof(TAPP_GlobalObj), 0);
    OSAL_memSet(*pGlobal_ptr, 0, sizeof(TAPP_GlobalObj));


    /* Read TAPP configuration xml file */
    if (0 != ezxml_alloc_str(filename_ptr, &xmlDoc_ptr, &xmlDocLen)) {
        return (TAPP_FAIL);
    }

    if (TAPP_PASS != TAPP_readConfiguration(*pGlobal_ptr, xmlDoc_ptr,
            xmlDocLen)) {
        OSAL_logMsg("%s:%d Failed to read tapp xml configuration file\n",
                __FILE__, __LINE__);
        EZXML_memFree(xmlDoc_ptr, OSAL_MEM_ARG_DYNAMIC_ALLOC);
        return (TAPP_FAIL);
    }

    EZXML_memFree(xmlDoc_ptr, OSAL_MEM_ARG_DYNAMIC_ALLOC);

    /* Init mock GSM */
    if (TAPP_PASS != TAPP_mockGsmInit(*pGlobal_ptr)) {
        OSAL_logMsg("%s:%d Init mock GSM device failed\n", __FILE__, __LINE__);
        return (TAPP_FAIL);
    }

    /* Init CSM */
    if (TAPP_PASS != TAPP_csmInit(*pGlobal_ptr)) {
        OSAL_logMsg("%s:%d Init CSM failed\n", __FILE__, __LINE__);
        TAPP_mockGsmShutdown(*pGlobal_ptr);
        return (TAPP_FAIL);
    }

    /* Initialize VPMD */
    if (OSAL_FAIL == VPMD_init()) {
        OSAL_logMsg("%s:%d Init VPMD failed.\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }
    /* Init AT interface */
    if (TAPP_PASS != TAPP_atInfcInit(*pGlobal_ptr)) {
        OSAL_logMsg("%s:%d Init AT interface failed\n", __FILE__, __LINE__);
        TAPP_csmShutdown();
        TAPP_mockGsmShutdown(*pGlobal_ptr);
        return (TAPP_FAIL);
    }
    
    /* Init fd set */
    if (TAPP_PASS != _TAPP_initFdSet(*pGlobal_ptr)) {
        OSAL_logMsg("%s:%d Initialized fd set failed\n", __FILE__, __LINE__);
        TAPP_atInfcShutdown(*pGlobal_ptr);
        TAPP_csmShutdown();
        TAPP_mockGsmShutdown(*pGlobal_ptr);
        return (TAPP_FAIL);
    }

    /*
     * Init tapp report, need to init it after read report filename from
     * tapp.xml.
     */
    TAPP_rptInit();

    /* Wait few seconds for protocol ready */
    OSAL_taskDelay(5000);

    return (TAPP_PASS);
}

/*
 * ======== TAPP_shutdown() ========
 *
 * This function is to shutdown all sub-modules and destroy all the resources.
 *
 * Returns:
 *     TAPP_PASS: TAPP shutdown successfully
 */
TAPP_Return TAPP_shutdown(
    TAPP_GlobalObj *global_ptr)
{

    TAPP_atInfcShutdown(global_ptr);
    TAPP_csmShutdown();
    TAPP_mockGsmShutdown(global_ptr);
    TAPP_rptShutdown();

    /* free global object */
    OSAL_memFree(global_ptr, 0);
    global_ptr = NULL;

    return (TAPP_PASS);
}

/*
 * ======== TAPP_processRepeatStart() ========
 *
 * This function is to process repeat start action
 *
 * Returns:
 *   TAPP_FAIL: Process failed.
 *   TAPP_PASS: Process done.
 */
TAPP_Return TAPP_processRepeatStart(
    TAPP_GlobalObj *global_ptr,
    TAPP_Action    *action_ptr)
{
    /* If repeat less than zero, might be some problem */
    if (0 >= action_ptr->u.repeat) {
        return (TAPP_FAIL);
    }

    /* Decrease repeat time */
    action_ptr->u.repeat--;

    return (TAPP_PASS);
}

/*
 * ======== TAPP_processRepeatEnd() ========
 *
 * This function is to process repeat end action
 *
 * Returns:
 *   TAPP_FAIL: Process failed.
 *   TAPP_PASS: Process done.
 */
TAPP_Return TAPP_processRepeatEnd(
    TAPP_GlobalObj *global_ptr,
    TAPP_Action    *action_ptr)
{
    TAPP_Action *startAction_ptr;
    vint *curActionIdx_ptr;
    vint  idx;

    idx = action_ptr->u.repeat;
    curActionIdx_ptr = &global_ptr->actionList.curIdx;

    /* For repeat end, repeat indicates the loop start index */
    if ((0 >= idx) ||
            (*curActionIdx_ptr <= idx)) { /* Should not be looping forward */
        return (TAPP_FAIL);
    }

    startAction_ptr = &global_ptr->actionList.actions[idx];
    if (TAPP_ACTION_TYPE_REPEAT_START != startAction_ptr->type) {
        return (TAPP_FAIL);
    }
    if (0 < startAction_ptr->u.repeat) {
        /* Loop to the start index */
        (*curActionIdx_ptr) = idx;
    }

    return (TAPP_PASS);
}

/*
 * ======== TAPP_getNextAction() ========
 *
 * This function is to get next action from action list
 *
 * Returns:
 *   TAPP_FAIL: No action is gotten.
 *   TAPP_PASS: An action is gotten and stored in the action_ptr
 */
TAPP_Return TAPP_getNextAction(
    TAPP_GlobalObj *global_ptr,
    TAPP_Action   **pAction_ptr)
{
    vint *curActionIdx_ptr;

    curActionIdx_ptr = &global_ptr->actionList.curIdx;
    if (*curActionIdx_ptr >= global_ptr->actionList.size) {
        /* No more action to get */
        return (TAPP_FAIL);
    }

    /* Point the action pointer the current index and increase current index */
    *pAction_ptr = 
            &global_ptr->actionList.actions[(*curActionIdx_ptr)++];

    return (TAPP_PASS);
}

/*
 * ======== _TAPP_eventFilter() ========
 *
 * This function is to filter out the event we want to ignore.
 *
 * Returns:
 *   TAPP_PASS: The event is the one we want to ignore.
 *   TAPP_FAIL: The event is the one we don't want to ignore.
 */
static TAPP_Return _TAPP_eventFilter(
    TAPP_Event  *event_ptr)
{
    switch (event_ptr->type) {
        case TAPP_EVENT_TYPE_ISIP:
            switch (event_ptr->msg.isip.code) {
                case ISIP_CODE_SYSTEM:
                    if (ISIP_SYSTEM_REASON_START ==
                            event_ptr->msg.isip.msg.system.reason) {
                        return (TAPP_PASS);
                    }
                case ISIP_CODE_SERVICE:
                    if (ISIP_SERVICE_REASON_CODERS ==
                            event_ptr->msg.isip.msg.service.reason) {
                        return (TAPP_PASS);
                    }
                default:
                    break;
            }
        default:
            break;
    }
    return (TAPP_FAIL);
}

/*
 * ======== TAPP_getInputEvent() ========
 *
 * This function is to get input event from ISI, AT inteface or GAPP or XCAP
 * A timeout value should be given to this APIs.
 *
 * Returns:
 *   TAPP_PASS: An event is gotten and stored in evt_ptr.
 *   TAPP_FAIL: Failed to get input event.
 */
TAPP_Return TAPP_getInputEvent(
    TAPP_GlobalObj  *global_ptr,
    TAPP_Action     *action_ptr,
    TAPP_Event      *evt_ptr,
    vint             timeout)
{
    OSAL_SelectSet      fdSet;
    OSAL_SelectTimeval  time;
    OSAL_SelectTimeval *timeout_ptr;
    OSAL_Boolean        isTimedOut;
    vint                size;
    TAPP_ActionType     actionType;

    OSAL_memSet(&time, 0, sizeof(OSAL_SelectTimeval));
    time.sec= timeout / 1000;
    time.usec= timeout % 1000;
    timeout_ptr = &time;

    actionType = action_ptr->type;

    TAPP_dbgPrintf("Want to get Event type: %s\n",
           _TAPP_ActionTypeStrings[actionType]);
    
_TAPP_GET_INPUT_EVENT_LOOP:
    /* Clean up TAPP_Event */
    OSAL_memSet(evt_ptr, 0, sizeof(TAPP_Event));

    /* Select Fd and read file. */
    OSAL_selectSetInit(&fdSet);

    switch (actionType) {
        case TAPP_ACTION_TYPE_VALIDATE_GSM_AT:
            OSAL_selectAddId(&global_ptr->fd.gsm, &fdSet);
            if (OSAL_FAIL == 
                    OSAL_select(&fdSet, NULL, timeout_ptr, &isTimedOut)) {
                return (TAPP_FAIL);
            }
            if (isTimedOut == OSAL_TRUE) {
                evt_ptr->type = TAPP_EVENT_TYPE_TIMEOUT;
                return (TAPP_FAIL);
            }

            size = TAPP_AT_COMMAND_STRING_SZ;
            OSAL_fileRead(&global_ptr->fd.gsm, evt_ptr->msg.at, &size);
            if (0 >= size) {
                return (TAPP_FAIL);
            }
            evt_ptr->msg.at[size] = 0;
            evt_ptr->type = TAPP_EVENT_TYPE_GSM_DEV;

            break;
        case TAPP_ACTION_TYPE_VALIDATE_AT:
            OSAL_selectAddId(&global_ptr->fd.atInfc, &fdSet);
            if (OSAL_FAIL == 
                    OSAL_select(&fdSet, NULL, timeout_ptr, &isTimedOut)) {
                return (TAPP_FAIL);
            }
            if (isTimedOut == OSAL_TRUE) {
                evt_ptr->type = TAPP_EVENT_TYPE_TIMEOUT;
                return (TAPP_FAIL);
            }

            size = OSAL_strlen(action_ptr->msg.at);
            OSAL_fileRead(&global_ptr->fd.atInfc, evt_ptr->msg.at, &size);
            if (0 >= size) {
                return (TAPP_FAIL);
            }
            evt_ptr->msg.at[size] = 0;
            evt_ptr->type = TAPP_EVENT_TYPE_AT_INFC;

            break;
        case TAPP_ACTION_TYPE_VALIDATE_ISIP:
        case TAPP_ACTION_TYPE_CLEAN_ISIP:
            OSAL_selectAddId(&global_ptr->fd.isiCmd, &fdSet);
            if (OSAL_FAIL == 
                    OSAL_select(&fdSet, NULL, timeout_ptr, &isTimedOut)) {
                return (TAPP_FAIL);
            }
            if (isTimedOut == OSAL_TRUE) {
                evt_ptr->type = TAPP_EVENT_TYPE_TIMEOUT;
                return (TAPP_FAIL);
            }
            size = sizeof(ISIP_Message);
            OSAL_fileRead(&global_ptr->fd.isiCmd, &evt_ptr->msg.isip,
                    &size);
            if (0 >= size) {
                return (TAPP_FAIL);
            }
            evt_ptr->type = TAPP_EVENT_TYPE_ISIP;

            break;
        case TAPP_ACTION_TYPE_VALIDATE_XCAP:
            OSAL_selectAddId(&global_ptr->fd.xcapCmd, &fdSet);
            if (OSAL_FAIL == 
                    OSAL_select(&fdSet, NULL, timeout_ptr, &isTimedOut)) {
                return (TAPP_FAIL);
            }
            if (isTimedOut == OSAL_TRUE) {
                evt_ptr->type = TAPP_EVENT_TYPE_TIMEOUT;
                return (TAPP_FAIL);
            }
            size = sizeof(XCAP_Cmd);
            OSAL_fileRead(&global_ptr->fd.xcapCmd, &evt_ptr->msg.xcapCmd,
                    &size);
            if (0 >= size) {
                return (TAPP_FAIL);
            }
            evt_ptr->type = TAPP_EVENT_TYPE_XCAP;
            break;
        case TAPP_ACTION_TYPE_VALIDATE_ISI_RPC_RETURN:
            OSAL_selectAddId(&global_ptr->fd.rpcCmd, &fdSet);
           
            if (OSAL_FAIL == 
                    OSAL_select(&fdSet, NULL, timeout_ptr, &isTimedOut)) {
                return (TAPP_FAIL);
            }
            if (isTimedOut == OSAL_TRUE) {
                evt_ptr->type = TAPP_EVENT_TYPE_TIMEOUT;
                return (TAPP_FAIL);
            }
            evt_ptr->msg.rpc.isiXdr.length = ISI_DATA_SIZE;
            OSAL_fileRead(&global_ptr->fd.rpcCmd,
                    evt_ptr->msg.rpc.isiXdr.data,
                    &evt_ptr->msg.rpc.isiXdr.length);
            if (0 >= evt_ptr->msg.rpc.isiXdr.length) {
                return (TAPP_FAIL);
            }

            evt_ptr->type = TAPP_EVENT_TYPE_ISI_RPC;
            break;
        case TAPP_ACTION_TYPE_VALIDATE_ISI_GET_EVT:
        case TAPP_ACTION_TYPE_CLEAN_ISI_EVT:
            OSAL_selectAddId(&global_ptr->fd.rpcEvtCmd, &fdSet);
           
            if (OSAL_FAIL == 
                    OSAL_select(&fdSet, NULL, timeout_ptr, &isTimedOut)) {
                return (TAPP_FAIL);
            }
            if (isTimedOut == OSAL_TRUE) {
                evt_ptr->type = TAPP_EVENT_TYPE_TIMEOUT;
                return (TAPP_FAIL);
            }
            evt_ptr->msg.rpc.isiXdr.length = ISI_DATA_SIZE;
            OSAL_fileRead(&global_ptr->fd.rpcEvtCmd,
                    evt_ptr->msg.rpc.isiXdr.data,
                    &evt_ptr->msg.rpc.isiXdr.length);
            if (0 >= evt_ptr->msg.rpc.isiXdr.length) {
                return (TAPP_FAIL);
            }

            evt_ptr->type = TAPP_EVENT_TYPE_ISI_RPC;
            break;
        default:
            return (TAPP_FAIL);
    }

    /* Print out the event */
    _TAPP_printEvent(evt_ptr);

    /* Filter out unwanted event */
    if (TAPP_PASS == _TAPP_eventFilter(evt_ptr)) {
        /* This event is the one we want to ignore, get next one */
        goto _TAPP_GET_INPUT_EVENT_LOOP;
    }

    return (TAPP_PASS);
}

/* 
 * ======== _TAPP_printBytes() ========
 * 
 * Private function to print the input string with length specified
 *
 * Returns: 
 *    None
 */
void TAPP_printBytes(
    unsigned char *s_ptr,
    int len)
{
    int x;
    for (x = 0 ; x < len ; x++) {
        OSAL_logMsg("%02X ", s_ptr[x]);
    }
    OSAL_logMsg("\n");
}

/*
 * ======== _TAPP_printEvent() ========
 *
 * Private funtion to print content of TAPP_Event
 *
 * Returns:
 *  None.
 */
void _TAPP_printEvent(
    TAPP_Event *event_ptr)
{
    TAPP_dbgPrintf("Event type: %s\n", _TAPP_EventTypeStrings[event_ptr->type]);
    ISIP_Message *isipEvt_ptr;

    isipEvt_ptr = &event_ptr->msg.isip;
    switch (event_ptr->type) {
        case TAPP_EVENT_TYPE_ISIP:
            TAPP_dbgPrintf("ISIP Code: %s\n", 
                    _TAPP_IsipCodeTable[isipEvt_ptr->code].string);
            TAPP_dbgPrintf("ISIP Protocol: %d\n",
                    isipEvt_ptr->protocol);
            if (ISIP_CODE_CALL == isipEvt_ptr->code) {
                TAPP_dbgPrintf("ISIP Reason: %d %s\n",
                    isipEvt_ptr->msg.call.reason,
                    _TAPP_IsipCallReasonTable[
                    isipEvt_ptr->msg.call.reason].string);
                TAPP_dbgPrintf("ISIP Status: %s\n",
                    _TAPP_IsipStatusTable[
                    isipEvt_ptr->msg.call.status].string);
                if (!isipEvt_ptr->msg.call.from[0]) {
                    TAPP_dbgPrintf("ISIP from: %s\n", 
                          isipEvt_ptr->msg.call.from);
                }
                if (!isipEvt_ptr->msg.call.to[0]) {
                    TAPP_dbgPrintf("ISIP to: %s\n", 
                          isipEvt_ptr->msg.call.to);
                }

                TAPP_dbgPrintf("ISIP Dir(%d): %s\n", 
                        isipEvt_ptr->msg.call.audioDirection,
                        _TAPP_IsipSessionDirTable[
                        isipEvt_ptr->msg.call.audioDirection].string);
                TAPP_dbgPrintf("ISIP call ID: %d\n",
                        isipEvt_ptr->id);
                if (ISIP_CALL_REASON_INITIATE == 
                        isipEvt_ptr->msg.call.reason) {
                    TAPP_dbgPrintf("ISIP Sesseion CID Type: %s\n",
                            _TAPP_IsipSessionCidTypeTable[
                            isipEvt_ptr->msg.call.cidType].string);
                }
            }
            else if (ISIP_CODE_SYSTEM == isipEvt_ptr->code) {
                TAPP_dbgPrintf("ISIP Reason: %s\n",
                        _TAPP_IsipSystemReasonTable[
                        isipEvt_ptr->msg.system.reason].string);
                TAPP_dbgPrintf("ISIP Status: %s\n",
                        _TAPP_IsipStatusTable[
                        isipEvt_ptr->msg.system.status].string);
            }
            else if (ISIP_CODE_MESSAGE == isipEvt_ptr->code) {
                TAPP_dbgPrintf("ISIP Reason: %s\n",
                        _TAPP_IsipTextReasonTable[
                        isipEvt_ptr->msg.message.reason].string);
                TAPP_dbgPrintf("ISIP Message Type: %s\n",
                        _TAPP_IsipMessageTypeTable[
                        isipEvt_ptr->msg.message.type].string);
                TAPP_dbgPrintf("ISIP message ID: %s\n",
                        isipEvt_ptr->msg.message.messageId);
                TAPP_dbgPrintf("ISIP PDU Len: %d\n",
                        isipEvt_ptr->msg.message.pduLen);
                TAPP_dbgPrintf("ISIP Message:");
                TAPP_printBytes(
                        (unsigned char *)isipEvt_ptr->msg.message.message,
                        isipEvt_ptr->msg.message.pduLen);
                TAPP_dbgPrintf("ISIP to: %s\n",
                        isipEvt_ptr->msg.message.to);
                TAPP_dbgPrintf("ISIP from: %s\n",
                        isipEvt_ptr->msg.message.from);
            }
            else if (ISIP_CODE_USSD == isipEvt_ptr->code) {
                TAPP_dbgPrintf("ISIP Reason: %s\n",
                        _TAPP_IsipUssdReasonTable[
                        isipEvt_ptr->msg.ussd.reason].string);
                TAPP_dbgPrintf("ISIP Message:");
                TAPP_printBytes(
                        (unsigned char *)isipEvt_ptr->msg.ussd.message,
                        OSAL_strlen(isipEvt_ptr->msg.ussd.message));
                TAPP_dbgPrintf("ISIP to: %s\n",
                        isipEvt_ptr->msg.ussd.to);
            }
            else if (ISIP_CODE_SERVICE == isipEvt_ptr->code) {
                TAPP_dbgPrintf("ISIP Reason:%d %s\n",
                        isipEvt_ptr->msg.service.reason,
                        _TAPP_IsipServiceReasonTable[
                        isipEvt_ptr->msg.service.reason].string);
                TAPP_dbgPrintf("ISIP Status: %s\n",
                        _TAPP_IsipStatusTable[
                        isipEvt_ptr->msg.service.status].string);
                if (ISIP_SERVICE_REASON_SERVER == 
                        isipEvt_ptr->msg.service.reason) {
                    TAPP_dbgPrintf("ISIP Service Server:%s\n",
                            _TAPP_IsipServerTypeTable[
                            isipEvt_ptr->msg.service.server].string);
                    TAPP_dbgPrintf("ISIP Service Service:%s\n",
                            isipEvt_ptr->msg.service.settings.server);
                }
                else if (ISIP_SERVICE_REASON_AUTH == 
                        isipEvt_ptr->msg.service.reason) {
                    TAPP_dbgPrintf("ISIP Service AUTH: uname=%s\n",
                            isipEvt_ptr->msg.service.settings.credentials.username);
                    TAPP_dbgPrintf("ISIP Service AUTH: password=%s\n",
                            isipEvt_ptr->msg.service.settings.credentials.password);
                    TAPP_dbgPrintf("ISIP Service AUTH: realm=%s\n",
                            isipEvt_ptr->msg.service.settings.credentials.realm);
                }
                else if (ISIP_SERVICE_REASON_IPSEC ==
                        isipEvt_ptr->msg.service.reason) {
                    TAPP_dbgPrintf("ISIP Service IPSEC: protected port=%d\n",
                            isipEvt_ptr->msg.service.settings.ipsec.cfg.protectedPort);
                    TAPP_dbgPrintf("ISIP Service IPSEC: protected port pool size=%d\n",
                            isipEvt_ptr->msg.service.settings.ipsec.cfg.protectedPortPoolSz);
                    TAPP_dbgPrintf("ISIP Service IPSEC: spi=%d\n",
                            isipEvt_ptr->msg.service.settings.ipsec.cfg.spi);
                    TAPP_dbgPrintf("ISIP Service IPSEC: spi pool size=%d\n",
                            isipEvt_ptr->msg.service.settings.ipsec.cfg.spiPoolSz);
                }
                else if (ISIP_SERVICE_REASON_BSID ==
                        isipEvt_ptr->msg.service.reason) {
                    TAPP_dbgPrintf("cgi-type:%s\n",
                            _TAPP_NetworkAccessTypeTable[
                            isipEvt_ptr->msg.service.settings.bsId.type].string);
                    TAPP_dbgPrintf("cgi:%s\n",
                            isipEvt_ptr->msg.service.settings.bsId.szBsId);
                }
            }
            else if (ISIP_CODE_PRESENCE == isipEvt_ptr->code) {
                TAPP_dbgPrintf("ISIP Reason: %s\n",
                        _TAPP_IsipPresReasonTable[
                        isipEvt_ptr->msg.presence.reason].string);
                TAPP_dbgPrintf("ISIP presence string: %s", 
                        isipEvt_ptr->msg.presence.presence);
            }
            break;
        case TAPP_EVENT_TYPE_AT_INFC:
        case TAPP_EVENT_TYPE_GSM_DEV:
            TAPP_dbgPrintf("AT command: %s\n", event_ptr->msg.at);
            break;
        case TAPP_EVENT_TYPE_XCAP:
            TAPP_dbgPrintf("XCAP event:\n");
            break;
        case TAPP_EVENT_TYPE_ISI_RPC:
            TAPP_dbgPrintf("ISI RPC event:\n");
            TAPP_dbgPrintf("Xdr return value = %d\n",
                    event_ptr->msg.rpc.isiXdr.data[3]);
            break;
        default:
            break;
            TAPP_dbgPrintf("Invalid event type.\n");
            break;
    }
}

/*
 * ======== _TAPP_printAction() ========
 *
 * Private funtion to print content of TAPP_Action
 *
 * Returns:
 *  None.
 */
void _TAPP_printAction(
    TAPP_Action    *action_ptr)
{
    ISIP_Message *isipAct_ptr;

    isipAct_ptr = &action_ptr->msg.isip;
    TAPP_dbgPrintf("*Action type:%s\n", _TAPP_ActionTypeStrings[action_ptr->type]);

    switch (action_ptr->type) {
        case TAPP_ACTION_TYPE_VALIDATE_AT:
        case TAPP_ACTION_TYPE_VALIDATE_GSM_AT:
            TAPP_dbgPrintf("Timeout:%d(ms)\n", action_ptr->u.timeout);
            /* Fall through */
        case TAPP_ACTION_TYPE_ISSUE_AT:
        case TAPP_ACTION_TYPE_ISSUE_GSM_AT:
            TAPP_dbgPrintf("AT command:%s\n", action_ptr->msg.at);
            break;
        case TAPP_ACTION_TYPE_VALIDATE_ISIP:
            TAPP_dbgPrintf("Timeout:%d(ms)\n", action_ptr->u.timeout);
            /* Fall through */
        case TAPP_ACTION_TYPE_ISSUE_ISIP:
            TAPP_dbgPrintf("ISIP Code:%s\n", 
                    _TAPP_IsipCodeTable[isipAct_ptr->code].string);
            TAPP_dbgPrintf("ISIP Protocol:%d\n",
                    isipAct_ptr->protocol);
            if (ISIP_CODE_CALL == isipAct_ptr->code) {
                TAPP_dbgPrintf("ISIP Reason: %d %s\n",
                    isipAct_ptr->msg.call.reason,
                    _TAPP_IsipCallReasonTable[
                    isipAct_ptr->msg.call.reason].string);
                TAPP_dbgPrintf("ISIP Status:%s\n",
                    _TAPP_IsipStatusTable[
                    isipAct_ptr->msg.call.status].string);
                if (!isipAct_ptr->msg.call.from[0]) {
                    TAPP_dbgPrintf("ISIP from:%s\n", 
                          isipAct_ptr->msg.call.from);
                }
                if (!isipAct_ptr->msg.call.to[0]) {
                    TAPP_dbgPrintf("ISIP to:%s\n", 
                          isipAct_ptr->msg.call.to);
                }

                TAPP_dbgPrintf("ISIP Dir:%s\n", 
                      _TAPP_IsipSessionDirTable[
                      isipAct_ptr->msg.call.audioDirection].string);              
                if (ISIP_CALL_REASON_INITIATE ==
                        isipAct_ptr->msg.call.reason) {
                    TAPP_dbgPrintf("ISIP Sesseion CID Type: %s\n",
                            _TAPP_IsipSessionCidTypeTable[
                            isipAct_ptr->msg.call.cidType].string);
                }
            }
            else if (ISIP_CODE_SYSTEM == isipAct_ptr->code) {
                TAPP_dbgPrintf("ISIP Reason:%s\n",
                    _TAPP_IsipSystemReasonTable[
                    isipAct_ptr->msg.system.reason].string);
                TAPP_dbgPrintf("ISIP Status:%s\n",
                    _TAPP_IsipStatusTable[
                    isipAct_ptr->msg.system.status].string);
            }
            else if (ISIP_CODE_SERVICE == isipAct_ptr->code) {
                TAPP_dbgPrintf("ISIP Reason:%s\n",
                    _TAPP_IsipServiceReasonTable[
                    isipAct_ptr->msg.service.reason].string);
                TAPP_dbgPrintf("ISIP Status:%s\n",
                    _TAPP_IsipStatusTable[
                    isipAct_ptr->msg.service.status].string);
                if (ISIP_SERVICE_REASON_SERVER == 
                        isipAct_ptr->msg.service.reason) {
                    TAPP_dbgPrintf("ISIP Service Server:%s\n",
                    _TAPP_IsipServerTypeTable[
                    isipAct_ptr->msg.service.server].string);
                    TAPP_dbgPrintf("ISIP Service Service:%s\n",
                            isipAct_ptr->msg.service.settings.server);
                }
                else if (ISIP_SERVICE_REASON_AUTH== 
                        isipAct_ptr->msg.service.reason) {
                    TAPP_dbgPrintf("ISIP Service AUTH: uname=%s\n",
                            isipAct_ptr->msg.service.settings.credentials.username);
                    TAPP_dbgPrintf("ISIP Service AUTH: password=%s\n",
                            isipAct_ptr->msg.service.settings.credentials.password);
                    TAPP_dbgPrintf("ISIP Service AUTH: realm=%s\n",
                            isipAct_ptr->msg.service.settings.credentials.realm);
                }
                else if (ISIP_SERVICE_REASON_IPSEC ==
                        isipAct_ptr->msg.service.reason) {
                    TAPP_dbgPrintf("ISIP Service IPSEC: protected port=%d\n",
                            isipAct_ptr->msg.service.settings.ipsec.cfg.protectedPort);
                    TAPP_dbgPrintf("ISIP Service IPSEC: protected port pool size=%d\n",
                            isipAct_ptr->msg.service.settings.ipsec.cfg.protectedPortPoolSz);
                    TAPP_dbgPrintf("ISIP Service IPSEC: spi=%d\n",
                            isipAct_ptr->msg.service.settings.ipsec.cfg.spi);
                    TAPP_dbgPrintf("ISIP Service IPSEC: spi pool size=%d\n",
                            isipAct_ptr->msg.service.settings.ipsec.cfg.spiPoolSz);
                }
                else if (ISIP_SERVICE_REASON_BSID ==
                        isipAct_ptr->msg.service.reason) {
                    TAPP_dbgPrintf("cgi-type:%s\n",
                            _TAPP_NetworkAccessTypeTable[
                            isipAct_ptr->msg.service.settings.bsId.type].string);
                    TAPP_dbgPrintf("cgi:%s\n",
                            isipAct_ptr->msg.service.settings.bsId.szBsId);
                }
                else if (ISIP_SERVICE_REASON_DEACTIVATE ==
                        isipAct_ptr->msg.service.reason) {
                    TAPP_dbgPrintf("reasonDesc:%s\n",
                            isipAct_ptr->msg.service.reasonDesc);
                }
            }
            else if (ISIP_CODE_MESSAGE == isipAct_ptr->code) {
                TAPP_dbgPrintf("ISIP Reason: %s\n",
                        _TAPP_IsipTextReasonTable[
                        isipAct_ptr->msg.message.reason].string);
                TAPP_dbgPrintf("ISIP Message Type: %s\n",
                        _TAPP_IsipMessageTypeTable[
                        isipAct_ptr->msg.message.type].string);
                TAPP_dbgPrintf("ISIP message ID: %s\n",
                        isipAct_ptr->msg.message.messageId);
                TAPP_dbgPrintf("ISIP PDU Len: %d\n",
                        isipAct_ptr->msg.message.pduLen);
                TAPP_dbgPrintf("ISIP Message:");
                TAPP_printBytes(
                        (unsigned char *)isipAct_ptr->msg.message.message,
                        isipAct_ptr->msg.message.pduLen);
                TAPP_dbgPrintf("ISIP to: %s\n",
                        isipAct_ptr->msg.message.to);
                TAPP_dbgPrintf("ISIP from: %s\n",
                        isipAct_ptr->msg.message.from);
            }
            else if (ISIP_CODE_USSD == isipAct_ptr->code) {
                TAPP_dbgPrintf("ISIP Reason: %s\n",
                        _TAPP_IsipUssdReasonTable[
                        isipAct_ptr->msg.ussd.reason].string);
                TAPP_dbgPrintf("ISIP message : %s\n",
                        isipAct_ptr->msg.ussd.message);
                TAPP_dbgPrintf("ISIP to: %s\n",
                        isipAct_ptr->msg.ussd.to);
            }
            else if (ISIP_CODE_PRESENCE== isipAct_ptr->code) {
                TAPP_dbgPrintf("ISIP Reason: %s\n",
                        _TAPP_IsipPresReasonTable[
                        isipAct_ptr->msg.presence.reason].string);
                TAPP_dbgPrintf("ISIP presence string : %s\n",
                        isipAct_ptr->msg.presence.presence);
            }           
            break;
        case TAPP_ACTION_TYPE_ISSUE_CSM:         
            if (CSM_EVENT_TYPE_RADIO == action_ptr->msg.csm.type) {
                TAPP_dbgPrintf("CSM Event Type: %s\n", "CSM_EVENT_TYPE_RADIO");
                if (CSM_RADIO_REASON_IP_CHANGE ==
                        action_ptr->msg.csm.evt.radio.reason) {
                    TAPP_dbgPrintf("CSM Reason: %s\n", 
                            "CSM_RADIO_REASON_IP_CHANGE");
                }
                TAPP_dbgPrintf("CSM IP: %s\n",
                        action_ptr->msg.csm.evt.radio.address);
                TAPP_dbgPrintf("CSM Name: %s\n", 
                        action_ptr->msg.csm.evt.radio.infcName);
            }
            else if (CSM_EVENT_TYPE_SERVICE == action_ptr->msg.csm.type) {
                if (CSM_SERVICE_REASON_SET_IMEI_URI ==
                        action_ptr->msg.csm.evt.service.reason) {
                    TAPP_dbgPrintf("CSM Reason: %s\n", 
                            "CSM_SERVICE_REASON_SET_IMEI_URI");
                    TAPP_dbgPrintf("IMEI URI: %s\n", 
                            action_ptr->msg.csm.evt.service.u.imeiUri);
                }
                if (CSM_SERVICE_REASON_SET_IMPU ==
                        action_ptr->msg.csm.evt.service.reason) {
                    TAPP_dbgPrintf("CSM Reason: %s\n",
                            "CSM_SERVICE_REASON_SET_IMPU");
                    TAPP_dbgPrintf("ISIM impu: %s\n",
                            action_ptr->msg.csm.evt.service.u.impu);
                }
                if (CSM_SERVICE_REASON_SET_IMPI ==
                        action_ptr->msg.csm.evt.service.reason) {
                    TAPP_dbgPrintf("CSM Reason: %s\n",
                            "CSM_SERVICE_REASON_SET_IMPI");
                    TAPP_dbgPrintf("ISIM impi: %s\n",
                            action_ptr->msg.csm.evt.service.u.impi);
                }
                if (CSM_SERVICE_REASON_SET_DOMAIN ==
                        action_ptr->msg.csm.evt.service.reason) {
                    TAPP_dbgPrintf("CSM Reason: %s\n",
                            "CSM_SERVICE_REASON_SET_DOMAIN");
                    TAPP_dbgPrintf("ISIM domain: %s\n",
                            action_ptr->msg.csm.evt.service.u.domain);
                }
                if (CSM_SERVICE_REASON_SET_PASSWORD ==
                        action_ptr->msg.csm.evt.service.reason) {
                    TAPP_dbgPrintf("CSM Reason: %s\n",
                            "CSM_SERVICE_REASON_SET_PASSWORD");
                    TAPP_dbgPrintf("ISIM password: %s\n",
                            action_ptr->msg.csm.evt.service.u.password);
                }
                if (CSM_SERVICE_REASON_SET_PCSCF ==
                        action_ptr->msg.csm.evt.service.reason) {
                    TAPP_dbgPrintf("CSM Reason: %s\n",
                            "CSM_SERVICE_REASON_SET_PCSCF");
                    TAPP_dbgPrintf("ISIM pcscf: %s\n",
                            action_ptr->msg.csm.evt.service.u.pcscf);
                }
                if (CSM_SERVICE_REASON_IMS_ENABLE ==
                        action_ptr->msg.csm.evt.service.reason) {
                    TAPP_dbgPrintf("CSM Reason: %s\n", 
                            "CSM_SERVICE_REASON_IMS_ENABLE");
                }
                if (CSM_SERVICE_REASON_IMS_DISABLE ==
                        action_ptr->msg.csm.evt.service.reason) {
                    TAPP_dbgPrintf("CSM Reason: %s\n", 
                            "CSM_SERVICE_REASON_IMS_DISABLE");
                }
            }
            break;
        case TAPP_ACTION_TYPE_PAUSE:
            TAPP_dbgPrintf("Pause %d(ms)\n", action_ptr->u.pause);
            break;
        case TAPP_ACTION_TYPE_CLEAN_ISIP:
            TAPP_dbgPrintf("Clean ISIP\n");
            break;
        case TAPP_ACTION_TYPE_VALIDATE_XCAP:
            TAPP_dbgPrintf("Validate XCAP Command\n");
            break;
        case TAPP_ACTION_TYPE_ISSUE_XCAP:
            TAPP_dbgPrintf("Issue XCAP event\n");
            break;
        case TAPP_ACTION_TYPE_VALIDATE_ISI_RPC_RETURN:
        case TAPP_ACTION_TYPE_VALIDATE_ISI_GET_EVT:
            TAPP_dbgPrintf("Validate ISI RPC Command\n");
            TAPP_dbgPrintf("Func type %s\n", _TAPP_RpcIsiFuncTypeTable[
                    action_ptr->msg.rpcMsg.funcType].string);
            TAPP_dbgPrintf("Xdr data %s\n", 
                    action_ptr->msg.rpcMsg.isiXdr.data);
            break;
        case TAPP_ACTION_TYPE_ISSUE_ISI_RPC:
            TAPP_dbgPrintf("Issue ISI RPC event\n");
            TAPP_dbgPrintf("Func type %s\n", _TAPP_RpcIsiFuncTypeTable[
                    action_ptr->msg.rpcMsg.funcType].string);
            TAPP_dbgPrintf("Xdr data %s\n", action_ptr->msg.rpcMsg.isiXdr.data);
            break;
        case TAPP_ACTION_TYPE_CLEAN_ISI_EVT:
            TAPP_dbgPrintf("Clean ISI Evt\n");
            break;
        default:
            TAPP_dbgPrintf("Invalid action type.\n");
            break;
    }
}
 

/*
 * THIS IS AN UNPUBLISHED WORK CONVETINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIEVETRY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2013 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 *
 */

#include "osal.h"
#include "csm_event.h"
#include "_csm_ut_utils.h"

#include <pdu_hlpr.h>
#include "jsmn.h"


vint CSM_UT_supsrvRun = 1;

#define _CSM_UT_SUPSRV_BUFFER_SIZE 8192
#define _CSM_UT_SUPSRV_JSON_TOKENS 1024

/*
 * to model two json string inputs, csm inputEvent and outputEvent
 * e.g.
 * {inputSupsrvEvent:{
    eventDesc:OIPDisable,reason:0,reasonDesc:"",mode:0,status:0,ruleParams:{
        noReplyTimer:0,mediaType:PCMA,cfwNumber:+8618616563563,addrType:145}}},
 *
 * {outputSupsrvEvent:{
    eventDesc:OIPQueryResult,reason:15,reasonDesc:"",cmdType:2,mode:0,
        queryEnb:0,prov:3,errorCode:0,ruleParams:{
        noReplyTimer:0,mediaType:PCMA,cfwNumber:+8618616563563,addrType:145}}}
 *
 * noted that we used jsmn relaxed parsing mode to save the quote" around the
 * string
 *
 * jsmn parsed the json string into depth-first tree of tokens. we mapped the
 * subtree in an object as sub-structure of the enclosing structure, such as
 * ruleParams in CSM_InputSupSrv struct.
 */

enum {
    _CSM_UT_SUPSRV_INPUT_TYPE=0,
    _CSM_UT_SUPSRV_INPUT_EVENT_DESC,
    _CSM_UT_SUPSRV_INPUT_REASON,
    _CSM_UT_SUPSRV_INPUT_REASON_DESC,
    _CSM_UT_SUPSRV_INPUT_MODE,
    _CSM_UT_SUPSRV_INPUT_STATUS,
    _CSM_UT_SUPSRV_INPUT_RULE_PARAMS,
    _CSM_UT_SUPSRV_INPUT_NOREPLYTIMER,
    _CSM_UT_SUPSRV_INPUT_MEDIA_TYPE,
    _CSM_UT_SUPSRV_INPUT_CFW_NUMBER,
    _CSM_UT_SUPSRV_INPUT_ADDR_TYPE,
    _CSM_UT_SUPSRV_INPUT_LAST_ITEM
};

enum {
    _CSM_UT_SUPSRV_OUTPUT_TYPE=0,
    _CSM_UT_SUPSRV_OUTPUT_EVENT_DESC,
    _CSM_UT_SUPSRV_OUTPUT_REASON,
    _CSM_UT_SUPSRV_OUTPUT_REASON_DESC,
    _CSM_UT_SUPSRV_OUTPUT_CMD_TYPE,
    _CSM_UT_SUPSRV_OUTPUT_MODE,
    _CSM_UT_SUPSRV_OUTPUT_QUERYENB,
    _CSM_UT_SUPSRV_OUTPUT_PROV,
    _CSM_UT_SUPSRV_OUTPUT_ERROR_CODE,
    _CSM_UT_SUPSRV_OUTPUT_RULE_PARAMS,
    _CSM_UT_SUPSRV_OUTPUT_NOREPLYTIMER,
    _CSM_UT_SUPSRV_OUTPUT_MEDIA_TYPE,
    _CSM_UT_SUPSRV_OUTPUT_CFW_URI,
    _CSM_UT_SUPSRV_OUTPUT_ADDR_TYPE,
    _CSM_UT_SUPSRV_OUTPUT_LAST_ITEM
};

typedef struct {
    int         keyIndex;
    char       *keyName;
    jsmntype_t  valueType;
} _CSM_UT_SupsrvJsonItem;

typedef union {
    CSM_InputEvent inputEvent;
    CSM_OutputEvent outputEvent;
} _CSM_UT_SupsrvEvent;


_CSM_UT_SupsrvJsonItem _CSM_UT_supsrvJsonInputEventItems[] = {
        {_CSM_UT_SUPSRV_INPUT_TYPE,         "inputSupsrvEvent", JSMN_STRING},
        {_CSM_UT_SUPSRV_INPUT_EVENT_DESC,   "eventDesc",        JSMN_STRING},
        {_CSM_UT_SUPSRV_INPUT_REASON,       "reason",           JSMN_PRIMITIVE},
        {_CSM_UT_SUPSRV_INPUT_REASON_DESC,  "reasonDesc",       JSMN_STRING},
        {_CSM_UT_SUPSRV_INPUT_MODE,         "mode",             JSMN_PRIMITIVE},
        {_CSM_UT_SUPSRV_INPUT_STATUS,       "status",           JSMN_PRIMITIVE},
        {_CSM_UT_SUPSRV_INPUT_RULE_PARAMS,  "ruleParams",       JSMN_OBJECT},
        {_CSM_UT_SUPSRV_INPUT_NOREPLYTIMER, "noReplyTimer",     JSMN_PRIMITIVE},
        {_CSM_UT_SUPSRV_INPUT_MEDIA_TYPE,   "mediaType",        JSMN_STRING},
        {_CSM_UT_SUPSRV_INPUT_CFW_NUMBER,      "cfwNumber",           JSMN_STRING},
        {_CSM_UT_SUPSRV_INPUT_ADDR_TYPE,    "addrType",         JSMN_PRIMITIVE},
};

_CSM_UT_SupsrvJsonItem _CSM_UT_supsrvJsonOutputEventItems[] = {
        {_CSM_UT_SUPSRV_OUTPUT_TYPE,         "inputSupsrvEvent", JSMN_STRING},
        {_CSM_UT_SUPSRV_OUTPUT_EVENT_DESC,   "eventDesc",        JSMN_STRING},
        {_CSM_UT_SUPSRV_OUTPUT_REASON,       "reason",           JSMN_PRIMITIVE},
        {_CSM_UT_SUPSRV_OUTPUT_REASON_DESC,  "reasonDesc",       JSMN_STRING},
        {_CSM_UT_SUPSRV_OUTPUT_CMD_TYPE,     "cmdType",          JSMN_PRIMITIVE},
        {_CSM_UT_SUPSRV_OUTPUT_MODE,         "mode",             JSMN_PRIMITIVE},
        {_CSM_UT_SUPSRV_OUTPUT_QUERYENB,     "status",           JSMN_PRIMITIVE},
        {_CSM_UT_SUPSRV_OUTPUT_PROV,         "prov",             JSMN_PRIMITIVE},
        {_CSM_UT_SUPSRV_OUTPUT_ERROR_CODE,   "errorCode",        JSMN_PRIMITIVE},
        {_CSM_UT_SUPSRV_OUTPUT_RULE_PARAMS,  "ruleParams",       JSMN_OBJECT},
        {_CSM_UT_SUPSRV_OUTPUT_NOREPLYTIMER, "noReplyTimer",     JSMN_PRIMITIVE},
        {_CSM_UT_SUPSRV_OUTPUT_MEDIA_TYPE,   "mediaType",        JSMN_STRING},
        {_CSM_UT_SUPSRV_OUTPUT_CFW_URI,      "cfwNumber",           JSMN_STRING},
        {_CSM_UT_SUPSRV_OUTPUT_ADDR_TYPE,    "addrType",         JSMN_PRIMITIVE},
};

UT_Return CSM_UT_supsrvQuit(
    int arg0)
{
    CSM_UT_supsrvRun = 0;
    return UT_PASS;
}

/*
 * ======== _CSM_UT_supsrvExecInputToken() ========
 *
 * function to convert the json string to 
 *
 * Returns:
 *  0 : failed
 *  n : number of tokens consummed to complete the token parsing
 */
int _CSM_UT_supsrvExecInputToken(char *json_ptr, jsmntok_t *actionToken_ptr)
{
    int endOffset   = actionToken_ptr[0].end;
    int curTokenIdx = 3; /* idx 0 is obj, idx 1 is inputType */
    int curItemIdx  = 1; /* idx 0 is inputType */
    CSM_InputEvent  inputEvent = {0};
    CSM_InputSupSrv *supsrvInputEvt_ptr;

    supsrvInputEvt_ptr = &inputEvent.evt.supSrv;
    inputEvent.type = CSM_EVENT_TYPE_SUPSRV;

    while ( (actionToken_ptr[curTokenIdx].start < endOffset) &&
            (_CSM_UT_SUPSRV_INPUT_LAST_ITEM > curItemIdx)) {
        if (0 != OSAL_strncmp(&json_ptr[actionToken_ptr[curTokenIdx].start],
                _CSM_UT_supsrvJsonInputEventItems[curItemIdx].keyName,
                actionToken_ptr[curTokenIdx].end - actionToken_ptr[curTokenIdx].start)) {
            OSAL_logMsg("Supsrv Parsing inputevent JSON failed\n");
            return 0;
        }
        switch (curItemIdx) {
            case _CSM_UT_SUPSRV_INPUT_EVENT_DESC:
                /* ignore it, as json comments */
                curTokenIdx += 2;
                curItemIdx += 1;
                break;
            case _CSM_UT_SUPSRV_INPUT_REASON:
                supsrvInputEvt_ptr->reason = OSAL_atoi(
                        &json_ptr[actionToken_ptr[curTokenIdx+1].start]);
                curTokenIdx += 2;
                curItemIdx += 1;
                break;
            case _CSM_UT_SUPSRV_INPUT_REASON_DESC:
                OSAL_strncpy(supsrvInputEvt_ptr->reasonDesc,
                        &json_ptr[actionToken_ptr[curTokenIdx+1].start],
                        actionToken_ptr[curTokenIdx+1].end -
                            actionToken_ptr[curTokenIdx+1].start);
                curTokenIdx += 2;
                curItemIdx += 1;
                break;
            case _CSM_UT_SUPSRV_INPUT_MODE:
                supsrvInputEvt_ptr->mode.cbMode = (CSM_SupSrvCbMode)OSAL_atoi(
                        &json_ptr[actionToken_ptr[curTokenIdx+1].start]);
                curTokenIdx += 2;
                curItemIdx += 1;
                break;
            case _CSM_UT_SUPSRV_INPUT_STATUS:
                supsrvInputEvt_ptr->status.genReqStatus = OSAL_atoi(
                        &json_ptr[actionToken_ptr[curTokenIdx+1].start]);
                curTokenIdx += 2;
                curItemIdx += 1;
                break;
            case _CSM_UT_SUPSRV_INPUT_RULE_PARAMS:
                /*
                 * value is the object for this sub-structure
                 * we will start parsing the sub-structure in next item
                 */
                curTokenIdx += 2;
                curItemIdx += 1;
                break;
            case _CSM_UT_SUPSRV_INPUT_NOREPLYTIMER:
                supsrvInputEvt_ptr->ruleParams.noReplyTimer = OSAL_atoi(
                        &json_ptr[actionToken_ptr[curTokenIdx+1].start]);
                curTokenIdx += 2;
                curItemIdx += 1;
                break;
            case _CSM_UT_SUPSRV_INPUT_MEDIA_TYPE:
                OSAL_strncpy(supsrvInputEvt_ptr->ruleParams.mediaType,
                        &json_ptr[actionToken_ptr[curTokenIdx+1].start],
                        actionToken_ptr[curTokenIdx+1].end -
                            actionToken_ptr[curTokenIdx+1].start);
                curTokenIdx += 2;
                curItemIdx += 1;
                break;
            case _CSM_UT_SUPSRV_INPUT_CFW_NUMBER:
                OSAL_strncpy(supsrvInputEvt_ptr->ruleParams.cfwNumber,
                        &json_ptr[actionToken_ptr[curTokenIdx+1].start],
                        actionToken_ptr[curTokenIdx+1].end -
                            actionToken_ptr[curTokenIdx+1].start);
                curTokenIdx += 2;
                curItemIdx += 1;
                break;
            case _CSM_UT_SUPSRV_INPUT_ADDR_TYPE:
                supsrvInputEvt_ptr->ruleParams.noReplyTimer = OSAL_atoi(
                        &json_ptr[actionToken_ptr[curTokenIdx+1].start]);
                curTokenIdx += 2;
                curItemIdx += 1;
                break;
            default:
                OSAL_logMsg("Supsrv Parsing inputevent JSON  extra key\n");
                curTokenIdx += 2;
                break;
        }
    }

    CSM_UT_writeCsmEvent(&inputEvent);
    return curTokenIdx;
}

/*
 * ======== _CSM_UT_supsrvExecOutputToken() ========
 *
 * function to convert the json string to 
 *
 * Returns:
 *  0 : failed, i.e no token consumed
 *  n : number of tokens consummed to complete the token parsing
 */
int _CSM_UT_supsrvExecOutputToken(char *json_ptr, jsmntok_t *actionToken_ptr)
{
    // CSM_OutputEvent outputEvent = {0};
    // xxx auto compare output result
    return 0;
}

/*
 * ======== CSM_UT_supsrvExecToken() ========
 *
 * function to convert the json string to 
 *
 * Returns:
 *  0 : failed, i.e no token consumed
 *  n : number of tokens consummed to complete the token parsing
 */
int CSM_UT_supsrvExecToken(char *json_ptr, jsmntok_t *actionToken_ptr)
{
    if (0 == OSAL_strncmp(&json_ptr[actionToken_ptr[1].start],
            _CSM_UT_supsrvJsonInputEventItems[_CSM_UT_SUPSRV_INPUT_TYPE].keyName,
            actionToken_ptr[1].end - actionToken_ptr[1].start)) {
        return _CSM_UT_supsrvExecInputToken(json_ptr, actionToken_ptr);
    }
    else if (0 == OSAL_strncmp(&json_ptr[actionToken_ptr[1].start],
            _CSM_UT_supsrvJsonOutputEventItems[_CSM_UT_SUPSRV_OUTPUT_TYPE].keyName,
            actionToken_ptr[1].end - actionToken_ptr[1].start)) {
        return _CSM_UT_supsrvExecOutputToken(json_ptr, actionToken_ptr);
    }
    else {
        return 0;
    }
}

/*
 * ======== CSM_UT_supsrvJson() ========
 *
 * function to read/parse json string and execut it
 *
 * Returns:
 *  UT_PASS: json string read and parsed
 *  UT_FAIL: failed to exec json string
 */
UT_Return CSM_UT_supsrvJson ()
{
    unsigned int n;
    jsmnerr_t    ret;
    jsmntok_t   *tokens;
    char        *jsonBuf;
    jsmntok_t   *tok_ptr;
    size_t       i,j,k;

    // xxx
    jsmn_parser parser;
    jsmn_init(&parser);
    n = _CSM_UT_SUPSRV_JSON_TOKENS;
    tokens = OSAL_memAlloc(sizeof(jsmntok_t) * n, 0);
    jsonBuf = OSAL_memAlloc(_CSM_UT_SUPSRV_BUFFER_SIZE, 0);

    OSAL_logMsg("\nSupsrv Testing JSON : ");
    CSM_UT_getBuffer(jsonBuf, _CSM_UT_SUPSRV_BUFFER_SIZE);

    ret = jsmn_parse(&parser, jsonBuf, tokens, n);
    if (ret == JSMN_ERROR_INVAL) {
        OSAL_logMsg("supsrvJson: invalid JSON string");
        return UT_FAIL;
    }
    if (ret == JSMN_ERROR_PART) {
        OSAL_logMsg("supsrvJson: truncated JSON string");
        return UT_FAIL;
    }
    
    tok_ptr = &tokens[0];
    // Should never reach uninitialized tokens
    // log_assert(t->start != -1 && t->end != -1);
    if (JSMN_OBJECT == tok_ptr->type) {
        /* single action json command string */
        return CSM_UT_supsrvExecToken(jsonBuf, tok_ptr);
    }
    else if (JSMN_ARRAY == tok_ptr->type) {
        i = 1;
        for (k=0;  k<=tok_ptr->size; k++){
            OSAL_logMsg("supsrvJson: exec-ing token sequence=%d\n", k);
            j = CSM_UT_supsrvExecToken(jsonBuf, &tokens[i]);
            if (0 == j) {
                OSAL_logMsg("supsrvJson: exec token idx=%d failed\n", i);
                return UT_FAIL;
            }
            i += j;
        }
        OSAL_logMsg("supsrvJson: doen all token sequences\n");
    } else {
        OSAL_logMsg("supsrvJson: illegal top level token");
        return UT_FAIL;
    }

    return UT_PASS;
}

CSM_UT_TestTableItem CSM_UT_supsrvTable[] =
{
    { "json", CSM_UT_supsrvJson, "supsrv ut using json cmd str" },
    { "q",   CSM_UT_supsrvQuit, "Quit (exit call menu"}
};



UT_Return CSM_UT_supsrv (
)
{
    CSM_UT_supsrvRun = 1;
    vint              item;
    vint              itemMax;
    CSM_UT_TestTableItem *item_ptr;
    CSM_UT_TestFunc      *func_ptr;
    int32             arg;
    char              buf[10];
    vint              printMenu;

    printMenu = 1;
    OSAL_taskDelay(500);    /* cleans up printing */
    itemMax = sizeof(CSM_UT_supsrvTable)/sizeof(CSM_UT_supsrvTable[0]);
    while (CSM_UT_supsrvRun) {
        if (printMenu > 0) {
            printMenu = 0;
            OSAL_logMsg("\n"
                    "=====================\n"
                    "D2 CSM supsrv test suite\n"
                    "=====================\n"
                    "Command  Description\n"
                    "-------  -----------\n");

            for (item = 0; item < itemMax; item++) {
                item_ptr = &CSM_UT_supsrvTable[item];
                OSAL_logMsg("%-9s%s\n", item_ptr->cmd, item_ptr->desc);
            }
        }

        OSAL_logMsg("\nCmd: ");
        _CSM_UT_getLine(buf, 2 * sizeof(CSM_UT_supsrvTable[0].cmd));
        for (item = 0, func_ptr = NULL; item < itemMax; item++) {
            item_ptr = &CSM_UT_supsrvTable[item];
            if (0 == OSAL_strncmp(buf, (char *)item_ptr->cmd, sizeof(buf))) {
                func_ptr = item_ptr->func_ptr;
                break;
            }
        }

        if (NULL != func_ptr) {
            OSAL_logMsg("\n\n\n"
                    "==============================================\n"
                    "%s\n"
                    "==============================================\n\n\n",
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
    OSAL_logMsg("\n%s:%d  UT_menu return\n\n", __FILE__, __LINE__);

    return UT_PASS;
}

/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2014 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 *
 */
#include <ezxml.h>
#include <_supsrv.h>
#include <supsrv.h>
#include "xcap_helper.h"
#include "xcap_resources.h"

static SUPSRV_IntResponse SUPSRV_RESPONSE_REASON[] = {
    {SUPSRV_EVENT_REASON_AT_CMD_OIR, "originating-identity-presentation-restriction"},
    {SUPSRV_EVENT_REASON_AT_CMD_OIP, "originating-identity-presentation"},
    {SUPSRV_EVENT_REASON_AT_CMD_TIP, "terminating-identity-presentation"},
    {SUPSRV_EVENT_REASON_AT_CMD_TIR, "terminating-identity-presentation-restriction"},
    {SUPSRV_EVENT_REASON_AT_CMD_CW,  "communication-waiting",},
    {SUPSRV_EVENT_REASON_AT_CMD_CF,  "communication-diversion"},
    {SUPSRV_EVENT_REASON_AT_CMD_CB,  "communication-barring"}, 
};

/*
 * ======== _SUPSRV_parseQueryResult()======== 
 * This function is to parse if a supplementary service is 
 * active or not from the xml query result.
 * tar_ptr is the tag of the supplementary service and this function
 * will see if the "active" attribute is set to "true" or "false".
 *
 * Returns:
 *  0: Active
 *  1: Deactive.
 *  -1: Error
 */
vint _SUPSRV_parseQueryResult(
    char *doc_ptr,
    vint  docLen,
    char *tag_ptr)
{
    ezxml_t xml_ptr;
    ezxml_t child_xml;
    char   *value_ptr;
    vint    ret;

    ret = -1;
    xml_ptr = NULL;

    if (NULL != doc_ptr) {
        xml_ptr = ezxml_parse_str(doc_ptr,
                docLen);
    }
    if (NULL == xml_ptr) {
        return (-1);
    }

    /* Get tag */
    child_xml = ezxml_child(xml_ptr, tag_ptr);
    if (NULL != child_xml) {
        value_ptr = (char *)ezxml_attr(child_xml, SUPSRV_XCAP_ATCIVE);
        if (NULL != value_ptr) {
            if (0 == OSAL_strncmp(value_ptr, SUPSRV_XCAP_TRUE, 4)) {
                ret = 1; /* Active */
            }
            else {
                ret = 0; /* Inactive */
            }
        }
    }

    ezxml_free(xml_ptr);
    return (ret);
}


/* 
 * ======== _SUPSRV_getCmdCnt() ========
 * This function returns a unique value is used to identify XCAP transactions.
 *
 * Returns:
 *  a unique value.
 */
uint32 _SUPSRV_getCmdCnt(
    SUPSRV_XcapObj   *gxcap_ptr)
{
    /* 
     * Note, there no roll over handling here but I don't think a device will 
     * ever make 2^32 XCAP transactions.
     */
    gxcap_ptr->cmdCnt++;
    _SUPSRV_dbgPrintf("%s: Global cmdCnt is :%d\n", __FUNCTION__, 
            gxcap_ptr->cmdCnt);
    return (gxcap_ptr->cmdCnt);
}


/* 
 * ======== _SUPSRV_allocXcapTrans() ========
 * This function retrieves an available object used to manage XCAP transactions.
 *
 * Returns:
 *  The next available XCAP transaction resource in an array of resources.
 */
SUPSRV_XcapTrans* _SUPSRV_allocXcapTrans(
    SUPSRV_XcapObj *xcapObj_ptr)
{
    vint index;
    
    for (index = 0 ; index < SUPSRV_MAX_TRANSACTIONS ; index++) {
        if (0 == xcapObj_ptr->trans[index].cmdCnt) {
            /* Then we found an available one */
            return (&xcapObj_ptr->trans[index]);
        }
    }
    return (NULL);
}


/*  ======== _SUPSRV_updateService()======== 
 *  This function make uri for CB, CD and CB and send command to XCAP
 *  Return: SUPSRV_OK
 *             SUPSRV_ERR
 */
vint _SUPSRV_updateService(
    SUPSRV_XcapObj  *xcap_ptr,
    char            *doc_ptr,
    vint             cmdType)
{
    XCAP_Cmd           cmd;
    SUPSRV_XcapTrans  *trans_ptr;
    char              *file_ptr = 0;

    /* Get some scratch to construct the XCAP uri stuff */
    if (NULL == (trans_ptr = _SUPSRV_allocXcapTrans(xcap_ptr))) {
        /* No available resources */
        _SUPSRV_dbgPrintf("%s %d\n", __FUNCTION__, __LINE__);
        return (SUPSRV_ERR);
    }
    switch (cmdType) {
        case SUPSRV_CMD_CD_OPERATION:
            file_ptr = XCAP_SIMSERVS_CD_DOC;
            break;
        case SUPSRV_CMD_CBIC_OPERATION:
        case SUPSRV_CMD_CBICR_OPERATION:
            file_ptr = XCAP_SIMSERVS_CBIC_DOC;
            break;
        case SUPSRV_CMD_CBOG_OPERATION:
        case SUPSRV_CMD_CBOIC_OPERATION:
            file_ptr = XCAP_SIMSERVS_CBOG_DOC;
            break;
        case SUPSRV_CMD_CW_OPERATION:
            file_ptr = XCAP_SIMSERVS_CW_DOC;
            break;    
        default:
            break;
    }

    /* Make a URI to get the RLS service doc. */
    if (0 == XCAP_helperMakeUri(
            xcap_ptr->username,    /* Username */
            xcap_ptr->password,    /* Password */
            xcap_ptr->server,      /* root */
            XCAP_SIMSERVS_AUID,    /* auid */
            XCAP_USERS,            /* folder */
            xcap_ptr->uri,         /* uri */
            file_ptr,              /* doc */
            NULL,                  /* node */
            trans_ptr->scratch[0], /* dst */
            SUPSRV_SCRATCH_SZ - 1)) {
        /* Then we failed to create the request */
        _SUPSRV_dbgPrintf("%s %d\n", __FUNCTION__, __LINE__);
        return (SUPSRV_ERR);
    }

    /* Construct doc */
    OSAL_strncpy(trans_ptr->scratch[1], doc_ptr, SUPSRV_SCRATCH_SZ);
    _SUPSRV_dbgPrintf("uri:%s\n", trans_ptr->scratch[0]);
    _SUPSRV_dbgPrintf("doc:%s\n", trans_ptr->scratch[1]);
    /* Setup a command */
    OSAL_memSet(&cmd, 0, sizeof(cmd));
    cmd.op = XCAP_OPERATION_CREATE_REPLACE;
    cmd.opType = XCAP_OPERATION_TYPE_DOCUMENT;
    cmd.uri_ptr = trans_ptr->scratch[0];
    cmd.auid_ptr = XCAP_SIMSERVS_AUID;
    cmd.username_ptr = xcap_ptr->username;
    cmd.password_ptr = xcap_ptr->password;
    cmd.userAgent_ptr = "vport";
    cmd.src_ptr = trans_ptr->scratch[1];
    cmd.srcSz = OSAL_strlen(trans_ptr->scratch[1]);
    cmd.infcAddr_ptr = &(xcap_ptr->infcAddress);
    cmd.x3gpp_ptr = xcap_ptr->impu;

    /* Set the command type */
    trans_ptr->cmdType = cmdType;

    /* Now send it */
    if (1 != XCAP_sendCmd(&(xcap_ptr->xcap), &cmd)) {
        /* Failed to send request */
        _SUPSRV_dbgPrintf("%s: FAILED to send XCAP command\n", __FUNCTION__);
        return (SUPSRV_ERR);
    }
    /*
     * Get the XCAP command count and copy it to the service object.
     * This is used to match XCAP commands to XCAP events.
     */
    trans_ptr->cmdCnt = _SUPSRV_getCmdCnt(xcap_ptr);
    return (SUPSRV_OK);
}


/*
 * ======== _SUPSRV_updateOip()========
 *  This function make uri for OIP and send command to XCAP
 *  Return:    SUPSRV_OK
 *  SUPSRV_ERR
 */
vint _SUPSRV_updateOip(
    SUPSRV_XcapObj  *xcap_ptr,
    vint             activate)
{
    XCAP_Cmd          cmd;
    SUPSRV_XcapTrans *trans_ptr;

    /* Get some scratch to construct the XCAP uri stuff */
    if (NULL == (trans_ptr = _SUPSRV_allocXcapTrans(xcap_ptr))) {
        /* No available resources */
        _SUPSRV_dbgPrintf("%s %d\n", __FUNCTION__, __LINE__);
        return (SUPSRV_ERR);
    }

    /* Make a URI to get the RLS service doc. */
    if (0 == XCAP_helperMakeUri(
            xcap_ptr->username,   /* Username */
            xcap_ptr->password,   /* Password */
            xcap_ptr->server,     /* root */
            XCAP_SIMSERVS_AUID,    /* auid */
            XCAP_USERS,            /* folder */
            xcap_ptr->uri,         /* uri */
            XCAP_SIMSERVS_OIR_DOC, /* doc */
            NULL,                  /* node */
            trans_ptr->scratch[0], /* dst */
            SUPSRV_SCRATCH_SZ - 1)) {
        /* Then we failed to create the request */
        _SUPSRV_dbgPrintf("%s %d\n", __FUNCTION__, __LINE__);
        return (SUPSRV_ERR);
    }

    /* Construct doc */
    if (0 != activate) {
        OSAL_snprintf(trans_ptr->scratch[1], SUPSRV_SCRATCH_SZ,
                        SUPSRV_XCAP_OIP_DOC, "true");
    }
    else {
        /* deactivate */
        OSAL_snprintf(trans_ptr->scratch[1], SUPSRV_SCRATCH_SZ,
                        SUPSRV_XCAP_OIP_DOC, "false");
    }
    _SUPSRV_dbgPrintf("uri:%s\n", trans_ptr->scratch[0]);
    _SUPSRV_dbgPrintf("doc:%s\n", trans_ptr->scratch[1]);
    /* Setup a command */
    OSAL_memSet(&cmd, 0, sizeof(cmd));
    cmd.op = XCAP_OPERATION_CREATE_REPLACE;
    cmd.opType = XCAP_OPERATION_TYPE_DOCUMENT;
    cmd.uri_ptr = trans_ptr->scratch[0];
    cmd.auid_ptr = XCAP_SIMSERVS_AUID;
    cmd.username_ptr = xcap_ptr->username;
    cmd.password_ptr =xcap_ptr->password;
    cmd.userAgent_ptr = "vport";
    cmd.src_ptr = trans_ptr->scratch[1];
    cmd.srcSz = OSAL_strlen(trans_ptr->scratch[1]);
    cmd.infcAddr_ptr = &(xcap_ptr->infcAddress);
    cmd.x3gpp_ptr = xcap_ptr->impu;

    /* Set the command type */
    trans_ptr->cmdType = SUPSRV_CMD_OIP_OPERATION;

    /* Now send it */
    if (1 != XCAP_sendCmd(&(xcap_ptr->xcap), &cmd)) {
        /* Failed to send request */
        _SUPSRV_dbgPrintf("%s: FAILED to send XCAP command\n", __FUNCTION__);
        return (SUPSRV_ERR);
    }
    /*
     * Get the XCAP command count and copy it to the service object.
     * This is used to match XCAP commands to XCAP events.
     */
    trans_ptr->cmdCnt = _SUPSRV_getCmdCnt(xcap_ptr);
    return (SUPSRV_OK);
}


/*
 * ======== _SUPSRV_updateOir()========
 *  This function make uri for ORI and send command to XCAP
 *  Return:    SUPSRV_OK
 *  SUPSRV_ERR
 */
vint _SUPSRV_updateOir(
    SUPSRV_XcapObj  *xcap_ptr,
    vint             activate)
{
    XCAP_Cmd           cmd;
    SUPSRV_XcapTrans  *trans_ptr;

    /* Get some scratch to construct the XCAP uri stuff */
    if (NULL == (trans_ptr = _SUPSRV_allocXcapTrans(xcap_ptr))) {
        /* No available resources */
        _SUPSRV_dbgPrintf("%s %d\n", __FUNCTION__, __LINE__);
        return (SUPSRV_ERR);
    }

    /* Make a URI to get the RLS service doc. */
    if (0 == XCAP_helperMakeUri(
            xcap_ptr->username,   /* Username */
            xcap_ptr->password,   /* Password */
            xcap_ptr->server,     /* root */
            XCAP_SIMSERVS_AUID,    /* auid */
            XCAP_USERS,            /* folder */
            xcap_ptr->uri,         /* uri */
            XCAP_SIMSERVS_OIR_DOC,     /* doc */
            NULL,                  /* node */
            trans_ptr->scratch[0], /* dst */
            SUPSRV_SCRATCH_SZ - 1)) {
        /* Then we failed to create the request */
        _SUPSRV_dbgPrintf("%s %d\n", __FUNCTION__, __LINE__);
        return (SUPSRV_ERR);
    }

    /* Construct doc */
    if (0 != activate) {
        OSAL_snprintf(trans_ptr->scratch[1], SUPSRV_SCRATCH_SZ,
                        SUPSRV_XCAP_OIR_DOC, "true");
    }
    else {
        /* deactivate */
        OSAL_snprintf(trans_ptr->scratch[1], SUPSRV_SCRATCH_SZ,
                        SUPSRV_XCAP_OIR_DOC, "false");
    }
    _SUPSRV_dbgPrintf("uri:%s\n", trans_ptr->scratch[0]);
    _SUPSRV_dbgPrintf("doc:%s\n", trans_ptr->scratch[1]);
    /* Setup a command */
    OSAL_memSet(&cmd, 0, sizeof(cmd));
    cmd.op = XCAP_OPERATION_CREATE_REPLACE;
    cmd.opType = XCAP_OPERATION_TYPE_DOCUMENT;
    cmd.uri_ptr = trans_ptr->scratch[0];
    cmd.auid_ptr = XCAP_SIMSERVS_AUID;
    cmd.username_ptr = xcap_ptr->username;
    cmd.password_ptr =xcap_ptr->password;
    cmd.userAgent_ptr = "vport";
    cmd.src_ptr = trans_ptr->scratch[1];
    cmd.srcSz = OSAL_strlen(trans_ptr->scratch[1]);
    cmd.infcAddr_ptr = &(xcap_ptr->infcAddress);
    cmd.x3gpp_ptr = xcap_ptr->impu;

    /* Set the command type */
    trans_ptr->cmdType = SUPSRV_CMD_OIR_OPERATION;

    /* Now send it */
    if (1 != XCAP_sendCmd(&(xcap_ptr->xcap), &cmd)) {
        /* Failed to send request */
        _SUPSRV_dbgPrintf("%s: FAILED to send XCAP command\n", __FUNCTION__);
        return (SUPSRV_ERR);
    }
    /*
     * Get the XCAP command count and copy it to the service object.
     * This is used to match XCAP commands to XCAP events.
     */
    trans_ptr->cmdCnt = _SUPSRV_getCmdCnt(xcap_ptr);
    return (SUPSRV_OK);
}


/* 
 * ======== _SUPSRV_fetchDocument() ========
 * This function will initiate an XCAP transaction to retrieve a document 
 * residing on an XCAP server.
  *
 * Returns:
 *  SUPSRV_OK:  The XCAP transaction was successfully initiated. 
 *  SUPSRV_ERR: The transaction failed to initiate.  Either the request could not
 *            be constructed or it failed to send.
 */
vint _SUPSRV_fetchDocument(
    SUPSRV_XcapObj  *xcap_ptr,
    SUPSRV_CmdType   cmdType,
    char            *auid_ptr,
    char            *folder_ptr,
    char            *doc_ptr)
{
    XCAP_Cmd           cmd;
    SUPSRV_XcapTrans  *trans_ptr;
    
    /* Get some scratch to construct the XCAP uri stuff */
    if (NULL == (trans_ptr = _SUPSRV_allocXcapTrans(xcap_ptr))) {
        /* No available resources */
        _SUPSRV_dbgPrintf("%s %d\n", __FUNCTION__, __LINE__); 
        return (SUPSRV_ERR);
    }
    
    /* Make a URI to get the RLS service doc. */
    if (0 == XCAP_helperMakeUri(
            xcap_ptr->username, /* Username */
            xcap_ptr->password, /* Password */
            xcap_ptr->server,
            auid_ptr,
            folder_ptr,
            xcap_ptr->uri,
            doc_ptr,
            NULL,
            trans_ptr->scratch[0],
            SUPSRV_SCRATCH_SZ - 1)) {
        /* Then we failed to create the request */
        _SUPSRV_dbgPrintf("%s %d\n", __FUNCTION__, __LINE__); 
        return (SUPSRV_ERR);
    }

    /* Setup a command */
    OSAL_memSet(&cmd, 0, sizeof(cmd));
    cmd.op = XCAP_OPERATION_FETCH;
    cmd.opType = XCAP_OPERATION_TYPE_DOCUMENT;
    cmd.uri_ptr = trans_ptr->scratch[0];
    cmd.auid_ptr = auid_ptr;
    cmd.username_ptr = xcap_ptr->username;
    cmd.password_ptr = xcap_ptr->password;
    cmd.userAgent_ptr = "vport";
    cmd.infcAddr_ptr = &(xcap_ptr->infcAddress);
    cmd.x3gpp_ptr = xcap_ptr->impu;

    /* Set the command type */
    trans_ptr->cmdType = cmdType;

    /* Now send it */
    if (1 != XCAP_sendCmd(&(xcap_ptr->xcap), &cmd)) {
        /* Failed to send request */
        _SUPSRV_dbgPrintf("%s: FAILED to send XCAP command\n", __FUNCTION__);
        return (SUPSRV_ERR);
    }
    /* 
     * Get the XCAP command count and copy it to the service object.  
     * This is used to match XCAP commands to XCAP events.
     */
    trans_ptr->cmdCnt = _SUPSRV_getCmdCnt(xcap_ptr);
    return (SUPSRV_OK);
}


/*
 * ======== _SUPSRV_updateTip()========
 *  This function make uri for TIP and send command to XCAP
 *
 * Return:
 *  SUPSRV_OK:  Operation executed successfully
 *  SUPSRV_ERR: Operation executed failed
 */
vint _SUPSRV_updateTip(
    SUPSRV_XcapObj  *xcap_ptr,
    vint             activate)
{
    XCAP_Cmd           cmd;
    SUPSRV_XcapTrans  *trans_ptr;

    /* Get some scratch to construct the XCAP uri stuff */
    if (NULL == (trans_ptr = _SUPSRV_allocXcapTrans(xcap_ptr))) {
        /* No available resources */
        _SUPSRV_dbgPrintf("%s %d\n", __FUNCTION__, __LINE__);
        return (SUPSRV_ERR);
    }

    /* Make a URI to get the RLS service doc. */
    if (0 == XCAP_helperMakeUri(
            xcap_ptr->username, /* Username */
            xcap_ptr->password, /* Password */
            xcap_ptr->server,     /* root */
            XCAP_SIMSERVS_AUID,    /* auid */
            XCAP_USERS,            /* folder */
            xcap_ptr->uri,      /* uri */
            XCAP_SIMSERVS_TIR_DOC,     /* doc */
            NULL,                  /* node */
            trans_ptr->scratch[0], /* dst */
            SUPSRV_SCRATCH_SZ - 1)) {
        /* Then we failed to create the request */
        _SUPSRV_dbgPrintf("%s %d\n", __FUNCTION__, __LINE__);
        return (SUPSRV_ERR);
    }

    /* Construct doc */
    if (activate) {
        OSAL_snprintf(trans_ptr->scratch[1], SUPSRV_SCRATCH_SZ,
                        SUPSRV_XCAP_TIP_DOC, SUPSRV_XCAP_TRUE);
    }
    else {
        /* deactivate */
        OSAL_snprintf(trans_ptr->scratch[1], SUPSRV_SCRATCH_SZ,
                        SUPSRV_XCAP_TIP_DOC, SUPSRV_XCAP_FALSE); 
    }
    _SUPSRV_dbgPrintf("uri:%s\n", trans_ptr->scratch[0]);
    _SUPSRV_dbgPrintf("doc:%s\n", trans_ptr->scratch[1]);
    /* Setup a command */
    OSAL_memSet(&cmd, 0, sizeof(cmd));
    cmd.op = XCAP_OPERATION_CREATE_REPLACE;
    cmd.opType = XCAP_OPERATION_TYPE_DOCUMENT;
    cmd.uri_ptr = trans_ptr->scratch[0];
    cmd.auid_ptr = XCAP_SIMSERVS_AUID;
    cmd.username_ptr = xcap_ptr->username;
    cmd.password_ptr = xcap_ptr->password;
    cmd.userAgent_ptr = "vport";
    cmd.src_ptr = trans_ptr->scratch[1];
    cmd.srcSz = OSAL_strlen(trans_ptr->scratch[1]);
    cmd.infcAddr_ptr = &(xcap_ptr->infcAddress);
    cmd.x3gpp_ptr = xcap_ptr->impu;

    /* Set the command type */
    trans_ptr->cmdType = SUPSRV_CMD_TIP_OPERATION;

    /* Now send it */
    if (1 != XCAP_sendCmd(&(xcap_ptr->xcap), &cmd)) {
        /* Failed to send request */
        _SUPSRV_dbgPrintf("%s: FAILED to send XCAP command\n", __FUNCTION__);
        return (SUPSRV_ERR);
    }
    /*
     * Get the XCAP command count and copy it to the service object.
     * This is used to match XCAP commands to XCAP events.
     */
    trans_ptr->cmdCnt = _SUPSRV_getCmdCnt(xcap_ptr);
    return (SUPSRV_OK);
}


/*
 * ======== _SUPSRV_updateTir()========
 *  This function make uri for TIR and send command to XCAP
 *
 * Return:
 *  SUPSRV_OK:  Operation executed successfully
 *  SUPSRV_ERR: Operation executed failed
 */
vint _SUPSRV_updateTir(
    SUPSRV_XcapObj  *xcap_ptr,
    vint             activate)
{
    XCAP_Cmd           cmd;
    SUPSRV_XcapTrans  *trans_ptr;

    /* Get some scratch to construct the XCAP uri stuff */
    if (NULL == (trans_ptr = _SUPSRV_allocXcapTrans(xcap_ptr))) {
        /* No available resources */
        _SUPSRV_dbgPrintf("%s %d\n", __FUNCTION__, __LINE__);
        return (SUPSRV_ERR);
    }

    /* Make a URI to get the RLS service doc. */
    if (0 == XCAP_helperMakeUri(
            xcap_ptr->username, /* Username */
            xcap_ptr->password, /* Password */
            xcap_ptr->server,     /* root */
            XCAP_SIMSERVS_AUID,    /* auid */
            XCAP_USERS,            /* folder */
            xcap_ptr->uri,      /* uri */
            XCAP_SIMSERVS_TIR_DOC,     /* doc */
            NULL,                  /* node */
            trans_ptr->scratch[0], /* dst */
            SUPSRV_SCRATCH_SZ - 1)) {
        /* Then we failed to create the request */
        _SUPSRV_dbgPrintf("%s %d\n", __FUNCTION__, __LINE__);
        return (SUPSRV_ERR);
    }

    /* Construct doc */
    if (activate) {
        OSAL_snprintf(trans_ptr->scratch[1], SUPSRV_SCRATCH_SZ,
                        SUPSRV_XCAP_TIR_DOC, SUPSRV_XCAP_TRUE);
    }
    else {
        /* deactivate */
        OSAL_snprintf(trans_ptr->scratch[1], SUPSRV_SCRATCH_SZ,
                        SUPSRV_XCAP_TIR_DOC, SUPSRV_XCAP_FALSE);
    }
    _SUPSRV_dbgPrintf("uri:%s\n", trans_ptr->scratch[0]);
    _SUPSRV_dbgPrintf("doc:%s\n", trans_ptr->scratch[1]);
    /* Setup a command */
    OSAL_memSet(&cmd, 0, sizeof(cmd));
    cmd.op = XCAP_OPERATION_CREATE_REPLACE;
    cmd.opType = XCAP_OPERATION_TYPE_DOCUMENT;
    cmd.uri_ptr = trans_ptr->scratch[0];
    cmd.auid_ptr = XCAP_SIMSERVS_AUID;
    cmd.username_ptr = xcap_ptr->username;
    cmd.password_ptr = xcap_ptr->password;
    cmd.userAgent_ptr = "vport";
    cmd.src_ptr = trans_ptr->scratch[1];
    cmd.srcSz = OSAL_strlen(trans_ptr->scratch[1]);
    cmd.infcAddr_ptr = &(xcap_ptr->infcAddress);
    cmd.x3gpp_ptr = xcap_ptr->impu;

    /* Set the command type */
    trans_ptr->cmdType = SUPSRV_CMD_TIR_OPERATION;

    /* Now send it */
    if (1 != XCAP_sendCmd(&(xcap_ptr->xcap), &cmd)) {
        /* Failed to send request */
        _SUPSRV_dbgPrintf("%s: FAILED to send XCAP command\n", __FUNCTION__);
        return (SUPSRV_ERR);
    }
    /*
     * Get the XCAP command count and copy it to the service object.
     * This is used to match XCAP commands to XCAP events.
     */
    trans_ptr->cmdCnt = _SUPSRV_getCmdCnt(xcap_ptr);
    return (SUPSRV_OK);
}


/*
 * ======== _SUPSRV_queryCd()========
 * This function will send query event to XCAP for getting call forward status
 *
 * Return:
 *  SUPSRV_OK:  Operation executed successfully
 *  SUPSRV_ERR: Operation executed failed
 */
vint _SUPSRV_queryCd(
    SUPSRV_XcapObj *xcap_ptr)
{
    
    if (SUPSRV_OK != _SUPSRV_fetchDocument(xcap_ptr,
            SUPSRV_CMD_GET_CD, XCAP_SIMSERVS_AUID, 
            XCAP_USERS, XCAP_SIMSERVS_CD_DOC)) {
        /* The command to fetch the document failed */
        return (SUPSRV_ERR);
    }
    return (SUPSRV_OK);
}


/*
 * ======== _SUPSRV_queryCbic()========
 * This function will send query event to XCAP for getting incoming
 * call barring  status
 *
 * Return:
 *  SUPSRV_OK:  Operation executed successfully
 *  SUPSRV_ERR: Operation executed failed
 */
vint _SUPSRV_queryCbic(
    SUPSRV_XcapObj *xcap_ptr)
{
    if (SUPSRV_OK != _SUPSRV_fetchDocument(xcap_ptr,
            SUPSRV_CMD_GET_CBIC, XCAP_SIMSERVS_AUID,
            XCAP_USERS, XCAP_SIMSERVS_CBIC_DOC)) {
        /* The command to fetch the document failed */
        return (SUPSRV_ERR);
    }
    return (SUPSRV_OK);
}


/*  ======== _SUPSRV_queryCboc()========
 * This function will send query event to XCAP for getting call barring
 * outgoing call status
 *
 * Return:
 *  SUPSRV_OK:  Operation executed successfully
 *  SUPSRV_ERR: Operation executed failed
 */
vint _SUPSRV_queryCboc(
    SUPSRV_XcapObj *xcap_ptr)
{
    if (SUPSRV_OK != _SUPSRV_fetchDocument(xcap_ptr,
            SUPSRV_CMD_GET_CBOG, XCAP_SIMSERVS_AUID, 
            XCAP_USERS, XCAP_SIMSERVS_CBOG_DOC)) {
        /* The command to fetch the document failed */
        return (SUPSRV_ERR);
    }
    return (SUPSRV_OK);
}


/*
 * ======== _SUPSRV_queryCw()======== 
 * This function will send query event to XCAP for getting Call Waiting status
 *
 * Return:
 *  SUPSRV_OK:  Operation executed successfully
 *  SUPSRV_ERR: Operation executed failed
 */
vint _SUPSRV_queryCw(
    SUPSRV_XcapObj *xcap_ptr)
{
    if (SUPSRV_OK != _SUPSRV_fetchDocument(xcap_ptr,
            SUPSRV_CMD_GET_CW, XCAP_SIMSERVS_AUID, 
            XCAP_USERS, XCAP_SIMSERVS_CW_DOC)) {
        /* The command to fetch the document failed */
        return (SUPSRV_ERR);
    }
    return (SUPSRV_OK);
}


/*
 * ======== _SUPSRV_getXcapEvtOwner()======== 
 * This function is to get the transaction owener of a XCAP event.
 *
 * Return:
 *  -1: Failed to get the transaction id.
 *  Otherwise: A transaction id of the owener of the XCAP event.
 */
vint _SUPSRV_getXcapEvtOwner(
    SUPSRV_Mngr  *mngr_ptr) 
{
    vint               transIndex;
    uint32             cmdCnt;
    SUPSRV_XcapTrans  *trans_ptr;

    /*
     * Find the service that this XCAP event belongs to.  The service with the
     * lowest cmdCnt is the one this event belongs to.
     */

    cmdCnt = 0xFFFFFFFF;
    for (transIndex = 0; transIndex < SUPSRV_MAX_TRANSACTIONS;
            transIndex++) {
           trans_ptr = &mngr_ptr->supSrvXcap.trans[transIndex]; 
        if (0 != trans_ptr->cmdCnt && trans_ptr->cmdCnt < cmdCnt) {
                return (transIndex);
        }
    }
    return(-1);
}

/*
 * ======== _SUPSRV_getXcapResponseCode()======== 
 * This function is to parse the response code of a XCAP event.
 *
 * Return:
 *  0: Failed to parse the response code.
 *  Otherwise: The response code of a XCAP event.
 */
vint _SUPSRV_getXcapResponseCode(
    XCAP_Evt *xcapEvt_ptr)
{
    char *h_ptr;
    char *start_ptr;
    vint  resCode;
    
    h_ptr = xcapEvt_ptr->hdr_ptr;

    if (NULL == h_ptr || 0 == *h_ptr) {
        return 0;
    }
    if (0 != OSAL_strncasecmp(h_ptr, SUPSRV_HTTP_HEADER,
            sizeof(SUPSRV_HTTP_HEADER) - 1)) {
        return 0;
    }
    /* Okay we have an HTTP response, let's get the response code */
    if (NULL == (start_ptr = OSAL_strchr(h_ptr, ' '))) {
        return 0;
    }
    start_ptr++;
    resCode = OSAL_atoi(start_ptr);
    if (0 > resCode) {
        return 0;
    }
    return (resCode);
}

/*
 * ======== _SUPSRV_isCfwModeExist()======== 
 * This function is to check the call forward mode from the XCAP content
 *
 * Returns:
 *     0: false, cannot find the condition that match mode.
 *     1: true, find the condition that match mode.
 */
vint _SUPSRV_isCfwModeExist(
    SUPSRV_CFMode  mode,
    char          *res_ptr)
{
    ezxml_t  xml_ptr;
    ezxml_t  child_xml;
    vint     ret;

    ret = 0;
    xml_ptr = ezxml_parse_str(res_ptr, OSAL_strlen(res_ptr));
    if (NULL != xml_ptr) {
        child_xml = ezxml_child(xml_ptr, SUPSRV_XCAP_CF);
        if (NULL != child_xml) {
            child_xml = ezxml_child(child_xml, SUPSRV_XCAP_CONDITIONS);
            if (NULL != child_xml) {
                if (0 == OSAL_strcmp((char *)ezxml_txt(child_xml), "")) {
                    if (SUPSRV_EVENT_CF_MODE_UNCONDITION == mode) {
                        ret = 1;
                    }
                }
                else {
                    if ((NULL != ezxml_child(child_xml, SUPSRV_CD_STR_BUSY)) &&
                            (SUPSRV_EVENT_CF_MODE_BUSY == mode)) {
                        ret = 1;
                    }
                    if ((NULL != ezxml_child(child_xml, SUPSRV_CD_STR_NOANS)) &&
                            (SUPSRV_EVENT_CF_MODE_NOREPLY == mode)) {
                        ret = 1;
                    }
                    if ((NULL != ezxml_child(child_xml, SUPSRV_CD_STR_NOTREACH)) &&
                            (SUPSRV_EVENT_CF_MODE_NOTREACH == mode)) {
                        ret = 1;
                    }
                    if ((NULL != ezxml_child(child_xml, SUPSRV_CD_STR_NOTREG)) &&
                            (SUPSRV_EVENT_CF_MODE_NOTLOGIN == mode)) {
                        ret = 1;
                    }
                    if ((NULL != ezxml_child(child_xml, SUPSRV_CD_STR_TIME)) &&
                            (SUPSRV_EVENT_CF_MODE_TIME == mode)) {
                        ret = 1;
                    }
                }
            }
        }
    }
    ezxml_free(xml_ptr);
    return (ret);
}

/*
 * ======== _SUPSRV_getCfwMode()======== 
 * This function is to parse call forward Result.
 *
 * Returns:
 *     SUPSRV_EVENT_CF_MODE_UNCONDITION, 
 *     SUPSRV_EVENT_CF_MODE_BUSY
 *     SUPSRV_EVENT_CF_MODE_NOREPLY, 
 *     SUPSRV_EVENT_CF_MODE_NOTREACH
 *     SUPSRV_EVENT_CF_MODE_NOTLOGIN
 */
vint _SUPSRV_parseCfwResult(
    char             *res_ptr,
    SUPSRV_Output    *out_ptr)
{
    ezxml_t  xml_ptr;
    ezxml_t  child_xml;
    ezxml_t  tar_xml;
    char    *value_ptr;
    char     content[SUPSRV_SCRATCH_SZ];

    OSAL_strcpy(content, res_ptr);
    xml_ptr = ezxml_parse_str(res_ptr, OSAL_strlen(res_ptr));
    if (NULL != xml_ptr) {
        /* Get active state by query communication-diversion */
        child_xml = ezxml_child(xml_ptr, SUPSRV_XCAP_CF);
        if (NULL != child_xml) {
            value_ptr = (char *)ezxml_attr(child_xml, SUPSRV_XCAP_ATCIVE);
            if (NULL != value_ptr) {
                /* Check the Call forward mode */
                if (1 != _SUPSRV_isCfwModeExist(out_ptr->mode.cfMode,
                        content)) {
                    /*  Return false mode */
                    out_ptr->queryEnb.genResStatus= SUPSRV_RES_DISABLE;
                }
                else {
                    if (0 == OSAL_strncmp(value_ptr, SUPSRV_XCAP_TRUE, 4)) {
                        /* Active is true */
                        out_ptr->queryEnb.genResStatus = SUPSRV_RES_ENABLE;
                    }
                    else {
                        /* Active is false*/
                        out_ptr->queryEnb.genResStatus = SUPSRV_RES_DISABLE;
                    }
                    /* 
                     * look for the value of <target> of first <forward-to> of
                     * first <actions> under the child which is
                     * <communication-diversion active="true">
                     */
                    tar_xml = ezxml_get(child_xml, SUPSRV_XCAP_ACTIONS, 0,
                            SUPSRV_XCAP_FORWARDTO, 0,
                            SUPSRV_XCAP_RESPONSE_TARGET, -1);
                    if (NULL != tar_xml) {
                        OSAL_strcpy(out_ptr->u.ruleParams.cfwNumber,
                                (char *)ezxml_txt(tar_xml));
                    }

                    /*
                     * look for the value of <time> of
                     * first <conditions> under the child which is
                     * <communication-diversion active="true">
                     */
                    tar_xml = ezxml_child(child_xml,
                            SUPSRV_XCAP_CONDITIONS);
                    if (NULL != tar_xml) {
                        tar_xml = ezxml_child(tar_xml,
                            SUPSRV_CD_STR_TIME);
                        if (NULL != tar_xml) {
                            OSAL_strcpy(out_ptr->u.ruleParams.timeRangeOfTheDay,
                                (char *)ezxml_txt(tar_xml));
                        }
                    }

                    /* Get the NoReplyTimer */
                    tar_xml = ezxml_child(child_xml,
                            SUPSRV_XCAP_RESPONSE_NOREPLYTIMER);
                    if (NULL != tar_xml) {
                        out_ptr->u.ruleParams.noReplyTimer =
                                OSAL_atoi((char *)ezxml_txt(tar_xml));
                    }
                }
                ezxml_free(xml_ptr);
                return(SUPSRV_OK);
            }
        }
    }

    /* Error */
    out_ptr->errorCode = SUPSRV_ERROR_XML_PARSING;
    ezxml_free(xml_ptr);
    return(SUPSRV_ERR);
}

/*
 * ======== _SUPSRV_parseCBResult()======== 
 * This function is to parse call barring result
 *
 * Returns:
 *  SUPSRV_OK:  Parsed successfully
 *  SUPSRV_ERR: Parsed failed.
 */
vint _SUPSRV_parseCbResult(
    char            *res_ptr,
    SUPSRV_Output   *out_ptr)
{
    ezxml_t          xml_ptr;
    ezxml_t          child_xml;
    char            *value_ptr;
    SUPSRV_CbMode    mode;

    mode = out_ptr->mode.cbMode;

    xml_ptr = ezxml_parse_str(res_ptr, OSAL_strlen(res_ptr));
    if (NULL != xml_ptr) {
        /* Get active state by query communication-barring */
        child_xml = ezxml_child(xml_ptr, SUPSRV_XCAP_CB);
        if (NULL != child_xml) {
            value_ptr = (char *)ezxml_attr(child_xml, SUPSRV_XCAP_ATCIVE);
            if (NULL != value_ptr) {
                if (0 == OSAL_strncmp(value_ptr, SUPSRV_XCAP_TRUE, 4)) {
                    /* Active is true */
                    if (SUPSRV_EVENT_CB_MODE_BOIC == mode ||
                            SUPSRV_EVENT_CB_MODE_BAOC == mode) {
                        /* Get the result of serv-cap-international */
                        child_xml = ezxml_child(xml_ptr,
                                SUPSRV_XCAP_RESPONSE_BOIC);
                        if (NULL != child_xml) {
                            if (0 == OSAL_strncmp((char *)ezxml_txt(child_xml),
                                    SUPSRV_XCAP_TRUE, 4)) {
                                /* <serv-cap-international> is true */
                                if (SUPSRV_EVENT_CB_MODE_BOIC == mode) {
                                    out_ptr->queryEnb.genResStatus =
                                            SUPSRV_RES_ENABLE;
                                }
                                else {
                                    out_ptr->queryEnb.genResStatus =
                                            SUPSRV_RES_DISABLE;
                                }
                            }
                            else {
                                /* <serv-cap-international> is false */
                                if (SUPSRV_EVENT_CB_MODE_BAOC == mode){
                                    /* mode = Barring all outgoing call */
                                    out_ptr->queryEnb.genResStatus =
                                            SUPSRV_RES_ENABLE;
                                }
                                else {
                                    out_ptr->queryEnb.genResStatus =
                                            SUPSRV_RES_DISABLE;
                                }
                            }
                            ezxml_free(xml_ptr);
                            return(SUPSRV_OK);
                        }
                    }
                    else if (SUPSRV_EVENT_CB_MODE_BOIC_EXHC == mode) {
                        /* Get the result of serv-cap-international-exHC */
                        child_xml = ezxml_child(xml_ptr,
                                SUPSRV_XCAP_RESPONSE_BOIC_EXHC);
                        if (NULL != child_xml) {
                            if (0 == OSAL_strncmp((char *)ezxml_txt(child_xml),
                                    SUPSRV_XCAP_TRUE, 4)) {
                                out_ptr->queryEnb.genResStatus =
                                        SUPSRV_RES_ENABLE;
                            }
                            else {
                                out_ptr->queryEnb.genResStatus =
                                        SUPSRV_RES_DISABLE;
                            }
                            ezxml_free(xml_ptr);
                            return(SUPSRV_OK);
                        }
                    }
                    else if (SUPSRV_EVENT_CB_MODE_BICR == mode ||
                            SUPSRV_EVENT_CB_MODE_BAIC == mode) {
                        /* Get the result of serv-cap-roaming */
                        child_xml = ezxml_child(xml_ptr, SUPSRV_XCAP_RESPONSE_BICR);
                        if (NULL != child_xml) {
                            if (0 == OSAL_strncmp((char *)ezxml_txt(child_xml),
                                    SUPSRV_XCAP_TRUE, 4)) {
                                /* true */
                                if (SUPSRV_EVENT_CB_MODE_BICR == mode) {
                                    out_ptr->queryEnb.genResStatus =
                                            SUPSRV_RES_ENABLE;
                                }
                                else {
                                    out_ptr->queryEnb.genResStatus =
                                            SUPSRV_RES_DISABLE;
                                }
                            }
                            else {
                                if (SUPSRV_EVENT_CB_MODE_BAIC == mode) {
                                    /*  Barring All Incoming Call Query*/
                                    out_ptr->queryEnb.genResStatus =
                                            SUPSRV_RES_ENABLE;
                                }
                                else {
                                    out_ptr->queryEnb.genResStatus =
                                            SUPSRV_RES_DISABLE;
                                }
                            }
                            ezxml_free(xml_ptr);
                            return(SUPSRV_OK);
                        }
                    }
                }
                else {
                    /* Active is false*/
                    out_ptr->queryEnb.genResStatus = SUPSRV_RES_DISABLE;
                    ezxml_free(xml_ptr);
                    return(SUPSRV_OK);
                }
            }
        }
    }

    /* Error */
    out_ptr->errorCode = SUPSRV_ERROR_XML_PARSING;
    ezxml_free(xml_ptr);
    return(SUPSRV_ERR);
}

/*
 * ======== _SUPSRV_parseSupSrvQueryResult()======== 
 * This function is to parse the XCAP query result
 *
 * Returns:
 *  SUPSRV_OK:  Result parsed successfully
 *  SUPSRV_ERR: Result parse failed.
 */
vint _SUPSRV_parseSupSrvQueryResult(
    const XCAP_Evt   *xcapEvt_ptr,
    SUPSRV_Output    *out_ptr,
    SUPSRV_CmdType    cmdType)
{
    SUPSRV_EventReason   response;
    vint   index;
    vint   ret;
    vint   size = sizeof(SUPSRV_RESPONSE_REASON) / sizeof(
            SUPSRV_IntResponse);

    response = SUPSRV_NONE;
    for (index = 0 ; index < size ; index++) {
        if (NULL != OSAL_strscan(xcapEvt_ptr->body_ptr,
                SUPSRV_RESPONSE_REASON[index].at_ptr)) {
            response = SUPSRV_RESPONSE_REASON[index].reason;
            break;
        }
    }
    switch (response) {
        case SUPSRV_EVENT_REASON_AT_CMD_OIP:
        case SUPSRV_EVENT_REASON_AT_CMD_OIR:
            /* Determine query from command type */
            if (SUPSRV_CMD_GET_OIP == cmdType) {
                ret = _SUPSRV_parseQueryResult(xcapEvt_ptr->body_ptr,
                        OSAL_strlen(xcapEvt_ptr->body_ptr),
                        SUPSRV_XCAP_OIP);
                if (0 == ret) {
                    /* Disabled */
                    out_ptr->queryEnb.genResStatus = SUPSRV_RES_DISABLE;
                    out_ptr->prov.genProv = SUPSRV_NOT_PROVISIONED;
                }
                else if (1 == ret) {
                    /* Enabled */
                    out_ptr->queryEnb.genResStatus = SUPSRV_RES_ENABLE;
                    out_ptr->prov.genProv = SUPSRV_PROVISIONED;
                }
                else {
                    /* xml parse error */
                    out_ptr->errorCode = SUPSRV_ERROR_XML_PARSING;
                    return (SUPSRV_ERR);
                }
                
            }
            else if (SUPSRV_CMD_GET_OIR == cmdType){
                ret = _SUPSRV_parseQueryResult(xcapEvt_ptr->body_ptr,
                        OSAL_strlen(xcapEvt_ptr->body_ptr),
                        SUPSRV_XCAP_OIR);
                if (0 == ret) {
                    /* Disabled */
                    out_ptr->queryEnb.oirResStatus = SUPSRV_OIR_SUPPRESSION;
                    out_ptr->prov.oirProv = SUPSRV_OIR_TEMP_MODE_ALLOWED;
                }
                else if (1 == ret) {
                    /* Enabled */
                    out_ptr->queryEnb.oirResStatus = SUPSRV_OIR_INVOCATION;
                    out_ptr->prov.oirProv = SUPSRV_OIR_TEMP_MODE_RESTRICTED;
                }
                else {
                    /* xml parse error */
                    out_ptr->errorCode = SUPSRV_ERROR_XML_PARSING;
                    return (SUPSRV_ERR);
                }
            }
            else {
                /* Invalide command */
                out_ptr->errorCode = SUPSRV_ERROR_UNKNOWN;
                return (SUPSRV_ERR);
            }

            break;
        case SUPSRV_EVENT_REASON_AT_CMD_TIP:
        case SUPSRV_EVENT_REASON_AT_CMD_TIR:
            /* Determine query from command type */
            if (SUPSRV_CMD_GET_TIP == cmdType) {
                ret = _SUPSRV_parseQueryResult(xcapEvt_ptr->body_ptr,
                        OSAL_strlen(xcapEvt_ptr->body_ptr),
                        SUPSRV_XCAP_TIP);
                if (0 == ret) {
                    /* Disabled */
                    out_ptr->queryEnb.genResStatus = SUPSRV_RES_DISABLE;
                    out_ptr->prov.genProv = SUPSRV_NOT_PROVISIONED;
                }
                else if (1 == ret) {
                    /* Enabled */
                    out_ptr->queryEnb.genResStatus = SUPSRV_RES_ENABLE;
                    out_ptr->prov.genProv = SUPSRV_PROVISIONED;
                }
                else {
                    /* xml parse error */
                    out_ptr->errorCode = SUPSRV_ERROR_XML_PARSING;
                    return (SUPSRV_ERR);
                }
                
            }
            else if (SUPSRV_CMD_GET_TIR == cmdType){
                ret = _SUPSRV_parseQueryResult(xcapEvt_ptr->body_ptr,
                        OSAL_strlen(xcapEvt_ptr->body_ptr),
                        SUPSRV_XCAP_TIR);
                if (0 == ret) {
                    /* Disabled */
                    out_ptr->queryEnb.genResStatus = SUPSRV_RES_DISABLE;
                    out_ptr->prov.genProv = SUPSRV_NOT_PROVISIONED;
                }
                else if (1 == ret) {
                    /* Enabled */
                    out_ptr->queryEnb.genResStatus = SUPSRV_RES_ENABLE;
                    out_ptr->prov.genProv = SUPSRV_PROVISIONED;
                }
                else {
                    /* xml parse error */
                    out_ptr->errorCode = SUPSRV_ERROR_XML_PARSING;
                    return (SUPSRV_ERR);
                }
            }
            else {
                /* Invalide command */
                out_ptr->errorCode = SUPSRV_ERROR_UNKNOWN;
                return (SUPSRV_ERR);
            }

            break;
        case SUPSRV_EVENT_REASON_AT_CMD_CW:
            /* Check if reponse match to the query command type */
            if (cmdType != SUPSRV_CMD_GET_CW){
                out_ptr->errorCode = SUPSRV_ERROR_UNKNOWN;
                return (SUPSRV_ERR);
            }
            ret = _SUPSRV_parseQueryResult(xcapEvt_ptr->body_ptr,
                    OSAL_strlen(xcapEvt_ptr->body_ptr),
                    SUPSRV_XCAP_CW);
            if (0 == ret) {
                /* Disabled */
                out_ptr->queryEnb.genResStatus = SUPSRV_RES_DISABLE;
                out_ptr->prov.genProv = SUPSRV_NOT_PROVISIONED;
            }
            else if (1 == ret) {
                /* Enabled */
                out_ptr->queryEnb.genResStatus = SUPSRV_RES_ENABLE;
                out_ptr->prov.genProv = SUPSRV_PROVISIONED;
            }
            else {
                /* xml parse error */
                out_ptr->errorCode = SUPSRV_ERROR_XML_PARSING;
                return (SUPSRV_ERR);
            }
            break;
        case SUPSRV_EVENT_REASON_AT_CMD_CF:
            /* Check if reponse match to the query command type */
            if (SUPSRV_CMD_GET_CD != cmdType){
                out_ptr->errorCode = SUPSRV_ERROR_UNKNOWN;
                return (SUPSRV_ERR);
            }
            return (_SUPSRV_parseCfwResult(xcapEvt_ptr->body_ptr, out_ptr));
        case SUPSRV_EVENT_REASON_AT_CMD_CB:
            /* Check if reponse match to the query command type */
            return (_SUPSRV_parseCbResult(xcapEvt_ptr->body_ptr, out_ptr));

        default:
            out_ptr->errorCode = SUPSRV_ERROR_UNKNOWN;
            return (SUPSRV_ERR);
    }
    return (SUPSRV_OK);
}

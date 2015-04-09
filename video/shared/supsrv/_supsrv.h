/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2014 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 *
 */
#include <osal.h>
#include <supsrv.h>
#include <xcap.h>

#ifndef SUPSRV_DEBUG
#define _SUPSRV_dbgPrintf(fmt, args...)
#else
#define _SUPSRV_dbgPrintf(fmt, args...) \
         OSAL_logMsg("[%s:%d] " fmt, __FUNCTION__, __LINE__, ## args)
#endif

#define SUPSRV_HTTP_HEADER                      ("HTTP")
#define SUPSRV_XCAP_RESPONSE_200OK              ("200 OK")
#define SUPSRV_XCAP_OIP                         ("originating-identity-presentation")
#define SUPSRV_XCAP_OIR                         ("originating-identity-presentation-restriction")
#define SUPSRV_XCAP_TIP                         ("terminating-identity-presentation")
#define SUPSRV_XCAP_TIR                         ("terminating-identity-presentation-restriction")
#define SUPSRV_XCAP_CF                          ("communication-diversion")
#define SUPSRV_XCAP_CW                          ("communication-waiting")
#define SUPSRV_XCAP_ATCIVE                      ("active")
#define SUPSRV_XCAP_TRUE                        ("true")
#define SUPSRV_XCAP_FALSE                       ("false")
#define SUPSRV_XCAP_ATCIVE_TRUE                 ("active=\"true\"")
#define SUPSRV_XCAP_ATCIVE_FALSE                ("active=\"false\"")
#define SUPSRV_XCAP_CONDITIONS                  ("conditions")
#define SUPSRV_XCAP_ACTIONS                     ("actions")
#define SUPSRV_XCAP_FORWARDTO                   ("forward-to")
#define SUPSRV_XCAP_RESPONSE_NOREPLYTIMER       ("NoReplyTimer")
#define SUPSRV_XCAP_RESPONSE_TARGET             ("target")
#define SUPSRV_XCAP_CB                          ("communication-barring")
#define SUPSRV_XCAP_RESPONSE_BOIC               ("serv-cap-international")
#define SUPSRV_XCAP_RESPONSE_BOIC_EXHC          ("serv-cap-international-exHC")
#define SUPSRV_XCAP_RESPONSE_BICR               ("serv-cap-roaming")
#define SUPSRV_XCAP_RESPONSE_BAOC               ("outgoing-communication-barring")
#define SUPSRV_XCAP_RESPONSE_BAIC               ("incoming-communication-barring")
#define SUPSRV_CD_STR_BUSY                      ("busy")
#define SUPSRV_CD_STR_NOANS                     ("no-answer")
#define SUPSRV_CD_STR_NOTREG                    ("not-registered")
#define SUPSRV_CD_STR_NOTREACH                  ("not-reachable")
#define SUPSRV_CD_STR_TIME                      ("time")

typedef enum {
    SUPSRV_CALL_ADDRESS_NATIONAL      = 129,
    SUPSRV_CALL_ADDRESS_INTERNATIONAL = 145,
    SUPSRV_CALL_ADDRESS_TYPE_3,
} SUPSRV_CallAddressType;

typedef struct {
    SUPSRV_EventReason     reason;
    char                  *at_ptr;
} SUPSRV_IntResponse;

vint _SUPSRV_parseQueryResult(
    char *doc_ptr,
    vint  docLen,
    char *tag_ptr);

uint32 _SUPSRV_getCmdCnt(
    SUPSRV_XcapObj   *gxcap_ptr);

SUPSRV_XcapTrans* _SUPSRV_allocXcapTrans(
    SUPSRV_XcapObj *xcapObj_ptr);

vint _SUPSRV_updateService(
    SUPSRV_XcapObj  *xcap_ptr,
    char            *doc_ptr,
    vint             cmdType);

vint _SUPSRV_updateOip(
    SUPSRV_XcapObj  *xcap_ptr,
    vint             activate);

vint _SUPSRV_updateOir(
    SUPSRV_XcapObj  *xcap_ptr,
    vint             activate);

vint _SUPSRV_fetchDocument(
    SUPSRV_XcapObj  *xcap_ptr,
    SUPSRV_CmdType   cmdType,
    char            *auid_ptr,
    char            *folder_ptr,
    char            *doc_ptr);

vint _SUPSRV_updateTip(
    SUPSRV_XcapObj  *xcap_ptr,
    vint             activate);

vint _SUPSRV_updateTir(
    SUPSRV_XcapObj  *xcap_ptr,
    vint             activate);

vint _SUPSRV_queryCd(
    SUPSRV_XcapObj *xcap_ptr);

vint _SUPSRV_queryCbic(
    SUPSRV_XcapObj *xcap_ptr);

vint _SUPSRV_queryCboc(
    SUPSRV_XcapObj *xcap_ptr);

vint _SUPSRV_queryCw(
    SUPSRV_XcapObj *xcap_ptr);

vint _SUPSRV_getXcapEvtOwner(
    SUPSRV_Mngr  *mngr_ptr);

vint _SUPSRV_getXcapResponseCode(
    XCAP_Evt *xcapEvt_ptr);

vint _SUPSRV_isCfwModeExist(
    SUPSRV_CFMode  mode,
    char          *res_ptr);

vint _SUPSRV_parseCfwResult(
    char             *res_ptr,
    SUPSRV_Output    *out_ptr);

vint _SUPSRV_parseCbResult(
    char            *res_ptr,
    SUPSRV_Output   *out_ptr);

vint _SUPSRV_parseSupSrvQueryResult(
    const XCAP_Evt  *xcapEvt_ptr,
    SUPSRV_Output   *out_ptr,
    SUPSRV_CmdType   cmdType);


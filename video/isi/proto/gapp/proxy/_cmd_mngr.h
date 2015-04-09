/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30347 $ $Date: 2014-12-11 12:16:45 +0800 (Thu, 11 Dec 2014) $
 */

#ifndef _CMD_MNGR_H_
#define _CMD_MNGR_H_

/*
 * Public constants
 */
#define PRXY_INVALID_COMMAND_ID (0)
#define PRXY_AT_RESPONSE_OK                 ("0")
#define PRXY_AT_RESPONSE_ERROR              ("+CME ERROR:")
#define PRXY_AT_RESPONSE_SMS_ERROR          ("+CMS ERROR:")
#define PRXY_AT_RESPONSE_REMOTE_DISCONNECT  ("3")
#define PRXY_AT_RESPONSE_INCOMING           ("+CRING: VOICE\r\n+CLIP:")
#define PRXY_AT_RESPONSE_INCOMING_VIDEO     ("+CRING: VOICE/VIDEO\r\n+CLIP:")
#define PRXY_AT_RESPONSE_REPORT             ("+CLCC:")
#define PRXY_AT_RESPONSE_REPORT_EXT         ("+CLCCS:")
#define PRXY_AT_RESPONSE_WAITING            ("+CCWA:")
#define PRXY_AT_RESPONSE_SMS_SENT           ("+CMGS:")
#define PRXY_AT_RESPONSE_SMS_SATAUS_REPORT  ("+CDS:")
#define PRXY_AT_RESPONSE_SMS_RECV           ("+CMT:")
#define PRXY_AT_RESPONSE_OIP                ("+CLIP:")
#define PRXY_AT_RESPONSE_OIR                ("+CLIR:")
#define PRXY_AT_RESPONSE_TIP                ("+COLP:")
#define PRXY_AT_RESPONSE_TIR                ("+COLR:")
#define PRXY_AT_RESPONSE_CF                 ("+CCFC:")
#define PRXY_AT_RESPONSE_CB                 ("+CLCK:")
#define PRXY_AT_RESPONSE_WAITING            ("+CCWA:")
#define PRXY_AT_RESPONSE_SUPSRV_MO_CALL        ("+CSSI:")
#define PRXY_AT_RESPONSE_SUPSRV_MT_CALL        ("+CSSU:")
#define PRXY_AT_RESPONSE_SUPSRV_HISTORY     ("+CSSH:")
#define PRXY_AT_RESPONSE_CR_LN              ("\r\n")
#define PRXY_AT_RESPONSE_CR                 ("\r")
#define PRXY_AT_RESPONSE_STR_OK             ("OK")
#define PRXY_AT_RESPONSE_CTRL_Z             (26)
#define PRXY_AT_RESPONSE_CLASS              ("1")
#define PRXY_AT_RESPONSE_CF_TYPE_145        (145)
#define PRXY_AT_RESPONSE_CF_TYPE_129        (129)
#define PRXY_AT_RESPONSE_ERROR_SIP          (176)
#define PRXY_AT_RESPONSE_USSD_SENT          ("+CUSD:")
#define PRXY_AT_RESPONSE_CDU                ("+CDU:")
#define PRXY_AT_RESPONSE_MONITOR_EXT        ("+CMCCSI:")
#define PRXY_AT_RESPONSE_SUPSRV_EXT1        ("+CMCCSS1:")
#define PRXY_AT_RESPONSE_SUPSRV_EXT2        ("+CMCCSS2:")
#define PRXY_AT_RESPONSE_SUPSRV_EXT3        ("+CMCCSS3:")
#define PRXY_AT_RESPONSE_SUPSRV_EXT4        ("+CMCCSS4:")
#define PRXY_AT_RESPONSE_SUPSRV_EXT5        ("+CMCCSS5:")
#define PRXY_AT_RESPONSE_SUPSRV_EXT6        ("+CMCCSS6:")
#define PRXY_AT_RESPONSE_SUPSRV_EXT7        ("+CMCCSS7:")
#define PRXY_AT_RESPONSE_SUPSRV_EXT_END     ("+CMCCSSEND")
#define PRXY_AT_RESPONSE_SUPSRV_TYPE_ALIAS_URI      (256)
#define PRXY_AT_RESPONSE_SUPSRV_TYPE_VIRTUAL_RING   (257)
#define PRXY_AT_RESPONSE_SUPSRV_TYPE_MEDIA_CONVERT  (258)
#define PRXY_AT_RESPONSE_SUPSRV_TYPE_PARTI_STATUS   (259)
#define PRXY_AT_CMD_CB_ALL_OUTGOING         ("AO") /* Barring of All Outgoing  Calls */
#define PRXY_AT_CMD_CB_OUTGOING_INTERNATION ("OI") /* Barring of Outgoing International Calls */
#define PRXY_AT_CMD_CB_OUTGOING_INTERNATION_EXHC ("OX") /* Barring of Outgoing International Calls except to Home Country */
#define PRXY_AT_CMD_CB_ALL_INCOMING         ("AI") /* Barring of All Incoming Calls */
#define PRXY_AT_CMD_CB_INCOMING_ROAMING     ("IR") /* Barring of Incoming Calls - When Roaming */
#define PRXY_AT_CMD_CREG                    ("AT+CREG")
#define PRXY_AT_RESPONSE_CREG               ("+CREG")
/* Define media profile command */
#define PRXY_AT_CMD_CDEFMP                  ("AT+CDEFMP") 
/* Dial URI command */
#define PRXY_AT_CMD_CDU                     ("AT+CDU")
/* D2 private AT command */
#define PRXY_AT_RESPONSE_DTMF_DETECT        ("\%DTMFD")
#define PRXY_AT_RESPONSE_VIDEO_REQUEST_KEY  ("\%VIDEO_REQUEST_KEY")
/* CCFC extension with timeRange */
#define PRXY_AT_RESPONSE_CCFCX              ("\%CCFCX:")


/* Private unsolicaited result */
#define PRXY_AT_RESPONSE_AKA_CLG            ("\%AKACLG:") /* AKA Challenge */
#define PRXY_AT_RESPONSE_CIREGU             ("\%CIREGU:") /* IMS registration status */
#define PRXY_AT_RESPONSE_IMS_SRV_NOT        ("\%IMSSN:") /* IMS service notification */

/* URI parameters */
#define PRXY_URI_PARAM_USE_ALIAS            ("useAlias")

/* Define CGI type */
typedef enum {
    PRXY_CGI_TYPE_3GPP_GERAN,
    PRXY_CGI_TYPE_3GPP_UTRAN_FDD,
    PRXY_CGI_TYPE_3GPP_UTRAN_TDD,
    PRXY_CGI_TYPE_3GPP_E_UTRAN_FDD,
    PRXY_CGI_TYPE_3GPP_E_UTRAN_TDD,
    PRXY_CGI_TYPE_IEEE_802_11
} PRXY_CgiType;

/*
 * Public structure definitions
 */
typedef struct {
    CSM_ServiceReason   reason;
    char               *reasonDesc_ptr;
    char               *at_ptr;
    vint                atLen;
} PRXY_ServiceIntExt;

typedef struct {
    CSM_RadioReason   reason;
    char             *reasonDesc_ptr;
    char             *at_ptr;
    vint              atLen;
} PRXY_RadioIntExt;

typedef struct {
    CSM_CallReason reason;
    char          *reasonDesc_ptr;
    char          *at_ptr;
    vint           atLen;
} PRXY_CallIntExt;

typedef struct {
    CSM_SmsReason  reason;
    char          *reasonDesc_ptr;
    char          *at_ptr;
    vint           atLen;
} PRXY_SmsIntExt;

typedef struct {
    CSM_SupSrvReason  reason;
    char          *reasonDesc_ptr;
    char          *at_ptr;
    vint           atLen;
} PRXY_SupSrvIntExt;

typedef struct {
    CSM_UssdReason  reason;
    char          *reasonDesc_ptr;
    char          *at_ptr;
    vint           atLen;
} PRXY_UssdIntExt;

/*
 * Private methods
 */
vint _PRXY_cmdMngrUpdateIntExtTbl(
    const CSM_OutputEvent *response_ptr);

vint _PRXY_cmdMngrConstructAtResult(
    PRXY_CommandMngr      *cmdMngr_ptr,
    const CSM_OutputEvent *response_ptr,
    char                  *out_ptr,
    vint                  maxOutSize);

vint _PRXY_cmdMngrConstructAtEvent(
    PRXY_CommandMngr      *cmdMngr_ptr,
    const CSM_OutputEvent *response_ptr,
    char                  *out_ptr,
    vint                   maxOutSize);

vint _PRXY_cmdMngrParseAtCommand(
    PRXY_CommandMngr *cmdMngr_ptr,
    const char       *at_ptr,
    CSM_InputEvent   *csmEvt_ptr);

vint _PRXY_cmdMngrParseSmsAtCommand(
    PRXY_CommandMngr *cmdMngr_ptr,
    const char       *at_ptr,
    CSM_InputEvent   *csmEvt_ptr);

vint _PRXY_cmdMngrParseNetworkRegAtCommand(
    PRXY_CommandMngr *cmdMngr_ptr,
    const char       *at_ptr);

vint _PRXY_cmdMngrParseSupSrvAtCommand(
    const char       *at_ptr,
    CSM_InputEvent   *csmEvt_ptr);

vint _PRXY_cmdMngrConstructSupsrvQueryResult(
    const CSM_OutputSupSrv *supSrv_ptr,
    char                   *out_ptr,
    vint                    maxOutSize);

vint _PRXY_cmdMngrParseUssdAtCommand(
    PRXY_CommandMngr *cmdMngr_ptr,
    const char       *at_ptr,
    CSM_InputEvent   *csmEvt_ptr);

vint _PRXY_cmdMngrParseMediaProfileAtCommand(
    PRXY_CommandMngr *cmdMngr_ptr,
    const char       *at_ptr,
    CSM_InputEvent   *csmEvt_ptr);

vint _PRXY_cmdMngrParseServiceAtCommand(
    const char       *at_ptr,
    CSM_InputEvent   *csmEvt_ptr);

vint _PRXY_cmdMngrParseRadioAtCommand(
    const char       *at_ptr,
    CSM_InputEvent   *csmEvt_ptr);

#endif

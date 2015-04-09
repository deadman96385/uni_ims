/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29904 $ $Date: 2014-11-19 02:54:52 +0800 (Wed, 19 Nov 2014) $
 */

#ifndef _PROXY_H_
#define _PROXY_H_

#define PRXY_TERMINAL_NAME "/dev/smd0"
#define PRXY_IO_MSG_Q_NAME "prxy.io.out.q"

/* The following are used to 'search and kill' RILD. */
#define PRXY_RILD_PROCESS      "/system/bin/rild"
#define PRXY_RILD_SEARCH_DIR   "/proc/%d/cmdline"
#define PRXY_RILD_SEARCH_START (10)
#define PRXY_RILD_SEARCH_END   (200)
#define PRXY_RILD_KIL_CMD      "kill -9 %d"

#ifdef GAPP_DISABLE_GSM
typedef uint32 GSM_Id;
#endif

/*
 * Define media profile size. Currently only support two, one for audio and one
 * for audio+video.
 */
#define PRXY_MEDIA_PROFILE_SIZE        (2)
/* Define media string for audio and video. */
#define PRXY_MEDIA_PROFILE_STR_AUDIO   "m=audio"
#define PRXY_MEDIA_PROFILE_STR_VIDEO   "m=video"
#define PRXY_RTP_MAP_STR               "a=rtpmap:"
#define PRXY_FORMAT_PARAMETER_STR      "a=fmtp:"
#define PRXY_FRAMESIZE_PARAMETER_STR   "a=framesize:"
#define PRXY_FRAMERATE_PARAMETER_STR   "a=framerate:"


#define PRXY_MEDIA_DESC_STRING_SZ_MAX  (512)
#define PRXY_MEDIA_TOKEN_STRING_SZ_MAX (32)


/* Define emergency call numbers table. */
#define PRXY_EMERGENCY_NUMBER_SZ        (16)
#define PRXY_EMERGENCY_NUMBER_STR_SZ    (128)

/* Used for debug logging */
#ifndef PRXY_DEBUG
#define PRXY_dbgPrintf(x, ...)
#else
#define PRXY_dbgPrintf OSAL_logMsg
#endif

#define PRXY_AT_COMMAND_SIZE            (512)
#define PRXY_AT_COMMAND_TIMEOUT_SECS    (30)
#define PRXY_SMSC_ADDR_LENGTH_FIELD_SZ  (2)

/*
 * Public constants and structures for the PRXY sub module
 */
typedef enum {
    PRXY_RETURN_CONTINUE = -3,
    PRXY_RETURN_WAIT     = -2,
    PRXY_RETURN_FAILED   = -1,
    PRXY_RETURN_OK       = 0,
} PRXY_Return;

typedef enum {
    PRXY_SMS_STATE_NONE  = 0,
    PRXY_SMS_STATE_READ_PDU,
    PRXY_SMS_STATE_COMPLETE,
} PRXY_SmsState;

typedef enum {
    PRXY_SMS_PDU_MODE_TPDU,
    PRXY_SMS_PDU_MODE_RPDU,
    PRXY_SMS_PDU_MODE_TEXT,
} PRXY_SmsPduMode;

typedef struct {
    PRXY_SmsState   state;
    char            pdu[PRXY_AT_COMMAND_SIZE + 1];
    CSM_SmsType     smsType;
    PRXY_SmsPduMode pduMode;
} PRXY_Sms;

typedef enum {
    PRXY_NET_REG_STATE_INACTIVE = 0, // +CREG:0
    PRXY_NET_REG_STATE_ACTIVE,       // +CREG:1
    PRXY_NET_REG_STATE_IN_PROGRESS,  // +CREG:2
    PRXY_NET_REG_STATE_FAILED,       // +CREG:3
    PRXY_NET_REG_STATE_UNKNOWN,      // +CREG:4
    PRXY_NET_REG_STATE_ROAMING,      // +CREG:5
} PRXY_NetRegState;

typedef enum {
    PRXY_NULL_DOMAIN = 0,
    PRXY_CS_DOMAIN,
    PRXY_PS_DOMAIN
} PRXY_AccessDomain;

typedef struct {
    PRXY_NetRegState gsmState;
    PRXY_NetRegState csmState;
    OSAL_Boolean     isCsmRegChange;
    int              mode; // enable mode: n = 0, 1, 2
    char             locationAreaCode[PRXY_AT_COMMAND_SIZE + 1];
    char             cellId[PRXY_AT_COMMAND_SIZE + 1];
    int              isEmergency;
} PRXY_NetReg;

typedef struct {
    char    name[128];
    int     fid;
    uint32  sz;
} _PRXY_QParams;

typedef struct {
    CSM_InputEvent      csmCommand;
    uint32              passThruCommandId;
    uint32              nonPassThruCommandId;
    char                scratch[PRXY_AT_COMMAND_SIZE + 1];
    PRXY_Sms            sms;
    PRXY_NetReg         networkReg;
    OSAL_Boolean        atClccIssued;
    OSAL_Boolean        extDialCmdEnabled;
    CSM_CallSessionType mediaProfiles[PRXY_MEDIA_PROFILE_SIZE];
    /* Emergency numbers talbe. */
    char                emergencyNumbers[PRXY_EMERGENCY_NUMBER_SZ]
                                [PRXY_EMERGENCY_NUMBER_STR_SZ + 1];
} PRXY_CommandMngr;

/*
 * Public methods for the PRXY sub module
 */

PRXY_Return PRXY_getEventQueue(
    OSAL_MsgQId *queue_ptr);

PRXY_Return PRXY_processAtCommand(
    PRXY_CommandMngr *cmdMngr_ptr,
    char             *at_ptr);

PRXY_Return PRXY_processCsmOutputEvent(
    PRXY_CommandMngr *cmdMngr_ptr,
    CSM_OutputEvent     *response_ptr);

PRXY_Return PRXY_processGsmEvent(
    PRXY_CommandMngr *cmdMngr_ptr,
    uint32            gsmId,
    char             *result_ptr,
    vint              resultLen);

PRXY_AccessDomain _PRXY_getSupSrvAccessDomain(
    PRXY_CommandMngr *cmdMngr_ptr);

OSAL_Status PRXY_init(
    const char *proxyName_ptr);

void PRXY_destroy(
    void);

#endif

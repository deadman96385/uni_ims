/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29683 $ $Date: 2014-11-04 14:18:29 +0800 (Tue, 04 Nov 2014) $
 */

#ifndef __GSM_H__
#define __GSM_H__

#define GSM_EVENT_ARG_LEN      (512)

#define GSM_CMD_TO_SECS        (10)

#define GSM_SCRATCH_BUFFER_SZ  (64)

/* Define media profile index. */
#define GSM_MEDIA_PROFILE_ID_AUDIO         (1)
#define GSM_MEDIA_PROFILE_ID_VIDEO         (2)

/* Define media profile m-line string for audio and video. */
#define GSM_MEDIA_PROFILE_STR_AUDIO        ("m=audio")
#define GSM_MEDIA_PROFILE_STR_VIDEO        ("m=video")
#define GSM_MEDIA_PROFILE_STR_AUDIO_VIDEO  ("m=audio\\0D\\0Am=video")

typedef uint32 GSM_Id;

typedef enum {
    GSM_RETURN_TIMEOUT  = -4,
    GSM_RETURN_ERR      = -3,
    GSM_RETURN_DEV      = -2,
    GSM_RETURN_FAILED   = -1,
    GSM_RETURN_OK       = 0,
} GSM_Return;

typedef struct {
    const char *cmd_ptr;
    const char *result_ptr;
} GSM_Result;

typedef struct {
    GSM_Id  id;
    char    arg[GSM_EVENT_ARG_LEN + 1];
} GSM_Event;

GSM_Return GSM_init(
    const char *configFile_ptr,
    OSAL_Boolean extDialCmdEnabled);

GSM_Return GSM_destroy(void);

GSM_Return GSM_atCmd(
    const char *cmd_ptr,
    GSM_Id     *id_ptr,
    vint        timeoutMs);

GSM_Return GSM_getEvent(
    GSM_Event *event_ptr,
    vint       timeoutMs);

GSM_Return GSM_getEventQueue(
    OSAL_MsgQId *queueId_ptr);

GSM_Return GSM_cmdCallMute(void *arg_ptr, GSM_Id *gsmId_ptr, int mute);

GSM_Return GSM_cmdSendUssd(void *arg_ptr, GSM_Id *gsmId_ptr, char *to_ptr);

GSM_Return GSM_cmdIsim(void *arg_ptr, GSM_Id *gsmId_ptr, vint attribute,
        char *arg1_ptr, char *arg2_ptr);

GSM_Return GSM_cmdReadSmsMem(void *arg_ptr, GSM_Id *gsmId_ptr, char *number_ptr);

GSM_Return GSM_cmdDeleteSmsMem(void *arg_ptr, GSM_Id *gsmId_ptr, char *number_ptr);

GSM_Return GSM_cmdDisableCallForwarding(void *arg_ptr, GSM_Id *gsmId_ptr, int type);

GSM_Return GSM_cmdEnableCallForwardingNoReply(void *arg_ptr, GSM_Id *gsmId_ptr, int type, char* to_ptr, int timeout);

GSM_Return GSM_cmdEnableCallForwarding(void *arg_ptr, GSM_Id *gsmId_ptr, int type, char* to_ptr);

GSM_Return GSM_cmdDialDigit(void *arg_ptr, GSM_Id *gsmId_ptr, char digit, int duration);

GSM_Return GSM_cmdRegister(void *arg_ptr, GSM_Id *gsmId_ptr);

GSM_Return GSM_cmdDeregister(void *arg_ptr, GSM_Id *gsmId_ptr);

GSM_Return GSM_cmdSendSms(void *arg_ptr, GSM_Id *gsmId_ptr, char *to_ptr, char* msg_ptr, int requestReport);

GSM_Return GSM_cmdWriteSms(void *arg_ptr, GSM_Id *gsmId_ptr, char* msg_ptr);

GSM_Return GSM_cmdDial(void *arg_ptr, GSM_Id *gsmId_ptr, char *to_ptr, int blockId);

GSM_Return GSM_cmdHangup(void *arg_ptr, GSM_Id *gsmId_ptr);

GSM_Return GSM_cmdReject(void *arg_ptr, GSM_Id *gsmId_ptr);

GSM_Return GSM_cmdCallHoldZero(void *arg_ptr, GSM_Id *gsmId_ptr);

GSM_Return GSM_cmdCallHoldTwo(void *arg_ptr, GSM_Id *gsmId_ptr);

GSM_Return GSM_cmdCallHoldOneIndex(void *arg_ptr, GSM_Id *gsmId_ptr, int callIndex);

GSM_Return GSM_cmdCallHoldTwoIndex(void *arg_ptr, GSM_Id *gsmId_ptr, int callIndex);

GSM_Return GSM_cmdHoldThree(void *arg_ptr, GSM_Id *gsmId_ptr);

GSM_Return GSM_cmdCallAnswer(void *arg_ptr, GSM_Id *gsmId_ptr);

GSM_Return GSM_cmdForward(void *arg_ptr, GSM_Id *gsmId_ptr, char *to_ptr);

GSM_Return GSM_cmdBlindTransfer(void *arg_ptr, GSM_Id *gsmId_ptr, char *to_ptr);

GSM_Return GSM_cmdFmcDialDigitString(void *arg_ptr, GSM_Id *gsmId_ptr, char *dialSequence_ptr);

GSM_Return GSM_cmdFmcDialDigit(void *arg_ptr, GSM_Id *gsmId_ptr, char digit, int duration);

GSM_Return GSM_cmdFmcDial(void *arg_ptr, GSM_Id *gsmId_ptr, char *to_ptr);

GSM_Return GSM_cmdSetMediaProfiles(
    void   *arg_ptr,
    GSM_Id *gsmId_ptr);

GSM_Return GSM_cmdDialUri(
    void   *arg_ptr,
    GSM_Id *gsmId_ptr,
    char   *to_ptr,
    int     mpIdx);

GSM_Return GSM_cmdMediaControl(
    void   *arg_ptr,
    GSM_Id *gsmId_ptr,
    vint    callIdx,
    vint    negStatus,
    char   *sdpMd_ptr);
#endif

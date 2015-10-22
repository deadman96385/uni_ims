#ifndef SPRD_RIL_CB_H
#define SPRD_RIL_CB_H

#include <telephony/sprd_ril.h>
#include "sprd_atchannel.h"

#define NEW_AT
#ifdef NEW_AT
#define AT_PREFIX "+SP"
#else
#define AT_PREFIX "^"
#endif

extern struct ATChannels *ATch_type[MAX_CHANNELS];
extern RIL_RadioState sState;
extern int s_isstkcall;
extern int s_testmode;
extern const struct timeval TIMEVAL_CALLSTATEPOLL;

RIL_RadioState currentState();
int onSupports (int requestCode);
void onCancel (RIL_Token t);
const char * getVersion(void);
void setRadioState(int channelID, RIL_RadioState newState);
void setTestMode(int channelID);
RIL_AppType getSimType(int channelID);
void requestOemHookRaw(int channelID, void *data, size_t datalen, RIL_Token t);
void requestCallCsFallBackAccept(int channelID, void *data, size_t datalen, RIL_Token t);
void requestCallCsFallBackReject(int channelID, void *data, size_t datalen, RIL_Token t);
void stopQueryNetwork(int channelID, void *data, size_t datalen, RIL_Token t);
void requestSendAT(int channelID, char *data, size_t datalen, RIL_Token t);
void sendCallStateChanged(void *param);
void sendVideoCallStateChanged(void *param);
void process_calls(int _calls);
int callFromCLCCLine(char *line, RIL_Call *p_call);
int callFromCLCCLineVoLTE(char *line, RIL_Call_VoLTE *p_call);

#ifdef RIL_SHLIB
extern const struct RIL_Env *s_rilenv;
#define RIL_onRequestComplete(t, e, response, responselen) s_rilenv->OnRequestComplete(t,e, response, responselen)
#define RIL_onUnsolicitedResponse(a,b,c) s_rilenv->OnUnsolicitedResponse(a,b,c)
#define RIL_requestTimedCallback(a,b,c) s_rilenv->RequestTimedCallback(a,b,c)
#endif

#endif

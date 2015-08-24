#ifndef SPRD_RIL_CB_H
#define SPRD_RIL_CB_H

#include <telephony/sprd_ril.h>
#include "sprd_atchannel.h"

extern struct ATChannels *ATch_type[MAX_CHANNELS];

RIL_RadioState currentState();
int onSupports (int requestCode);
void onCancel (RIL_Token t);
const char * getVersion(void);

#ifdef RIL_SHLIB
extern const struct RIL_Env *s_rilenv;
#define RIL_onRequestComplete(t, e, response, responselen) s_rilenv->OnRequestComplete(t,e, response, responselen)
#define RIL_onUnsolicitedResponse(a,b,c) s_rilenv->OnUnsolicitedResponse(a,b,c)
#define RIL_requestTimedCallback(a,b,c) s_rilenv->RequestTimedCallback(a,b,c)
#endif

#endif

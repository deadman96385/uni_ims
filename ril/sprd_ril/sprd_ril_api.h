#ifndef SPRD_RIL_API_H
#define SPRD_RIL_API_H

#include "sprd_atchannel.h"
#include "at_tok.h"
#include "misc.h"
#include <telephony/sprd_ril.h>

extern void RIL_onRequestComplete(RIL_Token t, RIL_Errno e,
                                void *response, size_t responselen);
extern void RIL_onUnsolicitedResponse(int unsolResponse, const void *data,
                                size_t datalen);
extern void RIL_requestTimedCallback (RIL_TimedCallback callback,
                                void *param, const struct timeval *relativeTime);
extern void RIL_removeTimedCallback(void *callbackInfo);

extern void putChannel(int channel);

extern int getChannel();

extern struct ATChannels *ATch_type[MAX_CHANNELS];

typedef enum {
	BANDGSM900	= 0,
	BANDDCS1800	= 1,
	BANDPCS1900	= 2,
	BANDGSM850	= 3,
	BANDGSM900_DCS1800	= 4,
	BANDGSM850_GSM900	= 5,
	BANDGSM850_DCS1800	= 6,
	BANDGSM850_PCS1900	= 7,
	BANDGSM900_PCS1900	= 8,
	BANDGSM850_GSM900_DCS1800	= 9,
	BANDGSM850_GSM900_PCS1900	= 10,
	BANDDCS1800_PCS1900	= 11,
	BANDGSM850_DCS1800_PCS1900	= 12,
	BANDGSM900_DCS1800_PCS1900	= 13,
	BANDALLGSM = 14,
	BANDINVALID = -1,
} BAND_TYPE_T;

void sprd_DataSwitch(int request, void *data, size_t datalen, RIL_Token t);
void sprd_IMEISimOut(int request, void *data, size_t datalen, RIL_Token t);
int sprd_BandSelect(BAND_TYPE_T band_type);
BAND_TYPE_T sprd_GetCurrentBand(void);
int sprd_DebugScreen(char **testbuffer);

#endif

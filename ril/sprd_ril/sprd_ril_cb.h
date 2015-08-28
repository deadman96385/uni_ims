#ifndef SPRD_RIL_CB_H
#define SPRD_RIL_CB_H

#include <telephony/sprd_ril.h>


RIL_RadioState currentState();
int onSupports (int requestCode);
void onCancel (RIL_Token t);
const char * getVersion(void);


extern struct ATChannels *ATch_type[MAX_CHANNELS];



#endif

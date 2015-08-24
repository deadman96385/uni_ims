#if defined (RIL_SUPPORT_CALL_BLACKLIST)


#ifndef RIL_CALL_BLACKLIST_H
#define RIL_CALL_BLACKLIST_H

#include <telephony/sprd_ril.h>

typedef struct blacklistnode{
    char *phonenumber;
    struct blacklistnode *next;
}blacklistnode;

int queryBlackList (int type, char *phonenumber);
void onCallCsFallBackAccept(void *param);
void requestCallBlackList(void *data, size_t datalen, RIL_Token t);

#endif


#endif

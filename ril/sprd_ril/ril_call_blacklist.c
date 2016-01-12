#if defined (RIL_SUPPORT_CALL_BLACKLIST)


#ifndef RIL_CALL_BLACKLIST_C
#define RIL_CALL_BLACKLIST_C

#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <alloca.h>
#include <string.h>
#include <stdlib.h>
#include <utils/Log.h>
#include "sprd_atchannel.h"
#include "ril_call_blacklist.h"
#include "sprd_ril_cb.h"
#include <cutils/properties.h>

#define LOG_TAG "RIL"
#define PROP_BUILD_TYPE "ro.build.type"

/*represent if blacklist is empty, 0--empty, 1--hava black number*/
static int s_blacklist = 0;
static int minMatch;
extern int s_isuserdebug;

blacklistnode *voice_black_list = NULL; // for voice call

pthread_mutex_t s_blackListMutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * SPRD: malloc memory for head of blacklist
 */
static void black_list_init(blacklistnode ** black_list)
{
    *black_list = (blacklistnode *)malloc(sizeof(blacklistnode));
    if (*black_list == NULL)
        RILLOGE("Malloc memory fail!");
    else
        (*black_list)->next = NULL;
}

/*
 * SPRD: free memory of blacklist, of which head node does not store a phonenumber
 */
static void free_list_memory(blacklistnode * head)
{
    if (head == NULL) {
        return;
    }

    blacklistnode *p1 = head;
    blacklistnode *tmp = NULL;

    tmp = p1->next;
    free(p1);
    p1 = tmp;

    while(p1)
    {
        tmp = p1->next;
        if ( p1->phonenumber != NULL) {
            free(p1->phonenumber);
        }
        free(p1);
        p1 = tmp;
    }
}

/*
 * SPRD: add for reverse a phonenumber
 */
char * revstr (char * string)
{
    char *start = string;
    char *left = string;
    char ch;

//    while (*string++)
//        ;
//    string -= 2;
//    }

    int len = strlen(string);
    string += len - 1;// point to the last char before "\0"

    while (left < string)
    {
        ch = *left;
        *left++ = *string;
        *string-- = ch;
    }
    return(start);
}

/*
 * SPRD: Exact Match phonenumber with blacknumber after minMatch
 */
static int isMatchedNumber(char *phonenumber, char *blacknumber, int datalen)
{
    int i;
    for(i = 0; i < datalen; i++) {
        if (phonenumber[i] != blacknumber[i])
            return 0;
    }
    return 1;
}

/*
 * SPRD: traverse blacklist to match the phonenumber with blacknumber of blacklist
 */
static int checkIsBlackNumber(char *phonenumber, blacklistnode *blacklist)
{
    if (blacklist == NULL || blacklist->next == NULL) {
        RILLOGD("Wrong blacklist\n");
        return 0;
    }

    int index =0;
    char *blacknumber;
    int blacknumLen, phonenumLen;
    blacklistnode *item = blacklist->next;

    phonenumLen = strlen(phonenumber);
    if(phonenumLen > minMatch) {
        index = phonenumLen - minMatch;
        phonenumLen = minMatch;
    }
    if (s_isuserdebug) {
        RILLOGD("phonenumber=%s, phonenumLen=%d, index=%d\n", phonenumber, phonenumLen, index);
    }

    while (item != NULL)
    {
        blacknumber = item->phonenumber;
        blacknumLen = strlen(blacknumber);

        if (phonenumLen == blacknumLen)
            if ( isMatchedNumber(phonenumber + index, blacknumber, phonenumLen) )
                return 1;

        item = item->next;
    }
    return 0;
}

static void onBlackCallHangup(void *param) {
    int channelID;
    char buf[125] = {0};

    channelID = getChannel();
    sprintf(buf,"AT+CHLD=0");
    at_send_command(ATch_type[channelID], buf, NULL);
    putChannel(channelID);
}

void onCallCsFallBackAccept(void *param) {
    int channelID;
    char buf[125] = {0};

    channelID = getChannel();
    sprintf(buf, "AT+SCSFB=1,1");
    at_send_command(ATch_type[channelID], buf, NULL);
    putChannel(channelID);
}

/*
 * SPRD: add for creat blacklist,
 * when phone reboot, add black phonenumbers or delete phonenumbers
 */
static void creatBlackList(void *data, size_t datalen)
{
    int numberLen = 0;
    char *tmp = NULL;
    char *type = NULL;
    char *number = NULL;
    char *blackList = NULL;
    char *payload = (char*)data;

    if(payload == NULL) {
        RILLOGE("Invalid blacklist\n");
        return;
    }

    int len = strlen(payload) + 1;
    blackList = (char*)alloca(sizeof(char) * len);
    memcpy(blackList, payload, len);

    black_list_init(&voice_black_list);
    if (voice_black_list == NULL)
        return;

    blacklistnode *p1 = NULL;
    blacklistnode *p2 = voice_black_list;

    while (blackList != NULL) {
        type = strsep(&blackList, ",");
        if (blackList != NULL) {
            number = strsep(&blackList, ",");
            numberLen = strlen(number) + 1;
            p1 = (blacklistnode *) malloc(sizeof(blacklistnode));
            p1->phonenumber = (char *) malloc (numberLen*sizeof(char));

            if (!strcmp(type, "0")) { //type == 0, represent voice black number
                tmp = revstr(number);
                strcpy(p1->phonenumber, tmp);
                p1->next = NULL;
                p2->next = p1;
                p2 = p1;
            }
        }
    }
}

#if defined (RIL_SUPPORTED_OEM_PROTOBUF)
void requestCallBlackList(void *data, size_t datalen, RIL_Token t) {
    OemRequest *blackListReq = (OemRequest *)data;
    RILLOGD("Blacklist subFuncID : %d", blackListReq->subFuncId);
    switch (blackListReq->subFuncId) {
        case OEM_REQ_SUBFUNCID_MINMATCH :
            {
                pthread_mutex_lock(&s_blackListMutex);
                minMatch = ((int*)(blackListReq->payload))[0];
                RILLOGD("minMatch = %d\n",minMatch);
                if (minMatch == 7 || minMatch ==11) {
                    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                } else {
                    RILLOGD("Invalid minMatch\n");
                    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                }
                pthread_mutex_unlock(&s_blackListMutex);
                break;
            }
        case OEM_REQ_SUBFUNCID_BLACKLIST :
            {
                 pthread_mutex_lock(&s_blackListMutex);
                 free_list_memory(voice_black_list);
                 voice_black_list = NULL;
                 if ( datalen == 0 ) {
                     RILLOGD("Blacklist is empty\n");
                     s_blacklist = 0;
                     RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                 } else if ( datalen > 0 && ( minMatch == 7 || minMatch == 11)) {
                     s_blacklist = 1;
                     creatBlackList(blackListReq->payload, datalen);
                     RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
                 } else {
                     RILLOGD("Wrong type of blacklist\n");
                     RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
                 }
                 pthread_mutex_unlock(&s_blackListMutex);
                 break;
            }
        default :
            RILLOGD("Not supported call blacklist subFuncId: %d", blackListReq->subFuncId);
            RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
            break;
    }
}
#endif

/*
 * SPRD: check if the phonenumber is in blacklist,
 * if in, report the record to firewallcall
 */
int queryBlackList (int type, char *phonenumber)
{
    int ret = 0;
    char versionStr[PROPERTY_VALUE_MAX];
    pthread_mutex_lock(&s_blackListMutex);
    property_get(PROP_BUILD_TYPE, versionStr, "user");
    if(strstr(versionStr, "userdebug")) {
        s_isuserdebug = 1;
    }
    if (s_blacklist == 0) {
        goto EXIT;
    } else if (s_blacklist == 1 && type == 0 && phonenumber != NULL) {
        ret = checkIsBlackNumber(phonenumber, voice_black_list);
    }
    RILLOGD("The incoming call is in Blacklist = %d\n", ret);
    if (ret == 1) {
        RIL_requestTimedCallback(onBlackCallHangup, NULL, NULL);

        RIL_OEM_NOTIFY *black_response = (RIL_OEM_NOTIFY *) alloca(sizeof(RIL_OEM_NOTIFY));
        char *black_call = NULL;
        asprintf(&black_call, "%d%s", type, phonenumber);

        black_response->oemFuncId = OEM_UNSOL_FUNCTION_ID_BLOCKCALLS;
        black_response->data = black_call;

        char *resp = NULL;
        asprintf(&resp, "%d%s", black_response->oemFuncId, black_response->data);
        RIL_onUnsolicitedResponse (RIL_UNSOL_OEM_HOOK_RAW, resp, strlen(resp));
        if (s_isuserdebug) {
            RILLOGD("RIL_UNSOL_OEM_HOOK_RAW, oemFuncId: %d, data: %s",
                    black_response->oemFuncId, black_response->data);
        }
        free(black_call);
        free(resp);
    }
EXIT:
    pthread_mutex_unlock(&s_blackListMutex);
    return ret;
}

#endif


#endif

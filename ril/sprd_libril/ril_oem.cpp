#if defined (RIL_SPRD_EXTENSION)
#define LOG_TAG "RILC"

#include <cutils/jstring.h>
#include <utils/Log.h>
#include <binder/Parcel.h>
#include <stdlib.h>
#include <assert.h>
#include <alloca.h>

#include <ril_ex.h>
#include <telephony/sprd_ril.h>
#include <vendor/sprd/open-source/libs/libril_oem/proto/ril_oem.pb.h>

using namespace android;

static void dispatchVoid (OemMsg *msg, RequestInfo *pRI);
static void dispatchInts (OemMsg *msg, RequestInfo *pRI);
static void dispatchString (OemMsg *msg, RequestInfo *pRI);
static void dispatchStrings (OemMsg *msg, RequestInfo *pRI);
static void dispatchCallBlack (OemMsg *msg, RequestInfo *pRI);
static void dispatchVideoPhone (OemMsg *msg, RequestInfo *pRI);
static void dispatchMBBMS (OemMsg *msg, RequestInfo *pRI);
static void dispatchPrioNet (OemMsg *msg, RequestInfo *pRI);
static void dispatchIMS (OemMsg *msg, RequestInfo *pRI);
static void dispatchVolte (OemMsg *msg, RequestInfo *pRI);
static void dispatchSimlock (OemMsg *msg, RequestInfo *pRI);

static int responseVoid(OemMsg *msg, void *response, size_t responselen);
static int responseInts(OemMsg *msg, void *response, size_t responselen);
static int responseString(OemMsg *msg, void *response, size_t responselen);
static int responseStrings(OemMsg *msg, void *response, size_t responselen);
static int responseVideoPhone(OemMsg *msg, void *response, size_t responselen);
static int responsePrioNet(OemMsg *msg, void *response, size_t responselen);
static int responseIMS(OemMsg *msg, void *response, size_t responselen);
static int responseVolte(OemMsg *msg, void *response, size_t responselen);
static int responseSimlock(OemMsg *msg, void *response, size_t responselen);

typedef struct {
    int requestNumber;
    void (*dispatchFunction) (OemMsg *msg, struct RequestInfo *pRI);
    int(*responseFunction) (OemMsg *msg, void *response, size_t responselen);
} CommandInfoOem;

static CommandInfoOem s_oem_commands[] = {
#include "sprd_ril_oem_commands.h"
};

extern "C" void dispatchRawSprd(Parcel &p, RequestInfo *pRI) {
    int32_t len;
    status_t status;
    const void *data;
    OemMsg *oemData = new OemMsg();
    CommandInfoOem *pCIO;
    int funcId;

    status = p.readInt32(&len);
    if (status != NO_ERROR) {
        goto invalid;
    }

    // The java code writes -1 for null arrays
    if (((int)len) == -1) {
        data = NULL;
        len = 0;
    }

    data = p.readInplace(len);

    startRequest;
    appendPrintBuf("%sraw_size=%d", printBuf, len);
    closeRequest;
    printRequest(pRI->token, pRI->pCI->requestNumber);

    oemData->ParseFromArray(data, len);

    funcId = oemData->funcid();
    if (FuncId_IsValid(funcId) == 0 && funcId != 0) {
        goto invalid;
    }

    pCIO = &(s_oem_commands[funcId]);

    pCIO->dispatchFunction(oemData, pRI);

    delete oemData;
    return;
invalid:
    delete oemData;
    invalidCommandBlock(pRI);
    return;
}

extern "C" int responseRawSprd(Parcel &p, void *response, size_t responselen) {
    if (response == NULL && responselen != 0) {
        RILLOGE("invalid response: NULL with responselen != 0");
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    // The java code reads -1 size as null byte array
    if (response == NULL) {
        p.writeInt32(-1);
    } else {
        OemMsg *msg = new OemMsg();
        OemRequest *resp = (OemRequest*)response;
        CommandInfoOem *pCI = &(s_oem_commands[resp->funcId]);
        int ret = pCI->responseFunction(msg, response, responselen);
        if (ret != 0) {
            return ret;
        }
        int length = msg->ByteSize();
        char* buf = new char[length];
        msg->SerializeToArray(buf,length);

        p.writeInt32(length);
        p.write(buf, length);
        delete buf;
        delete msg;
    }
    return 0;
}

/** Callee expects NULL */
static void dispatchVoid (OemMsg *msg, RequestInfo *pRI) {
    clearPrintBuf;
    printRequest(pRI->token, pRI->pCI->requestNumber);
    OemRequest *req = (OemRequest*)alloca(sizeof(OemRequest));
    req->funcId = msg->funcid();
    if (msg->has_subfuncid()) {
        req->subFuncId = msg->subfuncid();
    }
    s_callbacks.onRequest(pRI->pCI->requestNumber, req, 0, pRI);
}

static void dispatchInts (OemMsg *msg, RequestInfo *pRI) {
    int32_t count;
    size_t datalen;
    int *pInts;
    OemRequest *req = (OemRequest*)alloca(sizeof(OemRequest));

    count = msg->intpayload_size();
    if (count == 0) {
        goto invalid;
    }

    datalen = sizeof(int) * count;
    pInts = (int *)alloca(datalen);

    startRequest;
    for (int i = 0 ; i < count ; i++) {
        pInts[i] = (int)msg->intpayload(i);
        appendPrintBuf("%s%d,", printBuf, pInts[i]);
   }
   removeLastChar;
   closeRequest;
   printRequest(pRI->token, pRI->pCI->requestNumber);

   req->funcId = msg->funcid();
   if (msg->has_subfuncid()) {
       req->subFuncId = msg->subfuncid();
   }
   req->payload = (void*)pInts;

   s_callbacks.onRequest(pRI->pCI->requestNumber, req,
                       datalen, pRI);

#ifdef MEMSET_FREED
    memset(pInts, 0, datalen);
#endif
    return;

invalid:
    invalidCommandBlock(pRI);
    return;
}

/** Callee expects const char * */
static void dispatchString (OemMsg *msg, RequestInfo *pRI) {
    size_t datalen;
    size_t stringlen;
    char *string8 = NULL;

    string8 = (char*)msg->strpayload(0).data();

    startRequest;
    appendPrintBuf("%s%s", printBuf, string8);
    closeRequest;
    printRequest(pRI->token, pRI->pCI->requestNumber);

    OemRequest *req = (OemRequest*)alloca(sizeof(OemRequest));
    req->funcId = msg->funcid();
    if (msg->has_subfuncid()) {
        req->subFuncId = msg->subfuncid();
    }
    req->payload = (void*)string8;

    s_callbacks.onRequest(pRI->pCI->requestNumber, req,
                       strlen(string8)+1, pRI);
    return;
}

/** Callee expects const char ** */
static void dispatchStrings (OemMsg *msg, RequestInfo *pRI) {
    int32_t countStrings;
    size_t datalen;
    char **pStrings;

    countStrings = msg->strpayload_size();

    startRequest;
    if (countStrings == 0) {
        // just some non-null pointer
        pStrings = (char **)alloca(sizeof(char *));
        datalen = 0;
    } else if (((int)countStrings) == -1) {
        pStrings = NULL;
        datalen = 0;
    } else {
        datalen = sizeof(char *) * countStrings;

        pStrings = (char **)alloca(datalen);

        for (int i = 0 ; i < countStrings ; i++) {
            pStrings[i] = (char*)msg->strpayload(i).data();
            appendPrintBuf("%s%s,", printBuf, pStrings[i]);
        }
    }
    removeLastChar;
    closeRequest;
    printRequest(pRI->token, pRI->pCI->requestNumber);

    OemRequest *req = (OemRequest*)alloca(sizeof(OemRequest));
    req->funcId = msg->funcid();
    if (msg->has_subfuncid()) {
        req->subFuncId = msg->subfuncid();
    }
    req->payload = (void*)pStrings;

    s_callbacks.onRequest(pRI->pCI->requestNumber, req, datalen, pRI);

    if (pStrings != NULL) {
#ifdef MEMSET_FREED
        memset(pStrings, 0, datalen);
#endif
    }
    return;

invalid:
    invalidCommandBlock(pRI);
    return;
}

static void dispatchCallBlack (OemMsg *msg, RequestInfo *pRI) {
    if (msg->has_subfuncid()) {
        switch (msg->subfuncid()) {
            case OEM_REQ_SUBFUNCID_MINMATCH :
                dispatchInts(msg, pRI);
                break;
            case OEM_REQ_SUBFUNCID_BLACKLIST :
                dispatchString(msg, pRI);
                break;
            default :
                RILLOGD("Not Supported Blacklist Request!");
                goto invalid;
        }
    } else {
        goto invalid;
    }
    return;

invalid:
    invalidCommandBlock(pRI);
    return;
}


static void dispatchVideoPhoneDial(OemMsg *msg, RequestInfo *pRI){
    RIL_VideoPhone_Dial dial;
    OemRequest *req = (OemRequest*)alloca(sizeof(OemRequest));

    memset (&dial, 0, sizeof(dial));

    if (msg->strpayload_size() == 0) {
        goto invalid;
    }
    dial.address = (char*)(msg->strpayload(0).data());
    dial.sub_address = (char*)(msg->strpayload(1).data());

    dial.clir = msg->intpayload(0);

    startRequest;
    appendPrintBuf("%saddress=%s,sub_address=%s,clir=%d", printBuf, dial.address, dial.sub_address, dial.clir);
    closeRequest;
    printRequest(pRI->token, pRI->pCI->requestNumber);

    req->funcId = msg->funcid();
    req->subFuncId = msg->subfuncid();
    req->payload = (void*)&dial;

    s_callbacks.onRequest(pRI->pCI->requestNumber, req, sizeof(dial), pRI);

    return;

invalid:
    invalidCommandBlock(pRI);
    return;
}

static void dispatchVideoPhoneCodec(OemMsg *msg, RequestInfo *pRI){
    RIL_VideoPhone_Codec codec;
    OemRequest *req = (OemRequest*)alloca(sizeof(OemRequest));

    memset (&codec, 0, sizeof(codec));
    if (msg->intpayload_size() == 0) {
        goto invalid;
    }

    codec.type = msg->intpayload(0);

    startRequest;
    appendPrintBuf("%stype=%d", printBuf, codec.type);
    closeRequest;
    printRequest(pRI->token, pRI->pCI->requestNumber);

    req->funcId = msg->funcid();
    req->subFuncId = msg->subfuncid();
    req->payload = (void*)&codec;

    s_callbacks.onRequest(pRI->pCI->requestNumber, req, sizeof(codec), pRI);

    return;

invalid:
    invalidCommandBlock(pRI);
    return;
}

static void dispatchVideoPhone (OemMsg *msg, RequestInfo *pRI) {
    if (msg->has_subfuncid()) {
        switch (msg->subfuncid()) {
            case OEM_REQ_SUBFUNCID_VIDEOPHONE_DIAL :
                dispatchVideoPhoneDial(msg, pRI);
                break;
            case OEM_REQ_SUBFUNCID_VIDEOPHONE_CODEC :
                dispatchVideoPhoneCodec(msg, pRI);
                break;
            case OEM_REQ_SUBFUNCID_VIDEOPHONE_HANGUP :
            case OEM_REQ_SUBFUNCID_VIDEOPHONE_LOCAL_MEDIA :
            case OEM_REQ_SUBFUNCID_VIDEOPHONE_RECORD_VIDEO :
            case OEM_REQ_SUBFUNCID_VIDEOPHONE_RECORD_AUDIO :
            case OEM_REQ_SUBFUNCID_VIDEOPHONE_SET_VOICERECORDTYPE :
            case OEM_REQ_SUBFUNCID_VIDEOPHONE_TEST :
            case OEM_REQ_SUBFUNCID_VIDEOPHONE_CONTROL_AUDIO :
            case OEM_REQ_SUBFUNCID_VIDEOPHONE_CONTROL_IFRAME :
                dispatchInts(msg, pRI);
                break;
            case OEM_REQ_SUBFUNCID_VIDEOPHONE_ANSWER :
            case OEM_REQ_SUBFUNCID_VIDEOPHONE_FALLBACK :
            case OEM_REQ_SUBFUNCID_VIDEOPHONE_GET_CURRENT_VIDEOCALLS :
                dispatchVoid(msg, pRI);
                break;
            case OEM_REQ_SUBFUNCID_VIDEOPHONE_STRING :
                dispatchString(msg, pRI);
                break;
            default :
                RILLOGD("Not Supported VideoPhone Request!");
                goto invalid;
        }
    } else {
        goto invalid;
    }
    return;

invalid:
    invalidCommandBlock(pRI);
    return;
}

static void dispatchMBBMS (OemMsg *msg, RequestInfo *pRI) {
    if (msg->has_subfuncid()) {
        switch (msg->subfuncid()) {
            case OEM_REQ_SUBFUNCID_MBBMS_GSM_AUTHEN :
                dispatchString(msg, pRI);
                break;
            case OEM_REQ_SUBFUNCID_MBBMS_USIM_AUTHEN :
                dispatchStrings(msg, pRI);
                break;
            case OEM_REQ_SUBFUNCID_MBBMS_SIM_TYPE :
                dispatchVoid(msg, pRI);
                break;
            default :
                RILLOGD("Not Supported MBBMS Request!");
                goto invalid;
        }
    } else {
       goto invalid;
    }
   return;

invalid:
   invalidCommandBlock(pRI);
   return;
}

static void dispatchPrioNet(OemMsg *msg, RequestInfo *pRI) {
    if (msg->has_subfuncid()) {
        switch (msg->subfuncid()) {
            case OEM_REQ_SUBFUNCID_SET_PRIORITY_NETWORK_MODE :
                dispatchInts(msg, pRI);
                break;
            case OEM_REQ_SUBFUNCID_GET_PRIORITY_NETWORK_MODE :
                dispatchVoid(msg, pRI);
                break;
            default :
                RILLOGD("Not Supported Priority Network Request!");
                goto invalid;
        }
    } else {
       goto invalid;
    }
   return;

invalid:
   invalidCommandBlock(pRI);
   return;
}

static void dispatchSetInitialAttachApn(OemMsg *msg, RequestInfo *pRI)
{
    RIL_InitialAttachApn pf;
    int32_t  t;
    status_t status;

    memset(&pf, 0, sizeof(pf));

    pf.apn = (char*)(msg->strpayload(0).data());
    pf.protocol = (char*)(msg->strpayload(1).data());

    pf.authtype = msg->intpayload(0);

    pf.username = (char*)(msg->strpayload(2).data());
    pf.password = (char*)(msg->strpayload(3).data());

    startRequest;
    appendPrintBuf("%sapn=%s, protocol=%s, authtype=%d, username=%s, password=%s",
            printBuf, pf.apn, pf.protocol, pf.authtype, pf.username, pf.password);
    closeRequest;
    printRequest(pRI->token, pRI->pCI->requestNumber);

    OemRequest *req = (OemRequest*)alloca(sizeof(OemRequest));
    req->funcId = msg->funcid();
    req->subFuncId = msg->subfuncid();
    req->payload = (void*)(&pf);

    s_callbacks.onRequest(pRI->pCI->requestNumber, req, sizeof(pf), pRI);

    return;
}

static void dispatchIMS (OemMsg *msg, RequestInfo *pRI) {
    if (msg->has_subfuncid()) {
        switch (msg->subfuncid()) {
            case OEM_REQ_SUBFUNCID_GET_IMS_VOICE_CALL_AVAILABILITY :
            case OEM_REQ_SUBFUNCID_DISABLE_IMS :
                dispatchVoid(msg, pRI);
                break;
            case OEM_REQ_SUBFUNCID_SET_IMS_VOICE_CALL_AVAILABILITY :
                dispatchInts(msg, pRI);
                break;
            case OEM_REQ_SUBFUNCID_REGISTER_IMS_IMPU :
            case OEM_REQ_SUBFUNCID_REGISTER_IMS_IMPI :
            case OEM_REQ_SUBFUNCID_REGISTER_IMS_DOMAIN :
            case OEM_REQ_SUBFUNCID_REGISTER_IMS_IMEI :
            case OEM_REQ_SUBFUNCID_REGISTER_IMS_XCAP :
            case OEM_REQ_SUBFUNCID_REGISTER_IMS_BSF :
            case OEM_REQ_SUBFUNCID_SET_IMS_SMSC :
                dispatchString(msg, pRI);
                break;
            case OEM_REQ_SUBFUNCID_SET_INITIAL_ATTACH_IMS_APN :
                dispatchSetInitialAttachApn(msg, pRI);
                break;
            default :
                RILLOGD("Not Supported IMS Request!");
                goto invalid;
        }
    } else {
       goto invalid;
    }
   return;

invalid:
   invalidCommandBlock(pRI);
   return;
}

static void dispatchVolte (OemMsg *msg, RequestInfo *pRI) {
    if (msg->has_subfuncid()) {
        switch (msg->subfuncid()) {
            case OEM_REQ_SUBFUNCID_VOLTE_CALL_FALL_BACK_TO_VOICE :
            case OEM_REQ_SUBFUNCID_GET_CURRENT_CALLS_VOLTE :
                dispatchVoid(msg, pRI);
                break;
            case OEM_REQ_SUBFUNCID_VOLTE_CALL_REQUEST_MEDIA_CHANGE :
            case OEM_REQ_SUBFUNCID_VOLTE_CALL_RESPONSE_MEDIA_CHANGE :
                dispatchInts(msg, pRI);
                break;
            case OEM_REQ_SUBFUNCID_VOLTE_INITIAL_GROUP_CALL :
            case OEM_REQ_SUBFUNCID_VOLTE_ADD_TO_GROUP_CALL :
            case OEM_REQ_SUBFUNCID_VOLTE_SET_CONFERENCE_URI :
                dispatchString(msg, pRI);
                break;
            default :
                RILLOGD("Not Supported Volte Request!");
                goto invalid;
        }
    } else {
       goto invalid;
    }
   return;

invalid:
   invalidCommandBlock(pRI);
   return;
}

static void dispatchSimlock (OemMsg *msg, RequestInfo *pRI) {
    if (msg->has_subfuncid()) {
        switch (msg->subfuncid()) {
            case OEM_REQ_SUBFUNCID_GET_SIMLOCK_DUMMYS :
                dispatchVoid(msg, pRI);
                break;
            case OEM_REQ_SUBFUNCID_GET_SIMLOCK_REMAIN_TIMES :
            case OEM_REQ_SUBFUNCID_GET_SIMLOCK_STATUS :
            case OEM_REQ_SUBFUNCID_GET_SIMLOCK_WHITELIST :
                dispatchInts(msg, pRI);
                break;
            default :
                RILLOGD("Not Supported Simlock Request!");
                goto invalid;
        }
    } else {
       goto invalid;
    }
   return;

invalid:
   invalidCommandBlock(pRI);
   return;
}



static int responseVoid(OemMsg *msg, void *response, size_t responselen) {
    startResponse;
    removeLastChar;
    return 0;
}


/** response is an int* pointing to an array of ints*/
static int
responseInts(OemMsg *msg, void *response, size_t responselen) {
    if (responselen % sizeof(int) != 0) {
        RILLOGE("responseInts: invalid response length %d expected multiple of %d\n",
            (int)responselen, (int)sizeof(int));
        return RIL_ERRNO_INVALID_RESPONSE;
    }
    int numInts;
    OemRequest *resp = (OemRequest*)response;
    int *p_int = (int *) (resp->payload);

    numInts = responselen / sizeof(int);

    /* each int*/
    startResponse;
    for (int i = 0 ; i < numInts ; i++) {
        appendPrintBuf("%s%d,", printBuf, p_int[i]);
        msg->add_intpayload(p_int[i]);
    }
    removeLastChar;
    closeResponse;

    return 0;
}

/**
 * NULL strings are accepted
 * FIXME currently ignores responselen
 */
static int responseString(OemMsg *msg, void *response, size_t responselen) {
    /* one string only */
    startResponse;
    OemRequest *resp = (OemRequest*)response;
    msg->add_strpayload((char*)(resp->payload));
    appendPrintBuf("%s%s", printBuf, (char*)(resp->payload));
    closeResponse;
    return 0;
}

/* response is a char **, pointing to an array of char *'s */
static int responseStrings(OemMsg *msg, void *response, size_t responselen) {
    if (responselen % sizeof(char *) != 0) {
        RILLOGE("responseStrings: invalid response length %d expected multiple of %d\n",
            (int)responselen, (int)sizeof(char *));
        return RIL_ERRNO_INVALID_RESPONSE;
    }
    int numStrings;
    OemRequest *resp = (OemRequest*)response;
    char **p_cur = (char **) (resp->payload);
    numStrings = responselen / sizeof(char *);

    /* each string*/
    startResponse;
    for (int i = 0 ; i < numStrings ; i++) {
        appendPrintBuf("%s%s,", printBuf, (char*)p_cur[i]);
        msg->add_strpayload(p_cur[i]);
    }
    removeLastChar;
    closeResponse;

    return 0;
}

static int responseCallList(OemMsg *msg, void *response, size_t responselen) {
    if (responselen % sizeof (RIL_Call *) != 0) {
        RILLOGE("responseCallList: invalid response length %d expected multiple of %d\n",
            (int)responselen, (int)sizeof (RIL_Call *));
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    int num;
    startResponse;
    /* number of call info's */
    num = responselen / sizeof(RIL_Call *);
    msg->add_intpayload(num);

    OemRequest *resp = (OemRequest*)response;
    RIL_Call **pp_calls = ((RIL_Call **)(resp->payload));

    for (int i = 0 ; i < num ; i++) {
        RIL_Call *p_cur = pp_calls[i];
        /* each call info */
        msg->add_intpayload(p_cur->state);
        msg->add_intpayload(p_cur->index);
        msg->add_intpayload(p_cur->toa);
        msg->add_intpayload(p_cur->isMpty);
        msg->add_intpayload(p_cur->isMT);
        msg->add_intpayload(p_cur->als);
        msg->add_intpayload(p_cur->isVoice);
        msg->add_intpayload(p_cur->isVoicePrivacy);
        msg->add_intpayload(p_cur->numberPresentation);
        msg->add_intpayload(p_cur->namePresentation);
        msg->add_strpayload(p_cur->number);
        msg->add_strpayload(p_cur->name);
        // Remove when partners upgrade to version 3
        if ((s_callbacks.version < 3) || (p_cur->uusInfo == NULL || p_cur->uusInfo->uusData == NULL)) {
            msg->add_intpayload(0);/* UUS Information is absent */
        } else {
            RIL_UUS_Info *uusInfo = p_cur->uusInfo;
            msg->add_intpayload(1);/* UUS Information is present */
            msg->add_intpayload(uusInfo->uusType);
            msg->add_intpayload(uusInfo->uusDcs);
            msg->add_intpayload(uusInfo->uusLength);
            msg->add_strpayload(uusInfo->uusData);
        }
        appendPrintBuf("%s[id=%d,%s,toa=%d,",
            printBuf,
            p_cur->index,
            callStateToString(p_cur->state),
            p_cur->toa);
        appendPrintBuf("%s%s,%s,als=%d,%s,%s,",
            printBuf,
            (p_cur->isMpty)?"conf":"norm",
            (p_cur->isMT)?"mt":"mo",
            p_cur->als,
            (p_cur->isVoice)?"voc":"nonvoc",
            (p_cur->isVoicePrivacy)?"evp":"noevp");
        appendPrintBuf("%s%s,cli=%d,name='%s',%d]",
            printBuf,
            p_cur->number,
            p_cur->numberPresentation,
            p_cur->name,
            p_cur->namePresentation);
    }
    removeLastChar;
    closeResponse;

    return 0;
}

static int responseVideoPhone(OemMsg *msg, void *response, size_t responselen) {
    OemRequest *resp = (OemRequest*)response;
    switch (resp->subFuncId) {
        case OEM_REQ_SUBFUNCID_VIDEOPHONE_GET_CURRENT_VIDEOCALLS :
            responseCallList(msg, response, responselen);
            break;
        default :
            RILLOGD("Not Supported VideoPhone Response");
            return RIL_ERRNO_INVALID_RESPONSE;
    }
    return 0;
}

static int responsePrioNet(OemMsg *msg, void *response, size_t responselen) {
    OemRequest *resp = (OemRequest*)response;
    switch (resp->subFuncId) {
        case OEM_REQ_SUBFUNCID_GET_PRIORITY_NETWORK_MODE :
            responseInts(msg, response, responselen);
            break;
        default :
            RILLOGD("Not Supported Prority Network Response");
            return RIL_ERRNO_INVALID_RESPONSE;
    }
    return 0;
}

static int responseIMS(OemMsg *msg, void *response, size_t responselen) {
    OemRequest *resp = (OemRequest*)response;
    switch (resp->subFuncId) {
        case OEM_REQ_SUBFUNCID_GET_IMS_VOICE_CALL_AVAILABILITY :
            responseInts(msg, response, responselen);
            break;
        default :
            RILLOGD("Not Supported Prority Network Response");
            return RIL_ERRNO_INVALID_RESPONSE;
    }
    return 0;
}

static int responseCallListVoLTE(OemMsg *msg, void *response, size_t responselen) {
    int num;
    if (responselen % sizeof (RIL_Call_VoLTE *) != 0) {
        RILLOGE("invalid response length %d expected multiple of %d\n",
            (int)responselen, (int)sizeof (RIL_Call_VoLTE *));
        return RIL_ERRNO_INVALID_RESPONSE;
    }

    startResponse;
    /* number of call info's */
    num = responselen / sizeof(RIL_Call_VoLTE *);
    msg->add_intpayload(num);

    OemRequest *resp = (OemRequest*)response;
    RIL_Call_VoLTE **pp_calls = ((RIL_Call_VoLTE **)(resp->payload));

    for (int i = 0 ; i < num ; i++) {
        RIL_Call_VoLTE *p_cur = pp_calls[i];
        /* each call info */
        msg->add_intpayload(p_cur->index);
        msg->add_intpayload(p_cur->isMT);
        msg->add_intpayload(p_cur->negStatusPresent);
        msg->add_intpayload(p_cur->negStatus);
        msg->add_intpayload(p_cur->csMode);
        msg->add_intpayload(p_cur->state);
        msg->add_intpayload(p_cur->mpty);
        msg->add_intpayload(p_cur->numberType);
        msg->add_intpayload(p_cur->toa);
        msg->add_strpayload(p_cur->mediaDescription);
        if (p_cur->number != NULL) {
            char* number_tmp = strdup(p_cur->number);
            stripNumberFromSipAddress(p_cur->number, number_tmp, strlen(number_tmp) * sizeof(char));
            msg->add_strpayload(number_tmp);
            free(number_tmp);
        } else {
            msg->add_strpayload(p_cur->number);
        }
        msg->add_intpayload(p_cur->prioritypresent);
        msg->add_intpayload(p_cur->priority);
        msg->add_intpayload(p_cur->CliValidityPresent);
        msg->add_intpayload(p_cur->numberPresentation);
        msg->add_intpayload(p_cur->als);
        msg->add_intpayload(p_cur->isVoicePrivacy);
        msg->add_intpayload(p_cur->namePresentation);
        msg->add_strpayload(p_cur->name);

        // Remove when partners upgrade to version 3
        if ((s_callbacks.version < 3) || (p_cur->uusInfo == NULL || p_cur->uusInfo->uusData == NULL)) {
            msg->add_intpayload(0);/* UUS Information is absent */
        } else {
            RIL_UUS_Info *uusInfo = p_cur->uusInfo;
            msg->add_intpayload(1);/* UUS Information is present */
            msg->add_intpayload(uusInfo->uusType);
            msg->add_intpayload(uusInfo->uusDcs);
            msg->add_intpayload(uusInfo->uusLength);
            msg->add_strpayload(uusInfo->uusData);
        }
        appendPrintBuf("%s[id=%d,%s,[neg_Present=%d,",
            printBuf,
            p_cur->index,
            (p_cur->isMT)?"mt":"mo",
            p_cur->negStatusPresent);
        appendPrintBuf("%snegStatus=%d,mediaDes=%s],[csMode=%d,",
            printBuf,
            p_cur->negStatus,
            p_cur->mediaDescription,
            (p_cur->csMode));
        appendPrintBuf("%s,%s,conf=%d,numberType=%d,",
            printBuf,
            callStateToString(p_cur->state),
            (p_cur->mpty),
            p_cur->numberType);

        appendPrintBuf("%s,toa=%d,%s],[pri_p=%d,priority=%d,CliValidity=%d,",
            printBuf,
            p_cur->toa,
            p_cur->number,
            p_cur->prioritypresent,
            p_cur->priority,
            p_cur->CliValidityPresent);
        appendPrintBuf("%s,cli=%d],als='%d',%s,%s,%s]",
            printBuf,
            p_cur->numberPresentation,
            p_cur->als,
            (p_cur->isVoicePrivacy)?"voc":"nonvoc",
            p_cur->name,
            p_cur->namePresentation);
    }
    removeLastChar;
    closeResponse;

    return 0;
}

static int responseVolte(OemMsg *msg, void *response, size_t responselen) {
    OemRequest *resp = (OemRequest*)response;
    switch (resp->subFuncId) {
        case OEM_REQ_SUBFUNCID_GET_CURRENT_CALLS_VOLTE :
            responseCallListVoLTE(msg, response, responselen);
            break;
        default :
            RILLOGD("Not Supported Volte Response");
            return RIL_ERRNO_INVALID_RESPONSE;
    }
    return 0;
}

static int responseSimlock(OemMsg *msg, void *response, size_t responselen) {
    OemRequest *resp = (OemRequest*)response;
    switch (resp->subFuncId) {
        case OEM_REQ_SUBFUNCID_GET_SIMLOCK_REMAIN_TIMES :
        case OEM_REQ_SUBFUNCID_GET_SIMLOCK_STATUS :
        case OEM_REQ_SUBFUNCID_GET_SIMLOCK_DUMMYS :
            responseInts(msg, response, responselen);
            break;
        case OEM_REQ_SUBFUNCID_GET_SIMLOCK_WHITELIST :
            responseString(msg, response, responselen);
            break;
        default :
            RILLOGD("Not Supported Volte Response");
            return RIL_ERRNO_INVALID_RESPONSE;
    }
    return 0;
}

#endif

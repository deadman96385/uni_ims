
#define LOG_TAG 	"RILProxy"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <cutils/properties.h>
#include <cutils/record_stream.h>
#include <cutils/jstring.h>
#include <utils/Log.h>
#include <cutils/sockets.h>
#include <binder/Parcel.h>
#include "rilproxy.h"
#include "sprd_ril.h"
#include <netinet/in.h>

namespace android {
  
#define MAX_RESPONE_BUFFER_LEN      (4*1024)    // sms broadcast may be 4K
#define MAX_REQUEST_BUFFER_LEN      (8*1024)    // match with constant in RIL.java

#define MAX_RESPONSE_FROM_TDG_LTE   16
#define MAX_REQUEST_TYPE_ARRAY_LEN  (RIL_REQUEST_LAST + RIL_SPRD_REQUEST_LAST - RIL_SPRD_REQUEST_BASE)
#define RIL_LTE_USIM_READY_PROP          "ril.lte.usim.ready" // for SVLTE only, used by rilproxy
#define LTE_RADIO_POWER_TOKEN            0x00FFFFFE

static int sRILPServerFd = -1;
static int sTdGClientFd  = -1;
static int sLteClientFd  = -1;
static int sPSEnable  = PS_TD_ENABLE;
static int sLteReady  = 0;
static int sTdRadioPowerSent = 0;

static RecordStream *sReqRecordStream = NULL;
static pthread_mutex_t sWriteMutex    = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t sReqTDGLTEMutex= PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t sResponseMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t sLteCFdMutex   = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t sRadiopowerMutex   = PTHREAD_MUTEX_INITIALIZER;

static RILP_RequestType     sReqTypeArr[MAX_REQUEST_TYPE_ARRAY_LEN];      
static RILP_RspTDG_LTE      sRilP_RspTDGLTE[MAX_RESPONSE_FROM_TDG_LTE];
/* Add for dual signal bar */
static RIL_SignalStrength_v6 sSignalStrength;

static void  solicited_response (void *rspbuf, int nlen, int isfromTdg);
static void  unsolicited_response (void *rspbuf, int nlen, int isfromTdg);
static void  process_response(void *rspbuf, int nlen, int isfromTdg);
static void  process_request(void *reqbuf, int nlen);
static RILP_RequestType  get_reqtype_by_id (int reqId);
static void  init_request_type(void);
static void  add_reqid_token_to_table (int reqid, int token) ;
static void  set_rsptype_by_token(int token, int rsptype);
static int   get_rsptype_by_token(int token);
static void  clean_rsptye_token (int token);
static int   is_tdg_unsolicited(int rspid);
static int   is_lte_unsolicited(int rspid);
static RILP_RequestType  get_send_at_request_type(char* req);
/* Add for dual signal bar */
static void backupSignalStrength(Parcel &p, int isfromTdg);
static void mergeSignalStrength(Parcel &p, int isfromTdg);
static void signalToString(const char *funcname, RIL_SignalStrength_v6 signal);


static int get_test_mode(void) {
    int testmode = 0;
    char prop[PROPERTY_VALUE_MAX]="";
    
    property_get(SSDA_TESTMODE_PROP, prop, "0");
    testmode = atoi(prop);
    if ((testmode == 13) || (testmode == 14)) {
        testmode = 255;
    }
    return testmode;
}

extern "C" bool is_svlte(void) {
    char prop[PROPERTY_VALUE_MAX]="";
    
    property_get(SSDA_MODE_PROP, prop, "0");
    if (!strcmp(prop,"svlte") && (0 == get_test_mode())) {
        return true;
    }
    return false;
}

static void init_request_type(void) {
    
    int i;
    for (i=0; i< MAX_REQUEST_TYPE_ARRAY_LEN; i++) {
		sReqTypeArr[i] = ReqToTDG;
	}
	
    /* 
     * "index +1 " is the same as requesid defined in android original ril
     *  but for spreadtrum extension ril, should subcrat 'base'
     */
    // data connection AT to target modem (TD/G or LTE modem)
    sReqTypeArr[RIL_REQUEST_SETUP_DATA_CALL - 1]           = ReqToAuto;
    sReqTypeArr[RIL_REQUEST_DEACTIVATE_DATA_CALL -1]       = ReqToAuto;
    sReqTypeArr[RIL_REQUEST_DATA_REGISTRATION_STATE - 1]   = ReqToAuto;
    sReqTypeArr[RIL_REQUEST_LAST_DATA_CALL_FAIL_CAUSE - 1] = ReqToAuto;
    sReqTypeArr[RIL_REQUEST_DATA_CALL_LIST - 1]            = ReqToAuto;
    sReqTypeArr[RIL_REQUEST_SET_INITIAL_ATTACH_APN - 1]    = ReqToAuto;
    sReqTypeArr[RIL_REQUEST_GPRS_ATTACH - RIL_SPRD_REQUEST_BASE - 1 + RIL_REQUEST_LAST] = ReqToAuto;
    sReqTypeArr[RIL_REQUEST_GPRS_DETACH - RIL_SPRD_REQUEST_BASE - 1 + RIL_REQUEST_LAST] = ReqToAuto;
	
/*
	sReqTypeArr[RIL_REQUEST_GET_CELL_INFO_LIST - 1]        = ReqToAuto;
	sReqTypeArr[RIL_REQUEST_SET_UNSOL_CELL_INFO_LIST_RATE - 1] = ReqToAuto;
	sReqTypeArr[RIL_REQUEST_GET_NEIGHBORING_CELL_IDS - 1]  = ReqToAuto;
	sReqTypeArr[RIL_REQUEST_SET_LOCATION_UPDATES - 1]      = ReqToAuto;
*/
  
    // request only send to LTE modem
    // sReqTypeArr[] =  ReqToLTE; 
  
    // request send to both LTE modem and TD/G modem
    sReqTypeArr[RIL_REQUEST_SCREEN_STATE -1] =  ReqToTDG_LTE;
    sReqTypeArr[RIL_REQUEST_RADIO_POWER -1]  =  ReqToTDG_LTE;
    sReqTypeArr[RIL_REQUEST_SIGNAL_STRENGTH -1]  =  ReqToTDG_LTE;
    sReqTypeArr[RIL_REQUEST_SIM_POWER - RIL_SPRD_REQUEST_BASE - 1 + RIL_REQUEST_LAST] =  ReqToTDG_LTE;

    sReqTypeArr[RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE -1]    =  ReqToTDG_LTE;
    sReqTypeArr[RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC -1] =  ReqToTDG_LTE;
    // sReqTypeArr[RIL_REQUEST_OPERATOR -1]                        =  ReqToTDG_LTE;
    sReqTypeArr[RIL_REQUEST_QUERY_AVAILABLE_NETWORKS -1]        =  ReqToTDG_LTE;

    memset(sRilP_RspTDGLTE, 0 , sizeof(sRilP_RspTDGLTE));

}

static RILP_RequestType get_reqtype_by_id (int reqId) {
	
	int index = 0;
	
	if (is_svlte()) {
	    pthread_mutex_lock(&sLteCFdMutex);
	    if (sLteClientFd  != -1 && sLteReady) { // when lte rild connect, ril request should been dispatched.
			pthread_mutex_unlock(&sLteCFdMutex);
			
			if (reqId <= RIL_REQUEST_LAST)
				index = reqId - 1;
			else if (reqId > RIL_SPRD_REQUEST_BASE)
				index = reqId - RIL_SPRD_REQUEST_BASE - 1 + RIL_REQUEST_LAST;
			else 
				ALOGE("Invalid request id: %d, its range is [1 ~ %d] or [%d ~ %d]",
					 reqId, RIL_REQUEST_LAST, RIL_SPRD_REQUEST_BASE + 1, RIL_REQUEST_LAST);

			return sReqTypeArr[index];
		} else {
			pthread_mutex_unlock(&sLteCFdMutex);
			return ReqToTDG;
		}
	} else {
		return ReqToTDG;
	}
}


static void add_reqid_token_to_table(int reqId, int token) {
	
    int i;
    RILP_RspTDG_LTE *p_rsp = sRilP_RspTDGLTE;
    
    for(i=0; i< MAX_RESPONSE_FROM_TDG_LTE; i++, p_rsp++) {
		if (p_rsp->token == 0) {
			p_rsp->reqId   = reqId;
			p_rsp->token   = token;
			p_rsp->rspType = 0;
			p_rsp->sentSate= RSP_UNSENT;
			ALOGD("Add request id: %d token: %d to table at index: %d.",reqId, token,i);
			break;
		}
	}
	

	if (i == MAX_RESPONSE_FROM_TDG_LTE) {
		ALOGE("Too many request need send to both rild.");
	}
}

		
static int get_reqid_by_token(int token) {
    int i;
    RILP_RspTDG_LTE *p_rsp = sRilP_RspTDGLTE;
    
    for(i=0; i< MAX_RESPONSE_FROM_TDG_LTE; i++, p_rsp++) {
		if (p_rsp->token == token) {
			return p_rsp->reqId;
		}
	}
	
	return -1;
}


static void set_rsptype_by_token(int token, int rsptype) {
	
    int i;
    RILP_RspTDG_LTE *p_rsp = sRilP_RspTDGLTE;
    
    for(i=0; i< MAX_RESPONSE_FROM_TDG_LTE; i++, p_rsp++) {
		if (p_rsp->token == token) {
			p_rsp->rspType |= rsptype;
			ALOGD("set response type, its token: %d, its index: %d.",token, i);
			break;
		}
	}
	
	if (i == MAX_RESPONSE_FROM_TDG_LTE) {
		ALOGE("Too many request need send to both rild.");
	}
}

static int get_rsptype_by_token(int token) {
    int i;
    RILP_RspTDG_LTE *p_rsp = sRilP_RspTDGLTE;
    
    for(i=0; i< MAX_RESPONSE_FROM_TDG_LTE; i++, p_rsp++) {
		if (p_rsp->token == token) {
			return p_rsp->rspType;
		}
	}

	return -1;
}

static void set_rsp_sent_state_by_token(int token, int sentState) {
	int i;
    RILP_RspTDG_LTE *p_rsp = sRilP_RspTDGLTE;
    
    for(i=0; i< MAX_RESPONSE_FROM_TDG_LTE; i++, p_rsp++) {
		if (p_rsp->token == token) {
			p_rsp->sentSate = sentState;
			ALOGD("Clean record: token %d in the table at index: %d.",token, i);
		}
	}
}


static int get_rsp_sent_state_by_token(int token) {
	int i;
    RILP_RspTDG_LTE *p_rsp = sRilP_RspTDGLTE;
    
    for(i=0; i< MAX_RESPONSE_FROM_TDG_LTE; i++, p_rsp++) {
		if (p_rsp->token == token) {
			return p_rsp->sentSate;
		}
	}
	
	return RSP_UNSENT;
}


static void clean_rsptye_token(int token) {
    int i;
    RILP_RspTDG_LTE *p_rsp = sRilP_RspTDGLTE;
    
    for(i=0; i< MAX_RESPONSE_FROM_TDG_LTE; i++, p_rsp++) {
		if (p_rsp->token == token) {
			p_rsp->rspType = 0;
			p_rsp->token = 0;
			p_rsp->reqId = 0;
			p_rsp->sentSate= RSP_UNSENT;
			ALOGD("Clean record: token %d in the table at index: %d.",token, i);
		}
	}
}

static char *strdupReadString(Parcel &p) {
    size_t stringlen;
    const char16_t *s16;

    s16 = p.readString16Inplace(&stringlen);

    return strndup16to8(s16, stringlen);
}

static RILP_RequestType  get_send_at_request_type(char* req) {

    if (req == NULL) {
		ALOGE("Send_AT command should not be NULL!");
		return ReqToTDG;
	}

	if (is_svlte()) {
	    pthread_mutex_lock(&sLteCFdMutex);
	    if (sLteClientFd  != -1 && sLteReady) { // when lte rild connect, ril request should been dispatched.
			pthread_mutex_unlock(&sLteCFdMutex);
			if (strncasecmp(req, SEND_AT_TO_LTE_LTEBGTIMER, strlen(SEND_AT_TO_LTE_LTEBGTIMER)) == 0 ||
			    strncasecmp(req, SEND_AT_TO_LTE_LTESETRSRP, strlen(SEND_AT_TO_LTE_LTESETRSRP)) == 0 ||
			    strncasecmp(req, SEND_AT_TO_LTE_LTENCELLINFO, strlen(SEND_AT_TO_LTE_LTENCELLINFO)) == 0 ||
			    strncasecmp(req, SEND_AT_TO_LTE_CPOF, strlen(SEND_AT_TO_LTE_CPOF)) == 0) {
				return ReqToLTE;
			} else if (strncasecmp(req, SEND_AT_TO_AT_RESET, strlen(SEND_AT_TO_AT_RESET)) == 0 || 
			           strncasecmp(req, SEND_AT_TO_AT_SPATASSERT, strlen(SEND_AT_TO_AT_SPATASSERT)) == 0) {
				return ReqToTDG_LTE;
			}
		} else {
			pthread_mutex_unlock(&sLteCFdMutex);
			return ReqToTDG;
		}
	}

	return ReqToTDG;
}

static RILP_RequestType  get_manual_select_network_request_type(int act) {

	if (is_svlte()) {
	    pthread_mutex_lock(&sLteCFdMutex);
	    if (sLteClientFd  != -1 && sLteReady) { // when lte rild connect, ril request should been dispatched.
			pthread_mutex_unlock(&sLteCFdMutex);

			if (act == 7) {
				return ReqToLTE;
			}
		} else {
			pthread_mutex_unlock(&sLteCFdMutex);
			return ReqToTDG;
		}
	}

	return ReqToTDG;
}

/* unsolicited response sent or not according to PS state.
	RIL_UNSOL_DATA_CALL_LIST_CHANGED,

  unsolicited response send RILJ directly.
	RIL_UNSOL_SIM_PS_REJECT
	RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED
  
  default unsolicited response form TD/G send to RILJ dirctly
*/
static int   is_tdg_unsolicited(int rspid) {

	if ((sPSEnable == PS_LTE_ENABLE) &&
	    (rspid == RIL_UNSOL_DATA_CALL_LIST_CHANGED ))
	    return 0;
    else 
		return 1;
}

static int  is_lte_unsolicited(int rspid) {
	
	if (rspid == RIL_UNSOL_SIM_PS_REJECT ||
	    rspid == RIL_UNSOL_LTE_READY ||
	    rspid == RIL_UNSOL_SIGNAL_STRENGTH ||
	    rspid == RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED)
	    return 1;
   
    if ((sPSEnable == PS_LTE_ENABLE) &&
	    (rspid == RIL_UNSOL_DATA_CALL_LIST_CHANGED))
	    return 1;
	else
		return 0;
}

static int blocking_write(int fd, const void *buffer, size_t len) {
    size_t writeOffset = 0;
    const uint8_t *toWrite;

    toWrite = (const uint8_t *)buffer;

    while (writeOffset < len) {
        ssize_t written;
        do {
            written = write (fd, toWrite + writeOffset,
                                len - writeOffset);
        } while (written < 0 && ((errno == EINTR) || (errno == EAGAIN)));

        if (written >= 0) {
            writeOffset += written;
        } else {   // written < 0
            ALOGE ("Unexpected error on write errno:%d, %s", errno, strerror(errno));
            // close(fd);
            return -1;
        }
    }

    return 0;
}

static int
write_data (int fd , const void *data, size_t dataSize) {

    int ret;
    uint32_t header;

    pthread_mutex_lock(&sWriteMutex);

    header = htonl(dataSize);

    ret = blocking_write(fd, (void *)&header, sizeof(header));

    if (ret < 0) {
        pthread_mutex_unlock(&sWriteMutex);
        ALOGE("RILProxy:  blockingWrite header error");
        return ret;
    }

    ret = blocking_write(fd, data, dataSize);

    if (ret < 0) {
        pthread_mutex_unlock(&sWriteMutex);
        ALOGE("RILProxy:  blockingWrite data error");
        return ret;
    }

    pthread_mutex_unlock(&sWriteMutex);

    return 0;
}

static int  send_lte_enable_response(int token, int result) {

	char rspbuf[20];
	int  rsptype = RESPONSE_SOLICITED;
	int  error = ( result ? 0 : RIL_E_GENERIC_FAILURE) ;
	int  nlen = 1;

	memcpy(rspbuf,   &rsptype, 4);
	memcpy(&rspbuf[4], &token, 4);
	memcpy(&rspbuf[8], &error, 4);
	memcpy(&rspbuf[12], &nlen, 4);
	memcpy(&rspbuf[16], &result, 4);
	
	return write_data (sRILPServerFd, (void *)rspbuf, error == 0 ? 20 : 12);
}

static int  send_lte_radio_power(int poweron, int autoatt) {

    Parcel p;
    p.writeInt32(RIL_REQUEST_RADIO_POWER);
    p.writeInt32(LTE_RADIO_POWER_TOKEN);
    p.writeInt32(2);
    p.writeInt32(poweron);
    p.writeInt32(autoatt);

    return write_data (sLteClientFd, p.data(), p.dataSize());
}


static void backupSignalStrength(Parcel &p, int isfromTdg) {
    int skip;

    ALOGD("%s: enter isfromTdg = %d", __FUNCTION__, isfromTdg);
    p.readInt32(&skip);
    if (isfromTdg) {
        /* save current TD data */
        p.readInt32(&sSignalStrength.GW_SignalStrength.signalStrength);
        p.readInt32(&sSignalStrength.GW_SignalStrength.bitErrorRate);
    } else {
        /* skip */
        p.readInt32(&skip);
        p.readInt32(&skip);
        p.readInt32(&skip);
        p.readInt32(&skip);
        p.readInt32(&skip);
        p.readInt32(&skip);
        p.readInt32(&skip);
        /* save current LTE data */
        p.readInt32(&sSignalStrength.LTE_SignalStrength.signalStrength);
        p.readInt32(&sSignalStrength.LTE_SignalStrength.rsrp);
        p.readInt32(&sSignalStrength.LTE_SignalStrength.rsrq);
        p.readInt32(&sSignalStrength.LTE_SignalStrength.rssnr);
        p.readInt32(&sSignalStrength.LTE_SignalStrength.cqi);
    }
    signalToString(__FUNCTION__, sSignalStrength);
}

static void mergeSignalStrength(Parcel &p, int isfromTdg) {
    int skip;

    ALOGD("%s: enter isfromTdg = %d", __FUNCTION__, isfromTdg);
    if (isfromTdg) {
        /* save current TD data */
        p.readInt32(&sSignalStrength.GW_SignalStrength.signalStrength);
        p.readInt32(&sSignalStrength.GW_SignalStrength.bitErrorRate);
        /* skip */
        p.readInt32(&skip);
        p.readInt32(&skip);
        p.readInt32(&skip);
        p.readInt32(&skip);
        p.readInt32(&skip);
        /* write last LTE data */
        p.writeInt32(sSignalStrength.LTE_SignalStrength.signalStrength);
        p.writeInt32(sSignalStrength.LTE_SignalStrength.rsrp);
        p.writeInt32(sSignalStrength.LTE_SignalStrength.rsrq);
        p.writeInt32(sSignalStrength.LTE_SignalStrength.rssnr);
        p.writeInt32(sSignalStrength.LTE_SignalStrength.cqi);
    } else {
        /* write last TD data */
        p.writeInt32(sSignalStrength.GW_SignalStrength.signalStrength);
        p.writeInt32(sSignalStrength.GW_SignalStrength.bitErrorRate);
        /* skip */
        p.readInt32(&skip);
        p.readInt32(&skip);
        p.readInt32(&skip);
        p.readInt32(&skip);
        p.readInt32(&skip);
        /* save current LTE data */
        p.readInt32(&sSignalStrength.LTE_SignalStrength.signalStrength);
        p.readInt32(&sSignalStrength.LTE_SignalStrength.rsrp);
        p.readInt32(&sSignalStrength.LTE_SignalStrength.rsrq);
        p.readInt32(&sSignalStrength.LTE_SignalStrength.rssnr);
        p.readInt32(&sSignalStrength.LTE_SignalStrength.cqi);
    }
    signalToString(__FUNCTION__, sSignalStrength);
}

static void signalToString(const char *funcname, RIL_SignalStrength_v6 signal) {
    ALOGD("%s:signalStr = %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d",
                funcname,
                signal.GW_SignalStrength.signalStrength,
                signal.GW_SignalStrength.bitErrorRate,
                signal.CDMA_SignalStrength.dbm,
                signal.CDMA_SignalStrength.ecio,
                signal.EVDO_SignalStrength.dbm,
                signal.EVDO_SignalStrength.ecio,
                signal.EVDO_SignalStrength.signalNoiseRatio,
                signal.LTE_SignalStrength.signalStrength,
                signal.LTE_SignalStrength.rsrp,
                signal.LTE_SignalStrength.rsrq,
                signal.LTE_SignalStrength.rssnr,
                signal.LTE_SignalStrength.cqi);
}

static Parcel sNetworklistParcel;

static void make_network_list_response(Parcel &p, int nlen) {

    int    numStrings1, numStrings2;
    int    pos;
    int    skip;
    int    err, lasterr;

    p.readInt32(&err);
    sNetworklistParcel.setDataPosition(8);
    sNetworklistParcel.readInt32(&lasterr);

    if (lasterr) {  // last response is error.
        ALOGD("Error occurs in last response.");
    } else {
        if (err) {
            ALOGD("Error occurs in the response, use last response.");
            p.setDataPosition(0);
            p.appendFrom(&sNetworklistParcel, 0, sNetworklistParcel.dataSize());
            return;
        } else {
            sNetworklistParcel.readInt32(&numStrings2);
            ALOGD("Last respnose's length is %d", numStrings2);

            if (numStrings2 % 5 == 0) {
                pos = p.dataPosition();
                p.readInt32(&numStrings1);
                p.setDataPosition(pos);
                p.writeInt32(numStrings1 + numStrings2);

                p.setDataPosition(nlen);
                p.appendFrom(&sNetworklistParcel, 16, sNetworklistParcel.dataSize() - 16);
                ALOGD("After merged response, it's length: %d" , p.dataSize());
            }
        }
    }
}

static void backup_last_response(int reqid, Parcel &p, int isfromTdg) {

    ALOGD("backup_last_response, reqid = %d, nlen = %d", reqid, p.dataSize());
    switch (reqid) {
    case RIL_REQUEST_SIGNAL_STRENGTH :
        backupSignalStrength(p, isfromTdg);
        break;

    case RIL_REQUEST_QUERY_AVAILABLE_NETWORKS :
        sNetworklistParcel.setDataPosition(0);
        sNetworklistParcel.appendFrom(&p, 0, p.dataSize());
        break;

    default:
	// ALOGD("invalid response block");
        break;
    }
}

static void make_final_response(int reqid, Parcel &p, int nlen, int isfromTdg) {
    int skip;

    switch (reqid) {
    case RIL_REQUEST_SIGNAL_STRENGTH:
        p.readInt32(&skip);
        mergeSignalStrength(p, isfromTdg);
        break;

    case RIL_REQUEST_QUERY_AVAILABLE_NETWORKS:
        make_network_list_response(p, nlen);
        break;

    default:
        // ALOGD("invalid response block");
        break;
    }
}

static void solicited_response (void *rspbuf, int nlen, int isfromTdg) {
	int token = -1;
	RILP_RequestType reqType;
	int skip;
	int rspType;
	Parcel p;
	status_t status;
	int reqid;

	p.setData((uint8_t *) rspbuf, nlen);
	status = p.readInt32(&skip);
	status = p.readInt32(&token);
	if (status != NO_ERROR) {
		ALOGE("invalid response block");
		return;
	}

	reqid = get_reqid_by_token(token);
        if (reqid == RIL_REQUEST_RADIO_POWER && token == LTE_RADIO_POWER_TOKEN) {
            ALOGD("RilProxy process the first LTE radio power request.");
            return;
        }

	reqType = get_reqtype_by_id (reqid);
	if (reqType == ReqToTDG_LTE) {
		set_rsptype_by_token(token, isfromTdg ? RSP_FROM_TDG_RILD : RSP_FROM_LTE_RILD);
	}

	/* 
	 * when request is sent to TD/G and LTE, it responese only send
	 * to TD/G till two responses are also received. 
         */
	if (reqType != ReqToTDG_LTE) {
		write_data (sRILPServerFd, rspbuf, nlen);
	} else {
		if (get_rsptype_by_token(token) == (int)ReqToTDG_LTE){
			if (get_rsp_sent_state_by_token(token) == RSP_UNSENT) {
				if (reqid == RIL_REQUEST_SIGNAL_STRENGTH ||
				    reqid == RIL_REQUEST_QUERY_AVAILABLE_NETWORKS) {
				        make_final_response(reqid, p, nlen, isfromTdg);
					write_data (sRILPServerFd, p.data(), p.dataSize());
				} else{
					write_data (sRILPServerFd, rspbuf, nlen);
				}
				ALOGD("Solicited response has sent !");
			}
			clean_rsptye_token(token);
		} else {
#if 0
			int error;
			status = p.readInt32(&error);
			if (status != NO_ERROR) {
				ALOGE("invalid response block");
				return;
			}

			if (error) {
				write_data (sRILPServerFd, rspbuf, nlen);
				set_rsp_sent_state_by_token(token, RSP_SENT_DONE);
				ALOGD("There is an error in response for reqid: %d", reqid);
			} else {
				ALOGD("Waiting for another rild response reqid: %d", reqid);
			  backup_last_response(reqid, p, isfromTdg);
			}
#endif
			ALOGD("Waiting for another rild response reqid: %d", reqid);
			backup_last_response(reqid, p, isfromTdg);
		}
	}
}


static void unsolicited_response (void *rspbuf, int nlen, int isfromTdg) {
	int skip;
	int rspId;
	Parcel p;
	status_t status;
	/* Add for dual signal bar */
	int rssi = 0, ber = 0, blte = 0;
        char prop[PROPERTY_VALUE_MAX] = "";

	p.setData((uint8_t *) rspbuf, nlen);
	status = p.readInt32(&skip);
	status = p.readInt32(&rspId);
	if (status != NO_ERROR) {
		ALOGE("Invalid response block");
		return;
	}
	
	if ((isfromTdg  && is_tdg_unsolicited(rspId))||
	     (!isfromTdg && is_lte_unsolicited(rspId))){
		/* Add for dual signal bar */
		if(rspId == RIL_UNSOL_SIGNAL_STRENGTH) {
			ALOGD("Received RIL_UNSOL_SIGNAL_STRENGTH isfromTdg = %d", isfromTdg);
			mergeSignalStrength(p, isfromTdg);
			write_data (sRILPServerFd, p.data(), p.dataSize());
		} else {
			write_data (sRILPServerFd, rspbuf, nlen);
		}
		ALOGD("Unsolicited response has sent !");
	}

    /* Add for dual signal bar */
    switch(rspId) {
      case RIL_UNSOL_LTE_READY:
        ALOGD("Received RIL_UNSOL_LTE_READY isfromTdg = %d", isfromTdg);
        p.readInt32(&skip);
        p.readInt32(&blte);
        if (blte != 1 && blte != 5) {
            ALOGD("Clear saved sSignalStrength data");
            RIL_SIGNALSTRENGTH_INIT_LTE(sSignalStrength);
        }
        break;
      /* It doesn't sure which the two RIL_CONNETCED and SVLTE_USIM_READY appear first.*/
      case RIL_UNSOL_RIL_CONNECTED:
         property_get(RIL_LTE_USIM_READY_PROP, prop, "0");
         if (!atoi(prop)) break;
         ALOGD("SVLTE Ril connected");
      case RIL_UNSOL_SVLTE_USIM_READY:
        if (!isfromTdg) {
            ALOGD("SVLTE USIM READY");
            pthread_mutex_lock(&sRadiopowerMutex);
            if (sTdRadioPowerSent) {
                pthread_mutex_unlock(&sRadiopowerMutex);
                send_lte_radio_power(1,0);
            } else {
                pthread_mutex_unlock(&sRadiopowerMutex);
            }

            property_set(RIL_LTE_USIM_READY_PROP, "0");
            sLteReady = 1;
        }
        break;
      default:
        return;
    }
}


static void process_response(void *rspbuf, int nlen, int isfromTdg){

	int  respType ;
	Parcel p;
	status_t status;

	p.setData((uint8_t *) rspbuf, nlen);
	status = p.readInt32(&respType);
	if (status != NO_ERROR) {
		ALOGE("Invalid response block");
		return;
	}

	ALOGD("Response type :%d, len:%d", respType, nlen);
	switch (respType) {
		case  RESPONSE_SOLICITED:
		solicited_response(rspbuf, nlen, isfromTdg);
		break;
		
		case  RESPONSE_UNSOLICITED:
		unsolicited_response(rspbuf, nlen, isfromTdg);
		break;
		
		default :
		ALOGE("Invalid response type");
		break;
	}
}

static void process_request(void *reqbuf, int nlen) {
	
	int reqId;
	int token;
	RILP_RequestType reqType;
	Parcel p;
	status_t status;
	
	p.setData((uint8_t *) (reqbuf), nlen);
	status = p.readInt32(&reqId);
	status = p.readInt32(&token);
	if (status != NO_ERROR) {
		ALOGE("Invalid request block");
		return;
	}

	if (reqId == RIL_REQUEST_SET_RILPROXY_LTE_ENABLE) {
		int len;
		int lteEnable;
		status = p.readInt32(&len);
		status = p.readInt32(&lteEnable);
		if (status != NO_ERROR || len != 1) {
			ALOGE("Failed to get LTE enable state!");
			send_lte_enable_response(token, 0);
			return;
		}
		ALOGD("PS enable on %s modem", lteEnable ? "LTE" : "TD/G");
		sPSEnable = (lteEnable ? PS_LTE_ENABLE : PS_TD_ENABLE);
		send_lte_enable_response(token, 1);
		return;
	} else if (reqId == RIL_REQUEST_SEND_AT) {
		// skip reqId, token, len.
                char *atcmd = strdupReadString(p);
		reqType = get_send_at_request_type(atcmd);
                ALOGD("send at command :%s", atcmd);
                free(atcmd);
	} else if (reqId == RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL) {
                int act;
                p.readString16();
                p.readInt32(&act);
                ALOGD("manual selcect network act=%d", act);
		reqType = get_manual_select_network_request_type(act);
	} else {
		ALOGD("process_request request id %d, token %d", reqId, token);
		reqType = get_reqtype_by_id(reqId);
	}

	ALOGD("process_request request type %d", reqType);
        if (reqType == ReqToTDG_LTE) {
		add_reqid_token_to_table(reqId, token);	
	} else if (reqType == ReqToAuto) {
		if (sPSEnable == PS_TD_ENABLE) {
		    reqType = ReqToTDG;
		} else {
		    reqType = ReqToLTE;
		}
	} else if (reqType == ReqToTDG) {
               if (reqId == RIL_REQUEST_RADIO_POWER) {
                   pthread_mutex_lock(&sRadiopowerMutex);
                   sTdRadioPowerSent = 1;
                   pthread_mutex_unlock(&sRadiopowerMutex);
               }
        }
	
	if (reqType & ReqToTDG) {
		if (sTdGClientFd != -1) {
			write_data(sTdGClientFd, reqbuf, nlen);
		} else {
			ALOGE("TD client socket has been destroy!");
		}
	} 
	
	if (reqType & ReqToLTE) {
		if (sLteClientFd != -1 && sLteReady) {
			write_data(sLteClientFd, reqbuf, nlen);
		} else {
			ALOGE("LTE socket hasn't ready!");
		}
	}
}


static int rilproxy_connect(const char* sockname)
{
	int  sockfd;
	
    if (sockname == NULL) {
		ALOGE("%s: invalid socket name  is NULL !", __func__);
		return -1;		
	}
	
	ALOGD("Try to connect socket %s...", sockname);
	sockfd = socket_local_client(sockname,
		ANDROID_SOCKET_NAMESPACE_RESERVED, SOCK_STREAM);

	while(sockfd < 0) {
		sleep(1);
		sockfd = socket_local_client(sockname,
			 ANDROID_SOCKET_NAMESPACE_RESERVED, SOCK_STREAM);
	}
	ALOGD("Connect socket %s success", sockname);
	
    if (fcntl(sockfd, F_SETFL, O_NONBLOCK) < 0) {
        ALOGE ("Error setting O_NONBLOCK errno:%d, %s", errno, strerror(errno));
        close(sockfd);
        return -1;
    }
	return sockfd;
}

extern "C" void  rilproxy_init(void) {
	int sfd, fd;
	
	init_request_type();

	/* Add for dual signal bar */
	RIL_SIGNALSTRENGTH_INIT(sSignalStrength);

	sfd = android_get_control_socket(RILPROXY_SOCKET_NAME);
    if (sfd < 0) {
		ALOGE("Failed to get socket ' %s '", RILPROXY_SOCKET_NAME);
        exit(-1);
    }
    
    if (listen(sfd, 4) < 0) {
        ALOGE("Failed to listen on control socket '%d': %s", sfd, strerror(errno));
        exit(-1);
    }
    
    ALOGD("Waiting for connect from RILJ");
    if ((fd=accept(sfd,NULL,NULL)) == -1) {
		ALOGE("Socket accept error: %s ", strerror(errno));
		exit(-1);
	}
	sRILPServerFd = fd;
	sReqRecordStream = record_stream_new(fd, MAX_REQUEST_BUFFER_LEN);
}


extern "C" void*  rilproxy_client(void* sockname) {

	int ret,fd = -1;
	fd_set rfds;
	RecordStream *p_rs;
	void *p_record;
	size_t recordlen;
	int rspFromTd = 1;

	if ((fd = rilproxy_connect((char *)sockname)) < 0) {
		ALOGE("Failed to connect LTE Rild.");
		return NULL;
	} else {
		if (!strcmp((char *)sockname,LTE_RILD_SOCKET_NAME)) {
			sLteClientFd = fd;
			rspFromTd = 0;
		}
		else if (!strcmp((char *)sockname,TDG_RILD_SOCKET_NAME)) {
			sTdGClientFd = fd;
			rspFromTd = 1;
		}

		p_rs = record_stream_new(fd, MAX_RESPONE_BUFFER_LEN);
	}

	for (;;) {
		
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		
		do {
			ret = select(fd + 1, &rfds, NULL, NULL, 0);
		} while(ret == -1 && (errno == EINTR || errno == EAGAIN));
		
		if (ret > 0) {
			if (FD_ISSET(fd, &rfds)) {
				for (;;) {
					ret = record_stream_get_next(p_rs, &p_record, &recordlen);
					if (ret == 0 && p_record == NULL){
						ALOGE("Read socket failure, then should reconnect socket!");
						close(fd);
						exit(0);
						goto error;
					} else if (ret < 0){
						ALOGD("Process response buffer done.");
						break;  
					}  
					pthread_mutex_lock(&sResponseMutex);
					process_response(p_record, recordlen, rspFromTd);
					pthread_mutex_unlock(&sResponseMutex);
				}
			}
		}
	}
error :
	record_stream_free(p_rs);
	return NULL;
}

extern "C" void  rilproxy_server() {
	int ret;	
	void *p_record;
	size_t recordlen;
	
	for(;;){
		ALOGD("Beginning receive data form RILJ ");
		/* loop until EAGAIN/EINTR, end of stream, or other error */
		ret = record_stream_get_next(sReqRecordStream, &p_record, &recordlen);
		if (ret == 0 && p_record == NULL) {
			record_stream_free(sReqRecordStream);
			sReqRecordStream = NULL;
			ALOGE("Receive request failure, request is null!");
			break;  
		} else if (ret < 0) {
			if (errno == EINTR || errno == EAGAIN) continue;
			ALOGE("Receive request failure, error: %s !", strerror(errno));
			break;
		}
		
	    // ALOGD("buffer's length is %d", recordlen);
		process_request(p_record, recordlen);
	}
	close(sRILPServerFd);
	sRILPServerFd = -1;
}


}

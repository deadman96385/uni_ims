#ifndef SPRD_RIL_OEM_H
#define SPRD_RIL_OEM_H

/*
 * RIL_REQUEST_OEM_HOOK_RAW
 * request data struct
 */
#if defined (RIL_SUPPORTED_OEM_PROTOBUF)
typedef struct _OemRequest
{
    int funcId;
    int subFuncId;
    void * payload;
} OemRequest;
#else
typedef struct _OemRequest
{
    unsigned char funcId;
    unsigned char subFuncId;
    int len;
    void *payload;
} OemRequest;
#endif

/*
 * RIL_UNSOL_OEM_HOOK_RAW
 * response data struct
 */
typedef struct {
    unsigned char oemFuncId;
    char* data;
} RIL_OEM_NOTIFY;

/*
 * RIL_REQUEST_OEM_HOOK_RAW
 * function IDs
 */
//blacklist
#if defined (RIL_SUPPORT_CALL_BLACKLIST)
#define OEM_REQ_FUNCTION_ID_CALL_BLACKLIST                0x01
#endif
//Icc Card
#define OEM_REQ_FUNCTION_ID_SIM_POWER                     0x02
//Icc FDN
#define OEM_REQ_FUNCTION_ID_GET_REMAIN_TIMES              0x03
//STK Call
#define OEM_REQ_FUNCTION_ID_STK_DIAL                      0x04
//Query COLR/COLP
#define OEM_REQ_FUNCTION_ID_QUERY_COLP_COLR               0x05
//MMI Enter SIM
#define OEM_REQ_FUNCTION_ID_MMI_ENTER_SIM                 0x06
//Video Phone
#define OEM_REQ_FUNCTION_ID_VIDEOPHONE                    0x07
//MBBMS
#define OEM_REQ_FUNCTION_ID_MBBMS                         0x08
//Get SIM Capacity
#define OEM_REQ_FUNCTION_ID_GET_SIM_CAPACITY              0x09
//End All Connections
#define OEM_REQ_FUNCTION_ID_END_ALL_CONNECTIONS           0x0A
//Set Cmms
#define OEM_REQ_FUNCTION_ID_SET_CMMS                      0x0B
//CSFallBack
#define OEM_REQ_FUNCTION_ID_CSFALLBACK                    0x0C
//Priority Network
#define OEM_REQ_FUNCTION_ID_PRIORITY_NETWORK              0x0D
//Band Info
#define OEM_REQ_FUNCTION_ID_GET_BAND_INFO                 0x0E
//Switch Band Info Report 3_wire BT Wifi
#define OEM_REQ_FUNCTION_ID_SWITCH                        0x0F
//Stop query available networks
#define OEM_REQ_FUNCTION_ID_STOP_QUERY_AVAILABLE_NETWORKS 0x10
//Init ISIM
#define OEM_REQ_FUNCTION_ID_INIT_ISIM                     0x11
//IMS
#define OEM_REQ_FUNCTION_ID_IMS                           0x12
//Volte
#define OEM_REQ_FUNCTION_ID_VOLTE                         0x13
//Enable Broadcast SMS
#define OEM_REQ_FUNCTION_ID_ENABLE_BROADCAST_SMS          0x14
//Simlock
#define OEM_REQ_FUNCTION_ID_SIMLOCK                       0x15


/*
 * RIL_REQUEST_OEM_HOOK_RAW
 * sub-function IDs
 */
//blacklist
#if defined (RIL_SUPPORT_CALL_BLACKLIST)
#define OEM_REQ_SUBFUNCID_MINMATCH 0x01
#define OEM_REQ_SUBFUNCID_BLACKLIST 0x02
#endif
//video phone
#define OEM_REQ_SUBFUNCID_VIDEOPHONE_DIAL                   0x01
#define OEM_REQ_SUBFUNCID_VIDEOPHONE_CODEC                  0x02
#define OEM_REQ_SUBFUNCID_VIDEOPHONE_HANGUP                 0x03
#define OEM_REQ_SUBFUNCID_VIDEOPHONE_ANSWER                 0x04
#define OEM_REQ_SUBFUNCID_VIDEOPHONE_FALLBACK               0x05
#define OEM_REQ_SUBFUNCID_VIDEOPHONE_STRING                 0x06
#define OEM_REQ_SUBFUNCID_VIDEOPHONE_LOCAL_MEDIA            0x07
#define OEM_REQ_SUBFUNCID_VIDEOPHONE_RECORD_VIDEO           0x08
#define OEM_REQ_SUBFUNCID_VIDEOPHONE_RECORD_AUDIO           0x09
#define OEM_REQ_SUBFUNCID_VIDEOPHONE_TEST                   0x0A
#define OEM_REQ_SUBFUNCID_VIDEOPHONE_GET_CURRENT_VIDEOCALLS 0x0B
#define OEM_REQ_SUBFUNCID_VIDEOPHONE_CONTROL_IFRAME         0x0C
#define OEM_REQ_SUBFUNCID_VIDEOPHONE_SET_VOICERECORDTYPE    0x0D
#define OEM_REQ_SUBFUNCID_VIDEOPHONE_CONTROL_AUDIO          0x0E
//COLP_CLOR
#define OEM_REQ_SUBFUNCID_QUERY_COLP    0x01
#define OEM_REQ_SUBFUNCID_QUERY_COLR    0x02
//MBBMS
#define OEM_REQ_SUBFUNCID_MBBMS_GSM_AUTHEN     0x01
#define OEM_REQ_SUBFUNCID_MBBMS_USIM_AUTHEN    0x02
#define OEM_REQ_SUBFUNCID_MBBMS_SIM_TYPE       0x03
//Call CSFallBack
#define OEM_REQ_SUBFUNCID_CALL_CSFALLBACK_ACCEPT  0x01
#define OEM_REQ_SUBFUNCID_CALL_CSFALLBACK_REJECT  0x02
//Priority Network
#define OEM_REQ_SUBFUNCID_SET_PRIORITY_NETWORK_MODE  0x01
#define OEM_REQ_SUBFUNCID_GET_PRIORITY_NETWORK_MODE  0x02
//Switch Band Info Report 3_wire BT Wifi
#define OEM_REQ_SUBFUNCID_SWITCH_BAND_INFO_REPORT    0x01
#define OEM_REQ_SUBFUNCID_SWITCH_3_WIRE              0x02
#define OEM_REQ_SUBFUNCID_SWITCH_BT                  0x03
#define OEM_REQ_SUBFUNCID_SWITCH_WIFI                0x04
//IMS
#define OEM_REQ_SUBFUNCID_SET_IMS_VOICE_CALL_AVAILABILITY        0x01
#define OEM_REQ_SUBFUNCID_GET_IMS_VOICE_CALL_AVAILABILITY        0x02
#define OEM_REQ_SUBFUNCID_REGISTER_IMS_IMPU                      0x03
#define OEM_REQ_SUBFUNCID_REGISTER_IMS_IMPI                      0x04
#define OEM_REQ_SUBFUNCID_REGISTER_IMS_DOMAIN                    0x05
#define OEM_REQ_SUBFUNCID_REGISTER_IMS_IMEI                      0x06
#define OEM_REQ_SUBFUNCID_REGISTER_IMS_XCAP                      0x07
#define OEM_REQ_SUBFUNCID_REGISTER_IMS_BSF                       0x08
#define OEM_REQ_SUBFUNCID_DISABLE_IMS                            0x09
#define OEM_REQ_SUBFUNCID_SET_IMS_SMSC                           0x0A
#define OEM_REQ_SUBFUNCID_SET_INITIAL_ATTACH_IMS_APN             0x0B
//Volte
#define OEM_REQ_SUBFUNCID_VOLTE_CALL_FALL_BACK_TO_VOICE      0x01
#define OEM_REQ_SUBFUNCID_VOLTE_INITIAL_GROUP_CALL           0x02
#define OEM_REQ_SUBFUNCID_VOLTE_ADD_TO_GROUP_CALL            0x03
#define OEM_REQ_SUBFUNCID_VOLTE_SET_CONFERENCE_URI           0x04
#define OEM_REQ_SUBFUNCID_VOLTE_CALL_REQUEST_MEDIA_CHANGE    0x05
#define OEM_REQ_SUBFUNCID_VOLTE_CALL_RESPONSE_MEDIA_CHANGE   0x06
#define OEM_REQ_SUBFUNCID_GET_CURRENT_CALLS_VOLTE            0x07
//Simlock
#define OEM_REQ_SUBFUNCID_GET_SIMLOCK_REMAIN_TIMES   0x01
#define OEM_REQ_SUBFUNCID_GET_SIMLOCK_STATUS         0x02
#define OEM_REQ_SUBFUNCID_GET_SIMLOCK_DUMMYS         0x03
#define OEM_REQ_SUBFUNCID_GET_SIMLOCK_WHITELIST      0x04

/*
 * RIL_REQUEST_OEM_HOOK_STRINGS
 * function IDs
 */


/*
 * RIL_REQUEST_OEM_HOOK_STRINGS
 * sub-function IDs
 */


/*
 * RIL_UNSOL_OEM_HOOK_RAW
 * function IDs
 */
#if defined (RIL_SUPPORT_CALL_BLACKLIST)
#define OEM_UNSOL_FUNCTION_ID_BLOCKCALLS 0x01
#define OEM_UNSOL_FUNCTION_ID_SPPCI      0x02
#endif
#define OEM_UNSOL_FUNCTION_ID_EXPIREDSIM     0x03


/*
 * RIL_UNSOL_OEM_HOOK_STRINGS
 * function IDs
 */


#endif

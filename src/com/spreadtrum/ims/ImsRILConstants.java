package com.spreadtrum.ims;

public class ImsRILConstants {

    final static int RIL_SPRD_REQUEST_BASE = 2000;
    
    final static int RIL_REQUEST_GET_IMS_CURRENT_CALLS = RIL_SPRD_REQUEST_BASE + 1;
    final static int RIL_REQUEST_SET_IMS_VOICE_CALL_AVAILABILITY = RIL_SPRD_REQUEST_BASE + 2;
    final static int RIL_REQUEST_GET_IMS_VOICE_CALL_AVAILABILITY = RIL_SPRD_REQUEST_BASE + 3;
    final static int RIL_REQUEST_INIT_ISIM = RIL_SPRD_REQUEST_BASE + 4;
    final static int RIL_REQUEST_IMS_CALL_REQUEST_MEDIA_CHANGE  = RIL_SPRD_REQUEST_BASE + 5;
    final static int RIL_REQUEST_IMS_CALL_RESPONSE_MEDIA_CHANGE = RIL_SPRD_REQUEST_BASE + 6;
    final static int RIL_REQUEST_SET_IMS_SMSC = RIL_SPRD_REQUEST_BASE + 7;
    final static int RIL_REQUEST_IMS_CALL_FALL_BACK_TO_VOICE = RIL_SPRD_REQUEST_BASE + 8;
    final static int RIL_REQUEST_SET_IMS_INITIAL_ATTACH_APN = RIL_SPRD_REQUEST_BASE + 9;
    final static int RIL_REQUEST_QUERY_CALL_FORWARD_STATUS_URI = RIL_SPRD_REQUEST_BASE + 10;
    final static int RIL_REQUEST_SET_CALL_FORWARD_URI = RIL_SPRD_REQUEST_BASE + 11;
    final static int RIL_REQUEST_IMS_INITIAL_GROUP_CALL = RIL_SPRD_REQUEST_BASE + 12;
    final static int RIL_REQUEST_IMS_ADD_TO_GROUP_CALL = RIL_SPRD_REQUEST_BASE + 13;
    final static int RIL_REQUEST_VIDEOPHONE_DIAL = RIL_SPRD_REQUEST_BASE + 14;
    final static int RIL_REQUEST_ENABLE_IMS = RIL_SPRD_REQUEST_BASE + 15;
    final static int RIL_REQUEST_DISABLE_IMS = RIL_SPRD_REQUEST_BASE + 16;
    final static int RIL_REQUEST_GET_IMS_BEARER_STATE = RIL_SPRD_REQUEST_BASE + 17;
    final static int RIL_REQUEST_VIDEOPHONE_CODEC = RIL_SPRD_REQUEST_BASE + 18;

    final static int RIL_SPRD_UNSOL_RESPONSE_BASE = 3000;

    final static int RIL_UNSOL_RESPONSE_IMS_CALL_STATE_CHANGED = RIL_SPRD_UNSOL_RESPONSE_BASE + 0;
    final static int RIL_UNSOL_RESPONSE_VIDEO_QUALITY = RIL_SPRD_UNSOL_RESPONSE_BASE + 1;
    final static int RIL_UNSOL_RESPONSE_IMS_BEARER_ESTABLISTED = RIL_SPRD_UNSOL_RESPONSE_BASE + 2;
}
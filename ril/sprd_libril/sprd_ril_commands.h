/* //vendor/sprd/proprietories-source/ril/sprd_libril/sprd_ril_commands.h
**
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/
    {0, NULL, NULL},                   //none
    {RIL_REQUEST_GET_SIM_STATUS, dispatchVoid, responseSimStatus},
    {RIL_REQUEST_ENTER_SIM_PIN, dispatchStrings, responseInts},
    {RIL_REQUEST_ENTER_SIM_PUK, dispatchStrings, responseInts},
    {RIL_REQUEST_ENTER_SIM_PIN2, dispatchStrings, responseInts},
    {RIL_REQUEST_ENTER_SIM_PUK2, dispatchStrings, responseInts},
    {RIL_REQUEST_CHANGE_SIM_PIN, dispatchStrings, responseInts},
    {RIL_REQUEST_CHANGE_SIM_PIN2, dispatchStrings, responseInts},
    {RIL_REQUEST_ENTER_NETWORK_DEPERSONALIZATION, dispatchStrings, responseInts},
    {RIL_REQUEST_GET_CURRENT_CALLS, dispatchVoid, responseCallList},
    {RIL_REQUEST_DIAL, dispatchDial, responseVoid},
    {RIL_REQUEST_GET_IMSI, dispatchVoid, responseString},
    {RIL_REQUEST_HANGUP, dispatchInts, responseVoid},
    {RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND, dispatchVoid, responseVoid},
    {RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND, dispatchVoid, responseVoid},
    {RIL_REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE, dispatchVoid, responseVoid},
    {RIL_REQUEST_CONFERENCE, dispatchVoid, responseVoid},
    {RIL_REQUEST_UDUB, dispatchVoid, responseVoid},
    {RIL_REQUEST_LAST_CALL_FAIL_CAUSE, dispatchVoid, responseFailCause},
    {RIL_REQUEST_SIGNAL_STRENGTH, dispatchVoid, responseRilSignalStrength},
    {RIL_REQUEST_VOICE_REGISTRATION_STATE, dispatchVoid, responseStrings},
    {RIL_REQUEST_DATA_REGISTRATION_STATE, dispatchVoid, responseStrings},
    {RIL_REQUEST_OPERATOR, dispatchVoid, responseStrings},
    {RIL_REQUEST_RADIO_POWER, dispatchInts, responseVoid},
    {RIL_REQUEST_DTMF, dispatchString, responseVoid},
    {RIL_REQUEST_SEND_SMS, dispatchStrings, responseSMS},
    {RIL_REQUEST_SEND_SMS_EXPECT_MORE, dispatchStrings, responseSMS},
    {RIL_REQUEST_SETUP_DATA_CALL, dispatchDataCall, responseSetupDataCall},
    {RIL_REQUEST_SIM_IO, dispatchSIM_IO, responseSIM_IO},
    {RIL_REQUEST_SEND_USSD, dispatchString, responseVoid},
    {RIL_REQUEST_CANCEL_USSD, dispatchVoid, responseVoid},
    {RIL_REQUEST_GET_CLIR, dispatchVoid, responseInts},
    {RIL_REQUEST_SET_CLIR, dispatchInts, responseVoid},
    {RIL_REQUEST_QUERY_CALL_FORWARD_STATUS, dispatchCallForward, responseCallForwards},
    {RIL_REQUEST_SET_CALL_FORWARD, dispatchCallForward, responseVoid},
    {RIL_REQUEST_QUERY_CALL_WAITING, dispatchInts, responseInts},
    {RIL_REQUEST_SET_CALL_WAITING, dispatchInts, responseVoid},
    {RIL_REQUEST_SMS_ACKNOWLEDGE, dispatchInts, responseVoid},
    {RIL_REQUEST_GET_IMEI, dispatchVoid, responseString},
    {RIL_REQUEST_GET_IMEISV, dispatchVoid, responseString},
    {RIL_REQUEST_ANSWER,dispatchVoid, responseVoid},
    {RIL_REQUEST_DEACTIVATE_DATA_CALL, dispatchStrings, responseVoid},
    {RIL_REQUEST_QUERY_FACILITY_LOCK, dispatchStrings, responseInts},
    {RIL_REQUEST_SET_FACILITY_LOCK, dispatchStrings, responseInts},
    {RIL_REQUEST_CHANGE_BARRING_PASSWORD, dispatchStrings, responseVoid},
    {RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE, dispatchVoid, responseInts},
    {RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC, dispatchVoid, responseVoid},
#ifdef RIL_SPRD_EXTENSION
    {RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL, dispatchNetworkList, responseVoid},
#elif defined GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION
    {RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL, dispatchString, responseVoid},
#endif
    {RIL_REQUEST_QUERY_AVAILABLE_NETWORKS , dispatchVoid, responseStrings},
    {RIL_REQUEST_DTMF_START, dispatchString, responseVoid},
    {RIL_REQUEST_DTMF_STOP, dispatchVoid, responseVoid},
    {RIL_REQUEST_BASEBAND_VERSION, dispatchVoid, responseString},
    {RIL_REQUEST_SEPARATE_CONNECTION, dispatchInts, responseVoid},
    {RIL_REQUEST_SET_MUTE, dispatchInts, responseVoid},
    {RIL_REQUEST_GET_MUTE, dispatchVoid, responseInts},
    {RIL_REQUEST_QUERY_CLIP, dispatchVoid, responseInts},
    {RIL_REQUEST_LAST_DATA_CALL_FAIL_CAUSE, dispatchVoid, responseInts},
    {RIL_REQUEST_DATA_CALL_LIST, dispatchVoid, responseDataCallList},
    {RIL_REQUEST_RESET_RADIO, dispatchVoid, responseVoid},
    {RIL_REQUEST_OEM_HOOK_RAW, dispatchRaw, responseRaw},
    /*{RIL_REQUEST_OEM_HOOK_RAW, dispatchRawSprd, responseRawSprd},*/
    {RIL_REQUEST_OEM_HOOK_STRINGS, dispatchStrings, responseStrings},
    {RIL_REQUEST_SCREEN_STATE, dispatchInts, responseVoid},
    {RIL_REQUEST_SET_SUPP_SVC_NOTIFICATION, dispatchInts, responseVoid},
    {RIL_REQUEST_WRITE_SMS_TO_SIM, dispatchSmsWrite, responseInts},
    {RIL_REQUEST_DELETE_SMS_ON_SIM, dispatchInts, responseVoid},
    {RIL_REQUEST_SET_BAND_MODE, dispatchInts, responseVoid},
    {RIL_REQUEST_QUERY_AVAILABLE_BAND_MODE, dispatchVoid, responseInts},
    {RIL_REQUEST_STK_GET_PROFILE, dispatchVoid, responseString},
    {RIL_REQUEST_STK_SET_PROFILE, dispatchString, responseVoid},
    {RIL_REQUEST_STK_SEND_ENVELOPE_COMMAND, dispatchString, responseString},
    {RIL_REQUEST_STK_SEND_TERMINAL_RESPONSE, dispatchString, responseVoid},
    {RIL_REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM, dispatchInts, responseVoid},
    {RIL_REQUEST_EXPLICIT_CALL_TRANSFER, dispatchVoid, responseVoid},
    {RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE, dispatchInts, responseVoid},
    {RIL_REQUEST_GET_PREFERRED_NETWORK_TYPE, dispatchVoid, responseInts},
    {RIL_REQUEST_GET_NEIGHBORING_CELL_IDS, dispatchVoid, responseCellList},
    {RIL_REQUEST_SET_LOCATION_UPDATES, dispatchInts, responseVoid},
    {RIL_REQUEST_CDMA_SET_SUBSCRIPTION_SOURCE, dispatchInts, responseVoid},
    {RIL_REQUEST_CDMA_SET_ROAMING_PREFERENCE, dispatchInts, responseVoid},
    {RIL_REQUEST_CDMA_QUERY_ROAMING_PREFERENCE, dispatchVoid, responseInts},
    {RIL_REQUEST_SET_TTY_MODE, dispatchInts, responseVoid},
    {RIL_REQUEST_QUERY_TTY_MODE, dispatchVoid, responseInts},
    {RIL_REQUEST_CDMA_SET_PREFERRED_VOICE_PRIVACY_MODE, dispatchInts, responseVoid},
    {RIL_REQUEST_CDMA_QUERY_PREFERRED_VOICE_PRIVACY_MODE, dispatchVoid, responseInts},
    {RIL_REQUEST_CDMA_FLASH, dispatchString, responseVoid},
    {RIL_REQUEST_CDMA_BURST_DTMF, dispatchStrings, responseVoid},
    {RIL_REQUEST_CDMA_VALIDATE_AND_WRITE_AKEY, dispatchString, responseVoid},
    {RIL_REQUEST_CDMA_SEND_SMS, dispatchCdmaSms, responseSMS},
    {RIL_REQUEST_CDMA_SMS_ACKNOWLEDGE, dispatchCdmaSmsAck, responseVoid},
    {RIL_REQUEST_GSM_GET_BROADCAST_SMS_CONFIG, dispatchVoid, responseGsmBrSmsCnf},
    {RIL_REQUEST_GSM_SET_BROADCAST_SMS_CONFIG, dispatchGsmBrSmsCnf, responseVoid},
    {RIL_REQUEST_GSM_SMS_BROADCAST_ACTIVATION, dispatchInts, responseVoid},
    {RIL_REQUEST_CDMA_GET_BROADCAST_SMS_CONFIG, dispatchVoid, responseCdmaBrSmsCnf},
    {RIL_REQUEST_CDMA_SET_BROADCAST_SMS_CONFIG, dispatchCdmaBrSmsCnf, responseVoid},
    {RIL_REQUEST_CDMA_SMS_BROADCAST_ACTIVATION, dispatchInts, responseVoid},
    {RIL_REQUEST_CDMA_SUBSCRIPTION, dispatchVoid, responseStrings},
    {RIL_REQUEST_CDMA_WRITE_SMS_TO_RUIM, dispatchRilCdmaSmsWriteArgs, responseInts},
    {RIL_REQUEST_CDMA_DELETE_SMS_ON_RUIM, dispatchInts, responseVoid},
    {RIL_REQUEST_DEVICE_IDENTITY, dispatchVoid, responseStrings},
    {RIL_REQUEST_EXIT_EMERGENCY_CALLBACK_MODE, dispatchVoid, responseVoid},
    {RIL_REQUEST_GET_SMSC_ADDRESS, dispatchVoid, responseString},
    {RIL_REQUEST_SET_SMSC_ADDRESS, dispatchString, responseVoid},
    {RIL_REQUEST_REPORT_SMS_MEMORY_STATUS, dispatchInts, responseVoid},
    {RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING, dispatchVoid, responseVoid},
    {RIL_REQUEST_CDMA_GET_SUBSCRIPTION_SOURCE, dispatchCdmaSubscriptionSource, responseInts},
    {RIL_REQUEST_ISIM_AUTHENTICATION, dispatchString, responseString},
    {RIL_REQUEST_ACKNOWLEDGE_INCOMING_GSM_SMS_WITH_PDU, dispatchStrings, responseVoid},
    {RIL_REQUEST_STK_SEND_ENVELOPE_WITH_STATUS, dispatchString, responseSIM_IO},
    {RIL_REQUEST_VOICE_RADIO_TECH, dispatchVoiceRadioTech, responseInts},
    {RIL_REQUEST_GET_CELL_INFO_LIST, dispatchVoid, responseCellInfoList},
    {RIL_REQUEST_SET_UNSOL_CELL_INFO_LIST_RATE, dispatchInts, responseVoid},
    {RIL_REQUEST_SET_INITIAL_ATTACH_APN, dispatchSetInitialAttachApn, responseVoid},
    {RIL_REQUEST_IMS_REGISTRATION_STATE, dispatchVoid, responseInts},
    {RIL_REQUEST_IMS_SEND_SMS, dispatchImsSms, responseSMS},
    {RIL_REQUEST_SIM_TRANSMIT_APDU_BASIC, dispatchSIM_APDU, responseSIM_IO},
    {RIL_REQUEST_SIM_OPEN_CHANNEL, dispatchString, responseInts},
    {RIL_REQUEST_SIM_CLOSE_CHANNEL, dispatchInts, responseVoid},
    {RIL_REQUEST_SIM_TRANSMIT_APDU_CHANNEL, dispatchSIM_APDU, responseSIM_IO},
    {RIL_REQUEST_NV_READ_ITEM, dispatchNVReadItem, responseString},
    {RIL_REQUEST_NV_WRITE_ITEM, dispatchNVWriteItem, responseVoid},
    {RIL_REQUEST_NV_WRITE_CDMA_PRL, dispatchRaw, responseVoid},
    {RIL_REQUEST_NV_RESET_CONFIG, dispatchInts, responseVoid},
    {RIL_REQUEST_SET_UICC_SUBSCRIPTION, dispatchUiccSubscripton, responseVoid},
    {RIL_REQUEST_ALLOW_DATA, dispatchInts, responseVoid},
    {RIL_REQUEST_GET_HARDWARE_CONFIG, dispatchVoid, responseHardwareConfig},
    {RIL_REQUEST_SIM_AUTHENTICATION, dispatchSimAuthentication, responseSIM_IO},
    {RIL_REQUEST_GET_DC_RT_INFO, dispatchVoid, responseDcRtInfo},
    {RIL_REQUEST_SET_DC_RT_INFO_RATE, dispatchInts, responseVoid},
    {RIL_REQUEST_SET_DATA_PROFILE, dispatchDataProfile, responseVoid},
    {RIL_REQUEST_SHUTDOWN, dispatchVoid, responseVoid},
    {RIL_REQUEST_GET_RADIO_CAPABILITY, dispatchVoid, responseRadioCapability},
    {RIL_REQUEST_SET_RADIO_CAPABILITY, dispatchRadioCapability, responseRadioCapability},
    {RIL_REQUEST_START_LCE, dispatchInts, responseLceStatus},
    {RIL_REQUEST_STOP_LCE, dispatchVoid, responseLceStatus},
    {RIL_REQUEST_PULL_LCEDATA, dispatchVoid, responseLceData},
    {RIL_REQUEST_GET_ACTIVITY_INFO, dispatchVoid, responseActivityData}

#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
    ,{RIL_REQUEST_IMS_REGISTRATION_STATE, dispatchVoid, responseInts}
    ,{RIL_REQUEST_IMS_SEND_SMS, dispatchImsSendSms, responseImsSendSms}
    ,{RIL_REQUEST_GET_DATA_CALL_PROFILE, dispatchInts, responseDataCallProfile}
    ,{RIL_REQUEST_SET_UICC_SUBSCRIPTION, dispatchSetUiccSub, responseVoid}
    ,{RIL_REQUEST_SET_DATA_SUBSCRIPTION, dispatchVoid, responseVoid}
    ,{RIL_REQUEST_GET_UICC_SUBSCRIPTION, dispatchVoid, responseGetUiccSub}
    ,{RIL_REQUEST_GET_DATA_SUBSCRIPTION, dispatchVoid, responseInts}
    ,{RIL_REQUEST_SET_SUBSCRIPTION_MODE, dispatchInts, responseVoid}
    ,{RIL_REQUEST_SET_TRANSMIT_POWER, dispatchInts, responseVoid}
    ,{RIL_REQUEST_SETUP_QOS, dispatchString, responseString}
    ,{RIL_REQUEST_RELEASE_QOS, dispatchString, responseString}
    ,{RIL_REQUEST_GET_QOS_STATUS, dispatchString, responseString}
    ,{RIL_REQUEST_MODIFY_QOS, dispatchString, responseString}
    ,{RIL_REQUEST_SUSPEND_QOS, dispatchString, responseString}
    ,{RIL_REQUEST_RESUME_QOS, dispatchString, responseString}
#endif
#if defined (RIL_SPRD_EXTENSION)
    ,{RIL_REQUEST_VIDEOPHONE_DIAL, dispatchVideoPhoneDial, responseVoid}
    ,{RIL_REQUEST_VIDEOPHONE_CODEC, dispatchVideoPhoneCodec, responseVoid}
    ,{RIL_REQUEST_GET_IMS_CURRENT_CALLS, dispatchVoid, responseCallListIMS}//SPRD: add for VoLTE to handle +CLCCS
    ,{RIL_REQUEST_SET_IMS_VOICE_CALL_AVAILABILITY, dispatchInts, responseVoid}
    ,{RIL_REQUEST_GET_IMS_VOICE_CALL_AVAILABILITY, dispatchVoid, responseInts}
    ,{RIL_REQUEST_INIT_ISIM, dispatchStrings, responseInts}
    ,{RIL_REQUEST_REGISTER_IMS_IMPU, dispatchString, responseVoid}
    ,{RIL_REQUEST_REGISTER_IMS_IMPI, dispatchString, responseVoid}
    ,{RIL_REQUEST_REGISTER_IMS_DOMAIN, dispatchString, responseVoid}
    ,{RIL_REQUEST_ENABLE_IMS, dispatchVoid, responseVoid}
    ,{RIL_REQUEST_DISABLE_IMS, dispatchVoid, responseVoid}
    ,{RIL_REQUEST_REGISTER_IMS_IMEI, dispatchString, responseVoid}
    ,{RIL_REQUEST_IMS_CALL_REQUEST_MEDIA_CHANGE, dispatchInts, responseVoid}
    ,{RIL_REQUEST_IMS_CALL_RESPONSE_MEDIA_CHANGE, dispatchInts, responseVoid}
    ,{RIL_REQUEST_REGISTER_IMS_XCAP, dispatchString, responseVoid}
    ,{RIL_REQUEST_REGISTER_IMS_BSF, dispatchString, responseVoid}
    ,{RIL_REQUEST_SET_IMS_SMSC, dispatchString, responseVoid}
    ,{RIL_REQUEST_IMS_CALL_FALL_BACK_TO_VOICE, dispatchInts, responseVoid}
    ,{RIL_REQUEST_SET_IMS_INITIAL_ATTACH_APN, dispatchSetInitialAttachApn, responseVoid}
    ,{RIL_REQUEST_QUERY_CALL_FORWARD_STATUS_URI, dispatchCallForwardUri, responseCallForwardsUri}
    ,{RIL_REQUEST_SET_CALL_FORWARD_URI, dispatchCallForwardUri, responseVoid}
    ,{RIL_REQUEST_IMS_INITIAL_GROUP_CALL, dispatchString, responseVoid}
    ,{RIL_REQUEST_IMS_ADD_TO_GROUP_CALL, dispatchString, responseVoid}
    ,{RIL_REQUEST_IMS_SET_CONFERENCE_URI, dispatchString, responseVoid}
    ,{RIL_REQUEST_VIDEOPHONE_HANGUP, dispatchInts, responseVoid}
    ,{RIL_REQUEST_VIDEOPHONE_ANSWER, dispatchVoid, responseVoid}
    ,{RIL_REQUEST_VIDEOPHONE_FALLBACK, dispatchVoid, responseVoid}
    ,{RIL_REQUEST_VIDEOPHONE_STRING, dispatchString, responseVoid}
    ,{RIL_REQUEST_VIDEOPHONE_LOCAL_MEDIA, dispatchInts, responseVoid}
    ,{RIL_REQUEST_VIDEOPHONE_RECORD_VIDEO, dispatchInts, responseVoid}
    ,{RIL_REQUEST_VIDEOPHONE_RECORD_AUDIO, dispatchInts, responseVoid}
    ,{RIL_REQUEST_VIDEOPHONE_SET_VOICERECORDTYPE, dispatchInts, responseVoid}
    ,{RIL_REQUEST_VIDEOPHONE_TEST, dispatchInts, responseVoid}
    ,{RIL_REQUEST_GET_CURRENT_VIDEOCALLS, dispatchVoid, responseCallList}
    ,{RIL_REQUEST_VIDEOPHONE_CONTROL_AUDIO, dispatchInts, responseVoid}
    ,{RIL_REQUEST_VIDEOPHONE_CONTROL_IFRAME, dispatchInts, responseVoid}
    ,{RIL_REQUEST_MBBMS_GSM_AUTHEN, dispatchString, responseString}
    ,{RIL_REQUEST_MBBMS_USIM_AUTHEN, dispatchStrings, responseString}
    ,{RIL_REQUEST_MBBMS_SIM_TYPE, dispatchVoid, responseString}
    ,{RIL_REQUEST_GPRS_ATTACH, dispatchVoid, responseVoid}
    ,{RIL_REQUEST_GPRS_DETACH, dispatchVoid, responseVoid}
    ,{RIL_REQUEST_GET_SIM_CAPACITY, dispatchVoid, responseStrings}
    ,{RIL_REQUEST_QUERY_COLP, dispatchVoid, responseInts}
    ,{RIL_REQUEST_QUERY_COLR, dispatchVoid, responseInts}
    ,{RIL_REQUEST_MMI_ENTER_SIM, dispatchString, responseInts}
    ,{RIL_REQUEST_END_ALL_CONNECTIONS, dispatchVoid, responseVoid}
    ,{RIL_REQUEST_GET_REMAIN_TIMES, dispatchInts, responseInts}
    ,{RIL_REQUEST_SET_CMMS, dispatchInts, responseVoid}
    ,{RIL_REQUEST_SIM_POWER, dispatchInts, responseVoid}
    ,{RIL_REQUEST_SEND_AT, dispatchString, responseString}
    ,{RIL_REQUEST_SET_SPEED_MODE, dispatchInts, responseVoid}
    ,{RIL_REQUEST_SET_SIM_SLOT_CFG, dispatchInts, responseVoid}//SPRD:added for choosing WCDMA SIM
    ,{RIL_REQUEST_GET_SIMLOCK_REMAIN_TIMES, dispatchInts, responseInts}
    ,{RIL_REQUEST_CALL_CSFALLBACK_ACCEPT, dispatchVoid, responseVoid} //SPRD:add for LTE-CSFB to handle CS fall back of MT call
    ,{RIL_REQUEST_CALL_CSFALLBACK_REJECT, dispatchVoid, responseVoid} //SPRD:add for LTE-CSFB to handle CS fall back of MT call
    ,{RIL_REQUEST_SET_PRIORITY_NETWORK_MODE, dispatchInts, responseVoid} //SPRD:add for priority network mode
    ,{RIL_REQUEST_GET_PRIORITY_NETWORK_MODE, dispatchVoid, responseInts} //SPRD:add for priority network mode
    //SPRD: For WIFI get BandInfo report from modem,* BRCM4343+9620, Zhanlei Feng added. 2014.06.20 START
    ,{RIL_REQUEST_GET_BAND_INFO, dispatchVoid, responseString}
    ,{RIL_REQUEST_SWITCH_BAND_INFO_REPORT, dispatchInts, responseVoid}
    ,{RIL_REQUEST_SWITCH_3_WIRE, dispatchInts, responseVoid}
    ,{RIL_REQUEST_SWITCH_BT, dispatchInts, responseVoid}
    ,{RIL_REQUEST_SWITCH_WIFI, dispatchInts, responseVoid}
    //SPRD: For WIFI get BandInfo report from modem,* BRCM4343+9620, Zhanlei Feng added. 2014.06.20 END
    //SPRD: for stop query available networks
    ,{RIL_REQUEST_STOP_QUERY_AVAILABLE_NETWORKS, dispatchVoid, responseVoid}
    ,{RIL_REQUEST_SET_FACILITY_LOCK_FOR_USER, dispatchStrings, responseVoid}
    ,{RIL_REQUEST_ENABLE_BROADCAST_SMS, dispatchInts, responseVoid}
#if defined (RIL_SUPPORTED_OEMSOCKET)
    ,{RIL_EXT_REQUEST_GET_HD_VOICE_STATE, dispatchVoid, responseInts}
    ,{RIL_EXT_REQUEST_SIM_GET_ATR, dispatchVoid, responseString}
    ,{RIL_EXT_REQUEST_SIM_OPEN_CHANNEL_WITH_P2, dispatchStrings, responseInts}
    ,{RIL_EXT_REQUEST_ENABLE_RAU_NOTIFY, dispatchVoid, responseVoid}
    ,{RIL_EXT_REQUEST_SET_COLP, dispatchInts, responseVoid}
#endif
#endif
#if defined (GLOBALCONFIG_RIL_SAMSUNG_LIBRIL_INTF_EXTENSION)
    ,{RIL_REQUEST_SET_CELL_BROADCAST_CONFIG, dispatchSetCBConf, responseVoid}
    ,{RIL_REQUEST_GET_CELL_BROADCAST_CONFIG, dispatchVoid, responseGetCBConf}
    ,{RIL_REQUEST_CRFM_LINE_SMS_COUNT_MSG, dispatchVoid, responseVoid}
    ,{RIL_REQUEST_CRFM_LINE_SMS_READ_MSG, dispatchVoid, responseVoid}
    ,{RIL_REQUEST_SEND_ENCODED_USSD, dispatchSendUssd, responseVoid}
    ,{RIL_REQUEST_SET_PDA_MEMORY_STATUS, dispatchVoid, responseVoid}
    ,{RIL_REQUEST_GET_PHONEBOOK_STORAGE_INFO, dispatchInts, responseInts}
    ,{RIL_REQUEST_GET_PHONEBOOK_ENTRY, dispatchGetPB, responseGetPB}
    ,{RIL_REQUEST_ACCESS_PHONEBOOK_ENTRY, dispatchAccessPB, responseInts}
    ,{RIL_REQUEST_DIAL_VIDEO_CALL, dispatchVoid, responseVoid}
    ,{RIL_REQUEST_CALL_DEFLECTION, dispatchVoid, responseVoid}
    ,{RIL_REQUEST_READ_SMS_FROM_SIM, dispatchVoid, responseVoid}
    ,{RIL_REQUEST_USIM_PB_CAPA, dispatchVoid, responseInts}
//    ,{RIL_REQUEST_LOCK_INFO, dispatchInts, responseLockInfo}
    ,{RIL_REQUEST_LOCK_INFO, dispatchInts, responseInts}
    ,{RIL_REQUEST_SEND_MOBILE_TRACKER_SMS, dispatchVoid, responseVoid}
    ,{RIL_REQUEST_GET_STOREAD_MSG_COUNT, dispatchVoid, responseInts}
    ,{RIL_REQUEST_STK_SIM_INIT_EVENT, dispatchVoid, responseVoid}
    ,{RIL_REQUEST_GET_LINE_ID, dispatchVoid, responseInts}
    ,{RIL_REQUEST_SET_LINE_ID, dispatchInts, responseVoid}
    ,{RIL_REQUEST_GET_SERIAL_NUMBER, dispatchVoid, responseInts}
    ,{RIL_REQUEST_GET_MANUFACTURE_DATE_NUMBER, dispatchVoid, responseInts}
    ,{RIL_REQUEST_GET_BARCODE_NUMBER, dispatchVoid, responseInts}
    ,{RIL_REQUEST_UICC_GBA_AUTHENTICATE_BOOTSTRAP, dispatchVoid, responseVoid}
    ,{RIL_REQUEST_UICC_GBA_AUTHENTICATE_NAF, dispatchVoid, responseVoid}
    ,{RIL_REQUEST_SIM_APDU, dispatchVoid, responseVoid}
    ,{RIL_REQUEST_SIM_OPEN_CHANNEL, dispatchVoid, responseVoid}
    ,{RIL_REQUEST_SIM_CLOSE_CHANNEL, dispatchVoid, responseVoid}
    ,{RIL_REQUEST_SIM_TRANSMIT_CHANNEL, dispatchVoid, responseVoid}
    ,{RIL_REQUEST_SIM_AUTH, dispatchVoid, responseVoid}
#endif

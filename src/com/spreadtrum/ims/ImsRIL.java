/*
 * Copyright (C) 2006 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.spreadtrum.ims;

import static com.android.internal.telephony.RILConstants.*;
import static android.telephony.TelephonyManager.NETWORK_TYPE_UNKNOWN;
import static android.telephony.TelephonyManager.NETWORK_TYPE_EDGE;
import static android.telephony.TelephonyManager.NETWORK_TYPE_GPRS;
import static android.telephony.TelephonyManager.NETWORK_TYPE_UMTS;
import static android.telephony.TelephonyManager.NETWORK_TYPE_HSDPA;
import static android.telephony.TelephonyManager.NETWORK_TYPE_HSUPA;
import static android.telephony.TelephonyManager.NETWORK_TYPE_HSPA;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.display.DisplayManager;
import android.net.ConnectivityManager;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.os.AsyncResult;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.HwBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Parcel;
import android.os.PowerManager;
import android.os.BatteryManager;
import android.os.Registrant;
import android.os.RegistrantList;
import android.os.RemoteException;
import android.os.SystemProperties;
import android.os.Registrant;
import android.os.SystemClock;
import android.os.WorkSource;
import android.provider.Settings.SettingNotFoundException;
import android.telephony.CellInfo;
import android.telephony.ClientRequestStats;
import android.telephony.NeighboringCellInfo;
import android.telephony.PhoneNumberUtils;
import android.telephony.RadioAccessFamily;
import android.telephony.Rlog;
import android.telephony.SignalStrength;
import android.telephony.SmsManager;
import android.telephony.SmsMessage;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyHistogram;
import android.telephony.TelephonyManager;
import android.telephony.ModemActivityInfo;
import android.text.TextUtils;
import android.util.SparseArray;
import android.view.Display;

import com.android.internal.telephony.RIL;
import com.android.internal.telephony.dataconnection.ApnSetting;
import com.android.internal.telephony.gsm.SmsBroadcastConfigInfo;
import com.android.internal.telephony.gsm.SsData;
import com.android.internal.telephony.gsm.SuppServiceNotification;
import com.android.internal.telephony.metrics.TelephonyMetrics;
import com.android.internal.telephony.uicc.IccCardApplicationStatus;
import com.android.internal.telephony.uicc.IccCardStatus;
import com.android.internal.telephony.uicc.IccIoResult;
import com.android.internal.telephony.uicc.IccRefreshResponse;
import com.android.internal.telephony.uicc.IccUtils;
import com.android.internal.telephony.CommandsInterface.RadioState;
import com.android.internal.telephony.cdma.CdmaCallWaitingNotification;
import com.android.internal.telephony.cdma.CdmaInformationRecords;
import com.android.internal.telephony.cdma.CdmaSmsBroadcastConfigInfo;
import android.telephony.data.DataProfile;
import com.android.internal.telephony.RadioCapability;
import com.android.internal.telephony.TelephonyDevController;
import com.android.internal.telephony.HardwareConfig;
import com.android.internal.telephony.RILConstants;
import com.android.internal.telephony.DriverCall;
import com.android.internal.telephony.CallForwardInfo;
import com.android.internal.telephony.SmsResponse;
import com.android.internal.telephony.OperatorInfo;
import com.android.internal.telephony.UUSInfo;
import com.android.internal.telephony.GsmAlphabet;
import com.android.internal.telephony.LastCallFailCause;
import com.android.internal.telephony.CommandException;
import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.TelephonyProperties;
import android.telephony.ims.ImsCallForwardInfo;
import com.android.ims.internal.ImsCallForwardInfoEx;
import com.android.sprd.telephony.RadioInteractor;
import com.android.sprd.telephony.RadioInteractorCore;
import com.android.sprd.telephony.RadioInteractorHandler;
import com.android.sprd.telephony.RadioInteractorFactory;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_VIDEOPHONE_DIAL;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_QUERY_COLR;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_QUERY_COLP;

import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.FileDescriptor;
import java.io.IOException;
import java.io.InputStream;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.Random;
import java.util.concurrent.atomic.AtomicLong;

import android.hardware.radio.V1_0.Carrier;
import android.hardware.radio.V1_0.CarrierRestrictions;
import android.hardware.radio.V1_0.CdmaBroadcastSmsConfigInfo;
import android.hardware.radio.V1_0.CdmaSmsAck;
import android.hardware.radio.V1_0.CdmaSmsMessage;
import android.hardware.radio.V1_0.CdmaSmsWriteArgs;
import android.hardware.radio.V1_0.CellInfoCdma;
import android.hardware.radio.V1_0.CellInfoGsm;
import android.hardware.radio.V1_0.CellInfoLte;
import android.hardware.radio.V1_0.CellInfoType;
import android.hardware.radio.V1_0.CellInfoWcdma;
import android.hardware.radio.V1_0.DataProfileInfo;
import android.hardware.radio.V1_0.Dial;
import android.hardware.radio.V1_0.GsmBroadcastSmsConfigInfo;
import android.hardware.radio.V1_0.GsmSmsMessage;
import android.hardware.radio.V1_0.HardwareConfigModem;
import android.hardware.radio.V1_0.IRadio;
import android.hardware.radio.V1_0.IccIo;
import android.hardware.radio.V1_0.ImsSmsMessage;
import android.hardware.radio.V1_0.LceDataInfo;
import android.hardware.radio.V1_0.MvnoType;
import android.hardware.radio.V1_0.NvWriteItem;
import android.hardware.radio.V1_0.RadioError;
import android.hardware.radio.V1_0.RadioIndicationType;
import android.hardware.radio.V1_0.RadioResponseInfo;
import android.hardware.radio.V1_0.RadioResponseType;
import android.hardware.radio.V1_0.ResetNvType;
import android.hardware.radio.V1_0.SelectUiccSub;
import android.hardware.radio.V1_0.SetupDataCallResult;
import android.hardware.radio.V1_0.SimApdu;
import android.hardware.radio.V1_0.SmsWriteArgs;
import android.hardware.radio.V1_0.UusInfo;
import android.hardware.radio.deprecated.V1_0.IOemHook;

import vendor.sprd.hardware.radio.V1_0.CallForwardInfoUri;
import vendor.sprd.hardware.radio.V1_0.CallVoLTE;
import vendor.sprd.hardware.radio.V1_0.ExtPersoSubstate;
import vendor.sprd.hardware.radio.V1_0.ExtRadioErrno;
import vendor.sprd.hardware.radio.V1_0.ImsHandoverToVoWifiResult;
import vendor.sprd.hardware.radio.V1_0.ImsHandoverType;
import vendor.sprd.hardware.radio.V1_0.ImsNetworkInfo;
import vendor.sprd.hardware.radio.V1_0.ImsPdnStatus;
import vendor.sprd.hardware.radio.V1_0.ImsPhoneCMCCSI;
import vendor.sprd.hardware.radio.V1_0.NetworkList;
import vendor.sprd.hardware.radio.V1_0.StkCallControlResult;
import vendor.sprd.hardware.radio.V1_0.VideoPhoneCodec;
import vendor.sprd.hardware.radio.V1_0.VideoPhoneDial;
import vendor.sprd.hardware.radio.V1_0.VideoPhoneDSCI;


/**
 * RIL implementation of the CommandsInterface.
 *
 * {@hide}
 */
public final class ImsRIL {
    static final String RILJ_LOG_TAG = "ImsRILJ";
    static final boolean RILJ_LOGD = true;
    static final boolean RILJ_LOGV = false;



    //media request change
    public static final int MEDIA_REQUEST_DEFAULT = 0;
    public static final int MEDIA_REQUEST_AUDIO_UPGRADE_VIDEO_BIDIRECTIONAL = 1;
    public static final int MEDIA_REQUEST_AUDIO_UPGRADE_VIDEO_TX = 2;
    public static final int MEDIA_REQUEST_AUDIO_UPGRADE_VIDEO_RX = 3;
    public static final int MEDIA_REQUEST_VIDEO_TX_UPGRADE_VIDEO_BIDIRECTIONAL = 4;
    public static final int MEDIA_REQUEST_VIDEO_RX_UPGRADE_VIDEO_BIDIRECTIONAL = 5;
    public static final int MEDIA_REQUEST_VIDEO_BIDIRECTIONAL_DOWNGRADE_AUDIO = 6;
    public static final int MEDIA_REQUEST_VIDEO_TX_DOWNGRADE_AUDIO = 7;
    public static final int MEDIA_REQUEST_VIDEO_RX_DOWNGRADE_AUDIO = 8;
    public static final int MEDIA_REQUEST_VIDEO_BIDIRECTIONAL_DOWNGRADE_VIDEO_TX = 9;
    public static final int MEDIA_REQUEST_VIDEO_BIDIRECTIONAL_DOWNGRADE_VIDEO_RX = 10;

    // Used for call barring methods below
    static final String CB_FACILITY_BAOC         = "AO";
    static final String CB_FACILITY_BAOIC        = "OI";
    static final String CB_FACILITY_BAOICxH      = "OX";
    static final String CB_FACILITY_BAIC         = "AI";
    static final String CB_FACILITY_BAICr        = "IR";
    static final String CB_FACILITY_BA_ALL       = "AB";
    static final String CB_FACILITY_BA_MO        = "AG";
    static final String CB_FACILITY_BA_MT        = "AC";
    static final String CB_FACILITY_BA_SIM       = "SC";
    static final String CB_FACILITY_BA_FD        = "FD";

    static final String CB_FACILITY_BA_PS = "PS";
    static final String CB_FACILITY_BA_PN = "PN";
    static final String CB_FACILITY_BA_PU = "PU";
    static final String CB_FACILITY_BA_PP = "PP";
    static final String CB_FACILITY_BA_PC = "PC";
    static final String CB_FACILITY_BA_PS_PUK = "PSP";
    static final String CB_FACILITY_BA_PN_PUK = "PNP";
    static final String CB_FACILITY_BA_PU_PUK = "PUP";
    static final String CB_FACILITY_BA_PP_PUK = "PPP";
    static final String CB_FACILITY_BA_PC_PUK = "PCP";


    // Used for various supp services apis
    // See 27.007 +CCFC or +CLCK
    static final int SERVICE_CLASS_NONE     = 0; // no user input
    static final int SERVICE_CLASS_VOICE    = (1 << 0);
    static final int SERVICE_CLASS_DATA     = (1 << 1); //synonym for 16+32+64+128
    static final int SERVICE_CLASS_FAX      = (1 << 2);
    static final int SERVICE_CLASS_SMS      = (1 << 3);
    static final int SERVICE_CLASS_DATA_SYNC = (1 << 4);
    static final int SERVICE_CLASS_DATA_ASYNC = (1 << 5);
    static final int SERVICE_CLASS_PACKET   = (1 << 6);
    static final int SERVICE_CLASS_PAD      = (1 << 7);
    static final int SERVICE_CLASS_MAX      = (1 << 7); // Max SERVICE_CLASS value

    CommandsInterface mCi;
    Context mContext;
    RadioInteractorCore mRadioInteractor;
    RadioInteractorHandler mRadioInteractorHandler;


    //***** Instance Variables

    final Integer mPhoneId;

    /** Telephony metrics instance for logging metrics event */
    private TelephonyMetrics mMetrics = TelephonyMetrics.getInstance();
    final RilHandler mRilHandler;


    //***** Constants

    class RilHandler extends Handler {
        //***** Handler implementation
        @Override public void
        handleMessage(Message msg) {

            switch (msg.what) {
                default:
                    riljLog("handleMessage: default what = " + msg.what);
                    break;
            }
        }
    }

    //***** Constructors
    public ImsRIL(Context context, Integer instanceId, CommandsInterface ci) {
        mCi = ci;
        mContext = context;
        mPhoneId = instanceId;
        riljLog("ImsRIL: init mPhoneId=" + instanceId + ")");

        mRilHandler = new RilHandler();
        if(getRadioInteractor() == null){
            RadioInteractorFactory.init(context);
        }
    }

    private RadioInteractorCore getRadioInteractor(){
        if(mRadioInteractor == null && RadioInteractorFactory.getInstance() != null) {
            mRadioInteractor = RadioInteractorFactory.getInstance().getRadioInteractorCore(mPhoneId == null ? 0 : mPhoneId);
        }
        return mRadioInteractor;
    }

    private RadioInteractorHandler getRadioInteractorHandler() {
        if(mRadioInteractorHandler == null && RadioInteractorFactory.getInstance() != null) {
            mRadioInteractorHandler = RadioInteractorFactory.getInstance().getRadioInteractorHandler(mPhoneId == null ? 0 : mPhoneId);
        }
        return mRadioInteractorHandler;
    }

    private String convertNullToEmptyString(String string) {
        return string != null ? string : "";
    }

    public void getVoiceRadioTechnology(Message result) {
        mCi.getVoiceRadioTechnology(result);
    }


    public void getImsRegistrationState(Message result) {
        mCi.getImsRegistrationState(result);
    }

    public void
    getIccCardStatus(Message result) {
        mCi.getIccCardStatus(result);
    }

    public void setUiccSubscription(int slotId, int appIndex, int subId,
            int subStatus, Message result) {
        mCi.setUiccSubscription(slotId, appIndex, subId, subStatus,result);
    }

    // FIXME This API should take an AID and slot ID
    public void setDataAllowed(boolean allowed, Message result) {
        mCi.setDataAllowed(allowed, result);
    }

    public void
    supplyIccPin(String pin, Message result) {
        supplyIccPinForApp(pin, null, result);
    }

    public void
    supplyIccPinForApp(String pin, String aid, Message result) {
        mCi.supplyIccPinForApp(pin, aid, result);
    }

    public void
    supplyIccPuk(String puk, String newPin, Message result) {
        supplyIccPukForApp(puk, newPin, null, result);
    }

    public void
    supplyIccPukForApp(String puk, String newPin, String aid, Message result) {
        mCi.supplyIccPukForApp(puk, newPin, aid, result);
    }

    public void
    supplyIccPin2(String pin, Message result) {
        supplyIccPin2ForApp(pin, null, result);
    }

    public void
    supplyIccPin2ForApp(String pin, String aid, Message result) {
        mCi.supplyIccPin2ForApp(pin, aid, result);
    }

    public void
    supplyIccPuk2(String puk2, String newPin2, Message result) {
        supplyIccPuk2ForApp(puk2, newPin2, null, result);
    }

    public void
    supplyIccPuk2ForApp(String puk, String newPin2, String aid, Message result) {
        mCi.supplyIccPuk2ForApp(puk, newPin2, aid, result);
    }

    public void
    changeIccPin(String oldPin, String newPin, Message result) {
        changeIccPinForApp(oldPin, newPin, null, result);
    }

    public void
    changeIccPinForApp(String oldPin, String newPin, String aid, Message result) {
        mCi.changeIccPinForApp(oldPin, newPin, aid, result);
    }

    public void
    changeIccPin2(String oldPin2, String newPin2, Message result) {
        changeIccPin2ForApp(oldPin2, newPin2, null, result);
    }

    public void
    changeIccPin2ForApp(String oldPin2, String newPin2, String aid, Message result) {
        mCi.changeIccPin2ForApp(oldPin2, newPin2, aid, result);
    }


    public void
    changeBarringPassword(String facility, String oldPwd, String newPwd, Message result) {
        mCi.changeBarringPassword(facility, oldPwd, newPwd, result);
    }


    public void
    supplyNetworkDepersonalization(String netpin, Message result) {
        mCi.supplyNetworkDepersonalization(netpin, result);
    }


    public void
    getCurrentCalls (Message result) {
        mCi.getCurrentCalls(result);
    }


    @Deprecated public void
    getPDPContextList(Message result) {
        mCi.getDataCallList(result);
    }


    public void
    getDataCallList(Message result) {
        mCi.getDataCallList(result);
    }


    public void
    dial (String address, int clirMode, Message result) {
        mCi.dial(address, clirMode, null, result);
    }


    public void
    dial(String address, int clirMode, UUSInfo uusInfo, Message result) {
        mCi.dial(address, clirMode, uusInfo, result);
    }


    public void
    getIMSI(Message result) {
        mCi.getIMSIForApp(null, result);
    }


    public void
    getIMSIForApp(String aid, Message result) {
        mCi.getIMSIForApp(aid, result);
    }


    public void
    getIMEI(Message result) {
        mCi.getIMEI(result);
    }


    public void
    getIMEISV(Message result) {
        mCi.getIMEISV(result);
    }



    public void
    hangupConnection (int gsmIndex, Message result) {
        mCi.hangupConnection(gsmIndex, result);
    }


    public void
    hangupWaitingOrBackground (Message result) {
        mCi.hangupWaitingOrBackground(result);
    }


    public void
    hangupForegroundResumeBackground (Message result) {
        mCi.hangupForegroundResumeBackground(result);
    }


    public void
    switchWaitingOrHoldingAndActive (Message result) {
        mCi.switchWaitingOrHoldingAndActive(result);
    }


    public void
    conference (Message result) {
        mCi.conference(result);
    }



    public void setPreferredVoicePrivacy(boolean enable, Message result) {
        mCi.setPreferredVoicePrivacy(enable, result);
    }


    public void getPreferredVoicePrivacy(Message result) {
        mCi.getPreferredVoicePrivacy(result);
    }


    public void
    separateConnection (int gsmIndex, Message result) {
        mCi.separateConnection(gsmIndex, result);
    }


    public void
    acceptCall (Message result) {
        mCi.acceptCall(result);
    }


    public void
    rejectCall (Message result) {
        mCi.rejectCall(result);
    }


    public void
    explicitCallTransfer (Message result) {
        mCi.explicitCallTransfer(result);
    }


    public void
    getLastCallFailCause (Message result) {
        mCi.getLastCallFailCause(result);
    }

    /**
     * @deprecated
     */
    @Deprecated

    public void
    getLastPdpFailCause (Message result) {
        getLastDataCallFailCause (result);
    }

    /**
     * The preferred new alternative to getLastPdpFailCause
     */

    public void
    getLastDataCallFailCause (Message result) {
        mCi.getLastDataCallFailCause(result);
    }


    public void
    setMute (boolean enableMute, Message response) {
        mCi.setMute(enableMute, response);
    }


    public void
    getMute (Message response) {
        mCi.getMute(response);
    }


    public void
    getSignalStrength (Message result) {
        mCi.getSignalStrength(result);
    }


    public void
    getVoiceRegistrationState (Message result) {
        mCi.getVoiceRegistrationState(result);
    }


    public void
    getDataRegistrationState (Message result) {
        mCi.getDataRegistrationState(result);
    }


    public void
    getOperator(Message result) {
        mCi.getOperator(result);
    }


    public void
    getHardwareConfig (Message result) {
        mCi.getHardwareConfig(result);
    }


    public void
    sendDtmf(char c, Message result) {
        mCi.sendDtmf(c, result);
    }


    public void
    startDtmf(char c, Message result) {
        mCi.startDtmf(c, result);
    }


    public void
    stopDtmf(Message result) {
        mCi.stopDtmf(result);
    }


    public void
    sendBurstDtmf(String dtmfString, int on, int off, Message result) {
        mCi.sendBurstDtmf(dtmfString, on, off, result);
    }

    public void
    sendImsGsmSms (String smscPDU, String pdu, int retry, int messageRef,
            Message result) {
        mCi.sendImsGsmSms(smscPDU, pdu, retry, messageRef, result);
    }


    public void deleteSmsOnSim(int index, Message response) {
        mCi.deleteSmsOnSim(index, response);
    }


    public void deleteSmsOnRuim(int index, Message response) {
        mCi.deleteSmsOnRuim(index, response);
    }


    public void writeSmsToSim(int status, String smsc, String pdu, Message response) {
        mCi.writeSmsToSim(status, smsc, pdu, response);
    }


    public void writeSmsToRuim(int status, String pdu, Message response) {
        mCi.writeSmsToRuim(status, pdu, response);
    }

    public void
    setRadioPower(boolean on, Message result) {
        mCi.setRadioPower(on, result);
    }


    public void requestShutdown(Message result) {
        mCi.requestShutdown(result);
    }


    public void
    setSuppServiceNotifications(boolean enable, Message result) {
        mCi.setSuppServiceNotifications(enable,result);
    }

    public void
    acknowledgeIncomingGsmSmsWithPdu(boolean success, String ackPdu, Message result) {
        mCi.acknowledgeIncomingGsmSmsWithPdu(success, ackPdu, result);
    }


    public void
    iccIO (int command, int fileid, String path, int p1, int p2, int p3,
            String data, String pin2, Message result) {
        iccIOForApp(command, fileid, path, p1, p2, p3, data, pin2, null, result);
    }

    public void
    iccIOForApp (int command, int fileid, String path, int p1, int p2, int p3,
            String data, String pin2, String aid, Message result) {
        mCi.iccIOForApp(command, fileid, path, p1, p2, p3, data, pin2, aid, result);
    }


    public void
    getCLIR(Message result) {
        mCi.getCLIR(result);
    }


    public void
    setCLIR(int clirMode, Message result) {
        mCi.setCLIR(clirMode, result);
    }


    public void
    queryCallWaiting(int serviceClass, Message response) {
        mCi.queryCallWaiting(serviceClass, response);
    }


    public void
    setCallWaiting(boolean enable, int serviceClass, Message response) {
        mCi.setCallWaiting(enable,serviceClass, response);
    }


    public void
    setNetworkSelectionModeAutomatic(Message response) {
        mCi.setNetworkSelectionModeAutomatic(response);
    }


    public void
    setNetworkSelectionModeManual(String operatorNumeric, Message response) {
        mCi.setNetworkSelectionModeManual(operatorNumeric, response);
    }


    public void
    getNetworkSelectionMode(Message response) {
        mCi.getNetworkSelectionMode(response);
    }


    public void
    getAvailableNetworks(Message response) {
        mCi.getAvailableNetworks(response);
    }


    public void
    setCallForward(int action, int cfReason, int serviceClass,
            String number, int timeSeconds, Message response) {
        mCi.setCallForward(action, cfReason, serviceClass,
                number, timeSeconds, response);
    }


    public void
    queryCallForwardStatus(int cfReason, int serviceClass,
            String number, Message response) {
        mCi.queryCallForwardStatus(cfReason, serviceClass, number, response);
    }


    public void
    queryCLIP(Message response) {
        mCi.queryCLIP(response);
    }

    public void
    queryCOLP(Message response) {
        if(getRadioInteractor() != null) {
            mRadioInteractor.queryColp(response);
        }
    }

    public void
    queryCOLR(Message response) {
        if(getRadioInteractor() != null) {
            mRadioInteractor.queryColr(response);
        }
    }

    public void
    getBasebandVersion (Message response) {
        mCi.getBasebandVersion(response);
    }


    public void
    queryFacilityLock(String facility, String password, int serviceClass,
            Message response) {
        queryFacilityLockForApp(facility, password, serviceClass, null, response);
    }


    public void
    queryFacilityLockForApp(String facility, String password, int serviceClass, String appId,
            Message response) {
        mCi.queryFacilityLockForApp(facility, password, serviceClass, appId, response);
    }

    public void
    queryFacilityLockForAppExt(String facility, String password, int serviceClass,
                               Message response) {
        if(getRadioInteractor() != null) {
            mRadioInteractor.queryFacilityLockForAppExt(convertNullToEmptyString(facility),
                    convertNullToEmptyString(password), serviceClass, response);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_QUERY_FACILITY_LOCK_EXT));
        } else {
            riljLog("queryFacilityLockForAppExt, RadioInteractor is null");
        }
    }

    public void
    setFacilityLock (String facility, boolean lockState, String password,
            int serviceClass, Message response) {
        setFacilityLockForApp(facility, lockState, password, serviceClass, null, response);
    }


    public void
    setFacilityLockForApp(String facility, boolean lockState, String password,
            int serviceClass, String appId, Message response) {
        mCi.setFacilityLockForApp(facility, lockState, password, serviceClass, appId, response);
    }


    public void
    sendUSSD (String ussdString, Message response) {
        mCi.sendUSSD(ussdString, response);
    }

    // inherited javadoc suffices

    public void cancelPendingUssd (Message response) {
        mCi.cancelPendingUssd(response);
    }



    public void resetRadio(Message result) {
        mCi.resetRadio(result);
    }


    public void invokeOemRilRequestRaw(String cmd, Message response) {
        if(getRadioInteractor() != null) {
            mRadioInteractor.sendCmdAsync(cmd,response);
        }
    }

    /**
     * Assign a specified band for RF configuration.
     *
     * @param bandMode one of BM_*_BAND
     * @param response is callback message
     */

    public void setBandMode (int bandMode, Message response) {
        mCi.setBandMode(bandMode, response);
    }

    /**
     * Query the list of band mode supported by RF.
     *
     * @param response is callback message
     *        ((AsyncResult)response.obj).result  is an int[] where int[0] is
     *        the size of the array and the rest of each element representing
     *        one available BM_*_BAND
     */

    public void queryAvailableBandMode (Message response) {
        mCi.queryAvailableBandMode(response);
    }

    /**
     * {@inheritDoc}
     */

    public void sendTerminalResponse(String contents, Message response) {
        mCi.sendTerminalResponse(contents, response);
    }

    /**
     * {@inheritDoc}
     */

    public void sendEnvelope(String contents, Message response) {
        mCi.sendEnvelope(contents, response);
    }

    /**
     * {@inheritDoc}
     */

    public void sendEnvelopeWithStatus(String contents, Message response) {
        mCi.sendEnvelopeWithStatus(contents, response);
    }

    /**
     * {@inheritDoc}
     */

    public void handleCallSetupRequestFromSim(
            boolean accept, Message response) {
        mCi.handleCallSetupRequestFromSim(accept, response);
    }

    /**
     * {@inheritDoc}
     */

    public void setPreferredNetworkType(int networkType , Message response) {
        mCi.setPreferredNetworkType(networkType, response);
    }

    /**
     * {@inheritDoc}
     */

    public void getPreferredNetworkType(Message response) {
        mCi.getPreferredNetworkType(response);
    }



    /**
     * {@inheritDoc}
     */

    public void setLocationUpdates(boolean enable, Message response) {
        mCi.setLocationUpdates(enable, response);
    }

    /**
     * {@inheritDoc}
     */

    public void getSmscAddress(Message result) {
        mCi.getSmscAddress(result);
    }

    /**
     * {@inheritDoc}
     */

    public void setSmscAddress(String address, Message result) {
        mCi.setSmscAddress(address, result);
    }

    /**
     * {@inheritDoc}
     */

    public void reportSmsMemoryStatus(boolean available, Message result) {
        mCi.reportSmsMemoryStatus(available, result);
    }

    /**
     * {@inheritDoc}
     */

    public void reportStkServiceIsRunning(Message result) {
        mCi.reportStkServiceIsRunning(result);
    }

    /**
     * {@inheritDoc}
     */

    public void getGsmBroadcastConfig(Message response) {
        mCi.getGsmBroadcastConfig(response);
    }

    /**
     * {@inheritDoc}
     */

    public void setGsmBroadcastConfig(SmsBroadcastConfigInfo[] config, Message response) {
        mCi.setGsmBroadcastConfig(config, response);
    }

    /**
     * {@inheritDoc}
     */

    public void setGsmBroadcastActivation(boolean activate, Message response) {
        mCi.setGsmBroadcastActivation(activate, response);
    }

    public  void unregisterForSrvccStateChanged(Handler h){
        mCi.unregisterForSrvccStateChanged(h);
    }

//***** google default Private Methods

    static String retToString(int req, Object ret) {
        if (ret == null) return "";
        switch (req) {
            // Don't log these return values, for privacy's sake.
            case RIL_REQUEST_GET_IMSI:
            case RIL_REQUEST_GET_IMEI:
            case RIL_REQUEST_GET_IMEISV:
            case RIL_REQUEST_SIM_OPEN_CHANNEL:
            case RIL_REQUEST_SIM_TRANSMIT_APDU_CHANNEL:

                if (!RILJ_LOGV) {
                    // If not versbose logging just return and don't display IMSI and IMEI, IMEISV
                    return "";
                }
        }

        StringBuilder sb;
        String s;
        int length;
        if (ret instanceof int[]) {
            int[] intArray = (int[]) ret;
            length = intArray.length;
            sb = new StringBuilder("{");
            if (length > 0) {
                int i = 0;
                sb.append(intArray[i++]);
                while (i < length) {
                    sb.append(", ").append(intArray[i++]);
                }
            }
            sb.append("}");
            s = sb.toString();
        } else if (ret instanceof String[]) {
            String[] strings = (String[]) ret;
            length = strings.length;
            sb = new StringBuilder("{");
            if (length > 0) {
                int i = 0;
                sb.append(strings[i++]);
                while (i < length) {
                    sb.append(", ").append(strings[i++]);
                }
            }
            sb.append("}");
            s = sb.toString();
        } else if (req == RIL_REQUEST_GET_CURRENT_CALLS) {
            ArrayList<DriverCall> calls = (ArrayList<DriverCall>) ret;
            sb = new StringBuilder("{");
            for (DriverCall dc : calls) {
                sb.append("[").append(dc).append("] ");
            }
            sb.append("}");
            s = sb.toString();
        } else if (req == RIL_REQUEST_GET_NEIGHBORING_CELL_IDS) {
            ArrayList<NeighboringCellInfo> cells = (ArrayList<NeighboringCellInfo>) ret;
            sb = new StringBuilder("{");
            for (NeighboringCellInfo cell : cells) {
                sb.append("[").append(cell).append("] ");
            }
            sb.append("}");
            s = sb.toString();
        } else if (req == RIL_REQUEST_QUERY_CALL_FORWARD_STATUS) {
            CallForwardInfo[] cinfo = (CallForwardInfo[]) ret;
            length = cinfo.length;
            sb = new StringBuilder("{");
            for (int i = 0; i < length; i++) {
                sb.append("[").append(cinfo[i]).append("] ");
            }
            sb.append("}");
            s = sb.toString();
        } else if (req == RIL_REQUEST_GET_HARDWARE_CONFIG) {
            ArrayList<HardwareConfig> hwcfgs = (ArrayList<HardwareConfig>) ret;
            sb = new StringBuilder(" ");
            for (HardwareConfig hwcfg : hwcfgs) {
                sb.append("[").append(hwcfg).append("] ");
            }
            s = sb.toString();
        } else {
            s = ret.toString();
        }
        return s;
    }

    void writeMetricsNewSms(int tech, int format) {
        mMetrics.writeRilNewSms(mPhoneId, tech, format);
    }

    void writeMetricsCallRing(char[] response) {
        mMetrics.writeRilCallRing(mPhoneId, response);
    }

    void writeMetricsSrvcc(int state) {
        mMetrics.writeRilSrvcc(mPhoneId, state);
    }

    void writeMetricsModemRestartEvent(String reason) {
        mMetrics.writeModemRestartEvent(mPhoneId, reason);
    }


    static String
    requestToString(int request) {
        switch(request) {
            case RIL_REQUEST_GET_SIM_STATUS: return "GET_SIM_STATUS";
            case RIL_REQUEST_ENTER_SIM_PIN: return "ENTER_SIM_PIN";
            case RIL_REQUEST_ENTER_SIM_PUK: return "ENTER_SIM_PUK";
            case RIL_REQUEST_ENTER_SIM_PIN2: return "ENTER_SIM_PIN2";
            case RIL_REQUEST_ENTER_SIM_PUK2: return "ENTER_SIM_PUK2";
            case RIL_REQUEST_CHANGE_SIM_PIN: return "CHANGE_SIM_PIN";
            case RIL_REQUEST_CHANGE_SIM_PIN2: return "CHANGE_SIM_PIN2";
            case RIL_REQUEST_ENTER_NETWORK_DEPERSONALIZATION: return "ENTER_NETWORK_DEPERSONALIZATION";
            case RIL_REQUEST_GET_CURRENT_CALLS: return "GET_CURRENT_CALLS";
            case RIL_REQUEST_DIAL: return "DIAL";
            case RIL_REQUEST_GET_IMSI: return "GET_IMSI";
            case RIL_REQUEST_HANGUP: return "HANGUP";
            case RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND: return "HANGUP_WAITING_OR_BACKGROUND";
            case RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND: return "HANGUP_FOREGROUND_RESUME_BACKGROUND";
            case RIL_REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE: return "REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE";
            case RIL_REQUEST_CONFERENCE: return "CONFERENCE";
            case RIL_REQUEST_UDUB: return "UDUB";
            case RIL_REQUEST_LAST_CALL_FAIL_CAUSE: return "LAST_CALL_FAIL_CAUSE";
            case RIL_REQUEST_SIGNAL_STRENGTH: return "SIGNAL_STRENGTH";
            case RIL_REQUEST_VOICE_REGISTRATION_STATE: return "VOICE_REGISTRATION_STATE";
            case RIL_REQUEST_DATA_REGISTRATION_STATE: return "DATA_REGISTRATION_STATE";
            case RIL_REQUEST_OPERATOR: return "OPERATOR";
            case RIL_REQUEST_RADIO_POWER: return "RADIO_POWER";
            case RIL_REQUEST_DTMF: return "DTMF";
            case RIL_REQUEST_SEND_SMS: return "SEND_SMS";
            case RIL_REQUEST_SEND_SMS_EXPECT_MORE: return "SEND_SMS_EXPECT_MORE";
            case RIL_REQUEST_SETUP_DATA_CALL: return "SETUP_DATA_CALL";
            case RIL_REQUEST_SIM_IO: return "SIM_IO";
            case RIL_REQUEST_SEND_USSD: return "SEND_USSD";
            case RIL_REQUEST_CANCEL_USSD: return "CANCEL_USSD";
            case RIL_REQUEST_GET_CLIR: return "GET_CLIR";
            case RIL_REQUEST_SET_CLIR: return "SET_CLIR";
            case RIL_REQUEST_QUERY_CALL_FORWARD_STATUS: return "QUERY_CALL_FORWARD_STATUS";
            case RIL_REQUEST_SET_CALL_FORWARD: return "SET_CALL_FORWARD";
            case RIL_REQUEST_QUERY_CALL_WAITING: return "QUERY_CALL_WAITING";
            case RIL_REQUEST_SET_CALL_WAITING: return "SET_CALL_WAITING";
            case RIL_REQUEST_SMS_ACKNOWLEDGE: return "SMS_ACKNOWLEDGE";
            case RIL_REQUEST_GET_IMEI: return "GET_IMEI";
            case RIL_REQUEST_GET_IMEISV: return "GET_IMEISV";
            case RIL_REQUEST_ANSWER: return "ANSWER";
            case RIL_REQUEST_DEACTIVATE_DATA_CALL: return "DEACTIVATE_DATA_CALL";
            case RIL_REQUEST_QUERY_FACILITY_LOCK: return "QUERY_FACILITY_LOCK";
            case RIL_REQUEST_SET_FACILITY_LOCK: return "SET_FACILITY_LOCK";
            case RIL_REQUEST_CHANGE_BARRING_PASSWORD: return "CHANGE_BARRING_PASSWORD";
            case RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE: return "QUERY_NETWORK_SELECTION_MODE";
            case RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC: return "SET_NETWORK_SELECTION_AUTOMATIC";
            case RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL: return "SET_NETWORK_SELECTION_MANUAL";
            case RIL_REQUEST_QUERY_AVAILABLE_NETWORKS : return "QUERY_AVAILABLE_NETWORKS ";
            case RIL_REQUEST_DTMF_START: return "DTMF_START";
            case RIL_REQUEST_DTMF_STOP: return "DTMF_STOP";
            case RIL_REQUEST_BASEBAND_VERSION: return "BASEBAND_VERSION";
            case RIL_REQUEST_SEPARATE_CONNECTION: return "SEPARATE_CONNECTION";
            case RIL_REQUEST_SET_MUTE: return "SET_MUTE";
            case RIL_REQUEST_GET_MUTE: return "GET_MUTE";
            case RIL_REQUEST_QUERY_CLIP: return "QUERY_CLIP";
            case RIL_REQUEST_LAST_DATA_CALL_FAIL_CAUSE: return "LAST_DATA_CALL_FAIL_CAUSE";
            case RIL_REQUEST_DATA_CALL_LIST: return "DATA_CALL_LIST";
            case RIL_REQUEST_RESET_RADIO: return "RESET_RADIO";
            case RIL_REQUEST_OEM_HOOK_RAW: return "OEM_HOOK_RAW";
            case RIL_REQUEST_OEM_HOOK_STRINGS: return "OEM_HOOK_STRINGS";
            case RIL_REQUEST_SCREEN_STATE: return "SCREEN_STATE";
            case RIL_REQUEST_SET_SUPP_SVC_NOTIFICATION: return "SET_SUPP_SVC_NOTIFICATION";
            case RIL_REQUEST_WRITE_SMS_TO_SIM: return "WRITE_SMS_TO_SIM";
            case RIL_REQUEST_DELETE_SMS_ON_SIM: return "DELETE_SMS_ON_SIM";
            case RIL_REQUEST_SET_BAND_MODE: return "SET_BAND_MODE";
            case RIL_REQUEST_QUERY_AVAILABLE_BAND_MODE: return "QUERY_AVAILABLE_BAND_MODE";
            case RIL_REQUEST_STK_GET_PROFILE: return "REQUEST_STK_GET_PROFILE";
            case RIL_REQUEST_STK_SET_PROFILE: return "REQUEST_STK_SET_PROFILE";
            case RIL_REQUEST_STK_SEND_ENVELOPE_COMMAND: return "REQUEST_STK_SEND_ENVELOPE_COMMAND";
            case RIL_REQUEST_STK_SEND_TERMINAL_RESPONSE: return "REQUEST_STK_SEND_TERMINAL_RESPONSE";
            case RIL_REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM: return "REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM";
            case RIL_REQUEST_EXPLICIT_CALL_TRANSFER: return "REQUEST_EXPLICIT_CALL_TRANSFER";
            case RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE: return "REQUEST_SET_PREFERRED_NETWORK_TYPE";
            case RIL_REQUEST_GET_PREFERRED_NETWORK_TYPE: return "REQUEST_GET_PREFERRED_NETWORK_TYPE";
            case RIL_REQUEST_GET_NEIGHBORING_CELL_IDS: return "REQUEST_GET_NEIGHBORING_CELL_IDS";
            case RIL_REQUEST_SET_LOCATION_UPDATES: return "REQUEST_SET_LOCATION_UPDATES";
            case RIL_REQUEST_CDMA_SET_SUBSCRIPTION_SOURCE: return "RIL_REQUEST_CDMA_SET_SUBSCRIPTION_SOURCE";
            case RIL_REQUEST_CDMA_SET_ROAMING_PREFERENCE: return "RIL_REQUEST_CDMA_SET_ROAMING_PREFERENCE";
            case RIL_REQUEST_CDMA_QUERY_ROAMING_PREFERENCE: return "RIL_REQUEST_CDMA_QUERY_ROAMING_PREFERENCE";
            case RIL_REQUEST_SET_TTY_MODE: return "RIL_REQUEST_SET_TTY_MODE";
            case RIL_REQUEST_QUERY_TTY_MODE: return "RIL_REQUEST_QUERY_TTY_MODE";
            case RIL_REQUEST_CDMA_SET_PREFERRED_VOICE_PRIVACY_MODE: return "RIL_REQUEST_CDMA_SET_PREFERRED_VOICE_PRIVACY_MODE";
            case RIL_REQUEST_CDMA_QUERY_PREFERRED_VOICE_PRIVACY_MODE: return "RIL_REQUEST_CDMA_QUERY_PREFERRED_VOICE_PRIVACY_MODE";
            case RIL_REQUEST_CDMA_FLASH: return "RIL_REQUEST_CDMA_FLASH";
            case RIL_REQUEST_CDMA_BURST_DTMF: return "RIL_REQUEST_CDMA_BURST_DTMF";
            case RIL_REQUEST_CDMA_SEND_SMS: return "RIL_REQUEST_CDMA_SEND_SMS";
            case RIL_REQUEST_CDMA_SMS_ACKNOWLEDGE: return "RIL_REQUEST_CDMA_SMS_ACKNOWLEDGE";
            case RIL_REQUEST_GSM_GET_BROADCAST_CONFIG: return "RIL_REQUEST_GSM_GET_BROADCAST_CONFIG";
            case RIL_REQUEST_GSM_SET_BROADCAST_CONFIG: return "RIL_REQUEST_GSM_SET_BROADCAST_CONFIG";
            case RIL_REQUEST_CDMA_GET_BROADCAST_CONFIG: return "RIL_REQUEST_CDMA_GET_BROADCAST_CONFIG";
            case RIL_REQUEST_CDMA_SET_BROADCAST_CONFIG: return "RIL_REQUEST_CDMA_SET_BROADCAST_CONFIG";
            case RIL_REQUEST_GSM_BROADCAST_ACTIVATION: return "RIL_REQUEST_GSM_BROADCAST_ACTIVATION";
            case RIL_REQUEST_CDMA_VALIDATE_AND_WRITE_AKEY: return "RIL_REQUEST_CDMA_VALIDATE_AND_WRITE_AKEY";
            case RIL_REQUEST_CDMA_BROADCAST_ACTIVATION: return "RIL_REQUEST_CDMA_BROADCAST_ACTIVATION";
            case RIL_REQUEST_CDMA_SUBSCRIPTION: return "RIL_REQUEST_CDMA_SUBSCRIPTION";
            case RIL_REQUEST_CDMA_WRITE_SMS_TO_RUIM: return "RIL_REQUEST_CDMA_WRITE_SMS_TO_RUIM";
            case RIL_REQUEST_CDMA_DELETE_SMS_ON_RUIM: return "RIL_REQUEST_CDMA_DELETE_SMS_ON_RUIM";
            case RIL_REQUEST_DEVICE_IDENTITY: return "RIL_REQUEST_DEVICE_IDENTITY";
            case RIL_REQUEST_GET_SMSC_ADDRESS: return "RIL_REQUEST_GET_SMSC_ADDRESS";
            case RIL_REQUEST_SET_SMSC_ADDRESS: return "RIL_REQUEST_SET_SMSC_ADDRESS";
            case RIL_REQUEST_EXIT_EMERGENCY_CALLBACK_MODE: return "REQUEST_EXIT_EMERGENCY_CALLBACK_MODE";
            case RIL_REQUEST_REPORT_SMS_MEMORY_STATUS: return "RIL_REQUEST_REPORT_SMS_MEMORY_STATUS";
            case RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING: return "RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING";
            case RIL_REQUEST_CDMA_GET_SUBSCRIPTION_SOURCE: return "RIL_REQUEST_CDMA_GET_SUBSCRIPTION_SOURCE";
            case RIL_REQUEST_ISIM_AUTHENTICATION: return "RIL_REQUEST_ISIM_AUTHENTICATION";
            case RIL_REQUEST_ACKNOWLEDGE_INCOMING_GSM_SMS_WITH_PDU: return "RIL_REQUEST_ACKNOWLEDGE_INCOMING_GSM_SMS_WITH_PDU";
            case RIL_REQUEST_STK_SEND_ENVELOPE_WITH_STATUS: return "RIL_REQUEST_STK_SEND_ENVELOPE_WITH_STATUS";
            case RIL_REQUEST_VOICE_RADIO_TECH: return "RIL_REQUEST_VOICE_RADIO_TECH";
            case RIL_REQUEST_GET_CELL_INFO_LIST: return "RIL_REQUEST_GET_CELL_INFO_LIST";
            case RIL_REQUEST_SET_UNSOL_CELL_INFO_LIST_RATE: return "RIL_REQUEST_SET_CELL_INFO_LIST_RATE";
            case RIL_REQUEST_SET_INITIAL_ATTACH_APN: return "RIL_REQUEST_SET_INITIAL_ATTACH_APN";
            case RIL_REQUEST_SET_DATA_PROFILE: return "RIL_REQUEST_SET_DATA_PROFILE";
            case RIL_REQUEST_IMS_REGISTRATION_STATE: return "RIL_REQUEST_IMS_REGISTRATION_STATE";
            case RIL_REQUEST_IMS_SEND_SMS: return "RIL_REQUEST_IMS_SEND_SMS";
            case RIL_REQUEST_SIM_TRANSMIT_APDU_BASIC: return "RIL_REQUEST_SIM_TRANSMIT_APDU_BASIC";
            case RIL_REQUEST_SIM_OPEN_CHANNEL: return "RIL_REQUEST_SIM_OPEN_CHANNEL";
            case RIL_REQUEST_SIM_CLOSE_CHANNEL: return "RIL_REQUEST_SIM_CLOSE_CHANNEL";
            case RIL_REQUEST_SIM_TRANSMIT_APDU_CHANNEL: return "RIL_REQUEST_SIM_TRANSMIT_APDU_CHANNEL";
            case RIL_REQUEST_NV_READ_ITEM: return "RIL_REQUEST_NV_READ_ITEM";
            case RIL_REQUEST_NV_WRITE_ITEM: return "RIL_REQUEST_NV_WRITE_ITEM";
            case RIL_REQUEST_NV_WRITE_CDMA_PRL: return "RIL_REQUEST_NV_WRITE_CDMA_PRL";
            case RIL_REQUEST_NV_RESET_CONFIG: return "RIL_REQUEST_NV_RESET_CONFIG";
            case RIL_REQUEST_SET_UICC_SUBSCRIPTION: return "RIL_REQUEST_SET_UICC_SUBSCRIPTION";
            case RIL_REQUEST_ALLOW_DATA: return "RIL_REQUEST_ALLOW_DATA";
            case RIL_REQUEST_GET_HARDWARE_CONFIG: return "GET_HARDWARE_CONFIG";
            case RIL_REQUEST_SIM_AUTHENTICATION: return "RIL_REQUEST_SIM_AUTHENTICATION";
            case RIL_REQUEST_SHUTDOWN: return "RIL_REQUEST_SHUTDOWN";
            case RIL_REQUEST_SET_RADIO_CAPABILITY:
                return "RIL_REQUEST_SET_RADIO_CAPABILITY";
            case RIL_REQUEST_GET_RADIO_CAPABILITY:
                return "RIL_REQUEST_GET_RADIO_CAPABILITY";
            case RIL_REQUEST_START_LCE: return "RIL_REQUEST_START_LCE";
            case RIL_REQUEST_STOP_LCE: return "RIL_REQUEST_STOP_LCE";
            case RIL_REQUEST_PULL_LCEDATA: return "RIL_REQUEST_PULL_LCEDATA";
            case RIL_REQUEST_GET_ACTIVITY_INFO: return "RIL_REQUEST_GET_ACTIVITY_INFO";
            default: return "<unknown request>";
        }
    }

    static String
    responseToString(int request)
    {
        /*
 cat libs/telephony/ril_unsol_commands.h \
 | egrep "^ *{RIL_" \
 | sed -re 's/\{RIL_([^,]+),[^,]+,([^}]+).+/case RIL_\1: return "\1";/'
         */
        switch(request) {
            case ImsRILConstants.RIL_UNSOL_IMS_NETWORK_STATE_CHANGED:
                return "UNSOL_RESPONSE_IMS_NETWORK_STATE_CHANGED";
            case RIL_UNSOL_SRVCC_STATE_NOTIFY:
                return "UNSOL_SRVCC_STATE_NOTIFY";
            case ImsRILConstants.RIL_UNSOL_RESPONSE_IMS_CALL_STATE_CHANGED: return " RIL_UNSOL_RESPONSE_IMS_CALL_STATE_CHANGED";
            case ImsRILConstants.RIL_UNSOL_RESPONSE_IMS_BEARER_ESTABLISTED: return "RIL_UNSOL_RESPONSE_IMS_BEARER_ESTABLISTED";
            case ImsRILConstants.RIL_UNSOL_IMS_REGISTER_ADDRESS_CHANGE: return "RIL_UNSOL_IMS_REGISTER_ADDRESS_CHANGE";
            default: return "<unknown response>";
        }
    }

    void riljLog(String msg) {
        Rlog.d(RILJ_LOG_TAG, msg
                + (mPhoneId != null ? (" [SUB" + mPhoneId + "]") : ""));
    }

    void riljLoge(String msg) {
        Rlog.e(RILJ_LOG_TAG, msg
                + (mPhoneId != null ? (" [SUB" + mPhoneId + "]") : ""));
    }

    void riljLoge(String msg, Exception e) {
        Rlog.e(RILJ_LOG_TAG, msg
                + (mPhoneId != null ? (" [SUB" + mPhoneId + "]") : ""), e);
    }

    void riljLogv(String msg) {
        Rlog.v(RILJ_LOG_TAG, msg
                + (mPhoneId != null ? (" [SUB" + mPhoneId + "]") : ""));
    }

    void unsljLog(int response) {
        riljLog("[UNSL]< " + responseToString(response));
    }

    void unsljLogMore(int response, String more) {
        riljLog("[UNSL]< " + responseToString(response) + " " + more);
    }

    void unsljLogRet(int response, Object ret) {
        riljLog("[UNSL]< " + responseToString(response) + " " + retToString(response, ret));
    }

    void unsljLogvRet(int response, Object ret) {
        riljLogv("[UNSL]< " + responseToString(response) + " " + retToString(response, ret));
    }

    /**
     * {@inheritDoc}
     */

    public void exitEmergencyCallbackMode(Message response) {
        mCi.exitEmergencyCallbackMode(response);
    }

    public void requestIccSimAuthentication(int authContext, String data, String aid,
            Message response) {
        mCi.requestIccSimAuthentication(authContext, data, aid, response);
    }

    public void setInitialAttachSOSApn(DataProfile dataProfileInfo, Message result){
        if(getRadioInteractor() != null) {
            mRadioInteractor.setInitialAttachSOSApn(dataProfileInfo,result);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_SET_SOS_INITIAL_ATTACH_APN));
        } else {
            riljLog("setInitialAttachSOSApn, RadioInteractor is null");
        }
    }

    public void setIMSInitialAttachApn(DataProfile dataProfileInfo, Message result){
        if(getRadioInteractor() != null) {
            mRadioInteractor.setIMSInitialAttachApn(dataProfileInfo,result);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_SET_IMS_INITIAL_ATTACH_APN));
        } else {
            riljLog("setIMSInitialAttachApn, RadioInteractor is null");
        }
    }

    public static ArrayList<Byte> primitiveArrayToArrayList(byte[] arr) {
        ArrayList<Byte> arrayList = new ArrayList<>(arr.length);
        for (byte b : arr) {
            arrayList.add(b);
        }
        return arrayList;
    }

    public static byte[] arrayListToPrimitiveArray(ArrayList<Byte> bytes) {
        byte[] ret = new byte[bytes.size()];
        for (int i = 0; i < ret.length; i++) {
            ret[i] = bytes.get(i);
        }
        return ret;
    }

    /*==============SPRD implement=================*/
    public void dialVP(String address, String sub_address, int clirMode, Message result) {
        riljLog("ril--dialVP: address = " + address);
        if(getRadioInteractor() != null) {
            mRadioInteractor.videoPhoneDial(address, sub_address, clirMode, result);
        } else {
            riljLog("dialVP, RadioInteractor is null");
        }
    }


    public void
    getImsCurrentCalls (Message result) {
        if(getRadioInteractor() != null) {
            mRadioInteractor.getImsCurrentCalls(result);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_GET_IMS_CURRENT_CALLS));
        } else {
            riljLog("getIMSCurrentCalls, RadioInteractor is null");
        }
    }

    public void
    setImsVoiceCallAvailability(int state, Message response) {
        if(getRadioInteractor() != null) {
            mRadioInteractor.setImsVoiceCallAvailability(state,response);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_SET_IMS_VOICE_CALL_AVAILABILITY));
        } else {
            riljLog("getIMSCurrentCalls, RadioInteractor is null");
        }
    }


    public void
    getImsVoiceCallAvailability(Message response){
        if(getRadioInteractor() != null) {
            mRadioInteractor.getImsVoiceCallAvailability(response);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_GET_IMS_VOICE_CALL_AVAILABILITY));
        } else {
            riljLog("getImsVoiceCallAvailability, RadioInteractor is null");
        }
    }


    public void initISIM(String confUri, String instanceId, String impu,
            String impi, String domain, String xCap, String bspAddr,
            Message response) {
        if(getRadioInteractor() != null) {
            mRadioInteractor.initISIM(confUri,instanceId,impu,impi,domain,xCap,bspAddr,response);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_INIT_ISIM));
        } else {
            riljLog("initISIM, RadioInteractor is null");
        }
    }

    public void disableIms(Message response){
        if(getRadioInteractor() != null) {
            mRadioInteractor.disableIms(response);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_DISABLE_IMS));
        } else {
            riljLog("disableIms, RadioInteractor is null");
        }
    }


    public void
    requestVolteCallMediaChange(int action, int callId, Message response) {
        if(getRadioInteractor() != null) {
            mRadioInteractor.requestVolteCallMediaChange(action,callId,response);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_IMS_CALL_REQUEST_MEDIA_CHANGE));
        } else {
            riljLog("requestVolteCallMediaChange, RadioInteractor is null");
        }
    }


    public void
    responseVolteCallMediaChange(boolean isAccept, int callId, Message response) {
        if(getRadioInteractor() != null) {
            mRadioInteractor.responseVolteCallMediaChange(isAccept,callId,response);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_IMS_CALL_RESPONSE_MEDIA_CHANGE));
        } else {
            riljLog("responseVolteCallMediaChange, RadioInteractor is null");
        }
    }


    public void setImsSmscAddress(String smsc, Message response){
        if(getRadioInteractor() != null) {
            mRadioInteractor.setImsSmscAddress(smsc,response);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_SET_IMS_SMSC));
        } else {
            riljLog("setImsSmscAddress, RadioInteractor is null");
        }
    }


    public void requestVolteCallFallBackToVoice(int callId, Message response) {
        if(getRadioInteractor() != null) {
            mRadioInteractor.requestVolteCallFallBackToVoice(callId,response);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_IMS_CALL_FALL_BACK_TO_VOICE));
        } else {
            riljLog("requestVolteCallFallBackToVoice, RadioInteractor is null");
        }
    }

    public void
    requestInitialGroupCall(String numbers, Message response) {
        if(getRadioInteractor() != null) {
            mRadioInteractor.requestInitialGroupCall(numbers,response);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_IMS_CALL_FALL_BACK_TO_VOICE));
        } else {
            riljLog(imsRequestToString(ImsRILConstants.RIL_REQUEST_IMS_CALL_FALL_BACK_TO_VOICE)+" RadioInteractor is null");
        }
    }


    public void
    requestAddGroupCall(String numbers, Message response) {
        if(getRadioInteractor() != null) {
            mRadioInteractor.requestAddGroupCall(numbers,response);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_IMS_ADD_TO_GROUP_CALL));
        } else {
            riljLog(imsRequestToString(ImsRILConstants.RIL_REQUEST_IMS_ADD_TO_GROUP_CALL)+" RadioInteractor is null");
        }
    }

    public void
    setCallForward(int action, int cfReason, int serviceClass,
            String number, int timeSeconds, String ruleSet, Message response) {
        if(getRadioInteractor() != null) {
            mRadioInteractor.setCallForward(action,cfReason,serviceClass,number,timeSeconds,ruleSet,response);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_SET_CALL_FORWARD_URI));
        } else {
            riljLog(imsRequestToString(ImsRILConstants.RIL_REQUEST_SET_CALL_FORWARD_URI)+" RadioInteractor is null");
        }
    }

    public void
    queryCallForwardStatus(int cfReason, int serviceClass,
            String number, String ruleSet, Message response) {
        if(getRadioInteractor() != null) {
            mRadioInteractor.queryCallForwardStatus(cfReason,serviceClass,number,ruleSet,response);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_QUERY_CALL_FORWARD_STATUS_URI));
        } else {
            riljLog(imsRequestToString(ImsRILConstants.RIL_REQUEST_QUERY_CALL_FORWARD_STATUS_URI)+" RadioInteractor is null");
        }
    }

    public void enableIms(Message response) {
        if(getRadioInteractor() != null) {
            mRadioInteractor.enableIms(response);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_ENABLE_IMS));
        } else {
            riljLog(imsRequestToString(ImsRILConstants.RIL_REQUEST_ENABLE_IMS)+" RadioInteractor is null");
        }
    }

    public void registerForImsCallStateChanged(Handler h, int what, Object obj) {
        if(getRadioInteractor() != null) {
            mRadioInteractor.registerForImsCallStateChanged(h,what,obj);
        } else {
            riljLog("registerForImsCallStateChanged, RadioInteractor is null");
        }
    }

    public void unregisterForImsCallStateChanged(Handler h) {
        if(getRadioInteractor() != null) {
            mRadioInteractor.unregisterForImsCallStateChanged(h);
        } else {
            riljLog("unregisterForImsCallStateChanged, RadioInteractor is null");
        }
    }

    public void registerForRadioStateChanged(Handler h, int what, Object obj) {
        mCi.registerForRadioStateChanged(h, what, obj);
    }

    public void unregisterForRadioStateChanged(Handler h) {
        mCi.unregisterForRadioStateChanged(h);
    }

    public void registerForCallStateChanged(Handler h, int what, Object obj) {
        mCi.registerForCallStateChanged(h,what,obj);
    }

    public void unregisterForCallStateChanged(Handler h) {
        mCi.unregisterForCallStateChanged(h);
    }

    public void registerForSrvccStateChanged(Handler h, int what, Object obj) {
        mCi.registerForSrvccStateChanged(h, what, obj);
    }

    static String
    imsRequestToString(int request) {
        switch(request) {
            case ImsRILConstants.RIL_REQUEST_GET_IMS_CURRENT_CALLS: return "RIL_REQUEST_GET_IMS_CURRENT_CALLS";
            case ImsRILConstants.RIL_REQUEST_SET_IMS_VOICE_CALL_AVAILABILITY: return "RIL_REQUEST_SET_IMS_VOICE_CALL_AVAILABILITY";
            case ImsRILConstants.RIL_REQUEST_GET_IMS_VOICE_CALL_AVAILABILITY: return "RIL_REQUEST_GET_IMS_VOICE_CALL_AVAILABILITY";
            case ImsRILConstants.RIL_REQUEST_INIT_ISIM: return "RIL_REQUEST_INIT_ISIM";
            case ImsRILConstants.RIL_REQUEST_DISABLE_IMS: return "RIL_REQUEST_DISABLE_IMS";
            case ImsRILConstants.RIL_REQUEST_IMS_CALL_REQUEST_MEDIA_CHANGE : return "RIL_REQUEST_IMS_CALL_REQUEST_MEDIA_CHANGE ";
            case ImsRILConstants.RIL_REQUEST_IMS_CALL_RESPONSE_MEDIA_CHANGE: return "RIL_REQUEST_IMS_CALL_RESPONSE_MEDIA_CHANGE";
            case ImsRILConstants.RIL_REQUEST_SET_IMS_SMSC: return "RIL_REQUEST_SET_IMS_SMSC";
            case ImsRILConstants.RIL_REQUEST_IMS_CALL_FALL_BACK_TO_VOICE: return "RIL_REQUEST_IMS_CALL_FALL_BACK_TO_VOICE";
            case ImsRILConstants.RIL_REQUEST_SET_IMS_INITIAL_ATTACH_APN: return "RIL_REQUEST_SET_IMS_INITIAL_ATTACH_APN";
            case ImsRILConstants.RIL_REQUEST_QUERY_CALL_FORWARD_STATUS_URI: return "RIL_REQUEST_QUERY_CALL_FORWARD_STATUS_URI";
            case ImsRILConstants.RIL_REQUEST_SET_CALL_FORWARD_URI: return "RIL_REQUEST_SET_CALL_FORWARD_URI";
            case ImsRILConstants.RIL_REQUEST_IMS_INITIAL_GROUP_CALL: return "RIL_REQUEST_IMS_INITIAL_GROUP_CALL";
            case ImsRILConstants.RIL_REQUEST_IMS_ADD_TO_GROUP_CALL: return "RIL_REQUEST_IMS_ADD_TO_GROUP_CALL";
            case ImsRILConstants.RIL_REQUEST_ENABLE_IMS: return "RIL_REQUEST_ENABLE_IMS";
            case ImsRILConstants.RIL_REQUEST_GET_IMS_BEARER_STATE: return "RIL_REQUEST_GET_IMS_BEARER_STATE";
            case ImsRILConstants.RIL_REQUEST_SET_SOS_INITIAL_ATTACH_APN: return "RIL_REQUEST_SET_SOS_INITIAL_ATTACH_APN";
            case ImsRILConstants.RIL_REQUEST_IMS_HANDOVER: return "RIL_REQUEST_IMS_HANDOVER";
            case ImsRILConstants.RIL_REQUEST_IMS_HANDOVER_STATUS_UPDATE: return "RIL_REQUEST_IMS_HANDOVER_STATUS_UPDATE";
            case ImsRILConstants.RIL_REQUEST_IMS_NETWORK_INFO_CHANGE: return "RIL_REQUEST_IMS_NETWORK_INFO_CHANGE";
            case ImsRILConstants.RIL_REQUEST_IMS_HANDOVER_CALL_END: return "RIL_REQUEST_IMS_HANDOVER_CALL_END";
            case ImsRILConstants.RIL_REQUEST_IMS_WIFI_ENABLE: return "RIL_REQUEST_IMS_WIFI_ENABLE";
            case ImsRILConstants.RIL_REQUEST_IMS_WIFI_CALL_STATE_CHANGE: return "RIL_REQUEST_IMS_WIFI_CALL_STATE_CHANGE";
            case ImsRILConstants.RIL_REQUEST_GET_TPMR_STATE: return "RIL_REQUEST_GET_TPMR_STATE";
            case ImsRILConstants.RIL_REQUEST_IMS_UPDATE_DATA_ROUTER: return "RIL_REQUEST_IMS_UPDATE_DATA_ROUTER";
            case ImsRILConstants.RIL_REQUEST_IMS_HOLD_SINGLE_CALL: return "RIL_REQUEST_IMS_HOLD_SINGLE_CALL";
            case ImsRILConstants.RIL_REQUEST_IMS_MUTE_SINGLE_CALL: return "RIL_REQUEST_IMS_MUTE_SINGLE_CALL";
            case ImsRILConstants.RIL_REQUEST_IMS_SILENCE_SINGLE_CALL: return "RIL_REQUEST_IMS_SILENCE_SINGLE_CALL";
            case ImsRILConstants.RIL_REQUEST_IMS_ENABLE_LOCAL_CONFERENCE: return "RIL_REQUEST_IMS_ENABLE_LOCAL_CONFERENCE";
            case ImsRILConstants.RIL_REQUEST_IMS_NOTIFY_HANDOVER_CALL_INFO: return "RIL_REQUEST_IMS_NOTIFY_HANDOVER_CALL_INFO";
            case ImsRILConstants.RIL_REQUEST_GET_IMS_SRVCC_CAPBILITY: return "RIL_REQUEST_GET_IMS_SRVCC_CAPBILITY";
            case ImsRILConstants.RIL_REQUEST_GET_IMS_PCSCF_ADDR: return "RIL_REQUEST_GET_IMS_PCSCF_ADDR";
            case ImsRILConstants.RIL_REQUEST_QUERY_FACILITY_LOCK_EXT: return "RIL_REQUEST_QUERY_FACILITY_LOCK_EXT";
            case ImsRILConstants.RIL_REQUEST_GET_IMS_REGADDR: return "RIL_REQUEST_GET_IMS_REGADDR";
            default: return requestToString(request);
        }
    }

    public void registerForImsBearerStateChanged(Handler h, int what, Object obj) {
        if(getRadioInteractor() != null) {
            mRadioInteractor.registerForImsBearerStateChanged(h,what,obj);
        } else {
            riljLog("registerForImsBearerStateChanged, RadioInteractor is null");
        }
    }

    public void unregisterForImsBearerStateChanged(Handler h) {
        if(getRadioInteractor() != null) {
            mRadioInteractor.unregisterForImsBearerStateChanged(h);
        } else {
            riljLog("unregisterForImsBearerStateChanged, RadioInteractor is null");
        }
    }

    public void registerForImsVideoQos(Handler h, int what, Object obj) {
        if(getRadioInteractor() != null) {
            mRadioInteractor.registerForImsVideoQos(h,what,obj);
        } else {
            riljLog("registerForImsVideoQos, RadioInteractor is null");
        }
    }

    public void unregisterForImsVideoQos(Handler h) {
        if(getRadioInteractor() != null) {
            mRadioInteractor.unregisterForImsVideoQos(h);
        } else {
            riljLog("unregisterForImsVideoQos, RadioInteractor is null");
        }
    }

    public void getImsBearerState(Message response) {
        if(getRadioInteractor() != null) {
            mRadioInteractor.getImsBearerState(response);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_GET_IMS_BEARER_STATE));
        } else {
            riljLog(imsRequestToString(ImsRILConstants.RIL_REQUEST_GET_IMS_BEARER_STATE)+" RadioInteractor is null");
        }
    }
    /* @} */

    public void setOnSuppServiceNotification(Handler h, int what, Object obj) {
        mCi.setOnSuppServiceNotification(h,what,obj);
    }

    public void unSetOnSuppServiceNotification(Handler h) {
        mCi.unSetOnSuppServiceNotification(h);
    }

    public void notifyVoWifiEnable(boolean enable, Message response) {
        if(getRadioInteractor() != null) {
            mRadioInteractor.notifyVoWifiEnable(enable,response);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_IMS_WIFI_ENABLE));
        } else {
            riljLog(imsRequestToString(ImsRILConstants.RIL_REQUEST_IMS_WIFI_ENABLE)+" RadioInteractor is null");
        }
    }

    public void notifyVoWifiCallStateChanged(boolean incall, Message response){
        if(getRadioInteractor() != null) {
            mRadioInteractor.notifyVoWifiCallStateChanged(incall,response);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_IMS_WIFI_CALL_STATE_CHANGE));
        } else {
            riljLog(imsRequestToString(ImsRILConstants.RIL_REQUEST_IMS_WIFI_CALL_STATE_CHANGE)+" RadioInteractor is null");
        }
    }

    public void notifyDataRouter(Message response){
        if(getRadioInteractor() != null) {
            mRadioInteractor.notifyDataRouter(response);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_IMS_UPDATE_DATA_ROUTER));
        } else {
            riljLog(imsRequestToString(ImsRILConstants.RIL_REQUEST_IMS_UPDATE_DATA_ROUTER)+" RadioInteractor is null");
        }
    }

    /* SPRD: add for VoWiFi
     */
    public void registerForImsNetworkStateChanged(Handler h, int what, Object obj) {
        if(getRadioInteractor() != null) {
            mRadioInteractor.registerForImsNetworkStateChanged(h,what,obj);
        } else {
            riljLog("registerForImsNetworkStateChanged, RadioInteractor is null");
        }
    }

    public void unregisterForImsNetworkStateChanged(Handler h) {
        if(getRadioInteractor() != null) {
            mRadioInteractor.unregisterForImsNetworkStateChanged(h);
        } else {
            riljLog("registerForImsNetworkStateChanged, RadioInteractor is null");
        }
    }

    public void registerImsHandoverRequest(Handler h, int what, Object obj){
        if(getRadioInteractor() != null) {
            mRadioInteractor.registerImsHandoverRequest(h,what,obj);
        } else {
            riljLog("registerImsHandoverRequest, RadioInteractor is null");
        }
    }


    public void unregisterImsHandoverRequest(Handler h){
        if(getRadioInteractor() != null) {
            mRadioInteractor.unregisterImsHandoverRequest(h);
        } else {
            riljLog("unregisterImsHandoverRequest, RadioInteractor is null");
        }
    }

    public void registerImsHandoverStatus(Handler h, int what, Object obj){
        if(getRadioInteractor() != null) {
            mRadioInteractor.registerImsHandoverStatus(h,what,obj);
        } else {
            riljLog("registerImsHandoverStatus, RadioInteractor is null");
        }
    }

    public void unregisterImsHandoverStatus(Handler h){
        if(getRadioInteractor() != null) {
            mRadioInteractor.unregisterImsHandoverStatus(h);
        } else {
            riljLog("unregisterImsHandoverStatus, RadioInteractor is null");
        }
    }

    public void registerImsNetworkInfo(Handler h, int what, Object obj){
        if(getRadioInteractor() != null) {
            mRadioInteractor.registerImsNetworkInfo(h,what,obj);
        } else {
            riljLog("registerImsNetworkInfo, RadioInteractor is null");
        }
    }

    public void unregisterImsNetworkInfo(Handler h){
        if(getRadioInteractor() != null) {
            mRadioInteractor.unregisterImsNetworkInfo(h);
        } else {
            riljLog("unregisterImsNetworkInfo, RadioInteractor is null");
        }
    }

    public void registerImsRegAddress(Handler h, int what, Object obj) {
        if(getRadioInteractor() != null) {
            mRadioInteractor.registerImsRegAddress(h,what,obj);
        } else {
            riljLog("registerImsRegAddress, RadioInteractor is null");
        }
    }

    public void unregisterImsRegAddress(Handler h){
        if(getRadioInteractor() != null) {
            mRadioInteractor.unregisterImsRegAddress(h);
        } else {
            riljLog("unregisterImsRegAddress, RadioInteractor is null");
        }
    }

    public void registerImsWiFiParam(Handler h, int what, Object obj){
        if(getRadioInteractor() != null) {
            mRadioInteractor.registerImsWiFiParam(h,what,obj);
        } else {
            riljLog("registerImsWiFiParam, RadioInteractor is null");
        }
    }


    public void unregisterImsWiFiParam(Handler h){
        if(getRadioInteractor() != null) {
            mRadioInteractor.unregisterImsWiFiParam(h);
        } else {
            riljLog("unregisterImsWiFiParam, RadioInteractor is null");
        }
    }

    public void registerForIccRefresh(Handler h, int what, Object obj){
        mCi.registerForIccRefresh(h, what, obj);
    }

    public void requestImsHandover(int type, Message response) {
        if(getRadioInteractor() != null) {
            mRadioInteractor.requestImsHandover(type,response);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_IMS_HANDOVER));
        } else {
            riljLog(imsRequestToString(ImsRILConstants.RIL_REQUEST_IMS_HANDOVER)+" RadioInteractor is null");
        }
    }

    public void notifyImsHandoverStatus(int status, Message response){
        if(getRadioInteractor() != null) {
            mRadioInteractor.notifyImsHandoverStatus(status,response);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_IMS_HANDOVER_STATUS_UPDATE));
        } else {
            riljLog(imsRequestToString(ImsRILConstants.RIL_REQUEST_IMS_HANDOVER_STATUS_UPDATE)+" RadioInteractor is null");
        }
    }

    public void notifyImsNetworkInfo(int type, String info, Message response){
        if(getRadioInteractor() != null) {
            mRadioInteractor.notifyImsNetworkInfo(type,info,response);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_IMS_NETWORK_INFO_CHANGE));
        } else {
            riljLog(imsRequestToString(ImsRILConstants.RIL_REQUEST_IMS_NETWORK_INFO_CHANGE)+" RadioInteractor is null");
        }
    }

    public void notifyImsCallEnd(int type, Message response){
        if(getRadioInteractor() != null) {
            mRadioInteractor.notifyImsCallEnd(type,response);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_IMS_NETWORK_INFO_CHANGE));
        } else {
            riljLog(imsRequestToString(ImsRILConstants.RIL_REQUEST_IMS_NETWORK_INFO_CHANGE)+" RadioInteractor is null");
        }
    }

    public void imsHoldSingleCall(int callid, boolean enable, Message response){
        if(getRadioInteractor() != null) {
            mRadioInteractor.imsHoldSingleCall(callid,enable,response);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_IMS_HOLD_SINGLE_CALL));
        } else {
            riljLog(imsRequestToString(ImsRILConstants.RIL_REQUEST_IMS_HOLD_SINGLE_CALL)+" RadioInteractor is null");
        }
    }

    public void imsMuteSingleCall(int callid, boolean enable, Message response){
        if(getRadioInteractor() != null) {
            mRadioInteractor.imsMuteSingleCall(callid,enable,response);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_IMS_MUTE_SINGLE_CALL));
        } else {
            riljLog(imsRequestToString(ImsRILConstants.RIL_REQUEST_IMS_MUTE_SINGLE_CALL)+" RadioInteractor is null");
        }
    }

    public void imsSilenceSingleCall(int callid, boolean enable, Message response){
        if(getRadioInteractor() != null) {
            mRadioInteractor.imsSilenceSingleCall(callid,enable,response);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_IMS_SILENCE_SINGLE_CALL));
        } else {
            riljLog(imsRequestToString(ImsRILConstants.RIL_REQUEST_IMS_SILENCE_SINGLE_CALL)+" RadioInteractor is null");
        }
    }

    public void imsEnableLocalConference(boolean enable, Message response){
        if(getRadioInteractor() != null) {
            mRadioInteractor.imsEnableLocalConference(enable,response);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_IMS_ENABLE_LOCAL_CONFERENCE));
        } else {
            riljLog(imsRequestToString(ImsRILConstants.RIL_REQUEST_IMS_ENABLE_LOCAL_CONFERENCE)+" RadioInteractor is null");
        }
    }

    public void notifyHandoverCallInfo(String callInfo,Message response) {
        if(getRadioInteractor() != null) {
            mRadioInteractor.notifyHandoverCallInfo(callInfo,response);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_IMS_NOTIFY_HANDOVER_CALL_INFO));
        } else {
            riljLog(imsRequestToString(ImsRILConstants.RIL_REQUEST_IMS_NOTIFY_HANDOVER_CALL_INFO)+" RadioInteractor is null");
        }
    }

    //SPRD:add for bug671964
    public void setImsPcscfAddress(String addr,Message response){
        if(getRadioInteractor() != null) {
            mRadioInteractor.setImsPcscfAddress(addr,response);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_SET_IMS_PCSCF_ADDR));
        } else {
            riljLog(imsRequestToString(ImsRILConstants.RIL_REQUEST_SET_IMS_PCSCF_ADDR)+" RadioInteractor is null");
        }
    }

    public void getSrvccCapbility(Message response) {
        if(getRadioInteractor() != null) {
            mRadioInteractor.getSrvccCapbility(response);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_GET_IMS_SRVCC_CAPBILITY));
        } else {
            riljLog(imsRequestToString(ImsRILConstants.RIL_REQUEST_GET_IMS_SRVCC_CAPBILITY)+" RadioInteractor is null");
        }
    }

    public void
    getImsPcscfAddress(Message response) {
        if(getRadioInteractor() != null) {
            mRadioInteractor.getImsPcscfAddress(response);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_GET_IMS_PCSCF_ADDR));
        } else {
            riljLog(imsRequestToString(ImsRILConstants.RIL_REQUEST_GET_IMS_PCSCF_ADDR)+" RadioInteractor is null");
        }
    }

    public void getImsRegAddress(Message response){
        if(getRadioInteractor() != null) {
            mRadioInteractor.getImsRegAddress(response);
            if (RILJ_LOGD) riljLog( "> " + imsRequestToString(ImsRILConstants.RIL_REQUEST_GET_IMS_REGADDR));
        } else {
            riljLog(imsRequestToString(ImsRILConstants.RIL_REQUEST_GET_IMS_REGADDR)+" RadioInteractor is null");
        }
    }

    public void
    updateCLIP(int enable, Message response) {
        if (getRadioInteractor() != null) {
            mRadioInteractor.updateCLIP(enable, response);
        }else {
            riljLog("updateCLIP RadioInteractor is null");
        }
    }
    /**
     * Convert to DataProfileInfo defined in types.hal
     * @param dp Data profile
     * @return A converted data profile
     */
    private static DataProfileInfo convertToHalDataProfile(DataProfile dp) {
        DataProfileInfo dpi = new DataProfileInfo();

        dpi.profileId = dp.getProfileId();
        dpi.apn = dp.getApn();
        dpi.protocol = dp.getProtocol();
        dpi.roamingProtocol = dp.getRoamingProtocol();
        dpi.authType = dp.getAuthType();
        dpi.user = dp.getUserName();
        dpi.password = dp.getPassword();
        dpi.type = dp.getType();
        dpi.maxConnsTime = dp.getMaxConnsTime();
        dpi.maxConns = dp.getMaxConns();
        dpi.waitTime = dp.getWaitTime();
        dpi.enabled = dp.isEnabled();
        dpi.supportedApnTypesBitmap = dp.getSupportedApnTypesBitmap();
        dpi.bearerBitmap = dp.getBearerBitmap();
        dpi.mtu = dp.getMtu();
        dpi.mvnoType = convertToHalMvnoType(dp.getMvnoType());
        dpi.mvnoMatchData = dp.getMvnoMatchData();

        return dpi;
    }

    /**
     * Convert MVNO type string into MvnoType defined in types.hal.
     * @param mvnoType MVNO type
     * @return MVNO type in integer
     */
    private static int convertToHalMvnoType(String mvnoType) {
        switch (mvnoType) {
            case "imsi":
                return MvnoType.IMSI;
            case "gid":
                return MvnoType.GID;
            case "spn":
                return MvnoType.SPN;
            default:
                return MvnoType.NONE;
        }
    }

    public void registerForNotAvailable(Handler h, int what, Object obj) {
        mCi.registerForNotAvailable(h,what,obj);
    }

    public void setVideoResolution(int resolution, Message result){
        if(getRadioInteractor() != null) {
            mRadioInteractor.setVideoResolution(resolution, result);
        } else {
            riljLog("setVideoResolution, RadioInteractor is null");
        }
    }
    public void enableLocalHold(boolean enable, Message result){
        if(getRadioInteractor() != null) {
            mRadioInteractor.enableLocalHold(enable, result);
        } else {
            riljLog("enableLocalHold, RadioInteractor is null");
        }
    }
    public void enableWiFiParamReport(boolean enable, Message result){
        if(getRadioInteractor() != null) {
            mRadioInteractor.enableWiFiParamReport(enable, result);
        } else {
            riljLog("enableWiFiParamReport, RadioInteractor is null");
        }
    }
    public void callMediaChangeRequestTimeOut(int callId, Message result){
        if(getRadioInteractor() != null) {
            mRadioInteractor.callMediaChangeRequestTimeOut(callId, result);
        } else {
            riljLog("callMediaChangeRequestTimeOut, RadioInteractor is null");
        }
    }
}

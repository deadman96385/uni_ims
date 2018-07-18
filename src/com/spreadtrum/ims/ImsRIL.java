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
import android.os.PowerManager.WakeLock;
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

import com.android.internal.telephony.ClientWakelockTracker;
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
import com.android.internal.telephony.dataconnection.DcFailCause;
import com.android.internal.telephony.dataconnection.DataCallResponse;
import com.android.internal.telephony.dataconnection.DataProfile;
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
import com.android.ims.ImsCallForwardInfo;
import com.android.ims.internal.ImsCallForwardInfoEx;
import com.android.sprd.telephony.RadioInteractor;
import com.android.sprd.telephony.RadioInteractorCore;
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
import vendor.sprd.hardware.radio.V1_0.IExtRadio;
import vendor.sprd.hardware.radio.V1_0.IExtRadioIndication;
import vendor.sprd.hardware.radio.V1_0.IExtRadioResponse;
import vendor.sprd.hardware.radio.V1_0.IIMSRadioIndication;
import vendor.sprd.hardware.radio.V1_0.IIMSRadioResponse;
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
 * {@hide}
 */
class RILRequest {
    static final String LOG_TAG = "RilRequest";

    //***** Class Variables
    static Random sRandom = new Random();
    static AtomicInteger sNextSerial = new AtomicInteger(0);
    private static Object sPoolSync = new Object();
    private static RILRequest sPool = null;
    private static int sPoolSize = 0;
    private static final int MAX_POOL_SIZE = 4;

    //***** Instance Variables
    int mSerial;
    int mRequest;
    Message mResult;
    RILRequest mNext;
    int mWakeLockType;
    WorkSource mWorkSource;
    String mClientId;
    // time in ms when RIL request was made
    long mStartTimeMs;

    /**
     * Retrieves a new RILRequest instance from the pool.
     *
     * @param request RIL_REQUEST_*
     * @param result sent when operation completes
     * @return a RILRequest instance from the pool.
     */
    private static RILRequest obtain(int request, Message result) {
        RILRequest rr = null;

        synchronized(sPoolSync) {
            if (sPool != null) {
                rr = sPool;
                sPool = rr.mNext;
                rr.mNext = null;
                sPoolSize--;
            }
        }

        if (rr == null) {
            rr = new RILRequest();
        }

        rr.mSerial = sNextSerial.getAndIncrement();

        rr.mRequest = request;
        rr.mResult = result;

        rr.mWakeLockType = ImsRIL.INVALID_WAKELOCK;
        rr.mWorkSource = null;
        rr.mStartTimeMs = SystemClock.elapsedRealtime();
        if (result != null && result.getTarget() == null) {
            throw new NullPointerException("Message target must not be null");
        }

        return rr;
    }


    /**
     * Retrieves a new RILRequest instance from the pool and sets the clientId
     *
     * @param request RIL_REQUEST_*
     * @param result sent when operation completes
     * @param workSource WorkSource to track the client
     * @return a RILRequest instance from the pool.
     */
    static RILRequest obtain(int request, Message result, WorkSource workSource) {
        RILRequest rr = null;

        rr = obtain(request, result);
        if(workSource != null) {
            rr.mWorkSource = workSource;
            rr.mClientId = String.valueOf(workSource.get(0)) + ":" + workSource.getName(0);
        } else {
            Rlog.e(LOG_TAG, "null workSource " + request);
        }

        return rr;
    }

    /**
     * Returns a RILRequest instance to the pool.
     *
     * Note: This should only be called once per use.
     */
    void release() {
        synchronized (sPoolSync) {
            if (sPoolSize < MAX_POOL_SIZE) {
                mNext = sPool;
                sPool = this;
                sPoolSize++;
                mResult = null;
                if(mWakeLockType != ImsRIL.INVALID_WAKELOCK) {
                    //This is OK for some wakelock types and not others
                    if(mWakeLockType == ImsRIL.FOR_WAKELOCK) {
                        Rlog.e(LOG_TAG, "RILRequest releasing with held wake lock: "
                                + serialString());
                    }
                }
            }
        }
    }

    private RILRequest() {
    }

    static void
    resetSerial() {
        // use a random so that on recovery we probably don't mix old requests
        // with new.
        sNextSerial.set(sRandom.nextInt());
    }

    String
    serialString() {
        //Cheesy way to do %04d
        StringBuilder sb = new StringBuilder(8);
        String sn;

        long adjustedSerial = (((long)mSerial) - Integer.MIN_VALUE)%10000;

        sn = Long.toString(adjustedSerial);

        //sb.append("J[");
        sb.append('[');
        for (int i = 0, s = sn.length() ; i < 4 - s; i++) {
            sb.append('0');
        }

        sb.append(sn);
        sb.append(']');
        return sb.toString();
    }

    void
    onError(int error, Object ret) {
        CommandException ex;

        ex = CommandException.fromRilErrno(error);

         Rlog.d(LOG_TAG, serialString() + "< "
                + ImsRIL.imsRequestToString(mRequest)
                + " error: " + ex + " ret=" + ImsRIL.retToString(mRequest, ret));

        if (mResult != null) {
            AsyncResult.forMessage(mResult, ret, ex);
            mResult.sendToTarget();
        }
    }
}


/**
 * RIL implementation of the CommandsInterface.
 *
 * {@hide}
 */
public final class ImsRIL {
    static final String RILJ_LOG_TAG = "ImsRILJ";
    static final boolean RILJ_LOGD = true;
    static final boolean RILJ_LOGV = false; // STOPSHIP if true
    static final int RADIO_SCREEN_UNSET = -1;
    static final int RADIO_SCREEN_OFF = 0;
    static final int RADIO_SCREEN_ON = 1;

    // Have a separate wakelock instance for Ack
    static final String RILJ_ACK_WAKELOCK_NAME = "IMS_RIL_ACK_WL";
    static final int RIL_HISTOGRAM_BUCKET_COUNT = 5;

    /**
     * Wake lock timeout should be longer than the longest timeout in
     * the vendor ril.
     */
    private static final int DEFAULT_WAKE_LOCK_TIMEOUT_MS = 60000;

    // Wake lock default timeout associated with ack
    private static final int DEFAULT_ACK_WAKE_LOCK_TIMEOUT_MS = 200;

    private static final int DEFAULT_BLOCKING_MESSAGE_RESPONSE_TIMEOUT_MS = 2000;

    // ussd format type.
    private static final int GSM_TYPE = 15;
    private static final int UCS2_TYPE = 72;
    enum RadioState {
        RADIO_OFF,         /* Radio explicitly powered off (eg CFUN=0) */
        RADIO_UNAVAILABLE, /* Radio unavailable (eg, resetting or not booted) */
        RADIO_ON;          /* Radio is on */

        public boolean isOn() /* and available...*/ {
            return this == RADIO_ON;
        }

        public boolean isAvailable() {
            return this != RADIO_UNAVAILABLE;
        }
    }

    /*public static final int CALL_MEDIA_CHANGE_ACTION_DOWNGRADE_TO_VOICE   = 0;
    public static final int CALL_MEDIA_CHANGE_ACTION_UPGRADE_TO_VIDEO     = 1;
    public static final int CALL_MEDIA_CHANGE_ACTION_SET_TO_PAUSE         = 2;
    public static final int CALL_MEDIA_CHANGE_ACTION_RESUME_FORM_PAUSE    = 3;*/
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

    RadioState mRadioState;
    private final ClientWakelockTracker mClientWakelockTracker = new ClientWakelockTracker();

    /**
     * Wake lock timeout should be longer than the longest timeout in
     * the vendor ril.
     */
    private static final int DEFAULT_WAKE_LOCK_TIMEOUT = 60000;

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

    // Variables used to differentiate ack messages from request while calling clearWakeLock()
    public static final int INVALID_WAKELOCK = -1;
    public static final int FOR_WAKELOCK = 0;
    public static final int FOR_ACK_WAKELOCK = 1;

    CommandsInterface mCi;
    Context mContext;
    RadioInteractorCore mRadioInteractor;

    //***** Instance Variables

    final WakeLock mWakeLock;           // Wake lock associated with request/response
    final WakeLock mAckWakeLock;        // Wake lock associated with ack sent
    final int mWakeLockTimeout;         // Timeout associated with request/response
    final int mAckWakeLockTimeout;      // Timeout associated with ack sent
    // The number of wakelock requests currently active.  Don't release the lock
    // until dec'd to 0
    int mWakeLockCount;

    // Variables used to identify releasing of WL on wakelock timeouts
    volatile int mWlSequenceNum = 0;
    volatile int mAckWlSequenceNum = 0;

    SparseArray<RILRequest> mRequestList = new SparseArray<RILRequest>();
    static SparseArray<TelephonyHistogram> mRilTimeHistograms = new
            SparseArray<TelephonyHistogram>();

    Object[]     mLastNITZTimeInfo;

    // When we are testing emergency calls
    AtomicBoolean mTestingEmergencyCall = new AtomicBoolean(false);

    final Integer mPhoneId;

    /* default work source which will blame phone process */
    private WorkSource mRILDefaultWorkSource;

    /* Worksource containing all applications causing wakelock to be held */
    private WorkSource mActiveWakelockWorkSource;

    /** Telephony metrics instance for logging metrics event */
    private TelephonyMetrics mMetrics = TelephonyMetrics.getInstance();

    boolean mIsMobileNetworkSupported;
    IMSRadioResponse mRadioResponse;
    IMSRadioIndication mRadioIndication;
    volatile IExtRadio mRadioProxy = null;
    final AtomicLong mRadioProxyCookie = new AtomicLong(0);
    final RadioProxyDeathRecipient mRadioProxyDeathRecipient;
    final RilHandler mRilHandler;

    //***** Events
    static final int EVENT_WAKE_LOCK_TIMEOUT    = 2;
    static final int EVENT_ACK_WAKE_LOCK_TIMEOUT    = 4;
    static final int EVENT_BLOCKING_RESPONSE_TIMEOUT = 5;
    static final int EVENT_RADIO_PROXY_DEAD     = 6;

    //***** Constants

    static final String[] HIDL_SERVICE_NAME = {"slot1", "slot2", "slot3"};

    static final int IRADIO_GET_SERVICE_DELAY_MILLIS = 6 * 1000;

    public static List<TelephonyHistogram> getTelephonyRILTimingHistograms() {
        List<TelephonyHistogram> list;
        synchronized (mRilTimeHistograms) {
            list = new ArrayList<>(mRilTimeHistograms.size());
            for (int i = 0; i < mRilTimeHistograms.size(); i++) {
                TelephonyHistogram entry = new TelephonyHistogram(mRilTimeHistograms.valueAt(i));
                list.add(entry);
            }
        }
        return list;
    }

    class RilHandler extends Handler {
        //***** Handler implementation
        @Override public void
        handleMessage(Message msg) {
            RILRequest rr;

            switch (msg.what) {
                case EVENT_WAKE_LOCK_TIMEOUT:
                    // Haven't heard back from the last request.  Assume we're
                    // not getting a response and  release the wake lock.

                    // The timer of WAKE_LOCK_TIMEOUT is reset with each
                    // new send request. So when WAKE_LOCK_TIMEOUT occurs
                    // all requests in mRequestList already waited at
                    // least DEFAULT_WAKE_LOCK_TIMEOUT_MS but no response.
                    //
                    // Note: Keep mRequestList so that delayed response
                    // can still be handled when response finally comes.

                    synchronized (mRequestList) {
                        if (msg.arg1 == mWlSequenceNum && clearWakeLock(FOR_WAKELOCK)) {
                            if (RILJ_LOGD) {
                                int count = mRequestList.size();
                                Rlog.d(RILJ_LOG_TAG, "WAKE_LOCK_TIMEOUT " +
                                        " mRequestList=" + count);
                                for (int i = 0; i < count; i++) {
                                    rr = mRequestList.valueAt(i);
                                    Rlog.d(RILJ_LOG_TAG, i + ": [" + rr.mSerial + "] "
                                            + imsRequestToString(rr.mRequest));
                                }
                            }
                        }
                    }
                    break;

                case EVENT_ACK_WAKE_LOCK_TIMEOUT:
                    if (msg.arg1 == mAckWlSequenceNum && clearWakeLock(FOR_ACK_WAKELOCK)) {
                        if (RILJ_LOGV) {
                            Rlog.d(RILJ_LOG_TAG, "ACK_WAKE_LOCK_TIMEOUT");
                        }
                    }
                    break;

                case EVENT_BLOCKING_RESPONSE_TIMEOUT:
                    int serial = msg.arg1;
                    rr = findAndRemoveRequestFromList(serial);
                    // If the request has already been processed, do nothing
                    if(rr == null) {
                        break;
                    }

                    //build a response if expected
                    if (rr.mResult != null) {
                        Object timeoutResponse = getResponseForTimedOutRILRequest(rr);
                        AsyncResult.forMessage( rr.mResult, timeoutResponse, null);
                        rr.mResult.sendToTarget();
                        mMetrics.writeOnRilTimeoutResponse(mPhoneId, rr.mSerial, rr.mRequest);
                    }

                    decrementWakeLock(rr);
                    rr.release();
                    break;

                case EVENT_RADIO_PROXY_DEAD:
                    riljLog("handleMessage: EVENT_RADIO_PROXY_DEAD cookie = " + msg.obj +
                            " mRadioProxyCookie = " + mRadioProxyCookie.get());
                    if ((long) msg.obj == mRadioProxyCookie.get()) {
                        resetProxyAndRequestList();

                        // todo: rild should be back up since message was sent with a delay. this is
                        // a hack.
                        getRadioProxy(null);
                    }
                    break;
            }
        }
    }

    /**
     * In order to prevent calls to Telephony from waiting indefinitely
     * low-latency blocking calls will eventually time out. In the event of
     * a timeout, this function generates a response that is returned to the
     * higher layers to unblock the call. This is in lieu of a meaningful
     * response.
     * @param rr The RIL Request that has timed out.
     * @return A default object, such as the one generated by a normal response
     * that is returned to the higher layers.
     **/
    private static Object getResponseForTimedOutRILRequest(RILRequest rr) {
        if (rr == null ) return null;

        Object timeoutResponse = null;
        switch(rr.mRequest) {
            case RIL_REQUEST_GET_ACTIVITY_INFO:
                timeoutResponse = new ModemActivityInfo(
                        0, 0, 0, new int [ModemActivityInfo.TX_POWER_LEVELS], 0, 0);
                break;
        };
        return timeoutResponse;
    }

    final class RadioProxyDeathRecipient implements HwBinder.DeathRecipient {
        @Override
        public void serviceDied(long cookie) {
            // Deal with service going away
            riljLog("serviceDied");
            // todo: temp hack to send delayed message so that rild is back up by then
            //mRilHandler.sendMessage(mRilHandler.obtainMessage(EVENT_RADIO_PROXY_DEAD, cookie));
            mRilHandler.sendMessageDelayed(
                    mRilHandler.obtainMessage(EVENT_RADIO_PROXY_DEAD, cookie),
                    IRADIO_GET_SERVICE_DELAY_MILLIS);
        }
    }

    private void resetProxyAndRequestList() {
        mRadioProxy = null;

        // increment the cookie so that death notification can be ignored
        mRadioProxyCookie.incrementAndGet();

        mRadioState = RadioState.RADIO_UNAVAILABLE;

        mNotAvailRegistrants.notifyRegistrants();

        RILRequest.resetSerial();
        // Clear request list on close
        clearRequestList(RADIO_NOT_AVAILABLE, false);

        // todo: need to get service right away so setResponseFunctions() can be called for
        // unsolicited indications. getService() is not a blocking call, so it doesn't help to call
        // it here. Current hack is to call getService() on death notification after a delay.
    }

    private IExtRadio getRadioProxy(Message result) {
        if (!mIsMobileNetworkSupported) {
            if (RILJ_LOGV) riljLog("getRadioProxy: Not calling getService(): wifi-only");
            if (result != null) {
                AsyncResult.forMessage(result, null,
                        CommandException.fromRilErrno(RADIO_NOT_AVAILABLE));
                result.sendToTarget();
            }
            return null;
        }

        if (mRadioProxy != null) {
            return mRadioProxy;
        }

        try {
            mRadioProxy = IExtRadio.getService(HIDL_SERVICE_NAME[mPhoneId == null ? 0 : mPhoneId]);
            if (mRadioProxy != null) {
                mRadioProxy.linkToDeath(mRadioProxyDeathRecipient,
                        mRadioProxyCookie.incrementAndGet());
                mRadioProxy.setIMSResponseFunctions(mRadioResponse, mRadioIndication);
            } else {
                riljLoge("getRadioProxy: mRadioProxy == null");
            }
        } catch (RemoteException | RuntimeException e) {
            mRadioProxy = null;
            riljLoge("RadioProxy getService/setResponseFunctions: " + e);
        }

        if (mRadioProxy == null) {
            if (result != null) {
                AsyncResult.forMessage(result, null,
                        CommandException.fromRilErrno(RADIO_NOT_AVAILABLE));
                result.sendToTarget();
            }

            // if service is not up, treat it like death notification to try to get service again
            mRilHandler.sendMessageDelayed(
                    mRilHandler.obtainMessage(EVENT_RADIO_PROXY_DEAD,
                            mRadioProxyCookie.incrementAndGet()),
                    IRADIO_GET_SERVICE_DELAY_MILLIS);
        }

        return mRadioProxy;
    }

    //***** Constructors
    public ImsRIL(Context context, Integer instanceId, CommandsInterface ci) {
        mCi = ci;
        mContext = context;
        mPhoneId = instanceId;
        riljLog("ImsRIL: init mPhoneId=" + instanceId + ")");

        ConnectivityManager cm = (ConnectivityManager)context.getSystemService(
                Context.CONNECTIVITY_SERVICE);
        mIsMobileNetworkSupported = cm.isNetworkSupported(ConnectivityManager.TYPE_MOBILE);

        mRadioResponse = new IMSRadioResponse(this);
        mRadioIndication = new IMSRadioIndication(this);

        mRilHandler = new RilHandler();
        mRadioProxyDeathRecipient = new RadioProxyDeathRecipient();

        PowerManager pm = (PowerManager)context.getSystemService(Context.POWER_SERVICE);
        mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, RILJ_LOG_TAG);
        mWakeLock.setReferenceCounted(false);
        mAckWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, RILJ_ACK_WAKELOCK_NAME);
        mAckWakeLock.setReferenceCounted(false);
        mWakeLockTimeout = SystemProperties.getInt(TelephonyProperties.PROPERTY_WAKE_LOCK_TIMEOUT,
                DEFAULT_WAKE_LOCK_TIMEOUT_MS);
        mAckWakeLockTimeout = SystemProperties.getInt(
                TelephonyProperties.PROPERTY_WAKE_LOCK_TIMEOUT, DEFAULT_ACK_WAKE_LOCK_TIMEOUT_MS);
        mWakeLockCount = 0;
        mRILDefaultWorkSource = new WorkSource(context.getApplicationInfo().uid,
                context.getPackageName());

        // set radio callback; needed to set RadioIndication callback (should be done after
        // wakelock stuff is initialized above as callbacks are received on separate binder threads)
        getRadioProxy(null);
    }

    private RadioInteractorCore getRadioInteractor(){
        if(mRadioInteractor == null && RadioInteractorFactory.getInstance() != null) {
            mRadioInteractor = RadioInteractorFactory.getInstance().getRadioInteractorCore(mPhoneId == null ? 0 : mPhoneId);
        }
        return mRadioInteractor;
    }

    private void addRequest(RILRequest rr) {
        acquireWakeLock(rr, FOR_WAKELOCK);
        synchronized (mRequestList) {
            rr.mStartTimeMs = SystemClock.elapsedRealtime();
            mRequestList.append(rr.mSerial, rr);
        }
    }

    private RILRequest obtainRequest(int request, Message result, WorkSource workSource) {
        RILRequest rr = RILRequest.obtain(request, result, workSource);
        addRequest(rr);
        return rr;
    }

    private void handleRadioProxyExceptionForRR(RILRequest rr, String caller, Exception e) {
        riljLog(caller + ": " + e);
        resetProxyAndRequestList();

        // service most likely died, handle exception like death notification to try to get service
        // again
        mRilHandler.sendMessageDelayed(
                mRilHandler.obtainMessage(EVENT_RADIO_PROXY_DEAD,
                        mRadioProxyCookie.incrementAndGet()),
                IRADIO_GET_SERVICE_DELAY_MILLIS);
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
        IExtRadio radioProxy = getRadioProxy(response);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_QUERY_FACILITY_LOCK_EXT, response,
                    mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.getFacilityLockForAppExt(rr.mSerial, convertNullToEmptyString(facility),
                        convertNullToEmptyString(password), serviceClass, "");
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "queryFacilityLockForAppExt", e);
            }
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


    public void invokeOemRilRequestRaw(byte[] data, Message response) {
        mCi.invokeOemRilRequestRaw(data, response);
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

    /**
     * This is a helper function to be called when a RadioIndication callback is called.
     * It takes care of acquiring wakelock and sending ack if needed.
     * @param indicationType RadioIndicationType received
     */
    void processIndication(int indicationType) {
        if (indicationType == RadioIndicationType.UNSOLICITED_ACK_EXP) {
            sendAck();
            if (RILJ_LOGD) riljLog("Unsol response received; Sending ack to ril.cpp");
        } else {
            // ack is not expected to be sent back. Nothing is required to be done here.
        }
    }

    void processRequestAck(int serial) {
        RILRequest rr;
        synchronized (mRequestList) {
            rr = mRequestList.get(serial);
        }
        if (rr == null) {
            Rlog.w(RILJ_LOG_TAG, "processRequestAck: Unexpected solicited ack response! "
                    + "serial: " + serial);
        } else {
            decrementWakeLock(rr);
            if (RILJ_LOGD) {
                riljLog(rr.serialString() + " Ack < " + imsRequestToString(rr.mRequest));
            }
        }
    }

    /**
     * This is a helper function to be called when a RadioResponse callback is called.
     * It takes care of acks, wakelocks, and finds and returns RILRequest corresponding to the
     * response if one is found.
     * @param responseInfo RadioResponseInfo received in response callback
     * @return RILRequest corresponding to the response
     */
    RILRequest processResponse(RadioResponseInfo responseInfo) {
        int serial = responseInfo.serial;
        int error = responseInfo.error;
        int type = responseInfo.type;

        RILRequest rr = null;

        if (type == RadioResponseType.SOLICITED_ACK) {
            synchronized (mRequestList) {
                rr = mRequestList.get(serial);
            }
            if (rr == null) {
                Rlog.w(RILJ_LOG_TAG, "Unexpected solicited ack response! sn: " + serial);
            } else {
                decrementWakeLock(rr);
                if (RILJ_LOGD) {
                    riljLog(rr.serialString() + " Ack < " + imsRequestToString(rr.mRequest));
                }
            }
            return rr;
        }

        rr = findAndRemoveRequestFromList(serial);
        if (rr == null) {
            Rlog.e(RILJ_LOG_TAG, "processResponse: Unexpected response! serial: " + serial
                    + " error: " + error);
            return null;
        }

        // Time logging for RIL command and storing it in TelephonyHistogram.
        addToRilHistogram(rr);

        if (type == RadioResponseType.SOLICITED_ACK_EXP) {
            sendAck();
            if (RILJ_LOGD) {
                riljLog("Response received for " + rr.serialString() + " "
                        + imsRequestToString(rr.mRequest) + " Sending ack to ril.cpp");
            }
        } else {
            // ack sent for SOLICITED_ACK_EXP above; nothing to do for SOLICITED response
        }


        if (error != RadioError.NONE) {
            switch (rr.mRequest) {
                case RIL_REQUEST_ENTER_SIM_PIN:
                case RIL_REQUEST_ENTER_SIM_PIN2:
                case RIL_REQUEST_CHANGE_SIM_PIN:
                case RIL_REQUEST_CHANGE_SIM_PIN2:
                case RIL_REQUEST_SET_FACILITY_LOCK:
                        if (RILJ_LOGD) {
                            riljLog("ON some errors fakeSimStatusChanged");
                        }
                    break;

            }
        } else {
            switch (rr.mRequest) {
                case RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND:
                    if (mTestingEmergencyCall.getAndSet(false)) {
                        riljLog("testing emergency call, notify ECM Registrants");
                    }
            }
        }
        return rr;
    }

    /**
     * This is a helper function to be called at the end of all RadioResponse callbacks.
     * It takes care of sending error response, logging, decrementing wakelock if needed, and
     * releases the request from memory pool.
     * @param rr RILRequest for which response callback was called
     * @param responseInfo RadioResponseInfo received in the callback
     * @param ret object to be returned to request sender
     */
    void processResponseDone(RILRequest rr, RadioResponseInfo responseInfo, Object ret) {
        if (responseInfo.error == 0) {
            if (RILJ_LOGD) {
                riljLog(rr.serialString() + "< " + imsRequestToString(rr.mRequest)
                        + " " + retToString(rr.mRequest, ret));
            }
        } else {
            if (RILJ_LOGD) {
                riljLog(rr.serialString() + "< " + imsRequestToString(rr.mRequest)
                        + " error " + responseInfo.error);
            }
            rr.onError(responseInfo.error, ret);
        }
        mMetrics.writeOnRilSolicitedResponse(mPhoneId, rr.mSerial, responseInfo.error,
                rr.mRequest, ret);
        if (rr != null) {
            if (responseInfo.type == RadioResponseType.SOLICITED) {
                decrementWakeLock(rr);
            }
            rr.release();
        }
    }

    /**
     * Function to send ack and acquire related wakelock
     */
    private void sendAck() {
        // TODO: Remove rr and clean up acquireWakelock for response and ack
        RILRequest rr = RILRequest.obtain(RIL_RESPONSE_ACKNOWLEDGEMENT, null,
                mRILDefaultWorkSource);
        acquireWakeLock(rr, FOR_ACK_WAKELOCK);
        IExtRadio radioProxy = getRadioProxy(null);
        if (radioProxy != null) {
            try {
                radioProxy.responseAcknowledgement();
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "sendAck", e);
                riljLoge("sendAck: " + e);
            }
        } else {
            Rlog.e(RILJ_LOG_TAG, "Error trying to send ack, radioProxy = null");
        }
        rr.release();
    }

    private WorkSource getDeafultWorkSourceIfInvalid(WorkSource workSource) {
        if (workSource == null) {
            workSource = mRILDefaultWorkSource;
        }

        return workSource;
    }

    private String getWorkSourceClientId(WorkSource workSource) {
        if (workSource != null) {
            return String.valueOf(workSource.get(0)) + ":" + workSource.getName(0);
        }

        return null;
    }

    /**
     * Holds a PARTIAL_WAKE_LOCK whenever
     * a) There is outstanding RIL request sent to RIL deamon and no replied
     * b) There is a request pending to be sent out.
     *
     * There is a WAKE_LOCK_TIMEOUT to release the lock, though it shouldn't
     * happen often.
     */

    private void acquireWakeLock(RILRequest rr, int wakeLockType) {
        synchronized (rr) {
            if (rr.mWakeLockType != INVALID_WAKELOCK) {
                Rlog.d(RILJ_LOG_TAG, "Failed to aquire wakelock for " + rr.serialString());
                return;
            }

            switch(wakeLockType) {
                case FOR_WAKELOCK:
                    synchronized (mWakeLock) {
                        mWakeLock.acquire();
                        mWakeLockCount++;
                        mWlSequenceNum++;

                        String clientId = getWorkSourceClientId(rr.mWorkSource);
                        if (!mClientWakelockTracker.isClientActive(clientId)) {
                            if (mActiveWakelockWorkSource != null) {
                                mActiveWakelockWorkSource.add(rr.mWorkSource);
                            } else {
                                mActiveWakelockWorkSource = rr.mWorkSource;
                            }
                            mWakeLock.setWorkSource(mActiveWakelockWorkSource);
                        }

                        mClientWakelockTracker.startTracking(rr.mClientId,
                                rr.mRequest, rr.mSerial, mWakeLockCount);

                        Message msg = mRilHandler.obtainMessage(EVENT_WAKE_LOCK_TIMEOUT);
                        msg.arg1 = mWlSequenceNum;
                        mRilHandler.sendMessageDelayed(msg, mWakeLockTimeout);
                    }
                    break;
                case FOR_ACK_WAKELOCK:
                    synchronized (mAckWakeLock) {
                        mAckWakeLock.acquire();
                        mAckWlSequenceNum++;

                        Message msg = mRilHandler.obtainMessage(EVENT_ACK_WAKE_LOCK_TIMEOUT);
                        msg.arg1 = mAckWlSequenceNum;
                        mRilHandler.sendMessageDelayed(msg, mAckWakeLockTimeout);
                    }
                    break;
                default: //WTF
                    Rlog.w(RILJ_LOG_TAG, "Acquiring Invalid Wakelock type " + wakeLockType);
                    return;
            }
            rr.mWakeLockType = wakeLockType;
        }
    }

    private void decrementWakeLock(RILRequest rr) {
        synchronized (rr) {
            switch(rr.mWakeLockType) {
                case FOR_WAKELOCK:
                    synchronized (mWakeLock) {
                        mClientWakelockTracker.stopTracking(rr.mClientId,
                                rr.mRequest, rr.mSerial,
                                (mWakeLockCount > 1) ? mWakeLockCount - 1 : 0);
                        String clientId = getWorkSourceClientId(rr.mWorkSource);;
                        if (!mClientWakelockTracker.isClientActive(clientId)
                                && (mActiveWakelockWorkSource != null)) {
                            mActiveWakelockWorkSource.remove(rr.mWorkSource);
                            if (mActiveWakelockWorkSource.size() == 0) {
                                mActiveWakelockWorkSource = null;
                            }
                            mWakeLock.setWorkSource(mActiveWakelockWorkSource);
                        }

                        if (mWakeLockCount > 1) {
                            mWakeLockCount--;
                        } else {
                            mWakeLockCount = 0;
                            mWakeLock.release();
                        }
                    }
                    break;
                case FOR_ACK_WAKELOCK:
                    //We do not decrement the ACK wakelock
                    break;
                case INVALID_WAKELOCK:
                    break;
                default:
                    Rlog.w(RILJ_LOG_TAG, "Decrementing Invalid Wakelock type " + rr.mWakeLockType);
            }
            rr.mWakeLockType = INVALID_WAKELOCK;
        }
    }

    private boolean clearWakeLock(int wakeLockType) {
        if (wakeLockType == FOR_WAKELOCK) {
            synchronized (mWakeLock) {
                if (mWakeLockCount == 0 && !mWakeLock.isHeld()) return false;
                Rlog.d(RILJ_LOG_TAG, "NOTE: mWakeLockCount is " + mWakeLockCount
                        + "at time of clearing");
                mWakeLockCount = 0;
                mWakeLock.release();
                mClientWakelockTracker.stopTrackingAll();
                mActiveWakelockWorkSource = null;
                return true;
            }
        } else {
            synchronized (mAckWakeLock) {
                if (!mAckWakeLock.isHeld()) return false;
                mAckWakeLock.release();
                return true;
            }
        }
    }

    /**
     * Release each request in mRequestList then clear the list
     * @param error is the RIL_Errno sent back
     * @param loggable true means to print all requests in mRequestList
     */
    private void clearRequestList(int error, boolean loggable) {
        RILRequest rr;
        synchronized (mRequestList) {
            int count = mRequestList.size();
            if (RILJ_LOGD && loggable) {
                Rlog.d(RILJ_LOG_TAG, "clearRequestList " + " mWakeLockCount="
                        + mWakeLockCount + " mRequestList=" + count);
            }

            for (int i = 0; i < count; i++) {
                rr = mRequestList.valueAt(i);
                if (RILJ_LOGD && loggable) {
                    Rlog.d(RILJ_LOG_TAG, i + ": [" + rr.mSerial + "] "
                            + imsRequestToString(rr.mRequest));
                }
                rr.onError(error, null);
                decrementWakeLock(rr);
                rr.release();
            }
            mRequestList.clear();
        }
    }

    private RILRequest findAndRemoveRequestFromList(int serial) {
        RILRequest rr = null;
        synchronized (mRequestList) {
            rr = mRequestList.get(serial);
            if (rr != null) {
                mRequestList.remove(serial);
            }
        }

        return rr;
    }

    private void addToRilHistogram(RILRequest rr) {
        long endTime = SystemClock.elapsedRealtime();
        int totalTime = (int) (endTime - rr.mStartTimeMs);

        synchronized (mRilTimeHistograms) {
            TelephonyHistogram entry = mRilTimeHistograms.get(rr.mRequest);
            if (entry == null) {
                // We would have total #RIL_HISTOGRAM_BUCKET_COUNT range buckets for RIL commands
                entry = new TelephonyHistogram(TelephonyHistogram.TELEPHONY_CATEGORY_RIL,
                        rr.mRequest, RIL_HISTOGRAM_BUCKET_COUNT);
                mRilTimeHistograms.put(rr.mRequest, entry);
            }
            entry.addTimeTaken(totalTime);
        }
    }

    RadioCapability makeStaticRadioCapability() {
        // default to UNKNOWN so we fail fast.
        int raf = RadioAccessFamily.RAF_UNKNOWN;

        String rafString = mContext.getResources().getString(
                com.android.internal.R.string.config_radio_access_family);
        if (!TextUtils.isEmpty(rafString)) {
            raf = RadioAccessFamily.rafTypeFromString(rafString);
        }
        RadioCapability rc = new RadioCapability(mPhoneId.intValue(), 0, 0, raf,
                "", RadioCapability.RC_STATUS_SUCCESS);
        if (RILJ_LOGD) riljLog("Faking RIL_REQUEST_GET_RADIO_CAPABILITY response using " + raf);
        return rc;
    }

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


    public void requestIsimAuthentication(String nonce, Message response) {
        mCi.requestIsimAuthentication(nonce, response);
    }


    public void requestIccSimAuthentication(int authContext, String data, String aid,
            Message response) {
        mCi.requestIccSimAuthentication(authContext, data, aid, response);
    }

    public void setInitialAttachSOSApn(DataProfile dataProfileInfo, Message result){
        IExtRadio radioProxy = getRadioProxy(result);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_SET_SOS_INITIAL_ATTACH_APN, result,
                    mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.setInitialAttachSOSApn(rr.mSerial, convertToHalDataProfile(dataProfileInfo));
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "setInitialAttachSOSApn", e);
            }
        }
    }

    public void setIMSInitialAttachApn(DataProfile dataProfileInfo, Message result){
        IExtRadio radioProxy = getRadioProxy(result);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_SET_IMS_INITIAL_ATTACH_APN, result,
                    mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.setIMSInitialAttachApn(rr.mSerial, convertToHalDataProfile(dataProfileInfo));
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "setIMSInitialAttachApn", e);
            }
        }
    }

    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        pw.println("RIL: " + this);
        pw.println(" mWakeLock=" + mWakeLock);
        pw.println(" mWakeLockTimeout=" + mWakeLockTimeout);
        synchronized (mRequestList) {
            synchronized (mWakeLock) {
                pw.println(" mWakeLockCount=" + mWakeLockCount);
            }
            int count = mRequestList.size();
            pw.println(" mRequestList count=" + count);
            for (int i = 0; i < count; i++) {
                RILRequest rr = mRequestList.valueAt(i);
                pw.println("  [" + rr.mSerial + "] " + imsRequestToString(rr.mRequest));
            }
        }
        pw.println(" mLastNITZTimeInfo=" + Arrays.toString(mLastNITZTimeInfo));
        pw.println(" mTestingEmergencyCall=" + mTestingEmergencyCall.get());
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
        }
    }


    public void
    getImsCurrentCalls (Message result) {
        IExtRadio radioProxy = getRadioProxy(result);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_GET_IMS_CURRENT_CALLS, result,
                    mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.getIMSCurrentCalls(rr.mSerial);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "getIMSCurrentCalls", e);
            }
        }
    }

    public void
    setImsVoiceCallAvailability(int state, Message response) {

        IExtRadio radioProxy = getRadioProxy(response);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_SET_IMS_VOICE_CALL_AVAILABILITY, response,
                    mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.setIMSVoiceCallAvailability(rr.mSerial, state);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "setImsVoiceCallAvailability", e);
            }
        }
    }


    public void
    getImsVoiceCallAvailability(Message response){
        IExtRadio radioProxy = getRadioProxy(response);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_GET_IMS_VOICE_CALL_AVAILABILITY, response,
                    mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.getIMSVoiceCallAvailability(rr.mSerial);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "getImsVoiceCallAvailability", e);
            }
        }
    }


    public void initISIM(String confUri, String instanceId, String impu,
            String impi, String domain, String xCap, String bspAddr,
            Message response) {

        IExtRadio radioProxy = getRadioProxy(response);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_INIT_ISIM, response,
                    mRILDefaultWorkSource);
            ArrayList<String> list = new ArrayList<String>();
            list.add(confUri);
            list.add(instanceId);
            list.add(impu);
            list.add(impi);
            list.add(domain);
            list.add(xCap);
            list.add(bspAddr);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.initISIM(rr.mSerial, list);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "initISIM", e);
            }
        }
    }

    public void disableIms(Message response){
        IExtRadio radioProxy = getRadioProxy(response);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_DISABLE_IMS, response,
                    mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.disableIMS(rr.mSerial);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "disableIms", e);
            }
        }
    }


    public void
    requestVolteCallMediaChange(int action, int callId, Message response) {

        IExtRadio radioProxy = getRadioProxy(response);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_IMS_CALL_REQUEST_MEDIA_CHANGE, response,
                    mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.requestVolteCallMediaChange(rr.mSerial, callId , action);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "requestVolteCallMediaChange", e);
            }
        }
    }


    public void
    responseVolteCallMediaChange(boolean isAccept, int callId, Message response) {
        IExtRadio radioProxy = getRadioProxy(null);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_IMS_CALL_RESPONSE_MEDIA_CHANGE, null,
                    mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            int mediaRequest = 1000;
            if(response != null){
                mediaRequest = response.arg1;
            }

            try {
                radioProxy.responseVolteCallMediaChange(rr.mSerial, callId , isAccept, mediaRequest);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "responseVolteCallMediaChange", e);
            }
        }
    }


    public void setImsSmscAddress(String smsc, Message response){
        IExtRadio radioProxy = getRadioProxy(response);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_SET_IMS_SMSC, response,
                    mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.setIMSSmscAddress(rr.mSerial, smsc);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "setImsSmscAddress", e);
            }
        }
    }


    public void requestVolteCallFallBackToVoice(int callId, Message response) {
        IExtRadio radioProxy = getRadioProxy(response);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_IMS_CALL_FALL_BACK_TO_VOICE, response,
                    mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.volteCallFallBackToVoice(rr.mSerial, callId);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "requestVolteCallFallBackToVoice", e);
            }
        }
    }

    public void
    requestInitialGroupCall(String numbers, Message response) {
        IExtRadio radioProxy = getRadioProxy(response);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_IMS_INITIAL_GROUP_CALL, response,
                    mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.IMSInitialGroupCall(rr.mSerial, numbers);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "requestInitialGroupCall", e);
            }
        }
    }


    public void
    requestAddGroupCall(String numbers, Message response) {
        IExtRadio radioProxy = getRadioProxy(response);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_IMS_ADD_TO_GROUP_CALL, response,
                    mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.IMSAddGroupCall(rr.mSerial, numbers);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "requestAddGroupCall", e);
            }
        }
    }

    public void
    setCallForward(int action, int cfReason, int serviceClass,
            String number, int timeSeconds, String ruleSet, Message response) {
        CallForwardInfoUri info = new CallForwardInfoUri();
        info.status = action;
        info.reason = cfReason;
        //number type
        if(number != null && PhoneNumberUtils.isUriNumber(number)){
            info.numberType = 1;
        } else {
            info.numberType = 2;
        }
        info.ton = PhoneNumberUtils.toaFromString(number);
        info.serviceClass = serviceClass;
        info.number = number;
        info.timeSeconds = timeSeconds;
        info.ruleset = ruleSet;

        IExtRadio radioProxy = getRadioProxy(response);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_SET_CALL_FORWARD_URI, response,
                    mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.setCallForwardUri(rr.mSerial, info);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "setCallForward", e);
            }
        }
    }

    public void
    queryCallForwardStatus(int cfReason, int serviceClass,
            String number, String ruleSet, Message response) {

        CallForwardInfoUri info = new CallForwardInfoUri();
        info.reason = cfReason;//TODO:check this param
        //number type
        if(number != null && PhoneNumberUtils.isUriNumber(number)){
            info.numberType = 1;
        } else {
            info.numberType = 2;
        }
        info.ton = PhoneNumberUtils.toaFromString(number);
        info.serviceClass = serviceClass;
        info.number = number;
        info.ruleset = ruleSet;
        info.status = 2; //Bug710475:
                         //AT+CCFCU <mode>: integer type :2---query status
        info.timeSeconds = 0;
        if (RILJ_LOGD) riljLog("[queryCallForwardStatus]CallForwardInfoUri status: 2");
        IExtRadio radioProxy = getRadioProxy(response);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_QUERY_CALL_FORWARD_STATUS_URI, response,
                    mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.queryCallForwardStatus(rr.mSerial, info);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "queryCallForwardStatus", e);
            }
        }
    }

    public void enableIms(Message response) {
        IExtRadio radioProxy = getRadioProxy(response);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_ENABLE_IMS, response,
                    mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.enableIMS(rr.mSerial);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "enableIMS", e);
            }
        }
    }

    protected RegistrantList mImsCallStateRegistrants = new RegistrantList();

    public void registerForImsCallStateChanged(Handler h, int what, Object obj) {
        Registrant r = new Registrant (h, what, obj);

        mImsCallStateRegistrants.add(r);
    }

    public void unregisterForImsCallStateChanged(Handler h) {
        mImsCallStateRegistrants.remove(h);
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

    protected RegistrantList mImsBearerStateRegistrant = new RegistrantList();
    public void registerForImsBearerStateChanged(Handler h, int what, Object obj) {
        Registrant r = new Registrant (h, what, obj);
        mImsBearerStateRegistrant.add(r);
    }

    public void unregisterForImsBearerStateChanged(Handler h) {
        mImsBearerStateRegistrant.remove(h);
    }

    protected Registrant mImsVideoQosRegistrant;
    public void registerForImsVideoQos(Handler h, int what, Object obj) {
        mImsVideoQosRegistrant = new Registrant(h, what, obj);
    }

    public void unregisterForImsVideoQos(Handler h) {
        mImsVideoQosRegistrant.clear();
    }

    public void getImsBearerState(Message response) {
        IExtRadio radioProxy = getRadioProxy(response);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_GET_IMS_BEARER_STATE, response,
                    mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.getIMSBearerState(rr.mSerial);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "getImsBearerState", e);
            }
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
        IExtRadio radioProxy = getRadioProxy(response);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_IMS_WIFI_ENABLE, response,
                    mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.notifyVoWifiEnable(rr.mSerial, enable);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "notifyVoWifiEnable", e);
            }
        }
    }

    public void notifyVoWifiCallStateChanged(boolean incall, Message response){
        IExtRadio radioProxy = getRadioProxy(response);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_IMS_WIFI_CALL_STATE_CHANGE, response,
                    mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.notifyVoWifiCallStateChanged(rr.mSerial, incall);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "notifyWifiCallState", e);
            }
        }
    }

    public void notifyDataRouter(Message response){
        IExtRadio radioProxy = getRadioProxy(response);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_IMS_UPDATE_DATA_ROUTER, response,
                    mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.notifyDataRouterUpdate(rr.mSerial);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "notifyDataRouter", e);
            }
        }
    }

    /* SPRD: add for VoWiFi
     */
    protected Registrant mImsHandoverRequestRegistrant;
    protected Registrant mImsHandoverStatusRegistrant;
    protected Registrant mImsNetworkInfoRegistrant;
    protected Registrant mImsRegAddressRegistrant;
    protected Registrant mImsWiFiParamRegistrant;
    protected RegistrantList mImsNetworkStateChangedRegistrants = new RegistrantList();

    public void registerForImsNetworkStateChanged(Handler h, int what, Object obj) {
        Registrant r = new Registrant (h, what, obj);
        mImsNetworkStateChangedRegistrants.add(r);
    }

    public void unregisterForImsNetworkStateChanged(Handler h) {
        mImsNetworkStateChangedRegistrants.remove(h);
    }

    public void registerImsHandoverRequest(Handler h, int what, Object obj){
        mImsHandoverRequestRegistrant = new Registrant(h, what, obj);
    }


    public void unregisterImsHandoverRequest(Handler h){
        mImsHandoverRequestRegistrant.clear();
    }

    public void registerImsHandoverStatus(Handler h, int what, Object obj){
        mImsHandoverStatusRegistrant = new Registrant(h, what, obj);
    }

    public void unregisterImsHandoverStatus(Handler h){
        mImsHandoverStatusRegistrant.clear();
    }

    public void registerImsNetworkInfo(Handler h, int what, Object obj){
        mImsNetworkInfoRegistrant = new Registrant(h, what, obj);
    }

    public void unregisterImsNetworkInfo(Handler h){
        mImsNetworkInfoRegistrant.clear();
    }

    public void registerImsRegAddress(Handler h, int what, Object obj) {
        mImsRegAddressRegistrant = new Registrant(h, what, obj);
    }

    public void unregisterImsRegAddress(Handler h){
        mImsRegAddressRegistrant.clear();
    }

    public void registerImsWiFiParam(Handler h, int what, Object obj){
        mImsWiFiParamRegistrant = new Registrant(h, what, obj);
    }


    public void unregisterImsWiFiParam(Handler h){
        mImsWiFiParamRegistrant.clear();
    }

    public void registerForIccRefresh(Handler h, int what, Object obj){
        mCi.registerForIccRefresh(h, what, obj);
    }

    public void requestImsHandover(int type, Message response) {
        IExtRadio radioProxy = getRadioProxy(response);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_IMS_HANDOVER, response,
                    mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.IMSHandover(rr.mSerial, type);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "requestImsHandover", e);
            }
        }
    }

    public void notifyImsHandoverStatus(int status, Message response){
        IExtRadio radioProxy = getRadioProxy(response);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_IMS_HANDOVER_STATUS_UPDATE, response,
                    mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.notifyIMSHandoverStatusUpdate(rr.mSerial, status);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "notifyImsHandoverStatus", e);
            }
        }
    }

    public void notifyImsNetworkInfo(int type, String info, Message response){
        ImsNetworkInfo imsNetworkInfo = new ImsNetworkInfo();
        imsNetworkInfo.info = info;
        imsNetworkInfo.type = type;


        IExtRadio radioProxy = getRadioProxy(response);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_IMS_NETWORK_INFO_CHANGE, response,
                    mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.notifyIMSNetworkInfoChanged(rr.mSerial, imsNetworkInfo);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "notifyIMSNetworkInfoChanged", e);
            }
        }
    }

    public void notifyImsCallEnd(int type, Message response){
        IExtRadio radioProxy = getRadioProxy(response);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_IMS_HANDOVER_CALL_END, response,
                    mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.notifyIMSCallEnd(rr.mSerial, type);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "notifyImsCallEnd", e);
            }
        }
    }

    public void imsHoldSingleCall(int callid, boolean enable, Message response){

        IExtRadio radioProxy = getRadioProxy(response);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_IMS_HOLD_SINGLE_CALL, response,
                    mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.IMSHoldSingleCall(rr.mSerial, callid, enable);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "imsHoldSingleCall", e);
            }
        }
    }

    public void imsMuteSingleCall(int callid, boolean enable, Message response){
        IExtRadio radioProxy = getRadioProxy(response);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_IMS_MUTE_SINGLE_CALL, response,
                    mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.IMSMuteSingleCall(rr.mSerial, callid, enable);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "imsMuteSingleCall", e);
            }
        }
    }

    public void imsSilenceSingleCall(int callid, boolean enable, Message response){

        IExtRadio radioProxy = getRadioProxy(response);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_IMS_SILENCE_SINGLE_CALL, response,
                    mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.IMSSilenceSingleCall(rr.mSerial, callid, enable);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "imsMuteSingleCall", e);
            }
        }
    }

    public void imsEnableLocalConference(boolean enable, Message response){
        IExtRadio radioProxy = getRadioProxy(response);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_IMS_ENABLE_LOCAL_CONFERENCE, response,
                    mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.IMSEnableLocalConference(rr.mSerial, enable);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "imsEnableLocalConference", e);
            }
        }
    }

    public void notifyHandoverCallInfo(String callInfo,Message response) {
        IExtRadio radioProxy = getRadioProxy(response);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_IMS_NOTIFY_HANDOVER_CALL_INFO, response,
                    mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.notifyHandoverCallInfo(rr.mSerial, callInfo);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "notifyHandoverCallInfo", e);
            }
        }
    }

    //SPRD:add for bug671964
    public void setImsPcscfAddress(String addr,Message response){
        IExtRadio radioProxy = getRadioProxy(response);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_SET_IMS_PCSCF_ADDR, response,
                    mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.setIMSPcscfAddress(rr.mSerial, addr);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "setImsPcscfAddress", e);
            }
        }
    }

    public void getSrvccCapbility(Message response) {
        IExtRadio radioProxy = getRadioProxy(response);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_GET_IMS_SRVCC_CAPBILITY, response,
                    mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.getSrvccCapbility(rr.mSerial);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "getSrvccCapbility", e);
            }
        }
    }

    public void
    getImsPcscfAddress(Message result) {
        IExtRadio radioProxy = getRadioProxy(result);
        if (radioProxy != null) {
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_GET_IMS_PCSCF_ADDR, result,
                    mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.getIMSPcscfAddress(rr.mSerial);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "getIMSPcscfAddress", e);
            }
        }
    }

    public void getImsRegAddress(Message result){
        IExtRadio radioProxy = getRadioProxy(result);
        if(radioProxy != null){
            RILRequest rr = obtainRequest(ImsRILConstants.RIL_REQUEST_GET_IMS_REGADDR,result,mRILDefaultWorkSource);

            if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

            try {
                radioProxy.getImsRegAddress(rr.mSerial);
            } catch (RemoteException | RuntimeException e) {
                handleRadioProxyExceptionForRR(rr, "getImsRegAddress", e);
            }
        }
    }

    public void
    updateCLIP(int enable, Message response) {
        if (getRadioInteractor() != null) {
            mRadioInteractor.updateCLIP(enable, response);
        }
    }
    /**
     * Convert to DataProfileInfo defined in types.hal
     * @param dp Data profile
     * @return A converted data profile
     */
    private static DataProfileInfo convertToHalDataProfile(DataProfile dp) {
        DataProfileInfo dpi = new DataProfileInfo();

        dpi.profileId = dp.profileId;
        dpi.apn = dp.apn;
        dpi.protocol = dp.protocol;
        dpi.roamingProtocol = dp.roamingProtocol;
        dpi.authType = dp.authType;
        dpi.user = dp.user;
        dpi.password = dp.password;
        dpi.type = dp.type;
        dpi.maxConnsTime = dp.maxConnsTime;
        dpi.maxConns = dp.maxConns;
        dpi.waitTime = dp.waitTime;
        dpi.enabled = dp.enabled;
        dpi.supportedApnTypesBitmap = dp.supportedApnTypesBitmap;
        dpi.bearerBitmap = dp.bearerBitmap;
        dpi.mtu = dp.mtu;
        dpi.mvnoType = convertToHalMvnoType(dp.mvnoType);
        dpi.mvnoMatchData = dp.mvnoMatchData;

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

    /*SPRD: add for bug899924 @{*/
    protected RegistrantList mNotAvailRegistrants = new RegistrantList();

    public void registerForNotAvailable(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mNotAvailRegistrants.add(r);
    }

    public void unRegisterForNotAvailable(Handler h) {
        mNotAvailRegistrants.remove(h);
    }
    /*@}*/

    public void setVideoResolution(int resolution, Message result){
        if(getRadioInteractor() != null) {
            mRadioInteractor.setVideoResolution(resolution, result);
        }
    }
    public void enableLocalHold(boolean enable, Message result){
        if(getRadioInteractor() != null) {
            mRadioInteractor.enableLocalHold(enable, result);
        }
    }
    public void enableWiFiParamReport(boolean enable, Message result){
        if(getRadioInteractor() != null) {
            mRadioInteractor.enableWiFiParamReport(enable, result);
        }
    }
    public void callMediaChangeRequestTimeOut(int callId, Message result){
        if(getRadioInteractor() != null) {
            mRadioInteractor.callMediaChangeRequestTimeOut(callId, result);
        }
    }

}

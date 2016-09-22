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
import android.os.Looper;
import android.os.Message;
import android.os.Parcel;
import android.os.PowerManager;
import android.os.BatteryManager;
import android.os.Registrant;
import android.os.RegistrantList;
import android.os.SystemProperties;
import android.os.PowerManager.WakeLock;
import android.os.Registrant;
import android.os.SystemClock;
import android.provider.Settings.SettingNotFoundException;
import android.telephony.CellInfo;
import android.telephony.NeighboringCellInfo;
import android.telephony.PhoneNumberUtils;
import android.telephony.RadioAccessFamily;
import android.telephony.Rlog;
import android.telephony.SignalStrength;
import android.telephony.SmsManager;
import android.telephony.SmsMessage;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.telephony.ModemActivityInfo;
import android.text.TextUtils;
import android.util.SparseArray;
import android.view.Display;

import com.android.internal.telephony.gsm.SmsBroadcastConfigInfo;
import com.android.internal.telephony.gsm.SsData;
import com.android.internal.telephony.gsm.SuppServiceNotification;
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

import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.FileDescriptor;
import java.io.IOException;
import java.io.InputStream;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.Random;

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
    private Context mContext;

    //***** Instance Variables
    int mSerial;
    int mRequest;
    Message mResult;
    Parcel mParcel;
    RILRequest mNext;

    /**
     * Retrieves a new RILRequest instance from the pool.
     *
     * @param request RIL_REQUEST_*
     * @param result sent when operation completes
     * @return a RILRequest instance from the pool.
     */
    static RILRequest obtain(int request, Message result) {
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
        rr.mParcel = Parcel.obtain();

        if (result != null && result.getTarget() == null) {
            throw new NullPointerException("Message target must not be null");
        }

        // first elements in any RIL Parcel
        rr.mParcel.writeInt(request);
        rr.mParcel.writeInt(rr.mSerial);

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

        if (ImsRIL.RILJ_LOGD) Rlog.d(LOG_TAG, serialString() + "< "
                + ImsRIL.imsRequestToString(mRequest)
                + " error: " + ex + " ret=" + ImsRIL.retToString(mRequest, ret));

        if (mResult != null) {
            AsyncResult.forMessage(mResult, ret, ex);
            mResult.sendToTarget();
        }

        if (mParcel != null) {
            mParcel.recycle();
            mParcel = null;
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

    //***** Instance Variables
    private Context mContext;
    CommandsInterface mCi;
    LocalSocket mSocket;
    HandlerThread mSenderThread;
    RILSender mSender;
    Thread mReceiverThread;
    RILReceiver mReceiver;
    Display mDefaultDisplay;
    int mDefaultDisplayState = Display.STATE_UNKNOWN;
    int mRadioScreenState = RADIO_SCREEN_UNSET;
    boolean mIsDevicePlugged = false;
    WakeLock mWakeLock;
    final int mWakeLockTimeout;
    // The number of wakelock requests currently active.  Don't release the lock
    // until dec'd to 0
    int mWakeLockCount;

    SparseArray<RILRequest> mRequestList = new SparseArray<RILRequest>();

    Object     mLastNITZTimeInfo;

    // When we are testing emergency calls
    AtomicBoolean mTestingEmergencyCall = new AtomicBoolean(false);

    private Integer mInstanceId;

    //***** Events

    static final int EVENT_SEND                 = 1;
    static final int EVENT_WAKE_LOCK_TIMEOUT    = 2;

    //***** Constants

    // match with constant in ril.cpp
    static final int RIL_MAX_COMMAND_BYTES = (8 * 1024);
    static final int RESPONSE_SOLICITED = 0;
    static final int RESPONSE_UNSOLICITED = 1;

    static final String[] SOCKET_NAME_RIL = {"ims_socket1", "ims_socket2", "ims_socket3","ims_socket4"};

    static final int SOCKET_OPEN_RETRY_MILLIS = 4 * 1000;

    // The number of the required config values for broadcast SMS stored in the C struct
    // RIL_CDMA_BroadcastServiceInfo
    private static final int CDMA_BSI_NO_OF_INTS_STRUCT = 3;

    private static final int CDMA_BROADCAST_SMS_NO_OF_SERVICE_CATEGORIES = 31;

    class RILSender extends Handler implements Runnable {
        public RILSender(Looper looper) {
            super(looper);
        }

        // Only allocated once
        byte[] dataLength = new byte[4];

        //***** Runnable implementation
        @Override
        public void
        run() {
            //setup if needed
        }


        //***** Handler implementation
        @Override public void
        handleMessage(Message msg) {
            RILRequest rr = (RILRequest)(msg.obj);
            RILRequest req = null;

            switch (msg.what) {
                case EVENT_SEND:
                    try {
                        LocalSocket s;

                        s = mSocket;

                        if (s == null) {
                            rr.onError(RADIO_NOT_AVAILABLE, null);
                            rr.release();
                            decrementWakeLock();
                            return;
                        }

                        synchronized (mRequestList) {
                            mRequestList.append(rr.mSerial, rr);
                        }

                        byte[] data;

                        data = rr.mParcel.marshall();
                        rr.mParcel.recycle();
                        rr.mParcel = null;

                        if (data.length > RIL_MAX_COMMAND_BYTES) {
                            throw new RuntimeException(
                                    "Parcel larger than max bytes allowed! "
                                            + data.length);
                        }

                        // parcel length in big endian
                        dataLength[0] = dataLength[1] = 0;
                        dataLength[2] = (byte)((data.length >> 8) & 0xff);
                        dataLength[3] = (byte)((data.length) & 0xff);

                        //Rlog.v(RILJ_LOG_TAG, "writing packet: " + data.length + " bytes");

                        s.getOutputStream().write(dataLength);
                        s.getOutputStream().write(data);
                    } catch (IOException ex) {
                        Rlog.e(RILJ_LOG_TAG, "IOException", ex);
                        req = findAndRemoveRequestFromList(rr.mSerial);
                        // make sure this request has not already been handled,
                        // eg, if RILReceiver cleared the list.
                        if (req != null) {
                            rr.onError(RADIO_NOT_AVAILABLE, null);
                            rr.release();
                            decrementWakeLock();
                        }
                    } catch (RuntimeException exc) {
                        Rlog.e(RILJ_LOG_TAG, "Uncaught exception ", exc);
                        req = findAndRemoveRequestFromList(rr.mSerial);
                        // make sure this request has not already been handled,
                        // eg, if RILReceiver cleared the list.
                        if (req != null) {
                            rr.onError(GENERIC_FAILURE, null);
                            rr.release();
                            decrementWakeLock();
                        }
                    }

                    break;

                case EVENT_WAKE_LOCK_TIMEOUT:
                    // Haven't heard back from the last request.  Assume we're
                    // not getting a response and  release the wake lock.

                    // The timer of WAKE_LOCK_TIMEOUT is reset with each
                    // new send request. So when WAKE_LOCK_TIMEOUT occurs
                    // all requests in mRequestList already waited at
                    // least DEFAULT_WAKE_LOCK_TIMEOUT but no response.
                    //
                    // Note: Keep mRequestList so that delayed response
                    // can still be handled when response finally comes.

                    synchronized (mRequestList) {
                        if (clearWakeLock()) {
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
            }
        }
    }

    /**
     * Reads in a single RIL message off the wire. A RIL message consists
     * of a 4-byte little-endian length and a subsequent series of bytes.
     * The final message (length header omitted) is read into
     * <code>buffer</code> and the length of the final message (less header)
     * is returned. A return value of -1 indicates end-of-stream.
     *
     * @param is non-null; Stream to read from
     * @param buffer Buffer to fill in. Must be as large as maximum
     * message size, or an ArrayOutOfBounds exception will be thrown.
     * @return Length of message less header, or -1 on end of stream.
     * @throws IOException
     */
    private static int readRilMessage(InputStream is, byte[] buffer)
            throws IOException {
        int countRead;
        int offset;
        int remaining;
        int messageLength;

        // First, read in the length of the message
        offset = 0;
        remaining = 4;
        do {
            countRead = is.read(buffer, offset, remaining);

            if (countRead < 0 ) {
                Rlog.e(RILJ_LOG_TAG, "Hit EOS reading message length");
                return -1;
            }

            offset += countRead;
            remaining -= countRead;
        } while (remaining > 0);

        messageLength = ((buffer[0] & 0xff) << 24)
                | ((buffer[1] & 0xff) << 16)
                | ((buffer[2] & 0xff) << 8)
                | (buffer[3] & 0xff);

        // Then, re-use the buffer and read in the message itself
        offset = 0;
        remaining = messageLength;
        do {
            countRead = is.read(buffer, offset, remaining);

            if (countRead < 0 ) {
                Rlog.e(RILJ_LOG_TAG, "Hit EOS reading message.  messageLength=" + messageLength
                        + " remaining=" + remaining);
                return -1;
            }

            offset += countRead;
            remaining -= countRead;
        } while (remaining > 0);

        return messageLength;
    }

    class RILReceiver implements Runnable {
        byte[] buffer;

        RILReceiver() {
            buffer = new byte[RIL_MAX_COMMAND_BYTES];
        }

        @Override
        public void
        run() {
            int retryCount = 0;
            String rilSocket = "rild";

            try {for (;;) {
                LocalSocket s = null;
                LocalSocketAddress l;

                if (mInstanceId == null || mInstanceId == 0 ) {
                    rilSocket = SOCKET_NAME_RIL[0];
                } else {
                    rilSocket = SOCKET_NAME_RIL[mInstanceId];
                }

                try {
                    s = new LocalSocket();
                    l = new LocalSocketAddress(rilSocket,
                            LocalSocketAddress.Namespace.ABSTRACT);
                    s.connect(l);
                } catch (IOException ex){
                    try {
                        if (s != null) {
                            s.close();
                        }
                    } catch (IOException ex2) {
                        //ignore failure to close after failure to connect
                    }

                    // don't print an error message after the the first time
                    // or after the 8th time

                    if (retryCount == 8) {
                        Rlog.e (RILJ_LOG_TAG,
                                "Couldn't find '" + rilSocket
                                + "' socket after " + retryCount
                                + " times, continuing to retry silently");
                    } else if (retryCount >= 0 && retryCount < 8) {
                        Rlog.i (RILJ_LOG_TAG,
                                "Couldn't find '" + rilSocket
                                + "' socket; retrying after timeout");
                    }

                    try {
                        Thread.sleep(SOCKET_OPEN_RETRY_MILLIS);
                    } catch (InterruptedException er) {
                    }

                    retryCount++;
                    continue;
                }

                retryCount = 0;

                mSocket = s;
                Rlog.i(RILJ_LOG_TAG, "(" + mInstanceId + ") Connected to '"
                        + rilSocket + "' socket");

                int length = 0;
                try {
                    InputStream is = mSocket.getInputStream();

                    for (;;) {
                        Parcel p;

                        length = readRilMessage(is, buffer);

                        if (length < 0) {
                            // End-of-stream reached
                            break;
                        }

                        p = Parcel.obtain();
                        p.unmarshall(buffer, 0, length);
                        p.setDataPosition(0);

                        //Rlog.v(RILJ_LOG_TAG, "Read packet: " + length + " bytes");

                        processResponse(p);
                        p.recycle();
                    }
                } catch (java.io.IOException ex) {
                    Rlog.i(RILJ_LOG_TAG, "'" + rilSocket + "' socket closed",
                            ex);
                } catch (Throwable tr) {
                    Rlog.e(RILJ_LOG_TAG, "Uncaught exception read length=" + length +
                            "Exception:" + tr.toString());
                }

                Rlog.i(RILJ_LOG_TAG, "(" + mInstanceId + ") Disconnected from '" + rilSocket
                        + "' socket");
                try {
                    mSocket.close();
                } catch (IOException ex) {
                }

                mSocket = null;
                RILRequest.resetSerial();

                // Clear request list on close
                clearRequestList(RADIO_NOT_AVAILABLE, false);
            }} catch (Throwable tr) {
                Rlog.e(RILJ_LOG_TAG,"Uncaught exception", tr);
            }

            /* We're disconnected so we don't know the ril version */
        }
    }



    //***** Constructors
    public ImsRIL(Context context, Integer instanceId, CommandsInterface ci) {
        mCi = ci;
        mContext = context;
        mInstanceId = instanceId;

        PowerManager pm = (PowerManager)context.getSystemService(Context.POWER_SERVICE);
        mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, RILJ_LOG_TAG);
        mWakeLock.setReferenceCounted(false);
        mWakeLockTimeout = SystemProperties.getInt(TelephonyProperties.PROPERTY_WAKE_LOCK_TIMEOUT,
                DEFAULT_WAKE_LOCK_TIMEOUT);
        mWakeLockCount = 0;

        mSenderThread = new HandlerThread("RILSender" + mInstanceId);
        mSenderThread.start();

        Looper looper = mSenderThread.getLooper();
        mSender = new RILSender(looper);

        ConnectivityManager cm = (ConnectivityManager)context.getSystemService(
                Context.CONNECTIVITY_SERVICE);
        if (cm.isNetworkSupported(ConnectivityManager.TYPE_MOBILE) == false) {
            riljLog("Not starting RILReceiver: wifi-only");
        } else {
            riljLog("Starting RILReceiver" + mInstanceId);
            mReceiver = new RILReceiver();
            mReceiverThread = new Thread(mReceiver, "RILReceiver" + mInstanceId);
            mReceiverThread.start();

        }
    }

    public void getVoiceRadioTechnology(Message result) {
        mCi.getVoiceRadioTechnology(result);
    }


    public void getImsRegistrationState(Message result) {
        mCi.getImsRegistrationState(result);
    }

    public void
    setOnNITZTime(Handler h, int what, Object obj) {
        mCi.setOnNITZTime(h,what,obj);
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
    sendSMS (String smscPDU, String pdu, Message result) {
        mCi.sendSMS(smscPDU, pdu, result);
    }


    public void
    sendSMSExpectMore (String smscPDU, String pdu, Message result) {
        mCi.sendSMSExpectMore(smscPDU, pdu, result);
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
    setupDataCall(int radioTechnology, int profile, String apn,
            String user, String password, int authType, String protocol,
            Message result) {
        mCi.setupDataCall(radioTechnology, profile, apn,
                user, password, authType, protocol,result);
    }


    public void
    deactivateDataCall(int cid, int reason, Message result) {
        mCi.deactivateDataCall(cid, reason, result);
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
    acknowledgeLastIncomingGsmSms(boolean success, int cause, Message result) {
        mCi.acknowledgeLastIncomingGsmSms(success, cause, result);
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


    public void invokeOemRilRequestStrings(String[] strings, Message response) {
        mCi.invokeOemRilRequestStrings(strings, response);
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

    public void getNeighboringCids(Message response) {
        mCi.getNeighboringCids(response);
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


    /**
     * Holds a PARTIAL_WAKE_LOCK whenever
     * a) There is outstanding RIL request sent to RIL deamon and no replied
     * b) There is a request pending to be sent out.
     *
     * There is a WAKE_LOCK_TIMEOUT to release the lock, though it shouldn't
     * happen often.
     */

    private void
    acquireWakeLock() {
        synchronized (mWakeLock) {
            mWakeLock.acquire();
            mWakeLockCount++;

            mSender.removeMessages(EVENT_WAKE_LOCK_TIMEOUT);
            Message msg = mSender.obtainMessage(EVENT_WAKE_LOCK_TIMEOUT);
            mSender.sendMessageDelayed(msg, mWakeLockTimeout);
        }
    }

    private void
    decrementWakeLock() {
        synchronized (mWakeLock) {
            if (mWakeLockCount > 1) {
                mWakeLockCount--;
            } else {
                mWakeLockCount = 0;
                mWakeLock.release();
                mSender.removeMessages(EVENT_WAKE_LOCK_TIMEOUT);
            }
        }
    }

    // true if we had the wakelock
    private boolean
    clearWakeLock() {
        synchronized (mWakeLock) {
            if (mWakeLockCount == 0 && mWakeLock.isHeld() == false) return false;
            Rlog.d(RILJ_LOG_TAG, "NOTE: mWakeLockCount is " + mWakeLockCount + "at time of clearing");
            mWakeLockCount = 0;
            mWakeLock.release();
            mSender.removeMessages(EVENT_WAKE_LOCK_TIMEOUT);
            return true;
        }
    }

    private void
    send(RILRequest rr) {
        Message msg;

        if (mSocket == null) {
            rr.onError(RADIO_NOT_AVAILABLE, null);
            rr.release();
            return;
        }

        msg = mSender.obtainMessage(EVENT_SEND, rr);

        acquireWakeLock();

        msg.sendToTarget();
    }

    private void
    processResponse (Parcel p) {
        int type;

        type = p.readInt();

        if (type == RESPONSE_UNSOLICITED) {
            processUnsolicited (p);
        } else if (type == RESPONSE_SOLICITED) {
            RILRequest rr = processSolicited (p);
            if (rr != null) {
                rr.release();
                decrementWakeLock();
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
                Rlog.d(RILJ_LOG_TAG, "clearRequestList " +
                        " mWakeLockCount=" + mWakeLockCount +
                        " mRequestList=" + count);
            }

            for (int i = 0; i < count ; i++) {
                rr = mRequestList.valueAt(i);
                if (RILJ_LOGD && loggable) {
                    Rlog.d(RILJ_LOG_TAG, i + ": [" + rr.mSerial + "] " +
                            imsRequestToString(rr.mRequest));
                }
                rr.onError(error, null);
                rr.release();
                decrementWakeLock();
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

    private RILRequest
    processSolicited (Parcel p) {
        int serial, error;
        boolean found = false;

        serial = p.readInt();
        error = p.readInt();

        RILRequest rr;

        rr = findAndRemoveRequestFromList(serial);

        if (rr == null) {
            Rlog.w(RILJ_LOG_TAG, "Unexpected solicited response! sn: "
                    + serial + " error: " + error);
            return null;
        }

        Object ret = null;

        if (error == 0 || p.dataAvail() > 0) {
            // either command succeeds or command fails but with data payload
            try {switch (rr.mRequest) {
                /*
 cat libs/telephony/ril_commands.h \
 | egrep "^ *{RIL_" \
 | sed -re 's/\{([^,]+),[^,]+,([^}]+).+/case \1: ret = \2(p); break;/'
                 */
                case RIL_REQUEST_IMS_REGISTRATION_STATE: ret = responseInts(p); break;
                case RIL_REQUEST_IMS_SEND_SMS: ret =  responseSMS(p); break;
                case ImsRILConstants.RIL_REQUEST_GET_IMS_CURRENT_CALLS: ret = responseImsCallList(p);break;
                case ImsRILConstants.RIL_REQUEST_SET_IMS_VOICE_CALL_AVAILABILITY: ret = responseVoid(p); break;
                case ImsRILConstants.RIL_REQUEST_GET_IMS_VOICE_CALL_AVAILABILITY: ret = responseInts(p); break;
                case ImsRILConstants.RIL_REQUEST_INIT_ISIM: ret =  responseInts(p);break;
                case ImsRILConstants.RIL_REQUEST_SET_IMS_SMSC: ret = responseVoid(p); break;
                case ImsRILConstants.RIL_REQUEST_DISABLE_IMS: ret = responseVoid(p); break;
                case ImsRILConstants.RIL_REQUEST_IMS_CALL_REQUEST_MEDIA_CHANGE : ret = responseVoid(p); break;
                case ImsRILConstants.RIL_REQUEST_IMS_CALL_RESPONSE_MEDIA_CHANGE: ret = responseVoid(p); break;
                case ImsRILConstants.RIL_REQUEST_IMS_CALL_FALL_BACK_TO_VOICE: ret = responseVoid(p); break;
                case ImsRILConstants.RIL_REQUEST_SET_IMS_INITIAL_ATTACH_APN: ret = responseVoid(p); break;
                case ImsRILConstants.RIL_REQUEST_QUERY_CALL_FORWARD_STATUS_URI: ret =  responseCallForwardUri(p); break;
                case ImsRILConstants.RIL_REQUEST_SET_CALL_FORWARD_URI: ret =  responseVoid(p); break;
                case ImsRILConstants.RIL_REQUEST_IMS_INITIAL_GROUP_CALL: ret =  responseVoid(p); break;
                case ImsRILConstants.RIL_REQUEST_IMS_ADD_TO_GROUP_CALL: ret =  responseVoid(p); break;
                case ImsRILConstants.RIL_REQUEST_ENABLE_IMS: ret = responseVoid(p); break;
                case ImsRILConstants.RIL_REQUEST_GET_IMS_BEARER_STATE: ret = responseInts(p); break;
                case ImsRILConstants.RIL_REQUEST_VIDEOPHONE_DIAL: ret = responseVoid(p); break;
                default:
                    throw new RuntimeException("Unrecognized solicited response: " + rr.mRequest);
                    //break;
            }} catch (Throwable tr) {
                // Exceptions here usually mean invalid RIL responses

                Rlog.w(RILJ_LOG_TAG, rr.serialString() + "< "
                        + imsRequestToString(rr.mRequest)
                        + " exception, possible invalid RIL response", tr);

                if (rr.mResult != null) {
                    AsyncResult.forMessage(rr.mResult, null, tr);
                    rr.mResult.sendToTarget();
                }
                return rr;
            }
        }

        if (rr.mRequest == RIL_REQUEST_SHUTDOWN) {
            // Set RADIO_STATE to RADIO_UNAVAILABLE to continue shutdown process
            // regardless of error code to continue shutdown procedure.
            riljLog("Response to RIL_REQUEST_SHUTDOWN received. Error is " +
                    error + " Setting Radio State to Unavailable regardless of error.");
        }

        if (error != 0) {
            rr.onError(error, ret);
        }
        if (error == 0) {

            if (RILJ_LOGD) riljLog(rr.serialString() + "< " + imsRequestToString(rr.mRequest)
                    + " " + retToString(rr.mRequest, ret));

            if (rr.mResult != null) {
                AsyncResult.forMessage(rr.mResult, ret, null);
                rr.mResult.sendToTarget();
            }
        }
        return rr;
    }

    static String
    retToString(int req, Object ret) {
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
        if (ret instanceof int[]){
            int[] intArray = (int[]) ret;
            length = intArray.length;
            sb = new StringBuilder("{");
            if (length > 0) {
                int i = 0;
                sb.append(intArray[i++]);
                while ( i < length) {
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
                while ( i < length) {
                    sb.append(", ").append(strings[i++]);
                }
            }
            sb.append("}");
            s = sb.toString();
        }else if (req == RIL_REQUEST_GET_CURRENT_CALLS) {
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
            for(int i = 0; i < length; i++) {
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

    private void
    processUnsolicited (Parcel p) {
        int response;
        Object ret;

        response = p.readInt();



        try {switch(response) {
            /*
 cat libs/telephony/ril_unsol_commands.h \
 | egrep "^ *{RIL_" \
 | sed -re 's/\{([^,]+),[^,]+,([^}]+).+/case \1: \2(rr, p); break;/'
             */
            case ImsRILConstants.RIL_UNSOL_RESPONSE_IMS_CALL_STATE_CHANGED: ret = responseVoid(p); break;
            case ImsRILConstants.RIL_UNSOL_RESPONSE_IMS_BEARER_ESTABLISTED: ret = responseInts(p); break;
            case ImsRILConstants.RIL_UNSOL_RESPONSE_VIDEO_QUALITY: ret = responseInts(p); break;
            default:
                throw new RuntimeException("Unrecognized unsol response: " + response);
                //break; (implied)
        }} catch (Throwable tr) {
            Rlog.e(RILJ_LOG_TAG, "Exception processing unsol response: " + response +
                    "Exception:" + tr.toString());
            return;
        }

        switch(response) {
            case ImsRILConstants.RIL_UNSOL_RESPONSE_IMS_CALL_STATE_CHANGED:
                if (RILJ_LOGD) unsljLog(response);
                mImsCallStateRegistrants.notifyRegistrants(new AsyncResult(null, null, null));
                break;
            case ImsRILConstants.RIL_UNSOL_RESPONSE_IMS_BEARER_ESTABLISTED:
                if (mImsBearerStateRegistrant != null) {
                    mImsBearerStateRegistrant.notifyRegistrant(new AsyncResult(null, ret, null));
                }
                break;
            case ImsRILConstants.RIL_UNSOL_RESPONSE_VIDEO_QUALITY:
                if (mImsVideoQosRegistrant != null) {
                    mImsVideoQosRegistrant.notifyRegistrant(new AsyncResult(null, ret, null));
                }
                break;
        }
    }

    private Object
    responseInts(Parcel p) {
        int numInts;
        int response[];

        numInts = p.readInt();

        response = new int[numInts];

        for (int i = 0 ; i < numInts ; i++) {
            response[i] = p.readInt();
        }

        return response;
    }

    private Object
    responseFailCause(Parcel p) {
        LastCallFailCause failCause = new LastCallFailCause();
        failCause.causeCode = p.readInt();
        if (p.dataAvail() > 0) {
            failCause.vendorCause = p.readString();
        }
        return failCause;
    }

    private Object
    responseVoid(Parcel p) {
        return null;
    }

    private Object
    responseString(Parcel p) {
        String response;

        response = p.readString();

        return response;
    }

    private Object
    responseStrings(Parcel p) {
        int num;
        String response[];

        response = p.readStringArray();

        return response;
    }

    private Object
    responseRaw(Parcel p) {
        int num;
        byte response[];

        response = p.createByteArray();

        return response;
    }

    private Object
    responseSMS(Parcel p) {
        int messageRef, errorCode;
        String ackPDU;

        messageRef = p.readInt();
        ackPDU = p.readString();
        errorCode = p.readInt();

        SmsResponse response = new SmsResponse(messageRef, ackPDU, errorCode);

        return response;
    }

    static String
    requestToString(int request) {

        /*
 cat libs/telephony/ril_commands.h \
 | egrep "^ *{RIL_" \
 | sed -re 's/\{RIL_([^,]+),[^,]+,([^}]+).+/case RIL_\1: return "\1";/'
         */
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
            case RIL_UNSOL_RESPONSE_IMS_NETWORK_STATE_CHANGED:
                return "UNSOL_RESPONSE_IMS_NETWORK_STATE_CHANGED";
            case RIL_UNSOL_SRVCC_STATE_NOTIFY:
                return "UNSOL_SRVCC_STATE_NOTIFY";
            case ImsRILConstants.RIL_UNSOL_RESPONSE_IMS_CALL_STATE_CHANGED: return " RIL_UNSOL_RESPONSE_IMS_CALL_STATE_CHANGED";
            case ImsRILConstants.RIL_UNSOL_RESPONSE_IMS_BEARER_ESTABLISTED: return "RIL_UNSOL_RESPONSE_IMS_BEARER_ESTABLISTED";
            default: return "<unknown response>";
        }
    }

    private void riljLog(String msg) {
        Rlog.d(RILJ_LOG_TAG, msg
                + (mInstanceId != null ? (" [SUB" + mInstanceId + "]") : ""));
    }

    private void riljLogv(String msg) {
        Rlog.v(RILJ_LOG_TAG, msg
                + (mInstanceId != null ? (" [SUB" + mInstanceId + "]") : ""));
    }

    private void unsljLog(int response) {
        riljLog("[UNSL]< " + responseToString(response));
    }

    private void unsljLogMore(int response, String more) {
        riljLog("[UNSL]< " + responseToString(response) + " " + more);
    }

    private void unsljLogRet(int response, Object ret) {
        riljLog("[UNSL]< " + responseToString(response) + " " + retToString(response, ret));
    }

    private void unsljLogvRet(int response, Object ret) {
        riljLogv("[UNSL]< " + responseToString(response) + " " + retToString(response, ret));
    }


    // ***** Methods for CDMA support

    public void
    getDeviceIdentity(Message response) {
        mCi.getDeviceIdentity(response);
    }

    /**
     * {@inheritDoc}
     */

    public void queryTTYMode(Message response) {
        mCi.queryTTYMode(response);
    }

    /**
     * {@inheritDoc}
     */

    public void setTTYMode(int ttyMode, Message response) {
        mCi.setTTYMode(ttyMode, response);
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

    /**
     * {@inheritDoc}
     */

    public void getCellInfoList(Message result) {
        mCi.getCellInfoList(result);
    }

    /**
     * {@inheritDoc}
     */

    public void setCellInfoListRate(int rateInMillis, Message response) {
        mCi.setCellInfoListRate(rateInMillis, response);
    }

    public void setInitialAttachApn(String apn, String protocol, int authType, String username,
            String password, Message result) {
        RILRequest rr = RILRequest.obtain(RIL_REQUEST_SET_INITIAL_ATTACH_APN, null);

        if (RILJ_LOGD) riljLog("Set RIL_REQUEST_SET_INITIAL_ATTACH_APN");

        rr.mParcel.writeString(apn);
        rr.mParcel.writeString(protocol);
        rr.mParcel.writeInt(authType);
        rr.mParcel.writeString(username);
        rr.mParcel.writeString(password);

        if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest)
                + ", apn:" + apn + ", protocol:" + protocol + ", authType:" + authType
                + ", username:" + username + ", password:" + password);

        send(rr);
    }

    public void setDataProfile(DataProfile[] dps, Message result) {
        if (RILJ_LOGD) riljLog("Set RIL_REQUEST_SET_DATA_PROFILE");

        RILRequest rr = RILRequest.obtain(RIL_REQUEST_SET_DATA_PROFILE, null);
        DataProfile.toParcel(rr.mParcel, dps);

        if (RILJ_LOGD) {
            riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest)
                    + " with " + dps + " Data Profiles : ");
            for (int i = 0; i < dps.length; i++) {
                riljLog(dps[i].toString());
            }
        }

        send(rr);
    }

    /* (non-Javadoc)
     * @see com.android.internal.telephony.BaseCommands#testingEmergencyCall()
     */

    public void testingEmergencyCall() {
        if (RILJ_LOGD) riljLog("testingEmergencyCall");
        mTestingEmergencyCall.set(true);
    }

    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        pw.println("RIL: " + this);
        pw.println(" mSocket=" + mSocket);
        pw.println(" mSenderThread=" + mSenderThread);
        pw.println(" mSender=" + mSender);
        pw.println(" mReceiverThread=" + mReceiverThread);
        pw.println(" mReceiver=" + mReceiver);
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
        pw.println(" mLastNITZTimeInfo=" + mLastNITZTimeInfo);
        pw.println(" mTestingEmergencyCall=" + mTestingEmergencyCall.get());
    }

    /**
     * {@inheritDoc}
     */
    public void iccOpenLogicalChannel(String AID, Message response) {
        mCi.iccOpenLogicalChannel(AID, response);
    }

    /**
     * {@inheritDoc}
     */
    public void iccCloseLogicalChannel(int channel, Message response) {
        mCi.iccCloseLogicalChannel(channel, response);
    }


    public void nvReadItem(int itemID, Message response) {
        mCi.nvReadItem(itemID, response);
    }


    public void nvWriteItem(int itemID, String itemValue, Message response) {
        mCi.nvWriteItem(itemID, itemValue, response);
    }


    public void nvResetConfig(int resetType, Message response) {
        mCi.nvResetConfig(resetType, response);
    }


    public void setRadioCapability(RadioCapability rc, Message response) {
        mCi.setRadioCapability(rc, response);
    }


    public void getRadioCapability(Message response) {
        mCi.getRadioCapability(response);
    }


    public void startLceService(int reportIntervalMs, boolean pullMode, Message response) {
        mCi.startLceService(reportIntervalMs, pullMode, response);
    }


    public void stopLceService(Message response) {
        mCi.stopLceService(response);
    }


    public void pullLceData(Message response) {
        mCi.pullLceData(response);
    }

    /**
     * @hide
     */
    public void getModemActivityInfo(Message response) {
        mCi.getModemActivityInfo(response);
    }

    public void dialVP(String address, String sub_address, int clirMode, Message result) {
        riljLog("ril--dialVP: address = " + address);
        RILRequest rr = RILRequest.obtain(ImsRILConstants.RIL_REQUEST_VIDEOPHONE_DIAL,
                result);

        rr.mParcel.writeString(address);
        rr.mParcel.writeString(sub_address);
        rr.mParcel.writeInt(clirMode);

        if (RILJ_LOGD)
            riljLog(rr.serialString() + "> " + ImsRIL.imsRequestToString(rr.mRequest));
        send(rr);
    }


    public void
    getImsCurrentCalls (Message result) {
        RILRequest rr;
        rr = RILRequest.obtain(ImsRILConstants.RIL_REQUEST_GET_IMS_CURRENT_CALLS, result);
        if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

        send(rr);
    }

    protected Object
    responseImsCallList(Parcel p) {
        int num;
        int csMode;
        ArrayList<ImsDriverCall> response;
        ImsDriverCall dc;

        num = p.readInt();
        response = new ArrayList<ImsDriverCall>(num);

        if (RILJ_LOGV) {
            riljLog("responseCallListEx: num=" + num +
                    " mTestingEmergencyCall=" + mTestingEmergencyCall.get());
        }
        for (int i = 0 ; i < num ; i++) {
            dc = new ImsDriverCall();
            /* parameter from +CLCCS:
             * [+CLCCS: <ccid1>,<dir>,<neg_status_present>,<neg_status>,<SDP_md>,
             * <cs_mode>,<ccstatus>,<mpty>,[,<numbertype>,<ton>,<number>
             * [,<priority_present>,<priority>[,<CLI_validity_present>,<CLI_validity>]]]
             * @{*/
            dc.index = p.readInt();
            dc.isMT = (0 != p.readInt());
            dc.negStatusPresent = p.readInt();
            dc.negStatus = p.readInt();
            dc.mediaDescription = p.readString();
            csMode = p.readInt();
            dc.csMode = csMode;
            dc.state = ImsDriverCall.stateFromCLCCS(p.readInt());
            boolean videoMediaPresent = false;
            if(dc.mediaDescription != null && dc.mediaDescription.contains("video") && !dc.mediaDescription.contains("cap:")){
               videoMediaPresent = true;
            }
            boolean videoMode = videoMediaPresent &&
                    ((dc.negStatusPresent == 1 && dc.negStatus == 1)
                            ||(dc.negStatusPresent == 1 && dc.negStatus == 2 && dc.state == ImsDriverCall.State.INCOMING)
                            ||(dc.negStatusPresent == 1 && dc.negStatus == 3)
                            ||(dc.negStatusPresent == 0 && csMode == 0));
            if(videoMode || csMode == 2 || csMode >= 7){
                dc.isVoice = false;
            } else {
                dc.isVoice = true;
            }
            int mpty = p.readInt();
            dc.mptyState = mpty;
            dc.isMpty = (0 != mpty);
            dc.numberType = p.readInt();
            dc.TOA = p.readInt();
            dc.number = p.readString();
            dc.prioritypresent = p.readInt();
            dc.priority = p.readInt();
            dc.cliValidityPresent = p.readInt();
            int np = p.readInt();
            dc.numberPresentation = ImsDriverCall.presentationFromCLIP(np);

            dc.als = p.readInt();
            dc.isVoicePrivacy = (0 != p.readInt());
            dc.name = p.readString();
            dc.namePresentation = p.readInt();
            int uusInfoPresent = p.readInt();
            if (uusInfoPresent == 1) {
                dc.uusInfo = new UUSInfo();
                dc.uusInfo.setType(p.readInt());
                dc.uusInfo.setDcs(p.readInt());
                byte[] userData = p.createByteArray();
                dc.uusInfo.setUserData(userData);
                riljLogv(String.format("Incoming UUS : type=%d, dcs=%d, length=%d",
                        dc.uusInfo.getType(), dc.uusInfo.getDcs(),
                        dc.uusInfo.getUserData().length));
                riljLogv("Incoming UUS : data (string)="
                        + new String(dc.uusInfo.getUserData()));
                riljLogv("Incoming UUS : data (hex): "
                        + IccUtils.bytesToHexString(dc.uusInfo.getUserData()));
            } else {
                riljLogv("Incoming UUS : NOT present!");
            }

            // Make sure there's a leading + on addresses with a TOA of 145
            dc.number = PhoneNumberUtils.stringFromStringAndTOA(dc.number, dc.TOA);

            response.add(dc);
            if (RILJ_LOGD) {
                riljLog("responseCallListEx: dc=" +dc.toString());
            }
        }

        Collections.sort(response);

        return response;
    }


    public void
    setImsVoiceCallAvailability(int state, Message response) {
        RILRequest rr
        = RILRequest.obtain(ImsRILConstants.RIL_REQUEST_SET_IMS_VOICE_CALL_AVAILABILITY, response);

        rr.mParcel.writeInt(1);
        rr.mParcel.writeInt(state);

        if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

        send(rr);
    }


    public void
    getImsVoiceCallAvailability(Message response){
        RILRequest rr = RILRequest.obtain(
                ImsRILConstants.RIL_REQUEST_GET_IMS_VOICE_CALL_AVAILABILITY, response);

        if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

        send(rr);
    }


    public void initISIM(String confUri, String instanceId, String impu,
            String impi, String domain, String xCap, String bspAddr,
            Message response) {
        RILRequest rr = RILRequest.obtain(ImsRILConstants.RIL_REQUEST_INIT_ISIM, response);
        if (RILJ_LOGD)
            riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));
        rr.mParcel.writeInt(7);
        rr.mParcel.writeString(confUri);
        rr.mParcel.writeString(instanceId);
        rr.mParcel.writeString(impu);
        rr.mParcel.writeString(impi);
        rr.mParcel.writeString(domain);
        rr.mParcel.writeString(xCap);
        rr.mParcel.writeString(bspAddr);
        send(rr);
    }
    public void disableIms(Message response){
        RILRequest rr = RILRequest.obtain(
                ImsRILConstants.RIL_REQUEST_DISABLE_IMS, response);
        if (RILJ_LOGD)
            riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));
        send(rr);

    }


    public void
    requestVolteCallMediaChange(boolean isVideo, Message response) {
        RILRequest rr
        = RILRequest.obtain(ImsRILConstants.RIL_REQUEST_IMS_CALL_REQUEST_MEDIA_CHANGE , null);

        rr.mParcel.writeInt(2);
        if(response != null){
            rr.mParcel.writeInt(response.arg1);
        }else{
            rr.mParcel.writeInt(1);
        }
        /* @} */
        rr.mParcel.writeInt(isVideo?1:0);

        if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

        send(rr);
    }


    public void
    responseVolteCallMediaChange(boolean isAccept, Message response) {
        RILRequest rr
        = RILRequest.obtain(ImsRILConstants.RIL_REQUEST_IMS_CALL_RESPONSE_MEDIA_CHANGE, null);

        rr.mParcel.writeInt(2);
        if(response != null){
            rr.mParcel.writeInt(response.arg1);
        }else{
            rr.mParcel.writeInt(1);
        }
        rr.mParcel.writeInt(isAccept?1:0);

        if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));

        send(rr);
    }


    public void setImsSmscAddress(String smsc, Message response){
        RILRequest rr = RILRequest.obtain(ImsRILConstants.RIL_REQUEST_SET_IMS_SMSC, response);
        rr.mParcel.writeString(smsc);
        if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest)
                + " : " + smsc);
        send(rr);
    }


    public void requestVolteCallFallBackToVoice(Message response) {
        RILRequest rr = RILRequest.obtain(ImsRILConstants.RIL_REQUEST_IMS_CALL_FALL_BACK_TO_VOICE, null);
        rr.mParcel.writeInt(1);
        if(response != null){
            rr.mParcel.writeInt(response.arg1);
        }else{
            rr.mParcel.writeInt(1);
        }
        if (RILJ_LOGD)
            riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));
        send(rr);
    }


    public void setInitialAttachIMSApn(String apn, String protocol, int authType, String username,
            String password, Message result) {
        RILRequest rr = RILRequest.obtain(ImsRILConstants.RIL_REQUEST_SET_IMS_INITIAL_ATTACH_APN, null);

        if (RILJ_LOGD) riljLog("Set RIL_REQUEST_SET_IMS_INITIAL_ATTACH_APN");

        rr.mParcel.writeString(apn);
        rr.mParcel.writeString(protocol);
        rr.mParcel.writeInt(authType);
        rr.mParcel.writeString(username);
        rr.mParcel.writeString(password);

        if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest)
                + ", apn:" + apn + ", protocol:" + protocol + ", authType:" + authType
                + ", username:" + username + ", password:" + password);
        send(rr);
    }


    public void
    requestInitialGroupCall(String numbers, Message response) {
        RILRequest rr = RILRequest.obtain(
                ImsRILConstants.RIL_REQUEST_IMS_INITIAL_GROUP_CALL, response);
        rr.mParcel.writeString(numbers);

        if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest)
                + " numbers:" + numbers);

        send(rr);
    }


    public void
    requestAddGroupCall(String numbers, Message response) {
        RILRequest rr = RILRequest.obtain(
                ImsRILConstants.RIL_REQUEST_IMS_ADD_TO_GROUP_CALL, response);
        rr.mParcel.writeString(numbers);

        if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest)
                + " numbers:" + numbers);

        send(rr);
    }

    public void
    setCallForward(int action, int cfReason, int serviceClass,
            String number, int timeSeconds, String ruleSet, Message response) {

        RILRequest rr
        = RILRequest.obtain(ImsRILConstants.RIL_REQUEST_SET_CALL_FORWARD_URI, response);

        rr.mParcel.writeInt(action); // 2 is for query action, not in used anyway
        rr.mParcel.writeInt(cfReason);
        //number type
        if(number != null && PhoneNumberUtils.isUriNumber(number)){
            rr.mParcel.writeInt(1);
        } else {
            rr.mParcel.writeInt(2);
        }
        rr.mParcel.writeInt(PhoneNumberUtils.toaFromString(number));
        rr.mParcel.writeString(number);
        rr.mParcel.writeInt(serviceClass);
        rr.mParcel.writeString(ruleSet);
        rr.mParcel.writeInt (timeSeconds);

        if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest)
                + " " + action + " " + cfReason + " " + serviceClass
                + timeSeconds);

        send(rr);
    }

    public void
    queryCallForwardStatus(int cfReason, int serviceClass,
            String number, String ruleSet, Message response) {
        RILRequest rr
        = RILRequest.obtain(ImsRILConstants.RIL_REQUEST_QUERY_CALL_FORWARD_STATUS_URI, response);

        rr.mParcel.writeInt(2); // 2 is for query action, not in used anyway
        rr.mParcel.writeInt(cfReason);
        //number type
        if(number != null && PhoneNumberUtils.isUriNumber(number)){
            rr.mParcel.writeInt(1);
        } else {
            rr.mParcel.writeInt(2);
        }
        rr.mParcel.writeInt(PhoneNumberUtils.toaFromString(number));
        rr.mParcel.writeString(number);
        rr.mParcel.writeInt(serviceClass);
        rr.mParcel.writeString(ruleSet);
        rr.mParcel.writeInt (0);

        if (RILJ_LOGD) riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest)
                + " " + cfReason + " " + serviceClass);

        send(rr);
    }

    private Object
    responseCallForwardUri(Parcel p) {
        int numInfos;
        ImsCallForwardInfoEx infos[];

        numInfos = p.readInt();

        infos = new ImsCallForwardInfoEx[numInfos];

        for (int i = 0 ; i < numInfos ; i++) {
            infos[i] = new ImsCallForwardInfoEx();
            infos[i].mStatus = p.readInt();
            infos[i].mCondition = p.readInt();
            infos[i].mNumberType = p.readInt();
            infos[i].mToA = p.readInt();
            infos[i].mNumber = p.readString();
            infos[i].mServiceClass = p.readInt();
            infos[i].mRuleset = p.readString();
            infos[i].mTimeSeconds = p.readInt();
            riljLog("responseCallForwardUri> "
                    + " infos[i].number:" + infos[i].mNumber);
        }


        return infos;
    }


    public void enableIms(Message response) {
        RILRequest rr = RILRequest.obtain(
                ImsRILConstants.RIL_REQUEST_ENABLE_IMS, response);
        if (RILJ_LOGD)
            riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));
        send(rr);
    }

    protected RegistrantList mImsCallStateRegistrants = new RegistrantList();

    public void registerForImsCallStateChanged(Handler h, int what, Object obj) {
        Registrant r = new Registrant (h, what, obj);

        mImsCallStateRegistrants.add(r);
    }

    public void unregisterForImsCallStateChanged(Handler h) {
        mImsCallStateRegistrants.remove(h);
    }

    public void registerForImsNetworkStateChanged(Handler h, int what, Object obj) {
        mCi.registerForImsNetworkStateChanged(h, what, obj);
    }

    public void unregisterForImsNetworkStateChanged(Handler h) {
        mCi.unregisterForImsNetworkStateChanged(h);
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
            case ImsRILConstants.RIL_REQUEST_VIDEOPHONE_DIAL: return "VIDEOPHONE_DIAL";
            default: return requestToString(request);
        }
    }

    protected Registrant mImsBearerStateRegistrant;
    public void registerForImsBearerStateChanged(Handler h, int what, Object obj) {
        mImsBearerStateRegistrant = new Registrant(h, what, obj);
    }

    public void unregisterForImsBearerStateChanged(Handler h) {
        mImsBearerStateRegistrant.clear();
    }

    protected Registrant mImsVideoQosRegistrant;
    public void registerForImsVideoQos(Handler h, int what, Object obj) {
        mImsVideoQosRegistrant = new Registrant(h, what, obj);
    }

    public void unregisterForImsVideoQos(Handler h) {
        mImsVideoQosRegistrant.clear();
    }

    public void getImsBearerState(Message response) {
        RILRequest rr = RILRequest.obtain(ImsRILConstants.RIL_REQUEST_GET_IMS_BEARER_STATE, response);
        if (RILJ_LOGD)
            riljLog(rr.serialString() + "> " + imsRequestToString(rr.mRequest));
        send(rr);
    }
    /* @} */

    public void setOnSuppServiceNotification(Handler h, int what, Object obj) {
        mCi.setOnSuppServiceNotification(h,what,obj);
    }

    public void unSetOnSuppServiceNotification(Handler h) {
        mCi.unSetOnSuppServiceNotification(h);
    }
}

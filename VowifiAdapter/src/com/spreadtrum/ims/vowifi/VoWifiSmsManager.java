package com.spreadtrum.ims.vowifi;

import android.content.Context;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.RemoteException;
import android.telephony.PhoneNumberUtils;
import android.telephony.SmsManager;
import android.telephony.ims.stub.ImsSmsImplBase;
import android.text.TextUtils;
import android.util.Log;

import com.android.ims.internal.IVoWifiSms;
import com.android.ims.internal.IVoWifiSmsCallback;
import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.gsm.SmsMessage;
import com.android.internal.telephony.uicc.IccUtils;

import com.spreadtrum.ims.vowifi.Utilities.JSONUtils;
import com.spreadtrum.ims.vowifi.Utilities.Result;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;

public class VoWifiSmsManager extends ServiceManager {
    public interface ISmsChangedListener {
        public void onChanged(IVoWifiSms newInterface);
    }

    private static final String TAG = Utilities.getTag(VoWifiSmsManager.class.getSimpleName());

    private Context mContext;
    private IVoWifiSms mISms;
    private ImsSmsImpl mSmsImpl;
    private MySmsCallback mSmsCallback = new MySmsCallback();
    private ArrayList<Sms> mSmsList= new ArrayList<Sms>();

    private static final int MSG_HANDLE_EVENT = 1;
    private class MyHandler extends Handler {
        public MyHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            if (msg.what == MSG_HANDLE_EVENT) {
                handleEvent((String) msg.obj);
            }
        }
    }

    protected VoWifiSmsManager(Context context) {
        this(context, Utilities.SERVICE_PACKAGE, Utilities.SERVICE_CLASS_SMS,
                Utilities.SERVICE_ACTION_SMS);
    }

    protected VoWifiSmsManager(Context context, String pkg, String cls, String action) {
        super(context, pkg, cls, action);

        mContext = context;
        mSmsImpl = new ImsSmsImpl(this);

        HandlerThread thread = new HandlerThread("SmsManager");
        thread.start();
        Looper looper = thread.getLooper();
        mHandler = new MyHandler(looper);
    }

    @Override
    protected void onServiceChanged() {
        try {
            mISms = null;
            if (mServiceBinder != null) {
                mISms = IVoWifiSms.Stub.asInterface(mServiceBinder);
                mISms.registerCallback(mSmsCallback);
            } else {
                clearPendingList();
            }
        } catch (RemoteException ex) {
            Log.e(TAG, "Can not register callback as catch the RemoteException. ex: " + ex);
        }
    }

    public ImsSmsImplBase getSmsImplementation() {
        return mSmsImpl;
    }

    private void handleEvent(String json) {
        try {
            JSONObject jObject = new JSONObject(json);
            String eventName = jObject.optString(JSONUtils.KEY_EVENT_NAME, "");
            Log.d(TAG, "Handle the event '" + eventName + "'.");

            int eventCode = jObject.optInt(JSONUtils.KEY_EVENT_CODE, -1);
            switch (eventCode) {
                case JSONUtils.EVENT_CODE_SMS_SEND_FINISHED: {
                    int token = jObject.optInt(JSONUtils.KEY_SMS_TOKEN);
                    int messageRef = jObject.optInt(JSONUtils.KEY_SMS_MESSAGE_REF);
                    boolean success = jObject.optBoolean(JSONUtils.KEY_SMS_RESULT);
                    int sendStatus = ImsSmsImplBase.SEND_STATUS_OK;
                    int errorCode = SmsManager.RESULT_ERROR_NONE;
                    if (success) {
                        Sms sms = findSmsByToken(token);
                        sms._status = Sms.STATUS_SEND_FINISHED;
                    } else {
                        sendStatus = ImsSmsImplBase.SEND_STATUS_ERROR_FALLBACK;
                        errorCode = jObject.optInt(JSONUtils.KEY_SMS_REASON);
                        // Handle as error and it will fallback to CS, remove from the map.
                        mSmsList.remove(new Sms(token));
                    }
                    mSmsImpl.onSendSmsResult(token, messageRef, sendStatus, errorCode);
                    break;
                }
                case JSONUtils.EVENT_CODE_SMS_RECEIVED: {
                    String pdu = jObject.optString(JSONUtils.KEY_SMS_PDU);
                    if (TextUtils.isEmpty(pdu)) {
                        Log.e(TAG, "Failed to notify sms received as pdu is empty.");
                        return;
                    }

                    int token = generateToken();
                    Sms sms = new Sms(token, -1, Sms.DIR_RECEIVED, Sms.STATUS_RECEIVED, -1, false);
                    mSmsList.add(sms);
                    mSmsImpl.onSmsReceived(token, mSmsImpl.getSmsFormat(),
                            IccUtils.hexStringToBytes(pdu));
                    break;
                }
                case JSONUtils.EVENT_CODE_SMS_STATUS_REPORT_RECEIVED: {
                    String pdu = jObject.optString(JSONUtils.KEY_SMS_PDU);
                    if (TextUtils.isEmpty(pdu)) {
                        Log.e(TAG, "Failed to notify sms received as pdu is empty.");
                        return;
                    }

                    byte[] pduByte = IccUtils.hexStringToBytes(pdu);
                    SmsMessage smsMsg = SmsMessage.newFromCDS(pduByte);
                    Sms sms = findSmsByMessageRef(smsMsg.mMessageRef);
                    int token = -1;
                    if (sms == null) {
                        Log.w(TAG, "Do not send the sms before, please check the messageRef: "
                                + smsMsg.mMessageRef);
                        token = generateToken();
                    } else {
                        token = sms._token;
                        sms._status = Sms.STATUS_REPORT_RECEIVED;
                    }
                    int messageRefFromJSON = jObject.optInt(JSONUtils.KEY_SMS_MESSAGE_REF);
                    Log.d(TAG, "saved msgRef: " + smsMsg.mMessageRef + ", messageRefFromJSON: "
                            + messageRefFromJSON);
                    mSmsImpl.onSmsStatusReportReceived(
                            token, smsMsg.mMessageRef, mSmsImpl.getSmsFormat(), pduByte);
                    break;
                }
            }
        } catch (JSONException e) {
            Log.e(TAG, "As catch the exception, failed to handle the event for the json: " + json);
        }
    }

    private int generateToken() {
        String tokenStr = String.valueOf(System.currentTimeMillis());
        return Integer.valueOf(tokenStr.substring(3));
    }

    private Sms findSmsByToken(int token) {
        return findSms(Sms.FLAG_TOKEN, token);
    }

    private Sms findSmsByMessageRef(int messageRef) {
        return findSms(Sms.FLAG_MSGREF, messageRef);
    }

    private Sms findSms(int flag, int value) {
        if (mSmsList == null || mSmsList.size() < 1) {
            // There isn't any sms, return null.
            return null;
        }

        for (Sms sms : mSmsList) {
            int compareValue = -1000;
            switch (flag) {
                case Sms.FLAG_TOKEN:
                    compareValue = sms._token;
                    break;
                case Sms.FLAG_MSGREF:
                    compareValue = sms._messageRef;
                    break;
            }
            if (compareValue == value) {
                return sms;
            }
        }

        // Do not find the matched sms, return null.
        return null;

    }

    private class ImsSmsImpl extends ImsSmsImplBase {

        private VoWifiSmsManager mSmsMgr;

        private boolean mReady = false;

        protected ImsSmsImpl(VoWifiSmsManager smsMgr) {
            mSmsMgr = smsMgr;
        }

        @Override
        public void acknowledgeSms(int token, int messageRef, int result) {
            if (Utilities.DEBUG) {
                Log.i(TAG, "Acknowledge the sms: token[" + token + "], messageRef[" + messageRef
                        + "], result[" + result + "]");
            }

            if (mISms == null) {
                // TODO: handle as send failed.
            }

            try {
                int id = mISms.acknowledgeSms(token, messageRef, getCauseFromResult(result));
                if (id != Result.INVALID_ID) {
                    mSmsList.remove(new Sms(token));
                } else {
                    // TODO: handle as ack sms failed.
                }
            } catch (RemoteException ex) {
                Log.e(TAG, "Failed to acknowledge the sms as catch the ex: " + ex);
                // TODO: handle as send failed.
            }
        }

        @Override
        public void acknowledgeSmsReport(int token, int messageRef, int result) {
            if (Utilities.DEBUG) {
                Log.i(TAG, "Acknowledge the sms report: token[" + token + "], messageRef["
                        + messageRef + "], result[" + result + "]");
            }

            if (mISms == null) {
                // TODO: handle as send failed.
            }

            try {
                int id = mISms.acknowledgeSmsReport(token, messageRef, getCauseFromResult(result));
                if (id != Result.INVALID_ID) {
                    mSmsList.remove(new Sms(token));
                } else {
                    // TODO: handle as ack report failed.
                }
            } catch (RemoteException ex) {
                Log.e(TAG, "Failed to acknowledge the sms report as catch the ex: " + ex);
            }
        }

        @Override
        public void onReady() {
            mReady = true;
        }

        @Override
        public void sendSms(int token, int messageRef, String format, String smsc, boolean retry,
                byte[] pdu) {
            if (Utilities.DEBUG) {
                Log.i(TAG, "Send the sms: token[" + token + "], messageRef[" + messageRef
                        + "], smsc[" + smsc + "], retry[" + retry + "], pdu[" + pdu + "]");
            }

            if (mISms == null) {
                // TODO: handle as send failed.
            }

            try {
                byte[] smscByte = IccUtils.hexStringToBytes(smsc);
                int length = smscByte[0];
                String numberSMSC = PhoneNumberUtils.calledPartyBCDToString(smscByte, 1, length);

                int id = mISms.sendSms(token, messageRef,
                        1 /* Retry will be handled by framework, native needn't retry. */,
                        numberSMSC, IccUtils.bytesToHexString(pdu));
                if (id == Result.INVALID_ID) {
                    // TODO: handle as send failed.
                } else {
                    boolean requireSR = (pdu[0] & Sms.INDICATOR_STATUS_REPORT_REQUEST) > 0;
                    Log.d(TAG, "Send the sms as require status report: " + requireSR);
                    Sms sms = new Sms(
                            token, messageRef, Sms.STATUS_SEND, Sms.DIR_SEND, id, requireSR);
                    mSmsList.add(sms);
                }
            } catch (RemoteException ex) {
                Log.e(TAG, "Failed to send the sms as catch the ex: " + ex);
                // TODO: handle as send failed.
            }
        }

        public void resetReadyState() {
            mReady = false;
        }

        private int getCauseFromResult(int result) {
            switch (result) {
                case ImsSmsImplBase.STATUS_REPORT_STATUS_OK:
                    // Cause code is ignored if ok.
                    return 0;
                case ImsSmsImplBase.DELIVER_STATUS_ERROR_NO_MEMORY:
                    return CommandsInterface.GSM_SMS_FAIL_CAUSE_MEMORY_CAPACITY_EXCEEDED;
                default:
                    return CommandsInterface.GSM_SMS_FAIL_CAUSE_UNSPECIFIED_ERROR;
            }
        }
    }

    private class Sms {
        // TP-Message-Type-Indicator: TP-Status-Report-Request bit.
        public static final int INDICATOR_STATUS_REPORT_REQUEST = 0x20;

        public static final int FLAG_TOKEN = 1;
        public static final int FLAG_MSGREF = 2;

        public static final int DIR_SEND = 1;
        public static final int DIR_RECEIVED = 2;

        public static final int STATUS_SEND = 1;
        public static final int STATUS_SEND_FINISHED = 2;
        public static final int STATUS_REPORT_RECEIVED = 3;

        public static final int STATUS_RECEIVED = -1;

        public int _token;
        public int _messageRef;
        public int _status;
        public int _dir;
        public int _id;
        public boolean _requireStatusReport;

        public Sms(int token) {
            _token = token;
        }

        public Sms(int token, int messageRef, int dir, int status, int id,
                boolean requireStatusReport) {
            _token = token;
            _messageRef = messageRef;
            _status = status;
            _dir = dir;
            _id = id;
            _requireStatusReport = requireStatusReport;
        }

        @Override
        public boolean equals(Object obj) {
            if (obj instanceof Sms) {
                Sms sms = (Sms) obj;
                if (sms == this) {
                    return true;
                } else {
                    return sms._token == this._token;
                }
            } else {
                return false;
            }
        }

        @Override
        public String toString() {
            return "Sms[token:" + _token + ", messageRef:" + _messageRef + ", dir:" + _dir
                    + ", status:" + _status + ", id:" + _id + ", _requireStatusReport:"
                    + _requireStatusReport + "]";
        }
    }

    private class MySmsCallback extends IVoWifiSmsCallback.Stub {
        @Override
        public void onEvent(String json) throws RemoteException {
            if (Utilities.DEBUG) Log.i(TAG, "Get the vowifi sms event callback: " + json);
            if (TextUtils.isEmpty(json)) {
                Log.e(TAG, "Can not handle the ser callback as the json is null.");
                return;
            }

            mHandler.sendMessage(mHandler.obtainMessage(MSG_HANDLE_EVENT, json));
        }
    }
}

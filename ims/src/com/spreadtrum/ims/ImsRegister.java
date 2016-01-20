package com.spreadtrum.ims;

import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.database.ContentObserver;
import android.os.Message;
import android.os.Handler;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.gsm.GSMPhone;
import com.android.internal.telephony.PhoneProxy;
import com.android.internal.telephony.CommandsInterface;
import android.telephony.TelephonyManager;
import com.android.internal.telephony.TelephonyIntents;
import com.android.internal.telephony.uicc.UiccController;
import com.android.internal.telephony.TeleUtils;
import com.android.internal.telephony.uicc.IccRecords;
import com.android.internal.telephony.uicc.IsimUiccRecords;
import android.os.AsyncResult;
import android.os.Looper;
import android.util.Log;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.IccCardConstants;
import android.telephony.SubscriptionManager;
import android.provider.Settings;
import android.telephony.VoLteServiceState;
import com.android.ims.ImsManager;

public class ImsRegister {
    private static final String TAG = "ImsRegister";
    private static final boolean DBG = true;

    private Context mContext;
    private CommandsInterface mCi;
    private GSMPhone mPhone;

    private boolean mInitISIM;
    private boolean mConnIMSEN;
    private TelephonyManager mTelephonyManager;
    private int mPhoneId;
    private BaseHandler mHandler;
    private ContentObserver mEnhancedLTEObserver;
    private boolean mCurrentImsRegistered;

    protected static final int EVENT_RADIO_STATE_CHANGED               = 105;
    protected static final int EVENT_INIT_ISIM_DONE                    = 106;
    protected static final int EVENT_CONN_IMS_ENABLE                   = 107;

    public ImsRegister(GSMPhone phone , Context context, CommandsInterface ci) {
        mPhone = phone;
        mContext = context;
        mCi = ci;
        mTelephonyManager = TelephonyManager.from(mContext);
        mPhoneId = mPhone.getPhoneId();
        mHandler = new BaseHandler(mContext.getMainLooper());
        mEnhancedLTEObserver = new ContentObserver(mHandler) {
            @Override
            public void onChange(boolean selfChange) {
                boolean isEnhanced4gLteMode = ImsManager.isEnhanced4gLteModeSettingEnabledByUser(mContext);
                if(mPhoneId != mTelephonyManager.getPrimaryCard()) return;
                if (DBG) Log.i(TAG,"phone" + mPhoneId + " Enhanced4gLteMode changed " + isEnhanced4gLteMode);
                if(!isEnhanced4gLteMode) {
                    mCi.disableIms(null);
                } else if(mInitISIM && mConnIMSEN) {
                    mCi.enableIms(null);
                }
            }
        };

        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(TelephonyIntents.ACTION_SIM_STATE_CHANGED);
        mContext.registerReceiver(mReceiver, intentFilter);
        mCi.registerForRadioStateChanged(mHandler, EVENT_RADIO_STATE_CHANGED, null);
        mCi.registerForConnImsen(mHandler, EVENT_CONN_IMS_ENABLE, null);
        mContext.getContentResolver().registerContentObserver(
                Settings.Global.getUriFor(android.provider.Settings.Global.ENHANCED_4G_MODE_ENABLED),
                true, mEnhancedLTEObserver);
    }

    private class BaseHandler extends Handler {
        BaseHandler(Looper looper) {
            super(looper);
        }
        @Override
        public void handleMessage(Message msg) {
            if (DBG) Log.i(TAG,"handleMessage msg=" + msg);
            AsyncResult ar = (AsyncResult) msg.obj;
            switch (msg.what) {
                case EVENT_RADIO_STATE_CHANGED:
                    if (mPhone.isRadioOn()) {
                        Log.i(TAG,"EVENT_RADIO_STATE_CHANGED -> radio is on");
                        initISIM();
                    } else {
                        mInitISIM = false;
                        mConnIMSEN = false;
                        mCurrentImsRegistered = false;
                        boolean needNotifyRegisterState = false;
                        if(mPhone.getPhoneId() == mTelephonyManager.getPrimaryCard()){
                            Log.i(TAG, "radio not on, notify ImsRegestered state mCurrentImsRegistered = " + mCurrentImsRegistered);
                            needNotifyRegisterState = true;
                        } else {
                            int primaryCard = mTelephonyManager.getPrimaryCard();
                            if (SubscriptionManager.isValidPhoneId(primaryCard)) {
                                boolean isStandby = Settings.Global.getInt(
                                        mPhone.getContext().getContentResolver(),
                                        Settings.Global.SIM_STANDBY + primaryCard, 1) == 1;
                                if(!isStandby) {
                                    Log.i(TAG, "primary card standby is false, ImsRegestered state mCurrentImsRegistered = " + mCurrentImsRegistered);
                                    needNotifyRegisterState = true;
                                }
                            }
                        }
                        if(needNotifyRegisterState) {
                            sendVolteServiceStateChanged();
                        }
                    }
                    break;
                case EVENT_INIT_ISIM_DONE:
                    Log.i(TAG,"EVENT_INIT_ISIM_DONE");
                    ar = (AsyncResult) msg.obj;
                    int[] initResult = (int[]) ar.result;
                    if(ar.exception != null) {
                        Log.i(TAG, "EVENT_INIT_ISIM_DONE ar.exception");
                        break;
                    }
                    mInitISIM = true;
                    if(initResult[0] == 1) mConnIMSEN = true;
                    if(mConnIMSEN && ImsManager.isEnhanced4gLteModeSettingEnabledByUser(mContext)) {
                        mCi.enableIms(null);
                    }
                    break;
                case EVENT_CONN_IMS_ENABLE:
                    ar = (AsyncResult) msg.obj;
                    if(ar.exception != null) break;
                    int[] conn = (int[]) ar.result;
                    Log.i(TAG, "EVENT_CONN_IMSEN : conn = "+conn[0]);
                    if (conn[0] == 1) {
                        mConnIMSEN = true;
                        if (mInitISIM && ImsManager.isEnhanced4gLteModeSettingEnabledByUser(mContext)) {
                            mCi.enableIms(null);
                        }
                    }
                    break;
                default:
                    break;
            }
        }
    };
    private BroadcastReceiver mReceiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            int phoneId = intent.getIntExtra(PhoneConstants.PHONE_KEY,
                    SubscriptionManager.INVALID_SUBSCRIPTION_ID);

            if (action.equals(TelephonyIntents.ACTION_SIM_STATE_CHANGED)) {
                String state = intent.getStringExtra(IccCardConstants.INTENT_KEY_ICC_STATE);
                Log.d(TAG, "action = " + action + ", state = " + state + ", phoneId = " + phoneId);
                if (IccCardConstants.INTENT_VALUE_ICC_LOADED.equals(state)) {
                    if (mPhone.isRadioOn()) {
                        Log.i(TAG,"sim loaded and radio is on");
                        initISIM();
                    }
                }
            }
        }
    };

    private void initISIM() {
        if (!mInitISIM
                && mPhoneId == mTelephonyManager.getPrimaryCard()
                && mTelephonyManager.getSimState(mPhoneId) == TelephonyManager.SIM_STATE_READY) {
            String impi = null;
            String impu = null;
            String domain = null;
            String xCap = null;
            String bspAddr = null;
            String instanceId = null;
            String conferenceUri = null;
            UiccController uc = UiccController.getInstance();
            IccRecords iccRecords = null;
            String operatorNumberic = null;
            if (uc != null) {
                iccRecords = uc.getIccRecords(mPhoneId,
                        UiccController.APP_FAM_3GPP);
                if (iccRecords != null
                        && iccRecords.getOperatorNumeric() != null
                        && iccRecords.getOperatorNumeric().length() > 0) {
                    operatorNumberic = iccRecords.getOperatorNumeric();
                    String mnc = operatorNumberic.substring(3);
                    if (mnc != null && mnc.length() == 2) {
                        mnc = "0" + mnc;
                    }
                    domain = "ims.mnc" + mnc + ".mcc"
                            + operatorNumberic.substring(0, 3)
                            + ".3gppnetwork.org";
                    impi = iccRecords.getIMSI() + "@" + domain;
                    impu = "sip:" + impi;
                    Log.i(TAG, "iccRecords.getServiceProviderName() = "
                            + iccRecords.getServiceProviderName());
                    String operatorName = TeleUtils.updateOperator(
                            operatorNumberic.substring(0, 3)
                                    + Integer.parseInt(mnc),
                            "numeric_to_operator");
                    Log.i(TAG, "operatorName after update is " + operatorName);
                    if (!(iccRecords instanceof IsimUiccRecords)
                            && ("China Mobile".equals(operatorName)
                                    || "China Mobile".equals(iccRecords
                                            .getServiceProviderName()) || "CMCC"
                                        .equals(iccRecords
                                                .getServiceProviderName()))) {
                        xCap = "xcap." + "ims.mnc000" + ".mcc"
                                + operatorNumberic.substring(0, 3)
                                + ".pub.3gppnetwork.org";
                    } else {
                        xCap = "xcap." + "ims.mnc" + mnc + ".mcc"
                                + operatorNumberic.substring(0, 3)
                                + ".pub.3gppnetwork.org";
                    }
                    bspAddr = "bsf." + "mnc" + mnc + ".mcc"
                            + operatorNumberic.substring(0, 3)
                            + ".pub.3gppnetwork.org";
                    Log.i(TAG, "impu = " + impu);
                    Log.i(TAG, "impi= " + impi);
                    Log.i(TAG, "domain= " + domain);
                    Log.i(TAG, "xCap = " + xCap);
                    Log.i(TAG, "bspAddr = " + bspAddr);
                    String imei = mPhone.getDeviceId();
                    if (imei != null) {
                        instanceId = "urn:gsma:imei:" + imei.substring(0, 8)
                                + "-" + imei.substring(8, 14) + "-"
                                + imei.substring(14);
                    }
                    Log.i(TAG, "instanceId = " + instanceId);
                    conferenceUri = "sip:mmtel@conf-factory.ims.mnc" + mnc + ".mcc"
                            + operatorNumberic.substring(0, 3)
                            + ".3gppnetwork.org";
                    Log.i(TAG, "conferenceUri = " + conferenceUri);
                }
            }
            mCi.initISIM(conferenceUri, instanceId, impu, impi, domain, xCap,
                    bspAddr, mHandler.obtainMessage(EVENT_INIT_ISIM_DONE));
        }
    }

    public void notifyImsStateChanged(boolean imsRegistered) {
        if( mCurrentImsRegistered != imsRegistered) {
            mCurrentImsRegistered = imsRegistered;
            if( mPhone.getPhoneId() == mTelephonyManager.getPrimaryCard()) {
                sendVolteServiceStateChanged();
              }
        }
    }

    private void sendVolteServiceStateChanged() {
        mPhone.notifyVoLteServiceStateChanged(new VoLteServiceState(mCurrentImsRegistered ? VoLteServiceState.IMS_REG_STATE_REGISTERED : VoLteServiceState.IMS_REG_STATE_NOT_EGISTERED));
    }
}

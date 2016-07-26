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
import com.android.internal.telephony.PhoneFactory;
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
import com.sprd.android.internal.telephony.VolteConfig;
import android.text.TextUtils;
import java.util.HashMap;
import java.util.Map;
import android.telephony.ServiceState;

public class ImsRegister {
    private static final String TAG = "ImsRegister";
    private static final boolean DBG = true;

    private Context mContext;
    private CommandsInterface mCi;
    private GSMPhone mPhone;

    private boolean mInitISIMDone;
    private boolean mIMSBearerEstablished;
    private TelephonyManager mTelephonyManager;
    private int mPhoneId;
    private BaseHandler mHandler;
    private boolean mCurrentImsRegistered;
    private VolteConfig mVolteConfig;
    private String mNumeric;
    private String mLastNumeric="";

    protected static final int EVENT_RADIO_STATE_CHANGED               = 105;
    protected static final int EVENT_INIT_ISIM_DONE                    = 106;
    protected static final int EVENT_IMS_BEARER_ESTABLISTED            = 107;
    protected static final int EVENT_ENABLE_IMS                        = 108;

    public ImsRegister(GSMPhone phone , Context context, CommandsInterface ci) {
        mPhone = phone;
        mContext = context;
        mCi = ci;
        mTelephonyManager = TelephonyManager.from(mContext);
        mPhoneId = mPhone.getPhoneId();
        mVolteConfig = VolteConfig.getInstance();
        mHandler = new BaseHandler(mContext.getMainLooper());
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(TelephonyIntents.ACTION_SIM_STATE_CHANGED);
        mContext.registerReceiver(mReceiver, intentFilter);
        mCi.registerForRadioStateChanged(mHandler, EVENT_RADIO_STATE_CHANGED, null);
        mCi.registerForConnImsen(mHandler, EVENT_IMS_BEARER_ESTABLISTED, null);
        mCi.getImsBearerState(mHandler.obtainMessage(EVENT_IMS_BEARER_ESTABLISTED));
    }

    private class BaseHandler extends Handler {
        BaseHandler(Looper looper) {
            super(looper);
        }
        @Override
        public void handleMessage(Message msg) {
            log("handleMessage msg=" + msg);
            AsyncResult ar = (AsyncResult) msg.obj;
            switch (msg.what) {
                case EVENT_RADIO_STATE_CHANGED:
                    if (mPhone.isRadioOn()) {
                        log("EVENT_RADIO_STATE_CHANGED -> radio is on");
                        initISIM();
                    } else {
                        mInitISIMDone = false;
                        mIMSBearerEstablished = false;
                        mLastNumeric="";
                        mCurrentImsRegistered = false;
                        boolean needNotifyRegisterState = false;
                        if(mPhone.getPhoneId() == mTelephonyManager.getPrimaryCard()){
                            log("radio not on, notify ImsRegestered state mCurrentImsRegistered = " + mCurrentImsRegistered);
                            needNotifyRegisterState = true;
                        } else {
                            int primaryCard = mTelephonyManager.getPrimaryCard();
                            PhoneProxy phone = (PhoneProxy)PhoneFactory.getPhone(primaryCard);
                            if(SubscriptionManager.isValidPhoneId(primaryCard) && !((GSMPhone)phone.getActivePhone()).isRadioOn()){
                                needNotifyRegisterState = true;
                                log("primary card radio not on, notify ImsRegestered state mCurrentImsRegistered = " + mCurrentImsRegistered);
                            }
                        }
                        if(needNotifyRegisterState) {
                            sendVolteServiceStateChanged();
                        }
                    }
                    break;
                case EVENT_INIT_ISIM_DONE:
                    log("EVENT_INIT_ISIM_DONE");
                    ar = (AsyncResult) msg.obj;
                    int[] initResult = (int[]) ar.result;
                    if(ar.exception != null) {
                        log("EVENT_INIT_ISIM_DONE ar.exception");
                        break;
                    }
                    mInitISIMDone = true;
                    enableIms();
                    break;
                case EVENT_IMS_BEARER_ESTABLISTED:
                    ar = (AsyncResult) msg.obj;
                    if(ar.exception != null) break;
                    int[] conn = (int[]) ar.result;
                    log("EVENT_CONN_IMSEN : conn = "+conn[0]);
                    if (conn[0] == 1) {
                        mIMSBearerEstablished = true;
                        mLastNumeric = "";
                        enableIms();
                    }
                    break;
                case EVENT_ENABLE_IMS:
                    mNumeric = mTelephonyManager.getNetworkOperatorForPhone(mPhoneId);
                    mVolteConfig.loadVolteConfig(mContext);
                    boolean isSimConfig = getSimConfig();
                    if(!(mLastNumeric.equals(mNumeric))) {
                        if(isSimConfig && getNetworkConfig(mNumeric) && !(getNetworkConfig(mLastNumeric))){
                              mCi.enableIms(null);
                        } else if(isSimConfig && getNetworkConfig(mLastNumeric) && !(getNetworkConfig(mNumeric))){
                              mCi.disableIms(null);
                        }
                        mLastNumeric = mNumeric;
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
                log("action = " + action + ", state = " + state + ", phoneId = " + phoneId);
                if (IccCardConstants.INTENT_VALUE_ICC_LOADED.equals(state)) {
                    if (mPhone.isRadioOn()) {
                        log("sim loaded and radio is on");
                        initISIM();
                    }
                }
            }
        }
    };

    private void initISIM() {
        if (!mInitISIMDone
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
                    /* SPRD: modify for CONFERENCEURI/NAF AND BSF @{*/
                    mVolteConfig.loadVolteConfig(mContext);
                    Map<String, String> data = new HashMap<String, String>();
                    if(mVolteConfig.containsCarrier(operatorNumberic)){
                        data = mVolteConfig.getVolteConfig(operatorNumberic);
                    }
                    domain = getImsConfigUri(VolteConfig.KEY_DOMAIN, data, operatorNumberic, mnc);
                    xCap = getImsConfigUri(VolteConfig.KEY_XCAP, data, operatorNumberic, mnc);
                    bspAddr = getImsConfigUri(VolteConfig.KEY_BSF, data, operatorNumberic, mnc);
                    conferenceUri = getImsConfigUri(VolteConfig.KEY_CONFURI, data, operatorNumberic, mnc);
                    /* @}*/
                    impi = iccRecords.getIMSI() + "@" + domain;
                    impu = "sip:" + impi;
                    log("impu = " + impu);
                    log("impi= " + impi);
                    log("domain= " + domain);
                    log("xCap = " + xCap);
                    log("bspAddr = " + bspAddr);
                    log("conferenceUri = " + conferenceUri);
                    String imei = mPhone.getDeviceId();
                    if (imei != null) {
                        instanceId = "urn:gsma:imei:" + imei.substring(0, 8)
                                + "-" + imei.substring(8, 14) + "-"
                                + imei.substring(14);
                    }
                    log("instanceId = " + instanceId);
                }
            }
            mCi.initISIM(conferenceUri, instanceId, impu, impi, domain, xCap,
                    bspAddr, mHandler.obtainMessage(EVENT_INIT_ISIM_DONE));
        }
    }
    /* SPRD:Modify for bug576993 @{ */
    public synchronized void notifyImsStateChanged(boolean imsRegistered) {
        boolean isPrimaryCard = mPhone.getPhoneId() == mTelephonyManager.getPrimaryCard();
        Log.i(TAG, "notifyImsStateChanged mCurrentImsRegistered:" + mCurrentImsRegistered
                 + " imsRegistered:" + imsRegistered + " isPrimaryCard:" + isPrimaryCard);
        if(mCurrentImsRegistered != imsRegistered) {
            mCurrentImsRegistered = imsRegistered;
            if(isPrimaryCard) {
                sendVolteServiceStateChanged();
                if (mPhone.isRadioOn() && getServiceState().getState() != ServiceState.STATE_IN_SERVICE) {
                    log("voice regstate not in service, will call ImsNotifier to notifyServiceStateChanged");
                    mPhone.notifyServiceStateChangedForIms(getServiceState());
                }
            }
        }
    }
    /* @} */

    private void sendVolteServiceStateChanged() {
        mPhone.notifyVoLteServiceStateChanged(new VoLteServiceState(mCurrentImsRegistered ? VoLteServiceState.IMS_REG_STATE_REGISTERED : VoLteServiceState.IMS_REG_STATE_NOT_EGISTERED));
    }

    public void enableIms() {
        if(mIMSBearerEstablished && mInitISIMDone) {
        mHandler.sendMessage(mHandler.obtainMessage(EVENT_ENABLE_IMS));
        }
    }

    private void log(String s) {
        if (DBG) {
            Log.d(TAG, "[ImsRegister" + mPhoneId + "] " + s);
        }
    }

    /* SPRD: add for SIM Config and Network Config for VOLTE @{ */
    private boolean getNetworkConfig(String numeric){
        if(mVolteConfig.containsCarrier(numeric)){
            return mVolteConfig.getVolteEnable(numeric);
        }
        return false;
    }

    private boolean getSimConfig(){
        String numeric = mTelephonyManager.getSimOperatorNumericForPhone(mPhoneId);
        log("getSimConfig() numeric = " + numeric);
        if(mVolteConfig.containsCarrier(numeric)){
            return mVolteConfig.getVolteEnable(numeric);
        }
        return false;
    }
    /* @} */
    /* SPRD: modify for CONFERENCEURI/NAF AND BSF @{*/
    private String getImsConfigUri(String key, Map<String, String> data, String operatorNumberic, String mnc){
        String imsConfigUri = null;

        if("domain".equals(key)){
            imsConfigUri = (data == null || TextUtils.isEmpty(data.get(key)) ? "ims.mnc" + mnc + ".mcc"
                    + operatorNumberic.substring(0, 3) + ".3gppnetwork.org" : data.get(key));
        } else if("xcap".equals(key)){
            imsConfigUri = (data == null || TextUtils.isEmpty(data.get(key)) ? "xcap." + "ims.mnc" + mnc + ".mcc"
                    + operatorNumberic.substring(0, 3) + ".pub.3gppnetwork.org" : data.get(key));
        } else if("bsf".equals(key)){
            imsConfigUri = (data == null || TextUtils.isEmpty(data.get(key)) ? "bsf." + "mnc" + mnc + ".mcc"
                    + operatorNumberic.substring(0, 3) + ".pub.3gppnetwork.org" : data.get(key));
        } else if("confuri".equals(key)){
            imsConfigUri = (data == null || TextUtils.isEmpty(data.get(key)) ? "sip:mmtel@conf-factory.ims.mnc" + mnc + ".mcc"
                    + operatorNumberic.substring(0, 3) + ".3gppnetwork.org" : data.get(key));
        }
        return imsConfigUri;
    }
    /* @} */

    private ServiceState getServiceState() {
        if (mPhone.getServiceStateTracker() != null) {
            return mPhone.getServiceStateTracker().mSS;
        } else {
            return new ServiceState();
        }
    }
}

package com.spreadtrum.ims;

import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.os.Message;
import android.os.Handler;
import android.os.SystemProperties;

import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneFactory;
import com.android.internal.telephony.GsmCdmaPhone;
import android.telephony.TelephonyManager;
import com.android.internal.telephony.TelephonyIntents;
import com.android.internal.telephony.uicc.UiccCardApplication;
import com.android.internal.telephony.uicc.UiccController;
import com.android.internal.telephony.uicc.IccRecords;
import com.android.internal.telephony.uicc.IsimUiccRecords;
import android.os.AsyncResult;
import android.os.Looper;
import android.util.Log;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.IccCardConstants;
import com.android.internal.telephony.CommandException;
import android.telephony.SubscriptionManager;
import android.provider.Settings;
import android.content.res.Resources;
import android.text.TextUtils;
import com.android.internal.telephony.VolteConfig;
import java.util.HashMap;
import java.util.Map;
import android.telephony.ServiceState;

public class ImsRegister {
    private static final String TAG = "ImsRegister";
    private static final boolean DBG = true;
    private static final String MODEM_WORKMODE_PROP = "persist.radio.modem.workmode";

    private Context mContext;
    private ImsRIL mCi;
    private GsmCdmaPhone mPhone;
    private int mPhoneCount;

    private boolean mInitISIMDone;
    private boolean mIMSBearerEstablished;
    private boolean mSIMLoaded;
    private TelephonyManager mTelephonyManager;
    private int mPhoneId;
    private BaseHandler mHandler;
    private boolean mCurrentImsRegistered;
    private UiccController mUiccController;
    private IccRecords mIccRecords = null;
    private UiccCardApplication mUiccApplcation = null;
    private VolteConfig mVolteConfig;
    private String mNumeric;
    private String mLastNumeric="";
    private static final int DEFAULT_PHONE_ID   = 0;
    private static final int SLOTTWO_PHONE_ID   = 1;

    private static final int EVENT_ICC_CHANGED                       = 201;
    private static final int EVENT_RECORDS_LOADED                    = 202;
    private static final int EVENT_RADIO_STATE_CHANGED               = 203;
    private static final int EVENT_INIT_ISIM_DONE                    = 204;
    private static final int EVENT_IMS_BEARER_ESTABLISTED            = 205;
    private static final int EVENT_ENABLE_IMS                        = 206;

    public ImsRegister(GsmCdmaPhone phone , Context context, ImsRIL ci) {
        mPhone = phone;
        mContext = context;
        mCi = ci;
        mTelephonyManager = TelephonyManager.from(mContext);
        mPhoneId = mPhone.getPhoneId();
        mPhoneCount = mTelephonyManager.getPhoneCount();
        mHandler = new BaseHandler(mContext.getMainLooper());

        mUiccController = UiccController.getInstance();
        mVolteConfig = VolteConfig.getInstance();
        mUiccController.registerForIccChanged(mHandler, EVENT_ICC_CHANGED, null);
        mCi.registerForRadioStateChanged(mHandler, EVENT_RADIO_STATE_CHANGED, null);
        mCi.registerForImsBearerStateChanged(mHandler, EVENT_IMS_BEARER_ESTABLISTED, null);
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
            case EVENT_ICC_CHANGED:
                onUpdateIccAvailability();
                break;
            case EVENT_RECORDS_LOADED:
                mSIMLoaded = true;
                initISIM();
                break;
            case EVENT_RADIO_STATE_CHANGED:
                if (!mPhone.isRadioOn()) {
                    mInitISIMDone = false;
                    mIMSBearerEstablished = false;
                    mLastNumeric="";
                    mCurrentImsRegistered = false;
                } else {
                    log("EVENT_RADIO_STATE_CHANGED -> radio is on");
                    initISIM();
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
                if(ar.exception != null) {
                    CommandException.Error err=null;
                    if (ar.exception instanceof CommandException) {
                        err = ((CommandException)(ar.exception)).getCommandError();
                    }
                    if (err == CommandException.Error.RADIO_NOT_AVAILABLE) {
                        mCi.getImsBearerState(mHandler.obtainMessage(EVENT_IMS_BEARER_ESTABLISTED));
                    }
                    break;
                }
                int[] conn = (int[]) ar.result;
                log("EVENT_IMS_BEARER_ESTABLISTED : conn = "+conn[0]);
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

    private void initISIM() {
        if (mSIMLoaded && mPhone.isRadioOn() && !mInitISIMDone
                && mTelephonyManager.getSimState(mPhoneId) == TelephonyManager.SIM_STATE_READY && mPhoneId == getPrimaryCard()) {
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

                    mVolteConfig.loadVolteConfig(mContext);
                    Map<String, String> data = new HashMap<String, String>();
                    if(mVolteConfig.containsCarrier(operatorNumberic)){
                        data = mVolteConfig.getVolteConfig(operatorNumberic);
                    }
                    domain = getImsConfigUri(VolteConfig.KEY_DOMAIN, data, operatorNumberic, mnc);
                    xCap = getImsConfigUri(VolteConfig.KEY_XCAP, data, operatorNumberic, mnc);
                    bspAddr = getImsConfigUri(VolteConfig.KEY_BSF, data, operatorNumberic, mnc);
                    conferenceUri = getImsConfigUri(VolteConfig.KEY_CONFURI, data, operatorNumberic, mnc);
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
                } else {
                    log("sim"+ mPhoneId +" may has not loaded!");
                    return;
                }
            }
            mCi.initISIM(conferenceUri, instanceId, impu, impi, domain, xCap,
                    bspAddr, mHandler.obtainMessage(EVENT_INIT_ISIM_DONE));
        }
    }

    public void notifyImsStateChanged(boolean imsRegistered) {
        if( mCurrentImsRegistered != imsRegistered) {
            mCurrentImsRegistered = imsRegistered;
            if( mPhoneId == getPrimaryCard()) {
                if (mPhone.isRadioOn() && getServiceState().getState() != ServiceState.STATE_IN_SERVICE) {
                    log("voice regstate not in service, will call ImsNotifier to notifyServiceStateChanged");
                    mPhone.notifyServiceStateChanged(getServiceState());
                }
            }
        }
    }

    private int getPrimaryCard() {
        if (mPhoneCount == 1) {
            return DEFAULT_PHONE_ID;
        }

        String prop = SystemProperties.get(MODEM_WORKMODE_PROP);
        if ((prop != null) && (prop.length() > 0)) {
            String values[] = prop.split(",");
            int[] workMode = new int[mPhoneCount];
            for(int i = 0; i < mPhoneCount; i++) {
                workMode[i] = Integer.parseInt(values[i]);
            }
            return getPrimaryCardFromProp(workMode);
        }
        return DEFAULT_PHONE_ID;
    }

    private int getPrimaryCardFromProp(int[] workMode) {
        switch (workMode[DEFAULT_PHONE_ID]) {
        case 10:
            if(workMode[SLOTTWO_PHONE_ID] != 10 && workMode[SLOTTWO_PHONE_ID] != 254) {
                return SLOTTWO_PHONE_ID;
            }
            break;
        case 254:
            if(workMode[SLOTTWO_PHONE_ID] != 254) {
                return SLOTTWO_PHONE_ID;
            }
            break;
        }
        return DEFAULT_PHONE_ID;
    }

    private void onUpdateIccAvailability() {
        if (mUiccController == null ) {
            return;
        }

        UiccCardApplication newUiccApplication = getUiccCardApplication();

        if (mUiccApplcation != newUiccApplication) {
            if (mUiccApplcation != null) {
                log("Removing stale icc objects.");
                if (mIccRecords != null) {
                    mIccRecords.unregisterForRecordsLoaded(mHandler);
                }
                mIccRecords = null;
                mUiccApplcation = null;
                mInitISIMDone = false;
                mSIMLoaded    = false;
            }
            if (newUiccApplication != null) {
                log("New card found");
                mUiccApplcation = newUiccApplication;
                mIccRecords = mUiccApplcation.getIccRecords();
                if (mIccRecords != null) {
                    mIccRecords.registerForRecordsLoaded(mHandler, EVENT_RECORDS_LOADED, null);
                }
            }
        }
    }

    private UiccCardApplication getUiccCardApplication() {
        return mUiccController.getUiccCardApplication(mPhoneId,
                UiccController.APP_FAM_3GPP);
    }

    private void log(String s) {
        if (DBG) {
            Log.d(TAG, "[ImsRegister" + mPhoneId + "] " + s);
        }
    }

    public void enableIms() {
        if(mIMSBearerEstablished && mInitISIMDone) {
            mHandler.sendMessage(mHandler.obtainMessage(EVENT_ENABLE_IMS));
        }
    }

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

    private ServiceState getServiceState() {
        if (mPhone.getServiceStateTracker() != null) {
            return mPhone.getServiceStateTracker().mSS;
        } else {
            return new ServiceState();
        }
    }
}

package com.spreadtrum.ims;

import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.database.ContentObserver;
import android.os.Message;
import android.os.Handler;
import android.os.SystemProperties;
import android.sim.SimManager;
import com.android.internal.telephony.CommandsInterface.RadioState;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.gsm.GSMPhone;
import com.android.internal.telephony.PhoneFactory;
import com.android.internal.telephony.PhoneProxy;
import com.android.internal.telephony.CommandException;
import com.android.internal.telephony.CommandsInterface;
import android.telephony.TelephonyManager;
import com.android.internal.telephony.TelephonyIntents;
import com.android.internal.telephony.uicc.UiccCardApplication;
import com.android.internal.telephony.uicc.UiccController;
import com.sprd.internal.telephony.uicc.MsUiccController;
//import com.android.internal.telephony.TeleUtils;
import com.android.internal.telephony.uicc.IccRecords;
import com.android.internal.telephony.uicc.IsimUiccRecords;
import com.android.internal.telephony.uicc.IccRefreshResponse;

import android.os.AsyncResult;
import android.os.Looper;
import android.util.Log;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.IccCardConstants;
//import android.telephony.SubscriptionManager;
import android.provider.Settings;
import android.telephony.VoLteServiceState;
import com.android.ims.ImsManager;
import com.sprd.internal.telephony.VolteConfig;
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
    private UiccController mUiccController;
    private IccRecords mIccRecords = null;
    private UiccCardApplication mUiccApplcation = null;
    private VolteConfig mVolteConfig;
    private String mNumeric;
    private String mLastNumeric="";
    private boolean mSIMLoaded;
    private int mRetryCount = 0;
    private ImsService mImsService;

    protected static final int EVENT_ICC_CHANGED                       = 103;
    protected static final int EVENT_RECORDS_LOADED                    = 104;
    protected static final int EVENT_RADIO_STATE_CHANGED               = 105;
    protected static final int EVENT_INIT_ISIM_DONE                    = 106;
    protected static final int EVENT_IMS_BEARER_ESTABLISTED            = 107;
    protected static final int EVENT_ENABLE_IMS                        = 108;
    protected static final int EVENT_SIM_REFRESH                       = 109;

    private static String PROP_TEST_MODE = "persist.radio.ssda.testmode";

    public ImsRegister(GSMPhone phone , Context context, CommandsInterface ci) {
        mPhone = phone;
        mContext = context;
        mImsService = (ImsService)context;
        mCi = ci;
        //mTelephonyManager = TelephonyManager.from(mContext);
        mPhoneId = mPhone.getPhoneId();
        mTelephonyManager = (TelephonyManager)mContext.getSystemService(TelephonyManager.getServiceName(Context.TELEPHONY_SERVICE, mPhoneId));
        mVolteConfig = VolteConfig.getInstance();
        mHandler = new BaseHandler(mContext.getMainLooper());
        /*IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(TelephonyIntents.ACTION_SIM_STATE_CHANGED);
        mContext.registerReceiver(mReceiver, intentFilter);*/
        mUiccController = MsUiccController.getInstance(mPhoneId);
        mUiccController.registerForIccChanged(mHandler, EVENT_ICC_CHANGED, null);
        mCi.registerForRadioStateChanged(mHandler, EVENT_RADIO_STATE_CHANGED, null);
        mCi.registerForConnImsen(mHandler, EVENT_IMS_BEARER_ESTABLISTED, null);
        mCi.getImsBearerState(mHandler.obtainMessage(EVENT_IMS_BEARER_ESTABLISTED));
        mCi.registerForIccRefresh(mHandler, EVENT_SIM_REFRESH, null);
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
                    log("EVENT_RECORDS_LOADED");
                    mSIMLoaded = true;
                    initISIM();
                    break;
                case EVENT_RADIO_STATE_CHANGED:
                    if (mCi.getRadioState() == CommandsInterface.RadioState.RADIO_UNAVAILABLE
                    || mCi.getRadioState() == CommandsInterface.RadioState.RADIO_OFF) {
                        mInitISIMDone = false;
                        mIMSBearerEstablished = false;
                        mLastNumeric = "";
                        mCurrentImsRegistered = false;
                    } else {
                        log("VOLTE EVENT_RADIO_STATE_CHANGED -> radio is on");
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
                    if(mImsService.allowEnableIms()){
                        enableIms();
                    }
                    break;
                case EVENT_IMS_BEARER_ESTABLISTED:
                    ar = (AsyncResult) msg.obj;
                    if(ar.exception != null) {
                        CommandException.Error err=null;
                        if (ar.exception instanceof CommandException) {
                            err = ((CommandException)(ar.exception)).getCommandError();
                        }
                        if (err == CommandException.Error.RADIO_NOT_AVAILABLE) {
                            if (mRetryCount < 8) {
                                mCi.getImsBearerState(mHandler.obtainMessage(EVENT_IMS_BEARER_ESTABLISTED));
                                mRetryCount++;
                            }
                        }
                        break;
                    }
                    int[] conn = (int[]) ar.result;
                    log("EVENT_CONN_IMSEN : conn = "+conn[0]);
                    if (conn[0] == 1) {
                        mIMSBearerEstablished = true;
                        mLastNumeric = "";
                    }
                    break;
                case EVENT_ENABLE_IMS:
                    mNumeric = mTelephonyManager.getNetworkOperator();
                    mVolteConfig.loadVolteConfig(mContext);
                    boolean isSimConfig = getSimConfig();
                    log("EVENT_ENABLE_IMS : mNumeric = "+ mNumeric + "  mLastNumeric = " + mLastNumeric);
                    if(!(mLastNumeric.equals(mNumeric))) {
                        if(isSimConfig && getNetworkConfig(mNumeric) && !(getNetworkConfig(mLastNumeric)) && mImsService.allowEnableIms()){
                              mCi.enableIms(null);
                        } else if(isSimConfig && getNetworkConfig(mLastNumeric) && !(getNetworkConfig(mNumeric))){
                              mCi.disableIms(null);
                        }
                        mLastNumeric = mNumeric;
                    }
                    break;
                case EVENT_SIM_REFRESH:
                    log("EVENT_SIM_REFRESH");
                    ar = (AsyncResult)msg.obj;
                    if (ar != null && ar.exception == null) {
                        IccRefreshResponse resp = (IccRefreshResponse)ar.result;
                        if(resp != null && (resp.refreshResult == IccRefreshResponse.REFRESH_RESULT_INIT
                                || resp.refreshResult == IccRefreshResponse.REFRESH_RESULT_RESET)){//uicc init
                            log("Uicc initialized, need to init ISIM again.");
                            mInitISIMDone = false;
                            mLastNumeric = "";
                        }
                    } else {
                        log("Sim REFRESH with exception: " + ar.exception);
                    }
                    break;
                default:
                    break;
            }
        }
    };
    /*private BroadcastReceiver mReceiver = new BroadcastReceiver() {
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
    };*/

    private void initISIM() {
        if (!mInitISIMDone
                && mPhoneId == getPrimaryCard()
                && mTelephonyManager.getSimState() == TelephonyManager.SIM_STATE_READY
                && mSIMLoaded
                && !(mCi.getRadioState() == CommandsInterface.RadioState.RADIO_UNAVAILABLE
                        || mCi.getRadioState() == CommandsInterface.RadioState.RADIO_OFF)) {
            String impi = null;
            String impu = null;
            String domain = null;
            String xCap = null;
            String bspAddr = null;
            String instanceId = null;
            String conferenceUri = null;
            UiccController uc = MsUiccController.getInstance(mPhoneId);
            IccRecords iccRecords = null;
            String operatorNumberic = null;
            if (uc != null) {
                iccRecords = uc.getIccRecords(UiccController.APP_FAM_3GPP);
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
                } else {
                    log("sim"+ mPhoneId +" may has not loaded!");
                    return;
                }
            }
            mCi.initISIM(conferenceUri, instanceId, impu, impi, domain, xCap,
                    bspAddr, mHandler.obtainMessage(EVENT_INIT_ISIM_DONE));
        }
    }
    /* SPRD:Modify for bug576993 @{ */
    public synchronized void notifyImsStateChanged(boolean imsRegistered, boolean isImsFeatureChanged) {
        boolean isPrimaryCard = mPhone.getPhoneId() == mTelephonyManager.getPrimaryCard();
        Log.i(TAG, "notifyImsStateChanged mCurrentImsRegistered:" + mCurrentImsRegistered
                 + " imsRegistered:" + imsRegistered + " isPrimaryCard:" + isPrimaryCard
                 + " isImsFeatureChanged:" + isImsFeatureChanged);
        if(mCurrentImsRegistered != imsRegistered || isImsFeatureChanged) {
            mCurrentImsRegistered = imsRegistered;
            if(isPrimaryCard) {
                sendVolteServiceStateChanged();
                if (!(mCi.getRadioState() == CommandsInterface.RadioState.RADIO_UNAVAILABLE
                        || mCi.getRadioState() == CommandsInterface.RadioState.RADIO_OFF)
                        && getServiceState().getState() != ServiceState.STATE_IN_SERVICE) {
                    log("voice regstate not in service, will call ImsNotifier to notifyServiceStateChanged");
                    mPhone.notifyServiceStateChangedForIms(getServiceState());
                }
            }
        }
    }
    /* @} */

    private void sendVolteServiceStateChanged() {
        VoLteServiceState volteSS = new VoLteServiceState();
        volteSS.setImsState(mCurrentImsRegistered ? 1 : 0);
        mPhone.notifyVoLteServiceStateChanged(volteSS);
    }

    public void enableIms() {
        Log.i(TAG, "enableIms ->mIMSBearerEstablished:" + mIMSBearerEstablished + " mInitISIMDone:" + mInitISIMDone);
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
        String numeric = mTelephonyManager.getSimOperator();
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

    /* SPRD: Bug 570819 Sometimes radio state is on and AT+SPTESTMODEM is correct sent, but 'service_primary_card' value in database may be -1.
     * In this sisuation, we should get the primarycard by the prop. @{*/
    private int getPrimaryCard(){
        int primaryCard = mTelephonyManager.getPrimaryCard();
        if(SimManager.isValidPhoneId(primaryCard)){
            return primaryCard;
        }

        String radioTestMode;
        int[] testMode = new int[2];
        for(int i = 0; i < mTelephonyManager.getPhoneCount(); i++){
            radioTestMode = i == 0 ? PROP_TEST_MODE: PROP_TEST_MODE + i;
            testMode[i] = getTestMode(radioTestMode);
        }

        if(testMode[0] == 10){
            if(testMode[1] != 10 && testMode[1] != 254){
                return 1;
            }
        } else if(testMode[0] == 254){
            if(testMode[1] != 254){
                return 0;
            }
        }
        return 0;
    }

    private int getTestMode(String radioTestMode){
        return SystemProperties.getInt(radioTestMode, -1);
    }
    /* @} */

    private ServiceState getServiceState() {
        if (mPhone.getServiceStateTracker() != null) {
            return mPhone.getServiceStateTracker().mSS;
        } else {
            return new ServiceState();
        }
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
        return mUiccController.getUiccCardApplication(UiccController.APP_FAM_3GPP);
    }

    public void onImsPDNReady(){
        mIMSBearerEstablished = true;
    }
}

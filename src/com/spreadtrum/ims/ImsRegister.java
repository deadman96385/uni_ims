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
import android.telephony.RadioAccessFamily;
import android.telephony.TelephonyManager;
import com.android.internal.telephony.TelephonyIntents;
import com.android.internal.telephony.uicc.UiccCardApplication;
import com.android.internal.telephony.uicc.UiccController;
import com.android.internal.telephony.uicc.IccRecords;
import com.android.internal.telephony.uicc.IccRefreshResponse;
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
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import android.os.Environment;
import android.util.Xml;
import com.android.internal.util.XmlUtils;
import android.telephony.ServiceState;
import com.android.ims.internal.ImsManagerEx;

public class ImsRegister {
    private static final String TAG = "ImsRegister";
    private static final boolean DBG = true;

    private Context mContext;
    private ImsRIL mCi;
    private GsmCdmaPhone mPhone;
    private int mPhoneCount;
    private ImsService mImsService;

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
    private int mRetryCount = 0;
    private static final int DEFAULT_PHONE_ID   = 0;
    private static final int SLOTTWO_PHONE_ID   = 1;

    private HashMap<String, String> mCarrierSpnMap;
    static final String PARTNER_SPN_OVERRIDE_PATH ="etc/spn-conf.xml";
    static final String OEM_SPN_OVERRIDE_PATH = "telephony/spn-conf.xml";


    private static final int EVENT_ICC_CHANGED                       = 201;
    private static final int EVENT_RECORDS_LOADED                    = 202;
    private static final int EVENT_RADIO_STATE_CHANGED               = 203;
    private static final int EVENT_INIT_ISIM_DONE                    = 204;
    private static final int EVENT_IMS_BEARER_ESTABLISTED            = 205;
    private static final int EVENT_ENABLE_IMS                        = 206;
    private static final int EVENT_RADIO_CAPABILITY_CHANGED          = 207;
    //SPRD: Add for Bug 634502
    private static final int EVENT_SIM_REFRESH                       = 208;

    public ImsRegister(GsmCdmaPhone phone , Context context, ImsRIL ci) {
        mPhone = phone;
        mContext = context;
        mImsService = (ImsService)context;
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
        mCi.registerForIccRefresh(mHandler, EVENT_SIM_REFRESH, null);
        mPhone.registerForRadioCapabilityChanged(mHandler, EVENT_RADIO_CAPABILITY_CHANGED, null);
        if(ImsManagerEx.isDualVoLTEActive()){
            mCarrierSpnMap = new HashMap<String, String>();
            loadSpnOverrides();
        }
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
                if (!mPhone.isRadioOn()) {
                    mInitISIMDone = false;
                    //add for L+G dual volte, if secondary card no need to reset mIMSBearerEstablished
                    // add for Dual LTE
                    if (getLTECapabilityForPhone()) {
                        mIMSBearerEstablished = false;
                    }

                    mLastNumeric="";
                    mCurrentImsRegistered = false;
                } else {
                    log("EVENT_RADIO_STATE_CHANGED -> radio is on");
                    initISIM();
                    SetUserAgent();//SPRD:add for user agent future 670075
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
                // add for Dual LTE
                if (mImsService.allowEnableIms(mPhoneId)
                        || (ImsManagerEx.isDualVoLTEActive()
                               && !getLTECapabilityForPhone())) {
                    enableIms();
                }
                break;
            case EVENT_IMS_BEARER_ESTABLISTED:
                ar = (AsyncResult) msg.obj;
                if(ar.exception != null || ar.result == null) {
                    log("EVENT_IMS_BEARER_ESTABLISTED : ar.exception = "+ar.exception);
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
                /**
                 * 772714 should adapter int[] status.
                 * when use mCi.getImsBearerState, it will return int[] but not Integer
                 */
                try {
                    Integer conn = new Integer(-1);
                    if (ar.result instanceof Integer) {
                        conn = (Integer) ar.result;
                    } else {
                        int[] connArray = (int[]) ar.result;
                        conn = connArray[0];
                    }
                    log("EVENT_IMS_BEARER_ESTABLISTED : conn = " + conn);
                    if (conn.intValue() == 1) {
                        mIMSBearerEstablished = true;
                        mLastNumeric = "";
                        if (ImsManagerEx.isDualVoLTEActive() && mPhoneId != getPrimaryCard()) {
                            enableIms();
                        }
                    } else // add for L+G dual volte, if secondary card no need
                           // to reset mIMSBearerEstablished
                    // add for Dual LTE
                    if (getLTECapabilityForPhone() && ImsManagerEx.isDualVoLTEActive()) {
                        mIMSBearerEstablished = false;
                    } else if (conn.intValue() == 0) {
                        //
                        log("EVENT_IMS_BEARER_ESTABLISTED : conn.intValue() == 0: ");
                        mLastNumeric = "";
                    }
                } catch (Exception e) {
                    log("EVENT_IMS_BEARER_ESTABLISTED : exception: "
                            + e.getMessage());
                    e.printStackTrace();
                }
               break;
            case EVENT_ENABLE_IMS:
                log("EVENT_ENABLE_IMS");
                mNumeric = mTelephonyManager.getNetworkOperatorForPhone(mPhoneId);
                // add for 796527
                mVolteConfig.loadVolteConfig(mContext);
                log("current mNumeric = "+mNumeric);
                // add for Dual LTE
                if (getLTECapabilityForPhone()) {
                    log("PrimaryCard : mLastNumeric = "+mLastNumeric);
                    if(!(mLastNumeric.equals(mNumeric))) {
                        if(mImsService.allowEnableIms(mPhoneId)){
                              SystemProperties.set("gsm.ims.enable" + mPhoneId, "1");
                              mCi.enableIms(null);
                              mLastNumeric = mNumeric;
                        }
                    }
                }else if(dualVoLTEActive()){
                    //need disable ims when primary card not register to whilte list network?
                    log("Secondary Card, mLastNumeric = " + mLastNumeric);
                    if(!(mLastNumeric.equals(mNumeric))){
                        mLastNumeric = mNumeric;
                    }
                    if(dualVoLTEActive()){
                        SystemProperties.set("gsm.ims.enable" + mPhoneId, "1");
                        mCi.enableIms(null);
                    }
                }
                break;
            case EVENT_RADIO_CAPABILITY_CHANGED:
                // add for Dual LTE
                if (!getLTECapabilityForPhone()) {
                    //SPRD: Bug 671074 If dual volte active, need to reset some variables.
                    if(!ImsManagerEx.isDualVoLTEActive()){
                        mInitISIMDone = false;
                    }
                    mIMSBearerEstablished = false;
                    mLastNumeric = "";
                    mCurrentImsRegistered = false;
                } else {
                    log("EVENT_RADIO_CAPABILITY_CHANGED -> initisim");
                    initISIM();
                }
                break;
                /* SPRD: Add for Bug 634502 Need to init ISIM after uicc has been initialized @{ */
            case EVENT_SIM_REFRESH:
                log("EVENT_SIM_REFRESH");
                ar = (AsyncResult)msg.obj;
                if (ar != null && ar.exception == null) {
                    IccRefreshResponse resp = (IccRefreshResponse)ar.result;
                    if(resp!= null && resp.refreshResult == IccRefreshResponse.REFRESH_RESULT_INIT){//uicc init
                        log("Uicc initialized, need to init ISIM again.");
                        mInitISIMDone = false;
                        mLastNumeric="";
                    }
                } else {
                    log("Sim REFRESH with exception: " + ar.exception);
                }
                break;
                /* @} */
            default:
                break;
            }
        }
    };

    private void initISIM() {
        log("nitISIM() : mSIMLoaded = " + mSIMLoaded
                + " | mPhone.isRadioOn() = " + mPhone.isRadioOn()
                + " | mTelephonyManager.getSimState(mPhoneId) = "
                + mTelephonyManager.getSimState(mPhoneId) + " | mPhoneId = "
                + mPhoneId
                + " | getLTECapabilityForPhone() = "
                + getLTECapabilityForPhone());
        if (mSIMLoaded && mPhone.isRadioOn() && !mInitISIMDone
                && mTelephonyManager.getSimState(mPhoneId) == TelephonyManager.SIM_STATE_READY
                     // add for Dual LTE
                && (getLTECapabilityForPhone() || ImsManagerEx.isDualVoLTEActive())) {
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
        log("--notifyImsStateChanged : imsRegistered = " + imsRegistered + " | mCurrentImsRegistered = " + mCurrentImsRegistered);
        if( mCurrentImsRegistered != imsRegistered) {
            mCurrentImsRegistered = imsRegistered;
            /**
             * SPRD bug644157 should limit action to primary card
             * so remove if(){}
             */
//            if( mPhoneId == getPrimaryCard()) {
            if (mPhone.isRadioOn()
                    && getServiceState().getState() != ServiceState.STATE_IN_SERVICE) {
                log("voice regstate not in service, will call ImsNotifier to notifyServiceStateChanged");
                mPhone.notifyServiceStateChanged(getServiceState());
            }
//            }
        }
    }

    private int getPrimaryCard() {
        log("-getPrimaryCard() mPhoneCount = " + mPhoneCount);
        if (mPhoneCount == 1) {
            return DEFAULT_PHONE_ID;
        }
        int primaryCard = SubscriptionManager
                .getSlotIndex(SubscriptionManager.getDefaultDataSubscriptionId());
        if (primaryCard == -1) {
            return DEFAULT_PHONE_ID;
        }
        return primaryCard;
    }

    public static int getPrimaryCard(int phoneCount) {
        // SPRD add for dual LTE
        Log.d(TAG, "-getPrimaryCard(int phoneCount) phoneCount = " + phoneCount);
        if (phoneCount == 1) {
            return DEFAULT_PHONE_ID;
        }
        int primaryCard = SubscriptionManager
                .getSlotIndex(SubscriptionManager.getDefaultDataSubscriptionId());
        if (primaryCard == -1) {
            return DEFAULT_PHONE_ID;
        }
        return primaryCard;
    }

    private static int getPrimaryCardFromProp(int[] workMode) {
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
        // L+W mode SRPD: 675103 721982
        case 255:
            Log.d(TAG,"-getPrimaryCardFromProp() workMode[2] = " + workMode[SLOTTWO_PHONE_ID]);
            if (workMode[SLOTTWO_PHONE_ID] == 9
                || workMode[SLOTTWO_PHONE_ID] == 6
                || workMode[SLOTTWO_PHONE_ID] == 7
                || workMode[SLOTTWO_PHONE_ID] == 20) {

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
        log("enableIms ->mIMSBearerEstablished:" + mIMSBearerEstablished + " mInitISIMDone:" + mInitISIMDone);
        if(!ImsConfigImpl.isVolteEnabledBySystemProperties()){
            log("enableIms ->ImsConfigImpl.isVolteEnabledBySystemProperties():" + ImsConfigImpl.isVolteEnabledBySystemProperties());
            return;
        }
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

    public void onImsPDNReady(){
        mIMSBearerEstablished = true;
    }

    private boolean dualVoLTEActive() {
        if (!ImsManagerEx.isDualVoLTEActive()) {
            log("not support Dual volte");
            return false;
        }
        // SPRD: Add for DSDA Dual VoLTE switch
        if(mCarrierSpnMap == null) {
            mCarrierSpnMap = new HashMap<String, String>();
            loadSpnOverrides();
        }
        int primaryCard = getPrimaryCard();
        String primaryOperator = null;
        String secondOperator = null;
        IccRecords priIccRecords = mUiccController.getIccRecords(primaryCard,
                UiccController.APP_FAM_3GPP);
        IccRecords secIccRecords = mUiccController.getIccRecords(mPhoneId,
                UiccController.APP_FAM_3GPP);
        if (priIccRecords != null
                && priIccRecords.getOperatorNumeric() != null
                && priIccRecords.getOperatorNumeric().length() > 0) {
            if (mCarrierSpnMap.containsKey(priIccRecords.getOperatorNumeric())) {
                primaryOperator = mCarrierSpnMap.get(priIccRecords.getOperatorNumeric());
            }
        }
        if (secIccRecords != null
                && secIccRecords.getOperatorNumeric() != null
                && secIccRecords.getOperatorNumeric().length() > 0) {
            if (mCarrierSpnMap.containsKey(secIccRecords.getOperatorNumeric())) {
                secondOperator = mCarrierSpnMap.get(secIccRecords.getOperatorNumeric());
            }
        }
        log("primaryOperator = " + primaryOperator);
        log("secondOperator = " + secondOperator);
        boolean ignoreWhiteList = SystemProperties.getBoolean(
                "persist.radio.dsda.wl.ignore", false);
        log("ignoreWhiteList = " + ignoreWhiteList);
        boolean sameOperator = false;
        if (ignoreWhiteList) {
            sameOperator = secondOperator != null
                    && primaryOperator != null
                    && secondOperator.length() > 0
                    && primaryOperator.length() > 0
                    && (secondOperator.equals(primaryOperator) || isRelianceCard(primaryOperator)
                            && isRelianceCard(secondOperator));
        } else {
            sameOperator = isCmccCard(primaryOperator)
                    && primaryOperator.equals(secondOperator);
        }

       try{
        if (sameOperator) {
            log("same operator, check mcc");
            primaryOperator = priIccRecords != null
                    && priIccRecords.getOperatorNumeric() != null
                    && priIccRecords.getOperatorNumeric().length() > 3 ?priIccRecords.getOperatorNumeric().substring(0, 3): null;
            secondOperator = secIccRecords != null
                    && secIccRecords.getOperatorNumeric() != null
                    && secIccRecords.getOperatorNumeric().length() > 3 ?secIccRecords.getOperatorNumeric().substring(0, 3): null;
            sameOperator = secondOperator != null && primaryOperator != null
                    && secondOperator.equals(primaryOperator);
        }
        }catch(StringIndexOutOfBoundsException e){
            e.printStackTrace();
            return false;
        }
        log("sameOperator = " + sameOperator);
        return sameOperator;
    }

    private boolean isRelianceCard(String operatorName){
        if(operatorName != null &&(operatorName.equalsIgnoreCase("Reliance")
                || operatorName.equalsIgnoreCase("Jio"))){
            return true;
        }
        return false;
    }

    private boolean isCmccCard(String operatorName){
        if(operatorName != null &&operatorName.equalsIgnoreCase("China Mobile")){
            return true;
        }
        return false;
    }

    private void loadSpnOverrides() {
        FileReader spnReader;

        File spnFile = new File(Environment.getRootDirectory(),
                PARTNER_SPN_OVERRIDE_PATH);
        File oemSpnFile = new File(Environment.getOemDirectory(),
                OEM_SPN_OVERRIDE_PATH);

        if (oemSpnFile.exists()) {
            // OEM image exist SPN xml, get the timestamp from OEM & System image for comparison.
            long oemSpnTime = oemSpnFile.lastModified();
            long sysSpnTime = spnFile.lastModified();
            log("SPN Timestamp: oemTime = " + oemSpnTime + " sysTime = " + sysSpnTime);

            // To get the newer version of SPN from OEM image
            if (oemSpnTime > sysSpnTime) {
                log("SPN in OEM image is newer than System image");
                spnFile = oemSpnFile;
            }
        } else {
            // No SPN in OEM image, so load it from system image.
            log("No SPN in OEM image = " + oemSpnFile.getPath() +
                " Load SPN from system image");
        }

        try {
            spnReader = new FileReader(spnFile);
        } catch (FileNotFoundException e) {
            log("Can not open " + spnFile.getAbsolutePath());
            return;
        }

        try {
            XmlPullParser parser = Xml.newPullParser();
            parser.setInput(spnReader);

            XmlUtils.beginDocument(parser, "spnOverrides");

            while (true) {
                XmlUtils.nextElement(parser);

                String name = parser.getName();
                if (!"spnOverride".equals(name)) {
                    break;
                }

                String numeric = parser.getAttributeValue(null, "numeric");
                String data    = parser.getAttributeValue(null, "spn");

                mCarrierSpnMap.put(numeric, data);
            }
            spnReader.close();
        } catch (XmlPullParserException e) {
            log("Exception in spn-conf parser " + e);
        } catch (IOException e) {
            log("Exception in spn-conf parser " + e);
        }
    }
    /**
     * SPRD:add for user agent future
     * userAgent: deviceName_SW version
     ***/
    private void SetUserAgent() {
        String userAgent = SystemProperties.get("ro.config.useragent", "SPRD VOLTE");
        if ("SPRD VOLTE".equals(userAgent)) {
            return;
        }
        String[] cmd = new String[1];
        cmd[0] = "AT+SPENGMDVOLTE=22,1," + "\"" + userAgent + "\"";
        log("SetUserAgent :" + cmd[0]);
    }/* @} */

    public boolean getLTECapabilityForPhone(){
        int rafMax = mPhone.getRadioAccessFamily();
        return (rafMax & RadioAccessFamily.RAF_LTE) == RadioAccessFamily.RAF_LTE;
    }
}

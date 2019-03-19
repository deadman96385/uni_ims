package com.spreadtrum.ims;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicReference;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.database.ContentObserver;
import android.database.Cursor;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.PersistableBundle;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.AsyncResult;
import android.telephony.CarrierConfigManager;
import android.telephony.CarrierConfigManagerEx;
import android.telephony.ServiceState;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.telephony.VoLteServiceState;
import android.telephony.ims.feature.ImsFeature;
import android.telephony.ims.feature.MmTelFeature;
import android.util.Log;
import android.widget.Toast;

import android.provider.Telephony;
import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.PhoneFactory;
import com.android.internal.telephony.dataconnection.ApnSetting;
import com.android.internal.telephony.GsmCdmaPhone;

import com.android.ims.ImsException;
import com.android.ims.ImsManager;
import com.android.ims.ImsServiceClass;
import com.android.ims.ImsConfig;
import android.telephony.ims.ImsReasonInfo;

import com.android.sprd.telephony.RadioInteractor;
import com.spreadtrum.ims.ut.ImsUtImpl;
import com.spreadtrum.ims.ut.ImsUtProxy;

import com.android.ims.internal.IImsCallSession;
import android.telephony.ims.aidl.IImsCallSessionListener;
import com.android.ims.internal.IImsRegistrationListener;
import com.android.ims.internal.IImsPdnStateListener;
import com.android.ims.internal.IImsEcbm;
import com.android.ims.internal.IImsService;
import com.android.ims.internal.IImsUt;
import android.telephony.ims.aidl.IImsConfig;
import com.android.ims.internal.ImsManagerEx;
import com.android.internal.telephony.uicc.UiccController;
import com.android.internal.telephony.uicc.IccRecords;
import com.android.internal.telephony.DctConstants;
import com.android.internal.util.ArrayUtils;
import com.spreadtrum.ims.ImsService;
import com.spreadtrum.ims.data.ApnUtils;
import com.android.internal.telephony.VolteConfig;
import android.text.TextUtils;
import com.spreadtrum.ims.vowifi.VoWifiConfiguration;
import com.spreadtrum.ims.vowifi.VoWifiServiceImpl;
import com.spreadtrum.ims.vowifi.VoWifiServiceImpl.CallRatState;

import android.telephony.ims.ImsCallSession;
import android.telephony.ims.aidl.IImsRegistration;
import android.telephony.ims.aidl.IImsRegistrationCallback;
import android.telephony.ims.stub.ImsSmsImplBase;
import android.telephony.ims.stub.ImsRegistrationImplBase;
import android.telephony.ims.ImsCallProfile;
import android.telephony.ims.feature.CapabilityChangeRequest;

import android.telephony.ims.aidl.IImsMmTelFeature;
import com.android.ims.internal.IImsFeatureStatusCallback;
import com.android.ims.internal.IImsMultiEndpoint;
import com.android.ims.internal.IImsServiceListenerEx;
import vendor.sprd.hardware.radio.V1_0.ImsNetworkInfo;
import vendor.sprd.hardware.radio.V1_0.ImsErrorCauseInfo;
import com.android.sprd.telephony.CommandException;
import android.content.ComponentName;
import android.content.ServiceConnection;
import android.telephony.data.DataProfile;

public class ImsServiceImpl extends MmTelFeature {
    private static final String TAG = ImsServiceImpl.class.getSimpleName();
    private static final boolean DBG = true;
    private static final int IMS_CALLING_RTP_TIME_OUT        = 1;
    private static final int IMS_INVALID_VOLTE_SETTING       =-1;  // UNISOC: Add for bug968317

    public static final int IMS_REG_STATE_INACTIVE            = 0;
    public static final int IMS_REG_STATE_REGISTERED          = 1;
    public static final int IMS_REG_STATE_REGISTERING         = 2;
    public static final int IMS_REG_STATE_REG_FAIL            = 3;
    public static final int IMS_REG_STATE_UNKNOWN             = 4;
    public static final int IMS_REG_STATE_ROAMING             = 5;
    public static final int IMS_REG_STATE_DEREGISTERING       = 6;

    protected static final int EVENT_CHANGE_IMS_STATE                  = 101;
    protected static final int EVENT_IMS_STATE_CHANGED                 = 102;
    protected static final int EVENT_IMS_STATE_DONE                    = 103;
    protected static final int EVENT_IMS_CAPABILITY_CHANGED            = 104;
    protected static final int EVENT_SRVCC_STATE_CHANGED               = 105;
    protected static final int EVENT_SERVICE_STATE_CHANGED             = 106;
    protected static final int EVENT_RADIO_STATE_CHANGED               = 107;
    //add for bug612670
    protected static final int EVENT_SET_VOICE_CALL_AVAILABILITY_DONE  = 108;

    protected static final int EVENT_IMS_REGISTER_ADDRESS_CHANGED      = 109;
    protected static final int EVENT_IMS_HANDOVER_STATE_CHANGED        = 110;
    protected static final int EVENT_IMS_HANDOVER_ACTION_COMPLETE      = 111;
    protected static final int EVENT_IMS_PND_STATE_CHANGED             = 112;
    protected static final int EVENT_IMS_NETWORK_INFO_UPDATE           = 113;
    protected static final int EVENT_IMS_WIFI_PARAM                    = 114;
    protected static final int EVENT_IMS_GET_SRVCC_CAPBILITY           = 115;
    protected static final int EVENT_IMS_GET_PCSCF_ADDRESS             = 116;
    protected static final int EVENT_IMS_GET_IMS_REG_ADDRESS           = 117;
    protected static final int EVENT_RADIO_AVAILABLE                   = 118;
    protected static final int EVENT_RADIO_ON                          = 119;
    protected static final int EVENT_IMS_GET_IMS_CNI_INFO              = 122;

    protected static final int EVENT_IMS_ERROR_CAUSE_INFO                 = 123;


    /* UNISOC: add for bug968317 @{ */
    class VoLTECallAvailSyncStatus {
        public static final int VOLTE_CALL_AVAIL_SYNC_IDLE     = 0;
        public static final int VOLTE_CALL_AVAIL_SYNC_ONGOING  = 1;
        public static final int VOLTE_CALL_AVAIL_SYNC_FAIL     = 2;
    }
    /*@}*/

    public static final int IMS_ERROR_CAUSE_TYPE_IMSREG = 2;
    public static final int IMS_ERROR_CAUSE_ERRCODE_REG_FORBIDDED = 403;

    private GsmCdmaPhone mPhone;
    private ImsServiceState mImsServiceState;
    private int mServiceClass = ImsServiceClass.MMTEL;
    private int mServiceId;
    private PendingIntent mIncomingCallIntent;
    private IImsRegistrationListener mListener;
    private IImsFeatureStatusCallback mImsFeatureStatusCallback;
    private ConcurrentHashMap<IBinder, IImsRegistrationListener> mImsRegisterListeners = new ConcurrentHashMap<IBinder, IImsRegistrationListener>();
    private ConcurrentHashMap<IBinder, IImsPdnStateListener> mImsPdnStateListeners = new ConcurrentHashMap<IBinder, IImsPdnStateListener>();
    private Context mContext;
    private ImsRIL mCi;
    private ImsConfigImpl mImsConfigImpl;
    private com.spreadtrum.ims.ut.ImsUtImpl mImsUtImpl;
    private ImsEcbmImpl mImsEcbmImpl;
    private ImsServiceCallTracker mImsServiceCallTracker;
    private ImsHandler mHandler;
    private UiccController mUiccController;
    private ArrayList<ApnSetting> mAllApnSettings = null;
    private AtomicReference<IccRecords> mIccRecords = new AtomicReference<IccRecords>();
    private ApnChangeObserver mApnChangeObserver = null;
    private ImsRegister mImsRegister = null;
    private ArrayList<Listener> mListeners = new ArrayList<Listener>();
    private ImsService mImsService;
    private VolteConfig mVolteConfig;
    private boolean mSetSosApn = true;
    private VoWifiServiceImpl mWifiService;//Add for data router
    private ImsUtProxy mImsUtProxy = null;
    private String mImsRegAddress = "";
    private String mImsPscfAddress = "";
    private int mAliveCallLose = -1;
    private int mAliveCallJitter = -1;
    private int mAliveCallRtt = -1;
    private int mSrvccCapbility = -1;
    // SPRD: 730973
    private boolean mVolteRegisterStateOld = false;
    private RadioInteractor mRadioInteractor;

    private int mServiceState = ServiceState.STATE_POWER_OFF;
    public boolean mIsUtDisableByNetWork = false;

    /**
     * AndroidP start@{:
     */
    private ConcurrentHashMap<IBinder, IImsRegistrationCallback> mIImsRegistrationCallbacks = new ConcurrentHashMap<IBinder, IImsRegistrationCallback>();
    private MmTelCapabilities mVolteCapabilities = new MmTelCapabilities();
    private MmTelCapabilities mVowifiCapabilities = new MmTelCapabilities();
    //add for unisoc 911545
    private MmTelCapabilities mDeviceVolteCapabilities = new MmTelCapabilities();
    private MmTelCapabilities mDeviceVowifiCapabilities = new MmTelCapabilities();
    private int mCurrentImsFeature = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;  // UNISOC: Add for bug950573
    /* UNISOC: add for bug968317 @{ */
    private int VoLTECallAvailSync = VoLTECallAvailSyncStatus.VOLTE_CALL_AVAIL_SYNC_IDLE;
    private int currentVoLTESetting= IMS_INVALID_VOLTE_SETTING;
    private int pendingVoLTESetting= IMS_INVALID_VOLTE_SETTING;
    /*@}*/
    /* UNISOC: add for bug988585 @{ */
    private boolean mDeviceCapabilitiesFirstChange = true;
    private boolean mVideoCapabilityEnabled        = false;
    /*@}*/

    public IImsRegistration getRegistration(){
        return mImsRegistration;
    }

    private final IImsRegistration.Stub mImsRegistration = new IImsRegistration.Stub(){
        @Override
        public int getRegistrationTechnology(){
            if(isVoLteEnabled()){
                return ImsRegistrationImplBase.REGISTRATION_TECH_LTE;
            } else if(isVoWifiEnabled()){
                return ImsRegistrationImplBase.REGISTRATION_TECH_IWLAN;
            } else {
                return ImsRegistrationImplBase.REGISTRATION_TECH_NONE;
            }
        }

        @Override
        public void addRegistrationCallback(IImsRegistrationCallback c){
            synchronized (mIImsRegistrationCallbacks) {
                if (!mIImsRegistrationCallbacks.keySet().contains(c.asBinder())) {
                    mIImsRegistrationCallbacks.put(c.asBinder(), c);
                } else {
                    log("addRegistrationCallback Listener already add :" + c);
                }
            }
        }

        @Override
        public void removeRegistrationCallback(IImsRegistrationCallback c){
            synchronized (mIImsRegistrationCallbacks) {
                if (mIImsRegistrationCallbacks.keySet().contains(c.asBinder())) {
                    mIImsRegistrationCallbacks.remove(c.asBinder());
                } else {
                    log("addRegistrationCallback Listener already remove :" + c);
                }
            }
        }
    };

    @Override
    public IImsCallSession createCallSessionInterface(ImsCallProfile profile) {
        log("createCallSessionInterface->profile:" + profile);
        return mImsService.createCallSessionInternal(mServiceId,profile,null);
    }

    public IImsCallSession createCallSessionInterface(int serviceId, ImsCallProfile profile, IImsCallSessionListener listener) {
        log("createCallSessionInterface->profile:" + profile);
        return createCallSessionInternal(profile);
    }

    public IImsCallSession createCallSessionInternal(ImsCallProfile profile) {
        if(mImsServiceCallTracker != null){
            return mImsServiceCallTracker.createCallSession(profile);
        } else {
            return null;
        }
    }

    @Override
    public int shouldProcessCall(String[] uris){
        //TODO: return MmTelFeature.PROCESS_CALL_CSFB to make CS call
        return MmTelFeature.PROCESS_CALL_IMS;
    }


    @Override
    public void changeEnabledCapabilities(CapabilityChangeRequest request,
                                          ImsFeature.CapabilityCallbackProxy c){
        List<CapabilityChangeRequest.CapabilityPair> enableCap = request.getCapabilitiesToEnable();
        List<CapabilityChangeRequest.CapabilityPair> disableCap = request.getCapabilitiesToDisable();
        log("changeEnabledCapabilities->enableCap:" + enableCap + "/n disableCap:"+disableCap);
        boolean isVideoCapabilityChanged   = false; // UNISOC: Add for bug988585
        synchronized (mDeviceVolteCapabilities) {   // UNISOC: Add for bug978339,bug988585
            for (CapabilityChangeRequest.CapabilityPair pair : enableCap) {
                if (pair.getRadioTech() == ImsRegistrationImplBase.REGISTRATION_TECH_LTE) {
                    //add for unisoc 911545
                    mDeviceVolteCapabilities.addCapabilities(pair.getCapability());
                } else if (pair.getRadioTech() == ImsRegistrationImplBase.REGISTRATION_TECH_IWLAN) {
                    mDeviceVowifiCapabilities.addCapabilities(pair.getCapability());
                }
            }

            for (CapabilityChangeRequest.CapabilityPair pair : disableCap) {
                if (pair.getRadioTech() == ImsRegistrationImplBase.REGISTRATION_TECH_LTE) {
                    //add for unisoc 911545
                    mDeviceVolteCapabilities.removeCapabilities(pair.getCapability());
                } else if (pair.getRadioTech() == ImsRegistrationImplBase.REGISTRATION_TECH_IWLAN) {
                    mDeviceVowifiCapabilities.removeCapabilities(pair.getCapability());
                }
            }
            /* UNISOC: modify for bug968317 @{ */
            //add for unisoc 900059,911545
            int newVoLTESetting;
            if (mDeviceVolteCapabilities.isCapable(MmTelCapabilities.CAPABILITY_TYPE_VOICE)) {
                log("changeEnabledCapabilities-> setImsVoiceCallAvailability on");
                newVoLTESetting = ImsConfig.FeatureValueConstants.ON;
            } else {
                log("changeEnabledCapabilities-> setImsVoiceCallAvailability off");
                newVoLTESetting = ImsConfig.FeatureValueConstants.OFF;
            }

            if (VoLTECallAvailSync != VoLTECallAvailSyncStatus.VOLTE_CALL_AVAIL_SYNC_ONGOING) {
                currentVoLTESetting = newVoLTESetting;
                setVoLTECallAvailablity();
            } else {
                if (newVoLTESetting != currentVoLTESetting) {
                    pendingVoLTESetting = newVoLTESetting;
                } else {
                    pendingVoLTESetting = IMS_INVALID_VOLTE_SETTING;
                }
            }
            /*@}*/
            /*@}*/

            /* UNISOC: add for bug988585 @{ */
            boolean newVideoCapabilityEnabled = mDeviceVolteCapabilities.isCapable(MmTelCapabilities.CAPABILITY_TYPE_VIDEO);
            if(mVideoCapabilityEnabled != newVideoCapabilityEnabled)
            {
                isVideoCapabilityChanged = true;
                mVideoCapabilityEnabled = newVideoCapabilityEnabled;
                log("changeEnabledCapabilities->mVideoCapabilityEnabled:" + mVideoCapabilityEnabled);
            }
            /*@}*/
        }

        /* UNISOC: modify for bug988585 @{ */
        if(mDeviceCapabilitiesFirstChange || isVideoCapabilityChanged)
        {
            mImsService.updateImsFeature(mServiceId);
            mDeviceCapabilitiesFirstChange = false;
        }
        /*@}*/
    }


    @Override
    public boolean queryCapabilityConfiguration(int capability, int radioTech) {
        if(radioTech == ImsRegistrationImplBase.REGISTRATION_TECH_LTE){
            mDeviceVolteCapabilities.isCapable(capability);
        } else if(radioTech == ImsRegistrationImplBase.REGISTRATION_TECH_IWLAN){
            mDeviceVowifiCapabilities.isCapable(capability);
        }
        return false;
    }

    // SMS APIs
    @Override
    public ImsSmsImplBase getSmsImplementation() {
        //TODO: should implement ImsSmsImplBase
        if(mWifiService != null && mPhone != null){
            return mWifiService.getSmsImplementation(mPhone.getPhoneId());
        }else{
            log("getSmsImplementation mWifiService is null");
            return new ImsSmsImplBase();
        }
    }
    /* AndroidP end@} */

    /**
     * Handles changes to the APN db.
     */
    private class ApnChangeObserver extends ContentObserver {
        public ApnChangeObserver () {
            super(mHandler);
        }

        @Override
        public void onChange(boolean selfChange) {
            mHandler.sendMessage(mHandler.obtainMessage(DctConstants.EVENT_APN_CHANGED));
        }
    }

    //"VoLTE", "ViLTE", "VoWiFi", "ViWiFi","VOLTE-UT", "VOWIFI-UT"
    private int[] mEnabledFeatures = {
            ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN,
            ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN,
            ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN,
            ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN,
            ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN,
            ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN,
            ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN,
            ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN
    };
    private int[] mDisabledFeatures = {
            ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN,
            ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN,
            ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN,
            ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN,
            ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN,
            ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN,
            ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN,
            ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN
    };


    public ImsServiceImpl(Phone phone , Context context, VoWifiServiceImpl wifiService){
        mPhone = (GsmCdmaPhone)phone;
        mWifiService = wifiService;//Add for data router
        mContext = context;
        mImsService = (ImsService)context;
        mCi = new ImsRIL(phone.getContext(), phone.getPhoneId(), mPhone.mCi);
        mServiceId = phone.getPhoneId() + 1;
        mImsRegister = new ImsRegister(mPhone, mContext, mCi);
        mImsServiceState = new ImsServiceState(false,IMS_REG_STATE_INACTIVE);
        mImsConfigImpl = new ImsConfigImpl(mCi,context,this,mServiceId); // SPRD: bug805154
        mImsUtImpl = new ImsUtImpl(mCi,phone.getContext(),phone,this);
        com.spreadtrum.ims.vowifi.ImsUtImpl voWifiUtImpl =  mWifiService.getUtInterface(phone.getPhoneId());
        mImsUtProxy = new ImsUtProxy(context, mImsUtImpl, voWifiUtImpl, phone);
        mImsEcbmImpl = new ImsEcbmImpl(mCi);
        mHandler = new ImsHandler(mContext.getMainLooper());
        if(mImsServiceCallTracker == null) {
            mImsServiceCallTracker = new ImsServiceCallTracker(mContext, mCi, null, mServiceId, this, mWifiService);
            SessionListListener listListener = new SessionListListener();
            mImsServiceCallTracker.addListener(listListener);
        }

        /* UNISOC: add for bug916375 @{ */
        Intent intent = new Intent(ImsManager.ACTION_IMS_SERVICE_UP);
        intent.putExtra(ImsManager.EXTRA_PHONE_ID, phone.getPhoneId());
        mContext.sendStickyBroadcast(intent);
        mContext.sendBroadcast(intent);
        /*@}*/

        mCi.registerForImsBearerStateChanged(mHandler, EVENT_IMS_PND_STATE_CHANGED, null);
        mCi.registerForImsNetworkStateChanged(mHandler, EVENT_IMS_STATE_CHANGED, null);
        mCi.registerForSrvccStateChanged(mHandler, EVENT_SRVCC_STATE_CHANGED, null);
        mCi.registerImsHandoverStatus(mHandler, EVENT_IMS_HANDOVER_STATE_CHANGED, null);
        mCi.registerImsNetworkInfo(mHandler, EVENT_IMS_NETWORK_INFO_UPDATE, null);
        mCi.registerImsRegAddress(mHandler, EVENT_IMS_REGISTER_ADDRESS_CHANGED, null);
        mCi.registerImsWiFiParam(mHandler, EVENT_IMS_WIFI_PARAM, null);
        mCi.registerForAvailable(mHandler, EVENT_RADIO_AVAILABLE, null); // UNISOC: Add for bug968317
        mCi.registerForOn(mHandler, EVENT_RADIO_ON, null);               // UNISOC: Add for bug968317
        log("ImsServiceImpl onCreate->phoneId:"+phone.getPhoneId());
        mUiccController = UiccController.getInstance();
        mUiccController.registerForIccChanged(mHandler, DctConstants.EVENT_ICC_CHANGED, null);
        mApnChangeObserver = new ApnChangeObserver();
        mVolteConfig = VolteConfig.getInstance();
        mPhone.registerForServiceStateChanged(mHandler, EVENT_SERVICE_STATE_CHANGED, null);
        mPhone.getContext().getContentResolver().registerContentObserver(
                Telephony.Carriers.CONTENT_URI, true, mApnChangeObserver);
        mCi.registerForRadioStateChanged(mHandler, EVENT_RADIO_STATE_CHANGED, null);//SPRD:add for bug594553
        mCi.getImsRegAddress(mHandler.obtainMessage(EVENT_IMS_GET_IMS_REG_ADDRESS));//SPRD: add for bug739660

        mRadioInteractor = new RadioInteractor(context);//UNISOC:add for bug982110
        mCi.registerForImsErrorCause(mHandler, EVENT_IMS_ERROR_CAUSE_INFO, null);//UNISOC: add for bug1016116
    }

    /**
     * Used to listen to events.
     */
    private class ImsHandler extends Handler {
        ImsHandler(Looper looper) {
            super(looper);
        }
        @Override
        public void handleMessage(Message msg) {
            if (DBG) log("handleMessage msg=" + msg);
            AsyncResult ar = (AsyncResult) msg.obj;
            switch (msg.what) {
                case EVENT_IMS_STATE_CHANGED:
                    mCi.getImsRegistrationState(this.obtainMessage(EVENT_IMS_STATE_DONE));
                    // SPRD 681641 701983
                    if (ar.exception == null && ar.result != null && ar.result instanceof Integer) {
                        Integer responseArray = (Integer)ar.result;
                        mImsServiceState.mRegState = responseArray.intValue();
                        mImsServiceState.mImsRegistered = (mImsServiceState.mRegState == IMS_REG_STATE_REGISTERED
                                ? true : false);
                        // SPRD 869523
                        mImsServiceState.mSrvccState = -1;
                        log("EVENT_IMS_STATE_CHANGED->mRegState = "
                            + mImsServiceState.mRegState + " | mSrvccState = "
                            + mImsServiceState.mSrvccState);
                        switch(mImsServiceState.mRegState){
                            case IMS_REG_STATE_INACTIVE:
                                break;
                            case IMS_REG_STATE_REGISTERED:
                                break;
                            case IMS_REG_STATE_REG_FAIL:
                                break;
                            case IMS_REG_STATE_UNKNOWN:
                                break;
                            case IMS_REG_STATE_ROAMING:
                                break;
                            case IMS_REG_STATE_DEREGISTERING:
                                try{
                                    if(mListener == null){
                                        log("handleMessage msg=" + msg.what+" mListener is null!");
                                        break;
                                    }
                                    mListener.registrationProgressing();
                                    synchronized (mImsRegisterListeners) {
                                        for (IImsRegistrationListener l : mImsRegisterListeners.values()) {
                                            l.registrationProgressing();
                                        }
                                    }
                                } catch (RemoteException e){
                                    e.printStackTrace();
                                }
                                break;
                            default:
                                break;
                        }

                        /* UNISOC: add for bug968317 @{ */
                        if (((mImsServiceState.mRegState == IMS_REG_STATE_INACTIVE) || (mImsServiceState.mRegState == IMS_REG_STATE_REGISTERED))
                            && (VoLTECallAvailSync == VoLTECallAvailSyncStatus.VOLTE_CALL_AVAIL_SYNC_FAIL)) {
                            log("EVENT_IMS_STATE_CHANGED -> setVoLTECallAvailablity");
                            setVoLTECallAvailablity();
                        }
                        break;
                        /*@}*/
                    } else {
                        log("EVENT_IMS_STATE_CHANGED : ar.exception = "+ar.exception +" ar.result:"+ar.result);
                    }
                    break;
                case EVENT_IMS_STATE_DONE:
                    if (ar.exception == null && ar.result != null) {
                        // to be done
                        // SPRD 723085 mImsRegistered shouldn't be set twice time.
//                        int[] responseArray = (int[])ar.result;
//                        if(responseArray != null && responseArray.length >1){
//                            mImsServiceState.mImsRegistered = (responseArray[0] != 0 && responseArray[1]== 1);
//                            mImsServiceState.mSrvccState = -1;
//                        }
                    } else {
                        log("EVENT_IMS_STATE_DONE->ar.exception mServiceId:"+mServiceId);
                        mImsServiceState.mImsRegistered = false;
                    }
                    //add for SPRD:Bug 678430
                    log( "setTelephonyProperty mServiceId:"+mServiceId+"mImsRegistered:"+mImsServiceState.mImsRegistered);
                    log( "setTelephonyProperty isDualVolte:"+ImsManagerEx.isDualVoLTEActive());
                    TelephonyManager.setTelephonyProperty(mServiceId-1, "gsm.sys.volte.state",
                            mImsServiceState.mImsRegistered ? "1" :"0");
                    setPreferredNetworkRAT(isImsRegistered());
                    if(isImsRegistered()){
                        setUtDisableByNetWork(false);//UNISOC: add by bug1016166
                    }
                    notifyRegisterStateChange();
                    log("EVENT_IMS_STATE_DONE->mServiceState:" + mImsServiceState.mImsRegistered);
                    break;
                case DctConstants.EVENT_ICC_CHANGED:
                    onUpdateIcc();
                    break;
                case DctConstants.EVENT_RECORDS_LOADED:
                    onRecordsLoaded();
                    break;
                case DctConstants.EVENT_APN_CHANGED:
                    onApnChanged();
                    break;
                case EVENT_IMS_REGISTER_ADDRESS_CHANGED:
                    if(ar.exception == null && ar.result != null){
                        String[] address = (String[]) ar.result;
                        setIMSRegAddress(address[0]);
                        if (address.length > 1) {//SPRD: add for bug731711
                            log( "EVENT_IMS_REGISTER_ADDRESS_CHANGED psfcsAddr:" + address[1]);
                            mImsPscfAddress = address[1];
                        }
                    }else{
                        log("EVENT_IMS_REGISTER_ADDRESS_CHANGED has exception!");
                    }
                    break;
                case EVENT_SRVCC_STATE_CHANGED:
                    if (ar.exception == null) {
                        int[] ret = (int[]) ar.result;
                        if(ret != null && ret.length != 0){
                        	// SPRD:654852 add srvcc broadcast
//                        	if (mImsServiceState.mSrvccState == VoLteServiceState.HANDOVER_STARTED
//                                    || mImsServiceState.mSrvccState == VoLteServiceState.HANDOVER_COMPLETED) {
//                        		mImsServiceState.mImsRegistered = false;
//                        		
//                        	}
                            mImsServiceState.mSrvccState = ret[0];
                            // SPRD 689713
                            if (mImsServiceState.mSrvccState == VoLteServiceState.HANDOVER_COMPLETED) {
                                log( "Srvcc HANDOVER_COMPLETED : setTelephonyProperty mServiceId = " + mServiceId);
                                TelephonyManager.setTelephonyProperty(mServiceId-1, "gsm.sys.volte.state", "0");
                            }
                            mImsService.notifyImsRegisterState();
                            mImsService.notifySrvccState(mServiceId,mImsServiceState.mSrvccState);
                            log( "Srvcc state: " + ret[0]);
                        } else {
                            log( "Srvcc error ret: " + ret);
                        }
                    } else {
                        log( "Srvcc exception: " + ar.exception);
                    }
                    break;
                case EVENT_SERVICE_STATE_CHANGED:
                    log("EVENT_SERVICE_STATE_CHANGED->ServiceStateChange");
                    ServiceState state = (ServiceState) ((AsyncResult) msg.obj).result;
                    if (state != null && state.getDataRegState() == ServiceState.STATE_IN_SERVICE
                            && state.getRilDataRadioTechnology() == ServiceState.RIL_RADIO_TECHNOLOGY_LTE){
                        mImsRegister.enableIms();
                        setVideoResolution(state);
                    }
                    setInitialAttachSosApn(state);
                    break;
                /* SPRD: add for bug594553 @{ */
                case EVENT_RADIO_STATE_CHANGED:
                    log("EVENT_RADIO_STATE_CHANGED->mImsRegistered:" + mImsServiceState.mImsRegistered +"  isRaidoOn=" + mPhone.isRadioOn());
                    if (!mPhone.isRadioOn()) {
                        mImsServiceState.mImsRegistered = false;
                        // add for unisoc 947149
                        TelephonyManager.setTelephonyProperty(mServiceId-1, "gsm.sys.volte.state",
                            mImsServiceState.mImsRegistered ? "1" :"0");
                        notifyRegisterStateChange();
                    }
                   break;
                /* @} */
               case EVENT_IMS_HANDOVER_STATE_CHANGED:
                    if (ar != null && ar.exception == null && ar.result != null && ar.result instanceof Integer) {
                        Integer response = (Integer)ar.result;
                        mImsService.onImsHandoverStateChange(true, response.intValue());
                    } else {
                        log("EVENT_IMS_HANDOVER_STATE_CHANGED : ar.exception = "+ar.exception +" ar.result:"+ar.result);
                    }
                    break;
                case EVENT_IMS_HANDOVER_ACTION_COMPLETE:
                    if (ar == null || ar.exception != null) {
                        mImsService.onImsHandoverStateChange(false, ImsService.IMS_HANDOVER_ACTION_CONFIRMED);
                    } else {
                        mImsService.onImsHandoverStateChange(true, ImsService.IMS_HANDOVER_ACTION_CONFIRMED);
                    }
                    break;
                case EVENT_IMS_PND_STATE_CHANGED:
                    if (ar != null && ar.exception == null && ar.result != null && ar.result instanceof Integer) {
                        Integer responseArray = (Integer)ar.result;
                        mImsService.onImsPdnStatusChange(mServiceId,responseArray.intValue());
                        notifyImsPdnStateChange(responseArray.intValue());
                        if(responseArray.intValue() == ImsService.ImsPDNStatus.IMS_PDN_READY){
                                mCi.getImsPcscfAddress(mHandler.obtainMessage(EVENT_IMS_GET_PCSCF_ADDRESS));
                        }
                        /* UNISOC: add for bug968317 @{ */
                        if(((responseArray.intValue() == ImsService.ImsPDNStatus.IMS_PDN_READY) || (responseArray.intValue() == ImsService.ImsPDNStatus.IMS_PDN_ACTIVE_FAILED))
                            && (VoLTECallAvailSync == VoLTECallAvailSyncStatus.VOLTE_CALL_AVAIL_SYNC_FAIL)) {
                                log("EVENT_IMS_PND_STATE_CHANGED -> setVoLTECallAvailablity, ImsPDNStatus = " + responseArray.intValue());
                                setVoLTECallAvailablity();
                        }
                        /*@}*/
                    } else {
                        log("EVENT_IMS_PND_STATE_CHANGED : ar.exception = "+ar.exception +" ar.result:"+ar.result);
                    }
                    break;
                case EVENT_IMS_NETWORK_INFO_UPDATE:
                    if (ar != null && ar.exception == null && ar.result != null) {
                        ImsNetworkInfo info = (ImsNetworkInfo)ar.result;
                        log("EVENT_IMS_NETWORK_INFO_UPDATE->info.mType: " + info.type + "info.mInfo: " + info.info);
                        mImsService.onImsNetworkInfoChange(info.type, info.info);
                    }
                    break;
                /*SPRD: add for bug612670 @{ */
                case EVENT_SET_VOICE_CALL_AVAILABILITY_DONE:
                    /* UNISOC: modify for bug968317 @{ */
                    if(ar.exception != null){
                        log("EVENT_SET_VOICE_CALL_AVAILABILITY_DONE: exception "+ar.exception);
                        log("Set VoLTE Call Availability failure, currentVoLTESetting = " + currentVoLTESetting);
                    }

                    if(pendingVoLTESetting != IMS_INVALID_VOLTE_SETTING) {
                        log("set new VoLTESetting=" + pendingVoLTESetting);
                        currentVoLTESetting = pendingVoLTESetting;
                        setVoLTECallAvailablity();
                        pendingVoLTESetting = IMS_INVALID_VOLTE_SETTING;
                        break;
                    }

                    if(ar.exception != null){
                        VoLTECallAvailSync = VoLTECallAvailSyncStatus.VOLTE_CALL_AVAIL_SYNC_FAIL;
                    } else {
                        VoLTECallAvailSync  = VoLTECallAvailSyncStatus.VOLTE_CALL_AVAIL_SYNC_IDLE;
                        currentVoLTESetting = IMS_INVALID_VOLTE_SETTING;
                    }
                    /*@}*/
                    break;
                /*@}*/
                case EVENT_IMS_WIFI_PARAM:
                    if (ar != null && ar.exception == null && ar.result != null) {
                        onWifiParamEvent(ar);
                    }
                    break;
                case EVENT_IMS_GET_SRVCC_CAPBILITY:
                    if (ar != null && ar.exception == null && ar.result != null) {
                        int[] conn = (int[]) ar.result;
                        mSrvccCapbility = conn[0];
                        mImsService.notifySrvccCapbility(mSrvccCapbility);
                        log("EVENT_SET_VOICE_CALL_AVAILABILITY_DONE:"+mSrvccCapbility);
                    }
                    break;
                case EVENT_IMS_GET_PCSCF_ADDRESS:
                    if (ar != null && ar.exception == null && ar.result != null){
                        mImsPscfAddress = (String)ar.result;
                        log("EVENT_IMS_GET_PCSCF_ADDRESS,mImsPscfAddress:"+mImsPscfAddress);
                    }
                    break;
                case EVENT_IMS_GET_IMS_REG_ADDRESS:
                    if (ar != null && ar.exception == null && ar.result != null) {
                        String[] address = (String[]) ar.result;
                        if (address.length >= 2) {
                            setIMSRegAddress(address[0]);
                            mImsPscfAddress = address[1];
                            log( "EVENT_IMS_GET_IMS_REG_ADDRESS,mImsPscfAddress:" + mImsPscfAddress);
                        }
                    }
                    break;
                /* UNISOC: add for bug968317 @{ */
                case EVENT_RADIO_AVAILABLE:
                    log("EVENT_RADIO_AVAILABLE");
                    if (VoLTECallAvailSync == VoLTECallAvailSyncStatus.VOLTE_CALL_AVAIL_SYNC_FAIL) {
                        log("EVENT_RADIO_AVAILABLE -> setVoLTECallAvailablity");
                        setVoLTECallAvailablity();
                    }
                    break;
                case EVENT_RADIO_ON:
                    log("EVENT_RADIO_ON");
                    if (VoLTECallAvailSync == VoLTECallAvailSyncStatus.VOLTE_CALL_AVAIL_SYNC_FAIL) {
                        log("EVENT_RADIO_ON -> setVoLTECallAvailablity");
                        setVoLTECallAvailablity();
                    }
                    break;
                case EVENT_IMS_ERROR_CAUSE_INFO:
                    if (ar != null && ar.exception == null && ar.result != null) {
                        ImsErrorCauseInfo ImsErrorCauseInfo = (ImsErrorCauseInfo) ar.result;
                        onImsErrCauseInfoChange(ImsErrorCauseInfo);
                    } else {
                        if (ar != null) {
                            log("EVENT_IMS_REGISTER_SPIMS_REASON: ar.exception" + ar.exception + " ar.result: " + ar.result);
                        } else {
                            log("EVENT_IMS_REGISTER_SPIMS_REASON: ar == null");
                        }
                    }
                    break;
                /*@}*/
                case EVENT_IMS_GET_IMS_CNI_INFO:
                    if (ar != null && ar.exception == null && ar.result != null) {
                        ImsNetworkInfo info = (ImsNetworkInfo)ar.result;
                        Log.i(TAG,"EVENT_IMS_GET_IMS_CNI_INFO->info.type: " + info.type + " info.info:" + info.info + " info.age:" + info.age);
                        mImsService.onImsCNIInfoChange(info.type, info.info, info.age);
                    }
                    break;
                default:
                    break;
            }
        }
    };

    public int startSession(PendingIntent incomingCallIntent, IImsRegistrationListener listener) {
        mIncomingCallIntent = incomingCallIntent;
        mListener = listener;
        if(mImsServiceCallTracker == null) {
            mImsServiceCallTracker = new ImsServiceCallTracker(mContext, mCi, mIncomingCallIntent, mServiceId, this, mWifiService);
            SessionListListener listListener = new SessionListListener();
            mImsServiceCallTracker.addListener(listListener);
        }
        try{
            mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE]
                    = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
            mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_LTE]
                    = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
            mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI]
                    = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
            mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_WIFI]
                    = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
            mListener.registrationFeatureCapabilityChanged(
                    ImsServiceClass.MMTEL,mEnabledFeatures, mDisabledFeatures);
            synchronized (mImsRegisterListeners) {
                for (IImsRegistrationListener l : mImsRegisterListeners.values()) {
                    try{
                        l.registrationFeatureCapabilityChanged(
                                ImsServiceClass.MMTEL,mEnabledFeatures, mDisabledFeatures);
                    } catch(RemoteException e){
                        e.printStackTrace();
                        continue;
                    }
                }
            }
            setFeatureState(ImsFeature.STATE_READY);
        } catch (RemoteException e){
            e.printStackTrace();
        }
        return mServiceId;
    }

    @Override
    public int getFeatureState() {
        return ImsFeature.STATE_READY;
    }

    public void addRegistrationListener(IImsRegistrationListener listener) {
        if (listener == null) {
            log("addRegistrationListener->Listener is null!");
            Thread.dumpStack();
            return;
        }
        synchronized (mImsRegisterListeners) {
            if (!mImsRegisterListeners.keySet().contains(listener.asBinder())) {
                mImsRegisterListeners.put(listener.asBinder(), listener);
            } else {
                log("Listener already add :" + listener);
            }
        }
    }

    public void removeRegistrationListener(IImsRegistrationListener listener) {
        if (listener == null) {
            log("removeRegistrationListener->Listener is null!");
            Thread.dumpStack();
            return;
        }
        synchronized (mImsRegisterListeners) {
            if (mImsRegisterListeners.keySet().contains(listener.asBinder())) {
                mImsRegisterListeners.remove(listener.asBinder());
            } else {
                log("Listener already add :" + listener);
            }
        }
    }

    @Override
    public ImsCallProfile createCallProfile(int callSessionType, int callType) {
        return new ImsCallProfile(callSessionType,callType);
    }

    public IImsCallSession getPendingCallSession(int sessionId, String callId) {
        log("getPendingCallSession->callId:" + callId +
                " mImsServiceCallTracker:"+mImsServiceCallTracker);
        if(mImsServiceCallTracker != null){
            return mImsServiceCallTracker.getCallSession(callId);
        } else {
            return null;
        }
    }

    @Override
    public IImsUt getUtInterface() {
        return mImsUtProxy;
    }

    public IImsConfig getConfigInterface() {
        return (IImsConfig)mImsConfigImpl;
    }

    public void turnOnIms() {
        log("turnOnIms.");
        //add for bug 612670
    }

    public void turnOffIms() {
        log("turnOffIms.");
        //add for bug 612670
    }

    @Override
    public IImsEcbm getEcbmInterface() {
        return mImsEcbmImpl;
    }

    public void setUiTTYMode(int uiTtyMode, Message onComplete) {
    }

    @Override
    public void onFeatureRemoved() {

    }

    public IImsMmTelFeature  onCreateImsFeature(IImsFeatureStatusCallback c){
        log("onCreateImsFeature.");
        mImsFeatureStatusCallback = c;
        try {
            if(mImsFeatureStatusCallback == null){
                log("ImsServiceImpl mImsFeatureStatusCallback is null!");
            } else {
                mImsFeatureStatusCallback.notifyImsFeatureStatus(ImsFeature.STATE_READY);
            }
        } catch (RemoteException e){
            e.printStackTrace();
        }
        return getBinder();
    }


    public int open(int serviceClass, PendingIntent incomingCallIntent,
            IImsRegistrationListener listener){
        mServiceClass = serviceClass;
        mIncomingCallIntent = incomingCallIntent;
        mListener = listener;
        if(mImsServiceCallTracker == null) {
            mImsServiceCallTracker = new ImsServiceCallTracker(mContext, mCi, mIncomingCallIntent, mServiceId, this, mWifiService);
            SessionListListener listListener = new SessionListListener();
            mImsServiceCallTracker.addListener(listListener);
        }
        try{
            mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE]
                    = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
            mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_LTE]
                    = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
            mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI]
                    = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
            mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_WIFI]
                    = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
            mListener.registrationFeatureCapabilityChanged(
                    ImsServiceClass.MMTEL,mEnabledFeatures, mDisabledFeatures);
            synchronized (mImsRegisterListeners) {
                for (IImsRegistrationListener l : mImsRegisterListeners.values()) {
                    try{
                        l.registrationFeatureCapabilityChanged(
                                ImsServiceClass.MMTEL,mEnabledFeatures, mDisabledFeatures);
                    } catch(RemoteException e){
                        e.printStackTrace();
                        continue;
                    }
                }
            }
        } catch (RemoteException e){
            e.printStackTrace();
        }
        setFeatureState(ImsFeature.STATE_READY);
        return mServiceId;
    }

    public void close(){

    }


    public void setRegistrationListener(IImsRegistrationListener listener){
        mListener = listener;
    }

    public void setUiTTYMode(int serviceId, int uiTtyMode, Message onComplete) {

    }

    public ImsUtImpl getUtImpl(){
        return mImsUtImpl;
    }

    public IImsUt getUTProxy(){
        return mImsUtProxy;
    }

    private void onUpdateIcc() {
        if (mUiccController == null ) {
            return;
        }
        IccRecords newIccRecords = getUiccRecords(UiccController.APP_FAM_3GPP);

        IccRecords r = mIccRecords.get();
        if (r != newIccRecords) {
            if (r != null) {
                if (DBG) log("Removing stale icc objects.");
                r.unregisterForRecordsLoaded(mHandler);
                mIccRecords.set(null);
            }
            if (newIccRecords != null) {
                if (DBG) log("New records found");
                mIccRecords.set(newIccRecords);
                newIccRecords.registerForRecordsLoaded(
                        mHandler, DctConstants.EVENT_RECORDS_LOADED, null);
            }
        }
    }
    private IccRecords getUiccRecords(int appFamily) {
        return mUiccController.getIccRecords(mPhone.getPhoneId(), appFamily);
    }
    private void onRecordsLoaded() {
        if (DBG) log("onRecordsLoaded: createAllApnList");
        createAllApnList();
        setInitialAttachIMSApn();
    }
    private void createAllApnList(){
        mAllApnSettings = new ArrayList<ApnSetting>();
        IccRecords r = mIccRecords.get();
        String operator = (r != null) ? r.getOperatorNumeric() : "";
        if (operator != null) {
            String selection = "numeric = '" + operator + "'";
            String orderBy = "_id";
            if (DBG) log("createAllApnList: selection=" + selection);

            Cursor cursor = mPhone.getContext().getContentResolver().query(
                    Telephony.Carriers.CONTENT_URI, null, selection, null, orderBy);

            if (cursor != null) {
                if (cursor.getCount() > 0) {
                    mAllApnSettings = ApnUtils.createApnList(cursor);
                }
                cursor.close();
            }
        }
    }
    private void setInitialAttachIMSApn(){
        ApnSetting apn = null;
        for (ApnSetting a : mAllApnSettings) {
            if (ArrayUtils.contains(a.types, PhoneConstants.APN_TYPE_IMS)) {
                apn = a;
                break;
            }
        }
        if (apn == null) {
            if (DBG) log("initialAttachIMSApnSetting: X There in no available ims apn");
        }else {
            if (DBG) log("initialAttachIMSApnSetting: X selected ims Apn=" + apn);
            DataProfile dp = new DataProfile(apn.profileId, apn.apn, apn.protocol,
                                    apn.authType, apn.user, apn.password, apn.bearerBitmask == 0
                                            ? DataProfile.TYPE_COMMON : (ServiceState.bearerBitmapHasCdma(apn.bearerBitmask)
                                            ? DataProfile.TYPE_3GPP2 : DataProfile.TYPE_3GPP),
                                    apn.maxConnsTime, apn.maxConns, apn.waitTime, apn.carrierEnabled, apn.typesBitmap,
                                    apn.roamingProtocol, apn.bearerBitmask, apn.mtu, apn.mvnoType, apn.mvnoMatchData,
                                    apn.modemCognitive);
            mCi.setIMSInitialAttachApn(dp, null);

        }
    }
    /**
     * Handles changes to the APN database.
     */
    private void onApnChanged() {
        if (DBG) log("onApnChanged: createAllApnList");
        createAllApnList();
        setInitialAttachIMSApn();
    }

    public int getServiceId(){
        return mServiceId;
    }

    public IImsCallSession getPendingCallSession(String callId){
        log("getPendingCallSession->callId:" + callId +
                " mImsServiceCallTracker:"+mImsServiceCallTracker);
        if(mImsServiceCallTracker != null){
            return mImsServiceCallTracker.getCallSession(callId);
        } else {
            return null;
        }
    }

    public void enableImsWhenPDNReady(){
        log("enableImsWhenPDNReady.");
        mImsRegister.onImsPDNReady();
        mImsRegister.enableIms();
    }

    public void disableIms(){
        log("disableIms.");
        mCi.disableIms(null);
    }

    public int getImsRegisterState(){
        //add for Bug 620214
        if (mImsServiceState.mImsRegistered) {
            if (mPhone.getState() == PhoneConstants.State.IDLE) {
                return ImsManagerEx.IMS_REGISTERED;
            } else {
                if(mImsServiceState.mSrvccState == VoLteServiceState.HANDOVER_STARTED
                        || mImsServiceState.mSrvccState == VoLteServiceState.HANDOVER_COMPLETED){
                    return ImsManagerEx.IMS_UNREGISTERED;
                }
                return ImsManagerEx.IMS_REGISTERED;
            }
        }
        return ImsManagerEx.IMS_UNREGISTERED;
    }

    public boolean isImsRegisterState(){
        //add for Bug 620214
        if(mImsServiceState.mImsRegistered){
            if(mPhone.getState() == PhoneConstants.State.IDLE){
                return true;
            } else {
            	// SPRD 659914
                if (mImsServiceState.mSrvccState == VoLteServiceState.HANDOVER_COMPLETED) {
                    return false;
                } else if (mImsServiceState.mSrvccState == VoLteServiceState.HANDOVER_FAILED || 
                		mImsServiceState.mSrvccState == VoLteServiceState.HANDOVER_CANCELED) {
                	return true;
                }
                return true;
            }
        }
        return false;
    }

    public int getSrvccState(){
        return mImsServiceState.mSrvccState;
    }

    public void setVideoResolution(ServiceState state){
        String registOperatorNumeric = state.getDataOperatorNumeric();
        log("registOperatorNumeric = " + registOperatorNumeric);
        if(registOperatorNumeric == null){
            return;
        }
        String operatorCameraResolution = null;
        mVolteConfig.loadVolteConfig(mContext);
        if(mVolteConfig.containsCarrier(registOperatorNumeric)){
            operatorCameraResolution = mVolteConfig.getCameraResolution(registOperatorNumeric);
        }
        if(operatorCameraResolution == null || TextUtils.isEmpty(operatorCameraResolution)){
            return;
        }
        mImsConfigImpl.mDefaultVtResolution = Integer.parseInt(operatorCameraResolution);
        log("ImsServiceImpl ==> setVideoResolution mDefaultVtResolution = " + operatorCameraResolution);
        // SPRD:909828
        mImsConfigImpl.sendVideoQualitytoIMS(Integer.parseInt(operatorCameraResolution));
//        mImsConfigImpl.setVideoQualitytoPreference(Integer.parseInt(operatorCameraResolution));
    }

    /* UNISOC: 630048 add sos apn for yes 4G @{*/
    public void setInitialAttachSosApn(ServiceState state){
        String carrier = state.getOperatorNumeric();
        if (carrier != null && !carrier.isEmpty()) {
            String carrierApn = mVolteConfig.getApn(carrier);
            if (carrierApn != null && !carrierApn.isEmpty()) {
                if (DBG) log("SosApn: apn=" + carrierApn + ", set sos apn = " + mSetSosApn);
                if (mSetSosApn) {
                    mSetSosApn = false;
                    String[] emergency = {"emergency"};
                    ApnSetting apn = new ApnSetting(0, carrier, "", carrierApn, "", "", "", "", "", "", "", 0,
                            emergency, "IPV4V6", "IPV4V6", true, 0, 0, 0, false, 0, 0, 0, 0, "", "");
                    DataProfile dp = new DataProfile(apn.profileId, apn.apn, apn.protocol,
                            apn.authType, apn.user, apn.password, apn.bearerBitmask == 0
                            ? DataProfile.TYPE_COMMON : (ServiceState.bearerBitmapHasCdma(apn.bearerBitmask)
                            ? DataProfile.TYPE_3GPP2 : DataProfile.TYPE_3GPP),
                            apn.maxConnsTime, apn.maxConns, apn.waitTime, apn.carrierEnabled, apn.typesBitmap,
                            apn.roamingProtocol, apn.bearerBitmask, apn.mtu, apn.mvnoType, apn.mvnoMatchData,
                            apn.modemCognitive);

                    mCi.setInitialAttachSOSApn(dp, null);
                }
            }
        } else {
            if (DBG) log("carrier is null");
            mSetSosApn = true;
        }
    }
    /* @} */

    class SessionListListener implements ImsServiceCallTracker.SessionListListener {
        @Override
        public void onSessionConnected(ImsCallSessionImpl callSession){
        }

        @Override
        public void  onSessionDisonnected(ImsCallSessionImpl callSession){
            if(mImsServiceCallTracker.isSessionListEmpty()){
                notifySessionEmpty();
            }
        }
    }

    public interface Listener {
        void onRegisterStateChange(int serviceId);
        void onSessionEmpty(int serviceId);
    }

    public void addListener(Listener listener){
        if (listener == null) {
            log("addListener-> listener is null!");
            return;
        }
        if (!mListeners.contains(listener)) {
            mListeners.add(listener);
        } else {
            log("addListener-> listener already add!");
        }
    }

    public void removeListener(Listener listener){
        if (listener == null) {
            log("removeListener-> listener is null!");
            return;
        }
        if (mListeners.contains(listener)) {
            mListeners.remove(listener);
        } else {
            log("addListener-> listener already remove!");
        }
    }
    //SPRD: add for bug 771875
    public void updateImsFeature(int feature, int value) {
        log( "updateImsFeatures->feature:" + feature + " value:" + value);
        updateImsFeatures(mImsService.isVoLTEEnabled(), mImsService.isVoWifiEnabled());
    }

    public void notifyRegisterStateChange() {
        // SPRD Add for DSDA bug684926:
        // If dual volte active, update RAT to 4G and voice reg state to in service.
        // And if dual volte not active, service state need to be set to correct state.
        int phoneCount = TelephonyManager.from(mContext).getPhoneCount();
        if(phoneCount > 1 && mPhone.getPhoneId() != getImsRegister().getPrimaryCard(phoneCount)) {
            log( "Ims Register State Changed, poll state again on vice SIM,"
                    + "phone Id = " + mPhone.getPhoneId());
            mPhone.getServiceStateTracker().pollState();
        }
        for (Listener listener : mListeners) {
            listener.onRegisterStateChange(mServiceId);
        }
    }

    public void notifySessionEmpty() {
        for (Listener listener : mListeners) {
            listener.onSessionEmpty(mServiceId);
        }
    }

    public boolean isImsRegistered(){
        return mImsServiceState.mImsRegistered;
    }

    public ImsRegister getImsRegister() {
        return mImsRegister;
    }

    public void notifyImsRegister(boolean isRegistered, boolean isVolte, boolean isWifiRegistered){   // UNISOC: Modify for bug880865
        try{
            // SPRD: 730973
            if(isRegistered){
                mVolteRegisterStateOld = true;
            } else {
                mVolteRegisterStateOld = false;
            }
            synchronized (mIImsRegistrationCallbacks) {
                for (IImsRegistrationCallback l : mIImsRegistrationCallbacks.values()) {
                    if(isRegistered) {
                        l.onRegistered(isVolte ? ImsRegistrationImplBase.REGISTRATION_TECH_LTE
                                : ImsRegistrationImplBase.REGISTRATION_TECH_IWLAN);
                    } else {
                        l.onDeregistered(new ImsReasonInfo());
                    }
                }
            }

            log("notifyImsRegister->isRegistered:" + isRegistered
                    + " isWifiRegistered:"+isWifiRegistered
                    + " isImsEnabled():"+mImsService.isImsEnabled()
                    + " mImsService.isVoLTEEnabled():"+mImsService.isVoLTEEnabled()
                    + " mImsService.isVoWifiEnabled():"+mImsService.isVoWifiEnabled());
            log("notifyImsRegister->mServiceState:" + isRegistered);

            // UNISOC: Add for bug880865
            TelephonyManager.setTelephonyProperty(mServiceId-1, "gsm.sys.vowifi.state",
                                                  isWifiRegistered ? "1" :"0");

            mImsRegister.notifyImsStateChanged(isRegistered);
            if(mListener == null){
                log("notifyImsRegister->mListener is null!");
                return;
            }
            if(isRegistered){
                mListener.registrationConnected();
            } else {
                mListener.registrationDisconnected(new ImsReasonInfo());
            }
            synchronized (mImsRegisterListeners) {
                for (IImsRegistrationListener l : mImsRegisterListeners.values()) {
                    if(isRegistered){
                        l.registrationConnected();
                    } else {
                        l.registrationDisconnected(new ImsReasonInfo());
                    }
                }
            }

        } catch (RemoteException e){
            e.printStackTrace();
        }
    }

    public void sendIncomingCallIntent(IImsCallSession c, String callId, boolean unknownSession, boolean isVolteCall) { // UNISOC: Modify for bug909030
        Bundle extras = new Bundle();
        extras.putBoolean(ImsManager.EXTRA_USSD, false);
        extras.putBoolean(ImsManager.EXTRA_IS_UNKNOWN_CALL, unknownSession); // UNISOC: Modify for bug909030
        extras.putString(ImsManager.EXTRA_CALL_ID, callId);
            /*SPRD: Modify for bug586758{@*/
        log("sendIncomingCallIntent-> " + (isVolteCall ? "startVolteCall" : "startVoWifiCall") // UNISOC: Modify for bug909030
                + " mIsVowifiCall: " + mImsService.isVowifiCall()
                + " mIsVolteCall: " + mImsService.isVolteCall()
                + " isVoWifiEnabled(): " + mImsService.isVoWifiEnabled()
                + " isVoLTEEnabled(): " + mImsService.isVoLTEEnabled());

        /*UNISOC: Add for bug909030{@*/
        mImsService.setInCallPhoneId(mServiceId - 1);
        mImsService.updateInCallState(true);

        int phoneCount = TelephonyManager.from(mContext).getPhoneCount();
        boolean isPrimaryCard = (ImsRegister.getPrimaryCard(phoneCount) == (mServiceId - 1));

        if(isPrimaryCard) {
            /*VoLTE Call*/
            if (mImsService.isVoLTEEnabled() && !mImsService.isVowifiCall() && !mImsService.isVolteCall()) {
                mImsService.setCallType(ImsService.CallType.VOLTE_CALL);
                mWifiService.updateCallRatState(CallRatState.CALL_VOLTE);
            }

            /*VoWifi Call*/
            if (mImsService.isVoWifiEnabled() && !mImsService.isVowifiCall() && !mImsService.isVolteCall()) {
                mImsService.setCallType(ImsService.CallType.WIFI_CALL); // UNISOC: Add for bug909030
                mWifiService.updateCallRatState(CallRatState.CALL_VOWIFI);
            }
            /*@}*/
        }
        /*@}*/
        notifyIncomingCallSession(c,extras);
    }

    public String getIMSRegAddress() {
        if(DBG){
            log("getIMSRegAddress mImsRegAddress = " + mImsRegAddress);
        }
        return mImsRegAddress;
    }

    public String getImsPcscfAddress(){
        if(DBG){
            log("getImsPcscfAddress mImsPscfAddress = " + mImsPscfAddress);
        }
        return mImsPscfAddress;
    }

    public void setIMSRegAddress(String addr) {
        if(DBG){
            log( "setIMSRegAddress addr = " + addr);
        }
        mImsRegAddress = addr;

        int phoneCount = TelephonyManager.from(mContext).getPhoneCount();
        if((mPhone.getPhoneId() == getImsRegister().getPrimaryCard(phoneCount)) || (mServiceId == mImsService.getVoWifiServiceId())) { // SPRD: add for bug974910
            //update vowifi address for primary sim or vowifi service sim.
            Log.d(TAG, "setIMSRegAddress update VoWifi addr, phone Id =" + mPhone.getPhoneId() + " vowifiServId =" + mImsService.getVoWifiServiceId());
            mWifiService.setUsedLocalAddr(addr);
        }
    }
    public void requestImsHandover(int type){
        log("requestImsHandover->type:" + type);
        mCi.requestImsHandover(type,mHandler.obtainMessage(EVENT_IMS_HANDOVER_ACTION_COMPLETE));
    }

    public void notifyImsHandoverStatus(int status){
        log("notifyImsHandoverStatus->status:" + status);
        mCi.notifyImsHandoverStatus(status,null);
    }

    public void notifyImsCallEnd(int type){
        log("notifyImsCallEnd.");
        mCi.notifyImsCallEnd(type,null);
    }

    public void notifyVoWifiEnable(boolean enable){
        log("notifyVoWifiEnable.");
        mCi.notifyVoWifiEnable(enable,null);
    }
    /*SPRD: Add for get network info{@*/
    public void notifyImsNetworkInfo(int type, String info){
        log("notifyImsNetworkInfo->type:"+type+" info:"+info);
        mCi.notifyImsNetworkInfo(type, info,null);
    }
    /*@}*/
    public void onImsCallEnd(){
        mImsService.onImsCallEnd(mServiceId);
    }

    public void notifyWifiCalling(boolean inCall){
        mCi.notifyVoWifiCallStateChanged(inCall,null);
    }
    /*SPRD: Add for notify data router{@*/
    public void notifyDataRouter(){
        log("notifyDataRouter");
        mCi.notifyDataRouter(null);
    }
    /*@}*/
    /*SPRD: Add for bug586758{@*/
    public boolean isVolteSessionListEmpty() {
        if (mImsServiceCallTracker != null) {
            log("isSessionListEmpty: " + mImsServiceCallTracker.isSessionListEmpty());
            return mImsServiceCallTracker.isSessionListEmpty();
        }
        return false;
    }
    public boolean isVowifiSessionListEmpty() {
        if (mWifiService != null) {
            log("mWifiService.getCallCount(): " + mWifiService.getCallCount());
            return mWifiService.getCallCount()==0;
        }
        return false;
    }
    /*@}*/

    public void onWifiParamEvent(Object object){
        AsyncResult ar =(AsyncResult)object;
        int resultArray[] = (int[]) ar.result;
        log("onWifiParamEvent->rtp_time_Out:" + resultArray[3]);
        mAliveCallLose = resultArray[1];
        mAliveCallJitter = resultArray[2];
        mAliveCallRtt = resultArray[0];
        IImsServiceListenerEx imsServiceListenerEx = mImsService.getImsServiceListenerEx();
        try {
            if (imsServiceListenerEx != null) {
                log("onWifiParamEvent->onMediaQualityChanged->isvideo:false"
                        + " mAliveCallLose:" + mAliveCallLose + " mAliveCallJitter:" + mAliveCallJitter
                        + " mAliveCallRtt:" + mAliveCallRtt);
                imsServiceListenerEx.onMediaQualityChanged(false,mAliveCallLose,mAliveCallJitter,mAliveCallRtt);
                if (resultArray[3] == IMS_CALLING_RTP_TIME_OUT) {
                    log("onWifiParamEvent->onRtpReceived->isvideo:false");
                    imsServiceListenerEx.onNoRtpReceived(false);
                }
            } else {
                log("onWifiParamEvent->imsServiceListenerEx is null");
            }
        } catch(RemoteException e){
            e.printStackTrace();
        }
    }

    public int getAliveCallLose() {
        log("getAliveCallLose->mAliveCallLose:" + mAliveCallLose);
        return mAliveCallLose;
    }

    public int getAliveCallJitter() {
        log("getAliveCallJitter->mAliveCallJitter:" + mAliveCallJitter);
        return mAliveCallJitter;
    }

    public int getAliveCallRtt() {
        log("getAliveCallRtt->mAliveCallRtt:" + mAliveCallRtt);
        return mAliveCallRtt;
    }

    public void updateImsFeatures(boolean volteEnable, boolean wifiEnable){
        log("updateImsFeatures->volteEnable:" + volteEnable + " wifiEnable:" + wifiEnable+" id:"+mServiceId);
        ImsManager imsManager = ImsManager.getInstance(mContext,mPhone.getPhoneId());
        synchronized (mVolteCapabilities) {   // UNISOC: Add for bug978339, bug988585
            try {
                if (volteEnable) {
                    mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE]
                            = ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE;
                    mVolteCapabilities.addCapabilities(MmTelCapabilities.CAPABILITY_TYPE_VOICE);
                    mVolteCapabilities.addCapabilities(MmTelCapabilities.CAPABILITY_TYPE_UT);
                    if (imsManager.isVtEnabledByUser() && imsManager.isVtEnabledByPlatform()) {//SPRD:modify for bug805161
                        mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_LTE]
                                = ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_LTE;
                        mVolteCapabilities.addCapabilities(MmTelCapabilities.CAPABILITY_TYPE_VIDEO);
                    } else {
                        mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_LTE]
                                = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
                        mVolteCapabilities.removeCapabilities(MmTelCapabilities.CAPABILITY_TYPE_VIDEO);
                    }
                    mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_UT_OVER_LTE]
                            = ImsConfig.FeatureConstants.FEATURE_TYPE_UT_OVER_LTE;
                    notifyCapabilitiesStatusChanged(mVolteCapabilities);
                } else {
                    mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE]
                            = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
                    mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_LTE]
                            = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
                    mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_UT_OVER_LTE]
                            = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
                    mVolteCapabilities.removeCapabilities(MmTelCapabilities.CAPABILITY_TYPE_VOICE);
                    mVolteCapabilities.removeCapabilities(MmTelCapabilities.CAPABILITY_TYPE_UT);
                    mVolteCapabilities.removeCapabilities(MmTelCapabilities.CAPABILITY_TYPE_SMS);
                    mVolteCapabilities.removeCapabilities(MmTelCapabilities.CAPABILITY_TYPE_VIDEO);
                }
                if (wifiEnable) {
                    mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI]
                            = ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI;
                    mVowifiCapabilities.addCapabilities(MmTelCapabilities.CAPABILITY_TYPE_VOICE);
                    mVowifiCapabilities.addCapabilities(MmTelCapabilities.CAPABILITY_TYPE_UT);
                    if (VoWifiConfiguration.isSupportSMS(mContext)){
                        mVowifiCapabilities.addCapabilities(MmTelCapabilities.CAPABILITY_TYPE_SMS);
                    } else {
                        mVowifiCapabilities.removeCapabilities(MmTelCapabilities.CAPABILITY_TYPE_SMS);
                    }
                    if (imsManager.isVtEnabledByUser() && imsManager.isVtEnabledByPlatform()) {//SPRD:modify for bug810321
                        mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_WIFI]
                                = ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_WIFI;
                        mVowifiCapabilities.addCapabilities(MmTelCapabilities.CAPABILITY_TYPE_VIDEO);
                    } else {
                        mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_WIFI]
                                = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
                        mVowifiCapabilities.removeCapabilities(MmTelCapabilities.CAPABILITY_TYPE_VIDEO);
                    }
                    mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_UT_OVER_WIFI]
                            = ImsConfig.FeatureConstants.FEATURE_TYPE_UT_OVER_WIFI;
                    notifyCapabilitiesStatusChanged(mVowifiCapabilities);
                } else {
                    mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI]
                            = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
                    mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_WIFI]
                            = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
                    mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_UT_OVER_WIFI]
                            = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
                }
                synchronized (mImsRegisterListeners) {
                    for (IImsRegistrationListener l : mImsRegisterListeners.values()) {
                        l.registrationFeatureCapabilityChanged(
                                ImsServiceClass.MMTEL, mEnabledFeatures, mDisabledFeatures);
                    }
                }
                if (!volteEnable && !wifiEnable) {
                    notifyCapabilitiesStatusChanged(mVolteCapabilities);
                }

                //wifi capability caused by a handover for bug837323 { */
                updateImsCallProfile(wifiEnable);

                if (mListener == null) {
                    log("updateImsFeatures mListener is null!");
                    return;
                }
                mListener.registrationFeatureCapabilityChanged(
                        ImsServiceClass.MMTEL, mEnabledFeatures, mDisabledFeatures);

            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }
    }

    /* SPRD: wifi capability caused by a handover for bug837323 { */
    public void updateImsCallProfile(boolean wifiEnable){
        if(mImsServiceCallTracker != null){
            mImsServiceCallTracker.updateImsCallProfile(wifiEnable);
            log(" updateImsCallProfile->ok");
        }else{
            log(" updateImsCallProfile->mImsServiceCallTracker is null");
        }
    }
    /*@}*/

    public int getVolteRegisterState() {
        log("getVolteRegisterState->VolteRegisterState:" + mImsServiceState.mRegState);
        return mImsServiceState.mRegState;
    }
    public void terminateVolteCall(){
        if(mImsServiceCallTracker != null){
            mImsServiceCallTracker.terminateVolteCall();
            log(" terminateVolteCall->ok");
        }else{
            log(" terminateVolteCall->mImsServiceCallTracker is null");
        }
    }

    public void notifyHandoverCallInfo(String callInfo) {
        mCi.notifyHandoverCallInfo(callInfo,null);
    }

    public void getSrvccCapbility() {
        mCi.getSrvccCapbility(mHandler.obtainMessage(EVENT_IMS_GET_SRVCC_CAPBILITY));
    }

    //SPRD: add for bug671964
    public void setImsPcscfAddress(String addr) {
        log( "setImsPcscfAddress addr = " + addr);
        String pcscfAdd = "";
        if (addr != null && addr.length() != 0) {

            if (addr.matches("\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}")) {
                pcscfAdd = "1,\"" + addr + "\"";
            } else if (addr.contains(":")) {
                pcscfAdd = "2,\"" + addr + "\"";
            }

            log( "setImsPcscfAddress pcscfAdd = " + pcscfAdd);
            if (pcscfAdd.length() != 0) {
                mCi.setImsPcscfAddress(pcscfAdd, null);
            }
        }
    }
    public void setSrvccState(int srvccState){
        mImsServiceState.mSrvccState = srvccState;
    }

    /**
     * Used for add IMS PDN State Listener.
     */
    public void addImsPdnStateListener(IImsPdnStateListener listener){
        if (listener == null) {
            log("addImsPdnStateListener->Listener is null!");
            Thread.dumpStack();
            return;
        }
        synchronized (mImsPdnStateListeners) {
            if (!mImsPdnStateListeners.keySet().contains(listener.asBinder())) {
                mImsPdnStateListeners.put(listener.asBinder(), listener);
            } else {
                log("addImsPdnStateListener Listener already add :" + listener);
            }
        }
    }

    /**
     * Used for remove IMS PDN State Listener.
     */
    public void removeImsPdnStateListener(IImsPdnStateListener listener){
        if (listener == null) {
            log("removeImsPdnStateListener->Listener is null!");
            Thread.dumpStack();
            return;
        }
        synchronized (mImsPdnStateListeners) {
            if (mImsPdnStateListeners.keySet().contains(listener.asBinder())) {
                mImsPdnStateListeners.remove(listener.asBinder());
            } else {
                log("removeImsPdnStateListener Listener already add :" + listener);
            }
        }
    }

    public boolean isImsEnabled(){
        return ((mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE]
                == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE) ||
                (mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI]
                == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI));
    }

    public boolean isVoLteEnabled(){
        return (mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE]
                == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE);
    }

    public boolean isVoWifiEnabled(){
        return ((mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI]
                        == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI));
    }

    public void updateImsFeatureForAllService(){
        mImsService.updateImsFeatureForAllService();
    }

    public void getSpecialRatcap(ServiceState state) {

        if (state != null && mCi != null) {
            if (mServiceState != state.getState()) {
                log("getSpecialRatcap: servaiceState: " + state.getState());
                if (state.getState() == ServiceState.STATE_POWER_OFF) {
                    setUtDisableByNetWork(false);
                    log("getSpecialRatcap: mIsUtDisableByNetWork: " + mIsUtDisableByNetWork);
                }
            }
            mServiceState = state.getState();
        }
    }

    /*UNISOC: add for bug1016166 @{*/
    public void setUtDisableByNetWork(boolean value){
        mIsUtDisableByNetWork = value;
    }

    public boolean getCarrierCofValueByKey(int phoneId,String key) {

        Log.d(TAG, "getCarrierCofValueByKey phoneId = " + phoneId + " key = " + key);
        boolean carrierCofValue = false;
        CarrierConfigManager carrierConfig = (CarrierConfigManager) mContext.getSystemService(
                Context.CARRIER_CONFIG_SERVICE);
        if (carrierConfig != null) {
            PersistableBundle config = carrierConfig.getConfigForPhoneId(phoneId);
            if (config != null) {
                carrierCofValue = config.getBoolean(key, false);
            }
        }
        return carrierCofValue;
    }

    public void onImsErrCauseInfoChange(ImsErrorCauseInfo imsErrorCauseInfo) {

        if (getCarrierCofValueByKey(mPhone.getPhoneId(),
                CarrierConfigManagerEx.KEY_CARRIER_SUPPORT_DISABLE_UT_BY_NETWORK)) {
            if (imsErrorCauseInfo.type == IMS_ERROR_CAUSE_TYPE_IMSREG &&
                    imsErrorCauseInfo.errCode == IMS_ERROR_CAUSE_ERRCODE_REG_FORBIDDED) {
                setUtDisableByNetWork(true);
                Log.d(TAG, "onImsCsfbReasonInfoChange = " + mIsUtDisableByNetWork);
            }
        }
        log("onImsCsfbReasonInfoChange: info:" + imsErrorCauseInfo.type + " errCode:"
                + imsErrorCauseInfo.errCode + " errDescription: " + imsErrorCauseInfo.errDescription);
    }
    /*@}*/

    public void notifyImsPdnStateChange(int state){
        synchronized (mImsPdnStateListeners) {
            for (IImsPdnStateListener l : mImsPdnStateListeners.values()) {
                try{
                    l.imsPdnStateChange(state);
                } catch(RemoteException e){
                    e.printStackTrace();
                    continue;
                }
            }
        }
    }

    // SPRD Add for bug696648
    public boolean hasCall() {
        return mImsServiceCallTracker.hasCall();
    }

    public void enableWiFiParamReport(){
        mCi.enableWiFiParamReport(true,null);
    }

    public void disableWiFiParamReport(){
        mCi.enableWiFiParamReport(false, null);
    }

    // SPRD: 730973
    public boolean getVolteRegisterStateOld(){
        return mVolteRegisterStateOld;
    }

    public void setVolteRegisterStateOld(boolean state){
        mVolteRegisterStateOld = state;
    }

    public void onCallWaitingStatusUpdateForVoWifi(int status){
        mImsService.onCallWaitingStatusUpdateForVoWifi(status);
    }

    public void log(String info){
        Log.i(TAG,"["+mServiceId+"]:" + info);
    }

    // SPRD add for dual LTE
    private int getSubId() {
        int[] subIds = SubscriptionManager.getSubId(mPhone.getPhoneId());
        int subId = SubscriptionManager.INVALID_SUBSCRIPTION_ID;
        if (subIds != null && subIds.length >= 1) {
            subId = subIds[0];
        }
        return subId;
    }

    /* UNISOC: Add for bug950573 @{*/
    /**
     * Used for get IMS feature.
     *
     * @return: ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN = -1;
     *          ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE = 0;
     *          ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI = 2;
     */
    public int getCurrentImsFeature() {
        return mCurrentImsFeature;
    }
    /**
     * Used for set IMS feature.
     *
     * @param: imsFeature: ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN = -1;
     *                     ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE = 0;
     *                     ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI = 2;
     */
    public void setCurrentImsFeature(int imsFeature) {
        mCurrentImsFeature = imsFeature;
    }
    /*@}*/

    /* UNISOC: add for bug968317 @{ */
    /**
     * Used for set VoLTE Voice Call Availablity.
     *
     */
    private void setVoLTECallAvailablity() {
        if(currentVoLTESetting != IMS_INVALID_VOLTE_SETTING) {
            VoLTECallAvailSync = VoLTECallAvailSyncStatus.VOLTE_CALL_AVAIL_SYNC_ONGOING;

            log("setVoLTECallAvailablity, currentVoLTESetting = " + currentVoLTESetting);
            mCi.setImsVoiceCallAvailability(currentVoLTESetting , mHandler.obtainMessage(EVENT_SET_VOICE_CALL_AVAILABILITY_DONE, currentVoLTESetting));
        }
    }
    /*@}*/

    /*UNISOC:add for bug982110
     *Volte register: ps
     *2/3/4G :cs
     *  @} */
    public void setPreferredNetworkRAT(boolean volteRegister) {
        if(volteRegister == getVolteRegisterStateOld()){
            return;
        }
        CarrierConfigManager carrierConfig = (CarrierConfigManager) mContext.getSystemService(
                Context.CARRIER_CONFIG_SERVICE);
        int networkPref = 0;
        boolean setRat = false;
        if (carrierConfig != null) {
            PersistableBundle config = carrierConfig.getConfigForPhoneId(mPhone.getPhoneId());
            if (config != null) {

                setRat = config.getBoolean(CarrierConfigManagerEx.KEY_NETWORK_RAT_ON_SWITCH_IMS);
                if (setRat) {
                    networkPref = config.getInt(CarrierConfigManagerEx.KEY_NETWORK_RAT_PREFER_INT);
                }
            }
        }
        Log.d(TAG, "setPreferredNetworkRAT: networkPref = " + networkPref + " setRat = " + setRat);
        if (setRat) {
            if (volteRegister) {
                networkPref = 0;
            }
            mRadioInteractor.setNetworkSpecialRATCap(networkPref, mPhone.getPhoneId());
        }
    }/*@}*/

    public void getImsCNIInfo(){
        mCi.getImsCNIInfo(mHandler.obtainMessage(EVENT_IMS_GET_IMS_CNI_INFO));
    }
}

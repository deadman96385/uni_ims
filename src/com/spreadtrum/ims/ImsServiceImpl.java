package com.spreadtrum.ims;

import java.util.ArrayList;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicReference;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.database.ContentObserver;
import android.database.Cursor;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.AsyncResult;
import android.telephony.ServiceState;
import android.telephony.TelephonyManager;
import android.telephony.VoLteServiceState;
import android.telephony.ims.feature.ImsFeature;
import android.telephony.ims.feature.MMTelFeature;
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
import com.android.ims.ImsReasonInfo;
import com.android.ims.ImsServiceClass;
import com.android.ims.ImsCallProfile;
import com.android.ims.ImsConfig;
import com.spreadtrum.ims.ut.ImsUtImpl;
import com.spreadtrum.ims.ut.ImsUtProxy;
import com.android.ims.internal.IImsCallSession;
import com.android.ims.internal.IImsCallSessionListener;
import com.android.ims.internal.IImsRegistrationListener;
import com.android.ims.internal.IImsPdnStateListener;
import com.android.ims.internal.IImsEcbm;
import com.android.ims.internal.IImsService;
import com.android.ims.internal.IImsUt;
import com.android.ims.internal.IImsConfig;
import com.android.ims.internal.ImsCallSession;
import com.android.ims.internal.ImsManagerEx;
import com.android.internal.telephony.uicc.UiccController;
import com.android.internal.telephony.uicc.IccRecords;
import com.android.internal.telephony.DctConstants;
import com.android.internal.util.ArrayUtils;
import com.spreadtrum.ims.ImsService;
import com.spreadtrum.ims.data.ApnUtils;
import com.android.internal.telephony.VolteConfig;
import android.text.TextUtils;
import com.spreadtrum.ims.vowifi.VoWifiServiceImpl;
import com.spreadtrum.ims.vowifi.VoWifiServiceImpl.CallRatState;

import com.android.ims.internal.IImsFeatureStatusCallback;
import com.android.ims.internal.IImsMultiEndpoint;
import com.android.ims.internal.IImsServiceListenerEx;
import vendor.sprd.hardware.radio.V1_0.ImsNetworkInfo;
import android.content.ComponentName;
import android.content.ServiceConnection;
import com.android.internal.telephony.dataconnection.DataProfile;

public class ImsServiceImpl extends MMTelFeature {
    private static final String TAG = ImsServiceImpl.class.getSimpleName();
    private static final boolean DBG = true;
    private static final int IMS_CALLING_RTP_TIME_OUT        = 1;

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
        mCi = new ImsRIL(context, phone.getPhoneId(), mPhone.mCi);
        mServiceId = phone.getPhoneId() + 1;
        mImsRegister = new ImsRegister(mPhone, mContext, mCi);
        mImsServiceState = new ImsServiceState(false,IMS_REG_STATE_INACTIVE);
        mImsConfigImpl = new ImsConfigImpl(mCi,context,this,mServiceId); // SPRD: bug805154
        mImsUtImpl = new ImsUtImpl(mCi,context,phone,this);
        com.spreadtrum.ims.vowifi.ImsUtImpl voWifiUtImpl =  mWifiService.getUtInterface();
        mImsUtProxy = new ImsUtProxy(context, mImsUtImpl, voWifiUtImpl, phone);
        mImsEcbmImpl = new ImsEcbmImpl(mCi);
        mHandler = new ImsHandler(mContext.getMainLooper());
        Intent intent = new Intent(ImsManager.ACTION_IMS_SERVICE_UP);
        intent.putExtra(ImsManager.EXTRA_PHONE_ID, phone.getPhoneId());
        mContext.sendStickyBroadcast(intent);
        mContext.sendBroadcast(intent);

        mCi.registerForImsBearerStateChanged(mHandler, EVENT_IMS_PND_STATE_CHANGED, null);
        mCi.registerForImsNetworkStateChanged(mHandler, EVENT_IMS_STATE_CHANGED, null);
        mCi.registerForSrvccStateChanged(mHandler, EVENT_SRVCC_STATE_CHANGED, null);
        mCi.registerImsHandoverStatus(mHandler, EVENT_IMS_HANDOVER_STATE_CHANGED, null);
        mCi.registerImsNetworkInfo(mHandler, EVENT_IMS_NETWORK_INFO_UPDATE, null);
        mCi.registerImsRegAddress(mHandler, EVENT_IMS_REGISTER_ADDRESS_CHANGED, null);
        mCi.registerImsWiFiParam(mHandler, EVENT_IMS_WIFI_PARAM, null);
        Log.i(TAG,"ImsServiceImpl onCreate->phoneId:"+phone.getPhoneId());
        mUiccController = UiccController.getInstance();
        mUiccController.registerForIccChanged(mHandler, DctConstants.EVENT_ICC_CHANGED, null);
        mApnChangeObserver = new ApnChangeObserver();
        mVolteConfig = VolteConfig.getInstance();
        mPhone.registerForServiceStateChanged(mHandler, EVENT_SERVICE_STATE_CHANGED, null);
        mPhone.getContext().getContentResolver().registerContentObserver(
                Telephony.Carriers.CONTENT_URI, true, mApnChangeObserver);
        mCi.registerForRadioStateChanged(mHandler, EVENT_RADIO_STATE_CHANGED, null);//SPRD:add for bug594553
        mCi.getImsRegAddress(mHandler.obtainMessage(EVENT_IMS_GET_IMS_REG_ADDRESS));//SPRD: add for bug739660
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
            if (DBG) Log.i(TAG,"handleMessage msg=" + msg);
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
                        Log.i(TAG,"EVENT_IMS_STATE_CHANGED->mRegState:"+mImsServiceState.mRegState);
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
                                        Log.w(TAG,"handleMessage msg=" + msg.what+" mListener is null!");
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
                    } else {
                        Log.i(TAG,"EVENT_IMS_STATE_CHANGED : ar.exception = "+ar.exception +" ar.result:"+ar.result);
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
                        Log.i(TAG,"EVENT_IMS_STATE_DONE->ar.exception mServiceId:"+mServiceId);
                        mImsServiceState.mImsRegistered = false;
                    }
                    //add for SPRD:Bug 678430
                    Log.i(TAG, "setTelephonyProperty mServiceId:"+mServiceId+"mImsRegistered:"+mImsServiceState.mImsRegistered);
                    Log.i(TAG, "setTelephonyProperty isDualVolte:"+ImsManagerEx.isDualVoLTEActive());
                    TelephonyManager.setTelephonyProperty(mServiceId-1, "gsm.sys.volte.state",
                            mImsServiceState.mImsRegistered ? "1" :"0");
                    notifyRegisterStateChange();
                    Log.i(TAG,"EVENT_IMS_STATE_DONE->mServiceState:" + mImsServiceState.mImsRegistered);
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
                            Log.d(TAG, "EVENT_IMS_REGISTER_ADDRESS_CHANGED psfcsAddr:" + address[1]);
                            mImsPscfAddress = address[1];
                        }
                    }else{
                        Log.e(TAG,"EVENT_IMS_REGISTER_ADDRESS_CHANGED has exception!");
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
                                Log.i(TAG, "Srvcc HANDOVER_COMPLETED : setTelephonyProperty mServiceId = " + mServiceId);
                                TelephonyManager.setTelephonyProperty(mServiceId-1, "gsm.sys.volte.state", "0");
                            }
                            mImsService.notifyImsRegisterState();
                            mImsService.notifySrvccState(mServiceId,mImsServiceState.mSrvccState);
                            Log.i(TAG, "Srvcc state: " + ret[0]);
                        } else {
                            Log.w(TAG, "Srvcc error ret: " + ret);
                        }
                    } else {
                        Log.w(TAG, "Srvcc exception: " + ar.exception);
                    }
                    break;
                case EVENT_SERVICE_STATE_CHANGED:
                    Log.i(TAG,"EVENT_SERVICE_STATE_CHANGED->ServiceStateChange");
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
                    Log.i(TAG,"EVENT_RADIO_STATE_CHANGED->mImsRegistered:" + mImsServiceState.mImsRegistered +"  isRaidoOn=" + mPhone.isRadioOn());
                    if (!mPhone.isRadioOn()) {
                        mImsServiceState.mImsRegistered = false;
                        notifyRegisterStateChange();
                    }
                   break;
                /* @} */
               case EVENT_IMS_HANDOVER_STATE_CHANGED:
                    if (ar != null && ar.exception == null && ar.result != null && ar.result instanceof Integer) {
                        Integer response = (Integer)ar.result;
                        mImsService.onImsHandoverStateChange(true, response.intValue());
                    } else {
                        Log.i(TAG,"EVENT_IMS_HANDOVER_STATE_CHANGED : ar.exception = "+ar.exception +" ar.result:"+ar.result);
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
                    } else {
                        Log.i(TAG,"EVENT_IMS_PND_STATE_CHANGED : ar.exception = "+ar.exception +" ar.result:"+ar.result);
                    }
                    break;
                case EVENT_IMS_NETWORK_INFO_UPDATE:
                    if (ar != null && ar.exception == null && ar.result != null) {
                        ImsNetworkInfo info = (ImsNetworkInfo)ar.result;
                        Log.i(TAG,"EVENT_IMS_NETWORK_INFO_UPDATE->info.mType: " + info.type + "info.mInfo: " + info.info);
                        mImsService.onImsNetworkInfoChange(info.type, info.info);
                    }
                    break;
                /*SPRD: add for bug612670 @{ */
                case EVENT_SET_VOICE_CALL_AVAILABILITY_DONE:
                    if(mPhone.isRadioAvailable() && ar.exception != null && ar.userObj != null){
                        Log.i(TAG,"EVENT_SET_VOICE_CALL_AVAILABILITY_DONE: exception");
                        //SPRD: Bug 623247
                        int value = ((Integer) ar.userObj).intValue();
                        if (DBG) Log.i(TAG, "value = " + value);
                        /* SPRD: Bug 826164  @{ */
                        ImsManager.getInstance(mContext, mPhone.getPhoneId()).
                            setEnhanced4gLteModeSettingForSlot((value == 1) ? true : false);
                        /*@}*/
                        Toast.makeText(mContext.getApplicationContext(), mContext.getString(R.string.ims_switch_failed), Toast.LENGTH_SHORT).show();
                    }
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
                        Log.i(TAG,"EVENT_SET_VOICE_CALL_AVAILABILITY_DONE:"+mSrvccCapbility);
                    }
                    break;
                case EVENT_IMS_GET_PCSCF_ADDRESS:
                    if (ar != null && ar.exception == null && ar.result != null){
                        mImsPscfAddress = (String)ar.result;
                        Log.i(TAG,"EVENT_IMS_GET_PCSCF_ADDRESS,mImsPscfAddress:"+mImsPscfAddress);
                    }
                    break;
                case EVENT_IMS_GET_IMS_REG_ADDRESS:
                    if (ar != null && ar.exception == null && ar.result != null) {
                        String[] address = (String[]) ar.result;
                        if (address.length >= 2) {
                            setIMSRegAddress(address[0]);
                            mImsPscfAddress = address[1];
                            Log.i(TAG, "EVENT_IMS_GET_IMS_REG_ADDRESS,mImsPscfAddress:" + mImsPscfAddress);
                        }
                    }
                    break;
                default:
                    break;
            }
        }
    };

    @Override
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
    public void endSession(int sessionId) {
        Log.w(TAG,"endSession->sessionId:"+sessionId);
    }

    @Override
    public boolean isConnected(int callSessionType, int callType) {
        return false;
    }

    @Override
    public boolean isOpened() {
        return true;
    }

    @Override
    public int getFeatureState() {
        return ImsFeature.STATE_READY;
    }

    @Override
    public void addRegistrationListener(IImsRegistrationListener listener) {
        if (listener == null) {
            Log.w(TAG,"addRegistrationListener->Listener is null!");
            Thread.dumpStack();
            return;
        }
        synchronized (mImsRegisterListeners) {
            if (!mImsRegisterListeners.keySet().contains(listener.asBinder())) {
                mImsRegisterListeners.put(listener.asBinder(), listener);
            } else {
                Log.w(TAG,"Listener already add :" + listener);
            }
        }
    }

    @Override
    public void removeRegistrationListener(IImsRegistrationListener listener) {
        if (listener == null) {
            Log.w(TAG,"removeRegistrationListener->Listener is null!");
            Thread.dumpStack();
            return;
        }
        synchronized (mImsRegisterListeners) {
            if (mImsRegisterListeners.keySet().contains(listener.asBinder())) {
                mImsRegisterListeners.remove(listener.asBinder());
            } else {
                Log.w(TAG,"Listener already add :" + listener);
            }
        }
    }

    @Override
    public ImsCallProfile createCallProfile(int sessionId, int callSessionType, int callType) {
        //TODO: callSessionType == callSessionType？
        return new ImsCallProfile(callSessionType,callType);
    }

    @Override
    public IImsCallSession createCallSession(int sessionId, ImsCallProfile profile,
                                             IImsCallSessionListener listener) {
        Log.i(TAG,"createCallSession->profile:" + profile +
                " mImsServiceCallTracker:"+mImsServiceCallTracker);
        if(mImsServiceCallTracker != null){
            return mImsServiceCallTracker.createCallSession(profile, listener);
        } else {
            return null;
        }
    }

    @Override
    public IImsCallSession getPendingCallSession(int sessionId, String callId) {
        Log.i(TAG,"getPendingCallSession->callId:" + callId +
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

    @Override
    public IImsConfig getConfigInterface() {
        return mImsConfigImpl;
    }

    @Override
    public void turnOnIms() {
        Log.i(TAG,"turnOnIms.");
        //add for bug 612670
        mCi.setImsVoiceCallAvailability(1 , mHandler.obtainMessage(EVENT_SET_VOICE_CALL_AVAILABILITY_DONE, ImsConfig.FeatureValueConstants.OFF));
    }

    @Override
    public void turnOffIms() {
        Log.i(TAG,"turnOffIms.");
        //add for bug 612670
        mCi.setImsVoiceCallAvailability(0 , mHandler.obtainMessage(EVENT_SET_VOICE_CALL_AVAILABILITY_DONE, ImsConfig.FeatureValueConstants.ON));
    }

    @Override
    public IImsEcbm getEcbmInterface() {
        return mImsEcbmImpl;
    }

    @Override
    public void setUiTTYMode(int uiTtyMode, Message onComplete) {
    }

    @Override
    public IImsMultiEndpoint getMultiEndpointInterface() {
        return null;
    }

    @Override
    public void onFeatureRemoved() {

    }

    public void  onCreateImsFeature(IImsFeatureStatusCallback c){
        Log.i(TAG,"onCreateImsFeature.");
        mImsFeatureStatusCallback = c;
        try {
            if(mImsFeatureStatusCallback == null){
                Log.w(TAG,"ImsServiceImpl mImsFeatureStatusCallback is null!");
            } else {
                mImsFeatureStatusCallback.notifyImsFeatureStatus(ImsFeature.STATE_READY);
            }
        } catch (RemoteException e){
            e.printStackTrace();
        }
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

    public ImsCallProfile createCallProfile(int serviceType, int callType) {
        return new ImsCallProfile(serviceType,callType);
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
                if (DBG) Log.i(TAG,"Removing stale icc objects.");
                r.unregisterForRecordsLoaded(mHandler);
                mIccRecords.set(null);
            }
            if (newIccRecords != null) {
                if (DBG) Log.i(TAG,"New records found");
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
        if (DBG) Log.i(TAG,"onRecordsLoaded: createAllApnList");
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
            if (DBG) Log.i(TAG,"createAllApnList: selection=" + selection);

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
        ApnSetting initialAttachIMSApnSetting = null;
        for (ApnSetting apn : mAllApnSettings) {
            if (ArrayUtils.contains(apn.types, PhoneConstants.APN_TYPE_IMS)) {
                initialAttachIMSApnSetting = apn;
                break;
            }
        }
        if (initialAttachIMSApnSetting == null) {
            if (DBG) Log.i(TAG,"initialAttachIMSApnSetting: X There in no available ims apn");
        }else {
            if (DBG) Log.i(TAG,"initialAttachIMSApnSetting: X selected ims Apn=" + initialAttachIMSApnSetting);
            mCi.setIMSInitialAttachApn(new DataProfile(initialAttachIMSApnSetting), null);

        }
    }
    /**
     * Handles changes to the APN database.
     */
    private void onApnChanged() {
        if (DBG) Log.i(TAG,"onApnChanged: createAllApnList");
        createAllApnList();
        setInitialAttachIMSApn();
    }

    public int getServiceId(){
        return mServiceId;
    }

    public IImsCallSession getPendingCallSession(String callId){
        Log.i(TAG,"getPendingCallSession->callId:" + callId +
                " mImsServiceCallTracker:"+mImsServiceCallTracker);
        if(mImsServiceCallTracker != null){
            return mImsServiceCallTracker.getCallSession(callId);
        } else {
            return null;
        }
    }

    public void enableImsWhenPDNReady(){
        Log.i(TAG,"enableImsWhenPDNReady.");
        mImsRegister.onImsPDNReady();
        mImsRegister.enableIms();
    }

    public void disableIms(){
        Log.i(TAG,"disableIms.");
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
        Log.i(TAG,"registOperatorNumeric = " + registOperatorNumeric);
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
        Log.i(TAG, "ImsServiceImpl ==> setVideoResolution mDefaultVtResolution = " + operatorCameraResolution);
        mImsConfigImpl.setVideoQualitytoPreference(Integer.parseInt(operatorCameraResolution));
    }

    /* SPRD: 630048 add sos apn for yes 4G @{*/
    public void setInitialAttachSosApn(ServiceState state){
        String carrier = state.getOperatorNumeric();
        if (carrier != null && !carrier.isEmpty()) {
            String apn = mVolteConfig.getApn(carrier);
            if (apn != null && !apn.isEmpty()) {
                if (DBG) Log.i(TAG,"SosApn: apn=" + apn + ", set sos apn = " + mSetSosApn);
                if (mSetSosApn) {
                    mSetSosApn = false;
                    String[] emergency = {"emergency"};
                    ApnSetting sosApn = new ApnSetting(0, carrier, "", apn, "", "", "", "", "", "", "", 0,
                            emergency, "IPV4V6", "IPV4V6", true, 0, 0, 0, false, 0, 0, 0, 0, "", "");

                    mCi.setInitialAttachSOSApn(new DataProfile(sosApn), null);
                }
            }
        } else {
            if (DBG) Log.i(TAG,"carrier is null");
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
            Log.w(TAG,"addListener-> listener is null!");
            return;
        }
        if (!mListeners.contains(listener)) {
            mListeners.add(listener);
        } else {
            Log.w(TAG,"addListener-> listener already add!");
        }
    }

    public void removeListener(Listener listener){
        if (listener == null) {
            Log.w(TAG,"removeListener-> listener is null!");
            return;
        }
        if (mListeners.contains(listener)) {
            mListeners.remove(listener);
        } else {
            Log.w(TAG,"addListener-> listener already remove!");
        }
    }
    //SPRD: add for bug 771875
    public void updateImsFeature(int feature, int value) {
        Log.i(TAG, "updateImsFeatures->feature:" + feature + " value:" + value);
        updateImsFeatures(mImsService.isVoLTEEnabled(), mImsService.isVoWifiEnabled());
    }

    public void notifyRegisterStateChange() {
        // SPRD Add for DSDA bug684926:
        // If dual volte active, update RAT to 4G and voice reg state to in service.
        // And if dual volte not active, service state need to be set to correct state.
        int phoneCount = TelephonyManager.from(mContext).getPhoneCount();
        if(phoneCount > 1 && mPhone.getPhoneId() != getImsRegister().getPrimaryCard(phoneCount)) {
            Log.d(TAG, "Ims Register State Changed, poll state again on vice SIM,"
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

    public void notifyImsRegister(boolean isRegistered){
        try{
            // SPRD: 730973
            if(isRegistered){
                mVolteRegisterStateOld = true;
            } else {
                mVolteRegisterStateOld = false;
            }
            if(mListener == null){
                Log.w(TAG,"notifyImsRegister->mListener is null!");
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
            Log.i(TAG,"notifyImsRegister->isRegistered:" + isRegistered
                    + " isImsEnabled():"+mImsService.isImsEnabled()
                    + " mImsService.isVoLTEEnabled():"+mImsService.isVoLTEEnabled()
                    + " mImsService.isVoWifiEnabled():"+mImsService.isVoWifiEnabled());
            Log.i(TAG,"notifyImsRegister->mServiceState:" + isRegistered);
            mImsRegister.notifyImsStateChanged(isRegistered);
        } catch (RemoteException e){
            e.printStackTrace();
        }
    }

    public void sendIncomingCallIntent(String callId) {
        try {
            Intent intent = new Intent();
            intent.putExtra(ImsManager.EXTRA_CALL_ID, callId);
            intent.putExtra(ImsManager.EXTRA_USSD, false);
            intent.putExtra(ImsManager.EXTRA_SERVICE_ID, mServiceId);
            /*SPRD: Modify for bug586758{@*/
            Log.i (TAG,"sendIncomingCallIntent-> startVoWifiCall"
                    + " mIsVowifiCall: " + mImsService.isVowifiCall()
                    + " mIsVolteCall: " + mImsService.isVolteCall()
                    + " isVoWifiEnabled(): " + mImsService.isVoWifiEnabled()
                    + " isVoLTEEnabled(): " + mImsService.isVoLTEEnabled());
            if (mImsService.isVoWifiEnabled() && !mImsService.isVowifiCall() && !mImsService.isVolteCall()) {
                mWifiService.updateCallRatState(CallRatState.CALL_VOWIFI);
            }
            /*@}*/
            mIncomingCallIntent.send(mContext, ImsManager.INCOMING_CALL_RESULT_CODE,intent);
        } catch (PendingIntent.CanceledException e) {
            Log.e(TAG, "PendingIntent Canceled " + e);
        }
    }

    public String getIMSRegAddress() {
        if(DBG){
            Log.i(TAG, "getIMSRegAddress mImsRegAddress = " + mImsRegAddress);
        }
        return mImsRegAddress;
    }

    public String getImsPcscfAddress(){
        if(DBG){
            Log.i(TAG, "getImsPcscfAddress mImsPscfAddress = " + mImsPscfAddress);
        }
        return mImsPscfAddress;
    }

    public void setIMSRegAddress(String addr) {
        if(DBG){
            Log.i(TAG, "setIMSRegAddress addr = " + addr);
        }
        mImsRegAddress = addr;
        mWifiService.setUsedLocalAddr(addr);
    }
    public void requestImsHandover(int type){
        Log.i(TAG,"requestImsHandover->type:" + type);
        mCi.requestImsHandover(type,mHandler.obtainMessage(EVENT_IMS_HANDOVER_ACTION_COMPLETE));
    }

    public void notifyImsHandoverStatus(int status){
        Log.i(TAG,"notifyImsHandoverStatus->status:" + status);
        mCi.notifyImsHandoverStatus(status,null);
    }

    public void notifyImsCallEnd(int type){
        Log.i(TAG,"notifyImsCallEnd.");
        mCi.notifyImsCallEnd(type,null);
    }

    public void notifyVoWifiEnable(boolean enable){
        Log.i(TAG,"notifyVoWifiEnable.");
        mCi.notifyVoWifiEnable(enable,null);
    }
    /*SPRD: Add for get network info{@*/
    public void notifyImsNetworkInfo(int type, String info){
        Log.i(TAG,"notifyImsNetworkInfo->type:"+type+" info:"+info);
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
        Log.i(TAG,"notifyDataRouter");
        mCi.notifyDataRouter(null);
    }
    /*@}*/
    /*SPRD: Add for bug586758{@*/
    public boolean isVolteSessionListEmpty() {
        if (mImsServiceCallTracker != null) {
            Log.i(TAG,"isSessionListEmpty: " + mImsServiceCallTracker.isSessionListEmpty());
            return mImsServiceCallTracker.isSessionListEmpty();
        }
        return false;
    }
    public boolean isVowifiSessionListEmpty() {
        if (mWifiService != null) {
            Log.i(TAG,"mWifiService.getCallCount(): " + mWifiService.getCallCount());
            return mWifiService.getCallCount()==0;
        }
        return false;
    }
    /*@}*/

    public void onWifiParamEvent(Object object){
        AsyncResult ar =(AsyncResult)object;
        int resultArray[] = (int[]) ar.result;
        Log.i(TAG,"onWifiParamEvent->rtp_time_Out:" + resultArray[3]);
        mAliveCallLose = resultArray[1];
        mAliveCallJitter = resultArray[2];
        mAliveCallRtt = resultArray[0];
        IImsServiceListenerEx imsServiceListenerEx = mImsService.getImsServiceListenerEx();
        try {
            if (imsServiceListenerEx != null) {
                Log.i(TAG,"onWifiParamEvent->onMediaQualityChanged->isvideo:false"
                        + " mAliveCallLose:" + mAliveCallLose + " mAliveCallJitter:" + mAliveCallJitter
                        + " mAliveCallRtt:" + mAliveCallRtt);
                imsServiceListenerEx.onMediaQualityChanged(false,mAliveCallLose,mAliveCallJitter,mAliveCallRtt);
                if (resultArray[3] == IMS_CALLING_RTP_TIME_OUT) {
                    Log.i(TAG,"onWifiParamEvent->onRtpReceived->isvideo:false");
                    imsServiceListenerEx.onNoRtpReceived(false);
                }
            } else {
                Log.i(TAG,"onWifiParamEvent->imsServiceListenerEx is null");
            }
        } catch(RemoteException e){
            e.printStackTrace();
        }
    }

    public int getAliveCallLose() {
        Log.i(TAG,"getAliveCallLose->mAliveCallLose:" + mAliveCallLose);
        return mAliveCallLose;
    }

    public int getAliveCallJitter() {
        Log.i(TAG,"getAliveCallJitter->mAliveCallJitter:" + mAliveCallJitter);
        return mAliveCallJitter;
    }

    public int getAliveCallRtt() {
        Log.i(TAG,"getAliveCallRtt->mAliveCallRtt:" + mAliveCallRtt);
        return mAliveCallRtt;
    }

    public void updateImsFeatures(boolean volteEnable, boolean wifiEnable){
        Log.i(TAG,"updateImsFeatures->volteEnable:" + volteEnable + " wifiEnable:" + wifiEnable+" id:"+mServiceId);
        try{
            if(volteEnable){
                mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE]
                        = ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE;
                if(ImsManager.getInstance(mContext,mPhone.getPhoneId()).isVtEnabledByUserForSlot() && ImsManager.isVtEnabledByPlatform(mContext)){//SPRD:modify for bug805161
                mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_LTE]
                        = ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_LTE;
                }else{
                    mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_LTE]
                            = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
                }
                mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_UT_OVER_LTE]
                        = ImsConfig.FeatureConstants.FEATURE_TYPE_UT_OVER_LTE;
            } else {
                mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE]
                        = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
                mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_LTE]
                        = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
                mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_UT_OVER_LTE]
                        = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
            }
            if(wifiEnable){
                mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI]
                        = ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI;
                if(ImsManager.getInstance(mContext,mPhone.getPhoneId()).isVtEnabledByUserForSlot() && ImsManager.isVtEnabledByPlatform(mContext)){//SPRD:modify for bug810321
                    mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_WIFI]
                            = ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_WIFI;
                }else{
                    mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_WIFI]
                            = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
                }
                mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_UT_OVER_WIFI]
                        = ImsConfig.FeatureConstants.FEATURE_TYPE_UT_OVER_WIFI;
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
                            ImsServiceClass.MMTEL,mEnabledFeatures, mDisabledFeatures);
                }
            }

            //wifi capability caused by a handover for bug837323 { */
            updateImsCallProfile(wifiEnable);

            if(mListener == null){
                Log.w(TAG,"updateImsFeatures mListener is null!");
                return;
            }
            mListener.registrationFeatureCapabilityChanged(
                    ImsServiceClass.MMTEL,mEnabledFeatures, mDisabledFeatures);

        } catch (RemoteException e){
            e.printStackTrace();
        }
    }

    /* SPRD: wifi capability caused by a handover for bug837323 { */
    public void updateImsCallProfile(boolean wifiEnable){
        if(mImsServiceCallTracker != null){
            mImsServiceCallTracker.updateImsCallProfile(wifiEnable);
            Log.i(TAG," updateImsCallProfile->ok");
        }else{
            Log.i(TAG," updateImsCallProfile->mImsServiceCallTracker is null");
        }
    }
    /*@}*/

    public int getVolteRegisterState() {
        Log.i(TAG,"getVolteRegisterState->VolteRegisterState:" + mImsServiceState.mRegState);
        return mImsServiceState.mRegState;
    }
    public void terminateVolteCall(){
        if(mImsServiceCallTracker != null){
            mImsServiceCallTracker.terminateVolteCall();
            Log.i(TAG," terminateVolteCall->ok");
        }else{
            Log.i(TAG," terminateVolteCall->mImsServiceCallTracker is null");
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
        Log.d(TAG, "setImsPcscfAddress addr = " + addr);
        String pcscfAdd = "";
        if (addr != null && addr.length() != 0) {

            if (addr.matches("\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}")) {
                pcscfAdd = "1,\"" + addr + "\"";
            } else if (addr.contains(":")) {
                pcscfAdd = "2,\"" + addr + "\"";
            }

            Log.d(TAG, "setImsPcscfAddress pcscfAdd = " + pcscfAdd);
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
            Log.w(TAG,"addImsPdnStateListener->Listener is null!");
            Thread.dumpStack();
            return;
        }
        synchronized (mImsPdnStateListeners) {
            if (!mImsPdnStateListeners.keySet().contains(listener.asBinder())) {
                mImsPdnStateListeners.put(listener.asBinder(), listener);
            } else {
                Log.w(TAG,"addImsPdnStateListener Listener already add :" + listener);
            }
        }
    }

    /**
     * Used for remove IMS PDN State Listener.
     */
    public void removeImsPdnStateListener(IImsPdnStateListener listener){
        if (listener == null) {
            Log.w(TAG,"removeImsPdnStateListener->Listener is null!");
            Thread.dumpStack();
            return;
        }
        synchronized (mImsPdnStateListeners) {
            if (mImsPdnStateListeners.keySet().contains(listener.asBinder())) {
                mImsPdnStateListeners.remove(listener.asBinder());
            } else {
                Log.w(TAG,"removeImsPdnStateListener Listener already add :" + listener);
            }
        }
    }

    public boolean isImsEnabled(){
        return ((mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE]
                == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE) ||
                (mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI]
                == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI));
    }

    public void updateImsFeatureForAllService(){
        mImsService.updateImsFeatureForAllService();
    }

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
}

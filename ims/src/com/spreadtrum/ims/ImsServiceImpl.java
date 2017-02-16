package com.spreadtrum.ims;

import java.util.ArrayList;
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
import android.util.Log;
import android.widget.Toast;

import android.provider.Telephony;
import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.IccUtils;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.PhoneFactory;
import com.android.internal.telephony.dataconnection.ApnSetting;
import com.android.internal.telephony.gsm.GSMPhone;
import com.android.internal.telephony.PhoneProxy;
import com.android.ims.ImsException;
import com.android.ims.ImsManager;
import com.android.ims.ImsReasonInfo;
import com.android.ims.ImsServiceClass;
import com.android.ims.ImsCallProfile;
import com.android.ims.ImsConfig;
import com.android.ims.internal.IImsCallSession;
import com.android.ims.internal.IImsCallSessionListener;
import com.android.ims.internal.IImsRegistrationListener;
import com.android.ims.internal.IImsEcbm;
import com.android.ims.internal.IImsService;
import com.android.ims.internal.IImsUt;
import com.android.ims.internal.IImsConfig;
import com.android.ims.internal.ImsCallSession;
import com.android.internal.telephony.uicc.UiccController;
import com.android.internal.telephony.uicc.IccRecords;
import com.android.internal.telephony.DctConstants;
import com.android.internal.util.ArrayUtils;
import com.spreadtrum.ims.ImsService;
import com.spreadtrum.ims.data.ApnUtils;
import com.spreadtrum.ims.vowifi.VoWifiServiceImpl;
import com.spreadtrum.ims.vowifi.VoWifiServiceImpl.DataRouterState;
import com.android.internal.telephony.ImsNetworkInfo;

import com.android.ims.internal.IImsServiceListenerEx;
import com.android.sprd.telephony.RadioInteractor;
import com.android.sprd.telephony.RadioInteractorCallbackListener;
import android.content.ComponentName;
import android.content.ServiceConnection;


public class ImsServiceImpl {
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
    protected static final int EVENT_IMS_REGISTER_ADDRESS_CHANGED      = 105;
    protected static final int EVENT_IMS_HANDOVER_STATE_CHANGED        = 106;
    protected static final int EVENT_IMS_HANDOVER_ACTION_COMPLETE      = 107;
    protected static final int EVENT_IMS_PND_STATE_CHANGED             = 108;
    protected static final int EVENT_IMS_NETWORK_INFO_UPDATE           = 109;
    protected static final int EVENT_SET_VOICE_CALL_AVAILABILITY_DONE  = 110;

    private GSMPhone mPhone;
    private ImsServiceState mImsServiceState;
    private int mServiceClass = ImsServiceClass.MMTEL;
    private int mServiceId; 
    private PendingIntent mIncomingCallIntent;
    private IImsRegistrationListener mListener;
    private Context mContext;
    private CommandsInterface mCi;
    private ImsConfigImpl mImsConfigImpl;
    private ImsUtImpl mImsUtImpl;
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
    private VoWifiServiceImpl mWifiService;//Add for data router
    private String mImsRegAddress = "";
    private RadioInteractor mRadioInteractor;
    private RadioInteractorCallbackListener mRadioInteractorCallbackListener;
    private int mAliveCallLose = -1;
    private int mAliveCallJitter = -1;
    private int mAliveCallRtt = -1;
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

    //"VoLTE", "ViLTE", "VoWiFi", "ViWiFi"
    private int[] mEnabledFeatures = {
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
            ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN
    };


    public ImsServiceImpl(Phone phone , Context context, VoWifiServiceImpl wifiService){
        mImsService = (ImsService)context;
        mWifiService = wifiService;//Add for data router
        PhoneProxy proxy = (PhoneProxy)phone;
        mPhone = (GSMPhone)proxy.getActivePhone();
        mContext = context;
        mCi = mPhone.mCi;
        mServiceId = phone.getPhoneId() + 1;
        mImsRegister = new ImsRegister(mPhone, mContext, mCi);
        mImsServiceState = new ImsServiceState(false,IMS_REG_STATE_INACTIVE);
        mImsConfigImpl = new ImsConfigImpl(mCi,context);
        mImsUtImpl = new ImsUtImpl(mCi,context);
        mImsEcbmImpl = new ImsEcbmImpl(mCi);
        mHandler = new ImsHandler(mContext.getMainLooper());
        Intent intent = new Intent(ImsManager.ACTION_IMS_SERVICE_UP);
        intent.putExtra(ImsManager.EXTRA_PHONE_ID, phone.getPhoneId());
        mContext.sendStickyBroadcast(intent);
        mContext.sendBroadcast(intent);

        mCi.registerForConnImsen(mHandler, EVENT_IMS_PND_STATE_CHANGED, null);
        mCi.registerForImsNetworkStateChanged(mHandler, EVENT_IMS_STATE_CHANGED, null);
        mCi.registerImsHandoverStatus(mHandler, EVENT_IMS_HANDOVER_STATE_CHANGED, null);
        mCi.registerImsNetworkInfo(mHandler, EVENT_IMS_NETWORK_INFO_UPDATE, null);
        Log.i(TAG,"ImsServiceImpl onCreate->phoneId:"+phone.getPhoneId());
        mUiccController = UiccController.getInstance();
        mUiccController.registerForIccChanged(mHandler, DctConstants.EVENT_ICC_CHANGED, null);
        mApnChangeObserver = new ApnChangeObserver();
        mPhone.getContext().getContentResolver().registerContentObserver(
                Telephony.Carriers.CONTENT_URI, true, mApnChangeObserver);
        mCi.registerImsRegAddress(mHandler, EVENT_IMS_REGISTER_ADDRESS_CHANGED, null);
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
                    if (ar.exception == null && ar.result != null) {
                        int[] responseArray = (int[])ar.result;
                        mImsServiceState.mRegState = responseArray[0];
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
                                } catch (RemoteException e){
                                    e.printStackTrace();
                                }
                                break;
                            default:
                                break;
                        }
                    }
                    break;
                case EVENT_IMS_STATE_DONE:
                    if (ar.exception == null && ar.result != null) {
                        int[] responseArray = (int[])ar.result;
                        if(responseArray != null && responseArray.length >1){
                            mImsServiceState.mImsRegistered = (responseArray[0] != 0 && responseArray[1]== 1);
                        }
                        notifyRegisterStateChange();
                        if(mListener == null){
                            Log.w(TAG,"handleMessage msg=" + msg.what+" mListener is null!");
                            break;
                        }
                        Log.i(TAG,"EVENT_IMS_STATE_DONE->mImsRegistered:" + mImsServiceState.mImsRegistered);
                    } else {
                        mImsServiceState.mImsRegistered = false;
                        Log.i(TAG,"EVENT_IMS_STATE_DONE: error");
                    }
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
                case IMS_REG_STATE_REGISTERED:
                  //TODO :for test
                    try{
                        mListener.registrationConnected();
                    } catch (RemoteException e){
                        e.printStackTrace();
                    }
                    break;
                case EVENT_IMS_REGISTER_ADDRESS_CHANGED:
                    if(ar.exception == null && ar.result != null){
                         if (ar.result instanceof String) {
                             String addr = (String)ar.result;
                             setIMSRegAddress(addr);
                         }
                    }else{
                        Log.e(TAG,"EVENT_IMS_REGISTER_ADDRESS_CHANGED has exception!");
                    }
                    break;
                case EVENT_IMS_HANDOVER_STATE_CHANGED:
                    if (ar != null && ar.exception == null && ar.result != null) {
                        int[] responseArray = (int[])ar.result;
                        mImsService.onImsHandoverStateChange(true, responseArray[0]);
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
                    if (ar != null && ar.exception == null && ar.result != null) {
                        int[] responseArray = (int[])ar.result;
                        mImsService.onImsPdnStatusChange(mServiceId,responseArray[0]);
                    }
                    break;
                case EVENT_IMS_NETWORK_INFO_UPDATE:
                    if (ar != null && ar.exception == null && ar.result != null) {
                        ImsNetworkInfo info = (ImsNetworkInfo)ar.result;
                        Log.i(TAG,"EVENT_IMS_NETWORK_INFO_UPDATE->info.mType: " + info.mType + "info.mInfo: " + info.mInfo);
                        mImsService.onImsNetworkInfoChange(info.mType, info.mInfo);
                    }
                    break;
                case EVENT_SET_VOICE_CALL_AVAILABILITY_DONE:
                    if(ar.exception != null){
                        Log.i(TAG,"EVENT_SET_VOICE_CALL_AVAILABILITY_DONE: exception");
                        Toast.makeText(mContext.getApplicationContext(), mContext.getString(R.string.ims_switch_failed), Toast.LENGTH_SHORT).show();
                    }
                    break;
                default:
                    break;
            }
        }
    };

    public int open(int serviceClass, PendingIntent incomingCallIntent,
            IImsRegistrationListener listener){
        mServiceClass = serviceClass;
        mIncomingCallIntent = incomingCallIntent;
        mListener = listener;
        mImsServiceCallTracker = new ImsServiceCallTracker(
                mContext, mCi, mIncomingCallIntent, mServiceId, this, mWifiService);
        SessionListListener listListener = new SessionListListener();
        mImsServiceCallTracker.addListener(listListener);
        try{
            mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE]
                    = ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE;
            mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_LTE]
                    = ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_LTE;
            mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI]
                    = ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI;
            mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_WIFI]
                    = ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_WIFI;
            mListener.registrationFeatureCapabilityChanged(
                    ImsServiceClass.MMTEL,mEnabledFeatures, mDisabledFeatures);
        } catch (RemoteException e){
            e.printStackTrace();
        }
        return mServiceId;
    }

    public void close(){

    }

    public boolean isOpened(){
        return true;
    }

    public void setRegistrationListener(IImsRegistrationListener listener){
        mListener = listener;
    }

    public ImsCallProfile createCallProfile(int serviceType, int callType) {
        return new ImsCallProfile(serviceType,callType);
    }

    public void setUiTTYMode(int serviceId, int uiTtyMode, Message onComplete) {

    }

    public IImsConfig getConfigInterface(){
        return mImsConfigImpl;
    }

    public IImsUt getUtInterface(){
        return mImsUtImpl;
    }

    public ImsUtImpl getUtImpl(){
        return mImsUtImpl;
    }

    public IImsEcbm getEcbmInterface() {
        return mImsEcbmImpl;
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
        mRadioInteractor = new RadioInteractor(mPhone.getContext());
        mContext.bindService(
                new
                Intent("com.android.sprd.telephony.server.RADIOINTERACTOR_SERVICE")
                .setPackage("com.android.sprd.telephony.server"),
                new ServiceConnection() {
                    @Override
                    public void onServiceConnected(ComponentName name,
                            IBinder service) {
                        Log.d(TAG, "on radioInteractor service connected");
                        mRadioInteractorCallbackListener = getRadioInteractorCallbackListener(mPhone.getPhoneId());
                        mRadioInteractor.listen(mRadioInteractorCallbackListener,
                                RadioInteractorCallbackListener.LISTEN_WIFIPARAM_EVENT, false);
                    }
                    @Override
                    public void onServiceDisconnected(ComponentName name) {
                        mRadioInteractor.listen(mRadioInteractorCallbackListener,
                                RadioInteractorCallbackListener.LISTEN_NONE);
                    }
                }, Context.BIND_AUTO_CREATE);
        if (DBG) Log.i(TAG,"onRecordsLoaded: createAllApnList");
        createAllApnList();
        if(ImsManager.isVolteEnabledByPlatform(mPhone.getContext())){
            setInitialAttachIMSApn();
        }
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
            mCi.setInitialAttachIMSApn(initialAttachIMSApnSetting.apn, initialAttachIMSApnSetting.protocol,
                initialAttachIMSApnSetting.authType, initialAttachIMSApnSetting.user,
                initialAttachIMSApnSetting.password, null);
        }
    }
    /**
     * Handles changes to the APN database.
     */
    private void onApnChanged() {
        if (DBG) Log.i(TAG,"onApnChanged: createAllApnList");
        createAllApnList();
        if(ImsManager.isVolteProvisionedOnDevice(mPhone.getContext())){
            setInitialAttachIMSApn();
        }
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

    public IImsCallSession createCallSession(int serviceId, ImsCallProfile profile,
            IImsCallSessionListener listener){
        Log.i(TAG,"createCallSession->profile:" + profile +
                " mImsServiceCallTracker:"+mImsServiceCallTracker);
        if(mImsServiceCallTracker != null){
            return mImsServiceCallTracker.createCallSession(profile, listener);
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

    public void turnOnIms(){
        Log.i(TAG,"turnOnIms.");
        mCi.setImsVoiceCallAvailability(1 , mHandler.obtainMessage(EVENT_SET_VOICE_CALL_AVAILABILITY_DONE));
    }

    public void turnOffIms(){
        Log.i(TAG,"turnOffIms.");
        mCi.setImsVoiceCallAvailability(0 , mHandler.obtainMessage(EVENT_SET_VOICE_CALL_AVAILABILITY_DONE));
    }

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

    public void notifyRegisterStateChange() {
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
            if(mListener == null){
                Log.w(TAG,"notifyImsRegister->mListener is null!");
                return;
            }
            if(isRegistered){
                mListener.registrationConnected();
            } else {
                mListener.registrationDisconnected(new ImsReasonInfo());
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
                mWifiService.updateDataRouterState(DataRouterState.CALL_VOWIFI);
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

    public void setIMSRegAddress(String addr) {
        if(DBG){
            Log.i(TAG, "setIMSRegAddress addr = " + addr);
        }
        mImsRegAddress = addr;
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
        mCi.notifyWifiCallState(inCall,null);
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

    private RadioInteractorCallbackListener getRadioInteractorCallbackListener(final int phoneId) {
        return new RadioInteractorCallbackListener(phoneId){
            @Override
            public void onWifiParamEvent(Object object){
                AsyncResult ar =(AsyncResult)object;
                int resultArray[] = (int[]) ar.result;
                Log.i(TAG,"getRadioInteractorCallbackListener->onWifiParamEvent->rtp_time_Out:" + resultArray[3]);
                mAliveCallLose = resultArray[1];
                mAliveCallJitter = resultArray[2];
                mAliveCallRtt = resultArray[0];
                IImsServiceListenerEx imsServiceListenerEx = mImsService.getImsServiceListenerEx();
                try {
                    if (imsServiceListenerEx != null) {
                        Log.i(TAG,"getRadioInteractorCallbackListener->onWifiParamEvent->onMediaQualityChanged->isvideo:false"
                                    + " mAliveCallLose:" + mAliveCallLose + " mAliveCallJitter:" + mAliveCallJitter
                                    + " mAliveCallRtt:" + mAliveCallRtt);
                        imsServiceListenerEx.onMediaQualityChanged(false,mAliveCallLose,mAliveCallJitter,mAliveCallRtt);
                        if (resultArray[3] == IMS_CALLING_RTP_TIME_OUT) {
                            Log.i(TAG,"getRadioInteractorCallbackListener->onWifiParamEvent->onRtpReceived->isvideo:false");
                            imsServiceListenerEx.onNoRtpReceived(false);
                        }
                    } else {
                        Log.i(TAG,"getRadioInteractorCallbackListener->imsServiceListenerEx is null");
                    }
                } catch(RemoteException e){
                    e.printStackTrace();
                }
            }
        };
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
        Log.i(TAG,"updateImsFeatures->volteEnable:" + volteEnable + " wifiEnable:" + wifiEnable);
        try{
            if(volteEnable){
                mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE]
                        = ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE;
                mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_LTE]
                        = ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_LTE;
            } else {
                mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE]
                        = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
                mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_LTE]
                        = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
            }
            if(wifiEnable){
                mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI]
                        = ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI;
                mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_WIFI]
                        = ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_WIFI;
            } else {
                mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI]
                        = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
                mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_WIFI]
                        = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
            }
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
}

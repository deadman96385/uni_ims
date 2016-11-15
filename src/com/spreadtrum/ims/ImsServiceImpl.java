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
import android.telephony.VoLteServiceState;
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
import com.android.ims.internal.IImsCallSession;
import com.android.ims.internal.IImsCallSessionListener;
import com.android.ims.internal.IImsRegistrationListener;
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
import com.spreadtrum.ims.data.ApnUtils;
import com.android.internal.telephony.VolteConfig;
import android.text.TextUtils;

public class ImsServiceImpl {
    private static final String TAG = ImsServiceImpl.class.getSimpleName();
    private static final boolean DBG = true;

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

    private GsmCdmaPhone mPhone;
    private ImsServiceState mImsServiceState;
    private int mServiceClass = ImsServiceClass.MMTEL;
    private int mServiceId; 
    private PendingIntent mIncomingCallIntent;
    private IImsRegistrationListener mListener;
    private Context mContext;
    private ImsRIL mCi;
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
    private ImsService mImsService;
    private VolteConfig mVolteConfig;

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


    public ImsServiceImpl(Phone phone , Context context){
        mPhone = (GsmCdmaPhone)phone;
        mContext = context;
        mImsService = (ImsService)context;
        mCi = new ImsRIL(context, phone.getPhoneId(), mPhone.mCi);
        mServiceId = phone.getPhoneId() + 1;
        mImsRegister = new ImsRegister(mPhone, mContext, mCi);
        mImsServiceState = new ImsServiceState(false,IMS_REG_STATE_INACTIVE);
        mImsConfigImpl = new ImsConfigImpl(mCi,context);
        mImsUtImpl = new ImsUtImpl(mCi,context,phone);
        mImsEcbmImpl = new ImsEcbmImpl(mCi);
        mHandler = new ImsHandler(mContext.getMainLooper());
        Intent intent = new Intent(ImsManager.ACTION_IMS_SERVICE_UP);
        intent.putExtra(ImsManager.EXTRA_PHONE_ID, phone.getPhoneId());
        mContext.sendStickyBroadcast(intent);
        mContext.sendBroadcast(intent);

        mCi.registerForImsNetworkStateChanged(mHandler, EVENT_IMS_STATE_CHANGED, null);
        mCi.registerForSrvccStateChanged(mHandler, EVENT_SRVCC_STATE_CHANGED, null);
        Log.i(TAG,"ImsServiceImpl onCreate->phoneId:"+phone.getPhoneId());
        mUiccController = UiccController.getInstance();
        mUiccController.registerForIccChanged(mHandler, DctConstants.EVENT_ICC_CHANGED, null);
        mApnChangeObserver = new ApnChangeObserver();
        mVolteConfig = VolteConfig.getInstance();
        mPhone.registerForServiceStateChanged(mHandler, EVENT_SERVICE_STATE_CHANGED, null);
        mPhone.getContext().getContentResolver().registerContentObserver(
                Telephony.Carriers.CONTENT_URI, true, mApnChangeObserver);
        mCi.registerForRadioStateChanged(mHandler, EVENT_RADIO_STATE_CHANGED, null);//SPRD:add for bug594553
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
                        try{
                            if(mListener == null){
                                Log.w(TAG,"handleMessage msg=" + msg.what+" mListener is null!");
                                break;
                            }
                            if(mImsServiceState.mImsRegistered){
                                mListener.registrationConnected();
                            } else {
                                mListener.registrationDisconnected(new ImsReasonInfo());
                            }
                        } catch (RemoteException e){
                            e.printStackTrace();
                        }
                        Log.i(TAG,"EVENT_IMS_STATE_CHANGED->mServiceState:" + mImsServiceState.mImsRegistered);
                        mImsRegister.notifyImsStateChanged(mImsServiceState.mImsRegistered);
                        mImsService.notifyImsRegisterState(mServiceId-1, getImsRegisterState());
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
                case EVENT_SRVCC_STATE_CHANGED:
                    if (ar.exception == null) {
                        int[] ret = (int[]) ar.result;
                        if(ret != null && ret.length != 0){
                            mImsServiceState.mSrvccState = ret[0];
                            mImsService.notifyImsSrvccState(mServiceId-1, ret[0]);
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
                    break;
                /* SPRD: add for bug594553 @{ */
                case EVENT_RADIO_STATE_CHANGED:
                    Log.i(TAG,"EVENT_RADIO_STATE_CHANGED->mImsRegistered:" + mImsServiceState.mImsRegistered +"  isRaidoOn=" + mPhone.isRadioOn());
                    if (!mPhone.isRadioOn()) {
                        mImsServiceState.mImsRegistered = false;
                        mImsService.notifyImsRegisterState(mServiceId-1, getImsRegisterState());
                    }
                   break;
                /* @} */
                /*SPRD: add for bug612670 @{ */
                case EVENT_SET_VOICE_CALL_AVAILABILITY_DONE:
                    if(mPhone.isRadioAvailable() && ar.exception != null){
                        Log.i(TAG,"EVENT_SET_VOICE_CALL_AVAILABILITY_DONE: exception");
                        Toast.makeText(mContext.getApplicationContext(), mContext.getString(R.string.ims_switch_failed), Toast.LENGTH_SHORT).show();
                    }
                    break;
                /*@}*/
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
        mImsServiceCallTracker = new ImsServiceCallTracker(mContext,mCi,mIncomingCallIntent,mServiceId,this);
        try{
            mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE]
                    = ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE;
            mEnabledFeatures[ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_LTE]
                    = ImsConfig.FeatureConstants.FEATURE_TYPE_VIDEO_OVER_LTE;
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

    public void turnOnIms(){
        Log.i(TAG,"turnOnIms.");
        //add for bug 612670
        mCi.setImsVoiceCallAvailability(1 , mHandler.obtainMessage(EVENT_SET_VOICE_CALL_AVAILABILITY_DONE));
    }

    public void turnOffIms(){
        Log.i(TAG,"turnOffIms.");
        //add for bug 612670
        mCi.setImsVoiceCallAvailability(0 , mHandler.obtainMessage(EVENT_SET_VOICE_CALL_AVAILABILITY_DONE));
    }

    public int getImsRegisterState(){
        if(mImsServiceState.mImsRegistered &&
                mImsServiceState.mSrvccState != VoLteServiceState.HANDOVER_STARTED){
            return ImsManagerEx.IMS_REGISTERED;
        } else {
            return ImsManagerEx.IMS_UNREGISTERED;
        }
    }

    public boolean isImsRegisterState(){
        if(mImsServiceState.mImsRegistered &&
                mImsServiceState.mSrvccState != VoLteServiceState.HANDOVER_STARTED){
            return true;
        } else {
            return false;
        }
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
    }
}

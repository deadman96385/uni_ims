package com.spreadtrum.ims;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArrayList;
import com.android.ims.ImsUtInterface;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiInfo;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.ServiceManager;
import android.os.SystemProperties;
import android.os.RemoteException;
import android.telephony.PhoneNumberUtils;
import android.telephony.ServiceState;
import android.telephony.TelephonyManager;
import android.telephony.TelephonyManagerEx;
import android.telephony.PhoneStateListener;
import android.telephony.VoLteServiceState;
import android.util.Log;
import android.widget.Toast;
import android.telecom.VideoProfile;
import android.provider.Settings;

import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneFactory;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.CallManager;
import com.android.internal.telephony.Call;

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
import com.android.ims.internal.IImsUtEx;
import com.android.ims.internal.IImsConfig;
import com.android.ims.internal.ImsCallSession;
import com.spreadtrum.ims.ImsCallSessionImpl.Listener;
import com.spreadtrum.ims.vt.VTManagerProxy;
import com.spreadtrum.ims.ut.ImsUtImpl;
import com.spreadtrum.ims.ut.ImsUtProxy;
import com.android.ims.internal.IImsServiceEx;
import com.android.ims.internal.IImsServiceListenerEx;
import com.android.ims.internal.IImsRegisterListener;
import com.android.ims.internal.IImsUtListenerEx;
import com.android.ims.internal.ImsManagerEx;
import com.android.ims.internal.IImsMultiEndpoint;
import com.android.ims.internal.IImsExternalCallStateListener;
import com.android.ims.internal.ImsSrvccCallInfo;
import com.spreadtrum.ims.vowifi.Utilities.AttachState;
import com.spreadtrum.ims.vowifi.Utilities.RegisterState;
import com.spreadtrum.ims.vowifi.Utilities.SecurityConfig;
import com.spreadtrum.ims.vowifi.VoWifiServiceImpl;
import com.spreadtrum.ims.vowifi.VoWifiServiceImpl.DataRouterState;
import com.spreadtrum.ims.vowifi.VoWifiServiceImpl.IncomingCallAction;
import com.spreadtrum.ims.vowifi.VoWifiServiceImpl.VoWifiCallback;
import com.spreadtrum.ims.vowifi.VoWifiServiceImpl.WifiState;
import com.spreadtrum.ims.ImsVodafoneHelper;

public class ImsService extends Service {
    private static final String TAG = ImsService.class.getSimpleName();
    /** IMS registered state code. */
    public static final int IMS_REG_STATE_INACTIVE            = 0;
    public static final int IMS_REG_STATE_REGISTERED          = 1;
    public static final int IMS_REG_STATE_REGISTERING         = 2;
    public static final int IMS_REG_STATE_REG_FAIL            = 3;
    public static final int IMS_REG_STATE_UNKNOWN             = 4;
    public static final int IMS_REG_STATE_ROAMING             = 5;
    public static final int IMS_REG_STATE_DEREGISTERING       = 6;
    /** IMS service code. */
    private static final int ACTION_SWITCH_IMS_FEATURE = 100;
    private static final int ACTION_START_HANDOVER = 101;
    private static final int ACTION_NOTIFY_NETWORK_UNAVAILABLE = 102;
    private static final int ACTION_NOTIFY_VOWIFI_UNAVAILABLE = 103;
    private static final int ACTION_CANCEL_CURRENT_REQUEST = 104;
    private static final int ACTION_RELEASE_WIFI_RESOURCE = 105;

     /** WIFI service event. */
    private static final int EVENT_WIFI_ATTACH_STATE_UPDATE = 200;
    private static final int EVENT_WIFI_ATTACH_SUCCESSED = 201;
    private static final int EVENT_WIFI_ATTACH_FAILED = 202;
    private static final int EVENT_WIFI_ATTACH_STOPED = 203;
    private static final int EVENT_WIFI_INCOMING_CALL = 204;
    private static final int EVENT_WIFI_ALL_CALLS_END = 205;
    private static final int EVENT_WIFI_REFRESH_RESAULT = 206;
    private static final int EVENT_WIFI_REGISTER_RESAULT = 207;
    private static final int EVENT_WIFI_RESET_RESAULT = 208;
    private static final int EVENT_WIFI_DPD_DISCONNECTED = 209;
    private static final int EVENT_WIFI_NO_RTP = 210;
    private static final int EVENT_WIFI_UNSOL_UPDATE = 211;
    private static final int EVENT_WIFI_RTP_RECEIVED = 212;
    private static final int EVENT_UPDATE_DATA_ROUTER_FINISHED = 213;
    private static final int EVENT_NOTIFY_CP_VOWIFI_ATTACH_SUCCESSED = 214;
    private static final int ACTION_NOTIFY_VIDEO_CAPABILITY_CHANGE = 106;

    class ImsOperationType{
        public static final int IMS_OPERATION_SWITCH_TO_VOWIFI = 0;
        public static final int IMS_OPERATION_SWITCH_TO_VOLTE = 1;
        public static final int IMS_OPERATION_HANDOVER_TO_VOWIFI = 2;
        public static final int IMS_OPERATION_HANDOVER_TO_VOLTE = 3;
        public static final int IMS_OPERATION_SET_VOWIFI_UNAVAILABLE = 4;
        public static final int IMS_OPERATION_CANCEL_CURRENT_REQUEST = 5;
        public static final int IMS_OPERATION_CP_REJECT_SWITCH_TO_VOWIFI = 6;
        public static final int IMS_OPERATION_CP_REJECT_HANDOVER_TO_VOWIFI = 7;
        public static final int IMS_OPERATION_RELEASE_WIFI_RESOURCE = 8;
    }

    public static final int IMS_HANDOVER_ACTION_CONFIRMED = 999;

    class ImsHandoverType {
        public static final int IDEL_HANDOVER_TO_VOWIFI = 1;
        public static final int IDEL_HANDOVER_TO_VOLTE = 2;
        public static final int INCALL_HANDOVER_TO_VOWIFI = 3;
        public static final int INCALL_HANDOVER_TO_VOLTE = 4;
    }

    class ImsPDNStatus {
        public static final int IMS_PDN_ACTIVE_FAILED = 0;
        public static final int IMS_PDN_READY = 1;
        public static final int IMS_PDN_START = 2;
    }

    class ImsHandoverResult{
        public static final int IMS_HANDOVER_REGISTER_FAIL = 0;
        public static final int IMS_HANDOVER_SUCCESS = 1;
        public static final int IMS_HANDOVER_PDN_BUILD_FAIL = 2;
        public static final int IMS_HANDOVER_RE_REGISTER_FAIL = 3;
        public static final int IMS_HANDOVER_ATTACH_FAIL = 4;
        public static final int IMS_HANDOVER_ATTACH_SUCCESS = 5;
        public static final int IMS_HANDOVER_SRVCC_FAILED = 6;
    }
    /** Call end event. */
    public static class CallEndEvent {
        public static final int WIFI_CALL_END = 1;
        public static final int VOLTE_CALL_END = 2;
    }

    public static class UnsolicitedCode {
        public static final int SECURITY_DPD_DISCONNECTED = 1;
        public static final int SIP_TIMEOUT = 2;
        public static final int SIP_LOGOUT = 3;
        public static final int SECURITY_REKEY_FAILED = 4;
        public static final int SECURITY_STOP = 5;
    }

    /** Call type. */
    public static class CallType {
        public static final int NO_CALL = -1;
        public static final int VOLTE_CALL = 0;
        public static final int WIFI_CALL = 2;
    }

    /** S2b event code. */
    public static class S2bEventCode {
        public static final int S2b_STATE_IDLE = 0;
        public static final int S2b_STATE_PROGRESS = 1;
        public static final int S2b_STATE_CONNECTED = 2;
    }

    public static class ImsStackResetResult {
        public static final int INVALID_ID = -1;
        public static final int FAIL = 0;
        public static final int SUCCESS = 1;
    }

    // Only add for cmcc test, if this prop is FALSE, we needn't s2b function.
    // TODO: Need remove this after development.
    private static final String PROP_S2B_ENABLED = "persist.sys.s2b.enabled";

    private Map<Integer, ImsServiceImpl> mImsServiceImplMap = new HashMap<Integer, ImsServiceImpl>();

    private ConcurrentHashMap<IBinder, IImsRegisterListener> mImsRegisterListeners = new ConcurrentHashMap<IBinder, IImsRegisterListener>();
 
    private int mRequestId = -1;
    private Object mRequestLock = new Object();
    private IImsServiceListenerEx mImsServiceListenerEx;

    private int mCurrentImsFeature = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
    private int mInCallHandoverFeature = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
    private TelephonyManager mTelephonyManager;
    // add for Dual LTE
    private TelephonyManagerEx mTelephonyManagerEx;
    private PhoneStateListener mPhoneStateListener;
    private int mPhoneCount;
    private ImsServiceRequest mFeatureSwitchRequest;
    private ImsServiceRequest mReleaseVowifiRequest;
    private VoWifiServiceImpl mWifiService;
    private MyVoWifiCallback mVoWifiCallback;
    private VoLTERegisterListener mVoLTERegisterListener = new VoLTERegisterListener();
    private boolean mWifiRegistered = false;
    private boolean mVolteRegistered = false;//SPRD:Add for bug596304
    private boolean mIsWifiCalling = false;
    private boolean mIsCalling = false;
    private NotificationManager mNotificationManager;
    private String mVowifiRegisterMsg = "Wifi not register";
    private int mCurrentVowifiNotification = 100;
    private boolean mIsPendingRegisterVowifi;
    private boolean mIsPendingRegisterVolte;
    private boolean mIsVowifiCall;//This means call is started by Vowifi.
    private boolean mIsVolteCall;//This means call is started by Volte.
    private int mNetworkType = -1;
    private String mNetworkInfo = "Network info is null";
    private boolean mPendingAttachVowifiSuccess = false;//SPRD:Add for bug595321
    private boolean mPendingVowifiHandoverVowifiSuccess = false;//SPRD:Add for bug595321
    private boolean mPendingVolteHandoverVolteSuccess = false;
    private boolean mPendingActivePdnSuccess = false;//SPRD:Add for bug595321
    private boolean mAttachVowifiSuccess = false;//SPRD:Add for bug604833
    private boolean mPendingReregister = false;
    private boolean mIsS2bStopped = false;
    private boolean mIsCPImsPdnActived = false;
    private boolean mIsAPImsPdnActived = false;
    private boolean mIsLoggingIn =false;
    private boolean mPendingCPSelfManagement = false;
    private int mCallEndType = -1;
    private int mInCallPhoneId = -1;
    private boolean mIsEmergencyCallonCP = false;

    private class ImsServiceRequest {
        public int mRequestId;
        public int mEventCode;
        public int mServiceId;
        public int mTargetType;
        public ImsServiceRequest(int requestId, int eventCode, int serviceId, int targetType){
            mRequestId = requestId;
            mEventCode = eventCode;
            mServiceId = serviceId;
            mTargetType = targetType;
        }

        @Override
        public String toString(){
            return "ImsServiceRequest->mRequestId:"+mRequestId
                    +" mEventCode:"+mEventCode+ " mServiceId:"
                    +mServiceId+" mTargetType:"+mTargetType;
        }
    }

    /**
     * Used to listen to events.
     */
    private final Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            try{
                switch (msg.what) {
                    case ACTION_SWITCH_IMS_FEATURE:
                        if(mFeatureSwitchRequest != null){
                            if(mImsServiceListenerEx != null){
                                mImsServiceListenerEx.operationFailed(msg.arg1,"Repetitive operation",
                                        (msg.arg2 == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE)
                                        ? ImsOperationType.IMS_OPERATION_SWITCH_TO_VOLTE
                                                : ImsOperationType.IMS_OPERATION_SWITCH_TO_VOWIFI);
                            }
                            Log.w(TAG,"ACTION_SWITCH_IMS_FEATURE-> mFeatureSwitchRequest is exist!");
                        } else {
                            onReceiveHandoverEvent(false, msg.arg1/*requestId*/, msg.arg2/*targetType*/);
                        }
                        Log.i(TAG,"ACTION_SWITCH_IMS_FEATURE->mFeatureSwitchRequest:"+mFeatureSwitchRequest + " mAttachVowifiSuccess:" + mAttachVowifiSuccess);
                        break;
                    case ACTION_START_HANDOVER:
                        Log.i(TAG,"ACTION_START_HANDOVER->mIsPendingRegisterVowifi: " + mIsPendingRegisterVowifi + " mIsPendingRegisterVolte: " + mIsPendingRegisterVolte + " mAttachVowifiSuccess:" + mAttachVowifiSuccess);
                        if (mIsPendingRegisterVowifi) {
                            mIsPendingRegisterVowifi = false;
                            mFeatureSwitchRequest = null;
                        } else if(mIsPendingRegisterVolte) {
                            mIsPendingRegisterVolte = false;
                            mFeatureSwitchRequest = null;
                        }
                        if(mFeatureSwitchRequest != null){
                            if(mImsServiceListenerEx != null){
                                mImsServiceListenerEx.operationFailed(msg.arg1, "Already handle one request.",
                                        (msg.arg2 == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE)
                                        ? ImsOperationType.IMS_OPERATION_HANDOVER_TO_VOLTE
                                                : ImsOperationType.IMS_OPERATION_HANDOVER_TO_VOWIFI);
                            }
                            Log.w(TAG,"ACTION_START_HANDOVER-> mFeatureSwitchRequest is exist!");
                        } else {
                            onReceiveHandoverEvent(true, msg.arg1/*requestId*/, msg.arg2/*targetType*/);
                        }
                        break;
                    case ACTION_NOTIFY_NETWORK_UNAVAILABLE:
                        break;
                    case EVENT_WIFI_ATTACH_STATE_UPDATE:
                        int state = msg.arg1;
                        Log.i(TAG,"EVENT_WIFI_ATTACH_STATE_UPDATE-> state:" + state + ", mWifiRegistered:" + mWifiRegistered + ", mIsS2bStopped:" + mIsS2bStopped);
                        if (state != S2bEventCode.S2b_STATE_CONNECTED) {
                            mAttachVowifiSuccess = false;//SPRD:Add for bug604833
                        }
                        if(state != S2bEventCode.S2b_STATE_IDLE){
                            mIsS2bStopped = false;
                        }
                        break;
                    case EVENT_WIFI_ATTACH_SUCCESSED:
                        Log.i(TAG,"EVENT_WIFI_ATTACH_SUCCESSED-> mFeatureSwitchRequest:" + mFeatureSwitchRequest + " mIsCalling:" + mIsCalling
                                + " mPendingAttachVowifiSuccess:" + mPendingAttachVowifiSuccess
                                + " mPendingVowifiHandoverVowifiSuccess:" + mPendingVowifiHandoverVowifiSuccess + " mIsS2bStopped" + mIsS2bStopped
                                + " mAttachVowifiSuccess:" + mAttachVowifiSuccess);
                        if(mFeatureSwitchRequest != null){
                            notifyCPVowifiAttachSucceed();
                            if(mFeatureSwitchRequest.mEventCode == ACTION_START_HANDOVER){
                                /*SPRD: Modify for bug595321 and 610503{@*/
                                if (mIsCalling) {
                                    mInCallHandoverFeature = ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI;
                                    updateImsFeature(mFeatureSwitchRequest.mServiceId);
                                    mWifiService.updateDataRouterState(DataRouterState.CALL_VOWIFI);
                                    mIsPendingRegisterVowifi = true;
                                    if (mImsServiceListenerEx != null) {
                                        Log.i(TAG, "EVENT_WIFI_ATTACH_SUCCESSED -> operationSuccessed -> IMS_OPERATION_HANDOVER_TO_VOWIFI");
                                        mImsServiceListenerEx.operationSuccessed(mFeatureSwitchRequest.mRequestId,
                                                ImsOperationType.IMS_OPERATION_HANDOVER_TO_VOWIFI);
                                    }
                                    if(mIsVolteCall){
                                        ImsServiceImpl service = mImsServiceImplMap.get(
                                                Integer.valueOf(mFeatureSwitchRequest.mServiceId));
                                        service.enableWiFiParamReport();
                                    }
                                } else {
                                    updateImsFeature(mFeatureSwitchRequest.mServiceId);
                                    mCurrentImsFeature = ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI;
                                    if (mImsServiceListenerEx != null) {
                                        Log.i(TAG, "EVENT_WIFI_ATTACH_SUCCESSED -> operationSuccessed -> IMS_OPERATION_HANDOVER_TO_VOWIFI");
                                        mImsServiceListenerEx.operationSuccessed(mFeatureSwitchRequest.mRequestId,
                                                ImsOperationType.IMS_OPERATION_HANDOVER_TO_VOWIFI);
                                    }
                                    if (mWifiService != null && !mPendingVowifiHandoverVowifiSuccess) {
                                        mWifiService.register();
                                    }
                                    mPendingAttachVowifiSuccess = false;
                                    mWifiService.updateDataRouterState(DataRouterState.CALL_NONE);
                                }
                                Log.i(TAG, "EVENT_WIFI_ATTACH_SUCCESSED ->mFeatureSwitchRequest.mEventCode:" + mFeatureSwitchRequest.mEventCode + " mCurrentImsFeature:" + mCurrentImsFeature + " mIsCalling:" + mIsCalling
                                        + " mIsVowifiCall:" + mIsVowifiCall + " mIsVolteCall:" + mIsVolteCall + " mWifiRegistered:" + mWifiRegistered + " mVolteRegistered:" + mVolteRegistered);
                                /*@}*/
                            } else if(mFeatureSwitchRequest.mEventCode == ACTION_SWITCH_IMS_FEATURE){
                                mWifiService.register();
                            }
                        }
                        mIsAPImsPdnActived = true;
                        mIsS2bStopped = false;
                        mAttachVowifiSuccess = true;//SPRD:Add for bug604833
                        break;
                    case EVENT_WIFI_ATTACH_FAILED:
                        Log.i(TAG,"EVENT_WIFI_ATTACH_FAILED-> mFeatureSwitchRequest:" + mFeatureSwitchRequest + " mAttachVowifiSuccess:" + mAttachVowifiSuccess);
                        if(mImsServiceListenerEx != null){
                            if(mFeatureSwitchRequest != null){
                                mImsServiceListenerEx.operationFailed(mFeatureSwitchRequest.mRequestId,
                                        ""+msg.arg1
                                        ,(mFeatureSwitchRequest.mEventCode== ACTION_SWITCH_IMS_FEATURE)
                                        ? ImsOperationType.IMS_OPERATION_SWITCH_TO_VOWIFI
                                                : ImsOperationType.IMS_OPERATION_HANDOVER_TO_VOWIFI);
                                ImsServiceImpl service = mImsServiceImplMap.get(
                                        Integer.valueOf(mFeatureSwitchRequest.mServiceId));
                                service.notifyImsHandoverStatus(ImsHandoverResult.IMS_HANDOVER_ATTACH_FAIL);
                                if(mPendingAttachVowifiSuccess && !mIsCalling &&
                                        mFeatureSwitchRequest.mEventCode == ACTION_START_HANDOVER){
                                    mPendingAttachVowifiSuccess = false;
                                    mWifiService.updateDataRouterState(DataRouterState.CALL_NONE);
                                }
                                Log.i(TAG,"EVENT_WIFI_ATTACH_FAILED-> operationFailed, clear mFeatureSwitchRequest.");
                                mIsPendingRegisterVowifi = false;
                                mFeatureSwitchRequest = null;
                                if (msg.arg1 == 53766 && !mIsCalling) {//SPRD: add for bug661375 661372
                                    service.setIMSRegAddress(null);
                                }
                            }
                        }
                        mIsAPImsPdnActived = false;
                        mAttachVowifiSuccess = false;//SPRD:Add for bug604833
                        //SPRD:add for bug718067
                        if(mIsCalling && mInCallHandoverFeature == ImsConfig.FeatureConstants.FEATURE_TYPE_UT_OVER_WIFI){
                            Log.i(TAG,"EVENT_WIFI_ATTACH_FAILED-> handover to vowifi attach failed, set mInCallHandoverFeature unknow");
                            mInCallHandoverFeature = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
                            updateImsFeature();
                        }
                        break;
                    case EVENT_WIFI_ATTACH_STOPED:
                        Log.i(TAG, "EVENT_WIFI_ATTACH_STOPED, mWifiRegistered:" + mWifiRegistered);
                        mIsAPImsPdnActived = false;
                        mAttachVowifiSuccess = false;//SPRD:Add for bug604833
                        break;
                    case EVENT_WIFI_INCOMING_CALL:
                        ImsServiceImpl service = mImsServiceImplMap.get(
                                Integer.valueOf(ImsRegister.getPrimaryCard(mPhoneCount)+1));
                        service.sendIncomingCallIntent((String)msg.obj);
                        mInCallPhoneId = ImsRegister.getPrimaryCard(mPhoneCount);//sprd:add for bug635699
                        updateInCallState(true);
                        Log.i(TAG,"EVENT_WIFI_INCOMING_CALL-> callId:"+msg.obj);
                        break;
                    case EVENT_WIFI_ALL_CALLS_END:
                        Log.i(TAG,"EVENT_WIFI_ALL_CALLS_END-> mImsServiceListenerEx: "+mImsServiceListenerEx);
                        if (mImsServiceListenerEx != null) {
                            Log.i(TAG,"EVENT_WIFI_ALL_CALLS_END-> mFeatureSwitchRequest:" + mFeatureSwitchRequest + " mIsVowifiCall:" + mIsVowifiCall + " mIsVolteCall:" + mIsVolteCall + " mInCallHandoverFeature:" + mInCallHandoverFeature
                                    + " mIsPendingRegisterVolte:" + mIsPendingRegisterVolte + " mIsPendingRegisterVowifi:" + mIsPendingRegisterVowifi);
                            if(mFeatureSwitchRequest != null){
                                if(mFeatureSwitchRequest.mEventCode == ACTION_START_HANDOVER){
                                    /*SPRD: Add for bug586758,595321,610799{@*/
                                    ImsServiceImpl currentService = mImsServiceImplMap.get(
                                            Integer.valueOf(mFeatureSwitchRequest.mServiceId));
                                    if (currentService != null) {
                                        if (currentService.isVolteSessionListEmpty() && currentService.isVowifiSessionListEmpty()) {
                                            mCallEndType = CallEndEvent.WIFI_CALL_END;
                                            if (mInCallHandoverFeature != mFeatureSwitchRequest.mTargetType) {
                                                if (mFeatureSwitchRequest.mTargetType == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI) {
                                                    mPendingAttachVowifiSuccess = true;
                                                    if (mIsVowifiCall){
                                                        mPendingVowifiHandoverVowifiSuccess = true;
                                                    }
                                                } else if (mFeatureSwitchRequest.mTargetType == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE) {
                                                    mPendingActivePdnSuccess = true;
                                                }
                                            }
                                            Log.i(TAG,"EVENT_WIFI_ALL_CALLS_END->mPendingAttachVowifiSuccess:" + mPendingAttachVowifiSuccess + " mPendingActivePdnSuccess:" + mPendingActivePdnSuccess);
                                            if(mFeatureSwitchRequest.mTargetType == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI){
                                                if (mIsVolteCall && mIsPendingRegisterVowifi) {
                                                    mWifiService.register();
                                                }
                                                mIsPendingRegisterVowifi = false;
                                            }
                                            mInCallHandoverFeature = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
                                            if (!mPendingAttachVowifiSuccess && !mPendingActivePdnSuccess ) {
                                                mWifiService.updateDataRouterState(DataRouterState.CALL_NONE);
                                            }
                                            if(mIsVowifiCall && mCurrentImsFeature == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI
                                                    && mFeatureSwitchRequest != null && !mPendingAttachVowifiSuccess && !mPendingActivePdnSuccess) {
                                                mFeatureSwitchRequest = null;
                                            }
                                            if(mCurrentImsFeature != ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN){
                                                Log.i(TAG,"EVENT_WIFI_ALL_CALLS_END->mCurrentImsFeature:"+mCurrentImsFeature);
                                                updateInCallState(false);
                                            }
                                        }
                                    } else {
                                        Log.i(TAG,"EVENT_WIFI_ALL_CALLS_END->ImsServiceImpl is null");
                                    }
                                    /*@}*/
                                }
                            } else {
                                ImsServiceImpl currentService = mImsServiceImplMap.get(
                                        Integer.valueOf(ImsRegister.getPrimaryCard(mPhoneCount)+1));
                                if (currentService != null){
                                    if (currentService.isVolteSessionListEmpty() && currentService.isVowifiSessionListEmpty()) {
                                        Log.i(TAG,"EVENT_WIFI_ALL_CALLS_END->mCurrentImsFeature:"+mCurrentImsFeature);
                                        updateInCallState(false);
                                        mCallEndType = CallEndEvent.WIFI_CALL_END;
                                        mInCallHandoverFeature = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
                                        mWifiService.updateDataRouterState(DataRouterState.CALL_NONE);
                                    }
                                }
                            }
                            /*SPRD: Modify for bug595321{@*/
                            ImsServiceImpl imsService = mImsServiceImplMap.get(
                                    Integer.valueOf(ImsRegister.getPrimaryCard(mPhoneCount)+1));
                            Log.i(TAG, "EVENT_WIFI_ALL_CALLS_END -> mPhoneCount: "+mPhoneCount+"  imsService:  "+imsService+"  mIsVowifiCall: "+mIsVowifiCall+"   mIsVolteCall:" +mIsVolteCall);
                            if (imsService != null){
                                if (imsService.isVolteSessionListEmpty() && imsService.isVowifiSessionListEmpty()){
                                    if (mIsVowifiCall) {
                                        mIsVowifiCall = false;
                                    } else if (mIsVolteCall) {
                                        mIsVolteCall = false;
                                    }
                                }
                            } else {
                                Log.i(TAG,"EVENT_WIFI_ALL_CALLS_END->ImsServiceImpl is null");
                            }
                            /*@}*/
                        }
                        break;
                    case EVENT_WIFI_REFRESH_RESAULT:
                        break;
                    case EVENT_WIFI_REGISTER_RESAULT:
                        Boolean result = (Boolean)msg.obj;
                        if (result == null) break;
                        Log.i(TAG, "EVENT_WIFI_REGISTER_RESAULT -> mWifiRegistered:" + result.booleanValue()
                                + ", mFeatureSwitchRequest:" + mFeatureSwitchRequest + " mIsLoggingIn:" + mIsLoggingIn);
                        mWifiRegistered = result.booleanValue();
                        mIsLoggingIn = false;
                        updateImsFeature();
                        if (mFeatureSwitchRequest != null) {
                            ImsServiceImpl requestService = mImsServiceImplMap.get(
                                    Integer.valueOf(mFeatureSwitchRequest.mServiceId));
                            if (result.booleanValue()) {
                                if (mImsServiceListenerEx != null) {
                                    Log.i(TAG, "EVENT_WIFI_REGISTER_RESAULT -> operationSuccessed -> VoWifi register success");
                                    mImsServiceListenerEx.operationSuccessed(mFeatureSwitchRequest.mRequestId,
                                            (mFeatureSwitchRequest.mEventCode== ACTION_SWITCH_IMS_FEATURE)
                                            ? ImsOperationType.IMS_OPERATION_SWITCH_TO_VOWIFI
                                                    : ImsOperationType.IMS_OPERATION_HANDOVER_TO_VOWIFI);
                                    requestService.notifyImsHandoverStatus(ImsHandoverResult.IMS_HANDOVER_SUCCESS);
                                }
                                /*SPRD: Add for bug604833{@*/
                                if(mFeatureSwitchRequest.mEventCode == ACTION_SWITCH_IMS_FEATURE){
                                    if(requestService!=null){
                                        requestService.setIMSRegAddress(mWifiService.getCurLocalAddress());
                                    }
                                }
                                /*@}*/
                            } else if (mImsServiceListenerEx != null) {
                                Log.i(TAG, "EVENT_WIFI_REGISTER_RESAULT -> operationFailed -> VoWifi register failed");
                                mImsServiceListenerEx.operationFailed(mFeatureSwitchRequest.mRequestId
                                        ,"VoWifi register failed"
                                        ,(mFeatureSwitchRequest.mEventCode== ACTION_SWITCH_IMS_FEATURE)
                                        ? ImsOperationType.IMS_OPERATION_SWITCH_TO_VOWIFI
                                                : ImsOperationType.IMS_OPERATION_HANDOVER_TO_VOWIFI);
                                requestService.notifyImsHandoverStatus(ImsHandoverResult.IMS_HANDOVER_REGISTER_FAIL);
                                Toast.makeText(ImsService.this, R.string.vowifi_register_error,
                                        Toast.LENGTH_LONG).show();
                                /*SPRD: Modify for bug604833{@*/
                                if(mFeatureSwitchRequest.mEventCode== ACTION_SWITCH_IMS_FEATURE) {
                                    if (requestService != null){
                                        requestService.setIMSRegAddress(null);
                                    }
                                }
                                mAttachVowifiSuccess = false;
                                /*@}*/
                            }
                            if(mFeatureSwitchRequest.mTargetType == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI){
                                mFeatureSwitchRequest = null;
                            }
                        }
                        break;
                    case ACTION_NOTIFY_VOWIFI_UNAVAILABLE:
                        Boolean isOnlySendAT = (Boolean)msg.obj;
                        Log.w(TAG,"ACTION_NOTIFY_VOWIFI_UNAVAILABLE-> isOnlySendAT:" + isOnlySendAT + " mFeatureSwitchRequest:" + mFeatureSwitchRequest
                                + " mIsCPImsPdnActived:" + mIsCPImsPdnActived);
                        if (isOnlySendAT == null) break;
                        if(mReleaseVowifiRequest != null){
                            if(mImsServiceListenerEx != null){
                                mImsServiceListenerEx.operationFailed(msg.arg1, "Already handle one request.",
                                        ImsOperationType.IMS_OPERATION_SET_VOWIFI_UNAVAILABLE);
                            }
                            Log.w(TAG,"ACTION_NOTIFY_VOWIFI_UNAVAILABLE-> mReleaseVowifiRequest is exist!");
                        } else {
                            mReleaseVowifiRequest = new ImsServiceRequest(msg.arg1/*requestId*/,
                                    ACTION_NOTIFY_VOWIFI_UNAVAILABLE /*eventCode*/,
                                    ImsRegister.getPrimaryCard(mPhoneCount)+1/*serviceId*/,
                                    ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE);

                            //SPRD:delete for bug758003
                            if(!isOnlySendAT){
                                //SPRD: add for bug645935
                                int delaySend = Settings.Global.getInt(getApplicationContext().getContentResolver(),
                                        Settings.Global.AIRPLANE_MODE_ON, 0) > 0 ? 0:500;
                                if(mFeatureSwitchRequest == null ||
                                        mFeatureSwitchRequest.mEventCode != ACTION_START_HANDOVER){
                                    mWifiService.resetAll(msg.arg2 == 0 ? WifiState.DISCONNECTED : WifiState.CONNECTED,delaySend);
                                } else {
                                    mWifiService.resetAll(msg.arg2 == 0 ? WifiState.DISCONNECTED : WifiState.CONNECTED);
                                }
                            } else {
                                Log.i(TAG, "ACTION_NOTIFY_VOWIFI_UNAVAILABLE -> operationSuccessed -> IMS_OPERATION_SET_VOWIFI_UNAVAILABLE");
                                mImsServiceListenerEx.operationSuccessed(
                                        mReleaseVowifiRequest.mRequestId,
                                        ImsOperationType.IMS_OPERATION_SET_VOWIFI_UNAVAILABLE);
                                mReleaseVowifiRequest = null;
                            }
                            Log.i(TAG,"ACTION_NOTIFY_VOWIFI_UNAVAILABLE-> wifi state: " + msg.arg2);

                            if(!mIsCalling){
                                ImsServiceImpl imsService = mImsServiceImplMap.get(
                                        Integer.valueOf(ImsRegister.getPrimaryCard(mPhoneCount)+1));
                                if(imsService != null){
                                    imsService.notifyVoWifiEnable(false);
                                    mPendingCPSelfManagement = true;
                                    Log.i(TAG,"ACTION_NOTIFY_VOWIFI_UNAVAILABLE-> notifyVoWifiUnavaliable. mPendingCPSelfManagement:" + mPendingCPSelfManagement);
                                }
                            }
                            if (mFeatureSwitchRequest != null) {
                                mFeatureSwitchRequest = null;
                            }
                            mIsPendingRegisterVolte = false;//SPRD: add for bug723080
                        }
                        if(mPendingAttachVowifiSuccess){
                            Log.i(TAG,"ACTION_NOTIFY_VOWIFI_UNAVAILABLE->mPendingAttachVowifiSuccess is true->mCallEndType:"
                                    +mCallEndType +" mIsCalling:"+mIsCalling);
                            mPendingAttachVowifiSuccess = false;
                            if(mCallEndType != -1 && !mIsCalling){
                                Log.i(TAG,"ACTION_NOTIFY_VOWIFI_UNAVAILABLE-> mCallEndType:"+mCallEndType);
                                notifyCpCallEnd();
                            }
                        }
                        mAttachVowifiSuccess = false;//SPRD:Add for bug604833
                        break;
                    case EVENT_WIFI_RESET_RESAULT:
                        int releaseResult = msg.arg1;
                        int errorCode = msg.arg2;
                        if(releaseResult == ImsStackResetResult.SUCCESS){
                            mWifiRegistered = false;
                            updateImsFeature();
                        }
                        if(mReleaseVowifiRequest != null){
                            if (mImsServiceListenerEx != null) {
                                int actionType;
                                if (mReleaseVowifiRequest.mEventCode == ACTION_RELEASE_WIFI_RESOURCE) {
                                    actionType = ImsOperationType.IMS_OPERATION_RELEASE_WIFI_RESOURCE;
                                } else if (mReleaseVowifiRequest.mEventCode == ACTION_NOTIFY_VOWIFI_UNAVAILABLE) {
                                    actionType = ImsOperationType.IMS_OPERATION_SET_VOWIFI_UNAVAILABLE;
                                } else {
                                    actionType = ImsOperationType.IMS_OPERATION_CANCEL_CURRENT_REQUEST;
                                }
                                if(releaseResult == ImsStackResetResult.SUCCESS){
                                    Log.i(TAG, "EVENT_WIFI_RESET_RESAULT -> operationSuccessed -> wifi release success");
                                    mImsServiceListenerEx.operationSuccessed(mReleaseVowifiRequest.mRequestId,
                                            actionType);
                                } else {
                                    Log.i(TAG, "EVENT_WIFI_RESET_RESAULT -> operationFailed -> wifi release fail ");
                                    mImsServiceListenerEx.operationFailed(mReleaseVowifiRequest.mRequestId,
                                            "wifi release fail:"+errorCode,
                                            actionType);
                                }
                            }
                            mReleaseVowifiRequest = null;
                        }
                        Log.i(TAG,"EVENT_WIFI_RESET_RESAULT-> result:" + releaseResult + " errorCode:"+errorCode);
                        break;
                    case ACTION_RELEASE_WIFI_RESOURCE:
                        if (mReleaseVowifiRequest != null) {
                            if (mImsServiceListenerEx != null) {
                                mImsServiceListenerEx.operationFailed(msg.arg1,
                                        "Already handle one request.",
                                        ImsOperationType.IMS_OPERATION_RELEASE_WIFI_RESOURCE);
                            }
                            Log.w(TAG, "ACTION_RELEASE_WIFI_RESOURCE-> mReleaseVowifiRequest is exist!");
                        } else {
                            if (mWifiService != null) {
                                mWifiService.resetAll(WifiState.DISCONNECTED);
                            }
                            mReleaseVowifiRequest = new ImsServiceRequest(msg.arg1/* requestId */,
                                    ACTION_RELEASE_WIFI_RESOURCE /* eventCode */,
                                    ImsRegister.getPrimaryCard(mPhoneCount) + 1/* serviceId */,
                                    ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE);
                            Log.w(TAG, "ACTION_RELEASE_WIFI_RESOURCE-> wifi state: DISCONNECTED");
                        }
                        mAttachVowifiSuccess = false;//SPRD:Add for bug604833
                        break;
                    case ACTION_CANCEL_CURRENT_REQUEST:
                        Log.i(TAG,"ACTION_CANCEL_CURRENT_REQUEST-> mFeatureSwitchRequest: "+mFeatureSwitchRequest+" mAttachVowifiSuccess:"+mAttachVowifiSuccess);
                        if(mFeatureSwitchRequest != null
                                && mFeatureSwitchRequest.mTargetType == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI){
                            if(mReleaseVowifiRequest != null){
                                if(mImsServiceListenerEx != null){
                                    mImsServiceListenerEx.operationFailed(msg.arg1, "Already handle one request.",
                                            ImsOperationType.IMS_OPERATION_CANCEL_CURRENT_REQUEST);
                                }
                                Log.w(TAG,"ACTION_CANCEL_CURRENT_REQUEST-> mReleaseVowifiRequest is exist!");
                                return;
                            } else {
                                mReleaseVowifiRequest = new ImsServiceRequest(msg.arg1/*requestId*/,
                                        ImsOperationType.IMS_OPERATION_CANCEL_CURRENT_REQUEST /*eventCode*/,
                                        ImsRegister.getPrimaryCard(mPhoneCount)+1/*serviceId*/,
                                        ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE);
                            }
                            mWifiService.resetAll(WifiState.DISCONNECTED /* Do not attached now, same as wifi do not connected */);
                            /*SPRD: Modify for bug604833{@*/
                            ImsServiceImpl imsService = mImsServiceImplMap.get(
                                    Integer.valueOf(ImsRegister.getPrimaryCard(mPhoneCount)+1));
                            if (mFeatureSwitchRequest.mEventCode== ACTION_SWITCH_IMS_FEATURE && mAttachVowifiSuccess) {
                                if(imsService != null){
                                    imsService.setIMSRegAddress(null);
                                }
                            }
                            if(imsService != null){
                                imsService.notifyVoWifiEnable(false);
                                mPendingCPSelfManagement = true;
                                Log.i(TAG,"ACTION_CANCEL_CURRENT_REQUEST-> notifyVoWifiUnavaliable. mPendingCPSelfManagement:" + mPendingCPSelfManagement);
                            }
                            mAttachVowifiSuccess = false;
                            mFeatureSwitchRequest = null;
                            if(mPendingAttachVowifiSuccess){
                                mPendingAttachVowifiSuccess = false;
                                Log.i(TAG,"ACTION_CANCEL_CURRENT_REQUEST-> mPendingAttachVowifiSuccess is true!");
                                if(mCallEndType != -1 && !mIsCalling){
                                    Log.i(TAG,"ACTION_CANCEL_CURRENT_REQUEST-> mCallEndType:"+mCallEndType);
                                    notifyCpCallEnd();
                                }
                            }
                            /*@}*/
                            Log.i(TAG,"ACTION_CANCEL_CURRENT_REQUEST-> mIsCalling:"+mIsCalling);
                            if(!mIsCalling){
                                mWifiService.updateDataRouterState(DataRouterState.CALL_NONE);
                            }
                        } else {
                            if(mImsServiceListenerEx != null){
                                mImsServiceListenerEx.operationFailed(msg.arg1/*requestId*/,"Invalid Request",
                                        ImsOperationType.IMS_OPERATION_CANCEL_CURRENT_REQUEST);
                            } else{
                                Log.i(TAG, "ACTION_CANCEL_CURRENT_REQUEST -> mImsServiceListenerEx is null ");
                            }
                        }
                        break;
                    case EVENT_WIFI_DPD_DISCONNECTED:
                        if(mImsServiceListenerEx != null){
                            mImsServiceListenerEx.onDPDDisconnected();
                        }
                        break;
                    case EVENT_WIFI_NO_RTP:
                        Boolean isVideo = (Boolean)msg.obj;
                        if (isVideo == null) break;
                        if(mImsServiceListenerEx != null){
                            mImsServiceListenerEx.onNoRtpReceived(isVideo);
                        }
                        break;
                    case EVENT_WIFI_UNSOL_UPDATE:
                        Log.i(TAG,"EVENT_WIFI_UNSOL_UPDATE-> mFeatureSwitchRequest: " + mFeatureSwitchRequest + " UnsolicitedCode:" + msg.arg1 + " mIsVowifiCall:" + mIsVowifiCall + " mIsVolteCall:" + mIsVolteCall
                                + " mIsS2bStopped:" + mIsS2bStopped + " mPendingReregister:" + mPendingReregister);
                        if(msg.arg1==UnsolicitedCode.SECURITY_STOP){
                            mIsS2bStopped = true;
                        }
                        if (mFeatureSwitchRequest != null && mFeatureSwitchRequest.mEventCode == ACTION_START_HANDOVER
                                && (msg.arg1==UnsolicitedCode.SECURITY_DPD_DISCONNECTED || msg.arg1==UnsolicitedCode.SECURITY_REKEY_FAILED || msg.arg1==UnsolicitedCode.SECURITY_STOP)){
                            if(mFeatureSwitchRequest.mTargetType == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE
                                    && mCurrentImsFeature == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE
                                    && msg.arg1==UnsolicitedCode.SECURITY_STOP){
                                if (mIsS2bStopped && mPendingReregister){
                                    if (mInCallHandoverFeature != ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN) {
                                        ImsServiceImpl imsService = mImsServiceImplMap.get(
                                                Integer.valueOf(mFeatureSwitchRequest.mServiceId));
                                        if(mIsVolteCall){
                                            imsService.notifyDataRouter();
                                        }else if(mIsVowifiCall){
                                            mWifiService.reRegister(mNetworkType, mNetworkInfo);
                                        }
                                        mPendingReregister = false;
                                    }
                                }
                            }
                        }
                        notifyWiFiError(msg.arg1);
                        break;
                    case EVENT_WIFI_RTP_RECEIVED:
                        Boolean rtpIsVideo = (Boolean)msg.obj;
                        if (rtpIsVideo == null) break;
                        if(mImsServiceListenerEx != null){
                            mImsServiceListenerEx.onRtpReceived(rtpIsVideo);
                        }
                        break;
                    case EVENT_UPDATE_DATA_ROUTER_FINISHED:
                        if(mCallEndType != -1){
                            Log.i(TAG,"EVENT_UPDATE_DATA_ROUTER_FINISHED-> mCallEndType:"+mCallEndType);
                        }
                        notifyCpCallEnd();
                        Log.i(TAG,"EVENT_UPDATE_DATA_ROUTER_FINISHED-> mFeatureSwitchRequest: "+mFeatureSwitchRequest + " mInCallHandoverFeature: " + mInCallHandoverFeature
                                + " mPendingVowifiHandoverVowifiSuccess:" + mPendingVowifiHandoverVowifiSuccess + " mPendingVolteHandoverVolteSuccess:" + mPendingVolteHandoverVolteSuccess);
                        if (mFeatureSwitchRequest != null && (mInCallHandoverFeature != ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN || mPendingVolteHandoverVolteSuccess || mPendingVowifiHandoverVowifiSuccess)) {
                        ImsServiceImpl imsService = mImsServiceImplMap.get(
                                Integer.valueOf(mFeatureSwitchRequest.mServiceId));
                            if (mIsVolteCall || mPendingVolteHandoverVolteSuccess) {
                                if (mFeatureSwitchRequest.mEventCode == ACTION_START_HANDOVER
                                        && mFeatureSwitchRequest.mTargetType == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE
                                        && mCurrentImsFeature == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE) {
                                    if(mIsS2bStopped){
                                        Log.i(TAG,"EVENT_UPDATE_DATA_ROUTER_FINISHED->notifyDataRouter");
                                        imsService.notifyDataRouter();
                                    }else{
                                        mPendingReregister = true;
                                    }
                                    Log.i(TAG,"EVENT_UPDATE_DATA_ROUTER_FINISHED->mIsS2bStopped:" + mIsS2bStopped + " mPendingReregister" + mPendingReregister);
                                } else if (mFeatureSwitchRequest.mEventCode == ACTION_START_HANDOVER
                                        && mFeatureSwitchRequest.mTargetType == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI
                                        && mCurrentImsFeature == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI) {
                                    int type = 6;
                                    StringBuffer info = new StringBuffer();
                                    WifiInfo wifiInfo;
                                    String wifiInfoHead = "IEEE-802.11;i-wlan-node-id=";
                                    info.append(wifiInfoHead);
                                    WifiManager wifiManager = (WifiManager)ImsService.this.getSystemService(Context.WIFI_SERVICE);
                                    if (wifiManager != null) {
                                        Log.i(TAG,"EVENT_UPDATE_DATA_ROUTER_FINISHED-> wifiManager :" + wifiManager);
                                        wifiInfo = wifiManager.getConnectionInfo();
                                        if (wifiInfo != null && wifiInfo.getBSSID() != null) {
                                            Log.i(TAG,"EVENT_UPDATE_DATA_ROUTER_FINISHED-> wifiInfo.getBSSID(): " + wifiInfo.getBSSID());
                                            info.append(wifiInfo.getBSSID().replace(":",""));
                                        }
                                    }
                                    Log.i(TAG,"EVENT_UPDATE_DATA_ROUTER_FINISHED->notifyImsNetworkInfo->type: " + type + " info: " + info.toString());
                                    imsService.notifyImsNetworkInfo(type, info.toString());
                                }
                                if (mPendingVolteHandoverVolteSuccess) {
                                    if(mFeatureSwitchRequest != null){
                                        mFeatureSwitchRequest = null;
                                    }
                                    mPendingVolteHandoverVolteSuccess = false;
                                }
                            } else if (mIsVowifiCall || mPendingVowifiHandoverVowifiSuccess) {
                                if (mFeatureSwitchRequest.mEventCode == ACTION_START_HANDOVER
                                        && mFeatureSwitchRequest.mTargetType == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE
                                        && mCurrentImsFeature == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE) {
                                    Log.i(TAG,"EVENT_UPDATE_DATA_ROUTER_FINISHED->reRegister->type: " + mNetworkType + " info:" + mNetworkInfo);
                                    if(mIsS2bStopped){
                                        mWifiService.reRegister(mNetworkType, mNetworkInfo);
                                    }else{
                                        mPendingReregister = true;
                                    }
                                    Log.i(TAG,"EVENT_UPDATE_DATA_ROUTER_FINISHED->mIsS2bStopped:" + mIsS2bStopped + " mPendingReregister" + mPendingReregister);
                                } else if (mFeatureSwitchRequest.mEventCode == ACTION_START_HANDOVER
                                        && mFeatureSwitchRequest.mTargetType == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI
                                        && mCurrentImsFeature == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI) {
                                    int type = -1;//SPRD:"-1" means "EN_MTC_ACC_NET_IEEE_802_11"
                                    StringBuffer info = new StringBuffer();
                                    WifiInfo wifiInfo;
                                    WifiManager wifiManager = (WifiManager)ImsService.this.getSystemService(Context.WIFI_SERVICE);
                                    if (wifiManager != null) {
                                        Log.i(TAG,"EVENT_UPDATE_DATA_ROUTER_FINISHED-> wifiManager :" + wifiManager);
                                        wifiInfo = wifiManager.getConnectionInfo();
                                        if (wifiInfo != null) {
                                            Log.i(TAG,"EVENT_UPDATE_DATA_ROUTER_FINISHED-> wifiInfo.getBSSID(): " + wifiInfo.getBSSID());
                                            if(wifiInfo.getBSSID()!=null){
                                                info.append(wifiInfo.getBSSID().replace(":",""));
                                            }
                                        }
                                    }
                                    Log.i(TAG,"EVENT_UPDATE_DATA_ROUTER_FINISHED->reRegister->type: " + type + " info: " + info.toString());
                                    mWifiService.reRegister(type,info.toString());
                                }
                                if (mPendingVowifiHandoverVowifiSuccess) {
                                    if(mFeatureSwitchRequest != null){
                                        mFeatureSwitchRequest = null;
                                    }
                                    mPendingVowifiHandoverVowifiSuccess = false;
                                }
                            }
                    }
                    break;
                case EVENT_NOTIFY_CP_VOWIFI_ATTACH_SUCCESSED:
                    Log.i(TAG,"EVENT_NOTIFY_CP_VOWIFI_ATTACH_SUCCESSED-> notifyImsHandoverStatus:" + ImsHandoverResult.IMS_HANDOVER_ATTACH_SUCCESS);
                    ImsServiceImpl Impl = mImsServiceImplMap.get(
                            Integer.valueOf(ImsRegister.getPrimaryCard(mPhoneCount)+1));
                    Impl.notifyImsHandoverStatus(ImsHandoverResult.IMS_HANDOVER_ATTACH_SUCCESS);
                    break;
                case ACTION_NOTIFY_VIDEO_CAPABILITY_CHANGE: //SPRD: add for bug771875
                    updateImsFeatureForAllService();
                    break;

                default:
                    break;
                }
            } catch(RemoteException e){
                e.printStackTrace();
            }
        }
    };

    @Override
    public IBinder onBind(Intent intent) {
        iLog("Ims Service onBind.");
        return mImsBinder;
    }

    @Override
    public void onCreate() {
        iLog("Ims Service onCreate.");
        super.onCreate();
        ServiceManager.addService("ims", mImsBinder);
        ServiceManager.addService(ImsManagerEx.IMS_SERVICE_EX, mImsServiceExBinder);
        ServiceManager.addService(ImsManagerEx.IMS_UT_EX, mImsUtExBinder);

        mVoWifiCallback = new MyVoWifiCallback();
        mWifiService = new VoWifiServiceImpl(getApplicationContext());
        mWifiService.registerCallback(mVoWifiCallback);

        Phone[] phones = PhoneFactory.getPhones();
        VTManagerProxy.init(this);
        if(phones != null){
            for(Phone phone : phones){
                ImsServiceImpl impl = new ImsServiceImpl(phone, this, mWifiService);
                impl.addListener(mVoLTERegisterListener);
                mImsServiceImplMap.put(impl.getServiceId(), impl);
            }
        }
        mTelephonyManager = (TelephonyManager)this.getSystemService(Context.TELEPHONY_SERVICE);
        mTelephonyManagerEx = TelephonyManagerEx.from(getApplicationContext());
        mPhoneCount = mTelephonyManager.getPhoneCount();
        mPhoneStateListener = new PhoneStateListener() {
            @Override
            public void onCallStateChanged(int state, String incomingNumber) {
                boolean isInCall = false;
                mInCallPhoneId = -1;
                if(state != TelephonyManager.CALL_STATE_IDLE){
                    isInCall = true;
                    Phone[] phones = PhoneFactory.getPhones();
                    if(phones != null){
                        for(Phone phone : phones){
                            if(phone.getState() != PhoneConstants.State.IDLE){
                                mInCallPhoneId = phone.getPhoneId();
                                break;
                            }
                        }
                    }
                }
                /*SPRD: Modify for bug753958{@*/
                boolean isEmergency = PhoneNumberUtils.isEmergencyNumber(incomingNumber);
                if(!mIsWifiCalling && isEmergency && isInCall){
                    mIsEmergencyCallonCP = true;
                }else{
                    mIsEmergencyCallonCP = false;
                }
                /*@}*/
                updateInCallState(isInCall);
                iLog("onCallStateChanged->isInCall:"+isInCall+" mIsWifiCalling:"+mIsWifiCalling
                        +" inCallPhoneId:"+mInCallPhoneId +" mIsEmergencyCallonCP="+mIsEmergencyCallonCP);
            }
        };
        mTelephonyManager.listen(mPhoneStateListener, PhoneStateListener.LISTEN_CALL_STATE);
    }

    @Override
    public void onDestroy() {
        iLog("Ims Service Destroyed.");
        super.onDestroy();
    }

    /*
     * Implement the methods of the IImsService interface in this stub
     */
    private final IImsService.Stub mImsBinder = new IImsService.Stub() {
        @Override
        public int open(int phoneId, int serviceClass, PendingIntent incomingCallIntent,
                IImsRegistrationListener listener) {
            ImsServiceImpl impl = mImsServiceImplMap.get(Integer.valueOf(phoneId+1));
            if(impl != null){
                return impl.open(serviceClass, incomingCallIntent, listener);
            }
            Log.d (TAG, "Open returns serviceId " + phoneId + " ImsServiceImpl:"+impl);
            return 0;
        }


        @Override
        public void close(int serviceId) {
            ImsServiceImpl impl = mImsServiceImplMap.get(Integer.valueOf(serviceId));
            if(impl != null){
                impl.close();
            }
            Log.d (TAG, "close returns serviceId:" + serviceId + " ImsServiceImpl:"+impl);
        }

        @Override
        public boolean isConnected(int serviceId, int serviceType, int callType) {
            return true; //TODO:
        }


        @Override
        public boolean isOpened(int serviceId) {
            ImsServiceImpl impl = mImsServiceImplMap.get(Integer.valueOf(serviceId));
            if(impl != null){
                return impl.isOpened();
            }
            return false;
        }

        @Override
        public void setRegistrationListener(int serviceId, IImsRegistrationListener listener) {
            ImsServiceImpl service = mImsServiceImplMap.get(new Integer(serviceId));
            if (service == null) {
                Log.e (TAG, "Invalid ServiceId ");
                return;
            }
            service.setRegistrationListener(listener);
        }

        @Override
        public ImsCallProfile createCallProfile(int serviceId, int serviceType, int callType) {
            ImsServiceImpl service = mImsServiceImplMap.get(new Integer(serviceId));
            if (service == null) {
                Log.e (TAG, "Invalid ServiceId ");
                return null;
            }
            return service.createCallProfile(serviceType, callType);
        }

        @Override
        public IImsCallSession createCallSession(int serviceId, ImsCallProfile profile,
                IImsCallSessionListener listener) {
            /*SPRD: Modify for bug586758{@*/
            Log.i (TAG,"createCallSession->mIsVowifiCall: " + mIsVowifiCall
                    + " mIsVolteCall: " + mIsVolteCall + " isVoWifiEnabled(): " + isVoWifiEnabled()
                    + " isVoLTEEnabled(): " + isVoLTEEnabled()+"  serviceId: "+serviceId);
            mInCallPhoneId = serviceId-1;//SPRD:add for bug635699
            updateInCallState(true);
            if ((isVoWifiEnabled() && !mIsVowifiCall && !mIsVolteCall) || mIsVowifiCall) {
                if (isVoWifiEnabled() && !mIsVowifiCall && !mIsVolteCall) {
                    mIsVowifiCall = true;
                    mWifiService.updateDataRouterState(DataRouterState.CALL_VOWIFI); //Add for data router
                }
                Log.e(TAG, "createCallSession-> startVoWifiCall");
                return mWifiService.createCallSession(profile, listener);
            }
            /*@}*/
            ImsServiceImpl service = mImsServiceImplMap.get(new Integer(serviceId));
            if (service == null) {
                Log.e (TAG, "Invalid ServiceId ");
                return null;
            }
            if (isVoLTEEnabled() && !mIsVowifiCall && !mIsVolteCall) {
                mIsVolteCall = true;
                mWifiService.updateDataRouterState(DataRouterState.CALL_VOLTE);//Add for data router
            }
            Log.e (TAG,"createCallSession-> startVoLteCall");
            return service.createCallSession(serviceId, profile, listener);
        }

        @Override
        public IImsCallSession getPendingCallSession(int serviceId, String callId){
            Log.i (TAG," getPendingCallSession-> serviceId: " + serviceId + "  callId: " + callId + " mIsVowifiCall: " + mIsVowifiCall
                    + " mIsVolteCall: " + mIsVolteCall + " isVoWifiEnabled(): " + isVoWifiEnabled() + " isVoLTEEnabled(): " + isVoLTEEnabled());
            mInCallPhoneId = serviceId-1;//SPRD:add for bug635699
            updateInCallState(true);
            /*SPRD: Modify for bug586758{@*/
           if((isVoWifiEnabled() && !mIsVowifiCall && !mIsVolteCall) || mIsVowifiCall){
                mIsVowifiCall = true;
                IImsCallSession session = mWifiService.getPendingCallSession(callId);
                Log.i (TAG,"getPendingCallSession-> session: " + session);

                //SPRD: add for bug650614
                ImsServiceImpl service = mImsServiceImplMap.get(new Integer(serviceId));
                Log.i (TAG," getPendingCallSession-> getPendingCallSession...service: "+service);
                if(service.getPendingCallSession(callId)!=null){
                    Log.i (TAG,"Volte unknow call");
                    mIsVolteCall = true;
                    return service.getPendingCallSession(callId);
                }
                return session;
            }
            /*@}*/
            ImsServiceImpl service = mImsServiceImplMap.get(new Integer(serviceId));
            if (service == null || callId == null) {
                Log.i (TAG, "Invalid arguments " + service + " " + callId+"  service: "+service);
                return null;
            }//SPRD:modify by bug650141
            if ((isVoLTEEnabled()||service.getPendingCallSession(callId)!=null) && !mIsVowifiCall && !mIsVolteCall) {
                mIsVolteCall = true;
            }
            Log.i (TAG,"getPendingCallSession->service.getPendingCallSession(callId): " + service.getPendingCallSession(callId));
            return service.getPendingCallSession(callId);
        }

        /**
         * Ut interface for the supplementary service configuration.
         */
        @Override
        public IImsUt getUtInterface(int serviceId){
            ImsServiceImpl service = mImsServiceImplMap.get(new Integer(serviceId));
            if (service == null) {
                Log.e (TAG, "Invalid ServiceId " + serviceId);
                return null;
            }
            return service.getUTProxy();
        }

        /**
         * Config interface to get/set IMS service/capability parameters.
         */
        @Override
        public IImsConfig getConfigInterface(int phoneId) {
            ImsServiceImpl service = mImsServiceImplMap.get(new Integer(phoneId+1));
            if (service == null) {
                Log.e (TAG, "getConfigInterface Invalid phoneId " + phoneId);
                return null;
            }
            return service.getConfigInterface();
        }

        /**
         * Used for turning on IMS when its in OFF state.
         */
        @Override
        public void turnOnIms(int phoneId){
            if (!ImsManagerEx.isDualLteModem()) {
                Log.d(TAG, "ImsService turnOnIms() isNotDualLteModem ");
                for (int i = 0; i < TelephonyManager.getDefault()
                        .getPhoneCount(); i++) {
                    ImsServiceImpl impl = mImsServiceImplMap
                            .get(Integer.valueOf(i + 1));
                    if (impl == null) {
                        continue;
                    }
                    if (ImsManager.isEnhanced4gLteModeSettingEnabledByUser(
                            getApplicationContext())) {// SPRD: bug644353
                        impl.turnOnIms();
                    }
                }
                return;
            }
            Log.d(TAG, "ImsService turnOnIms() phoneId = " + phoneId);
            ImsServiceImpl impl = mImsServiceImplMap.get(Integer.valueOf(phoneId + 1));
            if (impl == null) {
                return;
            }
            Log.d(TAG, "ImsService turnOnIms() impl = " + impl);
            if (ImsManager.isEnhanced4gLteModeSettingEnabledByUser(
                    getApplicationContext(), phoneId)) {
                impl.turnOnIms();
            }
        }

        /**
         * Used for turning off IMS when its in ON state.
         * When IMS is OFF, device will behave as CSFB'ed.
         */
        @Override
        public void turnOffIms(int phoneId) {
            if (!ImsManagerEx.isDualLteModem()) {
                Log.d(TAG, "ImsService turnOffIms() isNotDualLteModem ");
                for (int i = 0; i < TelephonyManager.getDefault()
                        .getPhoneCount(); i++) {
                    ImsServiceImpl impl = mImsServiceImplMap.get(Integer.valueOf(i + 1));
                    if (impl == null) {
                        continue;
                    }
                    impl.turnOffIms();
                }
            }
            Log.d(TAG, "ImsService turnOffIms() phoneId = " + phoneId);
            ImsServiceImpl impl = mImsServiceImplMap
                    .get(Integer.valueOf(phoneId + 1));
            Log.d(TAG, "ImsService turnOffIms() impl = " + impl);
            if (impl == null) {
                return;
            }
            impl.turnOffIms();
        }

        /**
         * ECBM interface for Emergency callback notifications
         */
        @Override
        public IImsEcbm getEcbmInterface(int serviceId) {
            ImsServiceImpl service = mImsServiceImplMap.get(new Integer(serviceId));
            if (service == null) {
                Log.e(TAG, "getEcbmInterface: Invalid argument " + service);
                return null;
            }
            return service.getEcbmInterface();
        }

        /**
         * Used to set current TTY Mode.
         */
        @Override
        public void setUiTTYMode(int serviceId, int uiTtyMode, Message onComplete) {
            ImsServiceImpl service = mImsServiceImplMap.get(new Integer(serviceId));
            if (service == null) {
                Log.e (TAG, "Invalid arguments " + serviceId);
                return;
            }
            service.setUiTTYMode(serviceId, uiTtyMode, onComplete);
        }

        /**
         * MultiEndpoint interface for DEP.
         */
        @Override
        public IImsMultiEndpoint getMultiEndpointInterface(int serviceId){
        	return mImsMultiEndpointBinder;
        }
    };

    private final IImsUtEx.Stub mImsUtExBinder = new IImsUtEx.Stub() {
        /**
         * Retrieves the configuration of the call forward.
         */
        public int setCallForwardingOption(int phoneId, int commandInterfaceCFAction,
                int commandInterfaceCFReason,int serviceClass, String dialingNumber,
                int timerSeconds, String ruleSet){
            ImsServiceImpl service = mImsServiceImplMap.get(new Integer(phoneId +1));
            if (service == null) {
                Log.e (TAG, "Invalid phoneId " + phoneId);
                return -1;
            }
            ImsUtProxy ut = (ImsUtProxy) service.getUTProxy();
            return ut.setCallForwardingOption(commandInterfaceCFAction, commandInterfaceCFReason,
                    serviceClass, dialingNumber, timerSeconds, ruleSet);
        }

        /**
         * Updates the configuration of the call forward.
         */
        public int getCallForwardingOption(int phoneId, int commandInterfaceCFReason, int serviceClass,
                String ruleSet){
            ImsServiceImpl service = mImsServiceImplMap.get(new Integer(phoneId+1));
            if (service == null) {
                Log.e (TAG, "Invalid phoneId " + phoneId);
                return -1;
            }
            ImsUtProxy ut = (ImsUtProxy) service.getUTProxy();
            return ut.getCallForwardingOption(commandInterfaceCFReason, serviceClass, ruleSet);
        }

        /**
        * Sets the listener.
        */
        public void setListenerEx(int phoneId, IImsUtListenerEx listener){
            ImsServiceImpl service = mImsServiceImplMap.get(new Integer(phoneId+1));
            if (service == null) {
                Log.e (TAG, "Invalid phoneId " + phoneId);
                return;
            }
            ImsUtProxy ut = (ImsUtProxy) service.getUTProxy();
            ut.setListenerEx(listener);
        }

        public int setFacilityLock(int phoneId, String facility, boolean lockState, String password,
                int serviceClass){
            ImsServiceImpl service = mImsServiceImplMap.get(new Integer(phoneId +1));
            if (service == null) {
                Log.e (TAG, "Invalid phoneId " + phoneId);
                return -1;
            }
            ImsUtProxy ut = (ImsUtProxy) service.getUTProxy();
            return ut.setFacilityLock(facility, lockState, password, serviceClass);
        }

        public int changeBarringPassword(int phoneId, String facility, String oldPwd, String newPwd){
            ImsServiceImpl service = mImsServiceImplMap.get(new Integer(phoneId +1));
            if (service == null) {
                Log.e (TAG, "Invalid phoneId " + phoneId);
                return -1;
            }
            ImsUtProxy ut = (ImsUtProxy) service.getUTProxy();
            return ut.changeBarringPassword(facility, oldPwd, newPwd);
        }

        public int queryFacilityLock(int phoneId, String facility, String password, int serviceClass){
            ImsServiceImpl service = mImsServiceImplMap.get(new Integer(phoneId +1));
            if (service == null) {
                Log.e (TAG, "Invalid phoneId " + phoneId);
                return -1;
            }
            ImsUtProxy ut = (ImsUtProxy) service.getUTProxy();
            return ut.queryFacilityLock(facility, password, serviceClass);
        }
    };

    public IImsConfig getConfigInterface(int serviceId) {
        ImsServiceImpl service = mImsServiceImplMap.get(new Integer(serviceId));
        if (service == null) {
            Log.e (TAG, "getConfigInterface->Invalid serviceId " + serviceId);
            return null;
        }
        return service.getConfigInterface();
    }

    private final IImsServiceEx.Stub mImsServiceExBinder = new IImsServiceEx.Stub(){

        /**
         * Used for switch IMS feature.
         * @param type:
         * ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE = 0;
         * ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI = 2;
         * @return: request id
         */
        @Override
        public int switchImsFeature(int type){
            // If current ims register state is registed, and same as the switch to, will do nothing.
            if ((mWifiRegistered && type == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI)
                    || (isVoLTERegisted() && type == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE)) {
                // Do nothing, return -1.
                Log.w(TAG, "Needn't switch to type " + type + " as it already registed.");
                return -1;
            } else {
                int id = getReuestId();
                mHandler.obtainMessage(ACTION_SWITCH_IMS_FEATURE, id, type).sendToTarget();
                return id;
            }
        }

        /**
         * Used for start IMS handover.
         * @param targetType:
         * ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE = 0;
         * ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI = 2;
         * @return: request id
         */
        @Override
        public int startHandover(int targetType){
            int id = getReuestId();
            mHandler.obtainMessage(ACTION_START_HANDOVER, id, targetType).sendToTarget();
            return id;
        }

        /**
         * Used for notify network unavailable.
         */
        @Override
        public void notifyNetworkUnavailable(){
            mHandler.obtainMessage(ACTION_NOTIFY_NETWORK_UNAVAILABLE).sendToTarget();
        }

        /**
         * Used for get IMS feature.
         * @return:
         * ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN = -1;
         * ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE = 0;
         * ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI = 2;
         */
        @Override
        public int getCurrentImsFeature(){
            return mCurrentImsFeature;
        }

        /**
         * Used for set IMS service listener.
         */
        @Override
        public void setImsServiceListener(IImsServiceListenerEx listener){
            mImsServiceListenerEx = listener;
        }

        /**
         * Used for set release VoWifi Resource.
         */
        @Override
        public int releaseVoWifiResource(){
            int id = getReuestId();
            mHandler.obtainMessage(ACTION_RELEASE_WIFI_RESOURCE, id, -1).sendToTarget();
            return id;
        }

        /**
         * Used for set VoWifi unavailable.
         * param wifiState:
         * wifi_disabled = 0;
         * wifi_enabled = 1;
         * return: request id
          */
        @Override
        public int setVoWifiUnavailable(int wifiState, boolean isOnlySendAT){
            int id = getReuestId();
            mHandler.obtainMessage(ACTION_NOTIFY_VOWIFI_UNAVAILABLE, id, wifiState, new Boolean(isOnlySendAT)).sendToTarget();
            return id;
        }

         /**
         * Used for get IMS register address.
         */
        @Override
        public String getImsRegAddress(){
            ImsServiceImpl service = mImsServiceImplMap.get(
                Integer.valueOf(ImsRegister.getPrimaryCard(mPhoneCount)+1));
            if(service == null){
                Log.e (TAG, "getImsRegAddress->Invalid service id = " + ImsRegister.getPrimaryCard(mPhoneCount)+1);
                return null;
            }
            return service.getIMSRegAddress();
        }

        /**
         * Used for cancel current switch or handover request.
         * return: request id
         */
        @Override
        public int cancelCurrentRequest(){
            int id = getReuestId();
            mHandler.obtainMessage(ACTION_CANCEL_CURRENT_REQUEST, id, -1).sendToTarget();
            return id;
        }

         /**
         * Used for register IMS register listener.
         */
        @Override
        public void registerforImsRegisterStateChanged(IImsRegisterListener listener){
            if (listener == null) {
                Log.w(TAG,"registerforImsRegisterStateChanged->Listener is null!");
                Thread.dumpStack();
                return;
            }
            Log.i(TAG," registerforImsRegisterStateChanged() -> ");
            synchronized (mImsRegisterListeners) {
                if (!mImsRegisterListeners.keySet().contains(listener.asBinder())) {
                    mImsRegisterListeners.put(listener.asBinder(), listener);
                    Log.i(TAG," registerforImsRegisterStateChanged() -> notifyListenerWhenRegister");
                    notifyListenerWhenRegister(listener);
                } else {
                    Log.w(TAG,"Listener already add :" + listener);
                }
            }
        }

        /**
         * Used for unregister IMS register listener.
         */
        @Override
        public void unregisterforImsRegisterStateChanged(IImsRegisterListener listener){
            if (listener == null) {
                Log.w(TAG,"unregisterforImsRegisterStateChanged->Listener is null!");
                Thread.dumpStack();
                return;
            }

            synchronized (mImsRegisterListeners) {
                if (mImsRegisterListeners.keySet().contains(listener.asBinder())) {
                    mImsRegisterListeners.remove(listener.asBinder());
                } else {
                    Log.w(TAG,"Listener not find " + listener);
                }
            }
        }

        /**
        * Used for terminate VoWifi/Volte calls.
        * param wifiState:
        * wifi_disabled = 0;
        * wifi_enabled = 1;
         */
       @Override
       public void terminateCalls(int wifiState){
           terminateAllCalls(wifiState);
       }

       /**
        * Used for get P-CSCF address.
        * return: P-CSCF address
        */
       @Override
       public String getCurPcscfAddress(){
           String address = null;
           if(mWifiService != null){
               address = mWifiService.getCurPcscfAddress();
           }
           return address;
       }

       /**
        * Used for set monitor period millis.
         */
       @Override
       public void setMonitorPeriodForNoData(int millis){
           // Needn't, remove later.
       }

       /* SPRD: add for VoWiFi @{ */
       @Override
       public void showVowifiNotification(){
           Log.i(TAG,"showVowifiNotification");
           mNotificationManager =(NotificationManager)getSystemService(NOTIFICATION_SERVICE);
           mNotificationManager.cancel(mCurrentVowifiNotification);
           Intent intent = new Intent();
           intent.setClassName("com.android.settings","com.android.settings.Settings$WifiCallingSettingsActivity");
           intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
           ImsManager.setWfcSetting(ImsService.this,false);
           PendingIntent contentIntent = PendingIntent.getActivity(ImsService.this, 0, intent, 0);
           CharSequence vowifiTitle = getText(R.string.vowifi_attation);
           CharSequence vowifiContent = getText(R.string.vowifi_regist_fail_content);
           Notification notification = new Notification(android.R.drawable.stat_sys_warning,mVowifiRegisterMsg, System.currentTimeMillis());
           notification.flags = Notification.FLAG_AUTO_CANCEL;
           notification.setLatestEventInfo(ImsService.this, vowifiTitle, vowifiContent, contentIntent);
           mNotificationManager.notify(mCurrentVowifiNotification, notification);
       }
       /* @} */

       /**
        * Used for get local address.
        */
       @Override
       public String getCurLocalAddress(){
           if(mWifiService != null){
               return mWifiService.getCurLocalAddress();
           }
           return "";
       }

       /**
        * Get current IMS video state.
        * return: video state
        * {VideoProfile#STATE_AUDIO_ONLY},
        * {VideoProfile#STATE_BIDIRECTIONAL},
        * {VideoProfile#STATE_TX_ENABLED},
        * {VideoProfile#STATE_RX_ENABLED},
        * {VideoProfile#STATE_PAUSED}.
        */
       @Override
       public int getCurrentImsVideoState(){
           CallManager cm = CallManager.getInstance();
           int videoState = VideoProfile.STATE_AUDIO_ONLY;
           Call call = null;

           if(cm.hasActiveRingingCall()){
               call = cm.getFirstActiveRingingCall();
           } else if(cm.hasActiveFgCall()){
               call = cm.getActiveFgCall();
           } else if(cm.hasActiveBgCall()){
               call = cm.getFirstActiveBgCall();
           }

           if(call != null && call.getLatestConnection() != null){
               videoState = call.getLatestConnection().getVideoState();
           }
           return videoState;
       }

       @Override
       public int getAliveCallLose() {
           Log.i(TAG,"getAliveCallLose->mIsVowifiCall:" + mIsVowifiCall + " mIsVolteCall:" + mIsVolteCall);
           int aliveCallLose = -1;
           ImsServiceImpl imsService = mImsServiceImplMap.get(
                   Integer.valueOf(ImsRegister.getPrimaryCard(mPhoneCount)+1));
           if (mIsVowifiCall) {
               if (mWifiService != null) {
                   aliveCallLose = mWifiService.getAliveCallLose();
               } else {
                   Log.i(TAG,"getAliveCallLose->VowifiServiceImpl is null");
               }
           } else if(mIsVolteCall) {
               if (imsService != null) {
                   aliveCallLose = imsService.getAliveCallLose();
               } else {
                   Log.i(TAG,"getAliveCallLose->ImsServiceImpl is null");
               }
           }
           return aliveCallLose;
       }

       @Override
       public int getAliveCallJitter() {
           Log.i(TAG,"getAliveCallJitter->mIsVowifiCall:" + mIsVowifiCall + " mIsVolteCall:" + mIsVolteCall);
           int aliveCallJitter = -1;
           ImsServiceImpl imsService = mImsServiceImplMap.get(
                   Integer.valueOf(ImsRegister.getPrimaryCard(mPhoneCount)+1));
           if (mIsVowifiCall) {
               if (mWifiService != null) {
                   aliveCallJitter = mWifiService.getAliveCallJitter();
               } else {
                   Log.i(TAG,"getAliveCallJitter->VowifiServiceImpl is null");
               }
           } else if(mIsVolteCall) {
               if (imsService != null) {
                   aliveCallJitter = imsService.getAliveCallJitter();
               } else {
                   Log.i(TAG,"getAliveCallJitter->ImsServiceImpl is null");
               }
           }
           return aliveCallJitter;
       }

       @Override
       public int getAliveCallRtt() {
           Log.i(TAG,"getAliveCallRtt->mIsVowifiCall:" + mIsVowifiCall + " mIsVolteCall:" + mIsVolteCall);
           int aliveCallRtt = -1;
           ImsServiceImpl imsService = mImsServiceImplMap.get(
                   Integer.valueOf(ImsRegister.getPrimaryCard(mPhoneCount)+1));
           if (mIsVowifiCall) {
               if (mWifiService != null) {
                   aliveCallRtt = mWifiService.getAliveCallRtt();
               } else {
                   Log.i(TAG,"getAliveCallRtt->VowifiServiceImpl is null");
               }
           } else if(mIsVolteCall) {
               if (imsService != null) {
                   aliveCallRtt = imsService.getAliveCallRtt();
               } else {
                   Log.i(TAG,"getAliveCallRtt->ImsServiceImpl is null");
               }
           }
           return aliveCallRtt;
       }

       @Override
       public int getVolteRegisterState(){
           ImsServiceImpl imsService = mImsServiceImplMap.get(
                   Integer.valueOf(ImsRegister.getPrimaryCard(mPhoneCount)+1));
           int volteRegisterState = -1;
           if (imsService != null){
               volteRegisterState = imsService.getVolteRegisterState();
           } else {
               Log.i(TAG,"getVolteRegisterState->ImsServiceImpl is null");
           }
           return volteRegisterState;
       }

       @Override
       public int getCallType(){
           if(mIsVolteCall){
               return CallType.VOLTE_CALL;
           }else if(mIsVowifiCall){
               return CallType.WIFI_CALL;
           }
           return CallType.NO_CALL;
       }

       /**
        * notify SRVCC Call Info
        */
       @Override
       public void notifySrvccCallInfos(List<ImsSrvccCallInfo> list){
            ImsServiceImpl imsService = mImsServiceImplMap.get(
                    Integer.valueOf(ImsRegister.getPrimaryCard(mPhoneCount)+1));
            if(list != null && imsService != null){
                String commands = "";
                for(int i=0;i<list.size();i++){
                    commands = commands + ((ImsSrvccCallInfo)list.get(i)).toAtCommands();
                    if(i != list.size()-1){
                        commands = commands +",";
                    }
                }
                Log.i(TAG,"notifySrvccCallInfos->commands:"+commands);
                imsService.notifyHandoverCallInfo(commands);
            }
        }

        /**
         * Used for get local address.
         */
       public String getImsPcscfAddress(){
            ImsServiceImpl imsService = mImsServiceImplMap.get(
                    Integer.valueOf(ImsRegister.getPrimaryCard(mPhoneCount)+1));
            if(imsService != null){
                return imsService.getImsPcscfAddress();
            }
            return "";
        }
        /**
         * used for set register or de-regesiter Vowifi
         * para action
         * 0 de-register before start call
         * 1 register after call end
         * **/
       public void setVowifiRegister(int action) {
           try {
               if (mImsServiceListenerEx != null) {
                   mImsServiceListenerEx.onSetVowifiRegister(action);
               }
           } catch (RemoteException e) {
               e.printStackTrace();
           }
       }
        /**
         * used for VOWIFI get CLIR states from CP
         * return: ut request id
         *
         * **/
        @Override
        public int getCLIRStatus(int phoneId) {
            ImsServiceImpl imsService = mImsServiceImplMap.get(
                    Integer.valueOf(phoneId+1));
            int id = -1;
            Log.i(TAG, "getCLIRStatus phoneId = " + phoneId);
            if (imsService != null) {
                ImsUtImpl ut = imsService.getUtImpl();
                if (ut != null) {
                    id = ut.getCLIRStatus();
                    return id;
                }
            }
            return id;
        }



        public int updateCLIRStatus(int action) {
            int id = -1;
            Log.i(TAG, "updateCLIRStatus action = " + action);
            if (mWifiService != null) {
                id = mWifiService.updateCurCLIRMode(action);
            }
            return id;
        }

        //SPRD: add for bug 771875
        @Override
        public void notifyVideoCapabilityChange(){
            mHandler.removeMessages(ACTION_NOTIFY_VIDEO_CAPABILITY_CHANGE);
            mHandler.sendMessageDelayed(mHandler.obtainMessage(ACTION_NOTIFY_VIDEO_CAPABILITY_CHANGE),100);
        }
    };

    private final IImsMultiEndpoint.Stub mImsMultiEndpointBinder = new IImsMultiEndpoint.Stub()  {
        /**
         * Sets the listener.
         */
    	@Override
        public void setListener(IImsExternalCallStateListener listener){
    	}


        /**
         * Query api to get the latest Dialog Event Package information
         * Should be invoked only after setListener is done
         */
    	@Override
        public void requestImsExternalCallStateInfo(){
    	}


    };

    private void notifyListenerWhenRegister(IImsRegisterListener listener){
        Log.i(TAG," notifyListenerWhenRegister() -> ");
        // SPRD 708609 when switch data card, VOLTE icon also display
//        updateImsRegisterState();
        boolean isImsRegistered = ((mCurrentImsFeature == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE)
                || (mCurrentImsFeature == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI));
        Log.i(TAG," notifyListenerWhenRegister() -> isImsRegistered: "+isImsRegistered);
        synchronized (mImsRegisterListeners) {
            try{
                listener.imsRegisterStateChange(isImsRegistered);
            } catch(RemoteException e){
                e.printStackTrace();
            }
        }
    }

    public void notifyImsRegisterState(){
        Log.i(TAG," notifyImsRegisterState() -> ");
        updateImsRegisterState();
        boolean isImsRegistered = ((mCurrentImsFeature == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE)
                || (mCurrentImsFeature == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI));
        //SPRD: add for bug 771875
        if(ImsManagerEx.isDualLteModem()){
            Log.i(TAG," notifyImsRegisterState() -> isDualLteModem is ok ");
            isImsRegistered  = isImsRegistered || mVolteRegistered;
        }
        synchronized (mImsRegisterListeners) {
            /**
             * SPRD bug647508
             */
            for (IImsRegisterListener l : mImsRegisterListeners.values()) {
                try{
                    l.imsRegisterStateChange(isImsRegistered);
                } catch(RemoteException e){
                    iLog("DeadObjectException : l = " + l);
                    e.printStackTrace();
                    continue;
                }
            }
        }
    }

    public void updateImsRegisterState(){
        Log.i(TAG,"updateImsRegisterState() -> ");
        synchronized (mImsRegisterListeners) {
            for(Integer id : mImsServiceImplMap.keySet()){
                ImsServiceImpl service = mImsServiceImplMap.get(id);
                Log.i(TAG,"     -> for loop service.isImsRegisterState() = " + service.isImsRegisterState());
                if (service.isImsRegisterState()) {
                    mVolteRegistered = true;
                    return;
                }
            }
            Log.i(TAG,"     -> for loop end mVolteRegistered = " + mVolteRegistered);
            mVolteRegistered = false;
        }
    }

    private int getReuestId(){
        synchronized(mRequestLock){
            mRequestId++;
            if(mRequestId > 100){
                mRequestId = 0;
            }
        }
        return mRequestId;
    }

class MyVoWifiCallback implements VoWifiCallback {
        @Override
        public void onAttachFinished(boolean success, int errorCode) {
            if (success) {
                // Attach success, send the event to handler.
                SecurityConfig config = mWifiService.getSecurityConfig();
                mHandler.obtainMessage(EVENT_WIFI_ATTACH_SUCCESSED, -1, -1, config)
                        .sendToTarget();
            } else {
                // Attach failed, send the event to handler.
                mHandler.obtainMessage(EVENT_WIFI_ATTACH_FAILED, errorCode, -1, null)
                    .sendToTarget();
            }
        }

        @Override
        public void onAttachStopped(int stoppedReason) {
            mHandler.obtainMessage(EVENT_WIFI_ATTACH_STOPED).sendToTarget();
        }

        @Override
        public void onAttachStateChanged(int state) {
            mHandler.obtainMessage(EVENT_WIFI_ATTACH_STATE_UPDATE, state, -1, null).sendToTarget();
        }

        @Override
        public void onRegisterStateChanged(int state, int errorCode) {
            mHandler.obtainMessage(EVENT_WIFI_REGISTER_RESAULT, errorCode, -1,
                    new Boolean(state == RegisterState.STATE_CONNECTED)).sendToTarget();
        }

        @Override
        public void onReregisterFinished(boolean isSuccess, int errorCode) {
            mHandler.obtainMessage(EVENT_WIFI_REFRESH_RESAULT, errorCode, -1,
                    new Boolean(isSuccess)).sendToTarget();
        }

        @Override
        public void onResetFinished(int result, int errorCode){
            mHandler.obtainMessage(EVENT_WIFI_RESET_RESAULT, result, errorCode, null).sendToTarget();
        }

        /*SPRD: Add for notify data router{@*/
        @Override
        public void onUpdateDRStateFinished() {
            mHandler.obtainMessage(EVENT_UPDATE_DATA_ROUTER_FINISHED).sendToTarget();
        }
        /*@}*/

        @Override
        public void onCallIncoming(String callId,int type){
            mHandler.obtainMessage(EVENT_WIFI_INCOMING_CALL, type, -1, callId).sendToTarget();
        }

        @Override
        public void onAliveCallUpdate(boolean isVideoCall){
            onVideoStateChanged(isVideoCall ? VideoProfile.STATE_BIDIRECTIONAL : VideoProfile.STATE_AUDIO_ONLY);
        }

        @Override
        public void onAllCallsEnd(){
            mHandler.obtainMessage(EVENT_WIFI_ALL_CALLS_END).sendToTarget();
        }

        @Override
        public void onMediaQualityChanged(boolean isVideo, int lose, int jitter, int rtt) {
            try{
                if (mImsServiceListenerEx != null) {
                    mImsServiceListenerEx.onMediaQualityChanged(isVideo,lose,jitter,rtt);
                }
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }

        /**
         * notify the DPD disconnect event.
         */
        //@Override
        public void onDPDDisconnected(){
            mHandler.obtainMessage(EVENT_WIFI_DPD_DISCONNECTED).sendToTarget();
        }

        /**
         * notify no RTP event.
         */
        public void onNoRtpReceived(boolean isVideo){
            mHandler.obtainMessage(EVENT_WIFI_NO_RTP, -1, -1,
                    new Boolean(isVideo)).sendToTarget();
        }

        public void onRtpReceived(boolean isVideo) {
            mHandler.obtainMessage(EVENT_WIFI_RTP_RECEIVED, -1, -1,
                    new Boolean(isVideo)).sendToTarget();
        }

        @Override
        public void onUnsolicitedUpdate(int stateCode) {
            mHandler.obtainMessage(EVENT_WIFI_UNSOL_UPDATE, stateCode, -1).sendToTarget();
        }
    }

    class VoLTERegisterListener implements ImsServiceImpl.Listener {
        @Override
        public void onRegisterStateChange(int serviceId){
            try{
                /*SPRD: Modify for bug596304{@*/
                ImsServiceImpl service = mImsServiceImplMap.get(Integer.valueOf(serviceId));
                Log.i(TAG,"VoLTERegisterListener-> mFeatureSwitchRequest:"+mFeatureSwitchRequest
                        +" mIsCalling:"+ mIsCalling + " mVolteRegistered:" + mVolteRegistered + " service.isImsRegistered():" + service.isImsRegistered()
                        + " mIsLoggingIn:" + mIsLoggingIn +" mIsPendingRegisterVolte:"+mIsPendingRegisterVolte);
                if(service.getVolteRegisterState() == IMS_REG_STATE_REGISTERING
                        || service.getVolteRegisterState() == IMS_REG_STATE_DEREGISTERING
                        || (mIsVolteCall && service.isImsRegistered()) || mIsWifiCalling){
                    Log.i(TAG,"VoLTERegisterListener-> pending status service.getVolteRegisterState():"+service.getVolteRegisterState()
                            +" mIsVolteCall:"+mIsVolteCall +" mIsWifiCalling:"+mIsWifiCalling);
                    return;
                }
                //SPRD:add for bug674494
                if(mFeatureSwitchRequest == null && mIsPendingRegisterVolte){
                    mIsPendingRegisterVolte = false;
                }
                //If CP reports CIREGU as 1,3 , IMS Feature will be updated as Volte registered state firstly.
                if (service.getVolteRegisterState() == IMS_REG_STATE_REGISTERED || service.getVolteRegisterState() == IMS_REG_STATE_REG_FAIL){
                    mVolteRegistered = (service.getVolteRegisterState() == IMS_REG_STATE_REGISTERED);
                    // SPRD: 730973
                    service.setVolteRegisterStateOld(service.isImsRegistered());
                    if(!mIsLoggingIn){
                        if (ImsManagerEx.isDualVoLTEActive()){
                            updateImsFeature(serviceId);
                        } else {
                            updateImsFeature(serviceId);
                        }
                    }
                } else {
                    // SPRD: 730973
//                    if (mVolteRegistered != service.isImsRegistered()){
                    if (service.getVolteRegisterStateOld() != service.isImsRegistered()){
                        service.setVolteRegisterStateOld(service.isImsRegistered());
                        mVolteRegistered = service.isImsRegistered();
                        if(!mIsLoggingIn){
                            if (ImsManagerEx.isDualVoLTEActive()){
                                updateImsFeature(serviceId);
                            } else {
                                updateImsFeature(serviceId);
                            }
                        }
                    }
                }
                /*@}*/
                if(mFeatureSwitchRequest != null &&
                        mFeatureSwitchRequest.mServiceId == serviceId
                        && mFeatureSwitchRequest.mTargetType
                        == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE){
                    if(service.isImsRegistered()){
                        if (mIsPendingRegisterVolte && mWifiService != null) {
                            mWifiService.resetAll(WifiState.DISCONNECTED);
                        } else {
                            mWifiService.deregister();
                            mWifiService.deattach();
                            mWifiRegistered= false;//Set wifi registered state as false when make de-register operation in handover.
                            if (ImsManagerEx.isDualVoLTEActive()){
                                updateImsFeature(serviceId);
                            } else {
                                updateImsFeature(serviceId);
                            }
                        }
                        mIsPendingRegisterVolte = false;
                        if(mImsServiceListenerEx != null){
                            if(mFeatureSwitchRequest.mEventCode == ACTION_SWITCH_IMS_FEATURE){
                                Log.i(TAG, "VoLTERegisterListener -> operationSuccessed -> IMS_OPERATION_SWITCH_TO_VOLTE");
                                mImsServiceListenerEx.operationSuccessed(mFeatureSwitchRequest.mRequestId,
                                        ImsOperationType.IMS_OPERATION_SWITCH_TO_VOLTE);
                                mFeatureSwitchRequest = null;
                            }
                         } else {
                             Log.w(TAG, "VoLTERegisterListener -> operationSuccessed, mImsServiceListenerEx is null!");
                         }
                    } else if(mImsServiceListenerEx != null){
                        if(mFeatureSwitchRequest.mEventCode == ACTION_SWITCH_IMS_FEATURE){
                            Log.i(TAG, "VoLTERegisterListener -> operationFailed -> IMS_OPERATION_SWITCH_TO_VOLTE");
                            mImsServiceListenerEx.operationFailed(mFeatureSwitchRequest.mRequestId,
                                    "VoLTE register failed",
                                    ImsOperationType.IMS_OPERATION_SWITCH_TO_VOLTE);
                            mFeatureSwitchRequest = null;
                            mIsPendingRegisterVolte = false;
                        }
                    } else {
                        Log.w(TAG, "VoLTERegisterListener -> operationFailed, mImsServiceListenerEx is null!");
                    }
                    Log.i(TAG,"VoLTERegisterListener-> mPendingActivePdnSuccess"+ mPendingActivePdnSuccess
                            +" mIsCPImsPdnActived:"+mIsCPImsPdnActived+" mIsCalling:"+mIsCalling);
                     if(!mPendingActivePdnSuccess && mIsCPImsPdnActived && mFeatureSwitchRequest != null
                             && mFeatureSwitchRequest.mEventCode == ACTION_START_HANDOVER && !mIsCalling){//SPRD:add for bug718074
                         Log.i(TAG, "VoLTERegisterListener -> ACTION_START_HANDOVER, clear mFeatureSwitchRequest");
                         mFeatureSwitchRequest = null;
                         mIsPendingRegisterVolte = false;
                     }
                }
                Log.i(TAG,"VoLTERegisterListener-> mCurrentImsFeature:"+mCurrentImsFeature
                        + "serviceId:"+serviceId + " service is null:"+(service == null)
                         + " service:"+(service == null ? false : service.isImsRegistered()));
            } catch(RemoteException e){
                e.printStackTrace();
            }
        }

        @Override
        public void onSessionEmpty(int serviceId){
            ImsServiceImpl imsService = mImsServiceImplMap.get(Integer
                    .valueOf(serviceId));
            boolean volteRegistered = (imsService != null) ? imsService.isImsRegisterState() : false;
            if((serviceId-1) == ImsRegister.getPrimaryCard(mPhoneCount)){
                /*SPRD: Add for bug586758 and 595321{@*/
                ImsServiceImpl impl = mImsServiceImplMap.get(Integer.valueOf(serviceId));
                Log.i(TAG,"onSessionEmpty-> serviceId: "+serviceId + " mIsVowifiCall:" + mIsVowifiCall + " mIsVolteCall:" + mIsVolteCall +"  impl: "+impl);
                if (impl != null) {
                    if (impl.isVolteSessionListEmpty() && impl.isVowifiSessionListEmpty()) {
                        mCallEndType = CallEndEvent.VOLTE_CALL_END;

                        /*SPRD: Modify for bug595321 and 610799{@*/
                        Log.i(TAG,"onSessionEmpty-> mFeatureSwitchRequest:"+mFeatureSwitchRequest + " mIsVowifiCall:" + mIsVowifiCall + " mIsVolteCall:" + mIsVolteCall +" mInCallHandoverFeature" + mInCallHandoverFeature);
                        if(mFeatureSwitchRequest != null && mFeatureSwitchRequest.mEventCode == ACTION_START_HANDOVER && serviceId == mFeatureSwitchRequest.mServiceId){
                            if (mInCallHandoverFeature != mFeatureSwitchRequest.mTargetType) {
                                if (mFeatureSwitchRequest.mTargetType == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI) {
                                    mPendingAttachVowifiSuccess = true;
                                } else if (mFeatureSwitchRequest.mTargetType == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE) {
                                    mPendingActivePdnSuccess = true;
                                    if(mIsVolteCall){
                                        mPendingVolteHandoverVolteSuccess = true;
                                    }
                                }
                            }
                            if(mFeatureSwitchRequest.mTargetType == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI){
                                if (mIsVolteCall && mIsPendingRegisterVowifi) {
                                    mWifiService.register();
                                    mIsLoggingIn = true;
                                    mFeatureSwitchRequest = null;
                                }
                                mIsPendingRegisterVowifi = false;
                            }
                            Log.i(TAG,"onSessionEmpty-> mPendingAttachVowifiSuccess:" + mPendingAttachVowifiSuccess + " mPendingActivePdnSuccess:" + mPendingActivePdnSuccess + " mIsLoggingIn:" + mIsLoggingIn);
                            if (mIsVolteCall && mFeatureSwitchRequest != null
                                    && mFeatureSwitchRequest.mTargetType == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE
                                    && !mPendingAttachVowifiSuccess && !mPendingActivePdnSuccess) {
                                mFeatureSwitchRequest = null;
                                Log.i(TAG,"onSessionEmpty-> This is volte call,so mFeatureSwitchRequest has been emptyed.");
                            }
                        }
                        //SPRD:modify by bug687400
                        if(mCurrentImsFeature != ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN){
                            Log.i(TAG,"onSessionEmpty->mCurrentImsFeature:"+mCurrentImsFeature);
                            updateInCallState(false);
                        }
                        mInCallHandoverFeature = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
                        if (!mPendingAttachVowifiSuccess && !mPendingActivePdnSuccess ) {
                            mWifiService.updateDataRouterState(DataRouterState.CALL_NONE);
                        }
                        Log.i(TAG,"onSessionEmpty->serviceId: " + serviceId + "mIsVolteCall: " + mIsVolteCall + " mIsVowifiCall:" + mIsVowifiCall);
                        /*@}*/
                        if (mIsVowifiCall) {
                            mIsVowifiCall = false;
                        } else if (mIsVolteCall) {
                            mIsVolteCall = false;
                        }
                    }
                } else {
                    Log.i(TAG,"onSessionEmpty->ImsServiceImpl is null");
                }
                /*@}*/
                Log.i(TAG,"onSessionEmpty->serviceId: " + serviceId + "mIsVolteCall: " + mIsVolteCall + " mIsVowifiCall:" + mIsVowifiCall
                        + "mInCallHandoverFeature: " + mInCallHandoverFeature);
            }else{
                ImsServiceImpl implPrimay = mImsServiceImplMap.get(Integer.valueOf(ImsRegister.getPrimaryCard(mPhoneCount)+1));
                Log.i(TAG,"onSessionEmpty-> serviceId: "+serviceId + " mIsVolteCall:" + mIsVolteCall +"  imsService: "+imsService+"  volteRegistered: "+volteRegistered
                       +"  implPrimay: "+implPrimay);
                if(volteRegistered && (imsService != null && (imsService.isVolteSessionListEmpty())) && (implPrimay != null && (implPrimay.isVolteSessionListEmpty()))){
                     if (mIsVolteCall) {
                         mIsVolteCall = false;
                     }
                }
            }
        }
    }

    public boolean isImsEnabled(){
        return (mCurrentImsFeature == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE
                || mCurrentImsFeature == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI);
    }

    public boolean isVoWifiEnabled(){
        return (mCurrentImsFeature == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI);
    }

    public boolean isVoLTEEnabled(){
        return (mCurrentImsFeature == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE);
    }

    //SPRD: add for bug 771875
    public void updateImsFeatureForAllService() {
        synchronized (mImsRegisterListeners) {
            for (Integer id : mImsServiceImplMap.keySet()) {
                ImsServiceImpl service = mImsServiceImplMap.get(id);
                updateImsFeature(service.getServiceId());
            }
        }
    }

    public void updateImsFeature(){
        int serviceId = Integer.valueOf(ImsRegister.getPrimaryCard(mPhoneCount)+1);
        updateImsFeature(serviceId);
    }

    public void updateImsFeature(int serviceId){
        ImsServiceImpl imsService = mImsServiceImplMap.get(Integer
                .valueOf(serviceId));
        boolean isPrimaryCard = ImsRegister.getPrimaryCard(mPhoneCount) == (serviceId-1);
        Log.i(TAG,"updateImsFeature --> isPrimaryCard = " + ImsRegister.getPrimaryCard(mPhoneCount) + " | serviceId-1 = " + (serviceId-1));
        boolean volteRegistered = (imsService != null) ? imsService.isImsRegisterState() : false;

        if (!isPrimaryCard && imsService != null) {
            Log.i(TAG,
                    "updateImsFeature->isPrimaryCard:" + isPrimaryCard
                            + " volteRegistered:" + volteRegistered
                            + " serviceId:" + serviceId
                            + " | mTelephonyManagerEx.getLTECapabilityForPhone = "
                            + mTelephonyManagerEx
                                    .getLTECapabilityForPhone(serviceId - 1));
            if(ImsManagerEx.isDualVoLTEActive() || mTelephonyManagerEx.getLTECapabilityForPhone(serviceId - 1)) {
                imsService.updateImsFeatures(volteRegistered, false);
                imsService.notifyImsRegister(volteRegistered);
                notifyImsRegisterState();
            } else {
                if(imsService.isImsEnabled()) {
                    imsService.updateImsFeatures(false, false);
                }
            }
            return;
        }
        updateImsRegisterState();
        // add for Dual LTE
//        ImsServiceImpl imsService = mImsServiceImplMap.get(
//                Integer.valueOf(serviceId));
//        ImsServiceImpl imsService = null;
//        if (mTelephonyManagerEx.getLTECapabilityForPhone(serviceId - 1)) {
//            imsService = mImsServiceImplMap.get(Integer.valueOf(serviceId));
//        } else {
//            imsService = mImsServiceImplMap.get(
//                    Integer.valueOf(ImsRegister.getPrimaryCard(mPhoneCount)+1));
//        }
        int oldImsFeature = mCurrentImsFeature;//SPRD:add for bug673215
        boolean isImsRegistered = false;
        if(mInCallHandoverFeature != ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN){
            if(mInCallHandoverFeature == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI){
                mCurrentImsFeature = ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI;
                isImsRegistered = true; //SPRD: add for bug 771875
            } else if (imsService != null && imsService.getSrvccState() == VoLteServiceState.HANDOVER_COMPLETED){
                mCurrentImsFeature = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
                isImsRegistered = false; //SPRD: add for bug 771875
            }else {
                mCurrentImsFeature = ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE;
                isImsRegistered = true; //SPRD: add for bug 771875
            }
        } else if (volteRegistered) {
            mCurrentImsFeature = ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE;
//            if(imsService != null) imsService.notifyImsRegister(true);
            /*SPRD: Bug 667760 If dual volte avtive, need to notify ims state according serviceId.*/
            if (imsService != null) {
                if (ImsManagerEx.isDualVoLTEActive()) {
                    isImsRegistered = imsService.isImsRegisterState(); //SPRD: add for bug 771875
                } else {
                    isImsRegistered = true; //SPRD: add for bug 771875
                }
            }
            /* @} */
        } else if(mWifiRegistered){
            mCurrentImsFeature = ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI;
            isImsRegistered = true; //SPRD: add for bug 771875
        } else {
            mCurrentImsFeature = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
            isImsRegistered = false; //SPRD: add for bug 771875
        }
        if(imsService != null) {
            imsService.updateImsFeatures(mCurrentImsFeature == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE,
                    mCurrentImsFeature == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI);

            //SPRD: add for bug671964
            if(mCurrentImsFeature == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI){
                if(mWifiService != null){
                    String addr = mWifiService.getCurPcscfAddress();
                    imsService.setImsPcscfAddress(addr);
                }
            }
        }
        imsService.notifyImsRegister(isImsRegistered); //SPRD: add for bug 771875
        Log.i(TAG," notifyImsRegisterState again isImsRegistered: "+isImsRegistered );
        notifyImsRegisterState();

        Log.i(TAG,"updateImsFeature-> serviceId = " + serviceId + " | mWifiRegistered:"+mWifiRegistered +" | VolteRegistered:"+volteRegistered
                +" | mCurrentImsFeature:"+mCurrentImsFeature +" mInCallHandoverFeature:"+mInCallHandoverFeature);
    }

    private boolean isVoLTERegisted() {
        boolean volteRegistered = false;
        for (Integer id : mImsServiceImplMap.keySet()) {
            if(mImsServiceImplMap.get(id).isImsRegisterState()){
                volteRegistered = true;
                break;
            }
        }
        return volteRegistered;
    }

    public void onReceiveHandoverEvent(boolean isCalling, int requestId, int targetType){
        Log.i(TAG,"onReceiveHandoverEvent->isCalling:" + isCalling + " requestId:" + requestId
                +" targetType:" + targetType + " mCurrentImsFeature:" + mCurrentImsFeature
                + " mPendingActivePdnSuccess:"+mPendingActivePdnSuccess+" mPendingAttachVowifiSuccess:"+mPendingAttachVowifiSuccess);
        if (!mIsCalling && mPendingActivePdnSuccess){
            mPendingActivePdnSuccess =  false;
            if(mCallEndType != -1){
                Log.i(TAG,"onReceiveHandoverEvent-> mCallEndType:"+mCallEndType);
                notifyCpCallEnd();
            }
        }
        if (!mIsCalling && mPendingAttachVowifiSuccess){
            mPendingAttachVowifiSuccess =  false;
            if(mCallEndType != -1){
                Log.i(TAG,"onReceiveHandoverEvent-> mCallEndType:"+mCallEndType);
                notifyCpCallEnd();
            }
        }
        mFeatureSwitchRequest = new ImsServiceRequest(requestId,
                isCalling ? ACTION_START_HANDOVER : ACTION_SWITCH_IMS_FEATURE /*eventCode*/,
                        ImsRegister.getPrimaryCard(mPhoneCount)+1/*serviceId*/,
                        targetType);
        ImsServiceImpl service = mImsServiceImplMap.get(
                Integer.valueOf(mFeatureSwitchRequest.mServiceId));
        if(mFeatureSwitchRequest.mTargetType ==
                ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE){
            service.requestImsHandover(isCalling ? ImsHandoverType.INCALL_HANDOVER_TO_VOLTE
                    : ImsHandoverType.IDEL_HANDOVER_TO_VOLTE);
        } else if(mFeatureSwitchRequest.mTargetType ==
                ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI){
            service.requestImsHandover(isCalling ? ImsHandoverType.INCALL_HANDOVER_TO_VOWIFI
                    : ImsHandoverType.IDEL_HANDOVER_TO_VOWIFI);
        }
    }

    public void onImsHandoverStateChange(boolean allow, int state){
        Log.i(TAG,"onImsHandoverStateChange->allow:"+allow + " state:"+state
                +" mFeatureSwitchRequest:"+mFeatureSwitchRequest);
        if(mFeatureSwitchRequest == null){
            Log.w(TAG,"onImsHandoverStateChange->there is no handover request active!");
            return;
        }
        try{
            if(!allow){
                if(mImsServiceListenerEx != null){
                    if(mFeatureSwitchRequest.mTargetType ==
                            ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE){
                        mImsServiceListenerEx.operationFailed(mFeatureSwitchRequest.mRequestId, "Do not allow.",
                                (mFeatureSwitchRequest.mEventCode == ACTION_SWITCH_IMS_FEATURE)
                                ? ImsOperationType.IMS_OPERATION_SWITCH_TO_VOLTE
                                        : ImsOperationType.IMS_OPERATION_HANDOVER_TO_VOLTE);
                    } else {
                    mImsServiceListenerEx.operationFailed(mFeatureSwitchRequest.mRequestId, "Do not allow.",
                            (mFeatureSwitchRequest.mEventCode == ACTION_SWITCH_IMS_FEATURE)
                            ? ImsOperationType.IMS_OPERATION_CP_REJECT_SWITCH_TO_VOWIFI
                                    : ImsOperationType.IMS_OPERATION_CP_REJECT_HANDOVER_TO_VOWIFI);
                    }
                    mFeatureSwitchRequest = null;
                } else {
                    Log.w(TAG,"onImsHandoverStateChange->mImsServiceListenerEx is null!");
                }
            } else if(mFeatureSwitchRequest.mEventCode == ACTION_SWITCH_IMS_FEATURE){
                if(mFeatureSwitchRequest.mTargetType ==
                        ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE){
                    if(state == ImsHandoverResult.IMS_HANDOVER_PDN_BUILD_FAIL ||
                            state == ImsHandoverResult.IMS_HANDOVER_REGISTER_FAIL){
                        if(mImsServiceListenerEx != null){
                            mImsServiceListenerEx.operationFailed(mFeatureSwitchRequest.mRequestId, "VOLTE pdn failed.",
                                    ImsOperationType.IMS_OPERATION_SWITCH_TO_VOLTE);
                        }
                        mFeatureSwitchRequest = null;
                    }
                } else if(mFeatureSwitchRequest.mTargetType ==
                        ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI){
                    if(state == IMS_HANDOVER_ACTION_CONFIRMED){
                        if (SystemProperties.getBoolean(PROP_S2B_ENABLED, true)) {
                            mWifiService.attach();
                        } else {
                            mWifiService.register();
                        }
                    }
                }
            } else if(mFeatureSwitchRequest.mEventCode == ACTION_START_HANDOVER){
                if(mFeatureSwitchRequest.mTargetType ==
                        ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE){
                    if(state == ImsHandoverResult.IMS_HANDOVER_PDN_BUILD_FAIL ||
                            state == ImsHandoverResult.IMS_HANDOVER_REGISTER_FAIL){
                        mImsServiceListenerEx.operationFailed(mFeatureSwitchRequest.mRequestId, "VOLTE pdn failed.",
                                ImsOperationType.IMS_OPERATION_HANDOVER_TO_VOLTE);
                        mFeatureSwitchRequest = null;
                        if (!mIsCalling && mPendingActivePdnSuccess){
                            mPendingActivePdnSuccess = false;
                            mWifiService.updateDataRouterState(DataRouterState.CALL_NONE);
                            Log.i(TAG,"onImsHandoverStateChange->ACTION_START_HANDOVER fail,mPendingActivePdnSuccess is true!");
                        }
                    } else if(state == ImsHandoverResult.IMS_HANDOVER_SRVCC_FAILED){
                        mImsServiceListenerEx.onSrvccFaild();
                        Log.i(TAG,"onImsHandoverStateChange->IMS_HANDOVER_SRVCC_FAILED.");
                    }
                } else if(mFeatureSwitchRequest.mTargetType ==
                        ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI){
                    if (state == IMS_HANDOVER_ACTION_CONFIRMED && SystemProperties.getBoolean(PROP_S2B_ENABLED, true)) {
                        mWifiService.attach();
                    }
                }
            }
        } catch(RemoteException e){
            e.printStackTrace();
        }
    }

    public void onImsPdnStatusChange(int serviceId,int state){
        boolean isAirplaneModeOn = false;
        if(state == ImsPDNStatus.IMS_PDN_READY){
            mIsAPImsPdnActived = false;
            mIsCPImsPdnActived = true;
            mPendingCPSelfManagement = false;
        }else{
            mIsCPImsPdnActived = false;

            //SPRD: add for bug642021
            isAirplaneModeOn = Settings.Global.getInt(getApplicationContext().getContentResolver(),
                    Settings.Global.AIRPLANE_MODE_ON, 0) > 0;
            if(state == ImsPDNStatus.IMS_PDN_ACTIVE_FAILED ||isAirplaneModeOn){
                if(mPendingCPSelfManagement || mFeatureSwitchRequest == null && !mWifiRegistered){
                    ImsServiceImpl service = mImsServiceImplMap.get(
                            Integer.valueOf(ImsRegister.getPrimaryCard(mPhoneCount)+1));
                    if(service != null && !mIsCalling){//SPRD: add for bug717045
                       service.setIMSRegAddress(null);
                    }
                }
                mPendingCPSelfManagement = false;
            }
        }
        Log.i(TAG,"onImsPdnStatusChange->serviceId:"+serviceId +" state:" + state
                + " mFeatureSwitchRequitchRequest:" + mFeatureSwitchRequest
                + " mIsCalling:" + mIsCalling
                + " mIsCPImsPdnActived:" + mIsCPImsPdnActived + " mIsAPImsPdnActived:" + mIsAPImsPdnActived
                + " mWifiRegistered:" + mWifiRegistered + " mVolteRegistered:" + mVolteRegistered
                + " mPendingCPSelfManagement:" + mPendingCPSelfManagement
                + " mPendingActivePdnSuccess:"+mPendingActivePdnSuccess+" isAirplaneModeOn:"+isAirplaneModeOn
                + " mInCallHandoverFeature:"+mInCallHandoverFeature);
        try{
            if (mImsServiceListenerEx != null &&
                    serviceId == ImsRegister.getPrimaryCard(mPhoneCount)+1) {
                mImsServiceListenerEx.imsPdnStateChange(state);
            }
        } catch(RemoteException e){
            e.printStackTrace();
        }
        // If the switch request is null, we will start the volte call as default.
        if (mFeatureSwitchRequest == null && state == ImsPDNStatus.IMS_PDN_READY) {
            // add for Dual LTE
            ImsServiceImpl service = null;
            if (mTelephonyManagerEx.getLTECapabilityForPhone(serviceId - 1)) {
                service = mImsServiceImplMap.get(Integer.valueOf(serviceId));
            } else {
                service = mImsServiceImplMap.get(Integer
                        .valueOf(ImsRegister.getPrimaryCard(mPhoneCount) + 1));
            }
            /*SPRD: Modify for bug599233{@*/
            Log.i(TAG,"onImsPdnStatusChange->mIsPendingRegisterVolte:" + mIsPendingRegisterVolte + " service.isImsRegistered():" + service.isImsRegistered());
            // If pdn is ready when handover from vowifi to volte but volte is not registered , never to turn on ims.
            // If Volte is registered , never to turn on ims.
            Log.i(TAG,"onImsPdnStatusChange->mIsVolteCall:"+mIsVolteCall +" mIsVowifiCall:"+mIsVowifiCall +" mIsAPImsPdnActived:"+mIsAPImsPdnActived);

            if(!((mIsPendingRegisterVolte && !service.isImsRegistered()) || service.isImsRegistered()) && !mIsVolteCall && !mIsVowifiCall){
                Log.d(TAG, "Switch request is null, but the pdn start, will enable the ims.");
                service.enableImsWhenPDNReady();
            }
            /*@}*/
        } else if(mFeatureSwitchRequest != null && mFeatureSwitchRequest.mTargetType
                == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE){
            if(state == ImsPDNStatus.IMS_PDN_ACTIVE_FAILED){
                if (mImsServiceListenerEx != null) {
                    try {
                        Log.i(TAG, "onImsPdnStatusChange -> operationFailed");
                        mImsServiceListenerEx.operationFailed(
                                mFeatureSwitchRequest.mRequestId,
                                "VOLTE pdn failed.",
                                (mFeatureSwitchRequest.mEventCode == ACTION_START_HANDOVER)?
                                        ImsOperationType.IMS_OPERATION_HANDOVER_TO_VOLTE:
                                            ImsOperationType.IMS_OPERATION_SWITCH_TO_VOLTE);
                    } catch (RemoteException e) {
                        e.printStackTrace();
                    }
                } else {
                    Log.w(TAG, "onImsPdnStatusChange->mImsServiceListenerEx is null!");
                }
                if (!mIsCalling && mFeatureSwitchRequest.mEventCode == ACTION_START_HANDOVER){
                    mPendingActivePdnSuccess = false;
                    mWifiService.updateDataRouterState(DataRouterState.CALL_NONE);
                }
                mFeatureSwitchRequest = null;
                mIsPendingRegisterVolte = false;
            } else if(state == ImsPDNStatus.IMS_PDN_READY){
                ImsServiceImpl service = mImsServiceImplMap.get(
                        Integer.valueOf(ImsRegister.getPrimaryCard(mPhoneCount)+1));
                if(mFeatureSwitchRequest.mEventCode == ACTION_SWITCH_IMS_FEATURE){
                    service.turnOnIms();
                    if (mImsServiceListenerEx != null) {
                        try {
                            Log.i(TAG, "onImsPdnStatusChange -> operationSuccessed-> IMS_OPERATION_SWITCH_TO_VOLTE");
                            mImsServiceListenerEx.operationSuccessed(mFeatureSwitchRequest.mRequestId,
                                    ImsOperationType.IMS_OPERATION_SWITCH_TO_VOLTE);
                        } catch (RemoteException e) {
                            e.printStackTrace();
                        }
                    }
                } else if(mFeatureSwitchRequest.mEventCode == ACTION_START_HANDOVER){
                    /*SPRD: Modify for bug595321{@*/
                    int oldImsFeature = mCurrentImsFeature;
                    if (mIsCalling){
                        mInCallHandoverFeature = ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE;
                        mIsPendingRegisterVolte = true;
                        updateImsFeature(serviceId);
                        if (mImsServiceListenerEx != null) {
                            try {
                                Log.i(TAG, "onImsPdnStatusChange -> operationSuccessed-> IMS_OPERATION_HANDOVER_TO_VOLTE");
                                mImsServiceListenerEx.operationSuccessed(mFeatureSwitchRequest.mRequestId,
                                        ImsOperationType.IMS_OPERATION_HANDOVER_TO_VOLTE);
                                if(oldImsFeature != mCurrentImsFeature){
                                    Toast.makeText(ImsService.this, R.string.handover_to_volte_success,Toast.LENGTH_SHORT).show();
                                }
                            } catch (RemoteException e) {
                                e.printStackTrace();
                            }
                        }
                        mWifiService.deattach(true);
                        mPendingActivePdnSuccess = false;
                        mWifiService.updateDataRouterState(DataRouterState.CALL_VOLTE);
                        if(mIsVolteCall){
                            ImsServiceImpl reportService = mImsServiceImplMap.get(
                                    Integer.valueOf(mFeatureSwitchRequest.mServiceId));
                            reportService.disableWiFiParamReport();
                        }
                    } else {
                        mIsPendingRegisterVolte = true;
                        updateImsFeature(serviceId);
                        mCurrentImsFeature = ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE;
                        Log.i(TAG, "onImsPdnStatusChange -> mCurrentImsFeature:" + ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE + " mIsCalling:" + mIsCalling);
                        if (mImsServiceListenerEx != null) {
                            try {
                                Log.i(TAG, "onImsPdnStatusChange -> operationSuccessed-> IMS_OPERATION_HANDOVER_TO_VOLTE");
                                mImsServiceListenerEx.operationSuccessed(mFeatureSwitchRequest.mRequestId,
                                        ImsOperationType.IMS_OPERATION_HANDOVER_TO_VOLTE);
                                if(oldImsFeature != mCurrentImsFeature){
                                    Toast.makeText(ImsService.this, R.string.handover_to_volte_success,Toast.LENGTH_SHORT).show();
                                }
                            } catch (RemoteException e) {
                                e.printStackTrace();
                            }
                        }
                        mWifiService.deattach(true);
                        mPendingActivePdnSuccess = false;
                        mWifiService.updateDataRouterState(DataRouterState.CALL_NONE);
                    }
                    /*@}*/
                }
            }
        }
        //SPR:add for bug718067
        if (mIsCalling && mInCallHandoverFeature == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE && state == ImsPDNStatus.IMS_PDN_ACTIVE_FAILED) {
            Log.i(TAG, "onImsPdnStatusChange -> handvoer to Volte failed,set mInCallHandoverFeature unknow");
            mInCallHandoverFeature = ImsConfig.FeatureConstants.FEATURE_TYPE_UNKNOWN;
            updateImsFeature();
        }
    }

    public void onImsCallEnd(int serviceId){
        try{
            if (mImsServiceListenerEx != null) {
                mImsServiceListenerEx.imsCallEnd(
                        ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE);
            }
        } catch(RemoteException e){
            e.printStackTrace();
        }
    }

    public void onImsNetworkInfoChange(int type, String info){
        Log.i(TAG, "onImsNetworkInfoChange->type:" + type + " info:" + info);
        if (mFeatureSwitchRequest != null) {
            Log.i(TAG,"onImsNetworkInfoChange->mFeatureSwitchRequest:"
                            + mFeatureSwitchRequest.toString() + " mCurrentImsFeature:"
                            + mCurrentImsFeature);
            Log.i(TAG,"onImsNetworkInfoChange->type: " + type + " info: " + info);
            mNetworkType = type;
            mNetworkInfo = info;
        } else {
            Log.i(TAG, "onImsNetworkInfoChange->mFeatureSwitchRequest is null.");
        }
    }

    public void notifyWiFiError(int statusCode){
        try{
            if (mImsServiceListenerEx != null) {
                mImsServiceListenerEx.onVoWiFiError(statusCode);
            }
        } catch(RemoteException e){
            e.printStackTrace();
        }
    }
    /*SPRD: Add for bug586758{@*/
    public boolean isVowifiCall() {
        return mIsVowifiCall;
    }
    public boolean isVolteCall() {
        return mIsVolteCall;
    }
    /*@}*/
    private void iLog(String log){
        Log.i(TAG, log);
    }
    public IImsServiceListenerEx getImsServiceListenerEx() {
        return mImsServiceListenerEx;
    }

    /**
     * Used for terminate all calls.
     * param wifiState:
     * wifi_disabled = 0;
     * wifi_enabled = 1;
      */
    public void terminateAllCalls(int wifiState){
        Log.i(TAG,"terminateAllCalls->mIsVolteCall:" + mIsVolteCall + " mIsVowifiCall:" + mIsVowifiCall + " wifiState:" + wifiState);
        if(mIsVolteCall){
            ImsServiceImpl service = mImsServiceImplMap.get(
                    Integer.valueOf(ImsRegister.getPrimaryCard(mPhoneCount)+1));
            if(service != null){
                service.terminateVolteCall();
            }else{
                Log.i(TAG,"terminateAllCalls->ImsServiceImpl is null.");
            }
        }else if(mIsVowifiCall){
            VoWifiServiceImpl.WifiState state;
            if(wifiState == 1){
                state = VoWifiServiceImpl.WifiState.CONNECTED;
            } else {
                state = VoWifiServiceImpl.WifiState.DISCONNECTED;
            }
            if(mWifiService != null){
                mWifiService.terminateCalls(state);
            }else{
                Log.i(TAG,"terminateAllCalls->VowifiServiceImpl is null.");
            }
        }
    }

    public void notifyCPVowifiAttachSucceed(){
        Log.i(TAG,"EVENT_NOTIFY_CP_VOWIFI_ATTACH_SUCCESSED-> notifyImsHandoverStatus:" + ImsHandoverResult.IMS_HANDOVER_ATTACH_SUCCESS);
        ImsServiceImpl Impl = mImsServiceImplMap.get(
                Integer.valueOf(ImsRegister.getPrimaryCard(mPhoneCount)+1));
        Impl.notifyImsHandoverStatus(ImsHandoverResult.IMS_HANDOVER_ATTACH_SUCCESS);
    }

    public void notifyCpCallEnd(){
        int primaryPhoneId = ImsRegister.getPrimaryCard(mPhoneCount);
        Log.i(TAG,"notifyCpCallEnd->mCallEndType:"+mCallEndType +" primaryPhoneId:"+primaryPhoneId +" mIsCalling:"+mIsCalling);
        if(mCallEndType != -1 && !mIsCalling){
            ImsServiceImpl impl = mImsServiceImplMap.get(Integer.valueOf(primaryPhoneId+1));
            if(impl != null){
                impl.notifyImsCallEnd(mCallEndType);
                try{
                    if (mImsServiceListenerEx != null) {
                        mImsServiceListenerEx.imsCallEnd((mCallEndType == CallEndEvent.VOLTE_CALL_END) ?
                                ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE : ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI);
                    }
                } catch(RemoteException e){
                    e.printStackTrace();
                }
                mCallEndType = -1;
            } else {
                Log.w(TAG,"notifyCpCallEnd->notifyImsCallEnd-> ImsServiceImpl is null");
            }
        }
    }

    public boolean allowEnableIms(int serviceId){
//        ImsServiceImpl service = mImsServiceImplMap.get(
//                Integer.valueOf(ImsRegister.getPrimaryCard(mPhoneCount)+1));
        // add for Dual LTE
        ImsServiceImpl service = null;
        if (mTelephonyManagerEx.getLTECapabilityForPhone(serviceId)) {
            service = mImsServiceImplMap.get(Integer.valueOf(serviceId + 1));
        } else {
            service = mImsServiceImplMap.get(
                    Integer.valueOf(ImsRegister.getPrimaryCard(mPhoneCount)+1));
        }
        Log.i(TAG,"allowEnableIms->service:"+service +" mFeatureSwitchRequest:"+mFeatureSwitchRequest);
        if(mFeatureSwitchRequest != null || service == null){
            return false;
        }
        if(mIsVolteCall || mIsVowifiCall || mIsAPImsPdnActived){
            Log.i(TAG,"allowEnableIms->mIsVolteCall:"+mIsVolteCall +" mIsVowifiCall:"+mIsVowifiCall + " mIsAPImsPdnActived:" +mIsAPImsPdnActived);
            return false;
        }
        Log.i(TAG,"allowEnableIms->mIsPendingRegisterVolte:"+mIsPendingRegisterVolte+
                " service.isImsRegistered():"+service.isImsRegistered());
        if(!((mIsPendingRegisterVolte && !service.isImsRegistered()) || service.isImsRegistered())){
            return true;
        }
        return false;
    }

    public void updateInCallState(boolean isInCall){
        Log.i(TAG,"updateInCallState->mIsVolteCall:"+mIsVolteCall +" mIsVowifiCall:"+mIsVowifiCall + " isInCall:"+isInCall+"  mIsEmergencyCallonCP: "+mIsEmergencyCallonCP);
        if(mIsCalling != isInCall){
            mIsCalling = isInCall;
            if(mIsCalling &&
                    (mCurrentImsFeature != ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI
                    || mInCallPhoneId != ImsRegister.getPrimaryCard(mPhoneCount) ||mIsEmergencyCallonCP)){//SPRD: Modify for bug753958
                mWifiService.updateIncomingCallAction(IncomingCallAction.REJECT);
            } else {
                mWifiService.updateIncomingCallAction(IncomingCallAction.NORMAL);
            }
        }

        if ((!mIsEmergencyCallonCP && mCurrentImsFeature == ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_WIFI && isInCall != mIsWifiCalling)
                || (!isInCall && mIsWifiCalling)) {//SPRD: Modify for bug753958
            mIsWifiCalling = isInCall;
            for (Map.Entry<Integer, ImsServiceImpl> entry : mImsServiceImplMap.entrySet()) {
                entry.getValue().notifyWifiCalling(mIsWifiCalling);
            }
        }

        ImsServiceImpl imsService = mImsServiceImplMap.get(
                Integer.valueOf(ImsRegister.getPrimaryCard(mPhoneCount) + 1));
        if(!mIsCalling && imsService != null
                && imsService.getSrvccState() == VoLteServiceState.HANDOVER_COMPLETED){
            imsService.setSrvccState(-1);
        }
        /**
         * SPRD: 673414 687400
         */
        if (!isInCall) {
            if (mIsVowifiCall) {
                mIsVowifiCall = false;
            } else if (mIsVolteCall) {
                mIsVolteCall = false;
            }
        }

        if(!isInCall){
            ImsServiceImpl service = mImsServiceImplMap.get(
                    Integer.valueOf(ImsRegister.getPrimaryCard(mPhoneCount)+1));
            service.disableWiFiParamReport();
        }

        iLog("updateInCallState->isInCall:"+isInCall+" mIsWifiCalling:"+mIsWifiCalling
                +" inCallPhoneId:"+mInCallPhoneId + " mIsVolteCall: " + mIsVolteCall);
    }

    public void notifySrvccCapbility(int cap){
        if (mWifiService != null) {
            mWifiService.setSRVCCSupport(cap != 0 ? true : false);
        }
    }

    public void notifySrvccState(int phoneId,int status){
        int primaryPhoneId = ImsRegister.getPrimaryCard(mPhoneCount);
        iLog("notifySrvccState->phoneId:"+phoneId+" primaryPhoneId:"+primaryPhoneId
                +" status:"+status);
        if(phoneId == primaryPhoneId+1){
            updateImsFeature(phoneId);
            mWifiService.onSRVCCStateChanged(status);
            if(mWifiRegistered) {
                mWifiRegistered = false; // SPRD:add for bug659097
            }
        }
    }
    public void onVideoStateChanged(int videoState){
        Log.i(TAG,"onVideoStateChanged videoState:" + videoState);
        try{
            if (mImsServiceListenerEx != null) {
                mImsServiceListenerEx.onVideoStateChanged(videoState);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    // SPRD Add for bug696648
    public boolean moreThanOnePhoneHasCall() {
        int count = 0;
        for(int i = 1 ; i <= mImsServiceImplMap.size(); i ++) {
            ImsServiceImpl imsServiceImpl = mImsServiceImplMap.get(i);
            if(imsServiceImpl != null && imsServiceImpl.hasCall())
                count ++;
        }
        Log.i(TAG,"moreThanOnePhoneHasCall count=" + count);
        return count > 1;
    }
}

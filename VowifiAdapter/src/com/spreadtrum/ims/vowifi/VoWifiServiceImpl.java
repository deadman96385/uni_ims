
package com.spreadtrum.ims.vowifi;

import android.annotation.TargetApi;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.RemoteException;
import android.preference.PreferenceManager;
import android.util.Log;

import com.android.ims.ImsCallProfile;
import com.android.ims.ImsManager;
import com.android.ims.internal.IImsCallSessionListener;
import com.android.ims.internal.IImsServiceEx;
import com.android.ims.internal.ImsCallSession.State;
import com.android.ims.internal.ImsManagerEx;
import com.spreadtrum.ims.ImsConfigImpl;
import com.spreadtrum.ims.vowifi.Utilities.CallStateForDataRouter;
import com.spreadtrum.ims.vowifi.Utilities.CallType;
import com.spreadtrum.ims.vowifi.Utilities.ECBMRequest;
import com.spreadtrum.ims.vowifi.Utilities.IPVersion;
import com.spreadtrum.ims.vowifi.Utilities.NativeErrorCode;
import com.spreadtrum.ims.vowifi.Utilities.RegisterConfig;
import com.spreadtrum.ims.vowifi.Utilities.RegisterState;
import com.spreadtrum.ims.vowifi.Utilities.Result;
import com.spreadtrum.ims.vowifi.Utilities.S2bType;
import com.spreadtrum.ims.vowifi.Utilities.SecurityConfig;
import com.spreadtrum.ims.vowifi.Utilities.UnsolicitedCode;
import com.spreadtrum.ims.vowifi.VoWifiCallManager.CallListener;
import com.spreadtrum.ims.vowifi.VoWifiRegisterManager.RegisterListener;
import com.spreadtrum.ims.vowifi.VoWifiSecurityManager.SecurityListener;

/**
 * The VoWifi only enabled for primary SIM card.
 */
@TargetApi(23)
public class VoWifiServiceImpl implements OnSharedPreferenceChangeListener {
    private static final String TAG = Utilities.getTag(VoWifiServiceImpl.class.getSimpleName());

    private static final int CMD_STATE_INVALID = 0;
    private static final int CMD_STATE_PROGRESS = 1;
    private static final int CMD_STATE_FINISHED = 2;

    private static final int RESET_TIMEOUT = 5000; // 5s
    private static final int RESET_STEP_INVALID = 0;
    private static final int RESET_STEP_DEREGISTER = 1;
    private static final int RESET_STEP_DEATTACH = 2;
    private static final int RESET_STEP_FORCE_RESET = 3;

    private int mResetStep = RESET_STEP_INVALID;
    private int mEcbmStep = ECBMRequest.ECBM_STEP_INVALID;
    private int mCmdAttachState = CMD_STATE_INVALID;
    private int mCmdRegisterState = CMD_STATE_INVALID;
    private int mSubId = -1;
    private boolean mIsSRVCCSupport = true;
    private boolean mNeedLogoutAfterCall = false;
    private String mUsedLocalAddr = null;

    private Context mContext;
    private SharedPreferences mPreferences = null;
    private VoWifiCallback mCallback = null;
    private ECBMRequest mECBMRequest = null;

    private ImsUtImpl mImsUt;
    private ImsEcbmImpl mImsEcbm;
    private VoWifiCallManager mCallMgr;
    private VoWifiXCAPManager mXCAPMgr;
    private VoWifiRegisterManager mRegisterMgr;
    private VoWifiSecurityManager mSecurityMgr;

    private MyHandler mHandler = null;
    private MyCallListener mCallListener = new MyCallListener();
    private MyRegisterListener mRegisterListener = new MyRegisterListener();
    private MySecurityListener mSecurityListener = new MySecurityListener();

    private static final int ECBM_TIMEOUT = 60 * 1000; // 60s

    private static final int MSG_RESET = 1;
    private static final int MSG_RESET_FORCE = 2;
    private static final int MSG_ATTACH = 3;
    private static final int MSG_DEATTACH = 4;
    private static final int MSG_REGISTER = 5;
    private static final int MSG_DEREGISTER = 6;
    private static final int MSG_REREGISTER = 7;
    private static final int MSG_UPDATE_DATAROUTER_STATE = 8;
    private static final int MSG_TERMINATE_CALLS = 9;
    private static final int MSG_ECBM = 10;
    private static final int MSG_ECBM_TIMEOUT = 11;

    private class MyHandler extends Handler {
        public MyHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            if (Utilities.DEBUG) Log.i(TAG, "The handler get the message: " + msg.what);
            if (!ImsManager.isWfcEnabledByPlatform(mContext)) {
                Log.d(TAG, "WFC disabled by platform. Do nothing.");
                return;
            }

            switch (msg.what) {
                case MSG_RESET:
                    mResetStep = msg.arg1;
                    handleMsgReset(mResetStep);
                    break;
                case MSG_RESET_FORCE:
                    // Force stop the register and s2b.
                    mResetStep = RESET_STEP_FORCE_RESET;
                    registerForceStop();
                    securityForceStop();
                    break;
                case MSG_ATTACH:
                    attachInternal();
                    break;
                case MSG_DEATTACH:
                    boolean forHandover = (Boolean) msg.obj;
                    deattachInternal(forHandover);
                    break;
                case MSG_REGISTER:
                    registerPrepare();
                    break;
                case MSG_DEREGISTER:
                    deregisterInternal();
                    break;
                case MSG_REREGISTER:
                    int type = msg.arg1;
                    String info = (String) msg.obj;
                    reRegisterInternal(type, info);
                    break;
                case MSG_UPDATE_DATAROUTER_STATE:
                    int state = msg.arg1;
                    mCallMgr.updateDataRouterState(state);
                    if (mCallback != null) mCallback.onUpdateDRStateFinished();
                    break;
                case MSG_TERMINATE_CALLS:
                    WifiState wifiState = (WifiState) msg.obj;
                    try {
                        mCallMgr.terminateCalls(wifiState);
                    } catch (RemoteException e) {
                        Log.e(TAG, "Catch the RemoteException when terminate calls. e: " + e);
                    }
                    break;
                case MSG_ECBM:
                    mEcbmStep = msg.arg1;
                    handleMsgECBM(mEcbmStep);
                    break;
                case MSG_ECBM_TIMEOUT:
                    handleMsgECBMTimeout(msg.arg1);
                    break;
            }
        }

        private void handleMsgReset(int step) {
            Log.d(TAG, "Handle the reset message as step: " + step);
            if (mResetStep == RESET_STEP_DEREGISTER) {
                if (mRegisterMgr.getCurRegisterState() == RegisterState.STATE_CONNECTED) {
                    deregisterInternal();
                    // If the de-register is timeout, force reset.
                    mHandler.sendEmptyMessageDelayed(MSG_RESET_FORCE, RESET_TIMEOUT);
                } else {
                    // As do not register, we'd like to force reset.
                    Log.d(TAG, "Do not register now, transfer to force reset.");
                    mHandler.sendEmptyMessage(MSG_RESET_FORCE);
                }
            } else if (mResetStep == RESET_STEP_DEATTACH) {
                // Remove the reset force msg from the handler.
                mHandler.removeMessages(MSG_RESET_FORCE);

                // De-attach from the EPDG.
                deattachInternal(false);
            } else {
                mResetStep = RESET_STEP_INVALID;
                Log.e(TAG, "Shouldn't be here. reset step is: " + mResetStep);
            }
        }

        private void handleMsgECBM(int step) {
            // For enter ECBM, we need de-register & de-attach. Then attach & register for
            // emergency. After register success finished, start the emergency call.
            // Then try to exit ECBM, we need de-register & de-attach. Then attach & register
            // for normal.
            Log.d(TAG, "Handle the ECBM message as step: " + step);

            switch (step) {
                case ECBMRequest.ECBM_STEP_DEREGISTER_SOS:
                case ECBMRequest.ECBM_STEP_DEREGISTER_NORMAL:
                    deregisterInternal();
                    break;
                case ECBMRequest.ECBM_STEP_DEATTACH_SOS:
                case ECBMRequest.ECBM_STEP_DEATTACH_NORMAL:
                    deattachInternal(false);
                    break;
                case ECBMRequest.ECBM_STEP_ATTACH_SOS:
                case ECBMRequest.ECBM_STEP_ATTACH_NORMAL:
                    attachInternal();
                    break;
                case ECBMRequest.ECBM_STEP_REGISTER_SOS:
                case ECBMRequest.ECBM_STEP_REGISTER_NORMAL:
                    registerPrepare();
                    break;
                case ECBMRequest.ECBM_STEP_START_EMERGENCY_CALL:
                    ImsCallSessionImpl callSession = mImsEcbm.getEmergencyCall();
                    if (callSession == null || callSession.getState() > State.IDLE) {
                        Log.e(TAG, "Failed to start the emergency call: " + callSession);
                        // Handle this error as meet ecbm timeout.
                        mHandler.sendEmptyMessage(MSG_ECBM_TIMEOUT);
                        break;
                    }

                    callSession.dialEmergencyCall();
                    break;
                case ECBMRequest.ECBM_STEP_INVALID:
                    exitECBM();
                    break;
            }
        }

        private void handleMsgECBMTimeout(int forStep) {
            Log.d(TAG, "Handle the ECBM timeout message, for step: " + forStep
                    + ", and current ECBM step: " + mEcbmStep);
            // Exit the ECBM, and it will terminate the emergency call session.
            exitECBM();

            // Reset the security and sip stack.
            resetAll(WifiState.DISCONNECTED);
        }
    }

    public VoWifiServiceImpl(Context context) {
        mContext = context;

        HandlerThread thread = new HandlerThread("VoWifi");
        thread.start();
        Looper looper = thread.getLooper();
        mHandler = new MyHandler(looper);

        init();
    }

    @Override
    protected void finalize() throws Throwable {
        uninit();

        if (mHandler != null) {
            mHandler.removeCallbacksAndMessages(null);
            Looper looper = mHandler.getLooper();
            if (looper != null) {
                looper.quit();
            }
            mHandler = null;
        }

        super.finalize();
    }

    @Override
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
        // If the video quality changed, we need update the video quality.
        if (ImsConfigImpl.VT_RESOLUTION_VALUE.equals(key)) {
            // The video quality is changed.
            mCallMgr.updateVideoQuality(Utilities.getDefaultVideoQuality(mPreferences));
        }
    }

    public void registerCallback(VoWifiCallback callback) {
        if (Utilities.DEBUG) Log.i(TAG, "Register the vowifi service callback: " + callback);
        if (callback == null) {
            Log.e(TAG, "Can not register the callback as it is null.");
            return;
        }

        mCallback = callback;
    }

    public void unregisterCallback() {
        if (Utilities.DEBUG) Log.i(TAG, "Unregister the vowifi service callback: " + mCallback);
        mCallback = null;
    }

    /**
     * Ut interface for the supplementary service configuration.
     */
    public ImsUtImpl getUtInterface() {
        if (!ImsManager.isWfcEnabledByPlatform(mContext)) return null;

        if (mImsUt == null) {
            mImsUt = new ImsUtImpl(mContext, mXCAPMgr);
        }

        return mImsUt;
    }

    public ImsEcbmImpl getEcbmInterface() {
        if (!ImsManager.isWfcEnabledByPlatform(mContext)) return null;

        if (mImsEcbm == null) {
            mImsEcbm = new ImsEcbmImpl(mContext);
        }

        return mImsEcbm;
    }

    public void resetAll(WifiState state) {
        resetAll(state, 0/* Do not delay */);
    }

    /**
     * Reset the s2b and sip stack. If WIFI is disable, the de-register and de-attach couldn't
     * be sent. So we'd like to force reset the states.
     *
     * @param wifiState disable or enable.
     */
    public void resetAll(WifiState state, int delayMillis) {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Reset the security and sip stack with wifi state: "
                    + (WifiState.CONNECTED.equals(state) ? "connect" : "disconnect")
                    + ", delay millis: " + delayMillis);
        }

        if (state == null) {
            throw new NullPointerException("The wifi state couldn't be null.");
        }

        // Update the register state to call manager as it will be reset later.
        if (mCallMgr != null) mCallMgr.updateRegisterState(RegisterState.STATE_IDLE);

        // Before reset the sip stack, we'd like to terminate all the calls.
        terminateCalls(WifiState.CONNECTED);

        // Do not care the wifi state, we will try to de-register for reset first.
        Message msg = mHandler.obtainMessage(MSG_RESET);
        msg.arg1 = RESET_STEP_DEREGISTER;
        mHandler.sendMessageDelayed(msg, delayMillis);
    }

    public void attach() {
        mCmdAttachState = CMD_STATE_PROGRESS;
        mHandler.sendEmptyMessage(MSG_ATTACH);
    }

    private void attachInternal() {
        // Before start attach process, need get the SIM account info.
        // We will always use the primary card to attach and register now.
        mSubId = Utilities.getPrimaryCardSubId(mContext);
        if (mSubId < 0) {
            Log.e(TAG, "Can not get the sub id.");
            if (mCallback != null) mCallback.onAttachFinished(false, 0);
            return;
        }

        int s2bType = mEcbmStep == ECBMRequest.ECBM_STEP_ATTACH_SOS ? S2bType.SOS : S2bType.NORMAL;
        mSecurityMgr.attach(mSubId, s2bType, mUsedLocalAddr, mSecurityListener);
    }

    public void deattach() {
        deattach(false);
    }

    public void deattach(boolean forHandover) {
        mHandler.sendMessage(mHandler.obtainMessage(MSG_DEATTACH, forHandover));
    }

    private void deattachInternal(boolean forHandover) {
        int s2bType =
                mEcbmStep == ECBMRequest.ECBM_STEP_DEATTACH_SOS ? S2bType.SOS : S2bType.NORMAL;
        mSecurityMgr.deattach(mSubId, s2bType, forHandover, mSecurityListener);
    }

    /**
     * To force stop the s2b even it in the connect progress.
     */
    private void securityForceStop() {
        mSecurityMgr.forceStop(mSubId, mSecurityListener);
    }

    public SecurityConfig getSecurityConfig() {
        return mSecurityMgr == null ? null : mSecurityMgr.getConfig(mSubId, S2bType.NORMAL);
    }

    public RegisterConfig getRegisterConfig() {
        return mRegisterMgr == null ? null : mRegisterMgr.getCurRegisterConfig();
    }

    public void register() {
        mCmdRegisterState = CMD_STATE_PROGRESS;
        mHandler.sendEmptyMessage(MSG_REGISTER);
    }

    /**
     * Register to the IMS service.
     */
    private void registerPrepare() {
        // Check the security state, if it is attached. We could start the register process.
        int s2bType =
                mEcbmStep == ECBMRequest.ECBM_STEP_REGISTER_SOS ? S2bType.SOS : S2bType.NORMAL;
        SecurityConfig securityConfir = mSecurityMgr.getConfig(mSubId, s2bType);
        if (securityConfir == null) {
            Log.e(TAG, "Before register, need attach success, please check the attach state!");
            if (mCallback != null) mCallback.onReregisterFinished(false, 0);
            return;
        }

        // For register process, step as this:
        // 1. Prepare for login.
        // 2. If prepare finished, start the login process with the first local IP and PCSCF IP.
        // 3. If login failed, need try the left local IP and PCSCF IP until all of them already
        //    failed, then need notify the user login failed.
        RegisterConfig regConf = RegisterConfig.getInstance(securityConfir._ip4,
                securityConfir._ip6, securityConfir._pcscf4, securityConfir._pcscf6,
                getUsedPcscfAddr(), securityConfir._dns4, securityConfir._dns6);
        mRegisterMgr.prepareForLogin(mSubId, mIsSRVCCSupport, regConf, mRegisterListener);
    }

    public void deregister() {
        // Before de-register action, terminate all the calls.
        terminateCalls(WifiState.CONNECTED);
        // Handle the de-register action.
        mHandler.sendEmptyMessage(MSG_DEREGISTER);
    }

    /**
     * Unregister to the IMS service.
     */
    private void deregisterInternal() {
        mRegisterMgr.deregister(mRegisterListener);
    }

    /**
     * Refresh the register status.
     */
    public void reRegister() {
        reRegister(-1, null);
    }

    public void reRegister(int type, String info) {
        mHandler.sendMessage(mHandler.obtainMessage(MSG_REREGISTER, type, -1, info));
    }

    /**
     * Refresh the register status with the given network info.
     */
    private void reRegisterInternal(int type, String info) {
        mRegisterMgr.reRegister(type, info);
    }

    /**
     * To cancel the current register process. If already register, it will stop and close the
     * sip stack but not de-register.
     *
     * @return true if stop and close the sip stack, otherwise false.
     */
    private boolean registerForceStop() {
        return mRegisterMgr.forceStop(mRegisterListener);
    }

    public String getCurLocalAddress() {
        if (mRegisterMgr != null
                && mRegisterMgr.getCurRegisterState() == RegisterState.STATE_CONNECTED
                && getRegisterConfig() != null) {
            return getRegisterConfig().getCurUsedLocalIP();
        }
        return "";
    }

    public String getCurPcscfAddress() {
        if (mRegisterMgr != null
                && mRegisterMgr.getCurRegisterState() == RegisterState.STATE_CONNECTED
                && getRegisterConfig() != null) {
            return getRegisterConfig().getCurUsedPcscfIP();
        }
        return "";
    }

    /**
     * Create a new call session for the given profile.
     */
    public ImsCallSessionImpl createCallSession(ImsCallProfile profile,
            IImsCallSessionListener listener) {
        return mCallMgr == null ? null : mCallMgr.createMOCallSession(profile, listener);
    }

    public ImsCallSessionImpl getPendingCallSession(String callId) {
        return mCallMgr == null ? null : mCallMgr.getCallSession(callId);
    }

    public void terminateCalls(WifiState state) {
        if (state == null) {
            throw new NullPointerException("The wifi state couldn't be null.");
        }

        mHandler.sendMessage(mHandler.obtainMessage(MSG_TERMINATE_CALLS, state));
    }

    public int getCallCount() {
        if (Utilities.DEBUG) Log.i(TAG, "Get call count.");

        return mCallMgr == null ? 0 : mCallMgr.getCallCount();
    }

    public int getCurrentImsVideoState() {
        if (Utilities.DEBUG) Log.i(TAG, "Get alive call type.");

        if (mCallMgr == null) return -1;

        ImsCallSessionImpl callSession = mCallMgr.getCurrentCallSession();
        if (callSession == null) {
            Log.w(TAG, "There isn't any call, return unknown.");
            return -1;
        }

        ImsVideoCallProviderImpl videoProvider = callSession.getVideoCallProviderImpl();
        if (videoProvider == null) {
            Log.w(TAG, "There isn't video provider, return unknown.");
            return -1;
        }

        return videoProvider.getVideoState();
    }

    public int getAliveCallType() {
        if (Utilities.DEBUG) Log.i(TAG, "Get alive call type.");

        if (mCallMgr == null) return CallType.CALL_TYPE_UNKNOWN;

        ImsCallSessionImpl callSession = mCallMgr.getAliveCallSession();
        if (callSession == null) {
            Log.w(TAG, "There isn't any call in alive state, return unknown.");
            return CallType.CALL_TYPE_UNKNOWN;
        }

        boolean isVideo = Utilities.isVideoCall(callSession.getCallProfile().mCallType);
        return isVideo ? CallType.CALL_TYPE_VIDEO : CallType.CALL_TYPE_VOICE;
    }

    public int getAliveCallLose() {
        return mCallMgr == null ? 0 : mCallMgr.getPacketLose();
    }

    public int getAliveCallJitter() {
        return mCallMgr == null ? 0 : mCallMgr.getJitter();
    }

    public int getAliveCallRtt() {
        return mCallMgr == null ? 0 : mCallMgr.getRtt();
    }

    public void updateIncomingCallAction(IncomingCallAction action) {
        if (action == null) {
            throw new NullPointerException("The incoming call action couldn't be null.");
        }

        if (mCallMgr != null) mCallMgr.updateIncomingCallAction(action);
    }

    public void updateDataRouterState(DataRouterState dataRouterState) {
        if (dataRouterState == null) {
            throw new NullPointerException("The data router state couldn't be null.");
        }

        Message msg = mHandler.obtainMessage(MSG_UPDATE_DATAROUTER_STATE);
        int delay = 0;
        switch (dataRouterState) {
            case CALL_VOLTE:
                msg.arg1 = CallStateForDataRouter.VOLTE;
                break;
            case CALL_VOWIFI:
                msg.arg1 = CallStateForDataRouter.VOWIFI;
                break;
            case CALL_NONE:
                msg.arg1 = CallStateForDataRouter.NONE;
                // For call none state will be delay 1s, and it is caused by sometimes the "BYE"
                // can not be sent successfully as data router switch after this state update.
                delay = 1000; // 1s
                break;
        }
        mHandler.sendMessageDelayed(msg, delay);
    }

    public void setUsedLocalAddr(String addr) {
        Log.d(TAG, "Update the used local addr to: " + addr);
        mUsedLocalAddr = addr;
    }

    public void setSRVCCSupport(boolean support) {
        mIsSRVCCSupport = support;
    }

    public void onSRVCCStateChanged(int state) {
        if (mCallMgr != null) mCallMgr.onSRVCCStateChanged(state);
    }

    private void init() {
        if (ImsManager.isWfcEnabledByPlatform(mContext)) {
            mPreferences = PreferenceManager.getDefaultSharedPreferences(mContext);
            mPreferences.registerOnSharedPreferenceChangeListener(this);

            mCallMgr = new VoWifiCallManager(mContext);
            mRegisterMgr = new VoWifiRegisterManager(mContext);
            mSecurityMgr = new VoWifiSecurityManager(mContext);
            mXCAPMgr = new VoWifiXCAPManager(mContext, mSecurityMgr);

            mCallMgr.bindService();
            mXCAPMgr.bindService();
            mRegisterMgr.bindService();
            mSecurityMgr.bindService();
            mCallMgr.registerListener(mCallListener);
        }
    }

    private void uninit() {
        if (ImsManager.isWfcEnabledByPlatform(mContext)) {
            // Unbind the service.
            mCallMgr.unbindService();
            mXCAPMgr.unbindService();
            mRegisterMgr.unbindService();
            mSecurityMgr.unbindService();
            mCallMgr.unregisterListener();

            mPreferences.unregisterOnSharedPreferenceChangeListener(this);
        }
    }

    private String getUsedPcscfAddr() {
        try {
            IImsServiceEx imsServiceEx = ImsManagerEx.getIImsServiceEx();
            if (imsServiceEx != null) {
                return imsServiceEx.getImsPcscfAddress();
            }
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to get the volte register ip as catch the RemoteException: "
                    + e.toString());
        }

        return "";
    }

    private void resetFinished() {
        mResetStep = RESET_STEP_INVALID;
        mHandler.removeMessages(MSG_RESET);
        mHandler.removeMessages(MSG_RESET_FORCE);

        if (mEcbmStep == ECBMRequest.ECBM_STEP_FORCE_RESET) {
//            Message msg = mHandler.obtainMessage(MSG_ECBM);
//            msg.arg1 = ECBM_STEP_ATTACH;
//            mHandler.sendMessage(msg);
        } else if (mCallback != null) {
            mCallback.onResetFinished(Result.SUCCESS, 0);
        }
    }

    private void registerLogin(boolean isRelogin) {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Try to start the register login process, re-login: " + isRelogin);
        }

        int s2bType =
                mEcbmStep == ECBMRequest.ECBM_STEP_REGISTER_SOS ? S2bType.SOS : S2bType.NORMAL;
        SecurityConfig secConf = mSecurityMgr.getConfig(mSubId, s2bType);
        RegisterConfig regConf = mRegisterMgr.getCurRegisterConfig();
        if (regConf == null || secConf == null) {
            // Can not get the register IP.
            Log.e(TAG, "Failed to login as the register IP is null.");
            registerFailed();
            return;
        }

        boolean startRegister = false;
        int regVersion = regConf.getValidIPVersion(secConf._prefIPv4);
        if (regVersion != IPVersion.NONE) {
            if (regVersion == secConf._useIPVersion
                    || mSecurityMgr.setIPVersion(mSubId, s2bType, regVersion)) {
                startRegister = true;
                secConf._useIPVersion = regVersion;
                boolean useIPv4 = regVersion == IPVersion.IP_V4;
                String localIP = regConf.getLocalIP(useIPv4);
                String pcscfIP = regConf.getPcscfIP(useIPv4);
                String dnsSerIP = regConf.getDnsSerIP(useIPv4);
                boolean forSos = (mEcbmStep == ECBMRequest.ECBM_STEP_REGISTER_SOS);
                mRegisterMgr.login(forSos, useIPv4, localIP, pcscfIP, dnsSerIP, isRelogin);
            }
        }

        if (!startRegister) {
            // Do not start register as there isn't valid IPv6 & IPv4 address,
            // it means login failed. Notify the result.
            if (isRelogin) {
                // If is re-login, it means already notify register success before.
                // So we'd like to notify as register logout here.
                registerLogout(NativeErrorCode.REG_TIMEOUT);
            } else {
                registerFailed();
            }
        }
    }

    private void registerSuccess(int stateCode) {
        // As login success, we need set the video quality.
        mCallMgr.updateVideoQuality(Utilities.getDefaultVideoQuality(mPreferences));

        mCmdRegisterState = CMD_STATE_FINISHED;
        if (mCallback != null) {
            mCallback.onRegisterStateChanged(RegisterState.STATE_CONNECTED, stateCode);
        }
    }

    private void registerFailed() {
        mCmdRegisterState = CMD_STATE_INVALID;

        if (mEcbmStep != ECBMRequest.ECBM_STEP_INVALID) {
            // Exit the ECBM.
            exitECBM();

            // Notify the register as register logout.
            registerLogout(0);
        } else if (mCallback != null) {
            mCallback.onRegisterStateChanged(RegisterState.STATE_IDLE, 0);
        }

        mRegisterMgr.forceStop(mRegisterListener);
    }

    private void registerLogout(int stateCode) {
        mCmdRegisterState = CMD_STATE_INVALID;
        if (mCallback != null) {
            mCallback.onRegisterStateChanged(RegisterState.STATE_IDLE, 0);
            mCallback.onUnsolicitedUpdate(stateCode == NativeErrorCode.REG_TIMEOUT
                    ? UnsolicitedCode.SIP_TIMEOUT : UnsolicitedCode.SIP_LOGOUT);
        }

        mRegisterMgr.forceStop(mRegisterListener);
        mSecurityMgr.forceStop(mSubId, mSecurityListener);
    }

    private void sendECBMTimeoutMsg(int forStep) {
        mHandler.removeMessages(MSG_ECBM_TIMEOUT);

        Message msg = mHandler.obtainMessage(MSG_ECBM_TIMEOUT);
        msg.arg1 = forStep;
        mHandler.sendMessageDelayed(msg, ECBM_TIMEOUT);
    }

    private void exitECBM() {
        if (mImsEcbm != null) mImsEcbm.updateEcbm(false, null);

        mEcbmStep = ECBMRequest.ECBM_STEP_INVALID;
    }

    private class MyCallListener implements CallListener {
        @Override
        public void onCallIncoming(ImsCallSessionImpl callSession) {
            if (mCallback != null && callSession != null) {
                mCallback.onCallIncoming(
                        callSession.getCallId(), callSession.getCallProfile().mCallType);
            }
        }

        @Override
        public void onCallIsEmergency(ImsCallSessionImpl callSession) {
            // It means we received the network reply as this call is emergency call. We need
            // notify this to ImsService, and it will transfer this call to CP to handle.
            Log.d(TAG, "The call " + callSession + " is emergency call. Notify the result!");
//            if (mCallback != null) mCallback.onCallIsEmergency(callSession);
        }

        @Override
        public void onCallEnd(ImsCallSessionImpl callSession) {
            if (mCallMgr.getCallCount() < 1 && mCallback != null) {
                mCallback.onAllCallsEnd();
                // Check if need reset after all the calls end.
                if (mNeedLogoutAfterCall) {
                    mNeedLogoutAfterCall = false;

                    // Notify as register logout.
                    registerLogout(0);
                }
            }
        }

        @Override
        public void onCallRTPReceived(boolean isVideoCall, boolean isReceived) {
            if (mCallback != null) {
                if (isReceived) {
                    mCallback.onRtpReceived(isVideoCall);
                } else {
                    mCallback.onNoRtpReceived(isVideoCall);
                }
            }
        }

        @Override
        public void onCallRTCPChanged(boolean isVideoCall, int lose, int jitter, int rtt) {
            if (mCallback != null) {
                mCallback.onMediaQualityChanged(isVideoCall, lose, jitter, rtt);
            }
        }

        @Override
        public void onAliveCallUpdate(boolean isVideo) {
            if (mCallback != null) {
                mCallback.onAliveCallUpdate(isVideo);
            }
        }

        @Override
        public void onEnterECBM(ImsCallSessionImpl callSession, ECBMRequest request) {
            Log.d(TAG, "Need enter ECBM for the emergency call: " + callSession);
            if (mCallMgr != null) mCallMgr.updateIncomingCallAction(IncomingCallAction.REJECT);

            ImsEcbmImpl imsEcbm = getEcbmInterface();
            imsEcbm.updateEcbm(true, callSession);
            mECBMRequest = request;

            int handleStep = request.getEnterECBMStep();
            if (handleStep > ECBMRequest.ECBM_STEP_INVALID) {
                sendECBMTimeoutMsg(handleStep);
                Message msg = mHandler.obtainMessage(MSG_ECBM);
                msg.arg1 = handleStep;
                mHandler.sendMessage(msg);
            }
        }

        @Override
        public void onExitECBM() {
            Log.d(TAG, "Exit ECBM.");
            if (mCallMgr != null) mCallMgr.updateIncomingCallAction(IncomingCallAction.NORMAL);

            ImsEcbmImpl imsEcbm = getEcbmInterface();
            imsEcbm.updateEcbm(false, null);

            int handleStep = mECBMRequest.getExitECBMStep();
            if (handleStep > ECBMRequest.ECBM_STEP_INVALID) {
                sendECBMTimeoutMsg(handleStep);
                Message msg = mHandler.obtainMessage(MSG_ECBM);
                msg.arg1 = handleStep;
                mHandler.sendMessage(msg);
            }
        }
    }

    private class MyRegisterListener implements RegisterListener {
        @Override
        public void onLoginFinished(boolean success, int stateCode, int retryAfter) {
            if (success) {
                // If login success or get retry-after, notify the register state changed.
                if (success && mEcbmStep != ECBMRequest.ECBM_STEP_INVALID) {
                    if (mEcbmStep == ECBMRequest.ECBM_STEP_REGISTER_SOS
                            || mEcbmStep == ECBMRequest.ECBM_STEP_REGISTER_NORMAL) {
                        mHandler.removeMessages(MSG_ECBM_TIMEOUT);

                        Message msg = mHandler.obtainMessage(MSG_ECBM);
                        msg.arg1 = mECBMRequest.getNextStep();
                        mHandler.sendMessage(msg);
                    } else {
                        // Shouldn't be here.
                        Log.e(TAG, "[onLoginFinished] Shouldn't be here, please check! "
                                + "The ECBM step: " + mEcbmStep);
                    }
                } else {
                    registerSuccess(stateCode);
                }
            } else if (!success && stateCode == NativeErrorCode.REG_SERVER_FORBIDDEN) {
                // If failed caused by server forbidden, set register failed.
                Log.e(TAG, "Login failed as server forbidden. state code: " + stateCode);
                registerFailed();
            } else {
                // As the PCSCF address may be not only one. For example, there are two IPv6
                // addresses and two IPv4 addresses. So we will try to login again.
                Log.d(TAG, "Last login action is failed, try to use exist address to login again");
                registerLogin(false);
            }
        }

        @Override
        public void onLogout(int stateCode) {
            if (mResetStep != RESET_STEP_INVALID) {
                Log.d(TAG, "Get the register state changed to logout for reset action.");
                if (mResetStep == RESET_STEP_DEREGISTER) {
                    // Register state changed caused by reset. Update the step and send the message.
                    Message msg = mHandler.obtainMessage(MSG_RESET);
                    msg.arg1 = RESET_STEP_DEATTACH;
                    mHandler.sendMessage(msg);
                } else {
                    // Shouldn't be here, do nothing, ignore this event.
                    Log.w(TAG, "Do nothing for reset action as reset step is: " + mResetStep);
                }
            } else if (mEcbmStep != ECBMRequest.ECBM_STEP_INVALID) {
                Log.d(TAG, "Get the register state changed to logout in ECBM.");
                if (mEcbmStep == ECBMRequest.ECBM_STEP_DEREGISTER_SOS
                        || mEcbmStep == ECBMRequest.ECBM_STEP_DEREGISTER_NORMAL) {
                    int nextStep = mECBMRequest.getNextStep();
                    sendECBMTimeoutMsg(nextStep);
                    Message msg = mHandler.obtainMessage(MSG_ECBM);
                    msg.arg1 = nextStep;
                    mHandler.sendMessage(msg);
                } else {
                    // Normal, shouldn't be here.
                    // If logout from the IMS service, but in ECBM, we'd like to handle it as
                    // emergency call failed. And notify this register state changed.
                    Log.w(TAG, "Logout from IMS service, but current ecbm step is: " + mEcbmStep);

                    // Exit the ECBM.
                    exitECBM();
                    registerLogout(stateCode);
                }
            } else {
                registerLogout(stateCode);
            }
        }

        @Override
        public void onPrepareFinished(boolean success) {
            if (success) {
                // As prepare success, start the login process now.
                registerLogin(false);
            } else {
                // Prepare failed, give the register result as failed.
                registerFailed();
            }
        }

        @Override
        public void onRefreshRegFinished(boolean success, int errorCode) {
            if (success) {
                if (mCallback != null) mCallback.onReregisterFinished(success, errorCode);
            } else {
                int callCount = mCallMgr.getCallCount();
                if (callCount > 0) {
                    // It means there is call, and we'd like to notify as register logout after
                    // all the call ends.
                    mNeedLogoutAfterCall = true;
                } else {
                    // There isn't any calls. As get the re-register failed, we'd like to notify
                    // as register logout now.
                    errorCode = (errorCode == NativeErrorCode.REG_EXPIRED_TIMEOUT)
                            ? NativeErrorCode.REG_TIMEOUT : errorCode;
                    registerLogout(errorCode);
                }
            }
        }

        @Override
        public void onRegisterStateChanged(int newState, int errorCode) {
            // If the register state changed, update the register state to
            // call manager & xcap manager.
            if (mCallMgr != null) mCallMgr.updateRegisterState(newState);
            if (mXCAPMgr != null) {
                RegisterConfig regConfig = mRegisterMgr.getCurRegisterConfig();
                int ipVersion = IPVersion.NONE;
                if (regConfig != null) {
                    ipVersion = regConfig.isCurUsedIPv4() ? IPVersion.IP_V4 : IPVersion.IP_V6;
                }
                mXCAPMgr.updateRegisterState(newState, ipVersion);
            }

            // If the new state is registered, we need query the CLIR state from CP.
            // And if the query action success, telephony will update the CLIR via UtInterface.
            if (newState == RegisterState.STATE_CONNECTED) {
                queryCLIRStatus();
            }

            if (errorCode == NativeErrorCode.SERVER_TIMEOUT) {
                // If register state changed caused by server return 504 when UE is calling,
                // need to re-login.
                Log.d(TAG, "Re-login as UE send invite and server response 504, stateCode: "
                        + errorCode);
                registerLogin(true /* re-login */);
            }
        }

        @Override
        public void onResetBlocked() {
            // As the reset blocked, and the vowifi service will be kill soon,
            // We'd like to notify as reset finished if in reset step.
            if (mResetStep != RESET_STEP_INVALID) {
                Log.d(TAG, "Reset blocked, and the current reset step is " + mResetStep);
                resetFinished();
            }
        }

        @Override
        public void onDisconnected() {
            Log.d(TAG, "Register service disconnected, and current register cmd state is: "
                    + mCmdRegisterState);
            if (mCallback == null || mRegisterMgr == null) return;

            switch (mCmdRegisterState) {
                case CMD_STATE_INVALID:
                    // Do nothing as there isn't any register command.
                    break;
                case CMD_STATE_FINISHED:
                    mCallback.onUnsolicitedUpdate(UnsolicitedCode.SIP_LOGOUT);
                    // And same as in progress, need notify the register state change to idle.
                    // Needn't break here.
                case CMD_STATE_PROGRESS:
                    mCallback.onRegisterStateChanged(RegisterState.STATE_IDLE, 0);
                    break;
            }

            mCmdRegisterState = CMD_STATE_INVALID;
        }

        private void queryCLIRStatus() {
            try {
                IImsServiceEx imsServiceEx = ImsManagerEx.getIImsServiceEx();
                if (imsServiceEx != null) {
                    int id = imsServiceEx.getCLIRStatus(Utilities.getPrimaryCard(mContext));
                    if (id < 1) {
                        Log.w(TAG, "Failed to get CLIR status, please check!");
                    }
                }
            } catch (RemoteException e) {
                Log.e(TAG, "Failed to get the CLIR statue as catch the RemoteException: "
                        + e.toString());
            }
        }
    }

    private class MySecurityListener implements SecurityListener {
        @Override
        public void onProgress(int state) {
            if (mCallback != null) mCallback.onAttachStateChanged(state);
        }

        @Override
        public void onSuccessed(int sessionId) {
            if (mEcbmStep != ECBMRequest.ECBM_STEP_INVALID) {
                if (mEcbmStep == ECBMRequest.ECBM_STEP_ATTACH_SOS
                        || mEcbmStep == ECBMRequest.ECBM_STEP_ATTACH_NORMAL) {
                    int nextStep = mECBMRequest.getNextStep();
                    sendECBMTimeoutMsg(nextStep);
                    Message msg = mHandler.obtainMessage(MSG_ECBM);
                    msg.arg1 = nextStep;
                    mHandler.sendMessage(msg);
                } else {
                    // Shouldn't be here.
                    Log.e(TAG, "[onSuccessed] Shouldn't be here, please check! The ECBM step: "
                            + mEcbmStep);
                }
            } else if (mCallback != null) {
                mCmdAttachState = CMD_STATE_FINISHED;
                mCallback.onAttachFinished(true, 0);
            }
        }

        @Override
        public void onFailed(int reason) {
            if (mEcbmStep != ECBMRequest.ECBM_STEP_INVALID) {
                // Failed in ECBM. Exit the ECBM, and notify the state.
                exitECBM();
                if (mCallback != null) {
                    mCallback.onAttachStopped(0);
                    mCallback.onUnsolicitedUpdate(UnsolicitedCode.SECURITY_STOP);
                }
            } else if (mCallback != null) {
                mCmdAttachState = CMD_STATE_INVALID;
                mCallback.onAttachFinished(false, reason);
                if (mResetStep >= RESET_STEP_DEATTACH
                        && reason == Utilities.NativeErrorCode.IKE_INTERRUPT_STOP) {
                    Log.d(TAG, "Attached failed cased by interrupt. It means reset finished.");
                    resetFinished();
                }
            }
        }

        @Override
        public void onStopped(boolean forHandover, int errorCode) {
            if (!forHandover && mResetStep != RESET_STEP_INVALID) {
                // If the stop action is for reset, we will notify the reset finished result.
                if (mResetStep == RESET_STEP_DEREGISTER) {
                    // It means the de-register do not finished. We'd like to force stop register.
                    registerForceStop();
                }
                Log.d(TAG, "S2b stopped, it means the reset action finished. Notify the result.");
                // In reset process, and reset finished, we need reset the attach state to idle.
                mCmdAttachState = CMD_STATE_INVALID;
                resetFinished();
            } else if (!forHandover && mEcbmStep != ECBMRequest.ECBM_STEP_INVALID) {
                if (mEcbmStep == ECBMRequest.ECBM_STEP_DEATTACH_SOS
                        || mEcbmStep == ECBMRequest.ECBM_STEP_DEATTACH_NORMAL) {
                    int nextStep = mECBMRequest.getNextStep();
                    sendECBMTimeoutMsg(nextStep);

                    Message msg = mHandler.obtainMessage(MSG_ECBM);
                    msg.arg1 = nextStep;
                    mHandler.sendMessage(msg);
                } else {
                    // Normal, shouldn't be here.
                    // If stopped connect to EPDG, but in ECBM, we'd like to handle it as
                    // emergency call failed. And notify this register state changed.
                    exitECBM();
                    if (mCallback != null) {
                        mCallback.onAttachStopped(0);
                        mCallback.onUnsolicitedUpdate(UnsolicitedCode.SECURITY_STOP);
                    }
                }
            } else if (mCallback != null) {
                mCmdAttachState = CMD_STATE_INVALID;
                mCallback.onAttachStopped(errorCode);
                if (errorCode == NativeErrorCode.DPD_DISCONNECT) {
                    mCallback.onUnsolicitedUpdate(UnsolicitedCode.SECURITY_DPD_DISCONNECTED);
                } else if (errorCode == NativeErrorCode.IPSEC_REKEY_FAIL
                        || errorCode == NativeErrorCode.IKE_REKEY_FAIL) {
                    mCallback.onUnsolicitedUpdate(UnsolicitedCode.SECURITY_REKEY_FAILED);
                } else {
                    mCallback.onUnsolicitedUpdate(UnsolicitedCode.SECURITY_STOP);
                }
            }
        }

        @Override
        public void onDisconnected() {
            Log.d(TAG, "Security service disconnected, and current attach cmd state is: "
                    + mCmdAttachState);
            if (mCallback == null) return;

            switch (mCmdAttachState) {
                case CMD_STATE_INVALID:
                case CMD_STATE_PROGRESS:
                    mCallback.onAttachFinished(false, 0);
                    break;
                case CMD_STATE_FINISHED:
                    mCallback.onAttachStopped(0);
                    mCallback.onUnsolicitedUpdate(UnsolicitedCode.SECURITY_STOP);
                    break;
            }

            mCmdAttachState = CMD_STATE_INVALID;
        }
    }

    public enum IncomingCallAction {
        NORMAL, REJECT
    }

    public enum WifiState {
        CONNECTED, DISCONNECTED
    }

    public enum DataRouterState {
        CALL_VOLTE, CALL_VOWIFI, CALL_NONE
    }

    public interface VoWifiCallback {
        public void onAttachFinished(boolean success, int errorCode);

        public void onAttachStopped(int stoppedReason);

        public void onAttachStateChanged(int state);

        public void onDPDDisconnected();

        public void onRegisterStateChanged(int state, int stateCode);

        public void onReregisterFinished(boolean isSuccess, int errorCode);

        /**
         * release result callback
         *
         * @param result
         * @param errorCode
         */
        public void onResetFinished(int result, int errorCode);

        public void onUpdateDRStateFinished();

        /**
         * Call callback
         *
         * @param callId The only identity of the phone
         * @param type See the definition of ImsCallProfile
         */
        public void onCallIncoming(String callId, int type);

        public void onAliveCallUpdate(boolean isVideoCall);

        /**
         * Call this interface after all calls end.
         */
        public void onAllCallsEnd();

        /**
         * Media quality callback
         *
         * @param lose:packet loss rate(Molecular value in percentage)
         * @param jitter:jitter(millisecond)
         * @param rtt:delay(millisecond)
         */
        public void onMediaQualityChanged(boolean isVideo, int lose, int jitter, int rtt);

        /**
         * 5 seconds no RTP package callback
         *
         * @param video:Whether it is video
         */
        public void onNoRtpReceived(boolean isVideo);

        public void onRtpReceived(boolean isVideo);

        public void onUnsolicitedUpdate(int stateCode);
    }

}

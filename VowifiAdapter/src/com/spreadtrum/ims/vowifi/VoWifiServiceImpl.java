
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
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.util.Log;

import com.android.ims.ImsCallProfile;
import com.android.ims.internal.IImsCallSessionListener;
import com.spreadtrum.ims.ImsConfigImpl;
import com.spreadtrum.ims.vowifi.Utilities.AttachState;
import com.spreadtrum.ims.vowifi.Utilities.CallStateForDataRouter;
import com.spreadtrum.ims.vowifi.Utilities.CallType;
import com.spreadtrum.ims.vowifi.Utilities.IPVersion;
import com.spreadtrum.ims.vowifi.Utilities.NativeErrorCode;
import com.spreadtrum.ims.vowifi.Utilities.RegisterIPAddress;
import com.spreadtrum.ims.vowifi.Utilities.RegisterState;
import com.spreadtrum.ims.vowifi.Utilities.Result;
import com.spreadtrum.ims.vowifi.Utilities.SecurityConfig;
import com.spreadtrum.ims.vowifi.Utilities.UnsolicitedCode;
import com.spreadtrum.ims.vowifi.Utilities.UnsupportedCallTypeException;
import com.spreadtrum.ims.vowifi.VoWifiCallManager.CallListener;
import com.spreadtrum.ims.vowifi.VoWifiRegisterManager.RegisterListener;
import com.spreadtrum.ims.vowifi.VoWifiSecurityManager.SecurityListener;

/**
 * The VoWifi only enabled for primary SIM card.
 */
@TargetApi(23)
public class VoWifiServiceImpl implements OnSharedPreferenceChangeListener {
    private static final String TAG = Utilities.getTag(VoWifiServiceImpl.class.getSimpleName());

    private static final int RESET_TIMEOUT = 5000; // 5s
    private static final int RESET_STEP_INVALID = 0;
    private static final int RESET_STEP_DEREGISTER = 1;
    private static final int RESET_STEP_DEATTACH = 2;
    private static final int RESET_STEP_FORCE_RESET = 3;

    private int mResetStep = RESET_STEP_INVALID;

    private Context mContext;
    private SharedPreferences mPreferences = null;
    private VoWifiCallback mCallback = null;
    private RegisterIPAddress mRegisterIP = null;

    private ImsUtImpl mImsUt;
    private VoWifiCallManager mCallMgr;
    private VoWifiRegisterManager mRegisterMgr;
    private VoWifiSecurityManager mSecurityMgr;

    private MyHandler mHandler = null;
    private MyCallListener mCallListener = new MyCallListener();
    private MyRegisterListener mRegisterListener = new MyRegisterListener();
    private MySecurityListener mSecurityListener = new MySecurityListener();

    private static final int MSG_RESET = 1;
    private static final int MSG_RESET_FORCE = 2;
    private static final int MSG_ATTACH = 3;
    private static final int MSG_DEATTACH = 4;
    private static final int MSG_REGISTER = 5;
    private static final int MSG_DEREGISTER = 6;
    private static final int MSG_REREGISTER = 7;
    private static final int MSG_UPDATE_DATAROUTER_STATE = 8;
    private static final int MSG_TERMINATE_CALLS = 9;
    private class MyHandler extends Handler {
        public MyHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            if (Utilities.DEBUG) Log.i(TAG, "The handler get the message: " + msg.what);

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
                    registerInternal();
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

    }

    public VoWifiServiceImpl(Context context) {
        mContext = context;
        mPreferences = PreferenceManager.getDefaultSharedPreferences(mContext);
        mPreferences.registerOnSharedPreferenceChangeListener(this);

        mCallMgr = new VoWifiCallManager(context);
        mRegisterMgr = new VoWifiRegisterManager(context);
        mSecurityMgr = new VoWifiSecurityManager(context);

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
            int quality = Utilities.getDefaultVideoQuality(mPreferences);
            mCallMgr.setVideoQuality(quality);
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
        if (mImsUt == null) {
            mImsUt = new ImsUtImpl(mContext, mCallMgr);
        }
        return mImsUt;
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
                    + (WifiState.CONNECTED.equals(state) ? "connect" : "disconnect"));
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
        mHandler.sendEmptyMessage(MSG_ATTACH);
    }

    private void attachInternal() {
        mSecurityMgr.attach();
    }

    public void deattach() {
        deattach(false);
    }

    public void deattach(boolean forHandover) {
        mHandler.sendMessage(mHandler.obtainMessage(MSG_DEATTACH, forHandover));
    }

    private void deattachInternal(boolean forHandover) {
        mSecurityMgr.deattach(forHandover);
    }

    /**
     * To force stop the s2b even it in the connect progress.
     */
    private void securityForceStop() {
        mSecurityMgr.forceStop();
    }

    public SecurityConfig getSecurityConfig() {
        return mSecurityMgr.getConfig();
    }

    public void register() {
        mHandler.sendEmptyMessage(MSG_REGISTER);
    }

    /**
     * Register to the IMS service.
     */
    private void registerInternal() {
        // Check the security state, if it is attached. We could start the register process.
        if (mSecurityMgr.getCurSecurityState() != AttachState.STATE_CONNECTED
                && mSecurityMgr.getConfig() == null) {
            Log.e(TAG, "Please wait for attach success. Current attach state: "
                    + mSecurityMgr.getCurSecurityState() + ", security config: "
                    + mSecurityMgr.getConfig());
            if (mCallback != null) mCallback.onReregisterFinished(false, 0);
            return;
        }

        // We will always use the primary card to register.
        int phoneId = Utilities.getPrimaryCard(mContext);
        int[] subId = SubscriptionManager.getSubId(phoneId);
        if (subId == null || subId.length == 0) {
            Log.e(TAG, "Can not get the sub id from the phone id: " + phoneId);
            if (mCallback != null) mCallback.onReregisterFinished(false, 0);
            return;
        }

        // For register process, step as this:
        // 1. Prepare for login.
        // 2. If prepare finished, start the login process with the first local IP and PCSCF IP.
        // 3. If login failed, need try the left local IP and PCSCF IP until all of them already
        //    failed, then need notify the user login failed.
        mRegisterMgr.prepareForLogin(subId[0]);
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
        mRegisterMgr.deregister();
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
        return mRegisterMgr.forceStop();
    }

    public String getCurLocalAddress() {
        if (mRegisterIP != null
                && mRegisterMgr.getCurRegisterState() == RegisterState.STATE_CONNECTED) {
            return mRegisterIP.getCurUsedLocalIP();
        }
        return "";
    }

    public String getCurPcscfAddress() {
        if (mRegisterIP != null
                && mRegisterMgr.getCurRegisterState() == RegisterState.STATE_CONNECTED) {
            return mRegisterIP.getCurUsedPcscfIP();
        }
        return "";
    }

    /**
     * Create a new call session for the given profile.
     */
    public ImsCallSessionImpl createCallSession(ImsCallProfile profile,
            IImsCallSessionListener listener) {
        return mCallMgr.createCallSession(profile, listener);
    }

    public ImsCallSessionImpl getPendingCallSession(String callId) {
        return mCallMgr.getCallSession(callId);
    }

    public void terminateCalls(WifiState state) {
        if (state == null) {
            throw new NullPointerException("The wifi state couldn't be null.");
        }

        mHandler.sendMessage(mHandler.obtainMessage(MSG_TERMINATE_CALLS, state));
    }

    public int getCallCount() {
        if (Utilities.DEBUG) Log.i(TAG, "Get call count.");

        return mCallMgr.getCallCount();
    }

    public int getAliveCallType() {
        if (Utilities.DEBUG) Log.i(TAG, "Get alive call type.");

        ImsCallSessionImpl callSession = mCallMgr.getAliveCallSession();
        if (callSession == null) {
            Log.w(TAG, "There isn't any call in alive state, return unknown.");
            return CallType.CALL_TYPE_UNKNOWN;
        }

        try {
            boolean isVideo = Utilities.isVideoCall(callSession.getCallProfile().mCallType);
            return isVideo ? CallType.CALL_TYPE_VIDEO : CallType.CALL_TYPE_VOICE;
        } catch (UnsupportedCallTypeException e) {
            Log.e(TAG, "Catch the UnsupportedCallTypeException when get the call type. e: " + e);
            return CallType.CALL_TYPE_UNKNOWN;
        }
    }

    public int getAliveCallLose() {
        return mCallMgr.getPacketLose();
    }

    public int getAliveCallJitter() {
        return mCallMgr.getJitter();
    }

    public int getAliveCallRtt() {
        return mCallMgr.getRtt();
    }

    public void updateIncomingCallAction(IncomingCallAction action) {
        if (action == null) {
            throw new NullPointerException("The incoming call action couldn't be null.");
        }

        mCallMgr.updateIncomingCallAction(action);
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

    public void setMonitorPeriodForNoData(int millis) {
        // TODO:
    }

    private void init() {
        mCallMgr.bindService();
        mRegisterMgr.bindService();
        mSecurityMgr.bindService();
        mCallMgr.registerListener(mCallListener);
        mRegisterMgr.registerListener(mRegisterListener);
        mSecurityMgr.registerListener(mSecurityListener);
    }

    private void uninit() {
        // Unbind the service.
        mCallMgr.unbindService();
        mRegisterMgr.unbindService();
        mSecurityMgr.unbindService();
        mCallMgr.unregisterListener();
        mRegisterMgr.unregisterListener();
        mSecurityMgr.unregisterListener();
    }

    private void resetFinished() {
        mResetStep = RESET_STEP_INVALID;
        mHandler.removeMessages(MSG_RESET);
        mHandler.removeMessages(MSG_RESET_FORCE);

        mCallback.onResetFinished(Result.SUCCESS, 0);
    }

    private void registerLogin() {
        if (Utilities.DEBUG) Log.i(TAG, "Try to start the register login process.");

        if (mRegisterIP == null) {
            // Can not get the register IP.
            Log.e(TAG, "Failed to login as the register IP is null.");
            registerFailed();
            return;
        }

        boolean startRegister = false;
        int regVersion = mRegisterIP.getValidIPVersion(mSecurityMgr.getConfig()._prefIPv4);
        if (regVersion != IPVersion.NONE) {
            boolean useIPv4 = regVersion == IPVersion.IP_V4;
            if (regVersion == mSecurityMgr.getConfig()._useIPVersion
                    || mSecurityMgr.setIPVersion(regVersion)) {
                mSecurityMgr.getConfig()._useIPVersion= regVersion;
                startRegister = true;
                String localIP = mRegisterIP.getLocalIP(useIPv4);
                String pcscfIP = mRegisterIP.getPcscfIP(useIPv4);
                mRegisterMgr.login(useIPv4, localIP, pcscfIP);
            }
        }

        if (!startRegister) {
            // The IPv6 & IPv4 invalid, it means login failed. Update the result.
            // Can not get the register IP.
            registerFailed();
        }
    }

    private void registerFailed() {
        mRegisterIP = null;
        mRegisterMgr.forceStop();
        mCallback.onRegisterStateChanged(mRegisterMgr.getCurRegisterState(), 0);
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
        public void onCallEnd(ImsCallSessionImpl callSession) {
            if (mCallMgr.getCallCount() < 1 && mCallback != null) {
                mCallback.onAllCallsEnd();
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
    }

    private class MyRegisterListener implements RegisterListener {
        @Override
        public void onReregisterFinished(boolean success, int errorCode) {
            mCallback.onReregisterFinished(success, errorCode);
        }

        @Override
        public void onLoginFinished(boolean success, int stateCode, int retryAfter) {
            if (success || retryAfter > 0) {
                // If login success or get retry-after, notify the register state changed.
                if (mCallback != null) {
                    mCallback.onRegisterStateChanged(mRegisterMgr.getCurRegisterState(), stateCode);
                }
                // After login success, we need set the video quality.
                mCallMgr.setVideoQuality(Utilities.getDefaultVideoQuality(mPreferences));
            } else if (!success && stateCode == NativeErrorCode.REG_SERVER_FORBIDDEN) {
                // If failed caused by server forbidden, set register failed.
                Log.e(TAG, "Login failed as server forbidden. state code: " + stateCode);
                registerFailed();
            } else {
                // As the PCSCF address may be not only one. For example, there are two IPv6
                // addresses and two IPv4 addresses. So we will try to login again.
                Log.d(TAG, "Last login action is failed, try to use exist address to login again");
                registerLogin();
            }
        }

        @Override
        public void onLogout(int stateCode) {
            if (mResetStep != RESET_STEP_INVALID) {
                Log.d(TAG, "Get the register state changed for reset action.");
                if (mResetStep == RESET_STEP_DEREGISTER) {
                    // Register state changed caused by reset. Update the step and send the message.
                    Message msg = mHandler.obtainMessage(MSG_RESET);
                    msg.arg1 = RESET_STEP_DEATTACH;
                    mHandler.sendMessage(msg);
                } else {
                    // Shouldn't be here, do nothing, ignore this event.
                    Log.w(TAG, "Do nothing for reset action ignore this register state change.");
                }
            } else {
                mRegisterIP = null;
                mCallback.onRegisterStateChanged(mRegisterMgr.getCurRegisterState(), 0);
                mCallback.onUnsolicitedUpdate(stateCode == NativeErrorCode.REG_TIMEOUT
                        ? UnsolicitedCode.SIP_TIMEOUT : UnsolicitedCode.SIP_LOGOUT);
            }
            // As already logout, force stop to reset sip stack.
            mRegisterMgr.forceStop();
        }

        @Override
        public void onPrepareFinished(boolean success, int errorCode) {
            if (success) {
                // As prepare success, start the login process now. And try from the IPv6;
                SecurityConfig config = mSecurityMgr.getConfig();
                mRegisterIP = RegisterIPAddress.getInstance(
                        config._ip4, config._ip6, config._pcscf4, config._pcscf6);
                registerLogin();
            } else {
                // Prepare failed, give the register result as failed.
                registerFailed();
            }
        }

        @Override
        public void onRegisterStateChanged(int newState) {
            // If the register state changed, update the register state to call manager.
            if (mCallMgr != null) mCallMgr.updateRegisterState(newState);
        }
    }

    private class MySecurityListener implements SecurityListener {
        @Override
        public void onProgress(int state) {
            mCallback.onAttachStateChanged(state);
        }

        @Override
        public void onSuccessed() {
            mCallback.onAttachFinished(true, 0);
        }

        @Override
        public void onFailed(int reason) {
            mCallback.onAttachFinished(false, reason);
            if (mResetStep >= RESET_STEP_DEATTACH
                    && reason == Utilities.NativeErrorCode.IKE_INTERRUPT_STOP) {
                Log.d(TAG, "Attached failed cased by interrupt. It means reset finished.");
                resetFinished();
            }
        }

        @Override
        public void onStopped(boolean forHandover, int errorCode) {
            // If the stop action is for reset, we will notify the reset finished result.
            if (!forHandover && mResetStep != RESET_STEP_INVALID) {
                if (mResetStep == RESET_STEP_DEREGISTER) {
                    // It means the de-register do not finished. We'd like to force stop register.
                    registerForceStop();
                }
                Log.d(TAG, "S2b stopped, it means the reset action finished. Notify the result.");
                resetFinished();
            } else if (mCallback != null) {
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
         * @param type   See the definition of ImsCallProfile
         */
        public void onCallIncoming(String callId, int type);

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

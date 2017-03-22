package com.spreadtrum.ims.vowifi;

import android.annotation.TargetApi;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.RemoteException;
import android.os.SystemProperties;
import android.telecom.VideoProfile;
import android.telecom.Connection.VideoProvider;
import android.telecom.VideoProfile.CameraCapabilities;
import android.text.TextUtils;
import android.util.Log;
import android.view.WindowManager;
import android.widget.Toast;

import com.android.ims.ImsCallProfile;
import com.android.ims.ImsConferenceState;
import com.android.ims.ImsReasonInfo;
import com.android.ims.ImsStreamMediaProfile;
import com.android.ims.internal.IImsCallSessionListener;
import com.android.ims.internal.IImsServiceEx;
import com.android.ims.internal.ImsCallSession;
import com.android.ims.internal.ImsSrvccCallInfo;
import com.android.ims.internal.ImsCallSession.State;
import com.android.ims.internal.ImsManagerEx;
import com.spreadtrum.ims.R;
import com.spreadtrum.ims.vowifi.Utilities.CallStateForDataRouter;
import com.spreadtrum.ims.vowifi.Utilities.JSONUtils;
import com.spreadtrum.ims.vowifi.Utilities.PendingAction;
import com.spreadtrum.ims.vowifi.Utilities.RegisterState;
import com.spreadtrum.ims.vowifi.Utilities.Result;
import com.spreadtrum.ims.vowifi.Utilities.SRVCCResult;
import com.spreadtrum.ims.vowifi.Utilities.SRVCCSyncInfo;
import com.spreadtrum.ims.vowifi.Utilities.UnsupportedCallTypeException;
import com.spreadtrum.ims.vowifi.Utilities.VideoQuality;
import com.spreadtrum.ims.vowifi.VoWifiServiceImpl.IncomingCallAction;
import com.spreadtrum.ims.vowifi.VoWifiServiceImpl.WifiState;
import com.spreadtrum.vowifi.service.IVoWifiSerService;
import com.spreadtrum.vowifi.service.IVoWifiSerServiceCallback;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;

@TargetApi(23)
public class VoWifiCallManager extends ServiceManager {

    public interface CallListener {
        public void onCallIncoming(ImsCallSessionImpl callSession);
        public void onCallIsEmergency(ImsCallSessionImpl callSession);
        public void onCallEnd(ImsCallSessionImpl callSession);
        public void onCallRTPReceived(boolean isVideoCall, boolean isReceived);
        public void onCallRTCPChanged(boolean isVideoCall, int lose, int jitter, int rtt);
        public void onEnterECBM(ImsCallSessionImpl callSession);
        public void onExitECBM();
    }

    public interface ICallChangedListener {
        public void onChanged(IVoWifiSerService newServiceInterface);
    }

    private static final String TAG = Utilities.getTag(VoWifiCallManager.class.getSimpleName());

    private static final int MSG_ACTION_START_AUDIO_STREAM = 1;
    private static final int MSG_ACTION_STOP_AUDIO_STREAM  = 2;
    private static final int MSG_ACTION_SET_VIDEO_QUALITY  = 3;

    private static final String SERVICE_ACTION = "com.spreadtrum.vowifi.service.IVowifiService";
    private static final String SERVICE_PACKAGE = "com.spreadtrum.vowifi";
    private static final String SERVICE_CLASS = "com.spreadtrum.vowifi.service.VoWifiSerService";

    private static final String PROP_KEY_AUTO_ANSWER = "persist.sys.vowifi.autoanswer";

    private int mUseAudioStreamCount = 0;
    private int mRegisterState = RegisterState.STATE_IDLE;

    private MyAlertDialog mAlertDialog = null;
    private ImsCallSessionImpl mConferenceCallSession = null;
    private ImsCallSessionImpl mEmergencyCallSession = null;

    private CallListener mListener;
    private IncomingCallAction mIncomingCallAction = IncomingCallAction.NORMAL;
    private ArrayList<ImsCallSessionImpl> mSessionList = new ArrayList<ImsCallSessionImpl>();
    private ArrayList<ImsCallSessionImpl> mSRVCCSessionList = new ArrayList<ImsCallSessionImpl>();

    private IVoWifiSerService mICall;
    private ArrayList<ICallChangedListener> mICallChangedListeners =
            new ArrayList<ICallChangedListener>();
    private MySerServiceCallback mVoWifiServiceCallback = new MySerServiceCallback();

    private static final int MSG_HANDLE_EVENT = 0;
    private static final int MSG_INVITE_CALL = 1;
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_HANDLE_EVENT:
                    handleEvent((String) msg.obj);
                    break;
                case MSG_INVITE_CALL:
                    inviteCall((ImsCallSessionImpl) msg.obj);
                    break;
            }
        }
    };

    // Please do not change this defined, it samed as the state change value.
    private static final int MSG_SRVCC_START   = 0;
    private static final int MSG_SRVCC_SUCCESS = 1;
    private static final int MSG_SRVCC_CANCEL  = 2;
    private static final int MSG_SRVCC_FAILED  = 3;
    private SRVCCHandler mSRVCCHandler = null;
    private class SRVCCHandler extends Handler {
        private ArrayList<ImsSrvccCallInfo> mInfoList = new ArrayList<ImsSrvccCallInfo>();

        public SRVCCHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch(msg.what) {
                case MSG_SRVCC_START:
                    Log.d(TAG, "Will handle the SRVCC start event.");
                    // When SRVCC start, put all the call session to SRVCC session list.
                    mInfoList.clear();
                    mSRVCCSessionList.clear();
                    mSRVCCSessionList.addAll(mSessionList);

                    // Prepare the call state which will need sync to modem if SRVCC success.
                    for (ImsCallSessionImpl session : mSRVCCSessionList) {
                        session.prepareSRVCCSyncInfo(mInfoList, 0);
                    }
                    break;
                case MSG_SRVCC_SUCCESS:
                    Log.d(TAG, "Will handle the SRVCC success event.");
                    // If SRVCC success, we need do as this:
                    // 1. Sync the calls info to CP.
                    //    Inform UI-layer to update call UI for Video SRVCC.
                    // 2. Release the native call resource.
                    // 3. Give the callback to telephony if there is no response action,
                    //    and the failed reason should be ImsReasonInfo.CODE_LOCAL_HO_NOT_FEASIBLE.
                    // 4. Close the call session.
                    // 5. Clear the session list.

                    // Sync the calls' info.
                    IBinder binder =
                            android.os.ServiceManager.getService(ImsManagerEx.IMS_SERVICE_EX);
                    IImsServiceEx imsServiceEx = IImsServiceEx.Stub.asInterface(binder);
                    if (imsServiceEx == null) {
                        Log.e(TAG, "Can not get the ims ex service.");
                    } else {
                        try {
                            Log.d(TAG, "Notify the SRVCC call infos.");
                            imsServiceEx.notifySrvccCallInfos(mInfoList);
                        } catch (RemoteException e) {
                            Log.e(TAG, "Failed to sync the infos as catch the exception: " + e);
                        }
                    }

                    for (ImsCallSessionImpl session : mSRVCCSessionList) {
                        // Release the native call resource, but do not terminate the UI.
                        session.releaseCall();
                        // Give the callback if there is pending action.
                        session.processNoResponseAction();
                        // Close this call session.
                        session.close();
                    }

                    // Clear the SRVCC session list.
                    mInfoList.clear();
                    mSRVCCSessionList.clear();
                    break;
                case MSG_SRVCC_FAILED:
                case MSG_SRVCC_CANCEL:
                    Log.d(TAG, "Will handle the SRVCC failed/cancel event.");
                    for (ImsCallSessionImpl session : mSRVCCSessionList) {
                        int result = (msg.what == MSG_SRVCC_FAILED ? SRVCCResult.FAILURE
                                : SRVCCResult.CANCEL);
                        session.updateSRVCCResult(result);
                    }
                    mInfoList.clear();
                    mSRVCCSessionList.clear();
                    break;
            }
        }
    }

    protected VoWifiCallManager(Context context) {
        super(context);

        // New a thread to handle the SRVCC event.
        HandlerThread thread = new HandlerThread("SRVCC");
        thread.start();
        Looper looper = thread.getLooper();
        mSRVCCHandler = new SRVCCHandler(looper);
    }

    @Override
    protected void finalize() throws Throwable {
        if (mICall != null) mICall.unregisterCallback(mVoWifiServiceCallback);
        super.finalize();
    }

    @Override
    protected void onServiceChanged() {
        try {
            mICall = null;
            if (mServiceBinder != null) {
                mICall = IVoWifiSerService.Stub.asInterface(mServiceBinder);
                mICall.registerCallback(mVoWifiServiceCallback);
            }
            for (ICallChangedListener listener : mICallChangedListeners) {
                listener.onChanged(mICall);
            }
        } catch (RemoteException e) {
            Log.e(TAG, "Can not register callback as catch the RemoteException. e: " + e);
        }
    }

    @Override
    protected boolean handlePendingAction(Message msg) {
        if (Utilities.DEBUG) Log.i(TAG, "Handle the pending action, msg: " + msg);

        boolean handle = false;
        switch (msg.what) {
            case MSG_ACTION_START_AUDIO_STREAM: {
                startAudioStream();
                break;
            }
            case MSG_ACTION_STOP_AUDIO_STREAM: {
                stopAudioStream();
                break;
            }
            case MSG_ACTION_SET_VIDEO_QUALITY: {
                PendingAction action = (PendingAction) msg.obj;
                setVideoQuality((Integer) action._params.get(0));
                break;
            }
        }
        return handle;
    }

    public void bindService() {
        super.bindService(SERVICE_ACTION, SERVICE_PACKAGE, SERVICE_CLASS);
    }

    public void registerListener(CallListener listener) {
        if (listener == null) {
            Log.e(TAG, "Can not register the listener as it is null.");
            return;
        }

        mListener = listener;
    }

    public void unregisterListener() {
        if (Utilities.DEBUG) Log.i(TAG, "Unregister the listener: " + mListener);
        mListener = null;
    }

    public boolean registerCallInterfaceChanged(ICallChangedListener listener) {
        if (listener == null) {
            Log.w(TAG, "Can not register the call interface changed as the listener is null.");
            return false;
        }

        mICallChangedListeners.add(listener);

        // Notify the service changed immediately when register the listener.
        listener.onChanged(mICall);
        return true;
    }

    public boolean unregisterCallInterfaceChanged(ICallChangedListener listener) {
        if (listener == null) {
            Log.w(TAG, "Can not register the call interface changed as the listener is null.");
            return false;
        }

        return mICallChangedListeners.remove(listener);
    }

    public ImsCallSessionImpl createMOCallSession(ImsCallProfile profile,
            IImsCallSessionListener listener) {
        return createCallSession(profile, listener, null, SRVCCSyncInfo.CallDirection.MO);
    }

    public ImsCallSessionImpl createMTCallSession(ImsCallProfile profile,
            IImsCallSessionListener listener) {
        return createCallSession(profile, listener, null, SRVCCSyncInfo.CallDirection.MT);
    }

    public void terminateCalls(WifiState state) throws RemoteException {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Try to terminate all the calls with wifi state: "
                    + (WifiState.CONNECTED.equals(state) ? "connect" : "disconnect"));
        }

        ArrayList<ImsCallSessionImpl> callList =
                (ArrayList<ImsCallSessionImpl>) mSessionList.clone();
        for (ImsCallSessionImpl callSession : callList) {
            Log.d(TAG, "Terminate the call: " + callSession);
            switch (state) {
                case CONNECTED:
                    // If the current wifi is connect, we'd like to send the terminate request.
                    callSession.terminate(ImsReasonInfo.CODE_USER_TERMINATED);

                    // If the user disabled the "WIFI calling" button, then we will receive this
                    // action, and at the same time, we will also receive the de-register action.
                    // So we don't know if the terminate action could be sent to the IMS service.
                    // Then we'd like to terminate the call immediately.
                case DISCONNECTED:
                    handleCallTermed(callSession, ImsReasonInfo.CODE_USER_TERMINATED);
                    break;
            }
        }
    }

    public void updateRegisterState(int newState) {
        mRegisterState = newState;
    }

    public void updateIncomingCallAction(IncomingCallAction action) {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Update the incoming call action to: "
                    + (action == IncomingCallAction.NORMAL ? "normal" : "reject"));
        }
        mIncomingCallAction = action;
    }

    public void updateDataRouterState(int state) {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Update the call state to data router. state: " + getDRStateString(state));
        }

        // As this used to update the call state, if failed to update, needn't add to pending list.
        if (mICall != null) {
            try {
                int res = mICall.updateDataRouterState(state);
                if (res == Result.FAIL) {
                    Log.e(TAG, "Failed to update the data router state, please check!");
                }
            } catch (RemoteException e) {
                Log.e(TAG, "Failed to update the data router state for RemoteException e: " + e);
            }
        }
    }

    public int getPacketLose() {
        if (Utilities.DEBUG) Log.i(TAG, "Try to get the packet lose.");
        if (mICall == null) {
            Log.e(TAG, "Can not get the packet lose as the service do not bind success now.");
            return 0;
        }

        ImsCallSessionImpl callSession = getAliveCallSession();
        if (callSession == null) {
            Log.d(TAG, "Can not found the actived call, return packet lose as 0.");
            return 0;
        }

        return callSession.getPacketLose();
    }

    public int getJitter() {
        if (Utilities.DEBUG) Log.i(TAG, "Try to get the jitter.");
        if (mICall == null) {
            Log.e(TAG, "Can not get the jitter as the service do not bind success now.");
            return 0;
        }

        ImsCallSessionImpl callSession = getAliveCallSession();
        if (callSession == null) {
            Log.d(TAG, "Can not found the actived call, return jitter as 0.");
            return 0;
        }

        return callSession.getJitter();
    }

    public int getRtt() {
        if (Utilities.DEBUG) Log.i(TAG, "Try to get the rtt.");
        if (mICall == null) {
            Log.e(TAG, "Can not get the rtt as the service do not bind success now.");
            return 0;
        }

        ImsCallSessionImpl callSession = getAliveCallSession();
        if (callSession == null) {
            Log.d(TAG, "Can not found the actived call, return rtt as 0.");
            return 0;
        }

        return callSession.getRtt();
    }

    public void startAudioStream() {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Try to start the audio stream, current use audio stream count: "
                    + mUseAudioStreamCount);
        }

        // Only start the audio stream on the first call accept or start.
        mUseAudioStreamCount = mUseAudioStreamCount + 1;
        if (mICall != null && mUseAudioStreamCount == 1) {
            try {
                mICall.startAudioStream();
            } catch (RemoteException e) {
                Log.e(TAG, "Catch the remote exception when start the audio stream, e: " + e);
                // The start action is failed, need add this action to pending list.
                addToPendingList(
                        new PendingAction("startAudioStream", MSG_ACTION_START_AUDIO_STREAM));
            }
        }
    }

    public void stopAudioStream() {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Try to stop the audio stream, the current use audio stream count: "
                    + mUseAudioStreamCount);
        }

        // If there is any call used the audio stream, we need reduce the count, else do nothing.
        mUseAudioStreamCount =
                mUseAudioStreamCount > 0 ? mUseAudioStreamCount - 1 : mUseAudioStreamCount;
        if (mUseAudioStreamCount != 0) {
            Log.d(TAG, "There is call need audio stream, needn't stop audio stream, exist number: "
                    + mUseAudioStreamCount);
            return;
        }

        Log.d(TAG, "There isn't any call use the audio stream, need stop audio stream now.");
        boolean handle = false;
        if (mICall != null) {
            try {
                mICall.stopAudioStream();
                handle = true;
            } catch (RemoteException e) {
                Log.e(TAG, "Catch the remote exception when stop the audio stream, e: " + e);
            }
        }
        if (!handle) {
            // The stop action is failed, need add this action to pending list.
            addToPendingList(new PendingAction("stopAudioStream", MSG_ACTION_STOP_AUDIO_STREAM));
        }
    }

    public void setVideoQuality(int quality) {
        if (Utilities.DEBUG) Log.i(TAG, "Set the video quality as index is: " + quality);

        boolean handle = false;
        if (mICall != null) {
            try {
                VideoQuality video = Utilities.sVideoQualityList.get(quality);
                int res = mICall.setVideoQuality(video._width, video._height, video._frameRate,
                        video._bitRate, video._brHi, video._brLo, video._frHi, video._frLo);
                if (res == Result.SUCCESS) handle = true;
            } catch (RemoteException e) {
                Log.e(TAG, "Failed to set the video quality as catch the RemoteException e: " + e);
            }
        }
        if (!handle) {
            // The stop action is failed, need add this action to pending list.
            addToPendingList(new PendingAction("setVideoQuality", MSG_ACTION_SET_VIDEO_QUALITY,
                    Integer.valueOf(quality)));
        }
    }

    public void addCall(ImsCallSessionImpl callSession) {
        if (Utilities.DEBUG) Log.i(TAG, "Add the call[" + callSession + "] to list.");
        if (callSession == null) {
            Log.e(TAG, "Can not add this call[" + callSession + "] to list as it is null.");
            return;
        }

        mSessionList.add(callSession);
    }

    public void removeCall(ImsCallSessionImpl callSession) {
        if (Utilities.DEBUG) Log.i(TAG, "Remove the call[" + callSession + "] from the list.");
        if (callSession == null) {
            Log.e(TAG, "Can not remove this call[" + callSession + "] from list as it is null.");
            return;
        }

        // Remove the call session from the list.
        if (mSessionList.remove(callSession)) {
            Log.d(TAG, "The call[" + callSession + "] removed from the list.");
            // If the call removed, we need dismiss the alert dialog of this call session.
            dismissAlertDialog(Integer.parseInt(callSession.getCallId()));

            // It means the call is end now.
            if (mEmergencyCallSession != null
                    && callSession.equals(mEmergencyCallSession)) {
                // It means the emergency call end. Exit the ECBM.
                if (mListener != null) mListener.onExitECBM();
            } else if (mListener != null) {
                mListener.onCallEnd(callSession);
                if (mEmergencyCallSession != null) {
                    Log.e(TAG, "There is a call end, but not emergency call. Shouldn't be here.");
                }
            }

            // After remove the session, if the session list is empty, we need stop the audio.
            if (callSession.isAudioStart()) stopAudioStream();
        } else {
            Log.d(TAG, "Do not remove the call[" + callSession + "] from the list.");
        }
    }

    public void enterECBMWithCallSession(ImsCallSessionImpl emergencyCallSession) {
        mEmergencyCallSession = emergencyCallSession;

        if (mListener != null) mListener.onEnterECBM(mEmergencyCallSession);
    }

    /**
     * Return the call session list. This list may be empty if there isn't any call.
     */
    public ArrayList<ImsCallSessionImpl> getCallSessionList() {
        return mSessionList;
    }

    public boolean isCallFunEnabled() {
        return mRegisterState == RegisterState.STATE_CONNECTED;
    }

    public int getCallCount() {
        return mSessionList.size();
    }

    /**
     * Get the call session relate to this special call id.
     * @param id the session with this call id.
     * @return The call session for the given id. If couldn't found the session for given id or the
     *         given id is null, return null.
     */
    public ImsCallSessionImpl getCallSession(String id) {
        if (TextUtils.isEmpty(id)) {
            Log.w(TAG, "The param id is null, return null");
            return null;
        }

        for (ImsCallSessionImpl session : mSessionList) {
            if (id.equals(session.getCallId())) {
                Log.d(TAG, "Found the call session for this id: " + id);
                return session;
            }
        }

        // Can not found the call session relate to this id.
        Log.d(TAG, "Can not found the call session for this id: " + id);
        return null;
    }

    /**
     * Get the video call session list. Return null if can not found.
     */
    public ArrayList<ImsCallSessionImpl> getVideoCallSessions() {
        ArrayList<ImsCallSessionImpl> videoSessionList = new ArrayList<ImsCallSessionImpl>();
        for (ImsCallSessionImpl session : mSessionList) {
            if (Utilities.isVideoCall(session.getCallProfile().mCallType)) {
                videoSessionList.add(session);
            }
        }

        // If there isn't any video call session, return null.
        return videoSessionList.size() > 0 ? videoSessionList : null;
    }

    public ImsCallSessionImpl getAliveVideoCallSession() {
        ImsCallSessionImpl session = null;
        session = getAliveCallSession();
        if (session == null) return null;

        if (Utilities.isVideoCall(session.getCallProfile().mCallType)) {
            return session;
        }

        Log.d(TAG, "Can not found any video call in alive state, return null.");
        return null;
    }

    public ImsCallSessionImpl getAliveCallSession() {
        for (ImsCallSessionImpl session : mSessionList) {
            if (session.isAlive()) return session;
        }

        Log.w(TAG, "Can not found any call in active state, return null.");
        return null;
    }

    public ImsCallSessionImpl getConfCallSession() {
        for (ImsCallSessionImpl session : mSessionList) {
            if (session.isMultiparty()) return session;
        }

        Log.w(TAG, "Can not found any conference call.");
        return null;
    }

    public void onSRVCCStateChanged(int state) {
        if (!isCallFunEnabled()) {
            Log.d(TAG, "As call function disabled, do not handle the SRVCC state changed.");
            return;
        }

        Log.d(TAG, "The SRVCC state changed, state: " + state);
        mSRVCCHandler.sendEmptyMessage(state);
    }

    private ImsCallSessionImpl createCallSession(ImsCallProfile profile,
            IImsCallSessionListener listener, ImsVideoCallProviderImpl videoCallProvider,
            int callDir) {
        ImsCallSessionImpl session = new ImsCallSessionImpl(mContext, this, profile, listener,
                videoCallProvider, callDir);

        // Add this call session to the list.
        addCall(session);

        return session;
    }

    private void dismissAlertDialog(int sessionId) {
        if (mAlertDialog != null
                && mAlertDialog._sessionId == sessionId
                && mAlertDialog._dialog != null
                && mAlertDialog._dialog.isShowing()) {
            mAlertDialog._dialog.dismiss();
        }
    }

    private void inviteCall(ImsCallSessionImpl confSession) {
        if (Utilities.DEBUG) Log.i(TAG, "Try to invite call for the conference: " + confSession);

        if (confSession == null) {
            Log.e(TAG, "Failed to invite the call for the conference as it is null.");
            return;
        }

        // Invite participant.
        boolean success = false;
        try {
            ImsCallSessionImpl callSession = confSession.getNeedInviteCall();
            if (callSession == null) {
                Log.d(TAG, "All the calls already finish invite. Resume the conference call.");
                IImsCallSessionListener confListener = confSession.getListener();
                if (confListener != null) {
                    boolean isHeld = confSession.isHeld() || confSession.isAlive();
                    Log.d(TAG, "The conference call session is held: " + isHeld);
                    if (isHeld) {
                        // As conference connected, update it as resumed.
                        confSession.resume(confSession.getResumeMediaProfile());

                        confSession.updateAliveState(true);
                        confListener.callSessionResumed(confSession, confSession.getCallProfile());
                    }
                }
                return;
            }

            int res = mICall.confAddMembers(Integer.valueOf(confSession.getCallId()), null,
                    new int[] { Integer.valueOf(callSession.getCallId()) });
            if (res == Result.FAIL) {
                // Invite this call failed.
                Log.w(TAG, "Failed to invite the call " + callSession);
                String text = mContext.getString(R.string.vowifi_conf_invite_failed)
                        + callSession.getCallee();
                Toast.makeText(mContext, text, Toast.LENGTH_LONG).show();
            } else {
                success = true;
            }
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to invite the call as catch the RemoteException e: " + e);
        }

        if (!success) {
            // If the invite request send failed, we need try to invite other calls.
            mHandler.sendMessage(mHandler.obtainMessage(MSG_INVITE_CALL, confSession));
        }
    }

    private void handleEvent(String json) {
        try {
            JSONObject jObject = new JSONObject(json);
            int sessionId = jObject.optInt(JSONUtils.KEY_ID, -1);
            if (sessionId < 0) {
                Log.w(TAG, "The call session id is " + sessionId + ", need check.");
            }

            ImsCallSessionImpl callSession = getCallSession(Integer.toString(sessionId));
            if (callSession == null) {
                Log.w(TAG, "Can not found the call session for this call id: " + sessionId);
            }

            String eventName = jObject.optString(JSONUtils.KEY_EVENT_NAME, "");
            Log.d(TAG, "Handle the event '" + eventName + "' for the call: " + sessionId);

            int eventCode = jObject.optInt(JSONUtils.KEY_EVENT_CODE, -1);
            if (callSession != null) {
                VoWifiCallStateTracker tracker = callSession.getCallStateTracker();
                if (tracker != null) {
                    tracker.updateActionResponse(eventCode);
                }
            }

            switch (eventCode) {
                case JSONUtils.EVENT_CODE_CALL_INCOMING: {
                    boolean isVideo = jObject.optBoolean(JSONUtils.KEY_IS_VIDEO, false);
                    String callee = jObject.optString(JSONUtils.KEY_PHONE_NUM, "");
                    handleCallIncoming(sessionId, false /* not conference */, isVideo, callee);
                    break;
                }
                case JSONUtils.EVENT_CODE_CALL_OUTGOING:
                case JSONUtils.EVENT_CODE_CONF_OUTGOING:
                    Log.d(TAG, "Do nothing when the call or conference outgoing.");
                    break;
                case JSONUtils.EVENT_CODE_CALL_ALERTED: {
                    String phoneNumber = jObject.optString(JSONUtils.KEY_PHONE_NUM);
                    boolean isVideo = jObject.optBoolean(JSONUtils.KEY_IS_VIDEO, false);
                    handleCallAlerted(callSession, phoneNumber, isVideo);
                    break;
                }
                case JSONUtils.EVENT_CODE_CALL_TALKING: {
                    boolean isVideo = jObject.optBoolean(JSONUtils.KEY_IS_VIDEO, false);
                    handleCallTalking(callSession, isVideo);
                    break;
                }
                case JSONUtils.EVENT_CODE_CALL_TERMINATE: {
                    int stateCode = jObject.optInt(
                            JSONUtils.KEY_STATE_CODE, ImsReasonInfo.CODE_USER_TERMINATED);
                    handleCallTermed(callSession, stateCode);
                    break;
                }
                case JSONUtils.EVENT_CODE_CALL_HOLD_OK:
                case JSONUtils.EVENT_CODE_CALL_HOLD_FAILED:
                case JSONUtils.EVENT_CODE_CALL_RESUME_OK:
                case JSONUtils.EVENT_CODE_CALL_RESUME_FAILED:
                case JSONUtils.EVENT_CODE_CALL_HOLD_RECEIVED:
                case JSONUtils.EVENT_CODE_CALL_RESUME_RECEIVED:
                case JSONUtils.EVENT_CODE_CONF_HOLD_OK:
                case JSONUtils.EVENT_CODE_CONF_HOLD_FAILED:
                case JSONUtils.EVENT_CODE_CONF_RESUME_OK:
                case JSONUtils.EVENT_CODE_CONF_RESUME_FAILED:
                case JSONUtils.EVENT_CODE_CONF_HOLD_RECEIVED:
                case JSONUtils.EVENT_CODE_CONF_RESUME_RECEIVED: {
                    handleCallHoldOrResume(eventCode, callSession);
                    break;
                }
                case JSONUtils.EVENT_CODE_CALL_ADD_VIDEO_OK:
                case JSONUtils.EVENT_CODE_CALL_ADD_VIDEO_FAILED:
                case JSONUtils.EVENT_CODE_CALL_REMOVE_VIDEO_OK:
                case JSONUtils.EVENT_CODE_CALL_REMOVE_VIDEO_FAILED: {
                    handleCallUpdate(eventCode, callSession);
                    break;
                }
                case JSONUtils.EVENT_CODE_CALL_ADD_VIDEO_REQUEST: {
                    handleCallAddVideoRequest(callSession);
                    break;
                }
                case JSONUtils.EVENT_CODE_CALL_ADD_VIDEO_CANCEL: {
                    handleCallAddVideoCancel(callSession);
                    break;
                }
                case JSONUtils.EVENT_CODE_CALL_RTP_RECEIVED:
                case JSONUtils.EVENT_CODE_CONF_RTP_RECEIVED: {
                    boolean isVideo = jObject.optBoolean(JSONUtils.KEY_IS_VIDEO, false);
                    boolean isReceived = jObject.optBoolean(JSONUtils.KEY_RTP_RECEIVED, true);
                    handleRTPReceived(callSession, isVideo, isReceived);
                    break;
                }
                case JSONUtils.EVENT_CODE_CALL_IS_FOCUS: {
                    handleCallIsFocus(callSession);
                    break;
                }
                case JSONUtils.EVENT_CODE_CALL_IS_EMERGENCY: {
                    handleCallIsEmergency(callSession);
                    break;
                }
                case JSONUtils.EVENT_CODE_CONF_ALERTED: {
                    handleConfAlerted(callSession);
                    break;
                }
                case JSONUtils.EVENT_CODE_CONF_CONNECTED: {
                    handleConfConnected(callSession);
                    break;
                }
                case JSONUtils.EVENT_CODE_CONF_DISCONNECTED: {
                    int stateCode = jObject.optInt(
                            JSONUtils.KEY_STATE_CODE, ImsReasonInfo.CODE_USER_TERMINATED);
                    handleConfDisconnected(callSession, stateCode);
                    break;
                }
                case JSONUtils.EVENT_CODE_CONF_INVITE_ACCEPT:
                case JSONUtils.EVENT_CODE_CONF_INVITE_FAILED:
                case JSONUtils.EVENT_CODE_CONF_KICK_ACCEPT:
                case JSONUtils.EVENT_CODE_CONF_KICK_FAILED: {
                    String phoneNumber = jObject.optString(JSONUtils.KEY_PHONE_NUM, "");
                    String phoneUri = jObject.optString(JSONUtils.KEY_SIP_URI, "");
                    handleConfParticipantsChanged(
                            eventCode, callSession, phoneNumber, phoneUri, null);
                    break;
                }
                case JSONUtils.EVENT_CODE_CONF_PART_UPDATE: {
                    String phoneNumber = jObject.optString(JSONUtils.KEY_PHONE_NUM, "");
                    String phoneUri = jObject.optString(JSONUtils.KEY_SIP_URI, "");
                    String newStatus = jObject.optString(JSONUtils.KEY_CONF_PART_NEW_STATUS, "");
                    handleConfParticipantsChanged(
                            eventCode, callSession, phoneNumber, phoneUri, newStatus);
                    break;
                }
                case JSONUtils.EVENT_CODE_LOCAL_VIDEO_RESIZE:
                case JSONUtils.EVENT_CODE_REMOTE_VIDEO_RESIZE: {
                    // FIXME: This callback do not give the call session id, so need find the
                    // alive video call from the call list.
                    if (callSession == null) {
                        callSession = getAliveVideoCallSession();
                    }
                    int width = jObject.optInt(JSONUtils.KEY_VIDEO_WIDTH, -1);
                    int height = jObject.optInt(JSONUtils.KEY_VIDEO_HEIGHT, -1);

                    handleVideoResize(eventCode, callSession, width, height);
                    break;
                }
                case JSONUtils.EVENT_CODE_CALL_RTCP_CHANGED:
                case JSONUtils.EVENT_CODE_CONF_RTCP_CHANGED: {
                    ImsCallSessionImpl aliveCallSession = getAliveCallSession();
                    if (aliveCallSession != null && aliveCallSession.equals(callSession)) {
                        int lose = jObject.optInt(JSONUtils.KEY_RTCP_LOSE, -1);
                        int jitter = jObject.optInt(JSONUtils.KEY_RTCP_JITTER, -1);
                        int rtt = jObject.optInt(JSONUtils.KEY_RTCP_RTT, -1);
                        boolean isVideo = jObject.optBoolean(JSONUtils.KEY_IS_VIDEO, false);
                        handleRTCPChanged(callSession, lose, jitter, rtt, isVideo);
                    } else {
                        Log.w(TAG, "The alive call do not same as the rtcp changed. Ignore this: "
                                + callSession + ", and the alive call is: " + aliveCallSession);
                    }
                    break;
                }
                default:
                    Log.w(TAG, "The event '" + eventName + "' do not handle, please check!");
            }
        } catch (JSONException e) {
            Log.e(TAG, "Can not handle the json, catch the JSONException e: " + e);
        } catch (RemoteException e) {
            Log.e(TAG, "Can not handle the event, catch the RemoteException e: " + e);
        } catch (UnsupportedCallTypeException e) {
            Log.e(TAG, "Can not handle the event, catch the UnsupportedCallTypeException e: " + e);
        }
    }

    private void handleCallAlerted(ImsCallSessionImpl callSession, String phoneNumber,
            boolean isVideo) throws RemoteException {
        if (Utilities.DEBUG) Log.i(TAG, "Handle the alerted or outgoing call.");
        if (callSession == null) {
            Log.w(TAG, "[handleAlertedOrOutgoing] The call session is null");
            return;
        }

        ImsStreamMediaProfile mediaProfile = null;
        if (isVideo) {
            mediaProfile = new ImsStreamMediaProfile(
                    ImsStreamMediaProfile.AUDIO_QUALITY_AMR_WB,
                    ImsStreamMediaProfile.DIRECTION_SEND_RECEIVE,
                    ImsStreamMediaProfile.VIDEO_QUALITY_QCIF,
                    ImsStreamMediaProfile.DIRECTION_SEND_RECEIVE);
        } else {
            mediaProfile = new ImsStreamMediaProfile();
        }
        callSession.updateMediaProfile(mediaProfile);
        callSession.updateState(State.NEGOTIATING);

        IImsCallSessionListener listener = callSession.getListener();
        if (listener != null) {
            listener.callSessionProgressing(callSession, mediaProfile);
            String callee = callSession.getCallee();
            if (!callee.equals(phoneNumber)) {
                listener.callSessionUpdated(callSession, callSession.updateCallee(phoneNumber));
            }
        }
    }

    private void handleCallIncoming(int sessionId, boolean isConference, boolean isVideo,
            String callee) throws RemoteException {
        if (Utilities.DEBUG) Log.i(TAG, "Handle the incoming call.");

        // Create the profile for this incoming call.
        ImsCallProfile callProfile = null;
        ImsStreamMediaProfile mediaProfile = null;
        if (isVideo && !isConference) {
            callProfile = new ImsCallProfile(ImsCallProfile.SERVICE_TYPE_NORMAL,
                    ImsCallProfile.CALL_TYPE_VT);
            mediaProfile = new ImsStreamMediaProfile(
                    ImsStreamMediaProfile.AUDIO_QUALITY_AMR_WB,
                    ImsStreamMediaProfile.DIRECTION_SEND_RECEIVE,
                    ImsStreamMediaProfile.VIDEO_QUALITY_QCIF,
                    ImsStreamMediaProfile.DIRECTION_SEND_RECEIVE);
        } else {
            callProfile = new ImsCallProfile(ImsCallProfile.SERVICE_TYPE_NORMAL,
                    ImsCallProfile.CALL_TYPE_VOICE);
            mediaProfile = new ImsStreamMediaProfile();
        }

        callProfile.setCallExtra(ImsCallProfile.EXTRA_OI, callee);
        callProfile.setCallExtra(ImsCallProfile.EXTRA_CNA, null);
        callProfile.setCallExtraInt(
                ImsCallProfile.EXTRA_CNAP, ImsCallProfile.OIR_PRESENTATION_NOT_RESTRICTED);
        callProfile.setCallExtraInt(
                ImsCallProfile.EXTRA_OIR, ImsCallProfile.OIR_PRESENTATION_NOT_RESTRICTED);
        callProfile.setCallExtraBoolean(ImsCallProfile.EXTRA_CONFERENCE, isConference);

        ImsCallSessionImpl callSession = createMTCallSession(callProfile, null);
        callSession.addCallee(callee);
        callSession.setCallId(sessionId);
        callSession.updateMediaProfile(mediaProfile);
        callSession.updateState(State.NEGOTIATING);
        callSession.updateRequestAction(VoWifiCallStateTracker.ACTION_INCOMING);

        // If the current incoming call action is reject or call function is disabled,
        // we need reject the incoming call and give the reason as the local call is busy.
        // Note: The reject action will be set when the secondary card is in the calling,
        //       then we need reject all the incoming call from the VOWIFI.
        if (!isCallFunEnabled()
                || mIncomingCallAction == IncomingCallAction.REJECT) {
            callSession.reject(ImsReasonInfo.CODE_USER_DECLINE);
        } else {
            // Send the incoming call callback.
            if (mListener != null) mListener.onCallIncoming(callSession);

            IImsCallSessionListener listener = callSession.getListener();
            if (listener != null) {
                listener.callSessionProgressing(callSession, callSession.getMediaProfile());
            }

            // If the user enable the auto answer prop, we need answer this call immediately.
            boolean isAutoAnswer = SystemProperties.getBoolean(PROP_KEY_AUTO_ANSWER, false);
            if (isAutoAnswer) {
                callSession.accept(callProfile.mCallType, mediaProfile);
            }
        }
    }

    private void handleCallTermed(ImsCallSessionImpl callSession, int termReasonCode)
            throws RemoteException {
        if (Utilities.DEBUG) Log.i(TAG, "Handle the termed call.");
        if (callSession == null) {
            Log.w(TAG, "[handleCallTermed] The call session is null.");
            return;
        }

        // As call terminated, stop all the video.
        // Note: This stop action need before call session terminated callback. Otherwise,
        //       the video call provider maybe changed to null.
        ImsVideoCallProviderImpl videoCallProvider = callSession.getVideoCallProviderImpl();
        if (videoCallProvider != null) {
            videoCallProvider.stopAll();
        }

        int oldState = callSession.getState();
        callSession.updateState(State.TERMINATED);
        IImsCallSessionListener listener = callSession.getListener();
        if (listener != null) {
            ImsReasonInfo info = new ImsReasonInfo(termReasonCode, termReasonCode,
                    "The call terminated.");
            if (oldState < State.NEGOTIATING) {
                // It means the call do not ringing now, so we need give the call session start
                // failed call back.
                listener.callSessionStartFailed(callSession, info);
            } else {
                ImsCallSessionImpl confSession = callSession.getConfCallSession();
                if (confSession != null) {
                    // If the conference session is not null, it means this call session was
                    // invite as the participant, and when it received the terminate action,
                    // need set the info code as CODE_USER_TERMINATED_BY_REMOTE, and it will
                    // do not play the tone when disconnect.
                    info.mCode = ImsReasonInfo.CODE_USER_TERMINATED_BY_REMOTE;
                }

                // Terminate the call as normal.
                listener.callSessionTerminated(callSession, info);

                // After give the callback, close this call session if it is a participant
                // of a conference.
                if (confSession != null) callSession.close();
            }
        }

        removeCall(callSession);
    }

    private void handleCallTalking(ImsCallSessionImpl callSession, boolean isVideo)
            throws RemoteException {
        if (Utilities.DEBUG) Log.i(TAG, "Handle the talking call.");
        if (callSession == null) {
            Log.w(TAG, "[handleCallTalking] The call session is null.");
            return;
        }

        callSession.updateState(ImsCallSession.State.ESTABLISHED);

        // Update the call type, as if the user accept the video call as audio call,
        // isVideo will be false. Then we need update this to call profile.
        ImsCallProfile profile = callSession.getCallProfile();
        if (!isVideo) {
            profile.mCallType = ImsCallProfile.CALL_TYPE_VOICE;
        }

        IImsCallSessionListener listener = callSession.getListener();
        if (listener != null) {
            listener.callSessionStarted(callSession, profile);
        }
    }

    private void handleCallHoldOrResume(int eventCode, ImsCallSessionImpl callSession)
            throws RemoteException {
        if (Utilities.DEBUG) Log.i(TAG, "Handle the hold or resume event.");
        if (callSession == null) {
            Log.w(TAG, "[handleCallHoldOrResume] The call session is null.");
            return;
        }

        IImsCallSessionListener listener = callSession.getListener();
        if (listener == null) {
            Log.w(TAG, "The call session's listener is null, can't alert the hold&resume result");
            return;
        }

        int toastTextResId = -1;
        switch (eventCode) {
            case JSONUtils.EVENT_CODE_CALL_HOLD_OK:
            case JSONUtils.EVENT_CODE_CONF_HOLD_OK: {
                toastTextResId = R.string.vowifi_hold_success;
                callSession.updateAliveState(false /* held, do not alive */);
                listener.callSessionHeld(callSession, callSession.getCallProfile());
                // As the call hold, if the call is video call, we need stop all the video.
                ImsVideoCallProviderImpl videoProvider = callSession.getVideoCallProviderImpl();
                if (videoProvider != null) videoProvider.stopAll();
                break;
            }
            case JSONUtils.EVENT_CODE_CALL_HOLD_FAILED:
            case JSONUtils.EVENT_CODE_CONF_HOLD_FAILED: {
                toastTextResId = R.string.vowifi_hold_fail;
                ImsReasonInfo holdFailedInfo = new ImsReasonInfo(ImsReasonInfo.CODE_UNSPECIFIED,
                        ImsReasonInfo.CODE_UNSPECIFIED, "Unknown reason");
                listener.callSessionHoldFailed(callSession, holdFailedInfo);
                break;
            }
            case JSONUtils.EVENT_CODE_CALL_RESUME_OK:
            case JSONUtils.EVENT_CODE_CONF_RESUME_OK: {
                toastTextResId = R.string.vowifi_resume_success;
                callSession.updateAliveState(true /* resumed, alive now */);
                listener.callSessionResumed(callSession, callSession.getCallProfile());
                break;
            }
            case JSONUtils.EVENT_CODE_CALL_RESUME_FAILED:
            case JSONUtils.EVENT_CODE_CONF_RESUME_FAILED: {
                toastTextResId = R.string.vowifi_resume_fail;
                ImsReasonInfo resumeFailedInfo = new ImsReasonInfo(ImsReasonInfo.CODE_UNSPECIFIED,
                        ImsReasonInfo.CODE_UNSPECIFIED, "Unknown reason");
                listener.callSessionResumeFailed(callSession, resumeFailedInfo);
                break;
            }
            case JSONUtils.EVENT_CODE_CALL_HOLD_RECEIVED:
            case JSONUtils.EVENT_CODE_CONF_HOLD_RECEIVED: {
                listener.callSessionHoldReceived(callSession, callSession.getCallProfile());
                break;
            }
            case JSONUtils.EVENT_CODE_CALL_RESUME_RECEIVED:
            case JSONUtils.EVENT_CODE_CONF_RESUME_RECEIVED: {
                listener.callSessionResumeReceived(callSession, callSession.getCallProfile());
                break;
            }
            default: {
                Log.w(TAG, "The event " + eventCode + " do not belongs to hold or resume.");
                break;
            }
        }

        // If set the toast text, we need show the toast.
        if (toastTextResId > 0) {
            Toast.makeText(mContext, toastTextResId, Toast.LENGTH_LONG).show();
        }
    }

    private void handleCallUpdate(int eventCode, ImsCallSessionImpl callSession)
            throws RemoteException {
        if (Utilities.DEBUG) Log.i(TAG, "Handle the call update ok.");
        if (callSession == null) {
            Log.w(TAG, "[handleCallUpdate] The call session is null.");
            return;
        }

        IImsCallSessionListener listener = callSession.getListener();
        ImsVideoCallProviderImpl videoCallProvider = callSession.getVideoCallProviderImpl();
        if (eventCode == JSONUtils.EVENT_CODE_CALL_ADD_VIDEO_OK
                || eventCode == JSONUtils.EVENT_CODE_CALL_REMOVE_VIDEO_OK) {
            // Update the call type success.
            boolean isVideo = eventCode == JSONUtils.EVENT_CODE_CALL_ADD_VIDEO_OK;
            ImsCallProfile callProfile = callSession.getCallProfile();
            if (callProfile != null) {
                callProfile.mCallType =
                        isVideo ? ImsCallProfile.CALL_TYPE_VT : ImsCallProfile.CALL_TYPE_VOICE;
            } else {
                Log.e(TAG, "The call profile is null for this call: " + callSession);
            }

            if (listener != null) {
                listener.callSessionUpdated(callSession, callProfile);
            }

            if (videoCallProvider != null) {
                if (eventCode == JSONUtils.EVENT_CODE_CALL_ADD_VIDEO_OK
                        && videoCallProvider.isWaitForModifyResponse()) {
                    // If we send the request to callee, and the callee accept the request, need
                    // prompt the toast here.
                    Toast.makeText(mContext, R.string.vowifi_add_video_success,
                            Toast.LENGTH_LONG).show();
                } else if (eventCode == JSONUtils.EVENT_CODE_CALL_REMOVE_VIDEO_OK) {
                    // Show toast for remove video action.
                    Toast.makeText(mContext, R.string.vowifi_remove_video_success,
                            Toast.LENGTH_LONG).show();

                    // As remove video ok, we'd like to stop all the video before the response.
                    // If the surface destroyed, the remove render action will be blocked, and
                    // the remove action will failed actually. So we'd like to stop all the video
                    // before give the response.
                    videoCallProvider.stopAll();
                }

                VideoProfile videoProfile = (isVideo
                        ? new VideoProfile(VideoProfile.STATE_BIDIRECTIONAL)
                        : new VideoProfile(VideoProfile.STATE_AUDIO_ONLY));
                // As the response is OK, the request profile will be same as the response.
                videoCallProvider.receiveSessionModifyResponse(
                        VideoProvider.SESSION_MODIFY_REQUEST_SUCCESS,
                        videoProfile /* request profile */,
                        videoProfile /* response profile */);
            } else {
                Log.e(TAG, "The video call provider is null, can not give the response.");
            }
        } else {
            // Failed to update the call type.
            if (listener != null) {
                listener.callSessionUpdateFailed(callSession, new ImsReasonInfo());
            }
            if (videoCallProvider != null) {
                videoCallProvider.receiveSessionModifyResponse(
                        VideoProvider.SESSION_MODIFY_REQUEST_FAIL, null, null);
            }
            // Show toast for failed action.
            int toastTextResId = eventCode == JSONUtils.EVENT_CODE_CALL_ADD_VIDEO_FAILED
                    ? R.string.vowifi_add_video_failed : R.string.vowifi_remove_video_failed;
            Toast.makeText(mContext, toastTextResId, Toast.LENGTH_LONG).show();
        }
    }

    private void handleCallAddVideoRequest(ImsCallSessionImpl callSession) {
        if (Utilities.DEBUG) Log.i(TAG, "Handle the call add video request.");
        if (callSession == null) {
            Log.w(TAG, "[handleCallAddVideoRequest] The call session is null.");
            return;
        }

        // Receive the add video request, we need prompt one dialog to alert the user.
        // And the user could accept or reject this action.
        mAlertDialog = getVideoChangeRequestDialog(
                Integer.valueOf(callSession.getCallId()), true /* add video is upgrade action */);
        mAlertDialog._dialog.show();

        // For most situation, when we receive the upgrade video request, the screen is off.
        // And the user do not focus on the screen. So we'd like to give a tone alert when
        // the user receive this request.
        // FIXME: As InCallUI will give the tone when it receive "SessionModifyResponse"
        //        with the status is VideoProvider.SESSION_MODIFY_REQUEST_INVALID. Couldn't
        //        very understand, why not use the "SessionModifyRequest"?
        ImsVideoCallProviderImpl videoProvider = callSession.getVideoCallProviderImpl();
        if (videoProvider != null) {
            videoProvider.receiveSessionModifyResponse(
                    VideoProvider.SESSION_MODIFY_REQUEST_INVALID /* used to play the tone */,
                    null /* not used */, null /* not used */);
        }
    }

    private void handleCallAddVideoCancel(ImsCallSessionImpl callSession) {
        if (Utilities.DEBUG) Log.i(TAG, "Handle the call add video cancel.");
        if (callSession == null) {
            Log.w(TAG, "[handleCallAddVideoCancel] The call session is null.");
            return;
        }

        dismissAlertDialog(Integer.valueOf(callSession.getCallId()));
    }

    private void handleCallIsFocus(ImsCallSessionImpl callSession) throws RemoteException {
        if (Utilities.DEBUG) Log.i(TAG, "Handle the call is focus.");
        if (callSession == null) {
            Log.w(TAG, "[handleCallIsFocus] The call session is null.");
            return;
        }

        ImsCallProfile callProfile = callSession.getCallProfile();
        callProfile.setCallExtraBoolean(ImsCallProfile.EXTRA_CONFERENCE, true);
        Toast.makeText(mContext, R.string.vowifi_call_is_focus, Toast.LENGTH_LONG).show();

        IImsCallSessionListener listener = callSession.getListener();
        if (listener != null) {
            listener.callSessionMultipartyStateChanged(callSession, true);
        }
    }

    private void handleCallIsEmergency(ImsCallSessionImpl callSession) {
        if (Utilities.DEBUG) Log.i(TAG, "Handle the call is emergency.");
        if (callSession == null) {
            Log.w(TAG, "[handleCallIsEmergency] The call session is null.");
            return;
        }

        if (mListener != null) mListener.onCallIsEmergency(callSession);
    }

    private void handleVideoResize(int eventCode, ImsCallSessionImpl callSession, int width,
            int height) {
        if (Utilities.DEBUG) Log.i(TAG, "Handle video resize.");
        if (callSession == null) {
            Log.w(TAG, "[handleCallRemoteVideoResize] The call session is null.");
            return;
        }

        if (width < 0 || height < 0) {
            Log.e(TAG, "The width is: " + width + ", the height is: " + height + ", invalid.");
            return;
        }

        ImsVideoCallProviderImpl videoProvider = callSession.getVideoCallProviderImpl();
        if (videoProvider == null) {
            Log.e(TAG, "Failed to update the video size as video provider is null.");
            return;
        }

        if (eventCode == JSONUtils.EVENT_CODE_REMOTE_VIDEO_RESIZE) {
            Log.d(TAG, "The call " + callSession.getCallId() + " remote video resize: "
                    + width + "," + height);
            videoProvider.changePeerDimensions(width, height);
        } else if (eventCode == JSONUtils.EVENT_CODE_LOCAL_VIDEO_RESIZE) {
            if (!callSession.isMultiparty()) {
                // If the call do not conference, change the camera capabilities.
                CameraCapabilities newCap = new CameraCapabilities(width, height);
                if (!videoProvider.cameraCapabilitiesEquals(newCap)) {
                    Log.d(TAG, "The call " + callSession.getCallId() + " local video resize: "
                            + width + "," + height);
                    videoProvider.changeCameraCapabilities(newCap);
                }
            }
        }
    }

    private void handleRTCPChanged(ImsCallSessionImpl callSession, int lose, int jitter, int rtt,
            boolean isVideo) throws UnsupportedCallTypeException {
        if (Utilities.DEBUG) Log.i(TAG, "Handle the rtcp changed.");
        if (callSession == null) {
            Log.w(TAG, "[handleRTCPChanged] The call session is null.");
            return;
        }

        if (mListener != null) {
            mListener.onCallRTCPChanged(isVideo, lose, jitter, rtt);
        }
    }

    private void handleConfAlerted(ImsCallSessionImpl confSession) {
        if (Utilities.DEBUG) Log.i(TAG, "Handle the conference alerted.");
        if (confSession == null) {
            Log.w(TAG, "[handleConfConnected] The conference session is null ");
            return;
        }

        confSession.updateState(State.NEGOTIATING);
    }

    private void handleConfConnected(ImsCallSessionImpl confSession)
            throws NumberFormatException, RemoteException, UnsupportedCallTypeException {
        if (Utilities.DEBUG) Log.i(TAG, "Handle the conference connected.");
        if (confSession == null && mICall != null) {
            Log.w(TAG, "[handleConfConnected] The conference session or call interface is null ");
            return;
        }

        confSession.updateState(State.ESTABLISHED);
        confSession.updateAliveState(true);
        ImsCallSessionImpl hostCallSession = confSession.getHostCallSession();
        if (hostCallSession == null) {
            // It means this conference is start with the participants. Needn't notify merge
            // complete and invite the peers.
            return;
        }

        IImsCallSessionListener hostListener = hostCallSession.getListener();
        if (hostListener != null) {
            Log.d(TAG, "Notify the merge complete.");
            hostListener.callSessionMergeComplete(confSession);
            // As merge complete, set the host call session as null.
            confSession.setHostCallSession(null);
        }

        IImsCallSessionListener confListener = confSession.getListener();
        // Notify the multi-party state changed.
        if (confListener != null) {
            confListener.callSessionMultipartyStateChanged(confSession, true);
        }

        boolean hostIsVideo = Utilities.isVideoCall(hostCallSession.getCallProfile().mCallType);
        boolean confIsVideo = Utilities.isVideoCall(confSession.getCallProfile().mCallType);
        if (!hostIsVideo && confIsVideo) {
            // It means the host isn't video call, but the conference is video call. We need
            // update the conference to video here.
            if (confListener != null) {
                confListener.callSessionUpdated(confSession, confSession.getCallProfile());
            }

            ImsVideoCallProviderImpl videoCallProvider = confSession.getVideoCallProviderImpl();
            if (videoCallProvider != null) {
                VideoProfile videoProfile = new VideoProfile(VideoProfile.STATE_BIDIRECTIONAL);
                videoCallProvider.receiveSessionModifyResponse(
                        VideoProvider.SESSION_MODIFY_REQUEST_SUCCESS,
                        videoProfile /* request profile */,
                        videoProfile /* response profile */);
            }
        }

        // The conference call session is connected to the service. And now we could invite the
        // other call session as the participants and update the states.
        for (ImsCallSessionImpl callSession : getCallSessionList()) {
            if (callSession == confSession) {
                // This call session is the conference call session, do nothing.
                continue;
            }

            Log.d(TAG, "This call " + callSession + " will be invite to this conference call.");
            // Update the need merged call session.
            callSession.setConfCallSession(confSession);
            // As this call session will be invited to this conference, update the state and
            // stop all the video.
            callSession.updateAliveState(false);
            callSession.updateState(State.TERMINATING);
            callSession.getVideoCallProviderImpl().stopAll();

            // For normal call, there will be only one participant, and we need add it to this
            // conference participants list.
            String participant = callSession.getCallee();
            confSession.addCallee(participant);
            confSession.addAsWaitForInvite(callSession);
        }

        // Start to invite the calls.
        mHandler.sendMessage(mHandler.obtainMessage(MSG_INVITE_CALL, confSession));
    }

    private void handleConfDisconnected(ImsCallSessionImpl confSession, int stateCode)
            throws RemoteException {
        if (Utilities.DEBUG) Log.i(TAG, "Handle the conference disconnected.");
        if (confSession == null) {
            Log.w(TAG, "[handleConfDisconnected] The conference session is null ");
            return;
        }

        // Notify the merge failed.
        ImsCallSessionImpl hostCallSession = confSession.getHostCallSession();
        // The host call session may be null.
        if (hostCallSession != null) {
            // It means this conference merge failed
            IImsCallSessionListener hostListener = hostCallSession.getListener();
            if (hostListener != null) {
                Log.d(TAG, "Notify the merge failed.");
                ImsReasonInfo info = new ImsReasonInfo(ImsReasonInfo.CODE_UNSPECIFIED, stateCode);
                hostListener.callSessionMergeFailed(confSession, info);
            }
        }

        if (confSession.getState() >= State.ESTABLISHED) {
            // It means the conference already connect before. Now finished and disconnect.
            Toast.makeText(mContext, R.string.vowifi_conf_finished, Toast.LENGTH_LONG).show();
        } else {
            Toast.makeText(mContext, R.string.vowifi_conf_disconnect, Toast.LENGTH_LONG).show();
        }

        // Terminate this conference call.
        handleCallTermed(confSession, ImsReasonInfo.CODE_UNSPECIFIED);
    }

    private void handleConfParticipantsChanged(int eventCode, ImsCallSessionImpl confSession,
            String phoneNumber, String phoneUri, String newStatus) throws RemoteException {
        if (Utilities.DEBUG) Log.i(TAG, "Handle the conference participant update result.");

        if (confSession == null) {
            Log.w(TAG, "[handleConfParticipantsChanged] The conference session is null ");
            return;
        }

        if (TextUtils.isEmpty(phoneNumber) || TextUtils.isEmpty(phoneUri)) {
            Log.e(TAG, "Faile to handle the parts changed as the phone number or uri is empty.");
            return;
        }

        boolean needUpdateState = true;
        boolean needInviteNext = false;
        String bundleKey = null;
        Bundle bundle = new Bundle();
        bundle.putString(ImsConferenceState.USER, phoneNumber);
        bundle.putString(ImsConferenceState.DISPLAY_TEXT, phoneNumber);
        bundle.putString(ImsConferenceState.ENDPOINT, phoneUri);
        switch (eventCode) {
            case JSONUtils.EVENT_CODE_CONF_INVITE_ACCEPT: {
                Log.d(TAG, "Get the invite accept result for the user: " + phoneNumber);
                // It means the call accept to join the conference.
                ImsCallSessionImpl callSession = confSession.getInInviteCall();
                String callee = callSession.getCallee();
                // Add this call session as the conference's participant.
                confSession.addParticipant(callee, callSession);

                bundleKey = callee;
                bundle.putString(ImsConferenceState.USER, callee);
                bundle.putString(ImsConferenceState.DISPLAY_TEXT, callee);
                bundle.putString(ImsConferenceState.STATUS, ImsConferenceState.STATUS_CONNECTED);

                // As invite success, need invite the next participant.
                needInviteNext = true;
                break;
            }
            case JSONUtils.EVENT_CODE_CONF_INVITE_FAILED: {
                Log.d(TAG, "Get the invite failed result for the user: " + phoneNumber);
                // It means failed to invite the call to this conference.
                ImsCallSessionImpl callSession = confSession.getInInviteCall();
                String callee = callSession.getCallee();

                bundleKey = callee;
                bundle.putString(ImsConferenceState.USER, callee);
                bundle.putString(ImsConferenceState.DISPLAY_TEXT, callee);
                bundle.putString(ImsConferenceState.STATUS, ImsConferenceState.STATUS_DISCONNECTED);

                // As invite failed, need invite the next participant.
                needInviteNext = true;
                // If this call invite failed, remove the callee and terminate the failed call.
                confSession.removeCallee(callee);
                callSession.terminate(ImsReasonInfo.CODE_USER_TERMINATED);

                // Show the notify toast to the user.
                String text = mContext.getString(R.string.vowifi_conf_invite_failed) + phoneNumber;
                Toast.makeText(mContext, text, Toast.LENGTH_LONG).show();
                break;
            }
            case JSONUtils.EVENT_CODE_CONF_KICK_ACCEPT: {
                Log.d(TAG, "Get the kick accept result for the user: " + phoneNumber);
                // It means this call already disconnect.
                String callee = confSession.removeCallee(phoneNumber);
                if (TextUtils.isEmpty(callee)) {
                    Log.w(TAG, "Can not find the phoneNumber from callee list.");
                    callee = phoneNumber;
                } else {
                    // Remove from the conference's participants.
                    confSession.removeParticipant(callee);
                    confSession.kickActionFinished(callee);
                }

                bundleKey = callee;
                bundle.putString(ImsConferenceState.USER, callee);
                bundle.putString(ImsConferenceState.DISPLAY_TEXT, callee);
                bundle.putString(ImsConferenceState.STATUS, ImsConferenceState.STATUS_DISCONNECTED);
                break;
            }
            case JSONUtils.EVENT_CODE_CONF_KICK_FAILED: {
                Log.d(TAG, "Get the kick failed for the user: " + phoneNumber);
                needUpdateState = false;
                String callee = confSession.findCallee(phoneNumber);
                confSession.kickActionFinished(callee);
                if (!TextUtils.isEmpty(callee)) {
                    // Find the callee, then show the notify toast to user.
                    Toast.makeText(mContext, R.string.vowifi_conf_kick_failed, Toast.LENGTH_LONG)
                            .show();
                }
                break;
            }
            case JSONUtils.EVENT_CODE_CONF_PART_UPDATE: {
                Log.d(TAG, "Get the new status for the user: " + phoneNumber);
                if (!TextUtils.isEmpty(newStatus)
                        && ImsConferenceState.STATUS_DISCONNECTED.equals(newStatus)) {
                    // If the new status is disconnected, we need remove it from the participants.
                    String callee = confSession.removeCallee(phoneNumber);
                    if (TextUtils.isEmpty(callee)) {
                        Log.w(TAG, "Can not find the phoneNumber from callee list.");
                        callee = phoneNumber;
                    } else {
                        // Remove from the conference's participants.
                        confSession.removeParticipant(callee);
                    }

                    bundleKey = callee;
                    bundle.putString(ImsConferenceState.USER, callee);
                    bundle.putString(ImsConferenceState.DISPLAY_TEXT, callee);
                    bundle.putString(ImsConferenceState.STATUS, newStatus);
                } else {
                    needUpdateState = false;
                    Log.e(TAG, "Failed to update the participant new status: " + newStatus);
                }
                break;
            }
        }

        // After update the participants, if there isn't any participant, need terminate it.
        if (confSession.getParticipantsCount() < 1) {
            // Terminate this conference call.
            handleCallTermed(confSession, ImsReasonInfo.CODE_LOCAL_CALL_TERMINATED);
            Toast.makeText(mContext, R.string.vowifi_conf_none_participant, Toast.LENGTH_LONG)
                    .show();
        } else if (needUpdateState) {
            confSession.updateConfParticipants(bundleKey, bundle);

            IImsCallSessionListener confListener = confSession.getListener();
            if (confListener != null) {
                confListener.callSessionConferenceStateUpdated(
                        confSession, confSession.getConfParticipantsState());
            }
        }

        if (needInviteNext) {
            // Send the message to invite the next participant.
            mHandler.sendMessage(mHandler.obtainMessage(MSG_INVITE_CALL, confSession));
        }
    }

    private void handleRTPReceived(ImsCallSessionImpl callSession, boolean isVideo,
            boolean isReceived) {
        if (Utilities.DEBUG) Log.i(TAG, "Handle the call RTP received: " + isReceived);
        if (callSession == null) {
            Log.e(TAG, "[handleRTPReceived] The call session is null.");
            return;
        }

        if (!callSession.isAlive()) {
            Log.d(TAG, "The call " + callSession.getCallId() + " isn't alive, do nothing.");
            return;
        }

        ImsCallSessionImpl confSession = callSession.getConfCallSession();
        if (confSession != null) {
            Log.d(TAG, "The call " + callSession.getCallId() + " will be invited to conference: "
                    + confSession.getCallId() + ", do nothing.");
            return;
        }

        if (mListener != null) mListener.onCallRTPReceived(isVideo, isReceived);
    }

    /**
     * This dialog will be shown when the call upgrade or downgrade.
     *
     * @param sessionId
     * @param isUpgrade
     * @return
     */
    private MyAlertDialog getVideoChangeRequestDialog(final int sessionId,
            final boolean isUpgrade) {
        // Build the dialog.
        String title = "";
        String message = "";
        String acceptText = mContext.getString(R.string.remote_request_change_accept);
        String rejectText = mContext.getString(R.string.remote_request_change_reject);
        if (isUpgrade) {
            title = mContext.getString(R.string.vowifi_request_upgrade_title);
            message = mContext.getString(R.string.vowifi_request_upgrade_text);
        } else {
            title = mContext.getString(R.string.vowifi_request_downgrade_title);
            message = mContext.getString(R.string.vowifi_request_downgrade_text);
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        builder.setTitle(title);
        builder.setMessage(message);
        builder.setPositiveButton(acceptText, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                // Accept the request, send the modify response.
                if (mICall == null) {
                    Log.e(TAG, "Can not send the session modify request for accept.");
                    return;
                }

                // If the user accept the upgrade action, it should be upgrade to video call,
                // so isVideo is true. And if the user accept the downgrade action, it should
                // be downgrade to voice call, so isVideo is false.
                boolean isVideo = isUpgrade ? true : false;
                try {
                    mICall.sendSessionModifyResponse(sessionId, true, isVideo);
                } catch (RemoteException e) {
                    Log.e(TAG, "Failed to send the accept response. e: " + e);
                }
            }
        });
        builder.setNegativeButton(rejectText, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                // Reject the request, send the modify response.
                if (mICall == null) {
                    Log.e(TAG, "Can not send the session modify request for reject.");
                    return;
                }

                // If the user reject the upgrade action, it should be keep as voice call,
                // so isVideo is false. And if the user reject the downgrade action, it
                // should be keep as video call, so isVideo is true.
                boolean isVideo = isUpgrade ? false : true;
                try {
                    mICall.sendSessionModifyResponse(sessionId, true, isVideo);
                } catch (RemoteException e) {
                    Log.e(TAG, "Failed to send reject response. e: " + e);
                }
            }
        });
        builder.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                if (mAlertDialog != null
                        && mAlertDialog._sessionId == sessionId) {
                    mAlertDialog = null;
                }
            }
        });
        builder.setCancelable(false);

        AlertDialog dialog = builder.create();
        dialog.setCanceledOnTouchOutside(false);
        dialog.getWindow().setType(WindowManager.LayoutParams.TYPE_KEYGUARD_DIALOG);
        dialog.getWindow().addFlags(WindowManager.LayoutParams.FLAG_BLUR_BEHIND
                | WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED
                | WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD
                | WindowManager.LayoutParams.FLAG_TURN_SCREEN_ON);

        return new MyAlertDialog(sessionId, dialog);
    }

    private String getDRStateString(int state) {
        switch (state) {
            case CallStateForDataRouter.VOLTE:
                return "volte";
            case CallStateForDataRouter.VOWIFI:
                return "vowifi";
            case CallStateForDataRouter.NONE:
                return "none";
        }

        return null;
    }

    private class MySerServiceCallback extends IVoWifiSerServiceCallback.Stub {
        @Override
        public void onEvent(String json) {
            if (Utilities.DEBUG) Log.i(TAG, "Get the vowifi ser event callback.");
            if (TextUtils.isEmpty(json)) {
                Log.e(TAG, "Can not handle the ser callback as the json is null.");
                return;
            }

            Message msg = mHandler.obtainMessage(MSG_HANDLE_EVENT);
            msg.obj = json;
            mHandler.sendMessage(msg);
        }
    }

    private class MyAlertDialog {
        public int _sessionId;
        public AlertDialog _dialog;

        public MyAlertDialog(int sessionId, AlertDialog dialog) {
            _sessionId = sessionId;
            _dialog = dialog;
        }
    }
}

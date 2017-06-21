/**
 * version 1.
 */

package com.spreadtrum.ims.vowifi;

import android.content.Context;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.RemoteException;
import android.os.SystemProperties;
import android.telecom.VideoProfile.CameraCapabilities;
import android.telephony.PhoneNumberUtils;
import android.text.TextUtils;
import android.util.Log;
import android.view.Surface;
import android.widget.Toast;

import com.android.ims.ImsCallProfile;
import com.android.ims.ImsConferenceState;
import com.android.ims.ImsReasonInfo;
import com.android.ims.ImsStreamMediaProfile;
import com.android.ims.internal.IImsCallSession;
import com.android.ims.internal.IImsCallSessionListener;
import com.android.ims.internal.IImsVideoCallProvider;
import com.android.ims.internal.ImsCallSession.State;
import com.android.ims.ImsSrvccCallInfo;
import com.android.internal.telephony.TelephonyProperties;
import com.spreadtrum.ims.R;
import com.spreadtrum.ims.vowifi.Utilities.Camera;
import com.spreadtrum.ims.vowifi.Utilities.PendingAction;
import com.spreadtrum.ims.vowifi.Utilities.Result;
import com.spreadtrum.ims.vowifi.Utilities.SRVCCSyncInfo;
import com.spreadtrum.ims.vowifi.Utilities.VideoQuality;
import com.spreadtrum.ims.vowifi.VoWifiCallManager.ICallChangedListener;
import com.spreadtrum.vowifi.service.IVoWifiSerService;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Map.Entry;

public class ImsCallSessionImpl extends IImsCallSession.Stub implements LocationListener {
    private static final String TAG = Utilities.getTag(ImsCallSessionImpl.class.getSimpleName());

    // Temp, used to test emergency call.
    private static final String PROP_KEY_FORCE_SOS_CALL = "persist.vowifi.force.soscall";

    private static final int MERGE_TIMEOUT = 15 * 1000;

    private static final boolean SUPPORT_START_CONFERENCE = false;

    private static final String PARTICIPANTS_SEP = ";";

    private int mCallId = -1;
    private boolean mIsAlive = false;
    private boolean mIsConfHost = false;
    private boolean mAudioStart = false;
    private boolean mIsEmergency = false;
    private boolean mInSRVCC = false;

    private Context mContext;
    private Location mSosLocation;
    private VoWifiCallStateTracker mCallStateTracker;
    private LocationManager mLocationManager;
    private VoWifiCallManager mCallManager;
    private IVoWifiSerService mICall = null;

    private IImsCallSessionListener mListener = null;
    private ImsStreamMediaProfile mImsStreamMediaProfile = null;
    private ImsVideoCallProviderImpl mVideoCallProvider = null;

    private ImsCallSessionImpl mConfCallSession = null;
    private ImsCallSessionImpl mHostCallSession = null;
    private ImsCallSessionImpl mInInviteSession = null;
    private ArrayList<String> mInKickParticipants = new ArrayList<String>();
    private ArrayList<String> mParticipants = new ArrayList<String>();
    private HashMap<String, ImsCallSessionImpl> mParticipantSessions =
            new HashMap<String, ImsCallSessionImpl>();
    private HashMap<String, Bundle> mConfParticipantStates = new HashMap<String, Bundle>();
    private LinkedList<ImsCallSessionImpl> mWaitForInviteSessions =
            new LinkedList<ImsCallSessionImpl>();

    private ImsCallProfile mCallProfile;
    private ImsCallProfile mLocalCallProfile = new ImsCallProfile(
            ImsCallProfile.SERVICE_TYPE_NORMAL, ImsCallProfile.CALL_TYPE_VIDEO_N_VOICE);
    private ImsCallProfile mRemoteCallProfile = new ImsCallProfile(
            ImsCallProfile.SERVICE_TYPE_NORMAL, ImsCallProfile.CALL_TYPE_VIDEO_N_VOICE);

    private HashMap<String, PendingAction> mPendingActions = new HashMap<String, PendingAction>();

    private static final int MSG_SET_MUTE       = 1;
    private static final int MSG_START          = 2;
    private static final int MSG_START_CONF     = 3;
    private static final int MSG_ACCEPT         = 4;
    private static final int MSG_REJECT         = 5;
    private static final int MSG_TERMINATE      = 6;
    private static final int MSG_HOLD           = 7;
    private static final int MSG_RESUME         = 8;
    private static final int MSG_MERGE          = 9;
    private static final int MSG_UPDATE         = 10;
    private static final int MSG_EXTEND_TO_CONF = 11;
    private static final int MSG_START_FAIL     = 12;
    private static final int MSG_MERGE_FAILED   = 13;
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            if (msg.what == MSG_START_FAIL) {
                String failMessage = (String) msg.obj;
                Log.e(TAG, failMessage);
                if (mListener != null) {
                    try {
                        mListener.callSessionStartFailed(ImsCallSessionImpl.this,
                                new ImsReasonInfo(
                                        ImsReasonInfo.CODE_SIP_REQUEST_CANCELLED,
                                        ImsReasonInfo.CODE_UNSPECIFIED,
                                        failMessage));
                    } catch (RemoteException e) {
                        Log.e(TAG, "Failed to give the call session start failed callback.");
                        Log.e(TAG, "Catch the RemoteException e: " + e);
                    }
                }
                return;
            }

            String key = (String) msg.obj;
            PendingAction action = mPendingActions.get(key);

            if (action == null) {
                Log.w(TAG, "Try to handle the pending action, but the action is null.");
                // The action is null, remove it from the HashMap.
                synchronized (mPendingActions) {
                    mPendingActions.remove(key);
                }

                // If the action is null, do nothing.
                return;
            }

            if (Utilities.DEBUG) Log.i(TAG, "Handle the pending action: " + action._name);
            try {
                switch (msg.what) {
                    case MSG_SET_MUTE:
                        setMute((Boolean) action._params.get(0));
                        break;
                    case MSG_START:
                        start((String) action._params.get(0),
                                (ImsCallProfile) action._params.get(1));
                        break;
                    case MSG_START_CONF:
                        startConference((String[]) action._params.get(0),
                                (ImsCallProfile) action._params.get(1));
                        break;
                    case MSG_ACCEPT:
                        accept((Integer) action._params.get(0),
                                (ImsStreamMediaProfile) action._params.get(1));
                        break;
                    case MSG_REJECT:
                        reject((Integer) action._params.get(0));
                        break;
                    case MSG_TERMINATE:
                        terminate((Integer) action._params.get(0));
                        break;
                    case MSG_HOLD:
                        hold((ImsStreamMediaProfile) action._params.get(0));
                        break;
                    case MSG_RESUME:
                        resume((ImsStreamMediaProfile) action._params.get(0));
                        break;
                    case MSG_MERGE:
                        merge();
                        break;
                    case MSG_UPDATE:
                        update((Integer) action._params.get(0),
                                (ImsStreamMediaProfile) action._params.get(1));
                        break;
                    case MSG_EXTEND_TO_CONF:
                        String participants = (String) action._params.get(0);
                        extendToConference(participants.split(PARTICIPANTS_SEP));
                        break;
                    case MSG_MERGE_FAILED:
                        // Give a toast for connect timeout.
                        String text = mContext.getString(R.string.vowifi_conf_connect_timeout);
                        Toast.makeText(mContext, text, Toast.LENGTH_LONG).show();
                        // Handle as merge action failed.
                        handleMergeActionFailed(text);
                        // If the call is held, resume this call.
                        if (!mIsAlive) {
                            resume(getResumeMediaProfile());
                        }
                        break;
                }
            } catch (RemoteException e) {
                Log.e(TAG, "Catch the RemoteException when handle the action " + action._name);
            } finally {
                // TODO: If we catch the remote exception, need remove it from the HashMap?
                synchronized (mPendingActions) {
                    mPendingActions.remove(key);
                }
            }
        }
    };

    // To listener the IVoWifiSerService changed.
    private ICallChangedListener mICallChangedListener = new ICallChangedListener() {
        @Override
        public void onChanged(IVoWifiSerService newCallInterface) {
            if (newCallInterface != null) {
                mICall = newCallInterface;
                // If the pending action is not null, we need to handle the them.
                synchronized (mPendingActions) {
                    if (mPendingActions.size() < 1) {
                        if (Utilities.DEBUG) Log.d(TAG, "The pending action is null.");
                        return;
                    }

                    Iterator<Entry<String, PendingAction>> iterator = mPendingActions.entrySet()
                            .iterator();
                    while (iterator.hasNext()) {
                        Entry<String, PendingAction> entry = iterator.next();
                        Message msg = new Message();
                        msg.what = entry.getValue()._action;
                        msg.obj = entry.getKey();
                        mHandler.sendMessage(msg);
                    }
                }
            } else if (mICall != null) {
                Log.w(TAG, "The call interface changed to null, terminate the call.");
                mICall = newCallInterface;
                // It means the call interface disconnect. If the current call do not close,
                // we'd like to terminate this call.
                terminateCall(ImsReasonInfo.CODE_USER_TERMINATED);
            }
        }
    };

    protected ImsCallSessionImpl(Context context, VoWifiCallManager callManager,
            ImsCallProfile profile, IImsCallSessionListener listener,
            ImsVideoCallProviderImpl videoCallProvider, int callDir) {
        mContext = context;
        mCallStateTracker = new VoWifiCallStateTracker(State.IDLE, callDir);
        mCallManager = callManager;
        mCallProfile = profile;
        mListener = listener;
        mVideoCallProvider = videoCallProvider;
        if (mVideoCallProvider == null) {
            // The video call provider is null, create it.
            mVideoCallProvider = new ImsVideoCallProviderImpl(mContext, callManager, this);
        }

        // Register the service changed to get the IVowifiService.
        mCallManager.registerCallInterfaceChanged(mICallChangedListener);
    }

    @Override
    protected void finalize() throws Throwable {
        mCallManager.unregisterCallInterfaceChanged(mICallChangedListener);
        super.finalize();
    }

    @Override
    public String toString() {
        StringBuilder builder = new StringBuilder()
                .append("[callId = " + mCallId)
                .append(", state = " + mCallStateTracker)
                .append(", isAlive = " + mIsAlive + "]");
        return builder.toString();
    }

    @Override
    public void onLocationChanged(Location location) {
        if (Utilities.DEBUG) Log.i(TAG, "onLocationChanged");
        mSosLocation = location;
    }

    @Override
    public void onStatusChanged(String provider, int status, Bundle extras) {
        // Do nothing.
    }

    @Override
    public void onProviderEnabled(String provider) {
        // Do nothing.
    }

    @Override
    public void onProviderDisabled(String provider) {
        // Do nothing.
    }

    /**
     * Closes the object. This object is not usable after being closed.
     */
    @Override
    public void close() {
        if (Utilities.DEBUG) Log.i(TAG, "The call session(" + this + ") will be closed.");

        if (mInSRVCC) {
            Log.d(TAG, "In SRVCC process, this call session will be closed after SRVCC success.");
            return;
        }

        mCallProfile = null;
        mListener = null;
        mImsStreamMediaProfile = null;
        mVideoCallProvider = null;
        mCallStateTracker = null;

        if (mLocationManager != null) {
            mLocationManager.removeUpdates(this);
            mSosLocation = null;
            mLocationManager = null;
        }
    }

    /**
     * Gets the call ID of the session.
     */
    @Override
    public String getCallId() {
        if (Utilities.DEBUG) Log.i(TAG, "Get the call id: " + mCallId);

        return String.valueOf(mCallId);
    }

    /**
     * Gets the call profile that this session is associated with
     */
    @Override
    public ImsCallProfile getCallProfile() {
        if (Utilities.DEBUG) Log.i(TAG, "Get the call profile: " + mCallProfile);

        return mCallProfile;
    }

    /**
     * Gets the local call profile that this session is associated with
     */
    @Override
    public ImsCallProfile getLocalCallProfile() {
        if (Utilities.DEBUG) Log.i(TAG, "Get the local call profile: " + mLocalCallProfile);

        return mLocalCallProfile;
    }

    /**
     * Gets the remote call profile that this session is associated with
     */
    @Override
    public ImsCallProfile getRemoteCallProfile() {
        if (Utilities.DEBUG) Log.i(TAG, "Get the remote call profile: " + mRemoteCallProfile);

        return mRemoteCallProfile;
    }

    /**
     * Gets the value associated with the specified property of this session.
     */
    @Override
    public String getProperty(String name) {
        if (Utilities.DEBUG) Log.i(TAG, "Get the property by this name: " + name);

        return mCallProfile.getCallExtra(name, null);
    }

    /**
     * Gets the session state. The value returned must be one of the states in
     * {@link ImsCallSession#State}
     */
    @Override
    public int getState() {
        return mCallStateTracker != null ? mCallStateTracker.getCallState() : State.INVALID;
    }

    /**
     * Checks if the session is in a call.
     */
    @Override
    public boolean isInCall() {
        if (mICall == null) {
            // The ser service is null, so this call shouldn't be in call.
            return false;
        }

        int state = getState();
        return state > State.INITIATED && state < State.TERMINATED;
    }

    /**
     * Sets the listener to listen to the session events. A {@link IImsCallSession}
     * can only hold one listener at a time. Subsequent calls to this method
     * override the previous listener.
     *
     * @param listener to listen to the session events of this object
     */
    @Override
    public void setListener(IImsCallSessionListener listener) {
        if (Utilities.DEBUG) Log.i(TAG, "Set the listener: " + listener);

        mListener = listener;
    }

    /**
     * Mutes or unmutes the mic for the active call.
     *
     * @param muted true if the call is muted, false otherwise
     * @throws RemoteException
     */
    @Override
    public void setMute(boolean muted) throws RemoteException {
        if (Utilities.DEBUG) Log.i(TAG, "Mutes(" + muted + ") the mic for the active call.");

        if (mICall == null) {
            // As the vowifi service is null, need add this action to pending action.
            synchronized (mPendingActions) {
                String key = String.valueOf(System.currentTimeMillis());
                PendingAction action = new PendingAction("setMute", MSG_SET_MUTE, (Boolean) muted);
                mPendingActions.put(key, action);
            }
            return;
        }

        // The vowifi service is not null, mute or unmute the active call.
        int res = Result.FAIL;
        if (isConferenceCall()) {
            res = mICall.confSetMute(mCallId, muted);
        } else {
            res = mICall.sessSetMicMute(mCallId, muted);
        }

        if (res != Result.SUCCESS) {
            // Set mute action failed.
            Log.e(TAG, "Native set mute failed, res = " + res);
        }
    }

    /**
     * Initiates an IMS call with the specified target and call profile.
     * The session listener is called back upon defined session events.
     * The method is only valid to call when the session state is in
     *
     * @param callee  dialed string to make the call to
     * @param profile call profile to make the call with the specified service type,
     *                call type and media information
     * @throws RemoteException
     */
    @Override
    public void start(String callee, ImsCallProfile profile) throws RemoteException {
        if (Utilities.DEBUG) Log.i(TAG, "Initiates an ims call with " + callee);

        if (!mCallManager.isCallFunEnabled() || TextUtils.isEmpty(callee) || profile == null) {
            handleStartActionFailed("Start the call failed. Check the callee or profile.");
            Toast.makeText(mContext, R.string.vowifi_call_retry, Toast.LENGTH_LONG).show();
            return;
        }

        // Check if emergency call.
        boolean isPhoneInEcmMode =
                SystemProperties.getBoolean(TelephonyProperties.PROPERTY_INECM_MODE, false);
        boolean isEmergencyNumber = PhoneNumberUtils.isEmergencyNumber(callee);
        boolean forceSosCall = SystemProperties.getBoolean(PROP_KEY_FORCE_SOS_CALL, false);
        Log.i(TAG, "Emergency isPhoneInEcmMode = " + isPhoneInEcmMode +
                " isEmergencyNumber = " + isEmergencyNumber);

        // todo: need location for some special Operator
        boolean isNeedGeoLocation = false;
        if (isEmergencyNumber || forceSosCall) {
            isNeedGeoLocation = true;
        }
        if (isNeedGeoLocation) {
            mLocationManager = (LocationManager) mContext.getSystemService(Context.LOCATION_SERVICE);
            List<String> providers = mLocationManager.getProviders(true);
            String locationProvider = null;
            if (providers.contains(LocationManager.GPS_PROVIDER)) {
                locationProvider = LocationManager.GPS_PROVIDER;
            } else if (providers.contains(LocationManager.NETWORK_PROVIDER)) {
                locationProvider = LocationManager.NETWORK_PROVIDER;
            } else {
                Log.w(TAG, "Failed to get the location provider.!");
            }
            if (Utilities.DEBUG) {
                Location netLocation = mLocationManager.getLastKnownLocation(LocationManager.NETWORK_PROVIDER);
                if (netLocation != null) Log.w(TAG, "NETWORK_PROVIDER:get Latitude = " + netLocation.getLatitude() + "get Longitude=" + netLocation.getLongitude());
                Location passiveLocation = mLocationManager.getLastKnownLocation(LocationManager.PASSIVE_PROVIDER);
                if (passiveLocation != null) Log.w(TAG, "PASSIVE_PROVIDER:get Latitude = " + passiveLocation.getLatitude() + "get Longitude=" + passiveLocation.getLongitude());
                Location gpsLocation = mLocationManager.getLastKnownLocation(LocationManager.GPS_PROVIDER);
                if (gpsLocation != null) Log.w(TAG, "GPS LOCATION:get Latitude = " + gpsLocation.getLatitude() + "get Longitude=" + gpsLocation.getLongitude()); 
            }
            if (!TextUtils.isEmpty(locationProvider)) {
                mSosLocation = mLocationManager.getLastKnownLocation(locationProvider);
                mLocationManager.requestLocationUpdates(locationProvider, 1000, 2.0f, this);
            } else {
                mLocationManager = null;
                mSosLocation = null;
                Log.w(TAG, "Failed to get provider the location info!");
            }
        }

        if (isEmergencyNumber || forceSosCall) {
            startEmergencyCall(callee, profile);
        } else {
            startNormalCall(callee, profile);
        }
    }

    /**
     * Initiates an IMS call with the specified participants and call profile.
     * The session listener is called back upon defined session events.
     * The method is only valid to call when the session state is in
     *
     * @param participants participant list to initiate an IMS conference call
     * @param profile      call profile to make the call with the specified service type,
     *                     call type and media information
     */
    @Override
    public void startConference(String[] participants, ImsCallProfile profile)
            throws RemoteException {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Initiates an ims conference call with participants: "
                    + Utilities.getString(participants));
        }

        // As do not support now. Handle it as action failed.
        if (!SUPPORT_START_CONFERENCE) {
            handleStartActionFailed("Do not support this action now.");
            Toast.makeText(mContext, R.string.vowifi_conf_do_not_support, Toast.LENGTH_LONG).show();
            return;
        }

        if (participants == null) {
            handleStartActionFailed("Start the conference failed, the participants is null.");
            return;
        }

        if (participants.length < 1 || profile == null) {
            handleStartActionFailed("Start the conference failed. Check the parts or profile.");
            return;
        }

        // As the vowifi service is null, need add this action to pending action.
        if (mICall == null) {
            synchronized (mPendingActions) {
                String key = String.valueOf(System.currentTimeMillis());
                PendingAction action = new PendingAction("startConference", MSG_START_CONF,
                        participants, profile);
                mPendingActions.put(key, action);
            }
            return;
        }

        // TODO: update the profile
        mCallProfile = profile;
        // TODO: As this is conference, the oi set as null?
        mCallProfile.setCallExtra(ImsCallProfile.EXTRA_OI, participants[0]);
        mCallProfile.setCallExtra(ImsCallProfile.EXTRA_CNA, null);
        // TODO: why not {@link ImsCallProfile#OIR_DEFAULT}
        mCallProfile.setCallExtraInt(ImsCallProfile.EXTRA_CNAP,
                ImsCallProfile.OIR_PRESENTATION_NOT_RESTRICTED);
        mCallProfile.setCallExtraInt(ImsCallProfile.EXTRA_OIR,
                ImsCallProfile.OIR_PRESENTATION_NOT_RESTRICTED);

        StringBuilder phoneNumbers = new StringBuilder();
        for (int i = 0; i < participants.length; i++) {
            if (i > 0) phoneNumbers.append(PARTICIPANTS_SEP);
            phoneNumbers.append(participants[i]);
            mParticipants.add(participants[i]);
        }
        Log.d(TAG, "Start the conference with phone numbers: " + phoneNumbers);

        // TODO: if need return the error code
        // FIXME: Couldn't understand, if the the call id = 1, what does it mean?
        int res = mICall.confCall(participants, null /* cookie, do not use now */, false);
        if (Utilities.DEBUG) Log.d(TAG, "Start the conference call, and get the call id: " + res);
        if (res == Result.INVALID_ID) {
            handleStartActionFailed("Native start the conference call failed.");
        } else {
            mCallId = res;
            updateState(State.INITIATED);
        }
    }

    /**
     * Accepts an incoming call or session update.
     *
     * @param callType call type specified in {@link ImsCallProfile} to be answered
     * @param profile  stream media profile {@link ImsStreamMediaProfile} to be answered
     * @throws RemoteException
     */
    @Override
    public void accept(int callType, ImsStreamMediaProfile profile) throws RemoteException {
        if (Utilities.DEBUG) Log.i(TAG, "Accept an incoming call with call type is " + callType);

        if (profile == null) {
            Log.e(TAG, "Try to accept an incoming call, but the media profile is null.");
            handleStartActionFailed("Can not accept the call as the media profile is null.");
            return;
        }

        // As the vowifi service is null, need add this action to pending action.
        if (mICall == null) {
            synchronized (mPendingActions) {
                String key = String.valueOf(System.currentTimeMillis());
                PendingAction action =
                        new PendingAction("accept", MSG_ACCEPT, (Integer) callType, profile);
                mPendingActions.put(key, action);
            }
            return;
        }

        int res = Result.FAIL;
        if (isConferenceCall()) {
            res = mICall.confAcceptInvite(mCallId);
        } else {
            boolean isVideoCall = Utilities.isVideoCall(callType);
            res = mICall.sessAnswer(mCallId, null, true, isVideoCall);
        }

        if (res == Result.SUCCESS) {
            mIsAlive = true;
            startAudio();

            // Accept action success, update the last call action to accept.
            updateRequestAction(VoWifiCallStateTracker.ACTION_ACCEPT);
        } else {
            // Accept action failed.
            Log.e(TAG, "Native accept the incoming call failed, res = " + res);
            handleStartActionFailed("Native accept the incoming call failed.");
        }
    }

    private void handleStartActionFailed(String failMessage) {
        // As start action failed, remove the call first.
        mCallManager.removeCall(this);

        // When #ImsCall received the call session start failed callback will set the call session
        // to null. Then sometimes it will meet the NullPointerException. So we'd like to delay
        // 500ms to send this callback to let the ImsCall handle the left logic.
        Message msg = mHandler.obtainMessage(MSG_START_FAIL, failMessage);
        mHandler.sendMessageDelayed(msg, 500);
    }

    /**
     * Rejects an incoming call or session update.
     *
     * @param reason reason code to reject an incoming call
     */
    @Override
    public void reject(int reason) throws RemoteException {
        if (Utilities.DEBUG) Log.i(TAG, "Reject an incoming call as the reason is " + reason);

        // As the vowifi service is null, need add this action to pending action.
        if (mICall == null) {
            synchronized (mPendingActions) {
                String key = String.valueOf(System.currentTimeMillis());
                PendingAction action = new PendingAction("reject", MSG_REJECT, (Integer) reason);
                mPendingActions.put(key, action);
            }
            return;
        }

        int res = Result.FAIL;
        if (isConferenceCall()) {
            res = mICall.confTerm(mCallId, reason);
        } else {
            res = mICall.sessTerm(mCallId, reason);
        }

        if (res == Result.SUCCESS) {
            // Reject action success, update the last call action as reject.
            updateRequestAction(VoWifiCallStateTracker.ACTION_REJECT);

            // As the result is OK, terminate the call session.
            if (mListener != null) {
                mListener.callSessionTerminated(this,
                        new ImsReasonInfo(reason, reason, "reason: " + reason));
            }
            mCallManager.removeCall(this);
        } else {
            // Reject the call failed.
            Log.e(TAG, "Native reject the incoming call failed, res = " + res);
        }
    }

    /**
     * Terminates a call.
     */
    @Override
    public void terminate(int reason) throws RemoteException {
        if (Utilities.DEBUG) Log.i(TAG, "Terminate a call as the reason is " + reason);

        // As the vowifi service is null, need add this action to pending action.
        if (mICall == null) {
            synchronized (mPendingActions) {
                String key = String.valueOf(System.currentTimeMillis());
                PendingAction action =
                        new PendingAction("terminate", MSG_TERMINATE, (Integer) reason);
                mPendingActions.put(key, action);
            }
            return;
        }

        // Terminate the call
        terminateCall(reason);

        // As user terminate the call, we need to check if there is conference call.
        ImsCallSessionImpl confSession = mCallManager.getConfCallSession();
        if (confSession != null) {
            ImsCallSessionImpl hostSession = confSession.getHostCallSession();
            if (this.equals(hostSession)) {
                // Terminate the conference call and the close it.
                confSession.terminate(reason);
                confSession.close();
            }
        }
    }

    /**
     * Puts a call on hold. When it succeeds,
     *
     * @param profile stream media profile {@link ImsStreamMediaProfile} to hold the call
     */
    @Override
    public void hold(ImsStreamMediaProfile profile) throws RemoteException {
        if (Utilities.DEBUG) Log.i(TAG, "Hold a call with the media profile: " + profile);

        if (profile == null) {
            handleHoldActionFailed("Hold the call failed, the media profile is null.");
            return;
        }

        // As the vowifi service is null, need add this action to pending action.
        if (mICall == null) {
            synchronized (mPendingActions) {
                String key = String.valueOf(System.currentTimeMillis());
                PendingAction action = new PendingAction("hold", MSG_HOLD, profile);
                mPendingActions.put(key, action);
            }
            return;
        }

        int res = Result.FAIL;
        if (isConferenceCall()) {
            res = mICall.confHold(mCallId);
        } else {
            res = mICall.sessHold(mCallId);
        }

        if (res == Result.SUCCESS) {
            // Hold success will be handled in the callback.
            mImsStreamMediaProfile = profile;

            // Hold action success, update the last call action as hold.
            updateRequestAction(VoWifiCallStateTracker.ACTION_HOLD);
        } else {
            // Hold action failed.
            handleHoldActionFailed("Native hold the call failed, res = " + res);
        }
    }

    private void handleHoldActionFailed(String failMessage) throws RemoteException {
        Log.e(TAG, failMessage);
        if (mListener != null) {
            mListener.callSessionHoldFailed(this, new ImsReasonInfo(ImsReasonInfo.CODE_UNSPECIFIED,
                    ImsReasonInfo.CODE_UNSPECIFIED, failMessage));
        }
    }

    /**
     * Continues a call that's on hold. When it succeeds,
     * is called.
     *
     * @param profile stream media profile {@link ImsStreamMediaProfile} to resume the call
     */
    @Override
    public void resume(ImsStreamMediaProfile profile) throws RemoteException {
        if (Utilities.DEBUG) Log.i(TAG, "Continues a call with the media profile: " + profile);

        if (profile == null) {
            handleResumeActionFailed("Resume the call failed, the media profile is null.");
            return;
        }

        // As the vowifi service is null, need add this action to pending action.
        if (mICall == null) {
            synchronized (mPendingActions) {
                String key = String.valueOf(System.currentTimeMillis());
                PendingAction action = new PendingAction("resume", MSG_RESUME, profile);
                mPendingActions.put(key, action);
            }
            return;
        }

        int res = Result.FAIL;
        if (isConferenceCall()) {
            res = mICall.confResume(mCallId);
        } else {
            res = mICall.sessResume(mCallId);
        }

        if (res == Result.SUCCESS) {
            // Resume result will be handled in the callback.
            mImsStreamMediaProfile = profile;

            // Resume action success, update the last call action as resume.
            updateRequestAction(VoWifiCallStateTracker.ACTION_RESUME);
        } else {
            // Resume the call failed.
            handleResumeActionFailed("Native resume the call failed, res = " + res);
        }
    }

    private void handleResumeActionFailed(String failMessage) throws RemoteException {
        Log.e(TAG, failMessage);
        if (mListener != null) {
            mListener.callSessionResumeFailed(this, new ImsReasonInfo(
                    ImsReasonInfo.CODE_UNSPECIFIED, ImsReasonInfo.CODE_UNSPECIFIED, failMessage));
        }
    }

    /**
     * Merges the active & hold call. When it succeeds,
     * {@link Listener#callSessionMergeStarted} is called.
     */
    @Override
    public void merge() throws RemoteException {
        if (Utilities.DEBUG) Log.i(TAG, "Merge the active & hold call.");

        // As the vowifi service is null, need add this action to pending action.
        if (mICall == null) {
            synchronized (mPendingActions) {
                String key = String.valueOf(System.currentTimeMillis());
                PendingAction action = new PendingAction("merge", MSG_MERGE);
                mPendingActions.put(key, action);
            }
            return;
        }

        int sessionSize = mCallManager.getCallCount();
        if (sessionSize < 2) {
            handleMergeActionFailed(
                    "The number of active & hold call is " + sessionSize + ". Can not merge!");
            return;
        }

        // To check if there is conference, if yes, we needn't init a new call session.
        ImsCallSessionImpl confSession = mCallManager.getConfCallSession();
        if (confSession != null) {
            // If there is conference call, we need invite this call to the conference.
            inviteToConference(confSession);
        } else {
            // If there isn't conference call, we need create a new call session for it.
            createConfCall();
        }
    }

    private void handleMergeActionFailed(String failMessage) throws RemoteException {
        Log.e(TAG, failMessage);
        if (mListener != null) {
            mListener.callSessionMergeFailed(this, new ImsReasonInfo(ImsReasonInfo.CODE_UNSPECIFIED,
                    ImsReasonInfo.CODE_UNSPECIFIED, failMessage));

            // FIXME: As the call may be held or resumed before merge which can not tracked by
            //        ImsCallTracker, so before we give the merge failed callback, we'd like to
            //        give this callback refer to current state.
            //        Another, if this issue should be fixed by framework?
            if (mIsAlive) {
                mListener.callSessionResumed(this, mCallProfile);
            } else {
                mListener.callSessionHeld(this, mCallProfile);
            }
        }
    }

    /**
     * Updates the current call's properties (ex. call mode change: video upgrade / downgrade).
     *
     * @param callType call type specified in {@link ImsCallProfile} to be updated
     * @param profile  stream media profile {@link ImsStreamMediaProfile} to be updated
     */
    @Override
    public void update(int callType, ImsStreamMediaProfile profile) throws RemoteException {
        if (Utilities.DEBUG) Log.i(TAG, "Update the current call's type to " + callType + ".");

        // TODO: If the media profile will be update?
        if (callType == mCallProfile.mCallType) {
            handleUpdateActionFailed("This session's old call type is same as the new.");
            return;
        }

        // As the vowifi service is null, need add this action to pending action.
        if (mICall == null) {
            synchronized (mPendingActions) {
                String key = String.valueOf(System.currentTimeMillis());
                PendingAction action = new PendingAction("update", MSG_UPDATE, callType, profile);
                mPendingActions.put(key, action);
            }
            return;
        }

        int res = mICall.sessUpdate(mCallId, true, Utilities.isVideoCall(callType));
        if (res == Result.FAIL) {
            handleUpdateActionFailed("Native update result is " + res);
        } else {
            // TODO: change to update the media profile.
            mImsStreamMediaProfile = profile;
        }
    }

    private void handleUpdateActionFailed(String failMessage) throws RemoteException {
        Log.e(TAG, failMessage);
        if (mListener != null) {
            mListener.callSessionUpdateFailed(this, new ImsReasonInfo(
                    ImsReasonInfo.CODE_UNSPECIFIED, ImsReasonInfo.CODE_UNSPECIFIED, failMessage));
        }
    }

    /**
     * Extends this call to the conference call with the specified recipients.
     *
     * @param participants participant list to be invited to the conference call after extending
     *        the call
     */
    @Override
    public void extendToConference(String[] participants) throws RemoteException {
        Log.w(TAG, "Extends this call to conference call: " + participants + ", do not support.");
        // Do not support now.
    }

    /**
     * Requests the conference server to invite an additional participants to the conference.
     *
     * @param participants participant list to be invited to the conference call
     */
    @Override
    public void inviteParticipants(String[] participants) throws RemoteException {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Invite participants: " + Utilities.getString(participants));
        }

        // TODO: if need check this participants contains this session's callee.
        if (participants == null || participants.length < 1) {
            handleInviteParticipantsFailed("The participant to invite is null or empty.");
            return;
        }

        // TODO: Need change.
//        StringBuilder ids = new StringBuilder();
//        StringBuilder callees = new StringBuilder();
//        if (mICall != null) {
//            for (int i = 0; i < participants.length; i++) {
//                String id = mCallManager.getCallSessionId(participants[i]);
//                if (id == null) {
//                    if (callees.length() > 0) {
//                        callees.append(",");
//                    }
//                    callees.append(participants[i]);
//                } else {
//                    if (ids.length() > 0) {
//                        ids.append(",");
//                    }
//                    ids.append(id);
//                }
//            }
//            int ret = mICall.confAddMembers(mCallId, callees.toString(), ids.toString());
//            if (ret == Result.INVALID_ID) {
//                handleInviteParticipantsFailed("Failed");
//            }
//        }
    }

    private void handleInviteParticipantsFailed(String failMessage) throws RemoteException {
        Log.e(TAG, failMessage);
        if (mListener != null) {
            mListener.callSessionInviteParticipantsRequestFailed(this, new ImsReasonInfo(
                    ImsReasonInfo.CODE_UNSPECIFIED, ImsReasonInfo.CODE_UNSPECIFIED, failMessage));
        }
    }

    /**
     * Requests the conference server to remove the specified participants from the conference.
     *
     * @param participants participant list to be removed from the conference call
     */
    @Override
    public void removeParticipants(String[] participants) throws RemoteException {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Remove the participants: " + Utilities.getString(participants));
        }

        if (participants == null || participants.length < 1) {
            handleRemoveParticipantsFailed("The participants need to removed is null or empty.");
            return;
        }

        synchronized (mInKickParticipants) {
            String[] needRemoveParticipants = findNeedRemove(participants);
            if (needRemoveParticipants == null || needRemoveParticipants.length < 1) {
                Log.d(TAG, "There isn't any participant need remove this time: "
                        + Utilities.getString(participants));
                return;
            }

            if (mICall == null) {
                handleRemoveParticipantsFailed("The call interface is null.");
                return;
            }

            addAsInKickProcess(needRemoveParticipants);
            int ret = mICall.confKickMembers(mCallId, needRemoveParticipants);
            if (ret == Result.FAIL) {
                removeFromInKick(needRemoveParticipants);
                handleRemoveParticipantsFailed("Native failed to remove the participants.");
            }
        }
    }

    private void handleRemoveParticipantsFailed(String failMessage) throws RemoteException {
        Log.e(TAG, failMessage);
        // Show the failed toast.
        Toast.makeText(mContext, R.string.vowifi_conf_kick_failed, Toast.LENGTH_LONG).show();
        if (mListener != null) {
            mListener.callSessionRemoveParticipantsRequestFailed(this, new ImsReasonInfo(
                    ImsReasonInfo.CODE_UNSPECIFIED, ImsReasonInfo.CODE_UNSPECIFIED, failMessage));
        }
    }

    /**
     * Sends a DTMF code. According to <a href="http://tools.ietf.org/html/rfc2833">RFC 2833</a>,
     * event 0 ~ 9 maps to decimal value 0 ~ 9, '*' to 10, '#' to 11, event 'A' ~ 'D' to 12 ~ 15,
     * and event flash to 16. Currently, event flash is not supported.
     *
     * @param c      the DTMF to send. '0' ~ '9', 'A' ~ 'D', '*', '#' are valid inputs.
     * @param result
     */
    @Override
    public void sendDtmf(char c, Message result) throws RemoteException {
        // TODO: need change
        Log.i(TAG, "sendDtmf" + c);
        if (mICall != null) {
            mICall.sessDtmf(mCallId, getDtmfType(c));
        }
    }

    /**
     * Start a DTMF code. According to <a href="http://tools.ietf.org/html/rfc2833">RFC 2833</a>,
     * event 0 ~ 9 maps to decimal value 0 ~ 9, '*' to 10, '#' to 11, event 'A' ~ 'D' to 12 ~ 15,
     * and event flash to 16. Currently, event flash is not supported.
     *
     * @param c the DTMF to send. '0' ~ '9', 'A' ~ 'D', '*', '#' are valid inputs.
     */
    @Override
    public void startDtmf(char c) throws RemoteException {
        // TODO: need change
        Log.i(TAG, "startDtmf" + c);
        if (mICall != null) {
            mICall.sessDtmf(mCallId, getDtmfType(c));
        }
    }

    /**
     * Stop a DTMF code.
     */
    @Override
    public void stopDtmf() {
        Log.i(TAG, "stopDtmf");
        // TODO:
    }

    /**
     * Sends an USSD message.
     *
     * @param ussdMessage USSD message to send
     */
    @Override
    public void sendUssd(String ussdMessage) throws RemoteException {
        if (Utilities.DEBUG) Log.i(TAG, "Send an USSD message: " + ussdMessage);
        // TODO: need change
        if (mICall != null) {
            mICall.sendUSSDMessage(mCallId, ussdMessage);
        }

    }

    /**
     * Returns a binder for the video call provider implementation contained within the IMS service
     * process. This binder is used by the VideoCallProvider subclass in Telephony which
     * intermediates between the propriety implementation and Telecomm/InCall.
     */
    @Override
    public IImsVideoCallProvider getVideoCallProvider() {
        if (Utilities.DEBUG) Log.i(TAG, "Get the video call provider: " + mVideoCallProvider);
        return mVideoCallProvider == null ? null : mVideoCallProvider.getInterface();
    }

    /**
     * Determines if the current session is multiparty.
     * @return {@code True} if the session is multiparty.
     */
    @Override
    public boolean isMultiparty() {
        return mCallProfile == null ? false
                : mCallProfile.getCallExtraBoolean(ImsCallProfile.EXTRA_CONFERENCE, false);
    }

    public ImsVideoCallProviderImpl getVideoCallProviderImpl() {
        return mVideoCallProvider;
    }

    public ImsStreamMediaProfile getMediaProfile() {
        return mImsStreamMediaProfile;
    }

    public String getCallee() {
        return mParticipants.get(0);
    }

    public String findCallee(String phoneNumber) {
        if (TextUtils.isEmpty(phoneNumber)) return null;

        for (String participant : mParticipants) {
            if (isSameCallee(participant, phoneNumber)) {
                return participant;
            }
        }
        return null;
    }

    public ArrayList<String> getParticipants() {
        return mParticipants;
    }

    public ImsCallProfile updateCallee(String phoneNumber) {
        mParticipants.clear();
        mParticipants.add(phoneNumber);
        mCallProfile.setCallExtra(ImsCallProfile.EXTRA_OI, phoneNumber);

        return mCallProfile;
    }

    public void updateMediaProfile(ImsStreamMediaProfile profile) {
        // TODO: update or replace?
        mImsStreamMediaProfile = profile;
    }

    public IImsCallSessionListener getListener() {
        return mListener;
    }

    public void setCallId(int id) {
        mCallId = id;
    }

    public VoWifiCallStateTracker getCallStateTracker() {
        return mCallStateTracker;
    }

    public void updateState(int state) {
        if (mCallStateTracker != null) mCallStateTracker.updateCallState(state);
    }

    public void updateRequestAction(int requestAction){
        if (mCallStateTracker != null)  mCallStateTracker.updateRequestAction(requestAction);
    }

    public void updateAliveState(boolean alive) {
        mIsAlive = alive;
    }

    public boolean isAlive() {
        return mIsAlive;
    }

    public boolean isHeld() {
        return mImsStreamMediaProfile.mAudioDirection == ImsStreamMediaProfile.DIRECTION_SEND;
    }

    public void updateIsConfHost(boolean isHost) {
        mIsConfHost = isHost;
    }

    public void startAudio() {
        mAudioStart = true;
        mCallManager.startAudioStream();
    }

    public boolean isAudioStart() {
        return mAudioStart;
    }

    public String removeCallee(String phoneNumber) {
        if (TextUtils.isEmpty(phoneNumber)) return null;

        for (int i = 0; i < mParticipants.size(); i++) {
            String participant = mParticipants.get(i);
            if (isSameCallee(participant, phoneNumber)) {
                mParticipants.remove(i);
                return participant;
            }
        }
        return null;
    }

    public void addCallee(String callee) {
        boolean flag = false;
        for (int i = 0; i < mParticipants.size(); i++) {
            if (mParticipants.get(i).equals(callee)) {
                flag = true;
            }
        }
        if (!flag) {
            mParticipants.add(callee);
        }
    }

    public int getParticipantsCount() {
        return mParticipants.size();
    }

    public void updateConfParticipants(String participant, Bundle state) {
        mConfParticipantStates.put(participant, state);
    }

    public void kickActionFinished(String participant) {
        mInKickParticipants.remove(participant);
    }

    public ImsConferenceState getConfParticipantsState() {
        ImsConferenceState state = new ImsConferenceState();
        state.mParticipants = mConfParticipantStates;
        return state;
    }

    public ImsCallSessionImpl getConfCallSession() {
        return mConfCallSession;
    }

    public void setConfCallSession(ImsCallSessionImpl confCallSession) {
        mConfCallSession = confCallSession;
        if (confCallSession != null) {
            // It means the conference connected to server.
            mHandler.removeMessages(MSG_MERGE_FAILED);
        }
    }

    public ImsCallSessionImpl getHostCallSession() {
        return mHostCallSession;
    }

    public void setHostCallSession(ImsCallSessionImpl hostCallSession) {
        mHostCallSession = hostCallSession;
    }

    public void addAsWaitForInvite(ImsCallSessionImpl callSession) {
        if (callSession != null) mWaitForInviteSessions.add(callSession);
    }

    public ImsCallSessionImpl getNeedInviteCall() {
        mInInviteSession = mWaitForInviteSessions.pollFirst();
        return mInInviteSession;
    }

    public void setInInviteCall(ImsCallSessionImpl callSession) {
        mInInviteSession = callSession;
    }

    public ImsCallSessionImpl getInInviteCall() {
        return mInInviteSession;
    }

    public void addParticipant(String phoneNumber, ImsCallSessionImpl callSession) {
        if (TextUtils.isEmpty(phoneNumber) || callSession == null) {
            Log.e(TAG, "Failed to add this call: " + callSession + " as one participant.");
            return;
        }

        mParticipantSessions.put(phoneNumber, callSession);
    }

    public String getParticipantSessionId(String phoneNumber) {
        ImsCallSessionImpl session = mParticipantSessions.get(phoneNumber);
        if (session != null) {
            return session.getCallId();
        } else {
            return String.valueOf(Result.INVALID_ID);
        }
    }

    public void removeParticipant(String phoneNumber) {
        if (TextUtils.isEmpty(phoneNumber)) {
            Log.e(TAG, "Can not remove the participant for the number: " + phoneNumber);
            return;
        }

        mParticipantSessions.remove(phoneNumber);
    }

    public void prepareSRVCCSyncInfo(ArrayList<ImsSrvccCallInfo> infoList, int multipartyOrder) {
        if (Utilities.DEBUG) Log.i(TAG, "Prepare the SRVCC sync info for the call: " + this);

        if (infoList == null) return;

        mInSRVCC = true;
        if (mParticipantSessions.size() > 0) {
            // If this call is conference, we need prepare she SRVCC call info for each child.
            Iterator<Entry<String, ImsCallSessionImpl>> iterator =
                    mParticipantSessions.entrySet().iterator();
            int order = 0;
            while (iterator.hasNext()) {
                Entry<String, ImsCallSessionImpl> entry = iterator.next();
                ImsCallSessionImpl callSession = entry.getValue();
                callSession.prepareSRVCCSyncInfo(infoList, order);
                order = order + 1;
            }
        } else {
            // Prepare this call session's call info.
            ImsSrvccCallInfo info = new ImsSrvccCallInfo();
            info.mCallId = mCallId;
            info.mDir = mCallStateTracker.getSRVCCCallDirection();
            info.mCallState = mCallStateTracker.getSRVCCCallState();
            info.mHoldState = mCallStateTracker.getSRVCCCallHoldState();
            info.mMptyState = isConferenceCall() ? SRVCCSyncInfo.MultipartyState.YES
                    : SRVCCSyncInfo.MultipartyState.NO;
            info.mMptyOrder = multipartyOrder;
            info.mCallType = getSRVCCCallType();
            info.mNumType = SRVCCSyncInfo.PhoneNumberType.NATIONAL;
            info.mNumber = getCallee();
            infoList.add(info);
        }
    }

    public ImsStreamMediaProfile getHoldMediaProfile() {
        ImsStreamMediaProfile mediaProfile = new ImsStreamMediaProfile();

        if (mCallProfile == null) {
            return mediaProfile;
        }

        mediaProfile.mAudioQuality = mImsStreamMediaProfile.mAudioQuality;
        mediaProfile.mVideoQuality = mImsStreamMediaProfile.mVideoQuality;
        mediaProfile.mAudioDirection = ImsStreamMediaProfile.DIRECTION_SEND;

        if (mediaProfile.mVideoQuality != ImsStreamMediaProfile.VIDEO_QUALITY_NONE) {
            mediaProfile.mVideoDirection = ImsStreamMediaProfile.DIRECTION_SEND;
        }

        return mediaProfile;
    }

    public ImsStreamMediaProfile getResumeMediaProfile() {
        ImsStreamMediaProfile mediaProfile = new ImsStreamMediaProfile();

        if (mCallProfile == null) {
            return mediaProfile;
        }

        mediaProfile.mAudioQuality = mImsStreamMediaProfile.mAudioQuality;
        mediaProfile.mVideoQuality = mImsStreamMediaProfile.mVideoQuality;
        mediaProfile.mAudioDirection = ImsStreamMediaProfile.DIRECTION_SEND_RECEIVE;

        if (mediaProfile.mVideoQuality != ImsStreamMediaProfile.VIDEO_QUALITY_NONE) {
            mediaProfile.mVideoDirection = ImsStreamMediaProfile.DIRECTION_SEND_RECEIVE;
        }

        return mediaProfile;
    }

    public int releaseCall() {
        if (Utilities.DEBUG) Log.i(TAG, "Try to release the call: " + mCallId);

        if (mICall == null) {
            Log.e(TAG, "Can not release the call as the call interface is null.");
            return Result.FAIL;
        }

        try {
            // The call will be relaeased if the SRVCC success, update the mInSRVCC state.
            mInSRVCC = false;

            int res = Result.FAIL;
            if (isConferenceCall()) {
                res = mICall.confRelease(mCallId);
            } else {
                res = mICall.sessRelease(mCallId);
            }

            if (res == Result.SUCCESS) {
                boolean isVideoCall = Utilities.isVideoCall(mCallProfile.mCallType);
                if (isVideoCall && mVideoCallProvider != null) {
                    Log.d(TAG, "Need to stop all the video as SRVCC success.");
                    // Stop all the video
                    mVideoCallProvider.stopAll();
                }
            } else {
                Log.e(TAG, "Native failed to release the call.");
            }

            // Even native failed, we'd like to remove this call from the list.
            mCallManager.removeCall(this);
            return res;
        } catch (RemoteException e) {
            Log.e(TAG, "Can not release as catch the RemoteException e: " + e);
            return Result.FAIL;
        }
    }

    public int updateSRVCCResult(int srvccResult) {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Try to update the SRVCC result for the call: " + mCallId);
        }

        if (mICall == null) {
            Log.e(TAG, "Can not update the SRVCC result as the call interface is null.");
            return Result.FAIL;
        }

        try {
            // update the SRVCC result as SRVCC canceled or failed, update the mInSRVCC state.
            mInSRVCC = false;

            int res = Result.FAIL;
            if (isConferenceCall()) {
                res = mICall.confUpdateSRVCCResult(mCallId, srvccResult);
            } else {
                res = mICall.sessUpdateSRVCCResult(mCallId, srvccResult);
            }
            if (res == Result.FAIL) {
                Log.e(TAG, "Native failed to update the SRVCC result.");
            }
            return res;
        } catch (RemoteException e) {
            Log.e(TAG, "Can not update the SRVCC result as catch the RemoteException e: " + e);
            return Result.FAIL;
        }
    }

    public CameraCapabilities requestCameraCapabilites(int videoQuality) {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Try to request the camera capabilites for call: " + mCallId);
        }

        if (mICall == null) {
            Log.e(TAG, "Can not start the camera as the call interface is null.");
            return null;
        }

        try {
            // If the call do not establish, the camera capabilities do not consult, return null.
            if (getState() < State.ESTABLISHED) {
                // Use the given video quality to build the camera capabilities.
                VideoQuality video = Utilities.sVideoQualityList.get(videoQuality);
                return new CameraCapabilities(video._width, video._height);
            }

            String res = mICall.sessGetCameraCapabilities(mCallId);
            if (TextUtils.isEmpty(res)) {
                Log.w(TAG, "Can not request the camera capabilites for the call " + mCallId);
                VideoQuality video = Utilities.sVideoQualityList.get(videoQuality);
                return new CameraCapabilities(video._width, video._height);
            } else {
                String[] capabilites = res.split(",");
                if (capabilites.length != 2) {
                    Log.e(TAG, "The camera capabilites do not match the format: " + res);
                    return null;
                }

                int width = Integer.valueOf(capabilites[0]);
                int height = Integer.valueOf(capabilites[1]);
                return new CameraCapabilities(width, height);
            }
        } catch (RemoteException e) {
            Log.e(TAG, "Can not get the camera capabilities as catch the RemoteException e: " + e);
            return null;
        }
    }

    public int startCamera(String cameraId) {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Try to start the camera: " + cameraId + " for the call: " + mCallId);
        }
        if (TextUtils.isEmpty(cameraId)) {
            Log.e(TAG, "Can not start the camera as the camera id is null.");
            return Result.FAIL;
        }

        if (mICall == null) {
            Log.e(TAG, "Can not start the camera as the call interface is null.");
            return Result.FAIL;
        }

        try {
            int res = mICall.cameraAttach(isConferenceCall(), mCallId, Camera.isFront(cameraId));
            if (res == Result.FAIL) {
                Log.w(TAG, "Can not start the camera as " + cameraId);
            }
            return res;
        } catch (RemoteException e) {
            Log.e(TAG, "Can not start the camera as catch the RemoteException e: " + e);
            return Result.FAIL;
        }
    }

    public int stopCamera() {
        if (Utilities.DEBUG) Log.i(TAG, "Try to stop the camera for the call: " + mCallId);

        if (mICall == null) {
            Log.e(TAG, "Can not stop the camera as the call interface is null.");
            return Result.FAIL;
        }

        try {
            int res = mICall.cameraDetach(isConferenceCall(), mCallId);
            if (res == Result.FAIL) {
                Log.w(TAG, "Can not stop the camera.");
            }
            return res;
        } catch (RemoteException e) {
            Log.e(TAG, "Can not stop the camera as catch the RemoteException e: " + e);
            return Result.FAIL;
        }
    }

    public int startLocalRender(Surface previewSurface, String cameraId) {
        if (Utilities.DEBUG) Log.i(TAG, "Try to start the local render for the call: " + mCallId);

        if (mICall == null) {
            Log.e(TAG, "Can not start the local render as the call interface is null.");
            return Result.FAIL;
        }

        try {
            int res = mICall.localRenderAdd(previewSurface, Camera.isFront(cameraId));
            if (res == Result.FAIL) {
                Log.w(TAG, "Can not start the local render.");
            }
            return res;
        } catch (RemoteException e) {
            Log.e(TAG, "Can not start the local render as catch the RemoteException e: " + e);
            return Result.FAIL;
        }
    }

    public int stopLocalRender(Surface previewSurface, String cameraId) {
        if (Utilities.DEBUG) Log.i(TAG, "Try to stop the local render for the call: " + mCallId);

        if (mICall == null) {
            Log.e(TAG, "Can not stop the local render as the call interface is null.");
            return Result.FAIL;
        }

        try {
            int res = mICall.localRenderRemove(previewSurface, Camera.isFront(cameraId));
            if (res == Result.FAIL) {
                Log.w(TAG, "Can not stop the local render.");
            }
            return res;
        } catch (RemoteException e) {
            Log.e(TAG, "Can not stop the local render as catch the RemoteException e: " + e);
            return Result.FAIL;
        }
    }

    public int startRemoteRender(Surface displaySurface) {
        if (Utilities.DEBUG) Log.i(TAG, "Try to start the remote render for the call: " + mCallId);

        if (mICall == null) {
            Log.e(TAG, "Can not start the remote render as the call interface is null.");
            return Result.FAIL;
        }

        try {
            int res = mICall.remoteRenderAdd(displaySurface, isConferenceCall(), mCallId);
            if (res == Result.FAIL) {
                Log.w(TAG, "Can not start the remote render.");
            }
            return res;
        } catch (RemoteException e) {
            Log.e(TAG, "Can not start the remote render as catch the RemoteException e: " + e);
            return Result.FAIL;
        }
    }

    public int stopRemoteRender(Surface displaySurface, boolean isAsync) {
        if (Utilities.DEBUG) Log.i(TAG, "Try to stop the remote render for the call: " + mCallId);

        if (mICall == null) {
            Log.e(TAG, "Can not stop the remote render as the call interface is null.");
            return Result.FAIL;
        }

        try {
            int res = mICall.remoteRenderRemove(displaySurface, isConferenceCall(), mCallId);
            if (res == Result.FAIL) {
                Log.w(TAG, "Can not stop the remote render.");
            }
            return res;
        } catch (RemoteException e) {
            Log.e(TAG, "Can not stop the remote render as catch the RemoteException e: " + e);
            return Result.FAIL;
        }
    }

    public int startCapture(String cameraId, int videoQuality) {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Try to start capture for the call: " + mCallId + ", cameraId: " + cameraId
                    + ", videoQuality index: " + videoQuality);
        }

        if (mICall == null) {
            Log.e(TAG, "Can not start capture as the call interface is null.");
            return Result.FAIL;
        }

        try {
            VideoQuality video = Utilities.sVideoQualityList.get(videoQuality);
            int res = mICall.captureStart(
                    Camera.isFront(cameraId), video._width, video._height, video._frameRate);
            if (res == Result.FAIL) {
                Log.w(TAG, "Can not start capture.");
            }
            return res;
        } catch (RemoteException e) {
            Log.e(TAG, "Can not start capture as catch the RemoteException e: " + e);
            return Result.FAIL;
        }
    }

    public int stopCapture(String cameraId) {
        if (Utilities.DEBUG) Log.i(TAG, "Try to stop capture for the call: " + mCallId);

        if (mICall == null) {
            Log.e(TAG, "Can not stop capture as the call interface is null.");
            return Result.FAIL;
        }

        try {
            int res = mICall.captureStop(Camera.isFront(cameraId));
            if (res == Result.FAIL) {
                Log.w(TAG, "Can not stop capture.");
            }
            return res;
        } catch (RemoteException e) {
            Log.e(TAG, "Can not stop capture as catch the RemoteException e: " + e);
            return Result.FAIL;
        }
    }

    public int localRenderRotate(String cameraId, int angle, int deviceOrientation) {
        if (Utilities.DEBUG) Log.i(TAG, "Try to rotate local render for the call: " + mCallId);

        if (mICall == null) {
            Log.e(TAG, "Can not rotate local render as call interface is null.");
            return Result.FAIL;
        }

        try {
            int res = mICall.localRenderRotate(Camera.isFront(cameraId), angle, deviceOrientation);
            if (res == Result.FAIL) {
                Log.w(TAG, "Can not rotate local render for the call: " + mCallId);
            }
            return res;
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to rotate local render as catch RemoteException e: " + e);
            return Result.FAIL;
        }
    }

    public int remoteRenderRotate(int angle) {
        if (Utilities.DEBUG) Log.i(TAG, "Try to rotate remote render for the call: " + mCallId);

        if (mICall == null) {
            Log.e(TAG, "Can not rotate remote render as call interface is null.");
            return Result.FAIL;
        }

        try {
            int res = mICall.remoteRenderRotate(isConferenceCall(), mCallId, angle);
            if (res == Result.FAIL) {
                Log.w(TAG, "Can not rotate remote render for the call: " + mCallId);
            }
            return res;
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to rotate remote render as catch RemoteException e: " + e);
            return Result.FAIL;
        }
    }

    public int sendModifyRequest(boolean isVideo) {
        if (Utilities.DEBUG) Log.i(TAG, "Try to send the modify request, isVideo: " + isVideo);

        if (mICall == null) {
            Log.e(TAG, "Can not send the modify request as call interface is null.");
            return Result.FAIL;
        }

        try {
            int res = mICall.sendSessionModifyRequest(mCallId, true, isVideo);
            if (res == Result.FAIL) {
                Log.e(TAG, "Failed to send the modify request for the call: " + mCallId);
                mListener.callSessionUpdateFailed(this, new ImsReasonInfo());
                // Show toast for failed action.
                int toastTextResId = isVideo ? R.string.vowifi_add_video_failed
                        : R.string.vowifi_remove_video_failed;
                Toast.makeText(mContext, toastTextResId, Toast.LENGTH_LONG).show();
            }
            return res;
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to send the modify request as catch RemoteException e: " + e);
            return Result.FAIL;
        }
    }

    public int setPauseImage(Uri uri) {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Set the pause image to " + uri + " for the call: " + mCallId);
        }

        if (mICall == null) {
            Log.e(TAG, "Can not set the pause image as call interface is null.");
            return Result.FAIL;
        }

        try {
            boolean start = false;
            String uriString = "";
            if (uri != null && !TextUtils.isEmpty(uri.toString())) {
                start = true;
                uriString = uri.getPath().toString();
            }

            int res = mICall.confSetLocalImageForTrans(mCallId, uriString, start);
            if (res == Result.FAIL) {
                Log.w(TAG, "Can not set the pause image to " + uri + " as "
                        + (start ? "start." : "stop."));
            }
            return res;
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to sset the pause image as catch RemoteException e: " + e);
            return Result.FAIL;
        }
    }

    public int getPacketLose() {
        if (Utilities.DEBUG) Log.i(TAG, "Try to get the packet lose.");
        if (mICall == null) {
            Log.e(TAG, "Can not get the packet lose as the call interface is null.");
            return 0;
        }

        try {
            boolean isVideo = Utilities.isVideoCall(mCallProfile.mCallType);
            return mICall.getMediaLostRatio(mCallId, isConferenceCall(), isVideo);
        } catch (RemoteException e) {
            Log.e(TAG, "Catch the remote exception when get the media lose, e: " + e);
        }

        return Result.FAIL;
    }

    public int getJitter() {
        if (Utilities.DEBUG) Log.i(TAG, "Try to get the jitter.");
        if (mICall == null) {
            Log.e(TAG, "Can not get the jitter as the call interface is null.");
            return 0;
        }

        try {
            boolean isVideo = Utilities.isVideoCall(mCallProfile.mCallType);
            return mICall.getMediaJitter(mCallId, isConferenceCall(), isVideo);
        } catch (RemoteException e) {
            Log.e(TAG, "Catch the remote exception when get the media jitter, e: " + e);
        }

        return Result.FAIL;
    }

    public int getRtt() {
        if (Utilities.DEBUG) Log.i(TAG, "Try to get the rtt.");
        if (mICall == null) {
            Log.e(TAG, "Can not get the rtt as the call interface is null.");
            return 0;
        }

        try {
            boolean isVideo = Utilities.isVideoCall(mCallProfile.mCallType);
            return mICall.getMediaRtt(mCallId, isConferenceCall(), isVideo);
        } catch (RemoteException e) {
            Log.e(TAG, "Catch the remote exception when get the media rtt, e: " + e);
        }

        return Result.FAIL;
    }

    public void dialEmergencyCall() {
        if (Utilities.DEBUG) Log.i(TAG, "Try to dial this emergency call: " + this);

        try {
            startCall(mParticipants.get(0));
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to dial the emergency call as catch the RemoteException e: " + e);
        }
    }

    public void terminateCall(int reason) {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Terminate this call: " + this + " for reason: " + reason);
        }

        if (mICall == null) return;

        try {
            int res = Result.FAIL;
            if (isConferenceCall()) {
                res = mICall.confTerm(mCallId, reason);
            } else {
                res = mICall.sessTerm(mCallId, reason);
            }

            if (res == Result.SUCCESS) {
                // As call terminated, stop all the video.
                // Note: This stop action need before call session terminated callback. Otherwise,
                //       the video call provider maybe changed to null.
                if (mVideoCallProvider != null) {
                    mVideoCallProvider.stopAll();
                }

                int oldState = getState();
                updateState(State.TERMINATED);
                if (mListener != null) {
                    ImsReasonInfo info = new ImsReasonInfo(reason, reason, "reason: " + reason);
                    if (oldState < State.NEGOTIATING) {
                        // It means the call do not ringing now, so we need give the call session
                        // start failed call back.
                        mListener.callSessionStartFailed(this, info);
                    } else {
                        mListener.callSessionTerminated(this, info);
                    }
                }
                mCallManager.removeCall(this);
            } else {
                Log.e(TAG, "Native terminate a call failed, res = " + res);
            }
        } catch (RemoteException e) {
            Log.e(TAG, "Can not terminate the call as catch the RemoteException: " + e);
        }
    }

    public void processNoResponseAction() {
        if (Utilities.DEBUG) Log.i(TAG, "Process no response action.");

        try {
            ImsReasonInfo failedReason = new ImsReasonInfo(
                    ImsReasonInfo.CODE_LOCAL_HO_NOT_FEASIBLE,
                    ImsReasonInfo.CODE_UNSPECIFIED,
                    "No response when SRVCC");

            switch (mCallStateTracker.getSRVCCNoResponseAction()) {
                case VoWifiCallStateTracker.ACTION_ACCEPT:
                    mListener.callSessionStartFailed(ImsCallSessionImpl.this, failedReason);
                    break;
                case VoWifiCallStateTracker.ACTION_REJECT:
                case VoWifiCallStateTracker.ACTION_TERMINATE:
                    mListener.callSessionTerminated(ImsCallSessionImpl.this, failedReason);
                case VoWifiCallStateTracker.ACTION_HOLD:
                    mListener.callSessionHoldFailed(ImsCallSessionImpl.this, failedReason);
                case VoWifiCallStateTracker.ACTION_RESUME:
                    mListener.callSessionResumeFailed(ImsCallSessionImpl.this, failedReason);
            }
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to process the no response action as catch the exception: " + e);
        }
    }

    private void startEmergencyCall(String callee, ImsCallProfile profile) {
        if (Utilities.DEBUG) Log.i(TAG, "Try to start the emergency call.");

        mIsEmergency = true;
        mParticipants.add(callee);

        mCallProfile = profile;
        mCallProfile.setCallExtra(ImsCallProfile.EXTRA_OI, callee);
        mCallProfile.setCallExtra(ImsCallProfile.EXTRA_CNA, null);
        mCallProfile.setCallExtraInt(
                ImsCallProfile.EXTRA_CNAP, ImsCallProfile.OIR_PRESENTATION_NOT_RESTRICTED);
        mCallProfile.setCallExtraInt(
                ImsCallProfile.EXTRA_OIR, ImsCallProfile.OIR_PRESENTATION_NOT_RESTRICTED);

        boolean isPhoneInEcmMode =
                SystemProperties.getBoolean(TelephonyProperties.PROPERTY_INECM_MODE, false);

        if (isPhoneInEcmMode) {
            mCallManager.enterECBMWithCallSession(this);
        } else {
            // make an emergency session directly without Em-PDN and Em-Register.
            try {
                startCall(callee);
            } catch (RemoteException e) {
                Log.e(TAG, "Failed to start the emergency call as catch the RemoteException e: " + e);
            }
        }
    }

    private void startNormalCall(String callee, ImsCallProfile profile) throws RemoteException {
        if (Utilities.DEBUG) Log.i(TAG, "Try to start the normal call.");

        // As the vowifi service is null, need add this action to pending action.
        if (mICall == null) {
            synchronized (mPendingActions) {
                String key = String.valueOf(System.currentTimeMillis());
                PendingAction action = new PendingAction("start", MSG_START, callee, profile);
                mPendingActions.put(key, action);
            }
            return;
        }

        // The vowifi service is not null, init the IMS call.
        mParticipants.add(callee);

        // TODO: update the profile
        mCallProfile = profile;
        mCallProfile.setCallExtra(ImsCallProfile.EXTRA_OI, callee);
        mCallProfile.setCallExtra(ImsCallProfile.EXTRA_CNA, null);
        // TODO: why not {@link ImsCallProfile#OIR_DEFAULT}
        mCallProfile.setCallExtraInt(
                ImsCallProfile.EXTRA_CNAP, ImsCallProfile.OIR_PRESENTATION_NOT_RESTRICTED);
        mCallProfile.setCallExtraInt(
                ImsCallProfile.EXTRA_OIR, ImsCallProfile.OIR_PRESENTATION_NOT_RESTRICTED);

        startCall(callee);
    }

    private void startCall(String callee) throws RemoteException {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Start the call with the callee: " + callee + ", mSosLocation: " + mSosLocation);
        }

        // Start the call.
        boolean isVideoCall = Utilities.isVideoCall(mCallProfile.mCallType);

        int id = Result.INVALID_ID;

        // todo: need location for some special Operator
        boolean isNeedGeoLocation = false;
        if (mIsEmergency) {
            isNeedGeoLocation = true;
        }
        if (isNeedGeoLocation) {
            double latitude = mSosLocation == null ? 0 : mSosLocation.getLatitude();
            double longitude = mSosLocation == null ? 0 : mSosLocation.getLongitude();
            Log.i(TAG, "set session call with latitude= " + latitude + ";longitude=" + longitude);
            mICall.sessCallSetGeolocation(latitude, longitude);
        }

        String peerNumber = callee; //default
        if (mIsEmergency) {
            peerNumber = mCallManager.getEmergencyCallUrn(mCallProfile.getCallExtra(ImsCallProfile.EXTRA_ADDITIONAL_CALL_INFO));
            Log.d(TAG, "Start an emergency call.");
        }
        id = mICall.sessCall(peerNumber, null, true, isVideoCall, false, mIsEmergency);

        Log.d(TAG, "Start a normal call, and get the call id: " + id);

        if (id == Result.INVALID_ID) {
            handleStartActionFailed("Native start the call failed.");
        } else {
            mCallId = id;
            updateState(State.INITIATED);
            mIsAlive = true;
            startAudio();

            // Start action success, update the last call action as start.
            updateRequestAction(VoWifiCallStateTracker.ACTION_START);
        }
    }

    private void inviteToConference(ImsCallSessionImpl confSession) throws RemoteException {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Try to invite this call " + mCallId + " to the conference call "
                    + confSession.getCallId());
        }

        // Invite the this call session as the participants.
        int res = mICall.confAddMembers(
                Integer.valueOf(confSession.getCallId()), null, new int[] { mCallId });
        if (res == Result.SUCCESS) {
            // Invite this call success, set this call session as the in invite.
            confSession.setInInviteCall(this);

            // Notify merge complete.
            if (mListener != null) {
                // As there is a conference call at background or foreground. As this callback
                // defined in the framework, we need give this callback with null object.
                mListener.callSessionMergeComplete(null);
            }

            IImsCallSessionListener confListener = confSession.getListener();
            if (confListener != null) {
                // Invite participants success.
                confListener.callSessionInviteParticipantsRequestDelivered(confSession);
            }

            mConfCallSession = confSession;
            // As this call session will be invited to the conference, update the state and
            // stop all the video.
            mIsAlive = false;
            updateState(State.TERMINATING);
            mVideoCallProvider.stopAll();

            // For normal call, there will be only one participant, and we need add it to this
            // conference participants list.
            String participant = getCallee();
            confSession.addCallee(participant);
        } else {
            // Failed to invite this call to conference. Prompt the toast to alert the user.
            Log.w(TAG, "Failed to invite this call " + mCallId + " to conference.");
            String errorText =
                    mContext.getString(R.string.vowifi_conf_invite_failed) + getCallee();
            Toast.makeText(mContext, errorText, Toast.LENGTH_LONG).show();
        }
    }

    private void createConfCall() throws RemoteException {
        if (Utilities.DEBUG) Log.i(TAG, "Try to create the conference call.");

        // If there is the video call, we need start the conference as video conference.
        boolean isVideo = mCallManager.getVideoCallSessions() != null;
        // For merge action, we need setup the conference as init and setup.
        int confId = mICall.confInit(isVideo);
        if (confId == Result.INVALID_ID) {
            // It means init the conference failed, give the callback.
            handleMergeActionFailed("Init the conference call failed.");
            return;
        }

        int res = mICall.confSetup(confId, null /* cookie, do not use now */);
        if (res == Result.FAIL) {
            // Failed to setup the conference call.
            Log.e(TAG, "Failed to setup the conference call with the conference id: " + confId);
            handleMergeActionFailed("Failed to setup the conference call.");
        } else {
            // Create the new call session for the conference.
            int callType = isVideo ? ImsCallProfile.CALL_TYPE_VT : ImsCallProfile.CALL_TYPE_VOICE;
            ImsCallProfile imsCallProfile = new ImsCallProfile(mCallProfile.mServiceType, callType);
            imsCallProfile.setCallExtra(ImsCallProfile.EXTRA_OI, getCallee());
            imsCallProfile.setCallExtra(ImsCallProfile.EXTRA_CNA, null);
            // TODO: why not {@link ImsCallProfile#OIR_DEFAULT}
            imsCallProfile.setCallExtraInt(
                    ImsCallProfile.EXTRA_CNAP, ImsCallProfile.OIR_PRESENTATION_NOT_RESTRICTED);
            imsCallProfile.setCallExtraInt(
                    ImsCallProfile.EXTRA_OIR, ImsCallProfile.OIR_PRESENTATION_NOT_RESTRICTED);

            imsCallProfile.setCallExtraBoolean(ImsCallProfile.EXTRA_CONFERENCE, true);
            ImsCallSessionImpl newConfSession =
                    mCallManager.createMOCallSession(imsCallProfile, null);
            newConfSession.setCallId(confId);
            newConfSession.updateMediaProfile(mImsStreamMediaProfile);
            newConfSession.setHostCallSession(this);
            newConfSession.updateState(State.INITIATED);
            newConfSession.updateIsConfHost(true);
            newConfSession.startAudio();

            if (mListener != null) {
                mListener.callSessionMergeStarted(this, newConfSession, imsCallProfile);
            }
            mHandler.sendEmptyMessageDelayed(MSG_MERGE_FAILED, MERGE_TIMEOUT);
        }
    }

    private int getDtmfType(char c) {
        int type = -1;
        switch (c) {
            case '0':
                type = 0;
                break;
            case '1':
                type = 1;
                break;
            case '2':
                type = 2;
                break;
            case '3':
                type = 3;
                break;
            case '4':
                type = 4;
                break;
            case '5':
                type = 5;
                break;
            case '6':
                type = 6;
                break;
            case '7':
                type = 7;
                break;
            case '8':
                type = 8;
                break;
            case '9':
                type = 9;
                break;
            case '*':
                type = 10;
                break;
            case '#':
                type = 11;
                break;
            case 'A':
                type = 12;
                break;
            case 'B':
                type = 13;
                break;
            case 'C':
                type = 14;
                break;
            case 'D':
                type = 15;
                break;
            default:
                break;
        }
        return type;
    }

    private boolean isConferenceCall() {
        return mIsConfHost && (mCallProfile == null ? false
                : mCallProfile.getCallExtraBoolean(ImsCallProfile.EXTRA_CONFERENCE, false));
    }

    private boolean isSameCallee(String calleeNumber, String phoneNumber) {
        if (TextUtils.isEmpty(calleeNumber)
                || TextUtils.isEmpty(phoneNumber)) {
            return false;
        }

        if (phoneNumber.indexOf(calleeNumber) >= 0
                || calleeNumber.indexOf(phoneNumber) >= 0) {
            return true;
        } else if (calleeNumber.startsWith("0")) {
            // Sometimes, the phone number will be start will 0, we'd like to sub the string.
            String tempCallee = calleeNumber.substring(1);
            if (phoneNumber.indexOf(tempCallee) >= 0
                    || tempCallee.indexOf(phoneNumber) >= 0) {
                return true;
            }
        }

        return false;
    }

    private String[] findNeedRemove(String[] participants) {
        String[] remove = new String[participants.length];
        int index = 0;
        for (String participant : participants) {
            if (!mInKickParticipants.contains(participant)) {
                remove[index] = participant;
                index = index + 1;
            }
        }
        return remove;
    }

    private void addAsInKickProcess(String[] participants) {
        if (participants == null || participants.length <1) {
            Log.w(TAG, "The list which set as in kick is null, please check!");
            return;
        }

        for (String participant : participants) {
            mInKickParticipants.add(participant);
        }
    }

    private void removeFromInKick(String[] participants) {
        if (participants == null || participants.length <1) {
            Log.w(TAG, "The list which remove from in kick is null, please check!");
            return;
        }

        for (String participant : participants) {
            mInKickParticipants.remove(participant);
        }
    }

    private int getSRVCCCallType() {
        if (mIsEmergency) {
            return SRVCCSyncInfo.CallType.EMERGENCY;
        } else {
            // CP only supports audio SRVCC now, so set callType to NORMAL here.
            return SRVCCSyncInfo.CallType.NORMAL;
        }
    }

    public boolean getIsEmergency() {
        return mIsEmergency;
    }
}
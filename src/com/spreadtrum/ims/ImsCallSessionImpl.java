package com.spreadtrum.ims;

import java.util.List;
import java.util.Map;
import java.util.HashMap;
import java.util.Iterator;

import com.android.internal.telephony.CommandsInterface;
import com.spreadtrum.ims.ImsDriverCall;
import com.android.internal.telephony.LastCallFailCause;

import com.android.ims.ImsStreamMediaProfile;
import com.android.ims.ImsCallProfile;
import com.android.ims.ImsReasonInfo;
import com.android.ims.ImsConferenceState;
import com.android.ims.internal.ImsCallSession;
import com.android.ims.internal.IImsCallSession;
import com.android.ims.internal.IImsCallSessionListener;
import com.android.ims.internal.IImsVideoCallProvider;
import com.spreadtrum.ims.vt.ImsVideoCallProvider;
import com.spreadtrum.ims.vt.VTManagerUtils;

import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.AsyncResult;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.RemoteException;
import android.util.Log;
import java.util.concurrent.CopyOnWriteArrayList;
import android.telecom.VideoProfile;

public class ImsCallSessionImpl extends IImsCallSession.Stub {
    private static final String TAG = ImsCallSessionImpl.class.getSimpleName();

    private static final String IMS_CONFERENCE_ID = "id";

    private static final int ACTION_COMPLETE_DIAL = 1;
    private static final int ACTION_COMPLETE_HOLD = 2;
    private static final int ACTION_COMPLETE_RESUME = 3;
    private static final int ACTION_COMPLETE_ACCEPT = 4;
    private static final int ACTION_COMPLETE_HANGUP = 5;
    private static final int ACTION_COMPLETE_REJECT = 6;
    private static final int ACTION_COMPLETE_DEFLECT = 7;
    private static final int ACTION_COMPLETE_MERGE = 8;
    private static final int ACTION_COMPLETE_CONFERENCE = 9;
    private static final int ACTION_COMPLETE_ADD_PARTICIPANT = 10;
    private static final int ACTION_COMPLETE_RINGBACK_TONE = 11;
    private static final int ACTION_COMPLETE_REMOVE_PARTICIPANT = 12;
    private static final int ACTION_COMPLETE_GET_CALL_FAIL_CAUSE = 13;

    private List<Listener>  mCallSessionImplListeners = new CopyOnWriteArrayList<Listener>();
    private int mState = ImsCallSession.State.IDLE;
    private ImsHandler mHandler;
    public ImsCallProfile mImsCallProfile = new ImsCallProfile();
    private ImsCallProfile mLocalCallProfile = new ImsCallProfile(
            ImsCallProfile.SERVICE_TYPE_NORMAL, ImsCallProfile.CALL_TYPE_VIDEO_N_VOICE);
    private ImsCallProfile mRemoteCallProfile = new ImsCallProfile(
            ImsCallProfile.SERVICE_TYPE_NORMAL, ImsCallProfile.CALL_TYPE_VIDEO_N_VOICE);
    private ImsCallProfile mLocalRequestProfile = new ImsCallProfile();
    private ImsCallProfile mRemoteRequestProfile = new ImsCallProfile();
    private IImsCallSessionListener mIImsCallSessionListener;
    private Context mContext;
    public ImsRIL mCi;
    private ImsServiceCallTracker mImsServiceCallTracker;
    public ImsDriverCall mImsDriverCall;
    private int mDisconnCause = ImsReasonInfo.CODE_UNSPECIFIED;
    private boolean mDesiredMute = false;
    private String mCallee;
    private ImsVideoCallProvider mImsVideoCallProvider;
    // SPRD: add for bug524928
    private boolean mIsMegerAction;
    private boolean mShouldNotifyMegerd;
    private ImsConferenceState mImsConferenceState;
    private Object mConferenceLock = new Object();
    private boolean mConferenceHost = false;
    private boolean mIsConferenceHeld = false;
    private boolean mIsConferenceActived = false;

    public ImsCallSessionImpl(ImsCallProfile profile, IImsCallSessionListener listener, Context context,
            ImsRIL ci, ImsServiceCallTracker callTracker){
        mImsCallProfile = profile;
        mIImsCallSessionListener = listener;
        mContext = context;
        mCi = ci;
        mImsServiceCallTracker = callTracker;
        mHandler = new ImsHandler(context.getMainLooper(),this);
        mImsVideoCallProvider = new ImsVideoCallProvider(this,ci,mContext) ;
    }

    public ImsCallSessionImpl(ImsDriverCall dc, IImsCallSessionListener listener, Context context,
            ImsRIL ci, ImsServiceCallTracker callTracker){
        mImsDriverCall = new ImsDriverCall(dc);
        updateImsCallProfileFromDC(dc);
        mIImsCallSessionListener = listener;
        mContext = context;
        mCi = ci;
        mImsServiceCallTracker = callTracker;
        mHandler = new ImsHandler(context.getMainLooper(),this);
        mImsVideoCallProvider = new ImsVideoCallProvider(this,ci,mContext) ;
        updateVideoProfile(mImsDriverCall);
    }

    public ImsCallSessionImpl(ImsDriverCall dc, Context context,
            ImsRIL ci, ImsServiceCallTracker callTracker){
        mImsDriverCall = new ImsDriverCall(dc);
        updateImsCallProfileFromDC(dc);
        mContext = context;
        mCi = ci;
        mImsServiceCallTracker = callTracker;
        mHandler = new ImsHandler(context.getMainLooper(),this);
        mImsVideoCallProvider = new ImsVideoCallProvider(this,ci,mContext) ;
        updateVideoProfile(mImsDriverCall);
    }

    private void updateImsCallProfileFromDC(ImsDriverCall dc){
        if(mImsCallProfile == null) {
            mImsCallProfile = new ImsCallProfile();
        }
        mImsCallProfile.setCallExtra(ImsCallProfile.EXTRA_OI, dc.number);
        mImsCallProfile.setCallExtra(ImsCallProfile.EXTRA_CNA, dc.name);
        mImsCallProfile.setCallExtraInt(ImsCallProfile.EXTRA_OIR,
                ImsCallProfile.presentationToOIR(dc.numberPresentation));
        mImsCallProfile.setCallExtraInt(ImsCallProfile.EXTRA_CNAP,
                ImsCallProfile.presentationToOIR(dc.namePresentation));
        if(dc.isVideoCall()){
            mImsCallProfile.mCallType = ImsCallProfile.CALL_TYPE_VT;
        } else {
            mImsCallProfile.mCallType = ImsCallProfile.CALL_TYPE_VOICE_N_VIDEO;
        }
    }

    public boolean updateFromDc(ImsDriverCall dc){
        if(isImsSessionInvalid() || dc == null){
            Log.d(TAG, "updateFromDc->ImsSessionInvalid! dc:" + dc);
            return false;
        }
        boolean knownState = mImsDriverCall != null && dc != null &&
                mImsDriverCall.state == dc.state;

        updateImsCallProfileFromDC(dc);
        ImsDriverCall.State state = dc.state;
        boolean conferenceHeldStateChange = false;
        boolean conferenceActiveStateChange = false;
        boolean conferenceResumed = false;
        if(mConferenceHost){
            boolean conferenceHeld = false;
            boolean conferenceActive = false;
            for (Map.Entry<String, Bundle> entry : mImsConferenceState.mParticipants.entrySet()) {
                String stateString = entry.getValue().getString(ImsConferenceState.STATUS);
                ImsDriverCall.State conferenceState = ImsDriverCall.ConferenceStringToState(stateString);
                if(conferenceState == ImsDriverCall.State.HOLDING){
                    conferenceHeld = true;
                } else if(conferenceState == ImsDriverCall.State.ACTIVE){
                    conferenceActive = true;
                }
            }
            if(conferenceHeld && conferenceActive){
                conferenceActive = false;
                Log.d(TAG, "updateFromDc->conferenceHeld and conferenceActive both true!");
            }
            if(conferenceActive && mIsConferenceHeld){
                conferenceResumed = true;
            }
            if(conferenceHeld != mIsConferenceHeld){
                mIsConferenceHeld = conferenceHeld;
                conferenceHeldStateChange = true;
            }
            if(mIsConferenceActived != conferenceActive){
                mIsConferenceActived = conferenceActive;
                conferenceActiveStateChange = true;
            }

            Log.d(TAG, "updateFromDc->conferenceHeld:" + conferenceHeld + " conferenceResumed:"+conferenceResumed);
            if(mIsConferenceHeld){
                state = ImsDriverCall.State.HOLDING;
            } else if(mIsConferenceActived){
                state = ImsDriverCall.State.ACTIVE;
            }
        }
        Log.d(TAG, "updateFromDc->mIsConferenceHeld:" + mIsConferenceHeld + " mIsConferenceActived:"+mIsConferenceActived
                +" mIsMegerAction:"+mIsMegerAction);
        switch(state){
            case DIALING:
                try{
                    if (mIImsCallSessionListener != null) {
                        mIImsCallSessionListener.callSessionProgressing((IImsCallSession) this,
                                new ImsStreamMediaProfile());
                    }
                } catch(RemoteException e){
                    e.printStackTrace();
                }
                break;
            case ALERTING:
                try{
                    mState = ImsCallSession.State.NEGOTIATING;
                    if (mImsDriverCall != null && mImsDriverCall.state != ImsDriverCall.State.ALERTING
                            && mIImsCallSessionListener != null) {
                        mIImsCallSessionListener.callSessionProgressing((IImsCallSession) this,
                                new ImsStreamMediaProfile());
                    }
                } catch(RemoteException e){
                    e.printStackTrace();
                }
                break;
            case ACTIVE:
                mState = ImsCallSession.State.ESTABLISHED;
                try{
                    if (mIImsCallSessionListener != null) {
                        if(mImsDriverCall != null && mImsDriverCall.state == ImsDriverCall.State.HOLDING
                                || conferenceResumed){
                            mIImsCallSessionListener.callSessionResumed((IImsCallSession)this, mImsCallProfile);
                        } else if (mImsDriverCall != null && ((mImsDriverCall.state == ImsDriverCall.State.DIALING)
                                || (mImsDriverCall.state == ImsDriverCall.State.ALERTING)
                                || (mImsDriverCall.state == ImsDriverCall.State.INCOMING)
                                || (mImsDriverCall.state == ImsDriverCall.State.WAITING))) {
                            mIImsCallSessionListener.callSessionStarted((IImsCallSession) this,mImsCallProfile);
                        }
                    }
                } catch(RemoteException e){
                    e.printStackTrace();
                }
                break;
            case HOLDING:
                try{
                    if (mIImsCallSessionListener != null &&
                            (mImsDriverCall.state != ImsDriverCall.State.HOLDING || conferenceHeldStateChange)) {
                        mIImsCallSessionListener.callSessionHeld((IImsCallSession)this, mImsCallProfile);
                    }
                } catch(RemoteException e){
                    e.printStackTrace();
                }
                break;
            case INCOMING:
                break;
            case WAITING:
                break;
            case DISCONNECTED:
                mState = ImsCallSession.State.TERMINATED;
                try{
                    if (mIImsCallSessionListener != null){
                        mIImsCallSessionListener.callSessionTerminated((IImsCallSession) this,
                                new ImsReasonInfo(mDisconnCause, 0));
                    }
                } catch(RemoteException e){
                    e.printStackTrace();
                }
                break;
            default:
                Log.w(TAG, "updateFromDc->unsupported state: "+dc.state);
                break;
        }
        boolean hasUpdate = false;
        if (mImsDriverCall == null) {
            mImsDriverCall = new ImsDriverCall(dc);
            hasUpdate = true;
        }
        hasUpdate = mImsDriverCall.update(dc) || hasUpdate;
        if (mImsDriverCall.state == ImsDriverCall.State.ACTIVE) {
            if (mIsMegerAction) {
                mIsMegerAction = false;
                mImsServiceCallTracker.onCallMergeComplete(this);
                if(!mConferenceHost){
                    disconnectForConferenceMember();
                    return false;
                }
            }
        }
        try{
            if(hasUpdate && knownState
                    && mIImsCallSessionListener != null){
                mIImsCallSessionListener.callSessionUpdated((IImsCallSession)this, mImsCallProfile);
            }
            if(hasUpdate || !knownState){
                notifySessionUpdate();
            }
        } catch(RemoteException e){
            e.printStackTrace();
        }
        updateVideoProfile(mImsDriverCall);
        hasUpdate = hasUpdate || conferenceHeldStateChange || conferenceActiveStateChange;
        Log.d(TAG, "updateFromDc->hasUpdate:"+hasUpdate+" dc:" + dc);
        return hasUpdate;
    }

    private boolean isImsSessionInvalid() {
        boolean invalid = (mState == ImsCallSession.State.INVALID);
        if (invalid) {
            Log.w(TAG, "Session is invalid");
        }
        return invalid;
    }

    public interface Listener {
        void onDisconnected(ImsCallSessionImpl session);
        void onUpdate(ImsCallSessionImpl session);
    }

    public void addListener(Listener listener) {
        if (isImsSessionInvalid()) return;

        if (listener == null) {
            Log.w(TAG,"addListener->Listener is null!");
        }
        synchronized (mCallSessionImplListeners) {
            if (!mCallSessionImplListeners.contains(listener)) {
                mCallSessionImplListeners.add(listener);
            } else {
                Log.w(TAG,"Listener already add :" + listener);
            }
        }
    }

    public void removeListener(Listener listener) {
        if (isImsSessionInvalid()) return;

        if (listener == null) {
            Log.w(TAG,"removeListener->Listener is null!");
        }

        synchronized (mCallSessionImplListeners) {
            if (mCallSessionImplListeners.contains(listener)) {
                mCallSessionImplListeners.remove(listener);
            } else {
                Log.w(TAG,"Listener not find " + listener);
            }
        }
    }

    private void notifySessionUpdate(){
        if (isImsSessionInvalid()) return;
        synchronized (mCallSessionImplListeners) {
            for(Listener l : mCallSessionImplListeners) {
                l.onUpdate(this);
            }
        }
    }

    public void notifySessionDisconnected() {
        mState = ImsCallSession.State.TERMINATED;
        try {
            if ((mIImsCallSessionListener != null) && (mImsDriverCall != null)) {
                if (mImsDriverCall.state == ImsDriverCall.State.INCOMING) {
                    mDisconnCause = ImsReasonInfo.CODE_USER_TERMINATED_BY_REMOTE;
                }
                Log.w(TAG, "notifySessionDisconnected  mDisconnCause=" + mDisconnCause);
                mIImsCallSessionListener.callSessionTerminated((IImsCallSession) this,
                        new ImsReasonInfo(mDisconnCause, 0));
            }else if(mImsDriverCall == null){/* SPRD: add for bug525777 @{ */
                Log.w(TAG, "notifySessionDisconnected(Fdn)  mDisconnCause=" + mDisconnCause);
                mIImsCallSessionListener.callSessionStartFailed((IImsCallSession) this,
                        new ImsReasonInfo(mDisconnCause, 0));
                mCi.getLastCallFailCause(mHandler.obtainMessage(ACTION_COMPLETE_GET_CALL_FAIL_CAUSE,this));
            }/* @} */
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        synchronized (mCallSessionImplListeners) {
            for (Listener l : mCallSessionImplListeners) {
                l.onDisconnected(this);
                Log.i(TAG, "notifySessionDisconnected..l=" + l);
            }
        }
    }

    private class ImsHandler extends Handler {
        private ImsCallSessionImpl mImsCallSessionImpl;
        ImsHandler(Looper looper,ImsCallSessionImpl imsCallSessionImpl) {
            super(looper);
            mImsCallSessionImpl = imsCallSessionImpl;
        }
        @Override
        public void handleMessage(Message msg) {
            AsyncResult ar = (AsyncResult) msg.obj;
            Log.i(TAG,"handleMessage->message:"+msg.what);
            switch (msg.what) {
                case ACTION_COMPLETE_DIAL:
                    if (ar != null && ar.exception != null) {
                        Log.w(TAG,"handleMessage->ACTION_COMPLETE_DIAL error!");
                        try{
                            mIImsCallSessionListener.callSessionStartFailed(mImsCallSessionImpl,
                                    new ImsReasonInfo(ImsReasonInfo.CODE_UNSPECIFIED, 0,"Dial Failed"));
                        } catch(RemoteException e){
                            e.printStackTrace();
                        }
                    }
                    //SPRD: add for bug525777
                    mImsServiceCallTracker.pollCallsAfterOperationComplete();
                    break;
                case ACTION_COMPLETE_HOLD:
                    if (ar != null && ar.exception != null) {
                        Log.w(TAG,"handleMessage->ACTION_COMPLETE_HOLD error!");
                        if(ar.userObj != null) {
                            try{
                                mIImsCallSessionListener.callSessionHoldFailed((IImsCallSession)ar.userObj,
                                        new ImsReasonInfo(ImsReasonInfo.CODE_UNSPECIFIED, 0,"Hold Failed"));
                            } catch(RemoteException e){
                                e.printStackTrace();
                            }
                        }
                    }
                    break;
                case ACTION_COMPLETE_RESUME:
                    if (ar != null && ar.exception != null) {
                        Log.w(TAG,"handleMessage->ACTION_COMPLETE_RESUME error!");
                        if(ar.userObj != null) {
                            try{
                                mIImsCallSessionListener.callSessionResumeFailed((IImsCallSession)ar.userObj,
                                        new ImsReasonInfo(ImsReasonInfo.CODE_UNSPECIFIED, 0,"Resume Failed"));
                            } catch(RemoteException e){
                                e.printStackTrace();
                            }
                        }
                    }
                    break;
                case ACTION_COMPLETE_ACCEPT:
                    if (ar != null && ar.exception != null) {
                        Log.w(TAG,"handleMessage->ACTION_COMPLETE_ACCEPT error!");
                    }
                    break;
                case ACTION_COMPLETE_HANGUP:
                    if (ar != null && ar.exception != null) {
                        Log.w(TAG,"handleMessage->ACTION_COMPLETE_HANGUP error!");
                    }
                    mImsServiceCallTracker.pollCallsWhenSafe();
                    break;
                case ACTION_COMPLETE_REJECT:
                    if (ar != null && ar.exception != null) {
                        Log.w(TAG,"handleMessage->ACTION_COMPLETE_REJECT error!");
                        if(ar.userObj != null) {
                            try{
                                mIImsCallSessionListener.callSessionStartFailed((IImsCallSession)ar.userObj,
                                        new ImsReasonInfo(ImsReasonInfo.CODE_UNSPECIFIED, 0,"Reject Failed"));
                            } catch(RemoteException e){
                                e.printStackTrace();
                            }
                        }
                    }
                    break;
                case ACTION_COMPLETE_DEFLECT:
                    if (ar != null && ar.exception != null) {
                        Log.w(TAG,"handleMessage->ACTION_COMPLETE_DEFLECT error!");

                    }
                    break;
                case ACTION_COMPLETE_MERGE:
                    if (ar != null && ar.exception != null) {
                        Log.w(TAG,"handleMessage->ACTION_COMPLETE_MERGE error!");
                        synchronized(mConferenceLock){
                            mConferenceHost = false;
                            mImsConferenceState = null;
                        }
                        if(ar.userObj != null) {
                            try{
                                mIImsCallSessionListener.callSessionMergeFailed((IImsCallSession)ar.userObj,
                                        new ImsReasonInfo(ImsReasonInfo.CODE_UNSPECIFIED, 0,
                                                "Merge Failed"));
                            } catch(RemoteException e){
                                e.printStackTrace();
                            }
                        }
                    }
                    break;
                case ACTION_COMPLETE_CONFERENCE:
                    if(ar != null){
                        Log.i(TAG,"ACTION_COMPLETE_CONFERENCE->ar:"+ar+" ar.exception:"+ar.exception
                                +"  ar.userObj:"+ar.userObj);
                    }
                    if (ar != null && ar.exception != null) {
                        if(ar.userObj != null) {
                            try{
                                mIImsCallSessionListener.callSessionStartFailed((IImsCallSession)mImsCallSessionImpl,
                                        new ImsReasonInfo(ImsReasonInfo.CODE_UNSPECIFIED, 0,
                                                "Dial Conference Failed"));
                            } catch(RemoteException e){
                                e.printStackTrace();
                            }
                        }
                    }
                    break;
                case ACTION_COMPLETE_ADD_PARTICIPANT:
                    try{
                        if(ar.userObj != null) {
                            if (ar != null && ar.exception != null) {
                                Log.w(TAG,"handleMessage->ACTION_COMPLETE_ADD_PARTICIPANT error!");
                                mIImsCallSessionListener.callSessionInviteParticipantsRequestFailed(
                                        (IImsCallSession)mImsCallSessionImpl,new ImsReasonInfo(
                                                ImsReasonInfo.CODE_UNSPECIFIED, 0,
                                                "Dial Conference Failed"));
                            } else {
                                mIImsCallSessionListener.callSessionInviteParticipantsRequestDelivered(
                                        (IImsCallSession)ar.userObj);
                            }
                        }
                    } catch(RemoteException e){
                        e.printStackTrace();
                    }
                    break;
                case ACTION_COMPLETE_RINGBACK_TONE:
                    if (ar != null && ar.exception != null) {
                        Log.w(TAG,"handleMessage->ACTION_COMPLETE_RINGBACK_TONE error!");

                    }
                    break;
                case ACTION_COMPLETE_REMOVE_PARTICIPANT:
                    if (ar != null && ar.exception != null) {
                        Log.w(TAG,"handleMessage->ACTION_COMPLETE_REMOVE_PARTICIPANT error!");

                    }
                    break;
                case ACTION_COMPLETE_GET_CALL_FAIL_CAUSE:
                    if(ar != null && ar.exception != null){
                        Log.w(TAG,"handleMessage->ACTION_COMPLETE_GET_CALL_FAIL_CAUSE error!");
                    }else{
                        LastCallFailCause failCause = (LastCallFailCause)ar.result;
                        mDisconnCause = failCause.causeCode;
                        // SPRD: add for bug541710
                        if (mDisconnCause == VTManagerUtils.VODEO_CALL_FDN_BLOCKED) {
                            VTManagerUtils.showVideoCallFailToast(mContext, mDisconnCause);
                        }
                    }
                    break;
                default:
                    Log.w(TAG,"handleMessage->unsupport message:"+msg.what);
                    break;
            }
        }
    }

    /**
     * Closes the object. This object is not usable after being closed.
     */
    @Override
    public void close(){
        Log.i(TAG, "session close!");
        mState = ImsCallSession.State.INVALID;
        mImsCallProfile = null;
        mLocalCallProfile = null;
        mRemoteCallProfile = null;
        mImsDriverCall = null;
        mCallee = null;
        mImsVideoCallProvider = null;
    }

    /**
     * Gets the call ID of the session.
     *
     * @return the call ID
     */
    @Override
    public String getCallId(){
        if(mImsDriverCall != null){
            return Integer.toString(mImsDriverCall.index);
        }
        return "0";
    }

    /**
     * Gets the call profile that this session is associated with
     *
     * @return the call profile that this session is associated with
     */
    @Override
    public ImsCallProfile getCallProfile(){
        return mImsCallProfile;
    }

    /**
     * Gets the local call profile that this session is associated with
     *
     * @return the local call profile that this session is associated with
     */
    @Override
    public ImsCallProfile getLocalCallProfile(){
        return mLocalCallProfile;
    }

    /**
     * Gets the remote call profile that this session is associated with
     *
     * @return the remote call profile that this session is associated with
     */
    @Override
    public ImsCallProfile getRemoteCallProfile(){
        return mRemoteCallProfile;
    }

    /**
     * Gets the local call profile that this session is associated with
     *
     * @return the local call profile that this session is associated with
     */
    public ImsCallProfile getLocalRequestProfile(){
        return mLocalRequestProfile;
    }

    /**
     * Gets the remote call profile that this session is associated with
     *
     * @return the remote call profile that this session is associated with
     */
    public ImsCallProfile getRemoteRequestProfile(){
        return mRemoteRequestProfile;
    }

    /**
     * Gets the value associated with the specified property of this session.
     *
     * @return the string value associated with the specified property
     */
    @Override
    public String getProperty(String name){
        if(isImsSessionInvalid()) return null;
        return mImsCallProfile.getCallExtra(name);
    }

    /**
     * Gets the session state. The value returned must be one of the states in
     * {@link ImsCallSession#State}.
     *
     * @return the session state
     */
    @Override
    public int getState(){
        if(isImsSessionInvalid()) return ImsCallSession.State.INVALID;
        return mState;
    }

    /**
     * Checks if the session is in a call.
     *
     * @return true if the session is in a call
     */
    @Override
    public boolean isInCall(){
        if(isImsSessionInvalid() || mImsDriverCall == null){
            return false;
        } else if(mImsDriverCall.state != ImsDriverCall.State.DISCONNECTED){
            return true;
        }
        return false;
    }

    /**
     * Sets the listener to listen to the session events. A {@link IImsCallSession}
     * can only hold one listener at a time. Subsequent calls to this method
     * override the previous listener.
     *
     * @param listener to listen to the session events of this object
     */
    @Override
    public void setListener(IImsCallSessionListener listener){
        mIImsCallSessionListener = listener;
    }

    /**
     * Mutes or unmutes the mic for the active call.
     *
     * @param muted true if the call is muted, false otherwise
     */
    @Override
    public void setMute(boolean muted){
        Log.w(TAG, "setMute->muted state: "+muted);
        mDesiredMute = muted;
        mCi.setMute(mDesiredMute, null);
    }

    /**
     * Initiates an IMS call with the specified target and call profile.
     * The session listener is called back upon defined session events.
     * The method is only valid to call when the session state is in
     * {@link ImsCallSession#State#IDLE}.
     *
     * @param callee dialed string to make the call to
     * @param profile call profile to make the call with the specified service type,
     *      call type and media information
     * @see Listener#callSessionStarted, Listener#callSessionStartFailed
     */
    @Override
    public void start(String callee, ImsCallProfile profile){
        if(isImsSessionInvalid()){
            Log.w(TAG, "start-> ImsSessionInvalid!");
            return;
        }
        mImsCallProfile.mCallType = profile.mCallType;
        mImsCallProfile.mMediaProfile = profile.mMediaProfile;
        mState = ImsCallSession.State.INITIATED;
        mCallee = callee;
        int clir = profile.getCallExtraInt(ImsCallProfile.EXTRA_OIR);
        if(mImsCallProfile.mCallType == ImsCallProfile.CALL_TYPE_VT
                || mImsCallProfile.mCallType == ImsCallProfile.CALL_TYPE_VIDEO_N_VOICE){
            mCi.dialVP(mCallee,null,0,mHandler.obtainMessage(ACTION_COMPLETE_DIAL,this));
        } else {
            mCi.dial(mCallee,clir,null,mHandler.obtainMessage(ACTION_COMPLETE_DIAL,this));
        }
    }

    /**
     * Initiates an IMS call with the specified participants and call profile.
     * The session listener is called back upon defined session events.
     * The method is only valid to call when the session state is in
     * {@link ImsCallSession#State#IDLE}.
     *
     * @param participants participant list to initiate an IMS conference call
     * @param profile call profile to make the call with the specified service type,
     *      call type and media information
     * @see Listener#callSessionStarted, Listener#callSessionStartFailed
     */
    @Override
    public void startConference(String[] participants, ImsCallProfile profile){
        if(isImsSessionInvalid() || participants == null){
            Log.w(TAG, "startConference-> participants:"+participants);
            return;
        }
        synchronized(mConferenceLock){
            mConferenceHost = true;
            mImsConferenceState = new ImsConferenceState();
        }
        StringBuilder participantList = new StringBuilder();
        for(int i=0;i<participants.length;i++){
            if(i != (participants.length-1)){
                participantList.append(participants[i]+",");
            } else {
                participantList.append(participants[i]);
            }
        }
        Log.d(TAG, "startConference-> participantList:"+participantList.toString());
        mCi.requestInitialGroupCall(participantList.toString(),
                mHandler.obtainMessage(ACTION_COMPLETE_CONFERENCE));
    }

    /**
     * Accepts an incoming call or session update.
     *
     * @param callType call type specified in {@link ImsCallProfile} to be answered
     * @param profile stream media profile {@link ImsStreamMediaProfile} to be answered
     * @see Listener#callSessionStarted
     */
    @Override
    public void accept(int callType, ImsStreamMediaProfile profile){
        if(isImsSessionInvalid()){
            Log.w(TAG, "accept-> ImsSessionInvalid!");
            return;
        }
        Message message = new Message();
        message.arg1 = Integer.parseInt(getCallId());
        /*SPRD:bug523375 add voice accept video call @{*/
        if(callType == ImsCallProfile.CALL_TYPE_VOICE && (mImsCallProfile.mCallType == ImsCallProfile.CALL_TYPE_VT)){
            Log.i(TAG, "voice accept video call!");
            mCi.requestVolteCallFallBackToVoice(message);
        }/*@}*/

        mImsCallProfile.mMediaProfile = profile;

        if(profile.mVideoQuality != ImsStreamMediaProfile.VIDEO_QUALITY_NONE ){
            mCi.acceptCall(mHandler.obtainMessage(ACTION_COMPLETE_ACCEPT,this));
        } else {
            mCi.acceptCall(mHandler.obtainMessage(ACTION_COMPLETE_ACCEPT,this));
        }
    }

    /**
     * Rejects an incoming call or session update.
     *
     * @param reason reason code to reject an incoming call
     * @see Listener#callSessionStartFailed
     */
    @Override
    public void reject(int reason){
        if(isImsSessionInvalid()){
            Log.w(TAG, "reject-> ImsSessionInvalid!");
            return;
        }
        mCi.rejectCall(mHandler.obtainMessage(ACTION_COMPLETE_REJECT,this));
    }

    /**
     * Terminates a call.
     *
     * @see Listener#callSessionTerminated
     */
    @Override
    public void terminate(int reason){
        if(isImsSessionInvalid()){
            Log.w(TAG, "terminate-> ImsSessionInvalid!");
            return;
        }
        mDisconnCause = reason;
        if(mImsDriverCall != null){
            mCi.hangupConnection(mImsDriverCall.index,
                    mHandler.obtainMessage(ACTION_COMPLETE_HANGUP,this));
        } else {
            Log.w(TAG, "terminate-> mImsDriverCall is null!");
        }
    }

    /**
     * Puts a call on hold. When it succeeds, {@link Listener#callSessionHeld} is called.
     *
     * @param profile stream media profile {@link ImsStreamMediaProfile} to hold the call
     * @see Listener#callSessionHeld, Listener#callSessionHoldFailed
     */
    @Override
    public void hold(ImsStreamMediaProfile profile){
        if(isImsSessionInvalid()){
            Log.w(TAG, "hold-> ImsSessionInvalid!");
            return;
        }
        if(mIsMegerAction){
            try{
                Log.w(TAG, "hold-> mIsMegerAction!");
                mIImsCallSessionListener.callSessionHoldFailed(this,
                        new ImsReasonInfo(ImsReasonInfo.CODE_UNSPECIFIED, 0,"Hold Failed"));
            } catch(RemoteException e){
                e.printStackTrace();
            }
            return;
        }
        mCi.switchWaitingOrHoldingAndActive(
                mHandler.obtainMessage(ACTION_COMPLETE_HOLD,this));
    }

    /**
     * Continues a call that's on hold. When it succeeds, {@link Listener#callSessionResumed}
     * is called.
     *
     * @param profile stream media profile {@link ImsStreamMediaProfile} to resume the call
     * @see Listener#callSessionResumed, Listener#callSessionResumeFailed
     */
    @Override
    public void resume(ImsStreamMediaProfile profile){
        if(isImsSessionInvalid()){
            Log.w(TAG, "resume-> ImsSessionInvalid!");
            return;
        }
        mCi.switchWaitingOrHoldingAndActive(
                mHandler.obtainMessage(ACTION_COMPLETE_RESUME,this));
    }

    /**
     * Merges the active & hold call. When the merge starts,
     * {@link Listener#callSessionMergeStarted} is called.
     * {@link Listener#callSessionMergeComplete} is called if the merge is successful, and
     * {@link Listener#callSessionMergeFailed} is called if the merge fails.
     *
     * @see Listener#callSessionMergeStarted, Listener#callSessionMergeComplete,
     *      Listener#callSessionMergeFailed
     */
    @Override
    public void merge(){
        // SPRD: add for bug 552691
        if(!mImsServiceCallTracker.hasConferenceSession()){
            synchronized(mConferenceLock){
                mConferenceHost = true;
                mImsConferenceState = new ImsConferenceState();
                if(mImsDriverCall != null){
                    updateImsConfrenceMember(mImsDriverCall);
                } else {
                    Log.w(TAG, "merge-> mImsDriverCall is null!");
                }
            }
        }
        mImsServiceCallTracker.onCallMergeStart(this);
        mCi.conference(mHandler.obtainMessage(ACTION_COMPLETE_MERGE,this));
    }

    /**
     * Updates the current call's properties (ex. call mode change: video upgrade / downgrade).
     *
     * @param callType call type specified in {@link ImsCallProfile} to be updated
     * @param profile stream media profile {@link ImsStreamMediaProfile} to be updated
     * @see Listener#callSessionUpdated, Listener#callSessionUpdateFailed
     */
    @Override
    public void update(int callType, ImsStreamMediaProfile profile){

    }

    /**
     * Extends this call to the conference call with the specified recipients.
     *
     * @param participants participant list to be invited to the conference call after extending the call
     * @see Listener#sessionConferenceExtened, Listener#sessionConferenceExtendFailed
     */
    @Override
    public void extendToConference(String[] participants){

    }

    /**
     * Requests the conference server to invite an additional participants to the conference.
     *
     * @param participants participant list to be invited to the conference call
     * @see Listener#sessionInviteParticipantsRequestDelivered,
     *      Listener#sessionInviteParticipantsRequestFailed
     */
    @Override
    public void inviteParticipants(String[] participants){
        if(isImsSessionInvalid() || participants == null){
            Log.w(TAG, "inviteParticipants-> participants:"+participants);
            return;
        }
        StringBuilder participantList = new StringBuilder();
        for(int i=0;i<participants.length;i++){
            if(i != (participants.length-1)){
                participantList.append(participants[i]+",");
            } else {
                participantList.append(participants[i]);
            }
        }
        Log.d(TAG, "inviteParticipants-> participantList:"+participantList.toString());
        mCi.requestAddGroupCall(participantList.toString(),
                mHandler.obtainMessage(ACTION_COMPLETE_ADD_PARTICIPANT));
    }

    /**
     * Requests the conference server to remove the specified participants from the conference.
     *
     * @param participants participant list to be removed from the conference call
     * @see Listener#sessionRemoveParticipantsRequestDelivered,
     *      Listener#sessionRemoveParticipantsRequestFailed
     */
    @Override
    public void removeParticipants(String[] participants){
        if(isImsSessionInvalid() || participants == null){
            Log.w(TAG, "removeParticipants error-> participants:"+participants);
            return;
        }
        for(String s : participants){
            removeSessionFromConference(s);
        }
    }

    /**
     * Sends a DTMF code. According to <a href="http://tools.ietf.org/html/rfc2833">RFC 2833</a>,
     * event 0 ~ 9 maps to decimal value 0 ~ 9, '*' to 10, '#' to 11, event 'A' ~ 'D' to 12 ~ 15,
     * and event flash to 16. Currently, event flash is not supported.
     *
     * @param c the DTMF to send. '0' ~ '9', 'A' ~ 'D', '*', '#' are valid inputs.
     * @param result.
     */
    @Override
    public void sendDtmf(char c, Message result){
        if(isImsSessionInvalid()){
            Log.w(TAG, "sendDtmf-> ImsSessionInvalid!");
            return;
        }
        mCi.sendDtmf(c, null);
    }

    /**
     * Start a DTMF code. According to <a href="http://tools.ietf.org/html/rfc2833">RFC 2833</a>,
     * event 0 ~ 9 maps to decimal value 0 ~ 9, '*' to 10, '#' to 11, event 'A' ~ 'D' to 12 ~ 15,
     * and event flash to 16. Currently, event flash is not supported.
     *
     * @param c the DTMF to send. '0' ~ '9', 'A' ~ 'D', '*', '#' are valid inputs.
     */
    @Override
    public void startDtmf(char c){
        if(isImsSessionInvalid()){
            Log.w(TAG, "startDtmf-> ImsSessionInvalid!");
            return;
        }
        mCi.startDtmf(c, null);
    }

    /**
     * Stop a DTMF code.
     */
    @Override
    public void stopDtmf(){
        if(isImsSessionInvalid()){
            Log.w(TAG, "stopDtmf-> ImsSessionInvalid!");
            return;
        }
        mCi.stopDtmf(null);
    }

    /**
     * Sends an USSD message.
     *
     * @param ussdMessage USSD message to send
     */
    @Override
    public void sendUssd(String ussdMessage){

    }

    /**
     * Returns a binder for the video call provider implementation contained within the IMS service
     * process. This binder is used by the VideoCallProvider subclass in Telephony which
     * intermediates between the propriety implementation and Telecomm/InCall.
     */
    @Override
    public IImsVideoCallProvider getVideoCallProvider(){
        return (IImsVideoCallProvider)(mImsVideoCallProvider.getInterface());
    }

    public ImsVideoCallProvider getImsVideoCallProvider(){
        return mImsVideoCallProvider;
    }
    /**
     * Determines if the current session is multiparty.
     * @return {@code True} if the session is multiparty.
     */
    @Override
    public boolean isMultiparty(){
        if (isImsSessionInvalid()){
            Log.w(TAG, "isMultiparty->session is invalid");
            return false;
        }
        if(mConferenceHost){
            return true;
        }
        if (mImsDriverCall == null) {
            return false;
        }
        return mImsDriverCall.isMpty && mImsDriverCall.mptyState == 2;
    }

    public void hangup(){
        if(isImsSessionInvalid()){
            Log.w(TAG, "hangup-> ImsSessionInvalid!");
            return;
        }
        if(mImsDriverCall != null){
            Log.i(TAG, "hangup-> isMultiparty state:"+mImsDriverCall.state);
            mCi.hangupConnection(mImsDriverCall.index,
                    mHandler.obtainMessage(ACTION_COMPLETE_HANGUP,this));
        } else {
            Log.w(TAG, "terminate-> mImsDriverCall is null!");
        }
    }
    public int getServiceId(){
        return mImsServiceCallTracker.getServiceId();
    }

    public void updateVideoProfile(ImsDriverCall vdc){
        Log.i(TAG,"updateVideoProfile...vdc="+vdc);
        if((vdc!= null) && (vdc.mediaDescription != null) && (vdc.mediaDescription.contains("profile"))
                && (mImsCallProfile !=null) && (mImsCallProfile.mCallType == ImsCallProfile.CALL_TYPE_VT)){
            int index = vdc.mediaDescription.indexOf("profile=");
            String qualityString = null;
            int quality = -1;
            if(index >=0){
                qualityString = vdc.mediaDescription.substring(index+8);
                if(qualityString != null){
                    try {
                        quality = Integer.parseInt(qualityString);
                    } catch(NumberFormatException e){
                        e.printStackTrace();
                    }
                }
            }
            if(getImsVideoCallProvider() != null){
                VideoProfile  profile = new VideoProfile(VideoProfile.STATE_BIDIRECTIONAL,quality);
                getImsVideoCallProvider().updateVideoQuality(profile);
            }
            Log.w(TAG,"vdc.mediaDescription: " + vdc.mediaDescription + " quality:"+quality);
        }
    }

    public boolean isForegroundCall(){
        return (mImsDriverCall != null &&
                (mImsDriverCall.state == ImsDriverCall.State.ACTIVE
                || mImsDriverCall.state == ImsDriverCall.State.DIALING
                || mImsDriverCall.state == ImsDriverCall.State.ALERTING));
    }

    public boolean isBackgroundCall(ImsDriverCall dc){
        return (dc != null && dc.state == ImsDriverCall.State.HOLDING);
    }

    public boolean isForegroundCall(ImsDriverCall dc){
        return (dc != null &&
                (dc.state == ImsDriverCall.State.ACTIVE
                || dc.state == ImsDriverCall.State.DIALING
                || dc.state == ImsDriverCall.State.ALERTING));
    }

    public boolean isBackgroundCall(){
        return (mImsDriverCall != null && mImsDriverCall.state == ImsDriverCall.State.HOLDING);
    }

    /* SPRD: add for bug 552691 @{ */
    public void setMergeState() {
        mIsMegerAction = true;
    }

    public boolean inMergeState(){
        return mIsMegerAction;
    }

    public void notifyMergeComplete() {
        mShouldNotifyMegerd = false;
        try {
            mIImsCallSessionListener.callSessionMergeComplete((IImsCallSession) this);
            mIsMegerAction = false;
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }
    /* @} */

    public boolean isConferenceHost(){
        return mConferenceHost;
    }

    public boolean inSameConference(ImsDriverCall dc){
        if(!mConferenceHost || mImsDriverCall == null || dc == null){
            Log.d(TAG, "inSameConference-> mConferenceHost:"+mConferenceHost
                    +" mImsDriverCall is null:"+(mImsDriverCall==null)
                    +" dc is null:"+(dc==null));
            return false;
        }
        Log.d(TAG, "inSameConference-> mImsDriverCall.mptyState:"+dc.mptyState);
        if(dc.mptyState == 1){
            return true;
        }
        if(mImsConferenceState != null && mImsConferenceState.mParticipants.containsKey(dc.index)){
            return true;
        }
        return false;
    }

    public boolean updateImsConfrenceMember(ImsDriverCall dc){
        synchronized(mConferenceLock){
            if(isImsSessionInvalid() || mImsConferenceState == null){
                Log.w(TAG, "updateImsConfrenceMember-> isImsSessionInvalid or mImsConferenceState is null:"
                        +(mImsConferenceState == null));
                return false;
            }
            Bundle b;
            boolean isChanged = false;
            if(mImsConferenceState.mParticipants.containsKey(Integer.toString(dc.index))){
                b = mImsConferenceState.mParticipants.get(Integer.toString(dc.index));
                String number = b.getString(ImsConferenceState.ENDPOINT);
                if(number == null || !number.equals(dc.number)){
                    b.remove(ImsConferenceState.ENDPOINT);
                    b.remove(ImsConferenceState.USER);
                    b.putString(ImsConferenceState.ENDPOINT,dc.number);
                    b.putString(ImsConferenceState.USER,dc.number);
                    isChanged = true;
                }
                String state = b.getString(ImsConferenceState.STATUS);
                if(state == null || !state.equals(dc.state)){
                    b.remove(ImsConferenceState.STATUS);
                    b.putString(ImsConferenceState.STATUS,ImsDriverCall.stateToConferenceString(dc.state));
                    isChanged = true;
                }
            } else {
                b = new Bundle();
                b.putString(IMS_CONFERENCE_ID,Integer.toString(dc.index));
                b.putString(ImsConferenceState.USER,dc.number);
                b.putString(ImsConferenceState.ENDPOINT,dc.number);
                b.putString(ImsConferenceState.STATUS,ImsDriverCall.stateToConferenceString(dc.state));
                mImsConferenceState.mParticipants.put(Integer.toString(dc.index),b);
                isChanged = true;
            }
            Log.i(TAG, "updateImsConfrenceMember->getConferenceMemberFromDc cid:"+dc.index
                    +" state:"+dc.state);
            return isChanged;
        }
    }

    public void notifyConferenceStateChange(){
        if(mImsConferenceState == null){
            Log.w(TAG, "notifyConferenceStateChange->mImsConferenceState is null!");
            return;
        }
        try {
            mIImsCallSessionListener.callSessionConferenceStateUpdated((IImsCallSession) this, mImsConferenceState);
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    public void initConferenceDc(ImsDriverCall dc){
        if (mImsDriverCall == null) {
            Log.i(TAG, "initConferenceDc-> dc is null!");
            mImsDriverCall = new ImsDriverCall(dc);
        }
    }

    public void removeInvalidSessionFromConference(Map <String, ImsDriverCall> validDriverCall){
        if(mImsConferenceState == null){
            Log.w(TAG, "removeInvalidSessionFromConference->mImsConferenceState is null!");
            return;
        }
        for (Iterator<Map.Entry<String, Bundle>> it =
                mImsConferenceState.mParticipants.entrySet().iterator(); it.hasNext();) {
            Map.Entry<String, Bundle> e = it.next();
            String index = e.getValue().getString(IMS_CONFERENCE_ID);
            if (index != null && validDriverCall.get(index) == null) {
                it.remove();
                Log.i(TAG, "removeInvalidSessionFromConference-> index:"+index);
            }
        }
    }

    public void removeSessionFromConference(String number){
        if(mImsConferenceState == null || number == null){
            Log.w(TAG, "removeSessionFromConference->mImsConferenceState is null:"+(mImsConferenceState == null)
                    +" number is null:"+(number == null));
            return;
        }
        for (Iterator<Map.Entry<String, Bundle>> it =
                mImsConferenceState.mParticipants.entrySet().iterator(); it.hasNext();) {
            Map.Entry<String, Bundle> e = it.next();
            String index = e.getValue().getString(IMS_CONFERENCE_ID);
            String address = e.getValue().getString(ImsConferenceState.USER);
            Log.d(TAG, "removeSessionFromConference-> number:"+number+" address:"+address);
            if (number.equalsIgnoreCase(address)) {
                mCi.hangupConnection(Integer.parseInt(index),
                        mHandler.obtainMessage(ACTION_COMPLETE_HANGUP,this));
            }
        }
    }

    public void disconnectForConferenceMember(){
        mState = ImsCallSession.State.TERMINATED;
        try{
            if (mIImsCallSessionListener != null){
                mIImsCallSessionListener.callSessionTerminated((IImsCallSession)this,
                        new ImsReasonInfo(mDisconnCause, 0));
            }
            Log.d(TAG, "disconnectForConferenceMember->this is 3 way conference member.");
        } catch(RemoteException e){
            e.printStackTrace();
        }
        mImsServiceCallTracker.removeConferenceMemberSession(this);
    }
}

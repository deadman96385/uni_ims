package com.spreadtrum.ims;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.List;


import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.UserHandle;
import android.os.AsyncResult;
import android.util.Log;

import com.android.internal.telephony.CommandException;
import com.android.internal.telephony.CommandsInterface;
import com.spreadtrum.ims.ImsDriverCall;
import com.android.ims.internal.ImsManagerEx;

import android.telephony.ims.ImsCallProfile;
import android.telephony.ims.ImsConferenceState;
import android.telephony.ims.ImsReasonInfo;
import android.telephony.ims.ImsCallSession;
import com.android.ims.ImsManager;
import com.android.ims.ImsServiceClass;
import com.android.ims.internal.IImsCallSession;
import android.telephony.ims.aidl.IImsCallSessionListener;
import com.android.ims.internal.IImsRegistrationListener;
import java.util.concurrent.CopyOnWriteArrayList;

import com.spreadtrum.ims.ImsService;
import com.spreadtrum.ims.vowifi.VoWifiServiceImpl;
import com.spreadtrum.ims.vowifi.VoWifiServiceImpl.CallRatState;
import vendor.sprd.hardware.radio.V1_0.CallVoLTE;
import android.telephony.PhoneNumberUtils;


import android.os.SystemProperties;

public class ImsServiceCallTracker implements ImsCallSessionImpl.Listener {
    private static final String TAG = ImsServiceCallTracker.class.getSimpleName();
    private static final boolean DBG_POLL = false;
    private static final int POLL_DELAY_MSEC = 200;

    private static final int EVENT_CALL_STATE_CHANGE             = 1;
    private static final int EVENT_GET_CURRENT_CALLS             = 2;
    private static final int EVENT_OPERATION_COMPLETE            = 3;
    private static final int EVENT_POLL_CALLS_RESULT             = 4;
    private static final int EVENT_IMS_CALL_STATE_CHANGED        = 5;
    private static final int EVENT_POLL_CURRENT_CALLS            = 6;
    private static final int EVENT_POLL_CURRENT_CALLS_RESULT     = 7;
    private static final int EVENT_GET_CURRENT_CALLS_DELAY       = 8;
    private static final int EVENT_RADIO_NOT_AVAILABLE           = 9;

    private PendingIntent mIncomingCallIntent;
    private ImsRIL mCi;
    private Context mContext;
    private int mServiceId; 
    private ImsServiceImpl mImsServiceImpl;
    private ImsHandler mHandler;
    private ImsCallSessionImpl mConferenceSession;

    private ImsService mImsService;//Add for data router
    private VoWifiServiceImpl mWifiService;//Add for data router
    private Object mUpdateLock = new Object();

    protected int mPendingOperations;
    protected boolean mNeedsPoll;
    protected Message mLastRelevantPoll;

    private Map<String, ImsCallSessionImpl> mSessionList = new HashMap<String, ImsCallSessionImpl>();
    private List<ImsCallSessionImpl> mPendingSessionList = new CopyOnWriteArrayList<ImsCallSessionImpl>();
    private List<SessionListListener> mSessionListListeners = new CopyOnWriteArrayList<SessionListListener>();
    /*SPRD: add for 605475@{*/
    private int mCurrentUserId = UserHandle.USER_OWNER;
    private BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equals(Intent.ACTION_USER_SWITCHED)) {
                mCurrentUserId = intent.getIntExtra(Intent.EXTRA_USER_HANDLE, 0);
                Log.i(TAG,"onReceive mCurrentUserId = " + mCurrentUserId);
            }
        }
    };
    /* @} */
    public ImsServiceCallTracker(Context context,ImsRIL ci, PendingIntent intent, int id, ImsServiceImpl service,
               VoWifiServiceImpl wifiService){
        mContext = context;
        mCi = ci;
        mIncomingCallIntent = intent;
        mServiceId = id;
        mImsServiceImpl = service;
        mImsService = (ImsService)context;//Add for data router
        mWifiService = wifiService;//Add for data router
        mHandler = new ImsHandler(mContext.getMainLooper());
        mCi.registerForImsCallStateChanged(mHandler, EVENT_IMS_CALL_STATE_CHANGED, null);
        mCi.registerForCallStateChanged(mHandler, EVENT_CALL_STATE_CHANGE, null);
        mCi.registerForNotAvailable(mHandler, EVENT_RADIO_NOT_AVAILABLE, null);
        /*SPRD: add for 605475@{*/
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_USER_SWITCHED);
        mContext.registerReceiver(mBroadcastReceiver, filter);
        /* @} */
    }

    private String messageToString(int message){
        switch(message){
            case EVENT_CALL_STATE_CHANGE:
                return "EVENT_CALL_STATE_CHANGE";
            case EVENT_GET_CURRENT_CALLS:
                return "EVENT_GET_CURRENT_CALLS";
            case EVENT_OPERATION_COMPLETE:
                return "EVENT_OPERATION_COMPLETE";
            case EVENT_POLL_CALLS_RESULT:
                return "EVENT_POLL_CALLS_RESULT";
            case EVENT_IMS_CALL_STATE_CHANGED:
                return "EVENT_IMS_CALL_STATE_CHANGED";
            case EVENT_POLL_CURRENT_CALLS:
                return "EVENT_POLL_CURRENT_CALLS";
            case EVENT_POLL_CURRENT_CALLS_RESULT:
                return "EVENT_POLL_CURRENT_CALLS_RESULT";
            case EVENT_GET_CURRENT_CALLS_DELAY:
                return "EVENT_GET_CURRENT_CALLS_DELAY";
            case EVENT_RADIO_NOT_AVAILABLE:
                return "EVENT_RADIO_NOT_AVAILABLE";
            default:
                return "unkwon message:"+message;
        }
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
            AsyncResult ar = (AsyncResult) msg.obj;
            Log.i(TAG, "handleMessage: "+messageToString(msg.what));
            switch (msg.what) {
                case EVENT_CALL_STATE_CHANGE:
                    removeMessages(EVENT_POLL_CURRENT_CALLS);
                    sendEmptyMessageDelayed(EVENT_POLL_CURRENT_CALLS,500);
                    break;
                case EVENT_GET_CURRENT_CALLS:
                case EVENT_IMS_CALL_STATE_CHANGED:
                case EVENT_RADIO_NOT_AVAILABLE:
                    pollCallsWhenSafe();
                    break;
                case EVENT_OPERATION_COMPLETE:
                    operationComplete();
                    break;
                case EVENT_POLL_CALLS_RESULT:
                    if (msg == mLastRelevantPoll) {
                        Log.i(TAG, "handle EVENT_POLL_CALL_RESULT: set needsPoll=F");
                        mNeedsPoll = false;
                        mLastRelevantPoll = null;
                        handlePollCalls((AsyncResult)msg.obj);
                    }
                    break;
                case EVENT_POLL_CURRENT_CALLS:
                    pollCurrentCallsWhenSafe();
                    break;
                case EVENT_POLL_CURRENT_CALLS_RESULT:
                    if (msg == mLastRelevantPoll) {
                        Log.i(TAG, "handle EVENT_POLL_CURRENT_CALLS_RESULT: set needsPoll=F");
                        mNeedsPoll = false;
                        mLastRelevantPoll = null;
                        updateCurrentCalls((AsyncResult)msg.obj);
                    }
                    break;
                case EVENT_GET_CURRENT_CALLS_DELAY:
                    pollCallsWhenSafe();
                    break;
                default:
                    break;
            }
        }
    };

    public void sendNewSessionIntent(ImsCallSessionImpl session, int index, boolean unknownSession,
            ImsDriverCall.State state, String number) {
            Bundle extras = new Bundle();
            extras.putBoolean(ImsManager.EXTRA_USSD, false);
            extras.putBoolean(ImsManager.EXTRA_IS_UNKNOWN_CALL, unknownSession);
            extras.putString(ImsManager.EXTRA_CALL_ID, Integer.toString(index));
            /*SPRD: Modify for bug586758{@*/
            Log.i (TAG,"sendNewSessionIntent-> startVolteCall"
                    + " mIsVowifiCall: " + mImsService.isVowifiCall()
                    + " mIsVolteCall: " + mImsService.isVolteCall()
                    + " isVoWifiEnabled(): " + mImsService.isVoWifiEnabled()
                    + " isVoLTEEnabled(): " + mImsService.isVoLTEEnabled());
            if (mImsService.isVoLTEEnabled() && !mImsService.isVowifiCall() && !mImsService.isVolteCall()) {
                mImsService.updateInCallState(true);
                mWifiService.updateCallRatState(CallRatState.CALL_VOLTE);
            }
            /*@}*/
            mImsServiceImpl.notifyIncomingCallSession((IImsCallSession)session,extras);
    }

    public ImsCallSessionImpl createCallSession(ImsCallProfile profile,
            IImsCallSessionListener listener) {
        ImsCallSessionImpl session = new ImsCallSessionImpl(profile, listener, mContext, mCi, this);
        session.addListener(this);
        synchronized(mPendingSessionList) {
            mPendingSessionList.add(session);
        }
        synchronized(mSessionList) {
            boolean isConference = profile.getCallExtraBoolean(ImsCallProfile.EXTRA_CONFERENCE);
            if(isConference){
                mConferenceSession = session;
            }
        }
        return session;
    }

    public ImsCallSessionImpl createCallSession(ImsCallProfile profile) {
        mHandler.removeMessages(EVENT_GET_CURRENT_CALLS_DELAY);
        ImsCallSessionImpl session = new ImsCallSessionImpl(profile, mContext, mCi, this);
        session.addListener(this);
        synchronized(mPendingSessionList) {
            mPendingSessionList.add(session);
        }
        synchronized(mSessionList) {
            boolean isConference = profile.getCallExtraBoolean(ImsCallProfile.EXTRA_CONFERENCE);
            if(isConference){
                mConferenceSession = session;
            }
        }
        return session;
    }

    public ImsCallSessionImpl getCallSession(String callId) {
        ImsCallSessionImpl session = null;
        synchronized(mSessionList) {
            session = mSessionList.get(callId);
        }
        return session;
    }


    @Override
    public void onDisconnected(ImsCallSessionImpl session){
        removeDisconncetedSessionFromList(session);
    }

    @Override
    public void onUpdate(ImsCallSessionImpl session){

    }

    public void pollCallsWhenSafe() {
        mHandler.removeMessages(EVENT_POLL_CURRENT_CALLS);
        mNeedsPoll = true;

        synchronized(mUpdateLock){
            if (checkNoOperationsPending()) {
                if (mLastRelevantPoll != null) {
                    Log.i(TAG, "pollCallsWhenSafe: mLastRelevantPoll " + mLastRelevantPoll.what);
                    if (mLastRelevantPoll.what == EVENT_POLL_CALLS_RESULT) {
                        pollCallsAfterDelay();
                        return;
                    }
                }
                mLastRelevantPoll = mHandler.obtainMessage(EVENT_POLL_CALLS_RESULT);
                mCi.getImsCurrentCalls  (mLastRelevantPoll);
            }
        }
    }

    private boolean checkNoOperationsPending() {
        if (DBG_POLL) Log.i(TAG,"checkNoOperationsPending: pendingOperations=" +
                mPendingOperations);
        return mPendingOperations == 0;
    }

    private Message
    obtainCompleteMessage(int what) {
        mPendingOperations++;
        mLastRelevantPoll = null;
        mNeedsPoll = true;

        if (DBG_POLL) Log.i(TAG,"obtainCompleteMessage: pendingOperations=" +
                mPendingOperations + ", needsPoll=" + mNeedsPoll);

        return mHandler.obtainMessage(what);
    }
    /* SPRD: add for bug525777 @{ */
    public void
    pollCallsAfterOperationComplete() {
        mNeedsPoll = true;
        mPendingOperations ++;
        operationComplete();
    }
    /* @} */
    private void
    operationComplete() {
        mPendingOperations--;

        if (DBG_POLL) Log.i(TAG,"operationComplete: pendingOperations=" +
                mPendingOperations + ", needsPoll=" + mNeedsPoll);

        if (mPendingOperations == 0 && mNeedsPoll) {
            mHandler.removeMessages(EVENT_POLL_CURRENT_CALLS);
            mLastRelevantPoll = mHandler.obtainMessage(EVENT_POLL_CALLS_RESULT);
            mCi.getImsCurrentCalls  (mLastRelevantPoll);
        } else if (mPendingOperations < 0) {
            // this should never happen
            Log.i(TAG,"GsmCallTracker.pendingOperations < 0");
            mPendingOperations = 0;
        }
    }

    protected void pollCallsAfterDelay() {
        Log.i(TAG, "pollCallsAfterDelay");
        Message msg = mHandler.obtainMessage();

        msg.what = EVENT_IMS_CALL_STATE_CHANGED;
        mHandler.sendMessageDelayed(msg, POLL_DELAY_MSEC);
    }

    protected boolean
    isCommandExceptionRadioNotAvailable(Throwable e) {
        return e != null && e instanceof CommandException
                && ((CommandException)e).getCommandError()
                == CommandException.Error.RADIO_NOT_AVAILABLE;
    }

    private ArrayList<ImsDriverCall> getImsCallList(ArrayList<CallVoLTE> calls) {
        int num = calls.size();
        ArrayList<ImsDriverCall> dcCalls = new ArrayList<ImsDriverCall>(num);
        ImsDriverCall dc;

        for (int i = 0; i < num; i++) {
            dc = new ImsDriverCall();
            /* parameter from +CLCCS:
             * [+CLCCS: <ccid1>,<dir>,<neg_status_present>,<neg_status>,<SDP_md>,
             * <cs_mode>,<ccstatus>,<mpty>,[,<numbertype>,<ton>,<number>
             * [,<priority_present>,<priority>[,<CLI_validity_present>,<CLI_validity>]]]
             * @{*/
            dc.index = calls.get(i).index;
            dc.isMT = (calls.get(i).isMT != 0);
            dc.negStatusPresent = calls.get(i).negStatusPresent;
            dc.negStatus = calls.get(i).negStatus;
            dc.mediaDescription = calls.get(i).mediaDescription;
            dc.csMode = calls.get(i).csMode;
            dc.state = ImsDriverCall.stateFromCLCCS(calls.get(i).state);
            boolean videoMediaPresent = false;
            if(dc.mediaDescription != null && dc.mediaDescription.contains("video") && !dc.mediaDescription.contains("cap:")){
                videoMediaPresent = true;
            }
            boolean videoMode = videoMediaPresent &&
                    ((dc.negStatusPresent == 1 && dc.negStatus == 1)
                            ||(dc.negStatusPresent == 1 && dc.negStatus == 2 && dc.state == ImsDriverCall.State.INCOMING)
                            ||(dc.negStatusPresent == 1 && dc.negStatus == 3)
                            ||(dc.negStatusPresent == 1 && dc.negStatus == 4)
                            ||(dc.negStatusPresent == 0 && dc.csMode == 0));
            if(videoMode || dc.csMode == 2 || dc.csMode >= 7){
                dc.isVoice = false;
            } else {
                dc.isVoice = true;
            }
            int mpty = calls.get(i).mpty;
            dc.mptyState = mpty;
            dc.isMpty = (0 != mpty);
            dc.numberType = calls.get(i).numberType;
            dc.TOA = calls.get(i).toa;
            dc.number = calls.get(i).number;
            dc.prioritypresent = calls.get(i).prioritypresent;
            dc.priority = calls.get(i).priority;
            dc.cliValidityPresent = calls.get(i).cliValidityPresent;
            dc.numberPresentation = ImsDriverCall.presentationFromCLIP(calls.get(i).numberPresentation);

            dc.als = calls.get(i).als;
            dc.isVoicePrivacy = (calls.get(i).isVoicePrivacy == 1);
            dc.name = calls.get(i).name;
            dc.namePresentation = calls.get(i).namePresentation;

            // Make sure there's a leading + on addresses with a TOA of 145
            dc.number = PhoneNumberUtils.stringFromStringAndTOA(dc.number, dc.TOA);
            Log.d(TAG,"responseCallListEx: dc=" +dc.toString());
            dcCalls.add(dc);

            if (dc.isVoicePrivacy) {
                Log.d(TAG,"InCall VoicePrivacy is enabled");
            } else {
                Log.d(TAG,"InCall VoicePrivacy is disabled");
            }
        }

        Collections.sort(dcCalls);
        return dcCalls;
    }

    private void handlePollCalls(AsyncResult ar){
        ArrayList<ImsDriverCall> imsDcList;
        Map <String, ImsDriverCall> validDriverCall = new HashMap<String, ImsDriverCall>();
        if (ar.exception == null) {
            imsDcList = getImsCallList((java.util.ArrayList<CallVoLTE>) ar.result);
        } else if (isCommandExceptionRadioNotAvailable(ar.exception)) {
            // just a dummy empty ArrayList to cause the loop
            // to hang up all the calls
            imsDcList = new ArrayList<ImsDriverCall>();
        } else {
            // Radio probably wasn't ready--try again in a bit
            // But don't keep polling if the channel is closed
            pollCallsAfterDelay();
            return;
        }

        boolean isConferenceStateChange = false;
        ImsDriverCall conferenceDc = null;

        for (int i = 0; imsDcList!= null && i < imsDcList.size(); i++) {
            ImsCallSessionImpl callSession = null;
            ImsDriverCall imsDc = imsDcList.get(i);
            if(imsDc.csMode != 0){
                Log.d(TAG, "handlePollCalls the ImsDriverCall is cs call");
                /* SPRD: add for bug850940 @{ */
                synchronized(mPendingSessionList) {
                    for(int k=0;k<mPendingSessionList.size();k++){
                        if (imsDc.state == ImsDriverCall.State.DIALING || imsDc.state ==ImsDriverCall.State.ALERTING
                                || (!imsDc.isMT && imsDc.state ==ImsDriverCall.State.ACTIVE)) {
                            ImsCallSessionImpl pendingCallSession = mPendingSessionList.get(i);
                            if(pendingCallSession != null
                                    && pendingCallSession.getIsPendingTerminate()){
                                Log.d(TAG, "handlePollCalls disconnect isPendingTerminate Session, pendingCallSession:" + pendingCallSession);
                                pendingCallSession.terminatePendingCall(ImsReasonInfo.CODE_USER_TERMINATED, imsDc.index);
                                break;
                            }
                        }
                    }
                }
                /* @} */
                continue;
            }
            synchronized(mSessionList) {
                callSession = mSessionList.get(Integer.toString(imsDc.index));
                Log.d(TAG, "Find existing Session, index:"+imsDc.index +" callSession is null:"+(callSession== null));
            }

            if(callSession == null){
                synchronized(mPendingSessionList) {
                    int index = -1;
                    for(int j=0;j<mPendingSessionList.size();j++){
                        if (imsDc.state == ImsDriverCall.State.DIALING || imsDc.state ==ImsDriverCall.State.ALERTING
                                || (!imsDc.isMT && imsDc.state ==ImsDriverCall.State.ACTIVE) && !isConferenceMember(imsDc)) {
                            ImsCallSessionImpl session = mPendingSessionList.get(j);
                            if(session.getState() == ImsCallSession.State.INVALID){//SPRD: add for bug663110
                                Log.d(TAG, "PendingSession found session is INVALID remove");
                                continue;
                            }
                            Log.d(TAG, "PendingSession found, index:"+imsDc.index+" session:" + session);
                            addSessionToList(Integer.valueOf(imsDc.index), session);
                            callSession = session;
                            if(session.isConferenceHost()){
                                Log.d(TAG, "mConferenceSession found");
                                mConferenceSession = session;
                                mConferenceSession.initConferenceDc(imsDc);
                                isConferenceStateChange = mConferenceSession.updateImsConfrenceMember(imsDc);
                            }
                            index = j;
                            break;
                        }
                    }
                    if(index != -1){
                        Log.d(TAG, "PendingSession remove, index:"+imsDc.index);
                        mPendingSessionList.remove(index);
                    }
                }
            }

            if (callSession != null){
                // This is a existing call
                if(!callSession.isConferenceHost()){
                    callSession.updateFromDc(imsDc);
                    Log.d(TAG, "handlePollCalls known session->imsDc.isMpty:"+imsDc.isMpty
                            +" mConferenceSession is null:"+(mConferenceSession == null));
                    if(imsDc.isMpty && mConferenceSession != null
                            && mConferenceSession.inSameConference(imsDc)){
                        Log.d(TAG, "mConferenceSession member found");
                        isConferenceStateChange = mConferenceSession.updateImsConfrenceMember(imsDc);
                    }
                } else {
                    conferenceDc = imsDc;
                    Log.d(TAG, "handlePollCalls->updateconferenceDc:"+imsDc.index);
                }
            } else {
                Log.d(TAG, "handlePollCalls unknown session->imsDc.isMpty:"+imsDc.isMpty
                        +" mConferenceSession is null:"+(mConferenceSession == null));
                if(imsDc.isMpty && mConferenceSession != null
                        && mConferenceSession.inSameConference(imsDc)){
                    Log.d(TAG, "mConferenceSession member found");
                    isConferenceStateChange = mConferenceSession.updateImsConfrenceMember(imsDc);
                } else {
                    boolean shouldNotify = false;
                    if (imsDc.state == ImsDriverCall.State.DISCONNECTED) {
                        //ignore unknown disconnected call
                        continue;
                    }
                    callSession = new ImsCallSessionImpl(imsDc, mContext, mCi, this);
                    callSession.addListener(this);
                    addSessionToList(Integer.valueOf(imsDc.index), callSession);
                    if (imsDc.isMT) {
                        Log.d(TAG, "This is a MT Call.");
                        sendNewSessionIntent(callSession, imsDc.index, false, imsDc.state, imsDc.number);
                    } else if (imsDc.isMpty) {
                        Log.d(TAG, "This is a invalid conference session!");
                        shouldNotify = false;
                    } else if (imsDc.state != ImsDriverCall.State.DISCONNECTED) {
                        Log.d(TAG, "unknown ims session:" + imsDc.state);
                        shouldNotify = true;
                    }
                    if (shouldNotify) {
                        sendNewSessionIntent(callSession, imsDc.index, true, imsDc.state, imsDc.number);
                    }
                }
            }

            if (imsDc.state != ImsDriverCall.State.DISCONNECTED ) {
                validDriverCall.put(Integer.toString(imsDc.index), imsDc);
            }
        }
        if(mConferenceSession != null){
            boolean isConNumberVideoCall = false;
            for (Iterator<Map.Entry<String, ImsDriverCall>> it =
                validDriverCall.entrySet().iterator(); it.hasNext();) {
                Map.Entry<String, ImsDriverCall> e = it.next();
                ImsDriverCall imsdc = e.getValue();
                if(mConferenceSession.inSameConference(imsdc) && imsdc.isVideoCall()){
                    isConNumberVideoCall = true;
                    break;
                }
            }
            if(mConferenceSession.getImsVideoCallProvider() != null && mConferenceSession.getCallProfile() != null) {
                if (mConferenceSession.getImsVideoCallProvider().isVideoCall(mConferenceSession.getCallProfile().mCallType)
                        && !isConNumberVideoCall) {
                    mConferenceSession.getCallProfile().mCallType = ImsCallProfile.CALL_TYPE_VOICE_N_VIDEO;
                } else if (!mConferenceSession.getImsVideoCallProvider().isVideoCall(mConferenceSession.getCallProfile().mCallType)
                        && isConNumberVideoCall) {
                    mConferenceSession.getCallProfile().mCallType = ImsCallProfile.CALL_TYPE_VT;
                }
            }
            if(conferenceDc != null){
                isConferenceStateChange = mConferenceSession.updateImsConfrenceMember(conferenceDc)  || isConferenceStateChange;
                isConferenceStateChange = mConferenceSession.updateFromDc(conferenceDc) || isConferenceStateChange;
            }
            mConferenceSession.removeInvalidSessionFromConference(validDriverCall);
        }
        removeInvalidSessionFromList(validDriverCall);
        if(isConferenceStateChange && (mConferenceSession != null)){
            mConferenceSession.notifyConferenceStateChange();
            Log.d(TAG, "handlePollCalls->isConferenceStateChange:"+isConferenceStateChange);
        }
        if(mSessionList.isEmpty()){
            mImsServiceImpl.onImsCallEnd();
        }
    }

    protected void pollCurrentCallsWhenSafe() {
        mNeedsPoll = true;

        if (checkNoOperationsPending()) {
            mLastRelevantPoll = mHandler.obtainMessage(EVENT_POLL_CURRENT_CALLS_RESULT);
            mCi.getImsCurrentCalls  (mLastRelevantPoll);
        }
    }

    public void updateCurrentCalls(AsyncResult ar){
        ArrayList<ImsDriverCall> imsDcList;
        Map <String, ImsDriverCall> validDriverCall = new HashMap<String, ImsDriverCall>();
        if (ar.exception == null) {
            imsDcList = getImsCallList((java.util.ArrayList<CallVoLTE>) ar.result);
        } else if (isCommandExceptionRadioNotAvailable(ar.exception)) {
            // just a dummy empty ArrayList to cause the loop
            // to hang up all the calls
            imsDcList = new ArrayList<ImsDriverCall>();
        } else {
            // Radio probably wasn't ready--try again in a bit
            // But don't keep polling if the channel is closed
            Log.w(TAG, "updateCurrentCalls error: " + ar.exception);
            return;
        }
        for (int i = 0; imsDcList!= null && i < imsDcList.size(); i++) {
            ImsCallSessionImpl callSession = null;
            ImsDriverCall imsDc = imsDcList.get(i);
            synchronized(mSessionList) {
                callSession = mSessionList.get(Integer.toString(imsDc.index));
                if(callSession != null) {
                    Log.d(TAG, "Find existing Session, index:" + imsDc.index);
                }
            }
            if (callSession == null && mPendingSessionList != null) {
                synchronized(mPendingSessionList) {
                    int index = -1;
                    for(int j=0;j<mPendingSessionList.size();j++){
                        ImsCallSessionImpl session = mPendingSessionList.get(j);
                        if (imsDc.state == ImsDriverCall.State.DIALING) {
                            Log.d(TAG, "PendingSession found, index:"+imsDc.index+" session:" + session);
                            addSessionToList(Integer.valueOf(imsDc.index), session);
                            callSession = session;
                            index = j;
                            break;
                        }
                    }
                    if(index != -1){
                        Log.d(TAG, "PendingSession remove, index:"+imsDc.index);
                        mPendingSessionList.remove(index);
                    }
                }
            }
            if (callSession != null){
                // This is a existing call
                callSession.updateFromDc(imsDc);
            }

            if (imsDc.state != ImsDriverCall.State.DISCONNECTED ) {
                validDriverCall.put(Integer.toString(imsDc.index), imsDc);
            }
        }
        removeInvalidSessionFromList(validDriverCall);
        /*if(mSessionList.isEmpty()){
            mImsServiceImpl.onImsCallEnd();
        }*/
    }

    public void addSessionToList(Integer id, ImsCallSessionImpl session) {
        synchronized(mSessionList) {
            mSessionList.put(id.toString(), session);
        }
        notifySessionConnected(session);
    }

    public void removeInvalidSessionFromList(Map <String, ImsDriverCall> validDriverCall) {
        /* SPRD: add for bug525777 @{ */
        synchronized(mPendingSessionList) {
            if(mPendingSessionList.size() >0){
                for(int i=0;i<mPendingSessionList.size();i++){
                    ImsCallSessionImpl session = mPendingSessionList.get(i);
                    Log.d(TAG, "remove Invalid Pending Session, index:"+i+" session:" + session);
                    session.notifySessionDisconnected();
                    notifySessionDisonnected(session);
                }
                mPendingSessionList.clear();
            }
        }/* @} */
        synchronized(mSessionList) {
            int changeSessonId = -1;
            for (Iterator<Map.Entry<String, ImsCallSessionImpl>> it =
                    mSessionList.entrySet().iterator(); it.hasNext();) {
                Map.Entry<String, ImsCallSessionImpl> e = it.next();
                if (validDriverCall.get(e.getValue().getCallId()) == null) {
                    ImsCallSessionImpl session = e.getValue();
                    if(mConferenceSession == session){
                        //check conference disconnected
                        if(mConferenceSession.isConferenceAlive()){
                            Log.d(TAG, "removeInvalidSessionFromList->conference call does not disconnected.host is disconnected id = "+mConferenceSession.getCallId());
                            changeSessonId = Integer.parseInt(mConferenceSession.getCallId());
                            continue;
                        }
                        mConferenceSession = null;
                        Log.d(TAG, "removeInvalidSessionFromList->clear conference call.");
                    }
                    it.remove();
                    Log.d(TAG, "removeInvalidSessionFromList: " + session);
                    session.notifySessionDisconnected();
                    notifySessionDisonnected(session);
                }
            }
            /*SPRD:add for bug664628 @*/
            if (changeSessonId != -1) {
                mSessionList.remove(String.valueOf(changeSessonId));
                int newHostId = mConferenceSession.getOneConferenceMember();
                Log.d(TAG, "removeInvalidSessionFromList->change mConferenceSession linked to new DriverCall. id = " + newHostId);
                if (newHostId != -1) {
                    mSessionList.put(String.valueOf(newHostId), mConferenceSession);
                    mConferenceSession.mImsDriverCall = validDriverCall.get(String.valueOf(newHostId));
                    Log.d(TAG, "removeInvalidSessionFromList->" + mConferenceSession.mImsDriverCall);
                }
            }/*@}*/
        }
    }

    public void removeDisconncetedSessionFromList(ImsCallSessionImpl session) {
        //SPRD: modify for bug525777
        synchronized(mPendingSessionList) {
            int index = -1;
            for(int i=0;i<mPendingSessionList.size();i++){
                ImsCallSessionImpl s = mPendingSessionList.get(i);
                if(s == session){
                    Log.d(TAG, "remove disconnected Pending Session, index:"+i+" session:" + session);
                    notifySessionDisonnected(s);
                    index = i;
                }
            }
            if(index != -1){
                mPendingSessionList.remove(index);
            }
        }
        synchronized(mSessionList) {
            for (Iterator<Map.Entry<String, ImsCallSessionImpl>> it
                    = mSessionList.entrySet().iterator(); it.hasNext();) {
                Map.Entry<String, ImsCallSessionImpl> e = it.next();
                if (e.getValue() == session) {
                    Log.i(TAG, "DisconncetedSession:" + session);
                    it.remove();
                    if(mConferenceSession == session){
                        mConferenceSession = null;
                        Log.d(TAG, "removeDisconncetedSessionFromList->clear conference call.");
                    }
                    notifySessionDisonnected(e.getValue());
                }
            }
        }
    }

    public interface SessionListListener {
        void onSessionConnected(ImsCallSessionImpl callSession);

        void onSessionDisonnected(ImsCallSessionImpl callSession);
    }


    public void addListener(SessionListListener listener) {
        if (listener == null) {
            Log.w(TAG,"addListener-> listener is null!");
            return;
        }
        if (!mSessionListListeners.contains(listener)) {
            mSessionListListeners.add(listener);
        } else {
            Log.w(TAG,"addListener-> listener already add!");
        }
    }


    public void removeListener(SessionListListener listener) {
        if (listener == null) {
            Log.w(TAG,"removeListener-> listener is null!");
            return;
        }
        if (mSessionListListeners.contains(listener)) {
            mSessionListListeners.remove(listener);
        } else {
            Log.w(TAG,"addListener-> listener already remove!");
        }
    }

    public void notifySessionConnected(ImsCallSessionImpl session) {
        for (SessionListListener listener : mSessionListListeners) {
            listener.onSessionConnected(session);
        }
    }

    public void notifySessionDisonnected(ImsCallSessionImpl session) {
        for (SessionListListener listener : mSessionListListeners) {
            Log.d(TAG,"notifySessionDisonnected -> listener="+listener);
            listener.onSessionDisonnected(session);
        }
    }

    public void hangupAllMultipartyCall(boolean isForeground){
        Log.d(TAG,"hangupAllMultipartyCall->isForeground:"+isForeground);
        synchronized(mSessionList) {
            for (Iterator<Map.Entry<String, ImsCallSessionImpl>> it =
                    mSessionList.entrySet().iterator(); it.hasNext();) {
                Map.Entry<String, ImsCallSessionImpl> e = it.next();
                if (e.getValue().isMultiparty()
                        && ((e.getValue().isForegroundCall() && isForeground)
                        || (e.getValue().isBackgroundCall() && !isForeground))) {
                    e.getValue().hangup();
                }
            }
        }
    }

    /* SPRD: add for bug 552691 @{ */
    private ImsCallSessionImpl mMergeHost;

    public void onCallMergeStart(ImsCallSessionImpl mergeHost) {
        synchronized (mSessionList) {
            mMergeHost = mergeHost;
            if(mConferenceSession == null){
                mConferenceSession = mergeHost;
            }
            for (Iterator<Map.Entry<String, ImsCallSessionImpl>> it = mSessionList.entrySet()
                    .iterator(); it.hasNext();) {
                Map.Entry<String, ImsCallSessionImpl> e = it.next();
                if (!e.getValue().equals(mergeHost)) {
                    e.getValue().setMergeState(true);
                }
            }
        }
    }

    public void onCallMergeComplete(ImsCallSessionImpl conferenceMember) {
        synchronized (mSessionList) {
            if (mMergeHost != null) {
                mMergeHost.notifyMergeComplete();
                if(!mMergeHost.isConferenceHost()){
                    mMergeHost.disconnectForConferenceMember();
                }
                mMergeHost = null;
            }
        }
    }
    /* SPRD: add for bug 596461 @{ */
    public void onCallMergeFailed(ImsCallSessionImpl mergeHost) {
        synchronized (mSessionList) {
            mMergeHost = null;
            if((mergeHost == mConferenceSession) && !mergeHost.isConferenceHost()){
                mConferenceSession = null;
            }

            for (Iterator<Map.Entry<String, ImsCallSessionImpl>> it = mSessionList.entrySet()
                    .iterator(); it.hasNext();) {
                Map.Entry<String, ImsCallSessionImpl> e = it.next();
                if (!e.getValue().equals(mergeHost)) {
                    e.getValue().setMergeState(false);
                }
            }
        }
    }
    /* @} */

    public void removeConferenceMemberSession(ImsCallSessionImpl conferenceMember){
        Log.d(TAG,"removeConferenceMemberSession -> conferenceMember="+conferenceMember);
        removeDisconncetedSessionFromList(conferenceMember);
    }
    /* @} */

    public boolean hasConferenceSession(){
        return mConferenceSession != null;
    }

    public boolean isConferenceSession(ImsCallSessionImpl session){
        return mConferenceSession == session;
    }
    public boolean isMegerActionHost(){
        if(mMergeHost != null){
            return mMergeHost.isMegerActionHost();
        }
        return false;
    }

    public int getServiceId(){
        return mServiceId;
    }

    //SPRD: add for bug579560
    public boolean isHasBackgroundCallAndActiveCall(){
        synchronized(mSessionList){
            boolean flagActive = false;
            boolean flagHold = false;
            for(Iterator<Map.Entry<String, ImsCallSessionImpl>> it = mSessionList.entrySet().iterator(); it.hasNext();){
                Map.Entry<String, ImsCallSessionImpl> e = it.next();

                if(e.getValue().isBackgroundCall()){
                    flagHold = true;
                }else if(e.getValue().isForegroundCall()){
                    flagActive = true;
                }
                if(flagHold == true && flagActive == true){
                    Log.d(TAG, "updateConferenceState -flagHold = true, flagActive = true");
                    return true;
                }
            }
        }
        return false;
    }

    public boolean hasRingingCall(){
        synchronized(mSessionList) {
            for (Iterator<Map.Entry<String, ImsCallSessionImpl>> it =
                    mSessionList.entrySet().iterator(); it.hasNext();) {
                Map.Entry<String, ImsCallSessionImpl> e = it.next();
                if (e.getValue().isRingingCall()) {
                    return true;
                }
            }
        }
        return false;
    }
    /*SPRD: add for 605475@{*/
    @Override
    protected void finalize() throws Throwable {
        mContext.unregisterReceiver(mBroadcastReceiver);
        super.finalize();
    }
    public int getCurrentUserId(){
        return mCurrentUserId;
    }
    /* @} */
    public boolean isSessionListEmpty(){
        boolean isEmpty;
        synchronized(mSessionList) {
            isEmpty = mSessionList.isEmpty();
        }
        return isEmpty;
    }

    /* SPRD: fix for bug837323 @{ */
    public void updateImsCallProfile(boolean wifiEnable) {
        synchronized(mSessionList) {
            for (Iterator<Map.Entry<String, ImsCallSessionImpl>> it =
                    mSessionList.entrySet().iterator(); it.hasNext();) {
                Map.Entry<String, ImsCallSessionImpl> e = it.next();
                ImsCallSessionImpl session = e.getValue();
                session.updateImsCallProfile(wifiEnable);
                Log.d(TAG, "updateImsCallProfile->session: " + session);
            }
        }
    }
    /* @} */

    public void terminateVolteCall() {
        synchronized(mSessionList) {
            for (Iterator<Map.Entry<String, ImsCallSessionImpl>> it =
                    mSessionList.entrySet().iterator(); it.hasNext();) {
                Map.Entry<String, ImsCallSessionImpl> e = it.next();
                ImsCallSessionImpl session = e.getValue();
                session.terminate(ImsReasonInfo.CODE_USER_TERMINATED);
                Log.d(TAG, "terminateVolteCall->session: " + session);
            }
        }
    }

    public boolean isDualCallActive(){
        int count = 0;
        synchronized(mSessionList) {
            for (Iterator<Map.Entry<String, ImsCallSessionImpl>> it =
                    mSessionList.entrySet().iterator(); it.hasNext();) {
                Map.Entry<String, ImsCallSessionImpl> e = it.next();
                if (e.getValue().isActiveCall() && !e.getValue().isMultiparty()) {
                    count++;
                }
            }
        }
        return count >1;
    }

    public void onVideoStateChanged(int videoState){
        if(mImsService != null){
            mImsService.onVideoStateChanged(videoState);
        }
    }

    /* SPRD: add for bug676047 @{ */
    public boolean isHasInLocalConferenceSession(){
        synchronized(mSessionList) {
            for (Iterator<Map.Entry<String, ImsCallSessionImpl>> it =
                 mSessionList.entrySet().iterator(); it.hasNext();) {
                Map.Entry<String, ImsCallSessionImpl> e = it.next();
                if (e.getValue().isInLocalConference() && e.getValue().isActiveCall()) {
                    return true;
                }
            }
        }
        return false;
    }
    /* @} */

    // SPRD Add for bug696648
    public boolean hasCall() {
        return (mSessionList.size() + mPendingSessionList.size()) > 0;
    }

    // SPRD Add for bug696648
    public boolean moreThanOnePhoneHasCall() {
        return mImsService.moreThanOnePhoneHasCall();
    }


    public boolean isAllCallsActive(){
        synchronized(mSessionList){
            for(Iterator<Map.Entry<String, ImsCallSessionImpl>> it = mSessionList.entrySet().iterator(); it.hasNext();){
                Map.Entry<String, ImsCallSessionImpl> e = it.next();
                if(!e.getValue().isActiveCall()){
                    return false;
                }
            }
        }
        return true;
    }


    public boolean isConferenceMember(ImsDriverCall dc) {
        boolean isSame = false;
        if (mConferenceSession != null && dc.isMpty) {
            isSame = mConferenceSession.inSameConference(dc);
            Log.d(TAG, "isConferenceMember->isSame: " + isSame);
        }
        return isSame;
    }
}

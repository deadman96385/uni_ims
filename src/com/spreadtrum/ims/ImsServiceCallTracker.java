package com.spreadtrum.ims;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.List;


import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.AsyncResult;
import android.util.Log;

import com.android.internal.telephony.CommandException;
import com.android.internal.telephony.CommandsInterface;
import com.spreadtrum.ims.ImsDriverCall;
import com.android.ims.internal.ImsManagerEx;

import com.android.ims.ImsCallProfile;
import com.android.ims.ImsManager;
import com.android.ims.ImsServiceClass;
import com.android.ims.internal.IImsCallSessionListener;
import com.android.ims.internal.IImsRegistrationListener;
import com.android.ims.internal.ImsCallSession;
import com.android.ims.ImsConferenceState;
import java.util.concurrent.CopyOnWriteArrayList;



public class ImsServiceCallTracker implements ImsCallSessionImpl.Listener {
    private static final String TAG = ImsServiceCallTracker.class.getSimpleName();
    private static final boolean DBG_POLL = false;
    private static final int POLL_DELAY_MSEC = 250;

    private static final int EVENT_CALL_STATE_CHANGE             = 1;
    private static final int EVENT_GET_CURRENT_CALLS             = 2;
    private static final int EVENT_OPERATION_COMPLETE            = 3;
    private static final int EVENT_POLL_CALLS_RESULT             = 4;
    private static final int EVENT_IMS_CALL_STATE_CHANGED        = 5;
    private static final int EVENT_POLL_CURRENT_CALLS            = 6;
    private static final int EVENT_POLL_CURRENT_CALLS_RESULT     = 7;

    private PendingIntent mIncomingCallIntent;
    private ImsRIL mCi;
    private Context mContext;
    private int mServiceId; 
    private ImsServiceImpl mImsServiceImpl;
    private ImsHandler mHandler;
    private ImsCallSessionImpl mConferenceSession;

    protected int mPendingOperations;
    protected boolean mNeedsPoll;
    protected Message mLastRelevantPoll;
    private Object mUpdateLock = new Object();

    private Map<String, ImsCallSessionImpl> mSessionList = new HashMap<String, ImsCallSessionImpl>();
    private List<ImsCallSessionImpl> mPendingSessionList = new CopyOnWriteArrayList<ImsCallSessionImpl>();
    private List<SessionListListener> mSessionListListeners = new CopyOnWriteArrayList<SessionListListener>();
    public ImsServiceCallTracker(Context context,ImsRIL ci, PendingIntent intent, int id, ImsServiceImpl service){
        mContext = context;
        mCi = ci;
        mIncomingCallIntent = intent;
        mServiceId = id;
        mImsServiceImpl = service;
        mHandler = new ImsHandler(mContext.getMainLooper());
        mCi.registerForImsCallStateChanged(mHandler, EVENT_IMS_CALL_STATE_CHANGED, null);
        mCi.registerForCallStateChanged(mHandler, EVENT_CALL_STATE_CHANGE, null);
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
            Log.i(TAG, "handleMessage: "+msg.what);
            switch (msg.what) {
                case EVENT_CALL_STATE_CHANGE:
                    removeMessages(EVENT_POLL_CURRENT_CALLS);
                    sendEmptyMessageDelayed(EVENT_POLL_CURRENT_CALLS,500);
                    break;
                case EVENT_GET_CURRENT_CALLS:
                case EVENT_IMS_CALL_STATE_CHANGED:
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
                default:
                    break;
            }
        }
    };

    public void sendNewSessionIntent(ImsCallSessionImpl session, int index, boolean unknownSession,
            ImsDriverCall.State state, String number) {
        try {
            Intent intent = new Intent();
            intent.putExtra(ImsManager.EXTRA_CALL_ID, Integer.toString(index));
            intent.putExtra(ImsManager.EXTRA_USSD, false);
            intent.putExtra(ImsManager.EXTRA_SERVICE_ID, mServiceId);
            intent.putExtra(ImsManager.EXTRA_IS_UNKNOWN_CALL, unknownSession);
            mIncomingCallIntent.send(mContext, ImsManager.INCOMING_CALL_RESULT_CODE,intent);
        } catch (PendingIntent.CanceledException e) {
            Log.e(TAG, "PendingIntent Canceled " + e);
        }
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

    protected void pollCallsWhenSafe() {
        mHandler.removeMessages(EVENT_POLL_CURRENT_CALLS);
        mNeedsPoll = true;

        synchronized(mUpdateLock){
            if (checkNoOperationsPending()) {
                if (mLastRelevantPoll != null) {
                    Log.i(TAG, "pollCallsWhenSafe: mLastRelevantPoll " + mLastRelevantPoll.what);
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

    private void handlePollCalls(AsyncResult ar){
        ArrayList<ImsDriverCall> imsDcList;
        Map <String, ImsDriverCall> validDriverCall = new HashMap<String, ImsDriverCall>();
        if (ar.exception == null) {
            imsDcList = (ArrayList<ImsDriverCall>)ar.result;
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
            synchronized(mSessionList) {
                callSession = mSessionList.get(Integer.toString(imsDc.index));
                Log.d(TAG, "Find existing Session, index:"+imsDc.index +" callSession is null:"+(callSession== null));
            }

            if(callSession == null){
                synchronized(mPendingSessionList) {
                    int index = -1;
                    for(int j=0;j<mPendingSessionList.size();j++){
                        ImsCallSessionImpl session = mPendingSessionList.get(j);
                        if (imsDc.state == ImsDriverCall.State.DIALING || imsDc.state ==ImsDriverCall.State.ALERTING) {
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
        removeInvalidSessionFromList(validDriverCall);
        if(mConferenceSession != null){
            if(conferenceDc != null){
                isConferenceStateChange = mConferenceSession.updateImsConfrenceMember(conferenceDc)  || isConferenceStateChange;
                isConferenceStateChange = mConferenceSession.updateFromDc(conferenceDc) || isConferenceStateChange;
            }
            mConferenceSession.removeInvalidSessionFromConference(validDriverCall);
        }
        if(isConferenceStateChange && mConferenceSession != null){
            mConferenceSession.notifyConferenceStateChange();
            Log.d(TAG, "handlePollCalls->isConferenceStateChange:"+isConferenceStateChange);
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
            imsDcList = (ArrayList<ImsDriverCall>)ar.result;
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
                Log.d(TAG, "Find existing Session, index:"+imsDc.index);
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
            for (Iterator<Map.Entry<String, ImsCallSessionImpl>> it =
                    mSessionList.entrySet().iterator(); it.hasNext();) {
                Map.Entry<String, ImsCallSessionImpl> e = it.next();
                if (validDriverCall.get(e.getValue().getCallId()) == null) {
                    it.remove();
                    ImsCallSessionImpl session = e.getValue();
                    if(mConferenceSession == session){
                        mConferenceSession = null;
                        Log.d(TAG, "removeInvalidSessionFromList->clear conference call.");
                    }
                    Log.d(TAG, "removeInvalidSessionFromList: " + session);
                    session.notifySessionDisconnected();
                    notifySessionDisonnected(session);
                }
            }
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
                if (e.getValue().isBackgroundCall()) {
                    e.getValue().setMergeState();
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

    public int getServiceId(){
        return mServiceId;
    }
}

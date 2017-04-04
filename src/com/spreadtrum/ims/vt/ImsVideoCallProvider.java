package com.spreadtrum.ims.vt;

import android.content.Context;
import android.net.Uri;
import android.os.Handler;
import android.os.UserHandle;
import android.telecom.VideoProfile;
import android.view.Surface;
import android.util.Log;
import com.android.internal.telephony.Connection;
import com.spreadtrum.ims.ImsCallSessionImpl;
import com.spreadtrum.ims.ImsCmccHelper;
import com.spreadtrum.ims.ImsRIL;
import com.spreadtrum.ims.ImsServiceCallTracker;
import com.android.ims.ImsCallProfile;
import com.android.internal.telephony.CommandsInterface;
import android.app.AlertDialog;
import android.telecom.VideoProfile.CameraCapabilities;
import android.telecom.Connection.VideoProvider;
import android.os.PowerManager;
import android.os.Message;
import android.widget.Toast;
import com.spreadtrum.ims.R;
import com.spreadtrum.ims.ImsDriverCall;
import com.android.ims.internal.ImsCallSession;
import android.os.SystemClock;
import android.app.KeyguardManager;
import android.os.AsyncResult;
import android.telephony.VoLteServiceState;
import android.telephony.TelephonyManager;

public class ImsVideoCallProvider extends com.android.ims.internal.ImsVideoCallProvider {
    private static final String TAG = ImsVideoCallProvider.class.getSimpleName();
    private VTManagerProxy mVTManagerProxy;
    private Handler mHandler;
    private ImsCallProfile mNegotiatedCallProfile = new ImsCallProfile();
    private VideoProfile mLocalRequestProfile;
    private ImsCallSessionImpl mImsCallSessionImpl;
    private Context mContext;
    private ImsRIL mCi;
    private AlertDialog mVolteMediaUpdateDialog;
    private AlertDialog mVolteMediaDegradeDialog;
    private ImsCallSessionImplListner mImsCallSessionImplListner;
    private PowerManager.WakeLock mPartialWakeLock;
    private Message mCallIdMessage;
    private boolean mIsVideo;//SPRD:add for bug563112
    public boolean mIsVoiceRingTone = false;//SPRD: add for bug677255
    public boolean mIsOrigionVideo = false;

    /** volte media event code. */
    private static final int EVENT_VOLTE_CALL_REMOTE_REQUEST_MEDIA_CHANGED_TIMEOUT = 500;
    private static final int EVENT_SRVCC_STATE_CHANGED = 100;
    private static final int EVENT_VOLTE_CALL_REQUEST_MEDIA_CHANGED_TIMEOUT = 101;//SPRD: add for bug674565

    private Handler mVTHandler = new Handler(){
        @Override
        public void handleMessage(Message msg) {
            log("handleMessage msg = " + msg.what);
            AsyncResult ar;
            switch (msg.what) {
                case EVENT_VOLTE_CALL_REMOTE_REQUEST_MEDIA_CHANGED_TIMEOUT:
                    ImsRIL mCi = (ImsRIL)msg.obj;
                    mCi.responseVolteCallMediaChange(false,Integer.parseInt(mImsCallSessionImpl.getCallId()),null);
                    receiveSessionModifyResponse(android.telecom.Connection.VideoProvider.SESSION_MODIFY_REQUEST_INVALID,
                                null,null); //SPRD:add for bug610607
                    break;
                 /* SPRD:add for bug563112 @{ */
                 case EVENT_SRVCC_STATE_CHANGED:
                     ar = (AsyncResult)msg.obj;
                     if (ar.exception == null) {
                         handleSrvccStateChanged((int[]) ar.result);
                     } else {
                         log("Srvcc exception: " + ar.exception);
                     }
                    break;
                /* @} */
                case EVENT_VOLTE_CALL_REQUEST_MEDIA_CHANGED_TIMEOUT:{
                    log("handle message EVENT_VOLTE_CALL_REQUEST_MEDIA_CHANGED_TIMEOUT");
                    if (mImsCallSessionImpl != null
                            && mImsCallSessionImpl.mImsCallProfile != null
                            && mImsCallSessionImpl.mImsCallProfile.mCallType != ImsCallProfile.CALL_TYPE_VT
                            && mContext != null) {

                        ImsRIL imsCi = (ImsRIL)msg.obj;
                        String[] cmd = new String[1];
                        cmd[0] = "AT+CCMMD=" + Integer.parseInt(mImsCallSessionImpl.getCallId()) + ",5";
                        if(imsCi != null){
                            imsCi.invokeOemRilRequestStrings(cmd, null);
                            Log.d(TAG, "EVENT_VOLTE_CALL_REQUEST_MEDIA_CHANGED_TIMEOUT cmd[0]"+ cmd[0]);
                        }
                    }
                    mImsCallSessionImpl.getLocalRequestProfile().mCallType = ImsCallProfile.CALL_TYPE_VOICE_N_VIDEO;
                    receiveSessionModifyResponse(android.telecom.Connection.VideoProvider.SESSION_MODIFY_REQUEST_INVALID,
                            null,null);
                }
                break;
                default:
                    break;
            }
        }
    };

    /* SPRD:add for bug563112 @{ */
    private void handleSrvccStateChanged(int[] ret) {
        log("handleSrvccStateChanged");
        if (ret != null && ret.length != 0) {
            int state = ret[0];
            log("handleSrvccStateChanged..state:"+state+"   mIsVideo="+mIsVideo+"   mContext:"+mContext);
            switch(state) {
                case VoLteServiceState.HANDOVER_COMPLETED:
                    if(mIsVideo && (mContext != null)){
                       mIsVideo = false;
                       Toast.makeText(mContext,mContext.getResources().getString(R.string.videophone_fallback_title),Toast.LENGTH_LONG).show();
                    }
                    receiveSessionModifyResponse(android.telecom.Connection.VideoProvider.SESSION_MODIFY_REQUEST_INVALID,
                                null,null); //SPRD:add for bug610607
                    break;
                default:
                    return;
            }
        }
    }
    /* @} */

    public ImsVideoCallProvider(ImsCallSessionImpl imsCallSessionImpl,ImsRIL ci,Context context) {
        super();
        mVTManagerProxy = VTManagerProxy.getInstance();
        mHandler = mVTManagerProxy.mHandler;
        mImsCallSessionImpl = imsCallSessionImpl;
        mContext = context;
        mCi = ci;
        mImsCallSessionImplListner = new ImsCallSessionImplListner();
        mImsCallSessionImpl.addListener(mImsCallSessionImplListner);
        createWakeLock(mContext.getApplicationContext());
        if(isVideoCall(mImsCallSessionImpl.mImsCallProfile.mCallType)){
           onVTConnectionEstablished(mImsCallSessionImpl);
            log("ImsVideoCallProvider mIsOrigionVideo = true");
            mIsOrigionVideo = true;
            mIsVoiceRingTone = false;
        }
        mNegotiatedCallProfile.mCallType = mImsCallSessionImpl.mImsCallProfile.mCallType;
        mNegotiatedCallProfile.mMediaProfile.mAudioQuality =  mImsCallSessionImpl.mImsCallProfile.mMediaProfile.mAudioQuality;
        mNegotiatedCallProfile.mMediaProfile.mAudioDirection =  mImsCallSessionImpl.mImsCallProfile.mMediaProfile.mAudioDirection;
        mNegotiatedCallProfile.mMediaProfile.mVideoQuality =  mImsCallSessionImpl.mImsCallProfile.mMediaProfile.mVideoQuality;
        mNegotiatedCallProfile.mMediaProfile.mVideoDirection =  mImsCallSessionImpl.mImsCallProfile.mMediaProfile.mVideoDirection;
        mCallIdMessage = mHandler.obtainMessage();//SPRD: add for bug545171
        mCi.registerForSrvccStateChanged(mVTHandler, EVENT_SRVCC_STATE_CHANGED, null);//SPRD:add for bug563112
    }

    public void onVTConnectionEstablished(ImsCallSessionImpl mImsCallSessionImpl){
        mHandler.obtainMessage(mVTManagerProxy.EVENT_ON_VT_ESTABLISH, mImsCallSessionImpl).sendToTarget();
        acquireWakeLock();
    }

    public void onVTConnectionDisconnected(ImsCallSessionImpl mImsCallSessionImpl){
        mCi.unregisterForSrvccStateChanged(mVTHandler);
        /* SPRD: fix for bug547597 @{ */
        if (mLocalRequestProfile != null) {
            int result = android.telecom.Connection.VideoProvider.SESSION_MODIFY_REQUEST_FAIL;
            VideoProfile responseProfile = new VideoProfile(VideoProfile.STATE_AUDIO_ONLY);
            receiveSessionModifyResponse(result, mLocalRequestProfile, responseProfile);
            mLocalRequestProfile = null;
        }
        /* @} */
        mHandler.obtainMessage(mVTManagerProxy.EVENT_ON_VT_DISCONNECT, mImsCallSessionImpl).sendToTarget();
        releaseWakeLock();
        /* SPRD: fix for bug662570@{ */
        receiveSessionModifyResponse(android.telecom.Connection.VideoProvider.SESSION_MODIFY_REQUEST_INVALID,
                    null,null); //SPRD:add for bug610607
        /* @} */
    }
    /**
     * Sets the camera to be used for video recording in a video call.
     * @param cameraId The id of the camera.
     */
    @Override
    public void onSetCamera(String cameraId) {
        mHandler.obtainMessage(mVTManagerProxy.EVENT_ON_SET_CAMERA, cameraId).sendToTarget();
    }

    /**
     * Sets the surface to be used for displaying a preview of what the user's camera is
     * currently capturing. When video transmission is enabled, this is the video signal which
     * is sent to the remote device.
     * @param surface The surface.
     */
    @Override
    public void onSetPreviewSurface(Surface surface) {
        mHandler.obtainMessage(mVTManagerProxy.EVENT_ON_SET_PREVIEW_SURFACE, surface).sendToTarget();
    }

    /**
     * Sets the surface to be used for displaying the video received from the remote device.
     * @param surface The surface.
     */
    @Override
    public void onSetDisplaySurface(Surface surface) {
        mHandler.obtainMessage(mVTManagerProxy.EVENT_ON_SET_DISPLAY_SURFACE, surface).sendToTarget();
    }

    /**
     * Sets the device orientation, in degrees. Assumes that a standard portrait orientation of
     * the device is 0 degrees.
     * @param rotation The device orientation, in degrees.
     */
    @Override
    public void onSetDeviceOrientation(int rotation) {
        mHandler.obtainMessage(mVTManagerProxy.EVENT_ON_SET_DEVICE_ORIENTATION, new Integer(rotation))
                .sendToTarget();
    }

    /**
     * Sets camera zoom ratio.
     * @param value The camera zoom ratio.
     */
    @Override
    public void onSetZoom(float value) {

    }

    /**
     * Issues a request to modify the properties of the current session. The request is sent to
     * the remote device where it it handled by the In-Call UI. Some examples of session
     * modification requests: upgrade call from audio to video, downgrade call from video to
     * audio, pause video.
     * @param requestProfile The requested call video properties.
     */
    @Override
    public void onSendSessionModifyRequest(VideoProfile fromProfile, VideoProfile toProfile) {
        log("onSendSessionModifyRequest->fromProfile:" + fromProfile + "  toProfile ="
                + toProfile);

        if ((fromProfile == null || toProfile == null)
                || (fromProfile.getVideoState() == toProfile.getVideoState())) {
            log("onSendSessionModifyRequest->fromProfile = toProfile");
            return;
        }
        ImsCallProfile requestImsCallProfile = new ImsCallProfile();
        if(VideoProfile.isAudioOnly(toProfile.getVideoState())){
            requestImsCallProfile.mCallType = ImsCallProfile.CALL_TYPE_VOICE;
        }else if(VideoProfile.isTransmissionEnabled(toProfile.getVideoState())){
            requestImsCallProfile.mCallType = ImsCallProfile.CALL_TYPE_VT;
        }else if(VideoProfile.isReceptionEnabled(toProfile.getVideoState())){
            requestImsCallProfile.mCallType = ImsCallProfile.CALL_TYPE_VT_RX;
        }

        if (mImsCallSessionImpl == null || mImsCallSessionImpl.mImsCallProfile == null) {
            log("onSendSessionModifyRequest mImsCallSessionImpl = "+ mImsCallSessionImpl);
            log("onSendSessionModifyRequest mImsCallProfile = "+ mImsCallSessionImpl.mImsCallProfile);
            return ;
        }
        mImsCallSessionImpl.updateVideoTxState(!VideoProfile.isTransmissionEnabled(toProfile.getVideoState()));
        if(requestImsCallProfile.mCallType == ImsCallProfile.CALL_TYPE_VT_RX ||
                (isVideoCall(mImsCallSessionImpl.mImsCallProfile.mCallType) && isVideoCall(requestImsCallProfile.mCallType))){
            log("onSendSessionModifyRequest is pause or resume request.");
            if(requestImsCallProfile.mCallType == ImsCallProfile.CALL_TYPE_VT_RX){
                mCi.requestVolteCallMediaChange(ImsRIL.CALL_MEDIA_CHANGE_ACTION_SET_TO_PAUSE,Integer.parseInt(mImsCallSessionImpl.getCallId()),null);
            } else if(requestImsCallProfile.mCallType == ImsCallProfile.CALL_TYPE_VT){
                mCi.requestVolteCallMediaChange(ImsRIL.CALL_MEDIA_CHANGE_ACTION_RESUME_FORM_PAUSE,Integer.parseInt(mImsCallSessionImpl.getCallId()),null);
            }
            return;
        }

        if(requestImsCallProfile.mCallType != mImsCallSessionImpl.mImsCallProfile.mCallType
                && requestImsCallProfile.mCallType != mImsCallSessionImpl.getLocalRequestProfile().mCallType){
            mLocalRequestProfile = toProfile;
            mImsCallSessionImpl.getLocalRequestProfile().mCallType = requestImsCallProfile.mCallType;
            if(requestImsCallProfile.mCallType == ImsCallProfile.CALL_TYPE_VT){
               mCi.requestVolteCallMediaChange(ImsRIL.CALL_MEDIA_CHANGE_ACTION_DOWNGRADE_TO_VOICE, Integer.parseInt(mImsCallSessionImpl.getCallId()),null);
                //SPRD: add for bug674565
                if(mContext!=null){
                    Message msg = new Message();
                    msg.what = EVENT_VOLTE_CALL_REQUEST_MEDIA_CHANGED_TIMEOUT;
                    msg.obj = mCi;
                    mVTHandler.removeMessages(EVENT_VOLTE_CALL_REQUEST_MEDIA_CHANGED_TIMEOUT);
                    mVTHandler.sendMessageDelayed(msg, 20000);
                }
            } else {
               mCi.requestVolteCallMediaChange(ImsRIL.CALL_MEDIA_CHANGE_ACTION_UPGRADE_TO_VIDEO, Integer.parseInt(mImsCallSessionImpl.getCallId()),null);
            }
            requestImsCallProfile = null;
        }
    }

    /**
     * Provides a response to a request to change the current call session video properties.
     * This is in response to a request the InCall UI has received via the InCall UI.
     * @param responseProfile The response call video properties.
     */
    @Override
    public void onSendSessionModifyResponse(VideoProfile responseProfile) {
            log("onSendSessionModifyResponse->responseProfile:" + responseProfile +" callId:"+mImsCallSessionImpl.getCallId());
            if(VideoProfile.isVideo(responseProfile.getVideoState())){
                mCi.responseVolteCallMediaChange(true, Integer.parseInt(mImsCallSessionImpl.getCallId()),null);
                receiveSessionModifyResponse(android.telecom.Connection.VideoProvider.SESSION_MODIFY_REQUEST_INVALID,
                        null,null);
            }else if (VideoProfile.isAudioOnly(responseProfile.getVideoState())){
                mCi.responseVolteCallMediaChange(false, Integer.parseInt(mImsCallSessionImpl.getCallId()),null);
                receiveSessionModifyResponse(android.telecom.Connection.VideoProvider.SESSION_MODIFY_REQUEST_INVALID,
                        null,null);
            }
    }

    public void updateVideoQuality(VideoProfile responseProfile) {
            log("onSendSessionModifyResponse->responseProfile:" + responseProfile);
            if(responseProfile != null){
                log("onSendSessionModifyRequest.updateVideoQuality-> quality:"+ responseProfile.getQuality());
                mHandler.obtainMessage(mVTManagerProxy.EVENT_ON_UPDATE_DEVICE_QUALITY, new Integer(responseProfile.getQuality())).sendToTarget();
            }
    }
    /**
     * Issues a request to the video provider to retrieve the camera capabilities. Camera
     * capabilities are reported back to the caller via the In-Call UI.
     */
    @Override
    public void onRequestCameraCapabilities() {

    }

    /**
     * Issues a request to the video telephony framework to retrieve the cumulative data usage
     * for the current call. Data usage is reported back to the caller via the InCall UI.
     */
    @Override
    public void onRequestCallDataUsage() {

    }


    /**
     * Provides the video telephony framework with the URI of an image to be displayed to remote
     * devices when the video signal is paused.
     * @param uri URI of image to display.
     */
    @Override
    public void onSetPauseImage(Uri uri) {
        mHandler.obtainMessage(mVTManagerProxy.EVENT_ON_SET_PAUSE_IMAGE, uri).sendToTarget();
    }

    private void log(String string) {
        android.util.Log.i(TAG, string);
    }

    private void createWakeLock(Context context) {
        PowerManager pm = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        mPartialWakeLock = pm.newWakeLock(PowerManager.FULL_WAKE_LOCK, TAG);

    }

    private void acquireWakeLock() {
        log("acquireWakeLock, isHeld:"+mPartialWakeLock.isHeld());
        synchronized (mPartialWakeLock) {
            if (!mPartialWakeLock.isHeld()) {
                mPartialWakeLock.acquire();
            }
        }
    }

    private void releaseWakeLock() {
        log("releaseWakeLock : "+mPartialWakeLock.isHeld());
        synchronized (mPartialWakeLock) {
            if (mPartialWakeLock.isHeld()) {
                log("releaseWakeLock");
                mPartialWakeLock.release();
            }
        }
    }

    class ImsCallSessionImplListner implements ImsCallSessionImpl.Listener{
        @Override
        public void onDisconnected(ImsCallSessionImpl session){
            log("onDisconnected->session="+session);
            onVTConnectionDisconnected(session);
            mImsCallSessionImpl.removeListener(mImsCallSessionImplListner);
        }

        @Override
        public void onUpdate(ImsCallSessionImpl session){
            if(mImsCallSessionImpl != session){
                log("onUpdate->session is not match.");
                return;
            }
            updateNegotiatedCallProfile(session);
            handleClearLocalCallProfile(session);
            handleVolteCallMediaChange(session);
        }
     };

     public void updateNegotiatedCallProfile(ImsCallSessionImpl session){
         ImsCallProfile imsCallProfile = session.getCallProfile();
         VideoProfile responseProfile = new VideoProfile(VideoProfile.STATE_AUDIO_ONLY);
         log("updateNegotiatedCallProfilee->mCallType="+imsCallProfile.mCallType+" session state = "+session.getState()+ "mIsVoiceRingTone ="+mIsVoiceRingTone);
         //SPRD:fix for bug 597075
         if(isVideoCall(imsCallProfile.mCallType)
                 && (session != null && session.mImsDriverCall != null && session.mImsDriverCall.state != ImsDriverCall.State.HOLDING)){
             mIsVideo = true;//SPRD:add for bug563112
             responseProfile = new VideoProfile(VideoProfile.STATE_BIDIRECTIONAL);
             onVTConnectionEstablished(session);
             //SPRD:add for bug669739 & bug677255
             if (session.getState() == ImsCallSession.State.NEGOTIATING && !mIsOrigionVideo && !session.mImsDriverCall.isMT) {
                 log("updateNegotiatedCallProfilee->set mIsVoiceRingTone = true");
                 mIsVoiceRingTone = true;
             }
         } else {
             /* SPRD:add for bug563112 & 677255@{ */
             if(!isVideoCall(imsCallProfile.mCallType) && mIsVideo && (session != null && session.mImsDriverCall != null) ){
                 if(!mIsVoiceRingTone) {
                     Toast.makeText(mContext, mContext.getResources().getString(R.string.videophone_fallback_title), Toast.LENGTH_LONG).show();
                 }
                 log("updateNegotiatedCallProfilee->makeText");
                 mIsVideo = false;
             }
             /* @} */
             onVTConnectionDisconnected(session);
         }

         if (session.getState() == ImsCallSession.State.ESTABLISHED && mIsVoiceRingTone) {
             //SPRD: add for bug677255
             log("updateNegotiatedCallProfilee->set mIsVoiceRingTone false");
             mIsVoiceRingTone = false;
         }

         if (mLocalRequestProfile != null) {
             int result = android.telecom.Connection.VideoProvider.SESSION_MODIFY_REQUEST_FAIL;
             if(mImsCallSessionImpl.getLocalRequestProfile().mCallType == imsCallProfile.mCallType){
                 result = android.telecom.Connection.VideoProvider.SESSION_MODIFY_REQUEST_SUCCESS;
             }
             receiveSessionModifyResponse(result, mLocalRequestProfile, responseProfile);
             mLocalRequestProfile = null;
             showRequestStateToast();
         }
         if(mNegotiatedCallProfile.mCallType != imsCallProfile.mCallType){
             mNegotiatedCallProfile.mCallType = imsCallProfile.mCallType;
         }
         if(mNegotiatedCallProfile.mMediaProfile.mAudioQuality !=
                 mImsCallSessionImpl.mImsCallProfile.mMediaProfile.mAudioQuality){
             mNegotiatedCallProfile.mMediaProfile.mAudioQuality =
                     mImsCallSessionImpl.mImsCallProfile.mMediaProfile.mAudioQuality;
         }
         if(mNegotiatedCallProfile.mMediaProfile.mAudioDirection !=
                 mImsCallSessionImpl.mImsCallProfile.mMediaProfile.mAudioDirection){
             mNegotiatedCallProfile.mMediaProfile.mAudioDirection =
                     mImsCallSessionImpl.mImsCallProfile.mMediaProfile.mAudioDirection;
         }
         if(mNegotiatedCallProfile.mMediaProfile.mVideoQuality !=
                 mImsCallSessionImpl.mImsCallProfile.mMediaProfile.mVideoQuality){
             mNegotiatedCallProfile.mMediaProfile.mVideoQuality =
                     mImsCallSessionImpl.mImsCallProfile.mMediaProfile.mVideoQuality;
         }
         if(mNegotiatedCallProfile.mMediaProfile.mVideoDirection !=
                 mImsCallSessionImpl.mImsCallProfile.mMediaProfile.mVideoDirection){
             mNegotiatedCallProfile.mMediaProfile.mVideoDirection =
                     mImsCallSessionImpl.mImsCallProfile.mMediaProfile.mVideoDirection;
         }
     }

    public void handleClearLocalCallProfile(ImsCallSessionImpl session){
          /* SPRD: Modify for Bug538938 @{ */
          if(session.mImsDriverCall != null && (session.mImsDriverCall.isReuestAccept() || session.mImsDriverCall.isReuestReject() || session.mImsDriverCall.isRequestUpgradeToVideo() || session.mImsDriverCall.isRequestDowngradeToVoice()) ||
             mImsCallSessionImpl.getLocalRequestProfile().mCallType == mImsCallSessionImpl.mImsCallProfile.mCallType){
             mImsCallSessionImpl.getLocalRequestProfile().mCallType = ImsCallProfile.CALL_TYPE_VOICE_N_VIDEO;
              /*SPRD: add for bug674565*/
              if (mVTHandler != null && mVTHandler.hasMessages(EVENT_VOLTE_CALL_REQUEST_MEDIA_CHANGED_TIMEOUT) && mContext != null) {
                  log("handleClearLocalCallProfile remove EVENT_VOLTE_CALL_REQUEST_MEDIA_CHANGED_TIMEOUT");

                  mVTHandler.removeMessages(EVENT_VOLTE_CALL_REQUEST_MEDIA_CHANGED_TIMEOUT);
              }/* @} */
          }
          /* @} */
    }

    public void handleVolteCallMediaChange(ImsCallSessionImpl session){
        log("handleVolteCallMediaChange->session="+session);
            if(session.mImsDriverCall != null && session.mImsDriverCall.isRequestUpgradeToVideo()){
                if(mVolteMediaUpdateDialog != null){
                   mVolteMediaUpdateDialog.dismiss();
                }
                /*SPRD: add for bug606122, 605475, 676047@{*/
                if(!TelephonyManager.getDefault().isVideoCallingEnabled()
                        || mImsCallSessionImpl.getCurrentUserId() != UserHandle.USER_OWNER
                        || mImsCallSessionImpl.getIsInLocalConference()||mImsCallSessionImpl.mInLocalCallForward
                        || mImsCallSessionImpl.mImsServiceCallTracker.hasRingingCall()){
                    //SPRD:add for bug682362
                    log("handleVolteCallMediaChange reject");
                    mCi.responseVolteCallMediaChange(false, Integer.parseInt(mImsCallSessionImpl.getCallId()),mCallIdMessage);
                    return;
                }/*@}*/
                else if(ImsCmccHelper.getInstance(mContext).rejectMediaChange(mImsCallSessionImpl,mCi,mCallIdMessage)){
                    log("handleVolteCallMediaChange-is cmcc project, has one active adn one hold call reject MediaChange");
                }else{
                    /*TODO: remove for 8.0
                    mVolteMediaUpdateDialog = VTManagerUtils.showVolteCallMediaUpdateAlert(mContext.getApplicationContext(),mCi,mCallIdMessage,this);
                    mVolteMediaUpdateDialog.show();
                   */
                }

                Message msg = new Message();
                msg.what = EVENT_VOLTE_CALL_REMOTE_REQUEST_MEDIA_CHANGED_TIMEOUT;
                msg.obj = mCi;
                mVTHandler.removeMessages(EVENT_VOLTE_CALL_REMOTE_REQUEST_MEDIA_CHANGED_TIMEOUT);
                mVTHandler.sendMessageDelayed(msg, 10000);
                /* SPRD: add for bug543928 and bug601503@{ */
                PowerManager powerManager = (PowerManager) mContext.getSystemService(Context.POWER_SERVICE);
                if(powerManager != null && !powerManager.isScreenOn()){
                    powerManager.wakeUp(SystemClock.uptimeMillis(), "android.phone:WAKEUP");
                }

                VideoProfile mLoacalResponseProfile = new VideoProfile(VideoProfile.STATE_TX_ENABLED);
                VideoProfile mLoacalRequstProfile = new VideoProfile(VideoProfile.STATE_BIDIRECTIONAL);
                receiveSessionModifyRequest(mLoacalRequstProfile);

            }else if(session.mImsDriverCall != null && session.mImsDriverCall.isRequestDowngradeToVoice()){
                if(mVolteMediaDegradeDialog != null){
                   mVolteMediaDegradeDialog.dismiss();
                }
                mVolteMediaDegradeDialog = VTManagerUtils.showVolteCallMediaUpdateAlert(mContext.getApplicationContext(),mCi,mCallIdMessage,this);
                mVolteMediaDegradeDialog.show();
            }
    }

    @Override
    public void changeCameraCapabilities(CameraCapabilities CameraCapabilities) {
         super.changeCameraCapabilities(CameraCapabilities);
    }

    @Override
    public void receiveSessionModifyResponse(
            int status, VideoProfile requestedProfile, VideoProfile responseProfile) {
        super.receiveSessionModifyResponse(status,requestedProfile,responseProfile);
    }

    public void showRequestStateToast(){
        if (mImsCallSessionImpl.mImsDriverCall != null && mImsCallSessionImpl.mImsDriverCall.isReuestAccept()) {
            Toast.makeText(mContext.getApplicationContext(),
                    mContext.getString(R.string.remote_accept_request), Toast.LENGTH_SHORT).show();
        } else if(mImsCallSessionImpl.mImsDriverCall != null && mImsCallSessionImpl.mImsDriverCall.isReuestReject()){
            Toast.makeText(mContext.getApplicationContext(),
                    mContext.getString(R.string.remote_reject_request), Toast.LENGTH_SHORT).show();
        }
    }

    public boolean isVideoCall(int calltype){
        return calltype == ImsCallProfile.CALL_TYPE_VT
                || calltype == ImsCallProfile.CALL_TYPE_VT_RX
                || calltype == ImsCallProfile.CALL_TYPE_VT_TX;
    }
}

package com.spreadtrum.ims.vt;

import android.content.Context;
import android.net.Uri;
import android.os.Handler;
import android.telecom.VideoProfile;
import android.view.Surface;
import android.util.Log;
import com.android.internal.telephony.Connection;
import com.spreadtrum.ims.ImsCallSessionImpl;
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
import com.android.internal.telephony.ImsDriverCall;
import com.android.ims.internal.ImsCallSession;
import android.os.SystemClock;
import android.app.KeyguardManager;
import android.os.AsyncResult;
import android.telephony.VoLteServiceState;
import android.os.SystemProperties;

public class ImsVideoCallProvider extends com.android.ims.internal.ImsVideoCallProvider {
    private static final String TAG = ImsVideoCallProvider.class.getSimpleName();
    private VTManagerProxy mVTManagerProxy;
    private Handler mHandler;
    private ImsCallProfile mNegotiatedCallProfile = new ImsCallProfile();
    private VideoProfile mLocalRequestProfile;
    private ImsCallSessionImpl mImsCallSessionImpl;
    private Context mContext;
    private CommandsInterface mCi;
    private AlertDialog mVolteMediaUpdateDialog;
    private AlertDialog mVolteMediaDegradeDialog;
    private ImsCallSessionImplListner mImsCallSessionImplListner;
    private PowerManager.WakeLock mPartialWakeLock;
    private Message mCallIdMessage;
    private boolean mIsVideo;//SPRD:add for bug563112

    /** volte media event code. */
    private static final int EVENT_VOLTE_CALL_REMOTE_REQUEST_MEDIA_CHANGED_TIMEOUT = 500;
    private static final int EVENT_SRVCC_STATE_CHANGED = 100;

    private Handler mVTHandler = new Handler(){
        @Override
        public void handleMessage(Message msg) {
            log("handleMessage msg = " + msg.what);
            AsyncResult ar;
            switch (msg.what) {
                case EVENT_VOLTE_CALL_REMOTE_REQUEST_MEDIA_CHANGED_TIMEOUT:
                    /* SPRD: add for bug545171 @{ */
                    mCallIdMessage.arg1 = Integer.parseInt(mImsCallSessionImpl.getCallId());
                    /* @} */
                    if(mVolteMediaUpdateDialog != null && mVolteMediaUpdateDialog.isShowing()){
                        mVolteMediaUpdateDialog.dismiss();
                        CommandsInterface mCi = (CommandsInterface)msg.obj;
                        mCi.responseVolteCallMediaChange(false,mCallIdMessage);
                    }
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
            log("handleSrvccStateChanged..state:"+state+"   mIsVideo="+mIsVideo+"   mContext:"+mContext+"  imsvideocallprovider="+this);
            switch(state) {
                case VoLteServiceState.HANDOVER_COMPLETED:
                    if(mIsVideo && (mContext != null)){
                       mIsVideo = false;
                       Toast.makeText(mContext,mContext.getResources().getString(R.string.videophone_fallback_title),Toast.LENGTH_LONG).show();//modify by bug593544
                    }
                    break;
                default:
                    return;
            }
        }
    }
    /* @} */
    public ImsVideoCallProvider(ImsCallSessionImpl imsCallSessionImpl,CommandsInterface ci,Context context) {
        super();
        mVTManagerProxy = VTManagerProxy.getInstance();
        mHandler = mVTManagerProxy.mHandler;
        mImsCallSessionImpl = imsCallSessionImpl;
        mContext = context;
        mCi = ci;
        mImsCallSessionImplListner = new ImsCallSessionImplListner();
        mImsCallSessionImpl.addListener(mImsCallSessionImplListner);
        createWakeLock(mContext.getApplicationContext());
        if(mImsCallSessionImpl.mImsCallProfile.mCallType == ImsCallProfile.CALL_TYPE_VT){
           onVTConnectionEstablished(mImsCallSessionImpl);
        }
        mNegotiatedCallProfile.mCallType = mImsCallSessionImpl.mImsCallProfile.mCallType;
        mNegotiatedCallProfile.mMediaProfile.mAudioQuality =  mImsCallSessionImpl.mImsCallProfile.mMediaProfile.mAudioQuality;
        mNegotiatedCallProfile.mMediaProfile.mAudioDirection =  mImsCallSessionImpl.mImsCallProfile.mMediaProfile.mAudioDirection;
        mNegotiatedCallProfile.mMediaProfile.mVideoQuality =  mImsCallSessionImpl.mImsCallProfile.mMediaProfile.mVideoQuality;
        mNegotiatedCallProfile.mMediaProfile.mVideoDirection =  mImsCallSessionImpl.mImsCallProfile.mMediaProfile.mVideoDirection;
        mCallIdMessage = new Message();//SPRD: add for bug545171
        mCi.registerForSrvccStateChanged(mVTHandler, EVENT_SRVCC_STATE_CHANGED, null);//SPRD:add for bug563112
    }

    public void onVTConnectionEstablished(ImsCallSessionImpl mImsCallSessionImpl){
        mHandler.obtainMessage(mVTManagerProxy.EVENT_ON_VT_ESTABLISH, mImsCallSessionImpl).sendToTarget();
        acquireWakeLock();
    }

    public void onVTConnectionDisconnected(ImsCallSessionImpl mImsCallSessionImpl){
        /* SPRD: fix for bug588497@{ */
        mIsVideo = false;
        /* @} */
        mHandler.obtainMessage(mVTManagerProxy.EVENT_ON_VT_DISCONNECT, mImsCallSessionImpl).sendToTarget();
        releaseWakeLock();
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
        }

        if (mImsCallSessionImpl == null || mImsCallSessionImpl.mImsCallProfile == null) {
            log("onSendSessionModifyRequest mImsCallSessionImpl = "+ mImsCallSessionImpl);
            log("onSendSessionModifyRequest mImsCallProfile = "+ mImsCallSessionImpl.mImsCallProfile);
            return ;
        }

        if(requestImsCallProfile.mCallType != mImsCallSessionImpl.mImsCallProfile.mCallType
                && requestImsCallProfile.mCallType != mImsCallSessionImpl.getLocalRequestProfile().mCallType){
            mLocalRequestProfile = toProfile;
            mImsCallSessionImpl.getLocalRequestProfile().mCallType = requestImsCallProfile.mCallType;
            /* SPRD: add for bug533562 @{ */
            Message message = new Message();
            message.arg1 = Integer.parseInt(mImsCallSessionImpl.getCallId());
            /* @} */
            if(requestImsCallProfile.mCallType == ImsCallProfile.CALL_TYPE_VT){
               mCi.requestVolteCallMediaChange(false,message);
            } else {
               mCi.requestVolteCallMediaChange(true,message);
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
            log("onSendSessionModifyResponse->responseProfile:" + responseProfile);
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
            /* SPRD: fix for bug547597@{ */
            if (mLocalRequestProfile != null) {
                int result = android.telecom.Connection.VideoProvider.SESSION_MODIFY_REQUEST_FAIL;
                VideoProfile responseProfile = new VideoProfile(VideoProfile.STATE_AUDIO_ONLY);
                receiveSessionModifyResponse(result, mLocalRequestProfile, responseProfile);
                mLocalRequestProfile = null;
            }
            /* @} */
            onVTConnectionDisconnected(session);
            mImsCallSessionImpl.removeListener(mImsCallSessionImplListner);
            /*add for bug593544 @{*/
            if(mCi != null && mVTHandler != null ){
                mCi.unregisterForSrvccStateChanged(mVTHandler);
            }/*@}*/
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
         log("updateNegotiatedCallProfilee->mCallType="+imsCallProfile.mCallType +"imsvideocallprovider="+this);
         if(imsCallProfile.mCallType == ImsCallProfile.CALL_TYPE_VT){
             mIsVideo = true;//SPRD:add for bug563112
             responseProfile = new VideoProfile(VideoProfile.STATE_BIDIRECTIONAL);
             onVTConnectionEstablished(session);
         } else {
             /* SPRD:add for bug563112 @{ */
             if(mIsVideo && (session != null && session.mImsDriverCall != null)){

                 Toast.makeText(mContext,mContext.getResources().getString(R.string.videophone_fallback_title),Toast.LENGTH_LONG).show();//modify by bug593544
             }
             /* @} */
             onVTConnectionDisconnected(session);
         }
         if (mLocalRequestProfile != null) {
             int result = android.telecom.Connection.VideoProvider.SESSION_MODIFY_REQUEST_FAIL;
             if(mImsCallSessionImpl.getLocalRequestProfile().mCallType == imsCallProfile.mCallType
                     || ((mImsCallSessionImpl.getLocalRequestProfile().mCallType == ImsCallProfile.CALL_TYPE_VOICE)
                             && (imsCallProfile.mCallType == ImsCallProfile.CALL_TYPE_VOICE_N_VIDEO))){
                 result = android.telecom.Connection.VideoProvider.SESSION_MODIFY_REQUEST_SUCCESS;
             }
             receiveSessionModifyResponse(result, mLocalRequestProfile, responseProfile);
             showRequestStateToast();
             mLocalRequestProfile = null;
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
          }
          /* @} */
    }

    public void handleVolteCallMediaChange(ImsCallSessionImpl session){
        log("handleVolteCallMediaChange->session="+session);
        /* SPRD: add for bug545171 @{ */
        mCallIdMessage.arg1 = Integer.parseInt(mImsCallSessionImpl.getCallId());
        /* @} */
            if(session.mImsDriverCall != null && session.mImsDriverCall.isRequestUpgradeToVideo()){
                if(mVolteMediaUpdateDialog != null){
                   mVolteMediaUpdateDialog.dismiss();
                }
                if(!SystemProperties.getBoolean("persist.sys.support.vt", false)){
                    log("handleVolteCallMediaChange reject");
                    mCi.responseVolteCallMediaChange(false,mCallIdMessage);
                    return;
                }
                mVolteMediaUpdateDialog = VTManagerUtils.showVolteCallMediaUpdateAlert(mContext.getApplicationContext(),mCi,mCallIdMessage);
                mVolteMediaUpdateDialog.show();
                Message msg = new Message();
                msg.what = EVENT_VOLTE_CALL_REMOTE_REQUEST_MEDIA_CHANGED_TIMEOUT;
                msg.obj = mCi;
                mVTHandler.removeMessages(EVENT_VOLTE_CALL_REMOTE_REQUEST_MEDIA_CHANGED_TIMEOUT);
                mVTHandler.sendMessageDelayed(msg, 10000);
                /* SPRD: add for bug543928@{ */
                KeyguardManager keyguardManager = (KeyguardManager) mContext.getSystemService(Context.KEYGUARD_SERVICE);
                PowerManager powerManager = (PowerManager) mContext.getSystemService(Context.POWER_SERVICE);
                if(powerManager != null && !powerManager.isScreenOn()){
                    powerManager.wakeUp(SystemClock.uptimeMillis(), "android.phone:WAKEUP");
                }
                VideoProfile mLoacalResponseProfile = new VideoProfile(VideoProfile.STATE_TX_ENABLED);
                VideoProfile mLoacalRequstProfile = new VideoProfile(VideoProfile.STATE_BIDIRECTIONAL);
                receiveSessionModifyResponse(android.telecom.Connection.VideoProvider.SESSION_MODIFY_REQUEST_INVALID,
                        mLoacalRequstProfile,mLoacalResponseProfile);
                /* @} */
            }else if(session.mImsDriverCall != null && session.mImsDriverCall.isRequestDowngradeToVoice()){
                if(mVolteMediaDegradeDialog != null){
                   mVolteMediaDegradeDialog.dismiss();
                }
                mVolteMediaDegradeDialog = VTManagerUtils.showVolteCallMediaUpdateAlert(mContext.getApplicationContext(),mCi,mCallIdMessage);
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
}

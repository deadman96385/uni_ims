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

    public ImsVideoCallProvider(ImsCallSessionImpl imsCallSessionImpl,CommandsInterface ci,Context context) {
        super();
        mVTManagerProxy = VTManagerProxy.getInstance();
        mHandler = mVTManagerProxy.mHandler;
        mImsCallSessionImpl = imsCallSessionImpl;
        mContext = context;
        mCi = ci;
        mImsCallSessionImplListner = new ImsCallSessionImplListner();
        mImsCallSessionImpl.addListener(mImsCallSessionImplListner);
        if(mImsCallSessionImpl.mImsCallProfile.mCallType == ImsCallProfile.CALL_TYPE_VT){
           onVTConnectionEstablished(mImsCallSessionImpl);
        }
        mNegotiatedCallProfile.mCallType = mImsCallSessionImpl.mImsCallProfile.mCallType;
        mNegotiatedCallProfile.mMediaProfile.mAudioQuality =  mImsCallSessionImpl.mImsCallProfile.mMediaProfile.mAudioQuality;
        mNegotiatedCallProfile.mMediaProfile.mAudioDirection =  mImsCallSessionImpl.mImsCallProfile.mMediaProfile.mAudioDirection;
        mNegotiatedCallProfile.mMediaProfile.mVideoQuality =  mImsCallSessionImpl.mImsCallProfile.mMediaProfile.mVideoQuality;
        mNegotiatedCallProfile.mMediaProfile.mVideoDirection =  mImsCallSessionImpl.mImsCallProfile.mMediaProfile.mVideoDirection;
    }

    public void onVTConnectionEstablished(ImsCallSessionImpl mImsCallSessionImpl){
        mHandler.obtainMessage(mVTManagerProxy.EVENT_ON_VT_ESTABLISH, mImsCallSessionImpl).sendToTarget();
    }

    public void onVTConnectionDisconnected(ImsCallSessionImpl mImsCallSessionImpl){
        mHandler.obtainMessage(mVTManagerProxy.EVENT_ON_VT_DISCONNECT, mImsCallSessionImpl).sendToTarget();
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

        if(requestImsCallProfile.mCallType != mImsCallSessionImpl.mImsCallProfile.mCallType
                && requestImsCallProfile.mCallType != mImsCallSessionImpl.getLocalRequestProfile().mCallType){
            mLocalRequestProfile = toProfile;
            mImsCallSessionImpl.getLocalRequestProfile().mCallType = requestImsCallProfile.mCallType;
            if(requestImsCallProfile.mCallType == ImsCallProfile.CALL_TYPE_VT){
               mCi.requestVolteCallMediaChange(false,null);
            } else {
               mCi.requestVolteCallMediaChange(true,null);
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

     class ImsCallSessionImplListner implements ImsCallSessionImpl.Listener{
        @Override
        public void onDisconnected(ImsCallSessionImpl session){
            log("handleVolteCallMediaChange->session="+session);
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
         if(imsCallProfile.mCallType == ImsCallProfile.CALL_TYPE_VT){
             responseProfile = new VideoProfile(VideoProfile.STATE_BIDIRECTIONAL);
             onVTConnectionEstablished(session);
         } else {
             onVTConnectionDisconnected(session);
         }
         if (mLocalRequestProfile != null) {
             int result = android.telecom.Connection.VideoProvider.SESSION_MODIFY_REQUEST_FAIL;
             if(mImsCallSessionImpl.getLocalRequestProfile().mCallType == imsCallProfile.mCallType){
                 result = android.telecom.Connection.VideoProvider.SESSION_MODIFY_REQUEST_SUCCESS;
             }
             receiveSessionModifyResponse(result, mLocalRequestProfile, responseProfile);
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
          if(session.mImsDriverCall != null && (session.mImsDriverCall.isReuestAccept() || session.mImsDriverCall.isReuestReject()) || 
             mImsCallSessionImpl.getLocalRequestProfile().mCallType == mImsCallSessionImpl.mImsCallProfile.mCallType){
             mImsCallSessionImpl.getLocalRequestProfile().mCallType = ImsCallProfile.CALL_TYPE_VOICE_N_VIDEO;
          }
    }

    public void handleVolteCallMediaChange(ImsCallSessionImpl session){
        log("handleVolteCallMediaChange->session="+session);
            if(session.mImsDriverCall != null && session.mImsDriverCall.isRequestUpgradeToVideo()){
                if(mVolteMediaUpdateDialog != null){
                   mVolteMediaUpdateDialog.dismiss();
                }
                mVolteMediaUpdateDialog = VTManagerUtils.showVolteCallMediaUpdateAlert(mContext.getApplicationContext(),mCi);
                mVolteMediaUpdateDialog.show();
            }else if(session.mImsDriverCall != null && session.mImsDriverCall.isRequestDowngradeToVoice()){
                if(mVolteMediaDegradeDialog != null){
                   mVolteMediaDegradeDialog.dismiss();
                }
                mVolteMediaDegradeDialog = VTManagerUtils.showVolteCallMediaUpdateAlert(mContext.getApplicationContext(),mCi);
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
}

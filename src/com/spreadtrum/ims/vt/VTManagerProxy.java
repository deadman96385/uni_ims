package com.spreadtrum.ims.vt;

import android.app.AlertDialog;
import android.content.Context;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.AsyncResult;
import android.os.BatteryManager;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.telecom.Connection.VideoProvider;
import android.telecom.VideoProfile;
import android.view.Surface;

import com.spreadtrum.ims.ImsDriverCall;
import com.spreadtrum.ims.ImsCallSessionImpl;
import com.spreadtrum.ims.ImsRIL;
import com.spreadtrum.ims.ImsService;
import com.spreadtrum.ims.ImsServiceCallTracker;
import com.spreadtrum.ims.ImsConfigImpl;


import com.android.internal.telephony.Call;
import com.android.internal.telephony.Connection;
import com.android.internal.telephony.RIL;
import com.android.internal.telephony.GsmCdmaPhone;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;
import android.telephony.ims.ImsCallSession;
import com.android.ims.internal.IImsVideoCallProvider;
//import android.telephony.TelephonyManagerEx;

//TODO:import android.telephony.TelephonyManagerEx;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.IntentFilter;
import android.app.AlertDialog;
import com.spreadtrum.ims.ImsRIL;

public class VTManagerProxy{
    private static final String TAG = "ImsVTManagerProxy";

    /** phone event code. */
    private static final int EVENT_HANDOVER_STATE_CHANGED = 103;

    /** video call provider event code. */
    public static final int EVENT_ON_SET_CAMERA = 200;
    public static final int EVENT_ON_SET_PREVIEW_SURFACE = 201;
    public static final int EVENT_ON_SET_DISPLAY_SURFACE = 202;
    public static final int EVENT_ON_SET_PAUSE_IMAGE = 203;
    public static final int EVENT_ON_SET_DEVICE_ORIENTATION = 204;
    public static final int EVENT_ON_UPDATE_DEVICE_QUALITY = 205;


    /** video call cp event code. */
    private static final int EVENT_VIDEO_CALL_CODEC = 300;
    private static final int EVENT_VIDEO_CALL_FAIL = 301;
    private static final int EVENT_VIDEO_CALL_FALL_BACK = 302;
    private static final int EVENT_IMS_VIDEO_QOS = 303;

    /** video connection event code. */
    private static final int EVENT_CONNECTION_VIDEO_STATE_CHANGED = 400;
    private static final int EVENT_CONNECTION_LOCAL_VIDEO_CAPABILITY_CHANGED = 401;
    private static final int EVENT_CONNECTION_REMOTE_VIDEO_CAPABILITY_CHANGED = 402;
    private static final int EVENT_CONNECTION_VIDEO_PROVIDER_CHANGED = 403;
    private static final int EVENT_CONNECTION_AUDIO_QUALITY_CHANGED = 404;

    public static final int EVENT_ON_VT_ESTABLISH = 600;
    public static final int EVENT_ON_VT_DISCONNECT = 601;

    private static final Object mLock = new Object();
    private static VTManagerProxy mInstance;

    private ImsService mImsService;
    private Context mContext;
    private VideoProfile mLoacalRequestProfile;
    private VideoCallEngine mVideoCallEngine;
    private VideoCallCameraManager mVideoCallCameraManager;
    private ImsCallSessionImpl mActiveImsCallSessionImpl;
    private HandlerThread mMediaPhoneThread;

    private Surface mPreviewSurface;
    private Surface mDisplaySurface;
    private String mCameraId;
    private Uri mPauseImage;
    private AlertDialog mFallBackDialog;
    private ImsCallSessionImpl mImsCallSessionImpl;
    private int mPeerVideoQuality = -1;
    private int mPeerWidth = 480;
    private int mPeerHeight = 640;
    public int mPreviewWidth = 480;
    public int mPreviewHeight = 640;
    private int mRotation = -1;
    private int mImsVideoQos;

    private IntentFilter mIntentFilter;//SPRD:SPRD: Add feature of low battery for Reliance
    private boolean mIsBroadcastReceiverRegisterd;//SPRD:SPRD: Add feature of low battery for Reliance
    private AlertDialog mVolteMediaDialog;//SPRD:SPRD: Add feature of low battery for Reliance
    private ImsRIL mRIL;//SPRD:Add feature of low battery for Reliance


    private VTManagerProxy(ImsService imsService) {
        mImsService = imsService;
        mContext = (Context)mImsService;
    }

    public static VTManagerProxy init(ImsService imsService) {
         synchronized (mLock) {
            if (mInstance == null) {
                mInstance = new VTManagerProxy(imsService);
             }
           return (VTManagerProxy) mInstance;
        }
    }

    public static VTManagerProxy getInstance() {
        return mInstance;
    }

    /**
     * Used to listen to events.
     */
    public final Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case EVENT_ON_SET_CAMERA:
                    handleSetCamera((String) msg.obj);
                    break;
                case EVENT_ON_SET_PREVIEW_SURFACE:
                    handleSetPreviewSurface((Surface) msg.obj);
                    break;
                case EVENT_ON_SET_DISPLAY_SURFACE:
                    handleSetDisplaySurface((Surface) msg.obj);
                    break;
                case EVENT_ON_SET_PAUSE_IMAGE:
                    log("handleMessage:what = EVENT_ON_SET_PAUSE_IMAGE ->mPauseImage="
                            + mPauseImage);
                    break;
                case EVENT_ON_SET_DEVICE_ORIENTATION:
                    handleSetDeviceOrientation((Integer) msg.obj);
                    break;
                case EVENT_VIDEO_CALL_CODEC:
                    handleVideoCallCodecEvent((AsyncResult) msg.obj);
                    break;
                case EVENT_VIDEO_CALL_FAIL:
                    handleVideoCallFail((AsyncResult) msg.obj);
                    break;
                case EVENT_VIDEO_CALL_FALL_BACK:
                    handleVideoCallFallBack((AsyncResult) msg.obj);
                    break;
                case EVENT_ON_VT_ESTABLISH:
                    handleVTConnectionEstablished((ImsCallSessionImpl) msg.obj);
                    break;
                case EVENT_ON_VT_DISCONNECT:
                    handleDisconnect((ImsCallSessionImpl) msg.obj);
                    break;
                case EVENT_ON_UPDATE_DEVICE_QUALITY:
                    handleUpdateVideoQuality((Integer) msg.obj);
                    break;
                case EVENT_IMS_VIDEO_QOS:
                    handleImsQosReport((AsyncResult) msg.obj);
                    break;
                default:
                    log("handleMessage,unkwon message:what =" + msg.what);
                    break;
            }
        }
    };
    /* SPRD: Add feature of low battery for Reliance @{ */
    private BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equals(Intent.ACTION_BATTERY_CHANGED) /*TODO: && TelephonyManagerEx.isBatteryLow()*/) {
                int batteryLevel = intent.getIntExtra(BatteryManager.EXTRA_LEVEL, 0);
                boolean isShowLowBattery = context.getResources().getBoolean(com.android.internal.R.bool.config_show_low_battery_dialog);
                log("batteryLevel =" + batteryLevel + " isShowLowBattery = " + isShowLowBattery);
                if(batteryLevel == 15 && mActiveImsCallSessionImpl != null
                        && mActiveImsCallSessionImpl.mImsDriverCall != null
                        && mActiveImsCallSessionImpl.mImsDriverCall.state == ImsDriverCall.State.ACTIVE
                        && isShowLowBattery){
                    if (mVolteMediaDialog != null) {
                        mVolteMediaDialog.dismiss();
                    }
                    mVolteMediaDialog = VTManagerUtils.showLowBatteryMediaChangeAlert(context,
                            (mActiveImsCallSessionImpl != null)? Integer.parseInt(mActiveImsCallSessionImpl.getCallId()) : 0,mRIL,
                            (mActiveImsCallSessionImpl != null)? mActiveImsCallSessionImpl.getMediaRequest() : ImsRIL.MEDIA_REQUEST_DEFAULT);
                    mVolteMediaDialog.show();
                }
            }
        }
    };

    public void registerForLowBatteryNotify(ImsRIL ril) {
        mRIL = ril;
        mIntentFilter = new IntentFilter();
        mIntentFilter.addAction(Intent.ACTION_BATTERY_CHANGED);
        mContext.registerReceiver(mReceiver, mIntentFilter);
        mIsBroadcastReceiverRegisterd = true;
        mRIL.registerForImsVideoQos(mHandler, EVENT_IMS_VIDEO_QOS, null);
        log("registerForLowBatteryNotify");
    }

    public void unregisterForLowBatteryNotify() {
        if (mIsBroadcastReceiverRegisterd) {
            mContext.unregisterReceiver(mReceiver);
            mRIL.unregisterForImsVideoQos(mHandler);
            mIsBroadcastReceiverRegisterd = false;
            log("unregisterForLowBatteryNotify");
        }
    }
    /* @} */

    public void handleVTConnectionEstablished(ImsCallSessionImpl imsCallSessionImpl){
        log("VTConnectionEstablished->imsCallSessionImpl="+imsCallSessionImpl);
        if (isImsCallAlive() && imsCallSessionImpl.mImsDriverCall != null
                && imsCallSessionImpl.mImsDriverCall.state == ImsDriverCall.State.ACTIVE) {
            mActiveImsCallSessionImpl = imsCallSessionImpl;
        }
        if (isImsCallAlive()) {
            log("handleVTConnectionEstablished->Don't create VTManager cause : isVideoCallAlive()="
                    + isImsCallAlive());
            return;
        }
        mActiveImsCallSessionImpl = imsCallSessionImpl;
        if (mVideoCallEngine == null) {
            final Object syncObj = new Object();

            final ImsRIL ril = (ImsRIL)mActiveImsCallSessionImpl.mCi;
            final int serviceId = imsCallSessionImpl.getServiceId();
            mMediaPhoneThread = new HandlerThread("VideoCallEngine") {
                protected void onLooperPrepared() {
                    log("create mVideoCallEngine");
                    synchronized (syncObj) {
                        mVideoCallEngine = new VideoCallEngine(ril, mContext,
                                (ImsConfigImpl)mImsService.getConfigInterface(serviceId));
                        syncObj.notifyAll();
                    }
                    log("create mVideoCallEngine done");
                }
            };
            mMediaPhoneThread.start();
            log("before wait mVideoCallEngine");
            synchronized (syncObj) {
                try {
                    syncObj.wait(1500);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
            mVideoCallCameraManager = new VideoCallCameraManager(mVideoCallEngine, mContext, this);
            if(mPeerVideoQuality != -1){
                mVideoCallCameraManager.updateVideoQuality(mPeerVideoQuality);
            }
            log("after wait mVideoCallEngine, mVideoCallEngine is null?:" + (mVideoCallEngine == null));
            log("wait mVideoCallEngine done.");
            /* SPRD: Add feature of low battery for Reliance @{ */
            registerForLowBatteryNotify(ril);
            /* @} */
        }

    }

    private void handleDisconnect(ImsCallSessionImpl imsCallSessionImpl) {
        log("handleDisconnect->imsCallSessionImpl=" + imsCallSessionImpl);
        if (imsCallSessionImpl != null && imsCallSessionImpl == mActiveImsCallSessionImpl) {
            onVTConnectionDisconnected();
        }
    }

    public void onVTConnectionDisconnected() {
        /* SPRD: Add feature of low battery for Reliance @{ */
        unregisterForLowBatteryNotify();
        if (mVolteMediaDialog != null) {
            mVolteMediaDialog.dismiss();
        }
        /* @} */
        if (!isImsCallAlive()) {
            log("No active video call!");
            return;
        }
        if (mVideoCallEngine != null) {
            mVideoCallEngine.releaseVideocallEngine();
            if(mVideoCallCameraManager != null){
                mVideoCallCameraManager.releaseVideoCamera();
            }
            mVideoCallCameraManager = null;
            mVideoCallEngine = null;
            mActiveImsCallSessionImpl = null;
            mPeerVideoQuality = -1;
            mPeerWidth = 480;
            mPeerHeight = 640;
            mPreviewWidth = 480;
            mPreviewHeight = 640;
            mRotation = -1;
            log("onVTConnectionDisconnected::mMediaPhoneThread.quit(): " + mMediaPhoneThread.quit());
        }
    }

    private void handleSetCamera(String cameraId) {
        log("handleSetCamera->cameraId=" + cameraId);
        mCameraId = cameraId;
        if (mVideoCallCameraManager == null) {
            log("handleSetCamera mVideoCallCameraManager is null!");
            return;
        }
        mVideoCallCameraManager.handleSetCamera(cameraId);
        //updateSessionModificationState();//bug493552
    }

    /* SPRD:bug493552 @{ */
    private void updateSessionModificationState() {
        if (mActiveImsCallSessionImpl == null) {
            log("updateSessionModificationState mActiveImsCallSessionImpl is null!");
            return;
        }
        ImsVideoCallProvider vtProvider = (ImsVideoCallProvider)mActiveImsCallSessionImpl
                .getImsVideoCallProvider();
        if (vtProvider != null) {
            log("updateSessionModificationState receiveSessionModifyResponse");
            vtProvider.receiveSessionModifyResponse(VideoProvider.SESSION_MODIFY_REQUEST_SUCCESS,
                    null, null);
        }
    }
    /* @} */

    private void handleSetPreviewSurface(Surface surface) {
        log("handleSetPreviewSurface->Surface=" + surface);
        mPreviewSurface = surface;
        if (mVideoCallEngine != null) {
        // SPRD: delete for bug 752052
/*            if (surface == null) {
                log("handleSetPreviewSurface->Clean up camera object");
                mVideoCallEngine.setImsCamera(null);
            }*/
            mVideoCallEngine.setImsLocalSurface(mPreviewSurface);
            mVideoCallEngine.startPreview();
        }
        if (mVideoCallCameraManager == null) {
            log("handleSetPreviewSurface-->mVideoCallCameraManager is null");
            return;
        }
        /* SPRD: add handleSetCameraPreSurface for bug 408181 @{ */
        mVideoCallCameraManager.handleSetCameraPreSurface(mPreviewSurface);
        /* @} */
    }

    private void handleSetDisplaySurface(Surface surface) {
        log("handleSetDisplaySurface->Surface=" + surface);
        mDisplaySurface = surface;
        if (mVideoCallEngine != null) {
            mVideoCallEngine.setImsRemoteSurface(mDisplaySurface);
        }
        if (surface != null){
            setPeerDimensions(mPeerWidth,mPeerHeight);
        }
    }

    private void handleSetDeviceOrientation(Integer rotation) {
        log("handleSetDeviceOrientation->rotation=" + rotation+"  mRotation="+mRotation);
        if (mVideoCallCameraManager == null) {
            log("handleSetDeviceOrientation-->mVideoCallCameraManager is null");
            return;
        }
        if (rotation != null && (mRotation != rotation)) {
            mRotation = rotation;
            mVideoCallCameraManager.onSetDeviceRotation(rotation.intValue());
        }
    }

    private void handleUpdateVideoQuality(Integer quality) {
        log("handleUpdateVideoQuality->quality=" + quality);
        mPeerVideoQuality = quality;
        if(mVideoCallCameraManager != null){
            mVideoCallCameraManager.updateVideoQuality(quality);
        }
    }

    private void handleVideoCallCodecEvent(AsyncResult asyncResult) {
        log("handleVideoCallCodecEvent->asyncResult=" + asyncResult);
    }

    private void handleVideoCallFail(AsyncResult asyncResult) {
        log("handleVideoCallFail->asyncResult=" + asyncResult);
        if (asyncResult != null) {
            GsmCdmaPhone phone = (GsmCdmaPhone) asyncResult.userObj;
            AsyncResult arO = (AsyncResult) asyncResult.result;
            AsyncResult ar = (AsyncResult) arO.result;
            boolean isIncomingCall = arO.userObj != null ? (((Integer) arO.userObj == 1) ? true
                    : false) : false;
            String number = null;
            Integer cause = null;
            if (ar != null) {
                number = ar.userObj != null ? (String) ar.userObj : null;
                cause = ar.result != null ? (Integer) ar.result : null;
                log("handleVideoCallFail, number: " + number + ", cause: " + cause);
            }
            if (isIncomingCall || number == null || cause == null) {
                log("handleVideoCallFail->don't show fail message because: isIncomingCall:"
                        + isIncomingCall +
                        " or number/cause is null.");
                return;
            }
            onVideoCallFailOrFallBack();
            VTManagerUtils.showVideoCallFailToast(mContext, cause.intValue());
            if (cause.intValue() == VTManagerUtils.VIDEO_CALL_NO_SERVICE ||
                    cause.intValue() == VTManagerUtils.VIDEO_CALL_CAPABILITY_NOT_AUTHORIZED) {
                showFallBackDialog(number, cause.intValue(), phone);
            }
        }
    }

    private void handleVideoCallFallBack(AsyncResult asyncResult) {
        log("handleVideoCallFallBack->asyncResult=" + asyncResult);
        if (asyncResult != null) {
            GsmCdmaPhone phone = (GsmCdmaPhone) asyncResult.userObj;
            AsyncResult arO = (AsyncResult) asyncResult.result;
            AsyncResult ar = (AsyncResult) arO.result;
            boolean isIncomingCall = arO.userObj != null ? (((Integer) arO.userObj == 1) ? true
                    : false) : false;
            String number = null;
            Integer cause = null;
            if (ar != null) {
                number = ar.userObj != null ? (String) ar.userObj : null;
                cause = ar.result != null ? (Integer) ar.result : null;
                log("handleVideoCallFail, number: " + number + ", cause: " + cause);
            }
            if (isIncomingCall || number == null || cause == null) {
                log("handleVideoCallFail->don't show fail message because: isIncomingCall:"
                        + isIncomingCall +
                        " or number/cause is null.");
                return;
            }
            onVideoCallFailOrFallBack();
            showFallBackDialog(number, cause.intValue(), phone);
        }
    }

    private void showFallBackDialog(String number, int cause, GsmCdmaPhone phone) {
        dismissFallBackDialog();
        AlertDialog.Builder builder = new AlertDialog.Builder(mContext.getApplicationContext());
        mFallBackDialog = builder.setView(VTManagerUtils.getVideoCallFallBackView
                (mContext.getApplicationContext(), number, cause, phone)).create();
        VTManagerUtils.initVideoCallFallBackDialog(mFallBackDialog,
                mContext.getApplicationContext(), number, cause, phone);
        mFallBackDialog.show();
    }

    public void dismissFallBackDialog() {
        if (mFallBackDialog != null) {
            mFallBackDialog.dismiss();
        }
    }

    private void onVideoCallFailOrFallBack() {
        if (mActiveImsCallSessionImpl != null || mActiveImsCallSessionImpl.isInCall()) {
            log("onVideoCallFailOrFallBack->hangup alive call.");
            mActiveImsCallSessionImpl.terminate(16);
        }
    }

    public boolean isImsCallAlive() {
        return mActiveImsCallSessionImpl != null;
    }

    public void setPreviewSize(int width, int height) {
        if (mActiveImsCallSessionImpl != null) {
            ImsVideoCallProvider vp = (ImsVideoCallProvider)mActiveImsCallSessionImpl
                    .getImsVideoCallProvider();
            if (vp != null) {
                log("setPreviewSize->width=" + width + " height=" + height);
                VideoProfile.CameraCapabilities cc = new VideoProfile.CameraCapabilities(width,
                        height, false, 0);
                vp.changeCameraCapabilities(cc);
            }
        }
    }

    public void setPeerDimensions(int width, int height){
        mPeerWidth = width;
        mPeerHeight = height;
        if (mActiveImsCallSessionImpl != null) {
            ImsVideoCallProvider vp = (ImsVideoCallProvider)mActiveImsCallSessionImpl
                    .getImsVideoCallProvider();
            if (vp != null) {
                log("setPeerDimensions->width=" + width + " height=" + height);
                VideoProfile.CameraCapabilities cc = new VideoProfile.CameraCapabilities(width,
                        height, false, 0);
                vp.changePeerDimensions(width,height);
            }
        }
    }


    private void handleImsQosReport(AsyncResult ar){
        if(ar == null || ar.result == null || ar.exception != null){
            log("handleImsQosReport : ar = "+ ar +"exception = " + (ar == null ? null : ar.exception)
                    + " ar.result= " + (ar == null ? null : ar.result));
            return;
        }
        int[] result = (int[]) ar.result;
        if(result.length >= 4){
            mImsVideoQos = result[3];
            log("handleImsQosReport : qos = "+mImsVideoQos);
        }
        log("handleImsQosReport : result.length = "+result.length + " mVideoCallEngine:"+ mVideoCallEngine);
    }

    private void log(String string) {
        android.util.Log.i(TAG, string);
    }

    public boolean hasPermission(Context context, String permission) {
        return context.checkSelfPermission(permission) == PackageManager.PERMISSION_GRANTED;
    }
    /*
     * public void onRequestPermissionsResult(int requestCode, String[] permissions, int[]
     * grantResults) { if (requestCode == CAMERA_PERMISSION_REQUEST_CODE) { if (grantResults.length
     * >= 1 && PackageManager.PERMISSION_GRANTED == grantResults[0]) { mHasCameraPermission = true;
     * } } }
     */
}


package com.spreadtrum.ims.vowifi;

import android.content.Context;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.preference.PreferenceManager;
import android.telecom.Connection.VideoProvider;
import android.telecom.VideoProfile;
import android.telecom.VideoProfile.CameraCapabilities;
import android.util.Log;
import android.view.Display;
import android.view.OrientationEventListener;
import android.view.Surface;
import android.view.WindowManager;

import com.android.ims.internal.ImsVideoCallProvider;
import com.spreadtrum.ims.vowifi.Utilities.Camera;
import com.spreadtrum.ims.vowifi.Utilities.Result;
import com.spreadtrum.ims.vowifi.Utilities.VideoQuality;

public class ImsVideoCallProviderImpl extends ImsVideoCallProvider {
    private static final String TAG =
            Utilities.getTag(ImsVideoCallProviderImpl.class.getSimpleName());

    private Context mContext;

    private ImsCallSessionImpl mCallSession;

    private int mAngle = -1;
    private int mScreenRotation = -1;
    private int mDeviceOrientation = -1;
    private int mVideoQualityLevel = -1;
    private boolean mWaitForModifyResponse = false;

    private String mCameraId = null;
    private Display mDisplay = null;
    private MyHandler mHandler = null;
    private Surface mPreviewSurface = null;
    private Surface mDisplaySurface = null;
    private SharedPreferences mPreferences = null;
    private CameraCapabilities mCameraCapabilities = null;
    private MyOrientationListener mOrientationListener = null;

    private static final int MSG_ROTATE = 0;
    private static final int MSG_START_CAMERA = 1;
    private static final int MSG_STOP_CAMERA = 2;
    private static final int MSG_SWITCH_CAMERA = 3;
    private static final int MSG_SET_DISPLAY_SURFACE = 4;
    private static final int MSG_SET_PREVIEW_SURFACE = 5;
    private static final int MSG_REQUEST_CAMERA_CAPABILITIES = 6;
    private static final int MSG_STOP_REMOTE_RENDER = 7;
    private static final int MSG_SEND_MODIFY_REQUEST = 8;
    private static final int MSG_SET_PAUSE_IMAGE = 9;
    private class MyHandler extends Handler {
        private int mRotateRetryTimes = 0;

        public MyHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            if (Utilities.DEBUG) Log.i(TAG, "Handle the message: " + msg.what);

            switch (msg.what) {
                case MSG_ROTATE: {
                    if (mCameraId != null) {
                        Log.d(TAG, "Handle the rotate message, the device orientation: "
                                + mDeviceOrientation + ", the angle: " + mAngle);
                        mCallSession.localRenderRotate(mCameraId, mAngle, mDeviceOrientation);
                        mCallSession.remoteRenderRotate(mAngle);
                        mRotateRetryTimes = 0;
                    } else if (mRotateRetryTimes < 5){
                        mHandler.sendEmptyMessageDelayed(MSG_ROTATE, 500);
                        mRotateRetryTimes = mRotateRetryTimes + 1;
                    }
                    break;
                }
                case MSG_START_CAMERA: {
                    synchronized (this) {
                        String cameraId = (String) msg.obj;
                        if (mCallSession.startCamera(cameraId) == Result.SUCCESS) {
                            mCameraId = cameraId;
                            // As set camera success, we'd like to request the camera capabilities.
                            mHandler.sendEmptyMessage(MSG_REQUEST_CAMERA_CAPABILITIES);
                        }

                        // Enable the rotate now.
                        mOrientationListener.enable();
                        break;
                    }
                }
                case MSG_STOP_CAMERA: {
                    synchronized (this) {
                        if (mCameraId == null) {
                            Log.d(TAG, "Camera already stopped, do nothing.");
                            break;
                        }

                        int res = Result.SUCCESS;
                        res = res & mCallSession.stopLocalRender(mPreviewSurface, mCameraId);
                        res = res & mCallSession.stopCamera();
                        res = res & mCallSession.stopCapture(mCameraId);

                        if (res == Result.FAIL) {
                            // Sometimes, we will stop the camera failed as the camera already
                            // disconnect. For example, refer to this log:
                            // "Disconnect called on already disconnected client for device 1"
                            Log.w(TAG, "The camera can not stopped now, please check the reason.");
                        }

                        // Reset the values.
                        mCameraId = null;
                        mCameraCapabilities = null;
                        mPreviewSurface = null;

                        // Disable the rotate now.
                        mOrientationListener.disable();
                        break;
                    }
                }
                case MSG_SWITCH_CAMERA: {
                    // For switch the camera, we'd like to split this action to two step:
                    // 1. stop the old camera
                    // 2. start the new camera
                    mHandler.sendEmptyMessage(MSG_STOP_CAMERA);
                    mHandler.sendMessage(mHandler.obtainMessage(MSG_START_CAMERA, msg.obj));
                    break;
                }
                case MSG_SET_DISPLAY_SURFACE: {
                    synchronized (this) {
                        Surface displaySurface = (Surface) msg.obj;
                        if (displaySurface != null) {
                            int res = mCallSession.startRemoteRender(displaySurface);
                            if (res == Result.SUCCESS) mDisplaySurface = displaySurface;
                        }
                        break;
                    }
                }
                case MSG_SET_PREVIEW_SURFACE: {
                    Surface previewSurface = (Surface) msg.obj;

                    int res = Result.SUCCESS;
                    // Start the capture and start the render.
                    VideoQuality quality = Utilities.findVideoQuality(getVideoQualityLevel());
                    res = res & mCallSession.startCapture(
                            mCameraId, quality._width, quality._height, quality._frameRate);
                    if (previewSurface != null) {
                        res = res & mCallSession.startLocalRender(previewSurface, mCameraId);
                    }

                    if (res == Result.SUCCESS) {
                        mPreviewSurface = previewSurface;
                    } else {
                        Log.w(TAG, "Can not set the preview surface now.");
                    }
                }
                case MSG_REQUEST_CAMERA_CAPABILITIES: {
                    CameraCapabilities cameraCapabilities = getCameraCapabilities();

                    // If the device rotate to 90 or 270, we need exchange the height and width.
                    if ((mDeviceOrientation == 90 || mDeviceOrientation == 270)
                            && cameraCapabilities != null) {
                        Log.d(TAG, "The current orientation is 90 or 270, adjest capabilities.");
                        cameraCapabilities = new CameraCapabilities(
                                cameraCapabilities.getHeight(), cameraCapabilities.getWidth());
                    }

                    if (!cameraCapabilitiesEquals(cameraCapabilities)) {
                        if (cameraCapabilities != null) {
                            Log.d(TAG, "Change the camera capabilities: width = "
                                    + cameraCapabilities.getWidth() + ", height = "
                                    + cameraCapabilities.getHeight());
                            changeCameraCapabilities(cameraCapabilities);
                        }
                        mCameraCapabilities = cameraCapabilities;
                    } else {
                        Log.d(TAG, "The old camera capabilities is same as the new one.");
                    }

                    break;
                }
                case MSG_STOP_REMOTE_RENDER: {
                    synchronized (this) {
                        if (mDisplaySurface == null) {
                            Log.e(TAG, "Failed to stop remote render, display surface is null.");
                            break;
                        }

                        int res = mCallSession.stopRemoteRender(mDisplaySurface, false);
                        if (res == Result.SUCCESS) {
                            // Stop the remote render success, set the display surface to null.
                            mDisplaySurface = null;
                        } else {
                            Log.w(TAG, "Can not stop remote render now.");
                        }
                        break;
                    }
                }
                case MSG_SEND_MODIFY_REQUEST: {
                    boolean isVideo = (Boolean) msg.obj;
                    if (mCallSession.sendModifyRequest(isVideo) == Result.FAIL) {
                        Log.w(TAG, "Can not send the modify request now.");
                        receiveSessionModifyResponse(
                                VideoProvider.SESSION_MODIFY_REQUEST_FAIL, null, null);
                    } else {
                        // Send the modify request successfully.
                        mWaitForModifyResponse = true;
                    }
                    break;
                }
                case MSG_SET_PAUSE_IMAGE: {
                    mCallSession.setPauseImage((Uri) msg.obj);
                    break;
                }
            }
        }
    }

    protected ImsVideoCallProviderImpl(Context context, VoWifiCallManager callManager,
            ImsCallSessionImpl callSession) {
        mContext = context;
        mCallSession = callSession;
        mOrientationListener = new MyOrientationListener(mContext);
        mPreferences = PreferenceManager.getDefaultSharedPreferences(mContext);

        WindowManager wm = ((WindowManager) mContext.getSystemService(Context.WINDOW_SERVICE));
        mDisplay = wm.getDefaultDisplay();

        HandlerThread thread = new HandlerThread("VideoCall");
        thread.start();
        Looper looper = thread.getLooper();
        mHandler = new MyHandler(looper);
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();

        if (mHandler != null) {
            mHandler.removeCallbacksAndMessages(null);
            Looper looper = mHandler.getLooper();
            if (looper != null) {
                looper.quit();
            }
            mHandler = null;
        }
    }

    @Override
    public void receiveSessionModifyResponse(int status, VideoProfile requestProfile,
            VideoProfile responseProfile) {
        super.receiveSessionModifyResponse(status, requestProfile, responseProfile);

        // As the status "VideoProvider.SESSION_MODIFY_REQUEST_INVALID" used to play the tone.
        // So do not change "mWaitForModifyResponse" here.
        if (status != VideoProvider.SESSION_MODIFY_REQUEST_INVALID) {
            mWaitForModifyResponse = false;
        }
    }

    /**
     * Issues a request to retrieve the data usage (in bytes) of the video portion of the
     * {@link RemoteConnection} for the {@link RemoteConnection.VideoProvider}.
     *
     * @see Connection.VideoProvider#onRequestConnectionDataUsage()
     */
    @Override
    public void onRequestCallDataUsage() {
        Log.d(TAG, "On request call data usage. Do not handle now.");
    }

    @Override
    public void onRequestCameraCapabilities() {
        if (Utilities.DEBUG) Log.i(TAG, "On request camera capabilities.");
        mHandler.sendEmptyMessage(MSG_REQUEST_CAMERA_CAPABILITIES);
    }

    @Override
    public void onSendSessionModifyRequest(VideoProfile fromProfile, VideoProfile toProfile) {
        if (Utilities.DEBUG) {
            Log.i(TAG, "On send session modify request. From profile: " + fromProfile
                    + ", to profile: " + toProfile);
        }

        boolean wasVideo = VideoProfile.isVideo(fromProfile.getVideoState());
        boolean isVideo = VideoProfile.isVideo(toProfile.getVideoState());
        if (wasVideo != isVideo) {
            // For video type changed, we need send the modify request to server.
            mHandler.sendMessage(mHandler.obtainMessage(MSG_SEND_MODIFY_REQUEST, isVideo));
        } else {
            // If the video type do not changed. we need handle the transmission changed.
            boolean wasTrans = VideoProfile.isTransmissionEnabled(fromProfile.getVideoState());
            boolean isTrans = VideoProfile.isTransmissionEnabled(toProfile.getVideoState());
            if (wasTrans == isTrans) {
                // FIXME: There must be some error in telephony, when the user press
                //        "resume video", the wasTrans is same as isTrans.
                Log.w(TAG, "The fromProfile is same as the toProfile's trans. It's abnormal.");
                Log.w(TAG, "Ignore this abnormal, change the status as toProfile.");
            }

            // For transmission changed, needn't send the modify request. So give the
            // response immediately.
            if (isTrans) {
                // It means start the video transmission. And "setCamera" will start the
                // camera, so we need request the camera capabilities
                Log.d(TAG, "Start the video transmission successfully.");
                receiveSessionModifyResponse(
                        VideoProvider.SESSION_EVENT_TX_START, fromProfile, toProfile);
            } else {
                // It means stop the video transmission. And this action will be handled
                // when the camera set to null.
                Log.d(TAG, "Stop the video transmission successfully.");
                receiveSessionModifyResponse(
                        VideoProvider.SESSION_EVENT_TX_STOP, fromProfile, toProfile);
            }
        }
    }

    @Override
    public void onSendSessionModifyResponse(VideoProfile responseProfile) {
        Log.d(TAG, "On send session modify response. Do not handle.");
    }

    @Override
    public void onSetCamera(String cameraId) {
        synchronized (this) {
            if (Utilities.DEBUG) {
                Log.i(TAG, "On set the camera from " + Camera.toString(mCameraId) + " to "
                        + Camera.toString(cameraId));
            }

            if (mCameraId != null && cameraId == null) {
                // Set the camera to null, it means stop the camera capture.
                mHandler.sendEmptyMessage(MSG_STOP_CAMERA);
            } else if (cameraId != null && !cameraId.equals(mCameraId)) {
                // Start the camera or switch the camera.
                if (mCameraId == null) {
                    // Start the camera.
                    mHandler.sendMessage(mHandler.obtainMessage(MSG_START_CAMERA, cameraId));
                } else {
                    mHandler.sendMessage(mHandler.obtainMessage(MSG_SWITCH_CAMERA, cameraId));
                }
            } else {
                // case: mCameraId == null && cameraId == null or cameraId equals mCameraId
                Log.d(TAG, "Set the camera to " + Camera.toString(cameraId)
                        + ", but the old camera is " + Camera.toString(mCameraId));
                if (cameraId == null) {
                    // If the new camera is null, and the old camera is null, it means the start
                    // camera action do not handle now. So we'd like to remove the start camera
                    // action to keep the last camera state as null.
                    mHandler.removeMessages(MSG_START_CAMERA);
                } else {
                    // If the new camera is front or back, and it is same as the old, it means the
                    // stop camera action do not handle now. So we'd like to remove the stop camera
                    // action to keep the last camera state.
                    mHandler.removeMessages(MSG_STOP_CAMERA);
                }
            }
        }
    }

    @Override
    public void onSetDeviceOrientation(int deviceOrientation) {
        // If the device orientation do not init, calculate the angle with this
        // given device orientation.
        if (mDeviceOrientation < 0) {
            calculateAngle(deviceOrientation);
        }
    }

    @Override
    public void onSetDisplaySurface(Surface surface) {
        synchronized (this) {
            if (Utilities.DEBUG) Log.i(TAG, "On set the display surface to: " + surface);
            if (mDisplaySurface != null) {
                mHandler.sendEmptyMessage(MSG_STOP_REMOTE_RENDER);
            }
            mHandler.sendMessage(mHandler.obtainMessage(MSG_SET_DISPLAY_SURFACE, surface));
        }
    }

    @Override
    public void onSetPauseImage(Uri uri) {
        if (Utilities.DEBUG) Log.i(TAG, "On set the pause image to: " + uri);
        mHandler.sendMessage(mHandler.obtainMessage(MSG_SET_PAUSE_IMAGE, uri));
    }

    @Override
    public void onSetPreviewSurface(Surface surface) {
        if (Utilities.DEBUG) Log.i(TAG, "On set the preview surface as: " + surface);
        mHandler.sendMessage(mHandler.obtainMessage(MSG_SET_PREVIEW_SURFACE, surface));
    }

    @Override
    public void onSetZoom(float arg0) {
        if (Utilities.DEBUG) Log.i(TAG, "On set the zoom to: " + arg0);
        // Do not support now.
    }

    public String getCameraId() {
        return mCameraId;
    }

    public Surface getDisplaySurface() {
        return mDisplaySurface;
    }

    public Surface getPreviewSurface() {
        return mPreviewSurface;
    }

    public CameraCapabilities getCurCameraCapabilities() {
        return mCameraCapabilities;
    }

    public boolean isWaitForModifyResponse() {
        return mWaitForModifyResponse;
    }

    public void stopAll() {
        synchronized (this) {
            Log.d(TAG, "Stop all the video action.");
            if (mDisplaySurface != null) {
                mHandler.sendEmptyMessage(MSG_STOP_REMOTE_RENDER);
            }

            if (mCameraId != null) {
                mHandler.sendEmptyMessage(MSG_STOP_CAMERA);
            }
        }
    }

    public void updateVideoQualityLevel(int newLevel) {
        synchronized (this) {
            Log.d(TAG, "Update the video quality level from " + mVideoQualityLevel
                    + " to " + newLevel);
            if (newLevel > 0 && newLevel != mVideoQualityLevel) {
                mVideoQualityLevel = newLevel;
                // As video quality level changed, we need stop camera & start camera again.
                String oldCameraId = mCameraId;
                if (mCameraId != null) {
                    mHandler.sendEmptyMessage(MSG_STOP_CAMERA);
                }

                // Start the camera with old camera.
                if (oldCameraId != null) {
                    mHandler.sendMessage(mHandler.obtainMessage(MSG_START_CAMERA, oldCameraId));
                }
            }
        }
    }

    private boolean cameraCapabilitiesEquals(CameraCapabilities capabilities) {
        if ((mCameraCapabilities == null && capabilities == null)
                || (mCameraCapabilities == capabilities)) {
            return true;
        } else if (mCameraCapabilities == null && capabilities != null) {
            return false;
        } else if (mCameraCapabilities != null && capabilities == null) {
            return false;
        } else {
            return mCameraCapabilities.getWidth() == capabilities.getWidth()
                    && mCameraCapabilities.getHeight() == capabilities.getHeight()
                    && mCameraCapabilities.getMaxZoom() == capabilities.getMaxZoom()
                    && mCameraCapabilities.isZoomSupported() == capabilities.isZoomSupported();
        }
    }

    private float getVideoQualityLevel() {
        if (mVideoQualityLevel < 0) {
            mVideoQualityLevel = mCallSession.getDefaultVideoLevel();
            if (mVideoQualityLevel < 0) {
                Log.w(TAG, "Can not get the default video level, set it as default.");
                mVideoQualityLevel = Utilities.getDefaultVideoQuality(mPreferences)._level;
            }
        }

        return mVideoQualityLevel;
    }

    private CameraCapabilities getCameraCapabilities() {
        float videoLevel = getVideoQualityLevel();
        VideoQuality quality = Utilities.findVideoQuality(videoLevel);
        if (quality == null) {
            quality = Utilities.getDefaultVideoQuality(mPreferences);
        }

        return new CameraCapabilities(quality._width, quality._height);
    }

    private void calculateAngle(int deviceOrientation) {
        int screenRotation = mDisplay.getRotation();
        if (deviceOrientation == mDeviceOrientation
                && screenRotation == mScreenRotation) {
            // The old device orientation is same as the new. Do nothing.
            return;
        }

        // Update the device orientation and angle.
        if (Utilities.DEBUG) {
            Log.i(TAG, "Orientation changed, deviceOrientation = " + deviceOrientation
                    + ", screenRotation = " + screenRotation);
        }

        mDeviceOrientation = deviceOrientation;
        mScreenRotation = screenRotation;
        int angle = ((360 - mDeviceOrientation) + (360 - mScreenRotation * 90)) % 360;
        Log.d(TAG, "Get the new angle: " + angle + ", and the old angle is: " + mAngle);

        if (mAngle != angle) {
            mAngle = angle;
            mHandler.sendEmptyMessage(MSG_ROTATE);
            Log.d(TAG, "Send the rotate message for new angle: " + mAngle);
        }
    }

    private class MyOrientationListener extends OrientationEventListener {
        public MyOrientationListener(Context context) {
            super(context);
        }

        @Override
        public void disable() {
            super.disable();

            // If this listener is disable, reset the device orientation, screen rotation and angle.
            mAngle = -1;
            mScreenRotation = -1;
            mDeviceOrientation = -1;
        }

        @Override
        public void onOrientationChanged(int orientation) {
            // FIXME: Sometimes, the orientation is -1. Do nothing.
            if (orientation < 0) {
                return;
            }

            // This will be same as the VT manager.
            int deviceOrientation = mDeviceOrientation;
            if (orientation >= 350 || orientation <= 10) {
                deviceOrientation = 0;
            } else if (orientation >= 80 && orientation <= 110) {
                deviceOrientation = 90;
            } else if (orientation >= 170 && orientation <= 190) {
                deviceOrientation = 180;
            } else if (orientation >= 260 && orientation <= 280) {
                deviceOrientation = 270;
            }

            calculateAngle(deviceOrientation);
        }
    }

}

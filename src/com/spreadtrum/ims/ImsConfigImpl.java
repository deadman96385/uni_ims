package com.spreadtrum.ims;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.AsyncResult;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.RemoteException;
import android.os.SystemProperties;
import android.preference.PreferenceManager;
import android.util.Log;
import com.android.ims.ImsConfigListener;
import com.android.ims.internal.IImsConfig;
import com.android.ims.ImsConfig;
import com.android.internal.telephony.CommandsInterface;
import android.telephony.TelephonyManager;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;

public class ImsConfigImpl extends IImsConfig.Stub {

    private static final String TAG = ImsConfigImpl.class.getSimpleName();
    public static final String VT_RESOLUTION_VALUE = "vt_resolution";

    private static final int ACTION_GET_VT_RESOLUTION = 101;
    private static final int ACTION_SET_VT_RESOLUTION = 102;
    private static final int ACTION_GET_IMS_CALL_AVAILABILITY = 103;
    private static final int ACTION_SET_IMS_CALL_AVAILABILITY = 104;
    private static final int EVENT_VOLTE_CALL_DEDINE_MEDIA_TYPE = 105;

    public static final int VT_RESOLUTION_720P = 0;                //1280*720 Frame rate:30
    public static final int VT_RESOLUTION_VGA_REVERSED_15 = 1;     //480*640 Frame rate:15
    public static final int VT_RESOLUTION_VGA_REVERSED_30 = 2;     //480*640 Frame rate:30
    public static final int VT_RESOLUTION_QVGA_REVERSED_15 = 3;    //240*320 Frame rate:15
    public static final int VT_RESOLUTION_QVGA_REVERSED_30 = 4;    //240*320 Frame rate:30
    public static final int VT_RESOLUTION_CIF = 5;                 //352*288 Frame rate:30
    public static final int VT_RESOLUTION_QCIF = 6;                //176*144 Frame rate:30
    public static final int VT_RESOLUTION_VGA_15 = 7;              //640*480 Frame rate:15
    public static final int VT_RESOLUTION_VGA_30 = 8;              //640*480 Frame rate:30
    public static final int VT_RESOLUTION_QVGA_15 = 9;             //320*240 Frame rate:15
    public static final int VT_RESOLUTION_QVGA_30 = 10;            //320*240 Frame rate:30

    public static class VideoQualityConstants {
        public static final int FEATURE_VT_RESOLUTION = 50;
        public static final int NETWORK_VT_RESOLUTION = 51;
    }

    private ImsRIL mCi;
    private ImsHandler mHandler;
    private Context mContext;
    private SharedPreferences mSharedPreferences;
    private static final String VIDEO_CALL_RESOLUTION = "vt_resolution";
    // change default to 15
    private int mCameraResolution = VT_RESOLUTION_QVGA_REVERSED_30;
    public int mDefaultVtResolution = VT_RESOLUTION_QVGA_REVERSED_30;

    /**
     * Creates the Ims Config interface object for a sub.
     * @param senderRxr
     */
    public ImsConfigImpl(ImsRIL ci,Context context) {
        mCi = ci;
        mHandler = new ImsHandler(context.getMainLooper());
        mContext = context;
        mSharedPreferences = PreferenceManager.getDefaultSharedPreferences(context);
        mSharedPreferences.registerOnSharedPreferenceChangeListener(mSharedPreferenceListener);
        mCameraResolution = mSharedPreferences.getInt(VIDEO_CALL_RESOLUTION, mDefaultVtResolution);
        mHandler.removeMessages(EVENT_VOLTE_CALL_DEDINE_MEDIA_TYPE);
        mHandler.sendEmptyMessageDelayed(EVENT_VOLTE_CALL_DEDINE_MEDIA_TYPE, 1000);
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
            Log.d(TAG, "handleMessage what:" + msg.what +" msg.obj:"+msg.obj);
            switch (msg.what) {
                case ACTION_GET_VT_RESOLUTION:
                    try {
                        ImsConfigListener imsConfigListener = (ImsConfigListener)msg.obj;
                        int status = ImsConfig.OperationStatusConstants.SUCCESS;
                        int result = msg.arg1;
                        Log.i(TAG, "ACTION_GET_VT_RESOLUTION->status:"+status+" result:"+result);
                        if(imsConfigListener != null){
                            imsConfigListener.onGetVideoQuality(status,result);
                        } else {
                            Log.w(TAG, "ACTION_GET_VT_RESOLUTION->imsConfigListener is null!");
                        }
                    } catch(RemoteException e){
                        e.printStackTrace();
                    }
                    break;
                case ACTION_SET_VT_RESOLUTION:
                    try {
                        ImsConfigListener imsConfigListener = (ImsConfigListener)msg.obj;
                        if(imsConfigListener != null){
                            imsConfigListener.onSetVideoQuality(ImsConfig.OperationStatusConstants.SUCCESS);
                        } else {
                            Log.w(TAG, "ACTION_GET_VT_RESOLUTION->imsConfigListener is null!");
                        }
                    } catch(RemoteException e){
                        e.printStackTrace();
                    }
                    break;
                case ACTION_GET_IMS_CALL_AVAILABILITY:
                    try {
                        AsyncResult ar = (AsyncResult) msg.obj;
                        if(ar != null){
                            ImsConfigListener imsConfigListener = (ImsConfigListener)ar.userObj;
                            int status = -1;
                            int result = -1;
                            if(ar.exception != null || ar.userObj instanceof Throwable){
                                status = ImsConfig.OperationStatusConstants.FAILED;
                            } else {
                                status = ImsConfig.OperationStatusConstants.SUCCESS;
                                int[] results = (int[])ar.result;
                                if(results != null){
                                    result = results[0];
                                }
                            }
                            Log.i(TAG, "ACTION_GET_IMS_CALL_AVAILABILITY->status:"+status+" result:"+result);
                            if(imsConfigListener != null){
                                imsConfigListener.onGetFeatureResponse(ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE,
                                        TelephonyManager.NETWORK_TYPE_LTE, result, status);
                            } else {
                                Log.w(TAG, "ACTION_GET_IMS_CALL_AVAILABILITY->imsConfigListener is null!");
                            }
                        }
                    } catch(RemoteException e){
                        e.printStackTrace();
                    }
                    break;
                case ACTION_SET_IMS_CALL_AVAILABILITY:
                    try {
                        AsyncResult ar = (AsyncResult) msg.obj;
                        if(ar != null){
                            ImsConfigListener imsConfigListener = (ImsConfigListener)ar.userObj;
                            int status = -1;
                            int result = -1;
                            if(ar.exception != null){
                                status = ImsConfig.OperationStatusConstants.FAILED;
                            } else {
                                status = ImsConfig.OperationStatusConstants.SUCCESS;
                            }
                            Log.i(TAG, "ACTION_SET_IMS_CALL_AVAILABILITY->status:"+status+" result:"+result);
                            if(imsConfigListener != null){
                                imsConfigListener.onSetFeatureResponse(ImsConfig.FeatureConstants.FEATURE_TYPE_VOICE_OVER_LTE,
                                        TelephonyManager.NETWORK_TYPE_LTE, result, status);
                            } else {
                                Log.w(TAG, "ACTION_SET_IMS_CALL_AVAILABILITY->imsConfigListener is null!");
                            }
                        }
                    } catch(RemoteException e){
                        e.printStackTrace();
                    }
                    break;
                case EVENT_VOLTE_CALL_DEDINE_MEDIA_TYPE:
                    String[] cmd=new String[1];
                    mCameraResolution = VT_RESOLUTION_QVGA_REVERSED_30;
                    cmd[0] = "AT+CDEFMP=1,\""+mCameraResolution+"\"";
                    Log.i(TAG,"EVENT_VOLTE_CALL_DEDINE_MEDIA_TYPE->cmd[0]:"+cmd[0]);
                    mCi.invokeOemRilRequestStrings(cmd, null);
                    break;
                default:
                    Log.e(TAG, "handleMessage: unhandled message");
            }

        }

    };


    /**
     * Gets the value for ims service/capabilities parameters from the provisioned
     * value storage. Synchronous blocking call.
     *
     * @param item, as defined in com.android.ims.ImsConfig#ConfigConstants.
     * @return value in Integer format.
     */
    @Override
    public int getProvisionedValue(int item){
        return 0;
    }

    /**
     * Gets the value for ims service/capabilities parameters from the provisioned
     * value storage. Synchronous blocking call.
     *
     * @param item, as defined in com.android.ims.ImsConfig#ConfigConstants.
     * @return value in String format.
     */
    @Override
    public String getProvisionedStringValue(int item){
        return null;
    }

    /**
     * Sets the value for IMS service/capabilities parameters by the operator device
     * management entity. It sets the config item value in the provisioned storage
     * from which the master value is derived. Synchronous blocking call.
     *
     * @param item, as defined in com.android.ims.ImsConfig#ConfigConstants.
     * @param value in Integer format.
     * @return as defined in com.android.ims.ImsConfig#OperationStatusConstants.
     */
    @Override
    public int setProvisionedValue(int item, int value){
        return 0;
    }

    /**
     * Sets the value for IMS service/capabilities parameters by the operator device
     * management entity. It sets the config item value in the provisioned storage
     * from which the master value is derived.  Synchronous blocking call.
     *
     * @param item, as defined in com.android.ims.ImsConfig#ConfigConstants.
     * @param value in String format.
     * @return as defined in com.android.ims.ImsConfig#OperationStatusConstants.
     */
    @Override
    public int setProvisionedStringValue(int item, String value){
        return 0;
    }

    /**
     * Gets the value of the specified IMS feature item for specified network type.
     * This operation gets the feature config value from the master storage (i.e. final
     * value). Asynchronous non-blocking call.
     *
     * @param feature. as defined in com.android.ims.ImsConfig#FeatureConstants.
     * @param network. as defined in android.telephony.TelephonyManager#NETWORK_TYPE_XXX.
     * @param listener. feature value returned asynchronously through listener.
     * @return void
     */
    @Override
    public void getFeatureValue(int feature, int network, ImsConfigListener listener){
        if(feature == VideoQualityConstants.FEATURE_VT_RESOLUTION
                && network == VideoQualityConstants.NETWORK_VT_RESOLUTION){
            if(listener != null){
                getVideoQuality(listener);
            }
        }
    }

    /**
     * Sets the value for IMS feature item for specified network type.
     * This operation stores the user setting in setting db from which master db
     * is dervied.
     *
     * @param feature. as defined in com.android.ims.ImsConfig#FeatureConstants.
     * @param network. as defined in android.telephony.TelephonyManager#NETWORK_TYPE_XXX.
     * @param value. as defined in com.android.ims.ImsConfig#FeatureValueConstants.
     * @param listener, provided if caller needs to be notified for set result.
     * @return void
     */
    @Override
    public void setFeatureValue(int feature, int network, int value, ImsConfigListener listener){
        if(feature == VideoQualityConstants.FEATURE_VT_RESOLUTION
                && network == VideoQualityConstants.NETWORK_VT_RESOLUTION){
            if(listener != null){
                setVideoQuality(value,listener);
            }
        }
    }

    /**
     * Gets the value for IMS volte provisioned.
     * This should be the same as the operator provisioned value if applies.
     *
     * @return void
     */
    @Override
    public boolean getVolteProvisioned(){
        return isVolteEnabledBySystemProperties();
    }


    /**
     *
     * Gets the value for ims fature item video quality.
     *
     * @param listener. Video quality value returned asynchronously through listener.
     * @return void
     */
    @Override
    public void getVideoQuality(ImsConfigListener imsConfigListener) {
        Log.d(TAG, "getVideoQuality");
        Message m = mHandler.obtainMessage(ACTION_GET_VT_RESOLUTION, getVideoQualityFromPreference(),
                0, imsConfigListener);
        m.sendToTarget();
    }

    /**
     * Sets the value for IMS feature item video quality.
     *
     * @param quality, defines the value of video quality.
     * @param listener, provided if caller needs to be notified for set result.
     * @return void
     *
     * @throws ImsException if calling the IMS service results in an error.
     */
    @Override
    public void setVideoQuality(int quality, ImsConfigListener imsConfigListener) {
        Log.d(TAG, "setVideoQuality qualiy = " + quality);
        setVideoQualitytoPreference(quality);
        Message m = mHandler.obtainMessage(ACTION_SET_VT_RESOLUTION, quality, 0, imsConfigListener);
        m.sendToTarget();
    }

    public void setVideoQualitytoPreference(int value){
        Editor editor = mSharedPreferences.edit();
        editor.putInt(VT_RESOLUTION_VALUE,value);
        editor.apply();
    }

    public int getVideoQualityFromPreference(){
        return mSharedPreferences.getInt(VT_RESOLUTION_VALUE, mDefaultVtResolution);
    }

    public static boolean isVolteEnabledBySystemProperties(){
        return SystemProperties.getBoolean("persist.sys.volte.enable", false);
    }

    private OnSharedPreferenceChangeListener mSharedPreferenceListener = new OnSharedPreferenceChangeListener() {
        public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
            Log.d(TAG,"onSharedPreferenceChanged()->key:"+key);
            if(VIDEO_CALL_RESOLUTION.equals(key)){
                mCameraResolution = sharedPreferences.getInt(VIDEO_CALL_RESOLUTION, mDefaultVtResolution);
                mHandler.removeMessages(EVENT_VOLTE_CALL_DEDINE_MEDIA_TYPE);
                mHandler.sendEmptyMessageDelayed(EVENT_VOLTE_CALL_DEDINE_MEDIA_TYPE, 1000);
                Log.d(TAG,"onSharedPreferenceChanged()->mCameraResolution:"+mCameraResolution);
            }
        }
    };
}

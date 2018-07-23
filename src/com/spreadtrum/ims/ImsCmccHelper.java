package com.spreadtrum.ims;

import com.spreadtrum.ims.ImsCallSessionImpl;
import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import android.os.Message;
import com.spreadtrum.ims.R;
import com.spreadtrum.ims.ImsRIL;
import android.telephony.CarrierConfigManager;
import android.telephony.CarrierConfigManagerEx;
import android.telephony.SubscriptionManager;

public class ImsCmccHelper {

    private static final String TAG = "ImsCmccHelper";
    static ImsCmccHelper sInstance;
    private Context mContext;

    public static ImsCmccHelper getInstance(Context context) {
        Log.d(TAG, "getInstance...... ");
        if (sInstance == null) {
            AddonManager addonManager = new AddonManager(context);
            sInstance = (ImsCmccHelper) addonManager.getAddon(R.string.ims_cmcc_ImsCmccHelper, ImsCmccHelper.class);
            Log.d(TAG, "getInstance [" + sInstance + "]");
        }
        sInstance.setContext(context);
        return sInstance;
    }
    public ImsCmccHelper() {
    }
    public void setContext(Context context) {
        mContext = context;
    }

    //add for unisoc bug 900271
    public boolean isCmccNetwork(int serviceId) {
        CarrierConfigManager configManager = (CarrierConfigManager)mContext.getSystemService(mContext.CARRIER_CONFIG_SERVICE);
        if (configManager != null && configManager.getConfigForPhoneId(serviceId-1) != null) {
            return configManager.getConfigForPhoneId(serviceId-1).getBoolean(
                    CarrierConfigManagerEx.KEY_CARRIER_SUPPORTS_VIDEO_CALL_ONLY);
        }
        return false;
    }

    public boolean rejectMediaChange(ImsCallSessionImpl session) {
        //add for unisoc bug 900271
        if (isCmccNetwork(session.getServiceId()) && session != null && session.isHasBackgroundCallAndActiveCall()) {
            return true;
        }
        return false;
    }
}

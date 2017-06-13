package com.spreadtrum.ims;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;

/**
 * Created by SPREADTRUM on 5/12/17.
 */
public class ImsVodafoneHelper {

    private static final String TAG = "ImsVodafoneHelper";
    static ImsVodafoneHelper sInstance;

    public static ImsVodafoneHelper getInstance(Context context) {
        Log.d(TAG, "getInstance...... ");
        if (sInstance == null) {
            AddonManager addonManager = new AddonManager(context);
            sInstance = (ImsVodafoneHelper) addonManager.getAddon(R.string.ims_vodafone_ImsVodafonePlugin, ImsVodafoneHelper.class);
            Log.d(TAG, "getInstance [" + sInstance + "]"+context.getString(R.string.ims_vodafone_ImsVodafonePlugin));
        }
        return sInstance;
    }

    public ImsVodafoneHelper() {
    }

    public boolean showVowifiRegisterToast(final Context context) {

        Log.d(TAG, "not showVowifiRegisterToast");
        return  false;
    }

}

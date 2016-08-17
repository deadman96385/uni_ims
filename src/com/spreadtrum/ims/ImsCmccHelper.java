package com.spreadtrum.ims;

import com.spreadtrum.ims.ImsCallSessionImpl;
import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import android.os.Message;
import com.spreadtrum.ims.R;
import com.spreadtrum.ims.ImsRIL;

public class ImsCmccHelper {

    private static final String TAG = "ImsCmccHelper";
    static ImsCmccHelper sInstance;

    public static ImsCmccHelper getInstance(Context context) {
        Log.d(TAG, "getInstance...... ");
        if (sInstance == null) {
            AddonManager addonManager = new AddonManager(context);
            sInstance = (ImsCmccHelper) addonManager.getAddon(R.string.ims_cmcc_ImsCmccHelper, ImsCmccHelper.class);
            Log.d(TAG, "getInstance [" + sInstance + "]");
        }
        return sInstance;
    }

    public ImsCmccHelper() {
    }

    public boolean rejectMediaChange(ImsCallSessionImpl imsCallSessionImpl,final ImsRIL mCi, Message response) {
        Log.d(TAG, "not Cmcc project...... ");
        return false;
    }
}

//package com.spreadtrum.ims.vt;
//
//import com.spreadtrum.ims.ImsCallSessionImpl;
//
//
//import android.app.AddonManager;
//import android.content.Context;
//import android.util.Log;
//import android.os.Message;
//import com.spreadtrum.ims.R;
//
//import com.android.internal.telephony.CommandsInterface;
//
//public class ImsCmccUtils {
//    private static final String TAG = "ImsCmccUtils";
//    static ImsCmccUtils sInstance;
//
//    public static ImsCmccUtils getInstance(Context context) {
//        Log.d(TAG, "getInstance...... ");
//        if (sInstance == null) {
//            AddonManager addonManager = new AddonManager(context);
//            sInstance = (ImsCmccUtils) addonManager.getAddon(R.string.ims_cmcc_ImsCmccUtils, ImsCmccUtils.class);
//            Log.d(TAG, "getInstance [" + sInstance + "]");
//        }
//        return sInstance;
//    }
//
//    public ImsCmccUtils() {
//    }
//
//    public boolean rejectMediaChange(ImsCallSessionImpl imsCallSessionImpl,final CommandsInterface mCi, Message response) {
//        Log.d(TAG, "not Cmcc project...... ");
//        return false;
//    }
//}

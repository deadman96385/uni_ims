
package com.spreadtrum.ims.vt;

import android.app.AddonManager;
import android.util.Log;
import com.spreadtrum.ims.R;
import com.spreadtrum.ims.vt.ImsVideoCallProvider;
import android.os.Handler;
import android.os.Message;
import com.android.internal.telephony.CommandsInterface;
import android.content.Context;

public class CancleVideoRequestUtils {
    private static final String TAG = "CancleVideoRequestUtils";
    static CancleVideoRequestUtils sInstance;

    public static CancleVideoRequestUtils getInstance(Context context) {
        Log.d(TAG, "enter CancleVideoRequestUtils");
        if (sInstance != null)
            return sInstance;

        AddonManager addonManager = new AddonManager(context);

        sInstance = (CancleVideoRequestUtils) addonManager.getAddon(
                R.string.feature_cancle_video_request, CancleVideoRequestUtils.class);
        return sInstance;
    }

    public void sendMessage(Handler handler, int messageId, CommandsInterface ci) {
        Log.d(TAG, "sendMessage handler =" + handler);
    }

    public void handMessage(int callId,Message msg) {

        Log.d(TAG, "handMessage callId =" + callId+ "msg = "+msg);
    }

    public void removeMessage(Handler handler, int messageId) {
        Log.d(TAG, "removeMessage messageId =" + messageId);
    }
}

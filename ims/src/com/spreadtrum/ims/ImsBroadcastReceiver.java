package com.spreadtrum.ims;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.os.SystemProperties;
import android.util.Log;
import com.android.internal.telephony.TelephonyIntents;

public class ImsBroadcastReceiver extends BroadcastReceiver {

    private static final String TAG = ImsBroadcastReceiver.class.getSimpleName();
    @Override
    public void onReceive(Context context, Intent intent) {
        if (Intent.ACTION_BOOT_COMPLETED.equals(intent.getAction())) {
            Log.i(TAG, "ACTION_BOOT_COMPLETED.");
        } else if(TelephonyIntents.ACTION_SIM_STATE_CHANGED.equals(intent.getAction())){
            Log.i(TAG, "ACTION_SIM_STATE_CHANGED.");
        } else {
            Log.e(TAG, "Received Intent: " + intent.toString());
        }
    }
}
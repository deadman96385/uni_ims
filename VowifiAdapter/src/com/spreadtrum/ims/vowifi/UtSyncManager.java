package com.spreadtrum.ims.vowifi;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.RemoteException;
import android.provider.Settings;
import android.sim.SimManager;
import android.telephony.TelephonyManager;
import android.util.Log;

import com.android.ims.ImsManager;
import com.android.ims.internal.IImsServiceEx;
import com.android.internal.telephony.IccCardConstants;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.TelephonyIntents;

public class UtSyncManager {
    private static final String TAG = Utilities.getTag(UtSyncManager.class.getSimpleName());

    private static UtSyncManager sInstance = null;

    private boolean mSynced = false;
    private boolean mSyncedWhenAMOff = false;

    private Context mContext;
    private MyHandler mHandler;
    private AirplaneModeReceiver mReceiver = new AirplaneModeReceiver();

    private static final int MSG_SYNC = 0;
    private static final int MSG_SYNC_CW = 1;
    private static final int MSG_SYNC_CLIR = 2;
    private class MyHandler extends Handler {

        public MyHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_SYNC:
                    sync();
                    break;
                case MSG_SYNC_CW:
                    queryCWStatus();
                    break;
                case MSG_SYNC_CLIR:
                    queryCLIRStatus();
                    break;
            }
        }

    }

    public static UtSyncManager getInstance(Context context) {
        if (sInstance == null) {
            sInstance = new UtSyncManager(context);
        }
        return sInstance;
    }

    private UtSyncManager(Context context) {
        mContext = context;

        HandlerThread thread = new HandlerThread("SyncManager");
        thread.start();
        Looper looper = thread.getLooper();
        mHandler = new MyHandler(looper);
    }

    public void sync() {
        boolean isAirplaneModeOff = isAirplaneModeOff();
        Log.d(TAG, "Try to sync the UT items. mSynced[" + mSynced + "], mSyncedWhenAMOff["
                + mSyncedWhenAMOff + "], isAirplaneModeOff[" + isAirplaneModeOff + "]");

        if (!mSynced
                || (isAirplaneModeOff && !mSyncedWhenAMOff)) {
            mHandler.sendEmptyMessage(MSG_SYNC_CW);
            mHandler.sendEmptyMessage(MSG_SYNC_CLIR);

            mSynced = true;
            if (isAirplaneModeOff) {
                mSyncedWhenAMOff = true;
            } else {
                // First sync action, but the airplane mode is on. We'd like to sync when
                // the airplane mode changed to off.
                mContext.registerReceiver(
                        mReceiver, new IntentFilter(Intent.ACTION_AIRPLANE_MODE_CHANGED));
            }
        }
    }

    private void queryCWStatus() {
        Log.d(TAG, "Start query CW status.");
        try {
            IImsServiceEx imsServiceEx = ImsManager.getIImsServiceEx();
            if (imsServiceEx != null) {
                Log.d(TAG, "To get the CLIR mode from CP.");
                imsServiceEx.getCallWaitingStatus(Utilities.getPrimaryCard(mContext));
            }
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to get the CLIR statue as catch the RemoteException: "
                    + e.toString());
        }
    }

    private void queryCLIRStatus() {
        Log.d(TAG, "Start query CLIR status.");
        try {
            IImsServiceEx imsServiceEx = ImsManager.getIImsServiceEx();
            if (imsServiceEx != null) {
                Log.d(TAG, "To get the CLIR mode from CP.");
                imsServiceEx.getCLIRStatus(Utilities.getPrimaryCard(mContext));
            }
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to get the CLIR statue as catch the RemoteException: "
                    + e.toString());
        }
    }

    private boolean isAirplaneModeOff() {
        return Settings.Global.getInt(mContext.getContentResolver(),
                Settings.Global.AIRPLANE_MODE_ON, 0) == 0;
    }

    private class AirplaneModeReceiver extends BroadcastReceiver {

        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent == null) return;

            Log.d(TAG, "Receive the intent: " + intent.getAction());
            if (Intent.ACTION_AIRPLANE_MODE_CHANGED.equals(intent.getAction())) {
                boolean airplaneMode = intent.getBooleanExtra("state", false);
                if (!airplaneMode) {
                    // Airplane mode is off, register the radio state changed.
                    mContext.registerReceiver(mReceiver,
                            new IntentFilter(TelephonyIntents.ACTION_SIM_STATE_CHANGED));
                }
            } else if (TelephonyIntents.ACTION_SIM_STATE_CHANGED.equals(intent.getAction())) {
                int phoneId = intent.getIntExtra(
                        PhoneConstants.SLOT_KEY, PhoneConstants.SIM_ID_1);
                if (!SimManager.isValidPhoneId(phoneId)
                        || phoneId != TelephonyManager.getDefaultPhoneId()) {
                    Log.w(TAG, "Do nothing as the phone id is invalid, phoneId: " + phoneId);
                    return;
                }

                String simState = intent.getStringExtra(IccCardConstants.INTENT_KEY_ICC_STATE);
                Log.d(TAG, "Get the new sim state[" + simState + "] for the phone: " + phoneId);
                if (IccCardConstants.INTENT_VALUE_ICC_LOADED.equalsIgnoreCase(simState)) {
                    // Only start the sync action for primary card ready.
                    mHandler.sendEmptyMessage(MSG_SYNC);

                    Log.d(TAG, "Will start the sync action as primary card ready.");
                    mContext.unregisterReceiver(mReceiver);
                }
            }
        }

    }
}

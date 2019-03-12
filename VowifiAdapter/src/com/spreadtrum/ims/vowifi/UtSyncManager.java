package com.spreadtrum.ims.vowifi;

import android.content.Context;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.RemoteException;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.util.Log;

import com.android.ims.internal.IImsServiceEx;
import com.android.ims.internal.ImsManagerEx;

import java.util.ArrayList;

public class UtSyncManager {
    private static final String TAG = Utilities.getTag(UtSyncManager.class.getSimpleName());

    private static UtSyncManager sInstance = null;

    private int mCurSubId = -1;
    private Context mContext;
    private MyHandler mHandler;
    private ArrayList<SyncState> mPhoneSyncState = null;

    private static final int MSG_SYNC_CW = 1;
    private static final int MSG_SYNC_CLIR = 2;
    private class MyHandler extends Handler {

        public MyHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
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

    public void sync(int subId) {
        SyncState state = findSyncState(subId);
        if (state == null) {
            Log.e(TAG, "Failed to sync as can't find the matched sync state for the sub: " + subId);
            return;
        }

        if (mCurSubId != subId) {
            // There is only one prop to store the SS CW/CLIR status, so if we will sync
            // the UT state on the another sub. Need clear the sync state of the old sub.
            SyncState oldState = findSyncState(mCurSubId);
            if (oldState != null) oldState.reset();

            mCurSubId = subId;
        }

        boolean isAirplaneModeOff = Utilities.isAirplaneModeOff(mContext);
        Log.d(TAG, "Try to sync the UT items for the sub: " + subId + ", synced[" + state._synced
                + "], syncedWhenAMOff[" + state._syncedWhenAMOff + "], isAirplaneModeOff["
                + isAirplaneModeOff + "]");

        if (!state._synced
                || (isAirplaneModeOff && !state._syncedWhenAMOff)) {
            mHandler.sendEmptyMessage(MSG_SYNC_CW);
            mHandler.sendEmptyMessage(MSG_SYNC_CLIR);

            state._synced = true;
            if (isAirplaneModeOff) {
                state._syncedWhenAMOff = true;
            }
        }
    }

    private void queryCWStatus() {
        Log.d(TAG, "Start query CW status.");
        try {
            IImsServiceEx imsServiceEx = ImsManagerEx.getIImsServiceEx();
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
            IImsServiceEx imsServiceEx = ImsManagerEx.getIImsServiceEx();
            if (imsServiceEx != null) {
                Log.d(TAG, "To get the CLIR mode from CP.");
                imsServiceEx.getCLIRStatus(Utilities.getPrimaryCard(mContext));
            }
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to get the CLIR statue as catch the RemoteException: "
                    + e.toString());
        }
    }

    private SyncState findSyncState(int subId) {
        if (mPhoneSyncState == null) {
            // Init the sync state first.
            mPhoneSyncState = new ArrayList<SyncState>();
            int phoneCount = new TelephonyManager(mContext).getPhoneCount();
            for (int i = 0; i < phoneCount; i++) {
                mPhoneSyncState.add(new SyncState(i));
            }
        }

        if (subId < 0 || mPhoneSyncState.size() < 0) {
            return null;
        }

        for (SyncState state : mPhoneSyncState) {
            if (state._subId == subId) {
                return state;
            }
        }

        return null;
    }

    private class SyncState {
        public final int _subId;
        public boolean _synced = false;
        public boolean _syncedWhenAMOff = false;

        public SyncState(int phoneId) {
            _subId = Utilities.getSubId(phoneId);
        }

        public void reset() {
            _synced = false;
            _syncedWhenAMOff = false;
        }
    }

}

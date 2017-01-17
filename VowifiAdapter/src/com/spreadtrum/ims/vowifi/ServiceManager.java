package com.spreadtrum.ims.vowifi;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.util.Log;

import com.spreadtrum.ims.vowifi.Utilities.PendingAction;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Map.Entry;

public abstract class ServiceManager {
    private static final String TAG = Utilities.getTag(ServiceManager.class.getSimpleName());

    protected Context mContext;
    protected Intent mIntent;
    protected IBinder mServiceBinder;
    protected PendingActionMap mPendingActions;

    private ServiceConnection mConnection = new ServiceConnection() {
        @Override
        public void onServiceDisconnected(ComponentName name) {
            if (Utilities.DEBUG) Log.i(TAG, "The service " + name + " disconnected.");
            mServiceBinder = null;
            onServiceChanged();

            // Re-bind the service if the service disconnected.
            Log.d(TAG, "As service disconnected, will rebind the service after 30s.");
            mHandler.sendEmptyMessageDelayed(MSG_REBIND_SERVICE, 30 * 1000);
        }

        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            if (Utilities.DEBUG) Log.i(TAG, "The service " + name + " connected.");
            mServiceBinder = service;
            onServiceChanged();
        }
    };

    private static final int MSG_PROCESS_PENDING_ACTION = 0;
    private static final int MSG_REBIND_SERVICE = 1;
    protected Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            if (msg.what == MSG_PROCESS_PENDING_ACTION) {
                processPendingAction();
            } else if (msg.what == MSG_REBIND_SERVICE) {
                rebindService();
            } else {
                PendingAction action = (PendingAction) msg.obj;

                if (action == null) {
                    Log.w(TAG, "Try to handle the pending action, but the action is null.");
                    // The action is null, remove it from the HashMap.
                    synchronized (mPendingActions) {
                        mPendingActions.remove(msg.what);
                    }

                    // If the action is null, do nothing.
                    return;
                }

                Message actionMsg = new Message();
                actionMsg.what = action._action;
                actionMsg.obj = action;
                if (handlePendingAction(actionMsg)) {
                    synchronized (mPendingActions) {
                        mPendingActions.remove(msg.what);
                    }
                }
            }
         }
    };

    protected ServiceManager(Context context) {
        if (context == null) throw new NullPointerException("The context is null.");

        mContext = context;
        mPendingActions = new PendingActionMap();
    }

    protected void bindService(String action, String pkg, String cls) {
        if (mServiceBinder != null) {
            Log.w(TAG, "The service already bind, needn't init again.");
            return;
        }

        mIntent = new Intent(action);
        mIntent.setComponent(new ComponentName(pkg, cls));
        mContext.bindService(mIntent, mConnection, Context.BIND_AUTO_CREATE);
    }

    protected void rebindService() {
        if (mIntent != null) {
            mContext.bindService(mIntent, mConnection, Context.BIND_AUTO_CREATE);
        }
    }

    protected void unbindService() {
        mContext.unbindService(mConnection);
    }

    protected void onServiceChanged() {
        // Do nothing here.
    }

    private void processPendingAction() {
        if (mPendingActions.isEmpty()) return;

        Iterator<Entry<Integer, PendingAction>> iterator =
                mPendingActions.entrySet().iterator();
        while (iterator.hasNext()) {
            Entry<Integer, PendingAction> entry = iterator.next();
            Message newMsg = new Message();
            newMsg.what = entry.getKey();
            newMsg.obj = entry.getValue();
            mHandler.sendMessage(newMsg);
        }
    }

    protected void addToPendingList(PendingAction action) {
        if (action == null) {
            Log.e(TAG, "Can not add this action to pending list as it is null.");
            return;
        }

        synchronized (mPendingActions) {
            Integer key = (int) System.currentTimeMillis();
            mPendingActions.put(key, action);
        }
    }

    protected void clearPendingList() {
        synchronized (mPendingActions) {
            mPendingActions.clear();
        }
    }

    protected boolean handlePendingAction(Message msg) {
        // Do nothing here. Please override this function.
        return true;
    }

    private class PendingActionMap extends HashMap<Integer, PendingAction> {
        @Override
        public PendingAction put(Integer key, PendingAction value) {
            PendingAction res = super.put(key, value);
            mHandler.removeMessages(MSG_PROCESS_PENDING_ACTION);
            mHandler.sendEmptyMessageDelayed(MSG_PROCESS_PENDING_ACTION, 15 * 1000);
            return res;
        }

        @Override
        public void putAll(Map<? extends Integer, ? extends PendingAction> map) {
            super.putAll(map);
            mHandler.removeMessages(MSG_PROCESS_PENDING_ACTION);
            mHandler.sendEmptyMessageDelayed(MSG_PROCESS_PENDING_ACTION, 15 * 1000);
        }
    }
}

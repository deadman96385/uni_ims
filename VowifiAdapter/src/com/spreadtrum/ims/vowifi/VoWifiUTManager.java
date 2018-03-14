
package com.spreadtrum.ims.vowifi;

import android.content.Context;
import android.os.RemoteException;
import android.text.TextUtils;
import android.util.Log;

import com.android.ims.internal.IVoWifiUT;
import com.spreadtrum.ims.vowifi.Utilities.IPVersion;
import com.spreadtrum.ims.vowifi.Utilities.RegisterState;
import com.spreadtrum.ims.vowifi.Utilities.S2bType;
import com.spreadtrum.ims.vowifi.Utilities.SecurityConfig;
import com.spreadtrum.ims.vowifi.VoWifiSecurityManager.SecurityListener;

import java.util.ArrayList;

public class VoWifiUTManager extends ServiceManager {
    private static final String TAG = Utilities.getTag(VoWifiUTManager.class.getSimpleName());

    public interface UTStateChangedListener {
        public void onInterfaceChanged(IVoWifiUT newServiceInterface);
        public void onPrepareFinished(boolean success);
        public void onDisabled();
    }

    private static final String SERVICE_CLASS = Utilities.SERVICE_PACKAGE + ".service.UTService";

    private int mSessionId = -1;
    private int mRegisterState = RegisterState.STATE_IDLE;
    private int mCurIPVersion = IPVersion.NONE;
    private int mCurRegIPVersion = IPVersion.NONE;
    private boolean mInAttaching = false;
    private SecurityConfig mSecurityConfig;
    private VoWifiSecurityManager mSecurityMgr;
    private MySecurityListener mSecurityListener;

    private IVoWifiUT mIUT;
    private ArrayList<UTStateChangedListener> mIUTChangedListeners =
            new ArrayList<UTStateChangedListener>();

    protected VoWifiUTManager(Context context, VoWifiSecurityManager securityMgr) {
        super(context, Utilities.SERVICE_PACKAGE, SERVICE_CLASS, Utilities.SERVICE_ACTION_UT);
        mSecurityMgr = securityMgr;
        mSecurityListener = new MySecurityListener();
    }

    @Override
    protected void onServiceChanged() {
        mIUT = null;
        if (mServiceBinder != null) {
            mIUT = IVoWifiUT.Stub.asInterface(mServiceBinder);
        }
        if (mIUT == null && mSecurityConfig != null) {
            mSecurityMgr.deattach(mSessionId, false);
            resetConfig();
        }

        // Notify the interface changed.
        for (UTStateChangedListener listener : mIUTChangedListeners) {
            listener.onInterfaceChanged(mIUT);
        }
    }

    public boolean registerUTInterfaceChanged(UTStateChangedListener listener) {
        if (listener == null) {
            Log.w(TAG, "Can not register the ut interface changed as the listener is null.");
            return false;
        }

        mIUTChangedListeners.add(listener);

        // Notify the service changed immediately when register the listener.
        listener.onInterfaceChanged(mIUT);
        return true;
    }

    public boolean unregisterUTInterfaceChanged(UTStateChangedListener listener) {
        if (listener == null) {
            Log.w(TAG, "Can not register the ut interface changed as the listener is null.");
            return false;
        }

        return mIUTChangedListeners.remove(listener);
    }

    public void updateRegisterState(int newState, int ipVersion) {
        mRegisterState = newState;
        mCurRegIPVersion = ipVersion;

        updateServiceState();
    }

    public void prepare(int subId) {
        synchronized (TAG) {
            if (subId < 0 || mRegisterState != RegisterState.STATE_CONNECTED) {
                // Do not register now. set as prepare failed.
                resetConfig();
                notifyPrepareResult(false);
                return;
            } else if (mInAttaching) {
                Log.d(TAG, "Already in attaching process, ignore the prepare action.");
                return;
            }

            // Start the attach process for UT.
            mInAttaching = true;
            mSecurityMgr.attach(false, subId, S2bType.UT, null, mSecurityListener);
        }
    }

    public void disabled() {
        if (mSessionId > 0) {
            mSecurityMgr.deattach(mSessionId, false);
        }
    }

    private boolean updateSettings() {
        if (mIUT == null || mSecurityConfig == null) {
            Log.e(TAG, "Failed to update the settings, please check!");
            return false;
        }

        try {
            return mIUT.updateIPAddr(getLocalIP(), getDnsIP());
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to update the IP address as catch the e: " + e.toString());
        }

        return false;
    }

    private String getLocalIP() {
        return useIPv6() ? mSecurityConfig._ip6 : mSecurityConfig._ip4;
    }

    private String getDnsIP() {
        return useIPv6() ? mSecurityConfig._dns6 : mSecurityConfig._dns4;
    }

    private boolean useIPv6() {
        return mCurIPVersion == IPVersion.IP_V6;
    }

    private boolean isDisabled() {
        return mRegisterState != RegisterState.STATE_CONNECTED
                || mSessionId < 1
                || mSecurityConfig == null;
    }

    private void resetConfig() {
        mInAttaching = false;
        mSessionId = -1;
        mSecurityConfig = null;
    }

    private void notifyPrepareResult(boolean success) {
        for (UTStateChangedListener listener : mIUTChangedListeners) {
            listener.onPrepareFinished(success);
        }
    }

    private void updateServiceState() {
        if (isDisabled()) {
            for (UTStateChangedListener listener : mIUTChangedListeners) {
                listener.onDisabled();
            }
        }
    }

    private class MySecurityListener implements SecurityListener {

        @Override
        public void onProgress(int state) {
            // Do nothing.
        }

        @Override
        public void onSuccessed(int sessionId) {
            mSessionId = sessionId;
            mSecurityConfig = mSecurityMgr.getConfig(mSessionId);
            mCurIPVersion = mSecurityConfig._useIPVersion;

            // If the register IP version do not same as the attach IP version.
            // We'd like to adjust the UT IPVersion.
            if (mCurIPVersion != mCurRegIPVersion) {
                if (mCurRegIPVersion == IPVersion.IP_V4
                        && !TextUtils.isEmpty(mSecurityConfig._ip4)
                        && mSecurityMgr.setIPVersion(mSessionId, IPVersion.IP_V4)) {
                    mCurIPVersion = IPVersion.IP_V4;
                } else if (mCurRegIPVersion == IPVersion.IP_V6
                        && !TextUtils.isEmpty(mSecurityConfig._ip6)
                        && mSecurityMgr.setIPVersion(mSessionId, IPVersion.IP_V6)) {
                    mCurIPVersion = IPVersion.IP_V6;
                }
            }

            // After attach success, we need update the settings to native stack.
            if (updateSettings()) {
                notifyPrepareResult(true);
            } else {
                resetConfig();
                notifyPrepareResult(false);
            }
        }

        @Override
        public void onFailed(int reason) {
            resetConfig();
            // Notify the prepare failed.
            notifyPrepareResult(false);
        }

        @Override
        public void onStopped(boolean forHandover, int errorCode) {
            resetConfig();
            // Update the service state as attach stopped.
            updateServiceState();
        }

        @Override
        public void onDisconnected() {
            resetConfig();
            // Update the service state as attach stopped.
            updateServiceState();
        }
    }

}
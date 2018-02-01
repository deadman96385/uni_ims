
package com.spreadtrum.ims.vowifi;

import android.content.Context;
import android.os.Message;
import android.os.RemoteException;
import android.text.TextUtils;
import android.util.Log;

import com.spreadtrum.ims.vowifi.Utilities.AttachState;
import com.spreadtrum.ims.vowifi.Utilities.IPVersion;
import com.spreadtrum.ims.vowifi.Utilities.JSONUtils;
import com.spreadtrum.ims.vowifi.Utilities.NativeErrorCode;
import com.spreadtrum.ims.vowifi.Utilities.PendingAction;
import com.spreadtrum.ims.vowifi.Utilities.Result;
import com.spreadtrum.ims.vowifi.Utilities.S2bType;
import com.spreadtrum.ims.vowifi.Utilities.SIMAccountInfo;
import com.spreadtrum.ims.vowifi.Utilities.SecurityConfig;
import com.spreadtrum.vowifi.service.ISecurityService;
import com.spreadtrum.vowifi.service.ISecurityServiceCallback;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;

public class VoWifiSecurityManager extends ServiceManager {
    private static final String TAG = Utilities.getTag(VoWifiSecurityManager.class.getSimpleName());

    private static final int MSG_ACTION_ATTACH = 1;
    private static final int MSG_ACTION_DEATTACH = 2;
    private static final int MSG_ACTION_FORCE_STOP = 3;
    private static final int MSG_ACTION_SET_VOLTE_ADDR = 4;

    private final static String SERVICE_ACTION =
            "com.spreadtrum.vowifi.service.ISecurityService";
    private static final String SERVICE_PACKAGE = "com.spreadtrum.vowifi";
    private static final String SERVICE_CLASS = "com.spreadtrum.vowifi.service.SecurityService";

    private int mState = -1;
    private String mCurLocalAddr = null;
    private SecurityConfig mSecurityConfig = null;
    private SecurityListener mListener = null;

    private ISecurityService mISecurity = null;
    private SecurityCallback mCallback = new SecurityCallback();

    protected VoWifiSecurityManager(Context context) {
        super(context);
    }

    public void bindService() {
        super.bindService(SERVICE_ACTION, SERVICE_PACKAGE, SERVICE_CLASS);
    }

    public void registerListener(SecurityListener listener) {
        if (Utilities.DEBUG) Log.i(TAG, "Register the listener: " + listener);
        if (listener == null) {
            Log.e(TAG, "Can not register the callback as it is null.");
            return;
        }

        mListener = listener;
    }

    public void unregisterListener() {
        if (Utilities.DEBUG) Log.i(TAG, "Unregister the listener: " + mListener);
        mListener = null;
    }

    @Override
    protected void onServiceChanged() {
        try {
            mISecurity = null;
            if (mServiceBinder != null) {
                mISecurity = ISecurityService.Stub.asInterface(mServiceBinder);
                mISecurity.registerCallback(mCallback);
            } else {
                Log.d(TAG, "The security service disconnect. Notify the service disconnected.");
                if (mListener != null) {
                    mListener.onDisconnected();
                }

                mState = AttachState.STATE_IDLE;
                // Clear all the pending action except MSG_ACTION_SET_VOLTE_ADDR.
                ArrayList<Integer> notRemove = new ArrayList<Integer>();
                notRemove.add(MSG_ACTION_SET_VOLTE_ADDR);
                clearPendingList(notRemove);
            }
        } catch (RemoteException e) {
            Log.e(TAG, "Can not register callback as catch the RemoteException. e: " + e);
        }
    }

    @Override
    protected boolean handlePendingAction(Message msg) {
        if (Utilities.DEBUG) Log.i(TAG, "Handle the pending action, msg: " + msg);

        boolean handle = false;
        switch (msg.what) {
            case MSG_ACTION_ATTACH: {
                PendingAction action = (PendingAction) msg.obj;
                attach((Integer) action._params.get(0), (SIMAccountInfo) action._params.get(1));
                handle = true;
                break;
            }
            case MSG_ACTION_DEATTACH: {
                PendingAction action = (PendingAction) msg.obj;
                deattach((Boolean) action._params.get(0));
                handle = true;
                break;
            }
            case MSG_ACTION_FORCE_STOP: {
                forceStop();
                handle = true;
                break;
            }
            case MSG_ACTION_SET_VOLTE_ADDR: {
                // Use the last saved local address.
                setVolteUsedLocalAddr(mCurLocalAddr);
                handle = true;
                break;
            }
        }

        return handle;
    }

    public void attach(SIMAccountInfo info) {
        attach(S2bType.NORMAL, info);
    }

    public void attachForSos(SIMAccountInfo info) {
        attach(S2bType.SOS, info);
    }

    private void attach(int type, SIMAccountInfo info) {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Start the s2b attach. type: " + type + ", sim info: " + info);
        }
        if (info == null) {
            Log.e(TAG, "Can not start attach as the config is null.");
            attachFailed(0);
            return;
        }

        boolean handle = false;
        if (mISecurity != null) {
            try {
                if (mState <= AttachState.STATE_IDLE) {
                    // If the s2b state is idle, start the attach action.
                    int sessionId = mISecurity.start(
                            type, info._phoneId, info._imsi, info._hplmn, info._vplmn, info._imei);
                    if (sessionId == Result.INVALID_ID) {
                        // It means attach failed.
                        attachFailed(0);
                    } else {
                        mSecurityConfig = new SecurityConfig(sessionId);
                    }
                } else {
                    Log.e(TAG, "Can not start attach as the current state is: " + mState);
                }
                handle = true;
            } catch (RemoteException e) {
                Log.e(TAG, "Catch the remote exception when start the s2b attach. e: " + e);
            }
        }
        if (!handle) {
            // Do not handle the attach action, add to pending list.
            addToPendingList(
                    new PendingAction("attach", MSG_ACTION_ATTACH, Integer.valueOf(type), info));
        }
    }

    public void deattach(boolean isHandover) {
        if (Utilities.DEBUG) Log.i(TAG, "Try to de-attach, is handover: " + isHandover);

        boolean handle = false;
        if (mISecurity != null) {
            try {
                if (mSecurityConfig != null) {
                    mISecurity.stop(mSecurityConfig._sessionId, isHandover);
                } else {
                    // Handle it as stop finished.
                    attachStopped(isHandover, 0);
                }
                handle = true;
            } catch (RemoteException e) {
                Log.e(TAG, "Catch the remote exception when start the s2b deattach. e: " + e);
            }
        }
        if (!handle) {
            // Do not handle the attach action, add to pending list.
            addToPendingList(new PendingAction("de-attach", MSG_ACTION_DEATTACH,
                    Boolean.valueOf(isHandover)));
        }
    }

    public void forceStop() {
        if (Utilities.DEBUG) Log.i(TAG, "Force stop the s2b. Do as de-attach for no handover.");

        deattach(false);
    }

    public void setVolteUsedLocalAddr(String addr) {
        if (Utilities.DEBUG) Log.i(TAG, "Set volte used local address to : " + addr);

        mCurLocalAddr = addr;

        boolean handle = false;
        if (mISecurity != null) {
            try {
                mISecurity.setVolteUsedLocalAddr(mCurLocalAddr);
                handle = true;
            } catch (RemoteException e) {
                Log.e(TAG, "Catch the remote exception when set volte addr. e: " + e);
            }
        }
        if (!handle) {
            // Do not handle the attach action, add to pending list.
            addToPendingList(new PendingAction("set_volte_addr", MSG_ACTION_SET_VOLTE_ADDR));
        }
    }

    public boolean setIPVersion(int ipVersion) {
        if (Utilities.DEBUG) Log.i(TAG, "Set the IP version: " + ipVersion);

        if (mISecurity == null) {
            Log.e(TAG, "Failed to set the IP version as the security interface is null.");
            return false;
        }

        try {
            if (ipVersion == IPVersion.NONE) {
                mISecurity.deleteTunelIpsec(mSecurityConfig._sessionId);
                // Will be always true.
                return true;
            } else {
                return mISecurity.switchLoginIpVersion(mSecurityConfig._sessionId, ipVersion);
            }
        } catch (RemoteException e) {
            Log.e(TAG, "Catc the remote exception when set the IP version. e: " + e);
            return false;
        }
    }

    public SecurityConfig getConfig() {
        return mSecurityConfig;
    }

    public int getCurSecurityState() {
        return mState;
    }

    private void attachSuccess() {
        if (mListener != null) mListener.onSuccessed();
        mState = AttachState.STATE_CONNECTED;
    }

    private void attachFailed(int errorCode) {
        if (mListener != null) mListener.onFailed(errorCode);
        mState = AttachState.STATE_IDLE;
        mSecurityConfig = null;
    }

    private void attachProcessing(int stateCode) {
        if (mListener != null) mListener.onProgress(stateCode);
        mState = AttachState.STATE_PROGRESSING;
    }

    private void attachStopped(boolean forHandover, int errorCode) {
        if (mListener != null) mListener.onStopped(forHandover, errorCode);
        mState = AttachState.STATE_IDLE;
        mSecurityConfig = null;
    }

    private class SecurityCallback extends ISecurityServiceCallback.Stub {

        @Override
        public void onS2bStateChanged(String json) throws RemoteException {
            if (Utilities.DEBUG) Log.i(TAG, "Get the security callback: " + json);

            if (TextUtils.isEmpty(json)) {
                Log.e(TAG, "Can not handle the security callback as the json is null.");
                return;
            }

            try {
                JSONObject jObject = new JSONObject(json);
                int eventCode = jObject.optInt(JSONUtils.KEY_EVENT_CODE);
                switch (eventCode) {
                    case JSONUtils.EVENT_CODE_ATTACH_SUCCESSED: {
                        Log.d(TAG, "S2b attach success.");
                        int sessionId = jObject.optInt(JSONUtils.KEY_SESSION_ID, -1);
                        String localIP4 = jObject.optString(JSONUtils.KEY_LOCAL_IP4, null);
                        String localIP6 = jObject.optString(JSONUtils.KEY_LOCAL_IP6, null);
                        String pcscfIP4 = jObject.optString(JSONUtils.KEY_PCSCF_IP4, null);
                        String pcscfIP6 = jObject.optString(JSONUtils.KEY_PCSCF_IP6, null);
                        String dnsIP4 = jObject.optString(JSONUtils.KEY_DNS_IP4, null);
                        String dnsIP6 = jObject.optString(JSONUtils.KEY_DNS_IP6, null);
                        boolean prefIPv4 = jObject.optBoolean(JSONUtils.KEY_PREF_IP4, false);
                        boolean isSos = jObject.optBoolean(JSONUtils.KEY_SOS, false);
                        if (sessionId < 0) {
                            Log.w(TAG, "Get the attach success callback, but the sessionId is: "
                                    + sessionId);
                            break;
                        }

                        if (mSecurityConfig == null) {
                            mSecurityConfig = new SecurityConfig(sessionId);
                        }
                        mSecurityConfig.update(pcscfIP4, pcscfIP6, dnsIP4, dnsIP6, localIP4,
                                localIP6, prefIPv4, isSos);

                        // Switch the IP version as preferred first, then notify the result and
                        // update the state.
                        int useIPVersion =
                                mSecurityConfig._prefIPv4 ? IPVersion.IP_V4 : IPVersion.IP_V6;
                        if (setIPVersion(useIPVersion)) {
                            mSecurityConfig._useIPVersion = useIPVersion;
                            attachSuccess();
                        } else {
                            attachFailed(0);
                        }
                        break;
                    }
                    case JSONUtils.EVENT_CODE_ATTACH_FAILED: {
                        int errorCode = jObject.optInt(JSONUtils.KEY_STATE_CODE);
                        Log.d(TAG, "S2b attach failed, errorCode: " + errorCode);
                        attachFailed(errorCode);
                        break;
                    }
                    case JSONUtils.EVENT_CODE_ATTACH_PROGRESSING: {
                        int state = jObject.optInt(JSONUtils.KEY_PROGRESS_STATE);
                        Log.d(TAG, "S2b attach progress state changed to " + state);
                        attachProcessing(state);
                        break;
                    }
                    case JSONUtils.EVENT_CODE_ATTACH_STOPPED: {
                        // As received attach stopped, need check if the stop action is
                        // relate to current attach request.
                        int sessionId = jObject.optInt(JSONUtils.KEY_SESSION_ID, -1);
                        if (mSecurityConfig != null && sessionId == mSecurityConfig._sessionId) {
                            int errorCode = jObject.optInt(JSONUtils.KEY_STATE_CODE);
                            boolean forHandover = (errorCode == NativeErrorCode.IKE_HANDOVER_STOP);
                            Log.d(TAG, "S2b attach stopped, errorCode: " + errorCode
                                    + ", for handover: " + forHandover);

                            attachStopped(forHandover, errorCode);
                        } else {
                            Log.d(TAG, "Ignore the attach stopped callback as the cur attach is: "
                                    + mSecurityConfig);
                        }
                        break;
                    }
                }
            } catch (JSONException e) {
                Log.e(TAG, "Failed to parse the security callback as catch the JSONException, e: "
                        + e);
            }
        }
    }

    public interface SecurityListener {
        /**
         * Attachment process callback
         *
         * @param state:See S2bConfig related definitions
         */
        void onProgress(int state);

        /**
         * Attachment success callback
         *
         * @param config:Contains the pcscf address, DNS address, IP address, and other information
         *            distribution
         */
        void onSuccessed();

        /**
         * Attachment failure callback
         *
         * @param reason:Enumeration of causes for failure, see S2bConfig related definitions
         */
        void onFailed(int reason);

        /**
         * Attached to stop callback, call stop IS2b () will trigger the callback
         */
        void onStopped(boolean forHandover, int errorCode);

        void onDisconnected();
    }

}

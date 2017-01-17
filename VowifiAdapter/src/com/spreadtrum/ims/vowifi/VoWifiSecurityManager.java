
package com.spreadtrum.ims.vowifi;

import android.content.Context;
import android.os.Message;
import android.os.RemoteException;
import android.text.TextUtils;
import android.util.Log;

import com.juphoon.sprd.security.ISecurityS2b;
import com.juphoon.sprd.security.ISecurityS2bCallback;
import com.spreadtrum.ims.vowifi.Utilities.AttachState;
import com.spreadtrum.ims.vowifi.Utilities.IPVersion;
import com.spreadtrum.ims.vowifi.Utilities.PendingAction;
import com.spreadtrum.ims.vowifi.Utilities.SecurityConfig;

import org.json.JSONException;
import org.json.JSONObject;

public class VoWifiSecurityManager extends ServiceManager {
    private static final String TAG = Utilities.getTag(VoWifiSecurityManager.class.getSimpleName());

    private static final int MSG_ACTION_ATTACH = 1;
    private static final int MSG_ACTION_DEATTACH = 2;
    private static final int MSG_ACTION_FORCE_STOP = 3;

    private final static String SERVICE_ACTION =
            "com.juphoon.sprd.security.SecurityS2bService.action";
    private static final String SERVICE_PACKAGE = "com.sprd.vowifi.security";
    private static final String SERVICE_CLASS = "com.juphoon.sprd.security.SecurityS2bService";

    private int mState = -1;
    private SecurityConfig mSecurityConfig = null;
    private SecurityListener mListener = null;

    private ISecurityS2b mISecurity = null;
    private SecurityS2bCallback mCallback = new SecurityS2bCallback();

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
                mISecurity = ISecurityS2b.Stub.asInterface(mServiceBinder);
                mISecurity.registerCallback(mCallback);
            } else {
                Log.d(TAG, "The security service disconnect. Update the attach state.");
                mState = AttachState.STATE_IDLE;
                if (mListener != null) {
                    mListener.onStopped(false, 0);
                }
                clearPendingList();
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
                attach();
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
        }

        return handle;
    }

    public void attach() {
        if (Utilities.DEBUG) Log.i(TAG, "Start the s2b attach.");

        boolean handle = false;
        if (mISecurity != null) {
            try {
                if (mState <= AttachState.STATE_IDLE) {
                    // If the s2b state is idle, start the attach action.
                    mISecurity.Mtc_S2bStart("");
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
            addToPendingList(new PendingAction("attach", MSG_ACTION_ATTACH));
        }
    }

    public void deattach(boolean isHandover) {
        if (Utilities.DEBUG) Log.i(TAG, "Try to de-attach, is handover: " + isHandover);

        boolean handle = false;
        if (mISecurity != null) {
            try {
                mISecurity.Mtc_S2bStop(isHandover);
                handle = true;
            } catch (RemoteException e) {
                Log.e(TAG, "Catch the remote exception when start the s2b attach. e: " + e);
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

    public boolean setIPVersion(int ipVersion) {
        if (Utilities.DEBUG) Log.i(TAG, "Set the IP version: " + ipVersion);

        if (mISecurity == null) {
            Log.e(TAG, "Failed to set the IP version as the security interface is null.");
            return false;
        }

        try {
            if (ipVersion == IPVersion.NONE) {
                mISecurity.Mtc_S2bDeleteTunelIpsec();
                // Will be always true.
                return true;
            } else {
                return mISecurity.Mtc_S2bSwitchLogin(ipVersion);
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

    private class SecurityS2bCallback extends ISecurityS2bCallback.Stub {
        private static final String SECURITY_JSON_ACTION = "security_json_action";

        private static final String SECURITY_JSON_ACTION_S2B_SUCCESSED =
                "security_json_action_s2b_successed";
        private static final String SECURITY_JSON_ACTION_S2B_FAILED =
                "security_json_action_s2b_failed";
        private static final String SECURITY_JSON_ACTION_S2B_PROGRESSING =
                "security_json_action_s2b_progressing";
        private static final String SECURITY_JSON_ACTION_S2B_STOPPED =
                "security_json_action_s2b_stopped";

        private static final String SECURITY_JSON_PARAM_ERROR_CODE =
                "security_json_param_error_code";
        private static final String SECURITY_JSON_PARAM_PROGRESS_STATE =
                "security_json_param_progress_state";
        private static final String SECURITY_JSON_PARAM_LOCAL_IP4 = "security_json_param_local_ip4";
        private static final String SECURITY_JSON_PARAM_LOCAL_IP6 = "security_json_param_local_ip6";
        private static final String SECURITY_JSON_PARAM_PCSCF_IP4 = "security_json_param_pcscf_ip4";
        private static final String SECURITY_JSON_PARAM_PCSCF_IP6 = "security_json_param_pcscf_ip6";
        private static final String SECURITY_JSON_PARAM_DNS_IP4 = "security_json_param_dns_ip4";
        private static final String SECURITY_JSON_PARAM_DNS_IP6 = "security_json_param_dns_ip6";
        private static final String SECURITY_JSON_PARAM_PREF_IP4 = "security_json_param_pref_ip4";
        private static final String SECURITY_JSON_PARAM_HANDOVER = "security_json_param_handover";

        @Override
        public void onJsonCallback(String json) throws RemoteException {
            if (Utilities.DEBUG) Log.i(TAG, "Get the security callback: " + json);

            if (TextUtils.isEmpty(json)) {
                Log.e(TAG, "Can not handle the security callback as the json is null.");
                return;
            }

            try {
                JSONObject jObject = new JSONObject(json);
                String flag = jObject.optString(SECURITY_JSON_ACTION);
                if (SECURITY_JSON_ACTION_S2B_SUCCESSED.equals(flag)) {
                    Log.d(TAG, "S2b attach success.");
                    String localIP4 = jObject.optString(SECURITY_JSON_PARAM_LOCAL_IP4, null);
                    String localIP6 = jObject.optString(SECURITY_JSON_PARAM_LOCAL_IP6, null);
                    String pcscfIP4 = jObject.optString(SECURITY_JSON_PARAM_PCSCF_IP4, null);
                    String pcscfIP6 = jObject.optString(SECURITY_JSON_PARAM_PCSCF_IP6, null);
                    String dnsIP4 = jObject.optString(SECURITY_JSON_PARAM_DNS_IP4, null);
                    String dnsIP6 = jObject.optString(SECURITY_JSON_PARAM_DNS_IP6, null);
                    boolean prefIPv4 = jObject.optBoolean(SECURITY_JSON_PARAM_PREF_IP4, false);
                    mSecurityConfig = new SecurityConfig(
                            pcscfIP4, pcscfIP6, dnsIP4, dnsIP6, localIP4, localIP6, prefIPv4);

                    // Switch the IP version as preferred first, then notify the result and
                    // update the state.
                    int useIPVersion =
                            mSecurityConfig._prefIPv4 ? IPVersion.IP_V4 : IPVersion.IP_V6;
                    if (setIPVersion(useIPVersion)) {
                        mSecurityConfig._useIPVersion = useIPVersion;

                        if (mListener != null) mListener.onSuccessed();
                        mState = AttachState.STATE_CONNECTED;
                    } else {
                        if (mListener != null) mListener.onFailed(0);
                        mState = AttachState.STATE_IDLE;
                    }
                } else if (SECURITY_JSON_ACTION_S2B_FAILED.equals(flag)) {
                    int errorCode = jObject.optInt(SECURITY_JSON_PARAM_ERROR_CODE);
                    Log.d(TAG, "S2b attach failed, errorCode: " + errorCode);

                    if (mListener != null) {
                        mListener.onFailed(errorCode);
                    }
                    mState = AttachState.STATE_IDLE;
                } else if (SECURITY_JSON_ACTION_S2B_PROGRESSING.equals(flag)) {
                    int state = jObject.optInt(SECURITY_JSON_PARAM_PROGRESS_STATE);
                    Log.d(TAG, "S2b attach progress state changed to " + state);

                    if (mListener != null) {
                        mListener.onProgress(state);
                    }
                    mState = AttachState.STATE_PROGRESSING;
                } else if (SECURITY_JSON_ACTION_S2B_STOPPED.equals(flag)) {
                    int errorCode = jObject.optInt(SECURITY_JSON_PARAM_ERROR_CODE);
                    boolean forHandover = jObject.optBoolean(SECURITY_JSON_PARAM_HANDOVER, false);
                    Log.d(TAG, "S2b attach stopped, errorCode: " + errorCode + ", for handover: "
                            + forHandover);

                    if (mListener != null) {
                        mListener.onStopped(forHandover, errorCode);
                    }
                    mState = AttachState.STATE_IDLE;
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
    }

}

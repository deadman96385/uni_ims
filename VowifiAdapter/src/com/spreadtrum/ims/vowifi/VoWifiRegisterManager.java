
package com.spreadtrum.ims.vowifi;

import android.content.Context;
import android.os.Message;
import android.os.RemoteException;
import android.text.TextUtils;
import android.util.Log;

import com.android.ims.internal.IVoWifiRegister;
import com.android.ims.internal.IVoWifiRegisterCallback;
import com.spreadtrum.ims.vowifi.Utilities.JSONUtils;
import com.spreadtrum.ims.vowifi.Utilities.PendingAction;
import com.spreadtrum.ims.vowifi.Utilities.RegisterConfig;
import com.spreadtrum.ims.vowifi.Utilities.RegisterState;
import com.spreadtrum.ims.vowifi.Utilities.Result;
import com.spreadtrum.ims.vowifi.Utilities.VolteNetworkType;
import com.spreadtrum.ims.vowifi.Utilities.VowifiNetworkType;

import org.json.JSONException;
import org.json.JSONObject;

public class VoWifiRegisterManager extends ServiceManager {
    private static final String TAG = Utilities.getTag(VoWifiRegisterManager.class.getSimpleName());

    public interface RegisterListener {
        void onLoginFinished(boolean success, int stateCode, int retryAfter);

        void onLogout(int stateCode);

        void onPrepareFinished(boolean success);

        /**
         * Refresh the registration result callback
         *
         * @param success true if success or false if failed.
         */
        void onRefreshRegFinished(boolean success, int errorCode);

        void onRegisterStateChanged(int newState, int errorCode);

        void onResetBlocked();
    }

    private static final String SERVICE_ACTION = IVoWifiRegister.class.getCanonicalName();
    private static final String SERVICE_PACKAGE = "com.spreadtrum.vowifi";
    private static final String SERVICE_CLASS = "com.spreadtrum.vowifi.service.RegisterService";

    private static final int MSG_ACTION_LOGIN = 1;
    private static final int MSG_ACTION_DE_REGISTER = 2;
    private static final int MSG_ACTION_RE_REGISTER = 3;
    private static final int MSG_ACTION_FORCE_STOP = 4;
    private static final int MSG_ACTION_PREPARE_FOR_LOGIN = 5;

    private RegisterRequest mRequest = null;
    private IVoWifiRegister mIRegister = null;
    private RegisterCallback mCallback = new RegisterCallback();

    protected VoWifiRegisterManager(Context context) {
        super(context);
    }

    public void bindService() {
        super.bindService(SERVICE_ACTION, SERVICE_PACKAGE, SERVICE_CLASS);
    }

    @Override
    protected void onServiceChanged() {
        try {
            mIRegister = null;
            if (mServiceBinder != null) {
                mIRegister = IVoWifiRegister.Stub.asInterface(mServiceBinder);
                mIRegister.registerCallback(mCallback);
            } else {
                // As the register service disconnected, we'd like to update the register
                // state and notify the result. As we do not know why it disconnect, we
                // will add the "forceStop" to pending list first used to reset the native
                // sip stack, and it will be process in next loop.
                if (mRequest != null
                        && mRequest.mState == RegisterState.STATE_PROGRESSING
                        && mRequest.mListener != null) {
                    mRequest.mListener.onLoginFinished(false, 0, 0);
                } else if (mRequest != null
                        && mRequest.mState == RegisterState.STATE_CONNECTED
                        && mRequest.mListener != null) {
                    mRequest.mListener.onLogout(0);
                }
                updateRegisterState(RegisterState.STATE_IDLE);
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
            case MSG_ACTION_LOGIN: {
                PendingAction action = (PendingAction) msg.obj;
                login((Boolean) action._params.get(0), (Boolean) action._params.get(1),
                        (String) action._params.get(2), (String) action._params.get(3),
                        (String) action._params.get(4), (Boolean)action._params.get(5));
                handle = true;
                break;
            }
            case MSG_ACTION_DE_REGISTER: {
                PendingAction action = (PendingAction) msg.obj;
                deregister((RegisterListener) action._params.get(0));
                handle = true;
                break;
            }
            case MSG_ACTION_RE_REGISTER: {
                PendingAction action = (PendingAction) msg.obj;
                reRegister((Integer) action._params.get(0), (String) action._params.get(1));
                handle = true;
                break;
            }
            case MSG_ACTION_PREPARE_FOR_LOGIN: {
                PendingAction action = (PendingAction) msg.obj;
                prepareForLogin((Integer) action._params.get(0), (Boolean) action._params.get(1),
                        (RegisterConfig) action._params.get(2),
                        (RegisterListener) action._params.get(3));
                break;
            }
        }

        return handle;
    }

    public void prepareForLogin(int subId, boolean isSupportSRVCC, RegisterConfig config,
            RegisterListener listener) {
        if (Utilities.DEBUG) Log.i(TAG, "Prepare the info before login, subId: " + subId);
        if (subId < 0) {
            Log.e(TAG, "Can not get the account info as sub id is: " + subId);
            if (listener != null) listener.onPrepareFinished(false);
            return;
        }

        boolean handle = false;
        if (mIRegister != null) {
            try {
                // Reset first, then prepare.
                mRequest = null;
                int res = mIRegister.cliReset();
                updateRegisterState(RegisterState.STATE_IDLE);
                if (res == Result.FAIL) {
                    Log.w(TAG, "Reset action failed, notify as prepare failed.");
                    if (listener != null) listener.onPrepareFinished(false);
                    return;
                }

                // Prepare for login, need open account, start client and update settings.
                boolean success = false;
                if (cliOpen(subId) && cliStart() && cliUpdateSettings(isSupportSRVCC)) {
                    success = true;
                    mRequest = new RegisterRequest(subId, config, listener);
                }
                if (listener != null) listener.onPrepareFinished(success);

                handle = true;
            } catch (RemoteException e) {
                Log.e(TAG, "Failed to prepare for login as catch the RemoteException, e: " + e);
            }
        }

        if (!handle) {
            // If we catch the remote exception or register interface is null, handle is false.
            // And we will add this action to pending list.
            addToPendingList(new PendingAction("prepareForLogin", MSG_ACTION_PREPARE_FOR_LOGIN,
                    Integer.valueOf(subId), Boolean.valueOf(isSupportSRVCC), config, listener));
        }
    }

    public void login(boolean forSos, boolean isIPv4, String localIP, String pcscfIP,
            String dnsSerIP, boolean isRelogin) {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Try to login to the ims, for sos: " + forSos + ", is IPv4: " + isIPv4
                    + ", current register state: " + mRequest.mState + ", re-login: " + isRelogin);
            Log.i(TAG, "Login with the local ip: " + localIP + ", pcscf ip: " + pcscfIP
                    + ", dns server ip: " + dnsSerIP);
        }

        if (mRequest.mState == RegisterState.STATE_CONNECTED) {
            // Already registered notify the register state.
            if (mRequest.mListener != null) mRequest.mListener.onLoginFinished(true, 0, 0);
            return;
        } else if (mRequest.mState == RegisterState.STATE_PROGRESSING) {
            // Already in the register process, do nothing.
            return;
        } else if (mRequest == null) {
            // Make sure already prepare for login, otherwise the login process can not start.
            Log.e(TAG, "Do not prepare for login, please check!");
            return;
        }

        // The current register status is false.
        boolean handle = false;
        if (mIRegister != null) {
            try {
                updateRegisterState(RegisterState.STATE_PROGRESSING);
                int res = mIRegister.cliLogin(forSos, isIPv4, localIP, pcscfIP, dnsSerIP, isRelogin);
                if (res == Result.FAIL) {
                    Log.e(TAG, "Login to the ims service failed, Please check!");
                    updateRegisterState(RegisterState.STATE_IDLE);
                    // Register failed, give the callback.
                    if (mRequest.mListener != null) mRequest.mListener.onLoginFinished(false, 0, 0);
                }
                handle = true;
            } catch (RemoteException e) {
                updateRegisterState(RegisterState.STATE_IDLE);
                Log.e(TAG, "Catch the remote exception when login, e: " + e);
            }
        }

        if (!handle) {
            // Do not handle the register action, add to pending list.
            PendingAction action = new PendingAction("login", MSG_ACTION_LOGIN,
                    Boolean.valueOf(forSos), Boolean.valueOf(isIPv4), localIP, pcscfIP, dnsSerIP,
                    Boolean.valueOf(isRelogin));

            addToPendingList(action);
        }
    }

    public void deregister(RegisterListener listener) {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Try to logout from the ims.");
        }

        if (mRequest == null) {
            forceStop(listener);
        } else if (mRequest.mState == RegisterState.STATE_IDLE) {
            // The current status is idle or unknown, give the callback immediately.
            if (mRequest.mListener != null) mRequest.mListener.onLogout(0);
        } else if (mRequest.mState == RegisterState.STATE_PROGRESSING) {
            // Already in the register progress, we'd like to cancel current process.
            forceStop(mRequest.mListener);
        } else if (mRequest.mState == RegisterState.STATE_CONNECTED) {
            // The current register status is true;
            boolean handle = false;
            if (mIRegister != null) {
                try {
                    int res = mIRegister.cliLogout();
                    if (res == Result.FAIL) {
                        // Logout failed, shouldn't be here.
                        Log.w(TAG, "Logout from the ims service failed. Please check!");
                    } else {
                        handle = true;
                        updateRegisterState(RegisterState.STATE_PROGRESSING);
                    }
                } catch (RemoteException e) {
                    Log.e(TAG, "Catch the remote exception when unregister, e: " + e);
                }
            }
            if (!handle) {
                // Do not handle the unregister action, add to pending list.
                addToPendingList(new PendingAction("de-register", MSG_ACTION_DE_REGISTER, listener));
            }
        } else {
            // Shouldn't be here.
            Log.e(TAG, "Try to logout from the ims, shouldn't be here. register state: "
                    + mRequest.mState);
        }
    }

    public void reRegister(int type, String info) {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Re-register, with the type: " + type + ", info: " + info);
        }

        if (mRequest == null
                || mRequest.mState != RegisterState.STATE_CONNECTED
                || TextUtils.isEmpty(info)) {
            // The current register state is false, can not re-register.
            Log.e(TAG, "Failed to re-register, please check!");
            return;
        }

        boolean handle = false;
        if (mIRegister != null) {
            try {
                int res = mIRegister.cliRefresh(getVowifiNetworkType(type), info);
                if (res == Result.FAIL) {
                    // Logout failed, shouldn't be here.
                    Log.w(TAG, "Re-register to the ims service failed. Please check!");
                }
                handle = true;
            } catch (RemoteException e) {
                Log.e(TAG, "Catch the remote exception when re-register, e: " + e);
            }
        }
        if (!handle) {
            // Do not handle the re-register action, add to pending list.
            PendingAction action = new PendingAction("re-register", MSG_ACTION_RE_REGISTER,
                    Integer.valueOf(type), info);
            addToPendingList(action);
        }
    }

    public boolean forceStop(RegisterListener listener) {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Force stop current register process.");
        }

        if (mIRegister != null) {
            try {
                int res = mIRegister.cliReset();
                if (res == Result.FAIL) {
                    Log.e(TAG, "Failed to reset the sip stack, notify as reset block.");
                    if (listener != null) listener.onResetBlocked();
                }
            } catch (RemoteException e) {
                Log.e(TAG, "Catch the remote exception when unregister, e: " + e);
            }
        }

        // For force stop, we'd like do not handle the failed action, and set the register state
        // to idle immediately.
        updateRegisterState(RegisterState.STATE_IDLE);
        mRequest = null;
        return true;
    }

    public int getCurRegisterState() {
        return mRequest == null ? RegisterState.STATE_IDLE : mRequest.mState;
    }

    public RegisterConfig getCurRegisterConfig() {
        return mRequest == null ? null : mRequest.mRegisterConfig;
    }

    private boolean cliOpen(int subId) throws RemoteException {
        if (Utilities.DEBUG) Log.i(TAG, "Try to open the account.");

        if (mIRegister == null || subId < 0) {
            Log.e(TAG, "Failed open account as register interface or the account info is null.");
            return false;
        }

        return mIRegister.cliOpen(subId) == Result.SUCCESS;
    }

    private boolean cliStart() throws RemoteException {
        if (Utilities.DEBUG) Log.i(TAG, "Try to start the client.");

        if (mIRegister == null) {
            Log.e(TAG, "Failed start client as register interface is null.");
            return false;
        }

        return mIRegister.cliStart() == Result.SUCCESS;
    }

    private boolean cliUpdateSettings(boolean isSupportSRVCC)
            throws RemoteException {
        if (Utilities.DEBUG) Log.i(TAG, "Try to update the account settings.");

        if (mIRegister == null) {
            Log.e(TAG, "Failed update as register interface or the account info is null.");
            return false;
        }

        int res = mIRegister.cliUpdateSettings(isSupportSRVCC);
        return res == Result.SUCCESS;
    }

    private int getVowifiNetworkType(int volteType) {
        switch (volteType) {
            case VolteNetworkType.IEEE_802_11:
                return VowifiNetworkType.IEEE_802_11;
            case VolteNetworkType.E_UTRAN_FDD:
                return VowifiNetworkType.E_UTRAN_FDD;
            case VolteNetworkType.E_UTRAN_TDD:
                return VowifiNetworkType.E_UTRAN_TDD;
            default:
                Log.e(TAG, "Do not support this volte network type now, type: " + volteType);
                return VowifiNetworkType.IEEE_802_11;
        }
    }

    private void updateRegisterState(int newState) {
        updateRegisterState(newState, 0);
    }

    private void updateRegisterState(int newState, int errorCode) {
        if (mRequest == null) return;

        mRequest.mState = newState;
        if (mRequest.mListener != null) {
            mRequest.mListener.onRegisterStateChanged(newState, errorCode);
        }
    }

    private class RegisterRequest {
        public int mSubId;
        public int mState;
        public RegisterConfig mRegisterConfig;
        public RegisterListener mListener;

        public RegisterRequest(int subId, RegisterConfig config, RegisterListener listener) {
            mSubId = subId;
            mRegisterConfig = config;
            mListener = listener;
        }
    }

    private class RegisterCallback extends IVoWifiRegisterCallback.Stub {
        @Override
        public void onRegisterStateChanged(String json) throws RemoteException {
            if (Utilities.DEBUG) Log.i(TAG, "Get the register state changed callback: " + json);

            if (mRequest == null || TextUtils.isEmpty(json)) {
                Log.e(TAG, "Can not handle the callback, please check the request or response.");
                return;
            }

            try {
                JSONObject jObject = new JSONObject(json);
                int eventCode = jObject.optInt(
                        JSONUtils.KEY_EVENT_CODE, JSONUtils.REGISTER_EVENT_CODE_BASE);
                String eventName = jObject.optString(JSONUtils.KEY_EVENT_NAME);
                Log.d(TAG, "Handle the register event: " + eventName);

                switch (eventCode) {
                    case JSONUtils.EVENT_CODE_LOGIN_OK:
                        // Update the register state to connected, and notify the state changed.
                        updateRegisterState(RegisterState.STATE_CONNECTED);
                        if (mRequest.mListener != null) {
                            mRequest.mListener.onLoginFinished(true, 0, 0);
                        }
                        break;
                    case JSONUtils.EVENT_CODE_LOGIN_FAILED:
                        // Update the register state to unknown, and notify the state changed.
                        updateRegisterState(RegisterState.STATE_IDLE);
                        if (mRequest.mListener != null) {
                            int stateCode = jObject.optInt(JSONUtils.KEY_STATE_CODE, 0);
                            int retryAfter = jObject.optInt(JSONUtils.KEY_RETRY_AFTER, 0);
                            mRequest.mListener.onLoginFinished(false, stateCode, retryAfter);
                        }
                        break;
                    case JSONUtils.EVENT_CODE_LOGOUTED:
                        // Update the register state to idle, and reset the sip stack.
                        updateRegisterState(RegisterState.STATE_IDLE);
                        if (mRequest.mListener != null) {
                            int stateCode = jObject.optInt(JSONUtils.KEY_STATE_CODE, 0);
                            mRequest.mListener.onLogout(stateCode);
                            mRequest = null;
                        }
                        break;
                    case JSONUtils.EVENT_CODE_REREGISTER_OK:
                        if (mRequest.mListener != null) mRequest.mListener.onRefreshRegFinished(true, 0);
                        break;
                    case JSONUtils.EVENT_CODE_REREGISTER_FAILED:
                        if (mRequest.mListener != null) {
                            int stateCode = jObject.optInt(JSONUtils.KEY_STATE_CODE, 0);
                            mRequest.mListener.onRefreshRegFinished(false, stateCode);
                        }
                        break;
                    case JSONUtils.EVENT_CODE_REGISTER_STATE_UPDATE:
                        // Update the register state, and notify the state changed.
                        if (mRequest.mListener != null) {
                            int stateCode = jObject.optInt(JSONUtils.KEY_STATE_CODE, 0);
                            mRequest.mListener.onRegisterStateChanged(RegisterState.STATE_IDLE, stateCode);
                        }
                        break;
                }
            } catch (JSONException e) {
                Log.e(TAG, "");
            }
        }
    }

}

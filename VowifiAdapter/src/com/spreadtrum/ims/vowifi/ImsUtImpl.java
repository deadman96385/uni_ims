
package com.spreadtrum.ims.vowifi;

import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.RemoteException;
import android.text.TextUtils;
import android.util.Log;

import com.android.ims.ImsCallForwardInfo;
import com.android.ims.internal.ImsCallForwardInfoEx;
import com.android.ims.ImsReasonInfo;
import com.android.ims.ImsSsInfo;
import com.android.ims.ImsUtInterface;
import com.android.ims.internal.IImsUt;
import com.android.ims.internal.IImsUtListener;
import com.android.ims.internal.IImsUtListenerEx;
import com.android.ims.internal.IVoWifiUT;
import com.android.ims.internal.IVoWifiUTCallback;
import com.android.internal.telephony.CommandsInterface;
import com.spreadtrum.ims.vowifi.Utilities.CallBarringInfo;
import com.spreadtrum.ims.vowifi.Utilities.JSONUtils;
import com.spreadtrum.ims.vowifi.Utilities.PendingAction;
import com.spreadtrum.ims.vowifi.Utilities.Result;
import com.spreadtrum.ims.vowifi.VoWifiUTManager.UTStateChangedListener;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * Provides the Ut interface interworking to get/set the supplementary service configuration.
 * <p/>
 * {@hide}
 */
public class ImsUtImpl extends IImsUt.Stub {
    private static final String TAG = Utilities.getTag(ImsUtImpl.class.getSimpleName());

    // Call forward service class
    private static final int SERVICE_CLASS_VOICE = CommandsInterface.SERVICE_CLASS_VOICE;
    // FIXME: This value defined in CallForwardEditPreference but not CommandsInterface.
    private static final int SERVICE_CLASS_VIDEO = 2;
    private static final int SERVICE_CLASS_ALL = SERVICE_CLASS_VOICE | SERVICE_CLASS_VIDEO;

    // If there isn't any CMD to disabled the UT after 1min.
    private static final int DELAY_DISALBE_UT = 60 * 1000;

    private Context mContext;
    private boolean mUtEnabled;

    private IImsUtListener mListener = null;
    private IImsUtListenerEx mListenerEx = null;

    private CmdManager mCmdManager = null;
    private VoWifiUTManager mUtManager = null;
    private IVoWifiUT mIUT = null;
    private UtServiceCallback mUtServiceCallback = new UtServiceCallback();

    // Set the commands timeout as 1 minutes.
    private static final int CMD_TIMEOUT = 60 * 1000;

    private static final int MSG_DISABLE_UT = -2;
    private static final int MSG_HANDLE_EVENT = -1;
    private static final int MSG_CMD_TIMEOUT = 0;

    private static final int MSG_ACTION_QUERY_CALL_BARRING = 101;
    private static final int MSG_ACTION_QUERY_CALL_FORWARD = 102;
    private static final int MSG_ACTION_QUERY_CALL_FORWARDING_OPTION = 103;
    private static final int MSG_ACTION_QUERY_CALL_WAITING = 104;
    private static final int MSG_ACTION_UPDATE_CALL_BARRING = 105;
    private static final int MSG_ACTION_UPDATE_CALL_FORWARD = 106;
    private static final int MSG_ACTION_UPDATE_CALL_FORWARDING_OPTION = 107;
    private static final int MSG_ACTION_UPDATE_CALL_WAITING = 108;
    private static final int MSG_ACTION_SET_FACILITY_LOCK = 109;
    private static final int MSG_ACTION_QUERY_FACILITY_LOCK = 110;
    private static final int MSG_ACTION_CHANGE_LOCK_PWD = 111;
    private static final int MSG_ACTION_UPDATE_CLIR = 112;

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_DISABLE_UT: {
                    mUtManager.disabled();
                    mUtEnabled = false;
                    break;
                }
                case MSG_HANDLE_EVENT: {
                    handleEvent((String) msg.obj);
                    break;
                }
                case MSG_CMD_TIMEOUT: {
                    Integer timeoutKey = msg.arg1;
                    Integer key = mCmdManager.getFirstCmd();
                    if (timeoutKey == key) {
                        // It means this cmd meet the timeout event. And we'd like to give the
                        // failed result for this cmd.
                        ImsReasonInfo error = new ImsReasonInfo(ImsReasonInfo.CODE_UT_NETWORK_ERROR,
                                ImsReasonInfo.CODE_UNSPECIFIED);
                        mCmdManager.onActionFailed(error);
                    } else {
                        Log.d(TAG, "Ignore the cmd timeout as timeout key is " + timeoutKey
                                + ", but the first cmd key is " + key);
                    }
                    break;
                }
                case MSG_ACTION_QUERY_CALL_BARRING:
                case MSG_ACTION_QUERY_FACILITY_LOCK: {
                    UTAction action = (UTAction) msg.obj;
                    int condition = (Integer) action._params.get(0);
                    nativeQueryCallBarring(condition);
                    break;
                }
                case MSG_ACTION_QUERY_CALL_FORWARD:
                case MSG_ACTION_QUERY_CALL_FORWARDING_OPTION: {
                    nativeQueryCallForward();
                    break;
                }
                case MSG_ACTION_QUERY_CALL_WAITING: {
                    nativeQueryCallWaiting();
                    break;
                }
                case MSG_ACTION_UPDATE_CALL_BARRING:
                case MSG_ACTION_SET_FACILITY_LOCK: {
                    UTAction action = (UTAction) msg.obj;
                    int condition = (Integer) action._params.get(0);
                    boolean enabled = (Boolean) action._params.get(1);
                    String barringList = (String) action._params.get(2);
                    int serviceClass = (Integer) action._params.get(3);
                    nativeUpdateCallBarring(condition, enabled, barringList, serviceClass);
                    break;
                }
                case MSG_ACTION_UPDATE_CALL_FORWARD: {
                    UTAction action = (UTAction) msg.obj;
                    int utAction = getActionFromCFAction((Integer) action._params.get(0));
                    int condition = (Integer) action._params.get(1);
                    String number = (String) action._params.get(2);
                    int serviceClass = (Integer) action._params.get(3);
                    int timeSeconds = (Integer) action._params.get(4);
                    nativeUpdateCallForward(utAction, condition, number, serviceClass, timeSeconds);
                    break;
                }
                case MSG_ACTION_UPDATE_CALL_FORWARDING_OPTION: {
                    UTAction action = (UTAction) msg.obj;
                    int utAction = getActionFromCFAction((Integer) action._params.get(0));
                    int condition = getConditionFromCFReason((Integer) action._params.get(1));
                    int serviceClass = (Integer) action._params.get(2);
                    String number = (String) action._params.get(3);
                    int timeSeconds = (Integer) action._params.get(4);
                    nativeUpdateCallForward(utAction, condition, number, serviceClass, timeSeconds);
                    break;
                }
                case MSG_ACTION_UPDATE_CALL_WAITING: {
                    UTAction action = (UTAction) msg.obj;
                    nativeUpdateCallWaiting((Boolean) action._params.get(0));
                    break;
                }
                case MSG_ACTION_CHANGE_LOCK_PWD: {
                    UTAction action = (UTAction) msg.obj;
                    String condition = (String) action._params.get(0);
                    String oldPwd = (String) action._params.get(1);
                    String newPwd = (String) action._params.get(2);
                    nativeChangeBarringPwd(condition, oldPwd, newPwd);
                    break;
                }
                case MSG_ACTION_UPDATE_CLIR: {
                    UTAction action = (UTAction) msg.obj;
                    boolean enabled = (Boolean) action._params.get(0);
                    nativeUpdateCLIR(enabled);
                    break;
                }
            }
        }
    };

    // To listener the IVoWifiSerService changed.
    private UTStateChangedListener mIUTChangedListener = new UTStateChangedListener() {

        @Override
        public void onInterfaceChanged(IVoWifiUT newInterface) {
            mIUT = newInterface;
            if (mIUT == null) return;

            // The new call Interface is not null now, register the callback.
            try {
                mIUT.registerCallback(mUtServiceCallback);
            } catch (RemoteException e) {
                Log.e(TAG, "Failed to register callback for UT as catch RemoteException: " + e);
            }
        }

        @Override
        public void onPrepareFinished(boolean success) {
            // As prepare finished, we could process the action now.
            mUtEnabled = success;
            mCmdManager.processPendingAction();
        }

        @Override
        public void onDisabled() {
            mUtEnabled = false;

            // As UT disabled, handle the CMD as failed.
            ImsReasonInfo error = new ImsReasonInfo(ImsReasonInfo.CODE_UT_NETWORK_ERROR,
                    ImsReasonInfo.CODE_UNSPECIFIED);
            mCmdManager.onActionFailed(error);
        }
    };

    protected ImsUtImpl(Context context, VoWifiUTManager utManager) {
        mContext = context;
        mCmdManager = new CmdManager();
        mUtManager = utManager;

        // Register the service changed to get the IVowifiService.
        mUtManager.registerUTInterfaceChanged(mIUTChangedListener);
    }

    @Override
    protected void finalize() throws Throwable {
        // Un-register the service changed.
        mUtManager.unregisterUTInterfaceChanged(mIUTChangedListener);
        super.finalize();
    }

    /**
     * Closes the object. This object is not usable after being closed.
     */
    @Override
    public void close() {
        mListener = null;
        mListenerEx = null;
    }

    /**
     * Retrieves the configuration of the call barring.
     */
    @Override
    public int queryCallBarring(int cbType) {
        if (Utilities.DEBUG) Log.i(TAG, "Try to query the call barring with the type: " + cbType);

        UTAction action = new UTAction("queryCallBarring", MSG_ACTION_QUERY_CALL_BARRING,
                CMD_TIMEOUT, Integer.valueOf(cbType));
        return mCmdManager.addCmd(action);
    }

    /**
     * Retrieves the configuration of the call forward.
     */
    @Override
    public int queryCallForward(int condition, String number) {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Try to query the call forward as the condition: " + condition
                    + ", for the number: " + number);
        }

        UTAction action = new UTAction("queryCallForward", MSG_ACTION_QUERY_CALL_FORWARD,
                CMD_TIMEOUT, Integer.valueOf(condition), number);
        return mCmdManager.addCmd(action);
    }

    /**
     * Retrieves the configuration of the call waiting.
     */
    @Override
    public int queryCallWaiting() throws RemoteException {
        if (Utilities.DEBUG) Log.i(TAG, "Try to query the call waiting.");

        UTAction action = new UTAction("queryCallWaiting", MSG_ACTION_QUERY_CALL_WAITING,
                CMD_TIMEOUT);
        return mCmdManager.addCmd(action);
    }

    /**
     * Retrieves the default CLIR setting.
     */
    @Override
    public int queryCLIR() throws RemoteException {
        Log.w(TAG, "As CLIR based on UE, handle query CLIR as failed.");
        return ImsUtInterface.INVALID;
    }

    /**
     * Retrieves the CLIP call setting.
     */
    @Override
    public int queryCLIP() throws RemoteException {
        Log.w(TAG, "Do not support query CLIP now.");
        return ImsUtInterface.INVALID;
    }

    /**
     * Retrieves the COLR call setting.
     */
    @Override
    public int queryCOLR() throws RemoteException {
        Log.w(TAG, "Do not support query COLR now.");
        return ImsUtInterface.INVALID;
    }

    /**
     * Retrieves the COLP call setting.
     */
    @Override
    public int queryCOLP() throws RemoteException {
        Log.w(TAG, "Do not support query COLP now.");
        return ImsUtInterface.INVALID;
    }

    /**
     * Updates or retrieves the supplementary service configuration.
     */
    @Override
    public int transact(Bundle ssInfo) {
        // Do not support now.
        Log.w(TAG, "The vowifi do not support the transact function now.");
        return ImsUtInterface.INVALID;
    }

    /**
     * Updates the configuration of the call barring.
     */
    @Override
    public int updateCallBarring(int cbType, int enable, String[] barringList) {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Try to update the call barring with the type: " + cbType + ", enabled: "
                    + enable + ", barringList: " + Utilities.getString(barringList));
        }

        boolean enabled = (enable == CommandsInterface.CF_ACTION_ENABLE);
        UTAction action = new UTAction("updateCallBarring", MSG_ACTION_UPDATE_CALL_BARRING,
                CMD_TIMEOUT, Integer.valueOf(cbType), Boolean.valueOf(enabled), "",
                Integer.valueOf(SERVICE_CLASS_VOICE));
        return mCmdManager.addCmd(action);
    }

    /**
     * Updates the configuration of the call forward.
     */
    @Override
    public int updateCallForward(int action, int condition, String number, int serviceClass,
            int timeSeconds) {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Try to update the call forward with action: " + action + ", condition: "
                    + condition + ", number: " + number + ", timeSeconds: " + timeSeconds);
        }

        UTAction utAction = new UTAction("updateCallForward", MSG_ACTION_UPDATE_CALL_FORWARD,
                CMD_TIMEOUT, Integer.valueOf(action), Integer.valueOf(condition), number,
                Integer.valueOf(serviceClass), Integer.valueOf(timeSeconds));
        return mCmdManager.addCmd(utAction);
    }

    /**
     * Updates the configuration of the call waiting.
     */
    @Override
    public int updateCallWaiting(boolean enable, int serviceClass) throws RemoteException {
        if (Utilities.DEBUG) Log.i(TAG, "Try to update the call waiting to enable: " + enable);

        UTAction action = new UTAction("updateCallWaiting", MSG_ACTION_UPDATE_CALL_WAITING,
                CMD_TIMEOUT, Boolean.valueOf(enable), Integer.valueOf(serviceClass));
        return mCmdManager.addCmd(action);
    }

    /**
     * Updates the configuration of the CLIR supplementary service.
     */
    @Override
    public int updateCLIR(int clirMode) throws RemoteException {
        Log.w(TAG, "As CLIR based on UE, handle update CLIR as failed.");
        return ImsUtInterface.INVALID;
    }

    /**
     * Updates the configuration of the CLIP supplementary service.
     */
    @Override
    public int updateCLIP(boolean enable) throws RemoteException {
        Log.w(TAG, "Do not support update CLIP now.");
        return ImsUtInterface.INVALID;
    }

    /**
     * Updates the configuration of the COLR supplementary service.
     */
    @Override
    public int updateCOLR(int presentation) throws RemoteException {
        Log.w(TAG, "Do not support update COLR now.");
        return ImsUtInterface.INVALID;
    }

    /**
     * Updates the configuration of the COLP supplementary service.
     */
    @Override
    public int updateCOLP(boolean enable) throws RemoteException {
        Log.w(TAG, "Do not support update COLP now.");
        return ImsUtInterface.INVALID;
    }

    /**
     * Sets the listener.
     */
    @Override
    public void setListener(IImsUtListener listener) throws RemoteException {
        mListener = listener;
    }

    public void setListenerEx(IImsUtListenerEx listenerEx){
        mListenerEx = listenerEx;
    }

    /**
     * Retrieves the configuration of the call forward.
     * Calendar c;
     * SimpleDateFormat mSimpleFmt = new SimpleDateFormat("HH:mm");
     * Date curDate = new Date(c.getTimeInMillis());
     * String ruleSet = mSimpleFmt.format(curDate);
     */
    public int setCallForwardingOption(int cfAction, int reason, int serviceClass,
            String dialNumber, int timeSeconds, String ruleSet) {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Set the call forwarding option, call forward action: " + cfAction
                    + ", call forward reason: " + reason + ", service class: " + serviceClass
                    + ", dial number: " + dialNumber + ", time seconds: " + timeSeconds
                    + ", ruleSet: " + ruleSet);
        }

        UTAction action = new UTAction(true /* is extension action */, "setCallForwardingOption",
                MSG_ACTION_UPDATE_CALL_FORWARDING_OPTION, CMD_TIMEOUT,
                Integer.valueOf(cfAction), Integer.valueOf(reason), Integer.valueOf(serviceClass),
                dialNumber, Integer.valueOf(timeSeconds), ruleSet);
        return mCmdManager.addCmd(action);
    }

    /**
     * Updates the configuration of the call forward.
     */
    public int getCallForwardingOption(int reason, int serviceClass, String ruleSet) {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Get the call forwarding option, call forward reason: " + reason
                    + ", service class: " + serviceClass + ", ruleSet: " + ruleSet);
        }

        UTAction action = new UTAction(true /* is extension action */, "getCallForwardingOption",
                MSG_ACTION_QUERY_CALL_FORWARDING_OPTION, CMD_TIMEOUT,
                Integer.valueOf(reason), Integer.valueOf(serviceClass), ruleSet);
        return mCmdManager.addCmd(action);
    }

    /**
     * Updates the configuration of the call barring.
     */
    public int setFacilityLock(String facility, boolean lockState, String password,
            int serviceClass){
        if (Utilities.DEBUG) {
            Log.i(TAG, "setFacilityLock, reason: " + facility
                    + ", lock state: " + lockState + ", password: " + password);
        }

        int condition = getConditionFromCBReason(facility);
        UTAction action = new UTAction(true /* is extension action */, "setFacilityLock",
                MSG_ACTION_SET_FACILITY_LOCK, CMD_TIMEOUT, Integer.valueOf(condition),
                Boolean.valueOf(lockState), password, Integer.valueOf(serviceClass));
        return mCmdManager.addCmd(action);
    }

     public int queryFacilityLock(String facility, String password, int serviceClass){
        if (Utilities.DEBUG) {
            Log.i(TAG, "queryFacilityLock, reason: " + facility
                    + ", password: " + password + ", serviceclass: " + serviceClass);
        }

        int condition = getConditionFromCBReason(facility);
        UTAction action = new UTAction(true /* is extension action */, "queryFacilityLock",
                MSG_ACTION_QUERY_FACILITY_LOCK, CMD_TIMEOUT, Integer.valueOf(condition), password,
                Integer.valueOf(serviceClass));
        return mCmdManager.addCmd(action);
    }

    public int changeBarringPassword(String facility, String oldPwd, String newPwd) {
        if (Utilities.DEBUG) {
            Log.i(TAG, "changeBarringPassword, reason: " + facility + ", old password: " + oldPwd
                    + ", new password: " + newPwd);
        }

        UTAction action = new UTAction(true /* is extension action */, "changeBarringPassword",
                MSG_ACTION_CHANGE_LOCK_PWD, CMD_TIMEOUT, facility, oldPwd, newPwd);
        return mCmdManager.addCmd(action);
    }

    private void nativeQueryCallBarring(int condition) {
        if (Utilities.DEBUG) Log.i(TAG, "Native query the call barring options.");

        boolean success = false;
        try {
            if (mIUT != null) {
                int res = mIUT.queryCallBarring(condition);
                if (res == Result.SUCCESS) success = true;
            }
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to query the call barring as catch the RemoteException: " + e);
        }

        // If the action is failed, process action failed.
        if (!success) {
            Log.e(TAG, "Native failed to query the call barring.");
            ImsReasonInfo error = new ImsReasonInfo(ImsReasonInfo.CODE_UT_NETWORK_ERROR,
                    ImsReasonInfo.CODE_UNSPECIFIED);
            mCmdManager.onActionFailed(error);
        }
    }

    private void nativeQueryCallForward() {
        if (Utilities.DEBUG) Log.i(TAG, "Native query the call forward options.");

        boolean success = false;
        try {
            if (mIUT != null) {
                int res = mIUT.queryCallForward();
                if (res == Result.SUCCESS) success = true;
            }
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to query the call forward as catch the RemoteException: " + e);
        }

        // If the action is failed, process action failed.
        if (!success) {
            Log.e(TAG, "Native failed to query the call forward.");
            ImsReasonInfo error = new ImsReasonInfo(ImsReasonInfo.CODE_UT_NETWORK_ERROR,
                    ImsReasonInfo.CODE_UNSPECIFIED);
            mCmdManager.onActionFailed(error);
        }
    }

    private void nativeQueryCallWaiting() {
        if (Utilities.DEBUG) Log.i(TAG, "Native query the call waiting options.");

        boolean success = false;
        try {
            if (mIUT != null) {
                int res = mIUT.queryCallWaiting();
                if (res == Result.SUCCESS) success = true;
            }
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to query the call waiting as catch the RemoteException: " + e);
        }

        // If the action is failed, process action failed.
        if (!success) {
            Log.e(TAG, "Native failed to query the call waiting.");
            ImsReasonInfo error = new ImsReasonInfo(ImsReasonInfo.CODE_UT_NETWORK_ERROR,
                    ImsReasonInfo.CODE_UNSPECIFIED);
            mCmdManager.onActionFailed(error);
        }
    }

    private void nativeUpdateCallBarring(int cbCondition, boolean enable, String barringList,
            int serviceClass) {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Native update the call barring options, condition: " + cbCondition
                    + ", enable: " + enable + ", barringList: " + barringList + ", serviceClass: "
                    + serviceClass);
        }

        boolean success = false;
        try {
            if (mIUT != null) {
                String[] allowList = null;
                if (!TextUtils.isEmpty(barringList)) {
                    allowList = barringList.split(";");
                } else {
                    allowList = new String[] { "false" };
                }
                int res = mIUT.updateCallBarring(cbCondition, enable, allowList, serviceClass);
                if (res == Result.SUCCESS) success = true;
            }
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to query the call barring as catch the RemoteException: " + e);
        }

        // If the action is failed, process action failed.
        if (!success) {
            Log.e(TAG, "Native failed to update the call barring.");
            ImsReasonInfo error = new ImsReasonInfo(ImsReasonInfo.CODE_UT_NETWORK_ERROR,
                    ImsReasonInfo.CODE_UNSPECIFIED);
            mCmdManager.onActionFailed(error);
        }
    }

    private void nativeUpdateCallForward(int action, int condition, String number, int serviceClass,
            int timeSeconds) {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Native update the call forward options, action: " + action
                    + ", condition: " + condition + ", number: " + number + ", service class: "
                    + serviceClass + ", timeSeconds: " + timeSeconds);
        }

        boolean success = false;
        try {
            if (mIUT != null) {
                int res = mIUT.updateCallForward(
                        action, condition, number, serviceClass, timeSeconds);
                if (res == Result.SUCCESS) success = true;
            }
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to query the call forward as catch the RemoteException: " + e);
        }

        // If the action is failed, process action failed.
        if (!success) {
            Log.e(TAG, "Native failed to update the call forward.");
            ImsReasonInfo error = new ImsReasonInfo(ImsReasonInfo.CODE_UT_NETWORK_ERROR,
                    ImsReasonInfo.CODE_UNSPECIFIED);
            mCmdManager.onActionFailed(error);
        }
    }

    private void nativeUpdateCallWaiting(boolean enabled) {
        if (Utilities.DEBUG) Log.i(TAG, "Native update the call waiting as enabled: " + enabled);

        boolean success = false;
        try {
            if (mIUT != null) {
                int res = mIUT.updateCallWaiting(enabled);
                if (res == Result.SUCCESS) success = true;
            }
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to query the call waiting as catch the RemoteException: " + e);
        }

        // If the action is failed, process action failed.
        if (!success) {
            Log.e(TAG, "Native failed to update the call waiting.");
            ImsReasonInfo error = new ImsReasonInfo(ImsReasonInfo.CODE_UT_NETWORK_ERROR,
                    ImsReasonInfo.CODE_UNSPECIFIED);
            mCmdManager.onActionFailed(error);
        }
    }

    private void nativeChangeBarringPwd(String condition, String oldPwd, String newPwd) {
        if (Utilities.DEBUG) Log.i(TAG, "Native change the call barring password to : " + newPwd);
    }

    private void nativeUpdateCLIR(boolean enabled) {
        if (Utilities.DEBUG) Log.i(TAG, "Native update the CLIR as enabled: " + enabled);

        boolean success = false;
        try {
            if (mIUT != null) {
                int res = mIUT.updateCLIR(enabled);
                if (res == Result.SUCCESS) {
                    success = true;
                    handleUpdateActionOK();
                }
            }
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to update CLIR as catch the RemoteException: " + e);
        }

        // If the action is failed, process action failed.
        if (!success) {
            Log.e(TAG, "Native failed to update CLIR.");
            ImsReasonInfo error = new ImsReasonInfo(ImsReasonInfo.CODE_UT_NETWORK_ERROR,
                    ImsReasonInfo.CODE_UNSPECIFIED);
            mCmdManager.onActionFailed(error);
        }
    }

    private void handleEvent(String json) {
        try {
            JSONObject jObject = new JSONObject(json);
            String eventName = jObject.optString(JSONUtils.KEY_EVENT_NAME, "");
            Log.d(TAG, "Handle the event '" + eventName + "'");

            int eventCode = jObject.optInt(JSONUtils.KEY_EVENT_CODE, -1);
            switch (eventCode) {
                case JSONUtils.EVENT_CODE_UT_QUERY_CF_OK: {
                    handleQueryCallForwardOK(parseCallForwardInfos(json));
                    break;
                }
                case JSONUtils.EVENT_CODE_UT_QUERY_CB_OK: {
                    handleQueryCallBarringOK(parseCallBarringInfos(json));
                    break;
                }
                case JSONUtils.EVENT_CODE_UT_QUERY_CW_OK: {
                    boolean enabled = jObject.optBoolean(JSONUtils.KEY_UT_CW_ENABLED, false);
                    handleQueryCallWaitingOK(enabled);
                    break;
                }
                case JSONUtils.EVENT_CODE_UT_UPDATE_CB_OK:
                case JSONUtils.EVENT_CODE_UT_UPDATE_CF_OK:
                case JSONUtils.EVENT_CODE_UT_UPDATE_CW_OK: {
                    handleUpdateActionOK();
                    break;
                }
                case JSONUtils.EVENT_CODE_UT_QUERY_CB_FAILED:
                case JSONUtils.EVENT_CODE_UT_QUERY_CF_FAILED:
                case JSONUtils.EVENT_CODE_UT_QUERY_CW_FAILED:
                case JSONUtils.EVENT_CODE_UT_UPDATE_CB_FAILED:
                case JSONUtils.EVENT_CODE_UT_UPDATE_CF_FAILED:
                case JSONUtils.EVENT_CODE_UT_UPDATE_CW_FAILED: {
                    int stateCode = jObject.optInt(JSONUtils.KEY_STATE_CODE, 0);
                    handleActionFailed(stateCode);
                    break;
                }
            }
        } catch (JSONException e) {
            Log.e(TAG, "Failed to handle the event as catch the JSONException: " + e);
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to handle the event as catch the RemoteException: " + e);
        }
    }

    private void handleQueryCallForwardOK(ArrayList<ImsCallForwardInfo> infoList)
            throws RemoteException {
        if (Utilities.DEBUG) Log.i(TAG, "Query call forward finished, the result is OK.");

        mCmdManager.onQueryCallForwardFinished(infoList);
    }

    private void handleQueryCallBarringOK(ArrayList<CallBarringInfo> infoList)
            throws RemoteException {
        if (Utilities.DEBUG) Log.i(TAG, "Query call barring finished, the result is OK.");

        mCmdManager.onQueryCallBarringFinished(infoList);
    }

    private void handleQueryCallWaitingOK(boolean enabled) throws RemoteException {
        if (Utilities.DEBUG) Log.i(TAG, "Query call waiting finished, the result is OK.");

        mCmdManager.onQueryCallWaitingFinished(enabled);
   }

    private void handleUpdateActionOK() throws RemoteException {
        if (Utilities.DEBUG) Log.i(TAG, "Update action finished, the result is ok.");

        mCmdManager.onUpdateActionSuccessed();
    }

    private void handleActionFailed(int stateCode) throws RemoteException {
        if (Utilities.DEBUG) Log.i(TAG, "Update action finished, the result is failed.");
        ImsReasonInfo error = new ImsReasonInfo(ImsReasonInfo.CODE_UT_NETWORK_ERROR, stateCode);
        mCmdManager.onActionFailed(error);
    }

    private ArrayList<ImsCallForwardInfo> parseCallForwardInfos(String jsonString)
            throws JSONException {
        if (Utilities.DEBUG) Log.i(TAG, "Parse the CF infos from the json: " + jsonString);
        if (TextUtils.isEmpty(jsonString)) {
            Log.e(TAG, "Failed to parse the call forward info as the json string is empty.");
            return null;
        }

        ArrayList<ImsCallForwardInfo> infoList = new ArrayList<ImsCallForwardInfo>();

        JSONObject jObject = new JSONObject(jsonString);
        int timeSeconds = jObject.optInt(JSONUtils.KEY_UT_CF_TIME_SECONDS, 0);
        JSONArray rules = jObject.optJSONArray(JSONUtils.KEY_UT_CF_RULES);
        for (int i = 0; i < rules.length(); i++) {
            JSONObject rule = rules.getJSONObject(i);
            boolean enabled = rule.optBoolean(JSONUtils.KEY_UT_CF_RULE_ENABLED, true);
            String media = rule.optString(JSONUtils.KEY_UT_CF_RULE_MEDIA, null);
            JSONArray conditions = rule.optJSONArray(JSONUtils.KEY_UT_CF_CONDS);
            String targetNumber = rule.optString(JSONUtils.KEY_UT_CF_ACTION_TARGET, null);

            ImsCallForwardInfo info = new ImsCallForwardInfo();
            info.mToA = 0x81; // 0x81 means Unknown.
            info.mTimeSeconds = timeSeconds;
            info.mStatus = enabled ? 1 : 0;
            info.mNumber = targetNumber;
            if (conditions != null && conditions.length() == 1) {
                // This is the normal result.
                info.mCondition = conditions.getInt(0);
            } else {
                Log.w(TAG, "The condition is abnormal, please check rule: " + rule.toString());
            }
            if (TextUtils.isEmpty(media)) {
                info.mServiceClass = SERVICE_CLASS_ALL;
            } else if (JSONUtils.RULE_MEDIA_AUDIO.equals(media)) {
                info.mServiceClass = SERVICE_CLASS_VOICE;
            } else if (JSONUtils.RULE_MEDIA_VIDEO.equals(media)) {
                info.mServiceClass = SERVICE_CLASS_VIDEO;
            } else {
                Log.w(TAG, "The rule's media is: " + media + ", can not parse.");
            }
            infoList.add(info);
        }

        return infoList;
    }

    private ArrayList<CallBarringInfo> parseCallBarringInfos(String jsonString)
            throws JSONException {
        if (Utilities.DEBUG) Log.i(TAG, "Parse the CB infos from the json: " + jsonString);
        if (TextUtils.isEmpty(jsonString)) {
            Log.e(TAG, "Failed to parse the call barring info as the json string is empty.");
            return null;
        }

        ArrayList<CallBarringInfo> infoList = new ArrayList<CallBarringInfo>();

        JSONObject jObject = new JSONObject(jsonString);
        JSONArray rules = jObject.optJSONArray(JSONUtils.KEY_UT_CB_RULES);
        for (int i = 0; i < rules.length(); i++) {
            int condition = 0;
            int status = 0;
            JSONObject rule = rules.getJSONObject(i);
            boolean enabled = rule.optBoolean(JSONUtils.KEY_UT_CB_RULE_ENABLED, true);
            JSONArray conditions = rule.optJSONArray(JSONUtils.KEY_UT_CB_CONDS);
            status = enabled ? 1 : 0;
            if (conditions != null && conditions.length() == 1) {
                // This is the normal result.
                condition = conditions.getInt(0);
            } else {
                Log.w(TAG, "The condition is abnormal, please check rule: " + rule.toString());
            }
            CallBarringInfo info = new CallBarringInfo();
            info.setCondition(condition);
            info.setStatus(status);
            infoList.add(info);
        }
        return infoList;
    }

    private ImsCallForwardInfo[] getFromEx(ImsCallForwardInfoEx[] infoExs) {
        if (infoExs == null || infoExs.length < 1) {
            Log.d(TAG, "There isn't extention call forward info.");
            return null;
        }

        ImsCallForwardInfo[] infos = new ImsCallForwardInfo[infoExs.length];
        for (int i = 0; i < infoExs.length; i++) {
            ImsCallForwardInfo info = new ImsCallForwardInfo();
            info.mCondition = infoExs[i].mCondition;
            info.mStatus = infoExs[i].mStatus;
            info.mToA = infoExs[i].mToA;
            info.mServiceClass = infoExs[i].mServiceClass;
            info.mNumber = infoExs[i].mNumber;
            info.mTimeSeconds = infoExs[i].mTimeSeconds;

            infos[i] = info;
        }

        return infos;
    }

    private ImsCallForwardInfoEx[] findCallForwardInfoEx(ArrayList<ImsCallForwardInfo> infoList,
            int condition, String number, int serviceClass, String ruleset) {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Try to find the call forward info ex for condition[" + condition
                    + "] serviceClass[" + serviceClass + "]");
        }

        ArrayList<ImsCallForwardInfoEx> infos = new ArrayList<ImsCallForwardInfoEx>();
        ArrayList<String> items = getCFContainsItems(condition, serviceClass);
        for (ImsCallForwardInfo info : infoList) {
            if (isCFConditionMatched(condition, info.mCondition)
                    && (TextUtils.isEmpty(number) || number.equals(info.mNumber))
                    && (isAllSerClass(serviceClass) || (serviceClass & info.mServiceClass) > 0)) {
                if (isAllSerClass(serviceClass) && info.mServiceClass == SERVICE_CLASS_ALL) {
                    // It means this info matched video and audio class. And we need create two
                    // info as video service class and audio service class.
                    items.remove(getCFItem(info.mCondition, SERVICE_CLASS_VOICE));
                    infos.add(cloneCFInfoEx(info, SERVICE_CLASS_VOICE, ruleset));
                    items.remove(getCFItem(info.mCondition, SERVICE_CLASS_VIDEO));
                    infos.add(cloneCFInfoEx(info, SERVICE_CLASS_VIDEO, ruleset));
                    Log.d(TAG, "Found CF info for condition[" + info.mCondition + "].");
                } else {
                    // It means this info only matched one service class.
                    int newServiceClass =
                            isAllSerClass(serviceClass) ? info.mServiceClass : serviceClass;
                    items.remove(getCFItem(info.mCondition, newServiceClass));
                    infos.add(cloneCFInfoEx(info, newServiceClass, ruleset));
                    Log.d(TAG, "Found CF info for condition[" + info.mCondition
                            + "] serviceClass[" + newServiceClass + "].");
                }
            }
        }

        if (items.size() > 0) {
            for (String item : items) {
                // Do not found the matched CF info, we'd like to give the result as deactivate
                int[] temp = getInfoFromItem(item);
                ImsCallForwardInfoEx newInfo = new ImsCallForwardInfoEx();
                newInfo.mToA = 0x81; // 0x81 means Unknown.
                newInfo.mTimeSeconds = 20;
                newInfo.mCondition = temp[0];
                newInfo.mServiceClass = temp[1];
                newInfo.mNumber = number;
                newInfo.mStatus = 0; // Set it as deactivate
                newInfo.mNumberType = 0;
                newInfo.mRuleset = ruleset;
                Log.d(TAG, "Build the deactive CF infoEx: " + newInfo);
                infos.add(newInfo);
            }
        }

        if (infos.size() <= 0) {
            Log.w(TAG, "Do not find any CF info. Please check!");
            return null;
        }

        ImsCallForwardInfoEx[] res = new ImsCallForwardInfoEx[infos.size()];
        infos.toArray(res);
        return res;
    }

    private ArrayList<String> getCFContainsItems(int condition, int serviceClass) {
        // Build the condition list.
        ArrayList<Integer> conditionList = new ArrayList<Integer>();
        if (condition == ImsUtInterface.CDIV_CF_ALL
                || condition == ImsUtInterface.CDIV_CF_ALL_CONDITIONAL) {
            // If the condition is CF_ALL, it means we need give all the condition result.
            if (condition == ImsUtInterface.CDIV_CF_ALL) {
                conditionList.add(Integer.valueOf(ImsUtInterface.CDIV_CF_UNCONDITIONAL));
            }
            conditionList.add(Integer.valueOf(ImsUtInterface.CDIV_CF_BUSY));
            conditionList.add(Integer.valueOf(ImsUtInterface.CDIV_CF_NO_REPLY));
            conditionList.add(Integer.valueOf(ImsUtInterface.CDIV_CF_NOT_REACHABLE));
            conditionList.add(Integer.valueOf(ImsUtInterface.CDIV_CF_NOT_LOGGED_IN));
        } else {
            conditionList.add(Integer.valueOf(condition));
        }

        // Build the service class list.
        ArrayList<Integer> serviceClassList = new ArrayList<Integer>();
        if (isAllSerClass(serviceClass)) {
            serviceClassList.add(Integer.valueOf(SERVICE_CLASS_VOICE));
            serviceClassList.add(Integer.valueOf(SERVICE_CLASS_VIDEO));
        } else {
            serviceClassList.add(serviceClass);
        }

        // Build the keys.
        ArrayList<String> keyList = new ArrayList<String>();
        for (Integer cond : conditionList) {
            for (Integer serClass : serviceClassList) {
                keyList.add(getCFItem(cond, serClass));
            }
        }

        Log.d(TAG, "The CF must contains items size is: " + keyList.size());
        return keyList;
    }

    private String getCFItem(Integer condition, Integer serviceClass) {
        return condition + "," + serviceClass;
    }

    private int[] getInfoFromItem(String key) {
        String[] temp = key.split(",");
        return new int[] { Integer.valueOf(temp[0]), Integer.valueOf(temp[1]) };
    }

    private boolean isCFConditionMatched(int queryCondition, int infoCondition) {
        if (queryCondition == ImsUtInterface.CDIV_CF_ALL) {
            // If the query condition is CF_ALL, set it as matched.
            return true;
        } else if (queryCondition == ImsUtInterface.CDIV_CF_ALL_CONDITIONAL
                && infoCondition != ImsUtInterface.CDIV_CF_UNCONDITIONAL) {
            // If the query condition is CF_ALL_CONDITIONAL, and info condition
            // isn't UNCONDITIONAL, set it as matched.
            return true;
        } else {
            return queryCondition == infoCondition;
        }
    }

    private ImsCallForwardInfoEx cloneCFInfoEx(ImsCallForwardInfo info, int serviceClass,
            String ruleset) {
        ImsCallForwardInfoEx newInfo = new ImsCallForwardInfoEx();
        newInfo.mToA = info.mToA;
        newInfo.mTimeSeconds = info.mTimeSeconds;
        newInfo.mCondition = info.mCondition;
        newInfo.mServiceClass = serviceClass;
        newInfo.mNumber = info.mNumber;
        newInfo.mStatus = info.mStatus; // Set it as deactivate
        newInfo.mNumberType = 0;
        newInfo.mRuleset = ruleset;
        return newInfo;
    }

    private int[] findCallBarringInfo(ArrayList<CallBarringInfo> infoList, int condition,
            int serviceClass) {
        int[] infos = new int[2];
        CallBarringInfo matchedInfo = null;
        for (CallBarringInfo info : infoList) {
            if (info.mCondition == condition) {
                matchedInfo = info;
            }
        }

        if (matchedInfo != null) {
            Log.d(TAG, "Found the matched CB info: " + matchedInfo);
            infos[0] = matchedInfo.mStatus;
            infos[1] = serviceClass;
            return infos;
        } else {
            // Do not found the matched CB info, we'd like to give the result as deactivate.
            return null;
        }
    }

    private int getConditionFromCFReason(int reason) {
        switch(reason) {
            case CommandsInterface.CF_REASON_UNCONDITIONAL:
                return ImsUtInterface.CDIV_CF_UNCONDITIONAL;
            case CommandsInterface.CF_REASON_BUSY:
                return ImsUtInterface.CDIV_CF_BUSY;
            case CommandsInterface.CF_REASON_NO_REPLY:
                return ImsUtInterface.CDIV_CF_NO_REPLY;
            case CommandsInterface.CF_REASON_NOT_REACHABLE:
                return ImsUtInterface.CDIV_CF_NOT_REACHABLE;
            case CommandsInterface.CF_REASON_ALL:
                return ImsUtInterface.CDIV_CF_ALL;
            case CommandsInterface.CF_REASON_ALL_CONDITIONAL:
                return ImsUtInterface.CDIV_CF_ALL_CONDITIONAL;
            default:
                break;
        }

        return ImsUtInterface.INVALID;
    }

    private int getConditionFromCBReason(String sc) {
        if (sc == null) {
            throw new RuntimeException("invalid call barring sc");
        }
        Log.d(TAG, "getConditionFromCBReason the reason is: " + sc);

        if (sc.equals(CommandsInterface.CB_FACILITY_BAOC)) {
            return ImsUtInterface.CB_BAOC;
        } else if (sc.equals(CommandsInterface.CB_FACILITY_BAOIC)) {
            return ImsUtInterface.CB_BOIC;
        } else if (sc.equals(CommandsInterface.CB_FACILITY_BAOICxH)) {
            return ImsUtInterface.CB_BOIC_EXHC;
        } else if (sc.equals(CommandsInterface.CB_FACILITY_BAIC)) {
            return ImsUtInterface.CB_BAIC;
        } else if (sc.equals(CommandsInterface.CB_FACILITY_BAICr)) {
            return ImsUtInterface.CB_BIC_WR;
        } else if (sc.equals(CommandsInterface.CB_FACILITY_BA_ALL)) {
            return ImsUtInterface.CB_BA_ALL;
        } else if (sc.equals(CommandsInterface.CB_FACILITY_BA_MO)) {
            return ImsUtInterface.CB_BAOC;
        } else if (sc.equals(CommandsInterface.CB_FACILITY_BA_MT)) {
            return ImsUtInterface.CB_BA_MT;
        } else {
            throw new RuntimeException ("invalid call barring sc");
        }
    }

    private int getActionFromCFAction(int cfAction) {
        switch (cfAction) {
            case CommandsInterface.CF_ACTION_DISABLE:
                return ImsUtInterface.ACTION_DEACTIVATION;
            case CommandsInterface.CF_ACTION_ENABLE:
                return ImsUtInterface.ACTION_ACTIVATION;
            case CommandsInterface.CF_ACTION_ERASURE:
                return ImsUtInterface.ACTION_ERASURE;
            case CommandsInterface.CF_ACTION_REGISTRATION:
                return ImsUtInterface.ACTION_REGISTRATION;
            default:
                break;
        }

        return ImsUtInterface.INVALID;
    }

    private boolean isAllSerClass(int serviceClass) {
        return serviceClass != SERVICE_CLASS_VOICE && serviceClass != SERVICE_CLASS_VIDEO;
    }

    private class CmdManager {
        private static final int ACTION_TYPE_QUERY = 0;
        private static final int ACTION_TYPE_UPDATE = 1;

        private boolean mHandleCmd = false;

        private AtomicInteger mCmdKeyMgr = null;
        private LinkedList<Integer> mCmds = null;
        private HashMap<Integer, UTAction> mUTActions = null;

        public CmdManager() {
            mCmdKeyMgr = new AtomicInteger(101);
            mCmds = new LinkedList<Integer>();
            mUTActions = new HashMap<Integer, UTAction>();
        }

        public int addCmd(UTAction action) {
            Integer key = mCmdKeyMgr.getAndIncrement();
            Log.d(TAG, "The new action will be added to cmd list with key: " + key);

            synchronized (mCmds) {
                mCmds.add(key);
                mUTActions.put(key, action);
            }

            if (mUtEnabled) {
                mHandler.removeMessages(MSG_DISABLE_UT);
                processPendingAction();
            } else {
                mUtManager.prepare(Utilities.getPrimaryCardSubId(mContext));
            }
            return key;
        }

        private void processPendingAction() {
            if (mHandleCmd) {
                Log.d(TAG, "There is cmd in processing, can not process the other cmds.");
                return;
            }

            if (mCmds.size() < 1) {
                Log.d(TAG, "There isn't any pending action, Disable the UT after "
                        + DELAY_DISALBE_UT + "ms.");
                mHandler.sendEmptyMessageDelayed(MSG_DISABLE_UT, DELAY_DISALBE_UT);
                return;
            }

            if (mUtEnabled) {
                // Get the first cmd, send the pending action to handler.
                mHandleCmd = true;

                Integer key = mCmds.getFirst();
                UTAction action = mUTActions.get(key);

                Message msg = new Message();
                msg.what = action._action;
                msg.obj = action;

                mHandler.sendMessage(msg);
                Log.d(TAG, "The cmd " + action._name + " will be handled now.");

                if (action._timeoutMillis > 0) {
                    Message timeoutMsg = new Message();
                    timeoutMsg.what = MSG_CMD_TIMEOUT;
                    timeoutMsg.arg1 = key;
                    mHandler.sendMessageDelayed(timeoutMsg, action._timeoutMillis);
                }
            } else {
                // As UT disabled, handle the CMD as failed.
                ImsReasonInfo error = new ImsReasonInfo(ImsReasonInfo.CODE_UT_NETWORK_ERROR,
                        ImsReasonInfo.CODE_UNSPECIFIED);
                onActionFailed(error);
            }
        }

        public Integer getFirstCmd() {
            return mCmds.size() > 0 ? mCmds.getFirst() : ImsUtInterface.INVALID;
        }

        public void onActionFailed(ImsReasonInfo error) {
            synchronized (mCmds) {
                if (mCmds.size() < 1) return;

                Integer key = mCmds.getFirst();
                UTAction action = mUTActions.get(key);
                try {
                    // Notify the action failed.
                    int actionType = getActionType(action._action);
                    if (actionType == ACTION_TYPE_QUERY) {
                        Log.d(TAG, "Action failed for query action, and is extension: "
                                + action._isExAction);
                        if (action._isExAction && mListenerEx != null) {
                            mListenerEx.utConfigurationQueryFailed(ImsUtImpl.this, key, error);
                        } else if (mListener != null) {
                            mListener.utConfigurationQueryFailed(ImsUtImpl.this, key, error);
                        }
                    } else if (actionType == ACTION_TYPE_UPDATE) {
                        Log.d(TAG, "Action failed for update action, and is extension: "
                                + action._isExAction);
                        if (action._isExAction && mListenerEx != null) {
                            mListenerEx.utConfigurationUpdateFailed(ImsUtImpl.this, key, error);
                        } else if (mListener != null) {
                            mListener.utConfigurationUpdateFailed(ImsUtImpl.this, key, error);
                        }
                    }
                } catch (RemoteException e) {
                    Log.e(TAG, "Failed to notify the ut configuration acton failed result.");
                    Log.e(TAG, "Catch the RemoteException: " + e);
                }

                mUTActions.remove(key);
                mCmds.remove(key);
                mHandleCmd = false;
            }

            // After action failed, we need try to start the next pending action.
            processPendingAction();
        }

        public void onQueryCallForwardFinished(ArrayList<ImsCallForwardInfo> callForwardInfos)
                throws RemoteException {
            synchronized (mCmds) {
                if (!mHandleCmd) {
                    Log.e(TAG, "Do not handle any cmd now, shouldn't query CF finished.");
                    return;
                }

                if (mCmds.size() < 1) {
                    Log.e(TAG, "There isn't any pending action, shouldn't query CF finished.");
                    return;
                }

                Integer key = mCmds.getFirst();
                UTAction action = mUTActions.get(key);
                Log.d(TAG, "Query CF finished, action._action is: " + action._action);
                if (action._action == MSG_ACTION_QUERY_CALL_FORWARD) {
                    ImsCallForwardInfo[] infos = getFromEx(
                            findCallForwardInfoEx(callForwardInfos,
                                    (Integer) action._params.get(0), // condition
                                    (String) action._params.get(1), // number
                                    SERVICE_CLASS_ALL,
                                    null));
                    if (infos != null) {
                        // Find the call forward info for this action.
                        Log.d(TAG, "Success to query the call forward infos: " + infos);
                        if (mListener != null) {
                            mListener.utConfigurationCallForwardQueried(
                                    ImsUtImpl.this, key, infos);
                        }
                    } else {
                        // Can not find the call forward info for this action.
                        Log.w(TAG, "Failed to query call forward as can not found matched item.");
                        if (mListener != null) {
                            mListener.utConfigurationQueryFailed(
                                    ImsUtImpl.this, key, new ImsReasonInfo());
                        }
                    }
                } else if (action._action == MSG_ACTION_QUERY_CALL_FORWARDING_OPTION) {
                    ImsCallForwardInfoEx[] infoExs = findCallForwardInfoEx(callForwardInfos,
                            getConditionFromCFReason((Integer) action._params.get(0)),
                            null,
                            (Integer) action._params.get(1),
                            (String) action._params.get(2));
                    if (infoExs != null && infoExs.length > 0) {
                        // Find the call forward info for this action.
                        Log.d(TAG, "Success to query the call forward infoExs: " + infoExs);
                        if (mListenerEx != null) {
                            mListenerEx.utConfigurationCallForwardQueried(
                                    ImsUtImpl.this, key, infoExs);
                        }
                    } else {
                        // Can not find the call forward info for this action.
                        Log.w(TAG, "Failed to query call forward as can not found matched item.");
                        if (mListenerEx != null) {
                            mListenerEx.utConfigurationQueryFailed(
                                    ImsUtImpl.this, key, new ImsReasonInfo());
                        }
                    }
                } else {
                    Log.e(TAG, "The action do not handle: " + action._action);
                }

                mUTActions.remove(key);
                mCmds.remove(key);
                mHandleCmd = false;
            }

            // After action finished, we need try to start the next pending action.
            processPendingAction();
        }

        public void onQueryCallBarringFinished(ArrayList<CallBarringInfo> infolist)
                throws RemoteException {
            synchronized (mCmds) {
                if (!mHandleCmd) {
                    Log.e(TAG, "Do not handle any cmd now, shouldn't query CB finished.");
                    return;
                }

                if (mCmds.size() < 1) {
                    Log.e(TAG, "There isn't any pending action, shouldn't query CB finished.");
                    return;
                }

                Integer key = mCmds.getFirst();
                UTAction action = mUTActions.get(key);
                Log.d(TAG, "Query call barring finished, action is " + action._action);

                int[] info = findCallBarringInfo(infolist, (Integer) action._params.get(0),
                        (Integer) action._params.get(2));
                // Find the call barring info for this action.
                Log.d(TAG, "Success to query the call barring info: " + info);
                if (info != null && info.length > 0) {
                    if (mListenerEx != null) {
                        mListenerEx.utConfigurationCallBarringResult(key, info);
                    }
                } else {
                    mListenerEx.utConfigurationCallBarringFailed(
                            key, null, ImsReasonInfo.CODE_UT_NETWORK_ERROR);
                }

                mUTActions.remove(key);
                mCmds.remove(key);
                mHandleCmd = false;
            }
            // After action finished, we need try to start the next pending action.
            processPendingAction();
        }

        public void onQueryCallWaitingFinished(boolean enabled) throws RemoteException {
            synchronized (mCmds) {
                if (!mHandleCmd) {
                    Log.e(TAG, "Do not handle any cmd now, shouldn't query CW finished.");
                    return;
                }

                if (mCmds.size() < 1) {
                    Log.e(TAG, "There isn't any pending action, shouldn't query CW finished.");
                    return;
                }

                Integer key = mCmds.getFirst();
                if (mListener != null) {
                    ImsSsInfo info = new ImsSsInfo();
                    info.mStatus = enabled ? ImsSsInfo.ENABLED : ImsSsInfo.DISABLED;
                    mListener.utConfigurationCallWaitingQueried(ImsUtImpl.this, key,
                            new ImsSsInfo[] { info });
                }

                mUTActions.remove(key);
                mCmds.remove(key);
                mHandleCmd = false;
            }

            // After action finished, we need try to start the next pending action.
            processPendingAction();
        }

        public void onUpdateActionSuccessed()
                throws RemoteException {
            synchronized (mCmds) {
                if (mCmds.size() < 1) {
                    Log.e(TAG, "There isn't any pending action, shouldn't update action finished.");
                    return;
                }

                Integer key = mCmds.getFirst();
                UTAction action = mUTActions.get(key);
                if (action != null) {
                    if (action._isExAction && mListenerEx != null) {
                        mListenerEx.utConfigurationUpdated(ImsUtImpl.this, key);
                    } else if (mListener != null) {
                        mListener.utConfigurationUpdated(ImsUtImpl.this, key);
                    }
                }

                mUTActions.remove(key);
                mCmds.remove(key);
                mHandleCmd = false;
            }

            // After action finished, we need try to start the next pending action.
            processPendingAction();
        }

        private int getActionType(int msgAction) {
            switch (msgAction) {
                case MSG_ACTION_QUERY_CALL_BARRING:
                case MSG_ACTION_QUERY_CALL_FORWARD:
                case MSG_ACTION_QUERY_CALL_WAITING:
                case MSG_ACTION_QUERY_CALL_FORWARDING_OPTION:
                case MSG_ACTION_QUERY_FACILITY_LOCK:
                    return ACTION_TYPE_QUERY;
                case MSG_ACTION_UPDATE_CALL_BARRING:
                case MSG_ACTION_UPDATE_CALL_FORWARD:
                case MSG_ACTION_UPDATE_CALL_WAITING:
                case MSG_ACTION_UPDATE_CALL_FORWARDING_OPTION:
                case MSG_ACTION_SET_FACILITY_LOCK:
                    return ACTION_TYPE_UPDATE;
            }

            Log.e(TAG, "Shouldn't be here, do not defined this action now.");
            return ACTION_TYPE_UPDATE;
        }
    }

    private class UTAction extends PendingAction {
        public int _timeoutMillis;
        public boolean _isExAction;

        public UTAction(String name, int action, int timeout, Object... params) {
            this(false, name, action, timeout, params);
        }

        public UTAction(boolean isExAction, String name, int action, int timeout,
                Object... params) {
            super(name, action, params);
            _timeoutMillis = timeout;
            _isExAction = isExAction;
        }
    }

    private class UtServiceCallback extends IVoWifiUTCallback.Stub {
        @Override
        public void onEvent(String json) {
            if (Utilities.DEBUG) Log.i(TAG, "Get the vowifi ser event callback.");
            if (TextUtils.isEmpty(json)) {
                Log.e(TAG, "Can not handle the ser callback as the json is null.");
                return;
            }

            Message msg = mHandler.obtainMessage(MSG_HANDLE_EVENT);
            msg.obj = json;
            mHandler.sendMessage(msg);
        }
    }

}

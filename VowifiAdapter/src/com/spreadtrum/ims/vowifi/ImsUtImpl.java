
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
import com.android.internal.telephony.CommandsInterface;
import com.spreadtrum.ims.vowifi.Utilities.JSONUtils;
import com.spreadtrum.ims.vowifi.Utilities.PendingAction;
import com.spreadtrum.ims.vowifi.Utilities.Result;
import com.spreadtrum.ims.vowifi.Utilities.CallBarringInfo;
import com.spreadtrum.ims.vowifi.VoWifiCallManager.ICallChangedListener;
import com.spreadtrum.vowifi.service.IVoWifiSerService;
import com.spreadtrum.vowifi.service.IVoWifiSerServiceCallback;

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

    // We'd like to handle the timeout in the native. Disable the timeout action now.
    private static final boolean HANDLE_TIMEOUT = false;

    // Call forward service class
    private static final int SERVICE_CLASS_VOICE = CommandsInterface.SERVICE_CLASS_VOICE;
    // FIXME: This value defined in CallForwardEditPreference but not CommandsInterface.
    private static final int SERVICE_CLASS_VIDEO = 2;

    private Context mContext;

    private IImsUtListener mListener = null;
    private IImsUtListenerEx mListenerEx = null;

    private CmdManager mCmdManager = null;
    private VoWifiCallManager mCallManager = null;
    private IVoWifiSerService mICall = null;
    private MySerServiceCallback mSerServiceCallback = new MySerServiceCallback();

    private static final int CMD_TIMEOUT = 10000; // 10s

    private static final int MSG_HANDLE_EVENT = -1;
    private static final int MSG_CMD_TIMEOUT = 0;

    private static final int MSG_ACTION_QUERY_CALL_BARRING = 1 << 0;
    private static final int MSG_ACTION_QUERY_CALL_FORWARD = 1 << 1;
    private static final int MSG_ACTION_QUERY_CALL_FORWARDING_OPTION = 1 << 2;
    private static final int MSG_ACTION_QUERY_CALL_WAITING = 1 << 3;
    private static final int MSG_ACTION_UPDATE_CALL_BARRING = 1 << 4;
    private static final int MSG_ACTION_UPDATE_CALL_FORWARD = 1 << 5;
    private static final int MSG_ACTION_UPDATE_CALL_FORWARDING_OPTION = 1 << 6;
    private static final int MSG_ACTION_UPDATE_CALL_WAITING = 1 << 7;
    private static final int MSG_ACTION_SET_FACILITY_LOCK = 1 << 8;
    private static final int MSG_ACTION_QUERY_FACILITY_LOCK = 1 << 9;
    private static final int MSG_ACTION_CHANGE_LOCK_PWD = 1 << 10;
	
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
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
                    }
                    break;
                }
                case MSG_ACTION_QUERY_CALL_BARRING: 
		  case MSG_ACTION_QUERY_FACILITY_LOCK:
		  {
                    UTAction action = (UTAction) msg.obj;
		      int condition = getConditionFromCBReason((String) action._params.get(0));
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
		  case MSG_ACTION_SET_FACILITY_LOCK:
                    break;
                case MSG_ACTION_UPDATE_CALL_FORWARD:
                    break;
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
		case MSG_ACTION_CHANGE_LOCK_PWD:{
		     UTAction action = (UTAction) msg.obj;
		     String condition =  (String) action._params.get(0);
		     String oldPwd =  (String) action._params.get(1);
		     String newPwd =  (String) action._params.get(2);
		     nativeChangeBarringPwd(condition, oldPwd, newPwd);
		     break;
               }
            }
        }
    };

    // To listener the IVoWifiSerService changed.
    private ICallChangedListener mICallChangedListener = new ICallChangedListener() {
        @Override
        public void onChanged(IVoWifiSerService newCallInterface) {
            mICall = newCallInterface;
            if (mICall == null) return;

            // The new call Interface is not null now, register the callback.
            try {
                mICall.registerCallback(mSerServiceCallback);
            } catch (RemoteException e) {
                Log.e(TAG, "Failed to register callback for UT as catch RemoteException: " + e);
            }
        }
    };

    protected ImsUtImpl(Context context, VoWifiCallManager callManager) {
        mContext = context;
        mCmdManager = new CmdManager();
        mCallManager = callManager;

        // Register the service changed to get the IVowifiService.
        mCallManager.registerCallInterfaceChanged(mICallChangedListener);
    }

    @Override
    protected void finalize() throws Throwable {
        // Un-register the service changed.
        mCallManager.unregisterCallInterfaceChanged(mICallChangedListener);
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
        Log.w(TAG, "Do not support query CLIR now.");
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
    public int updateCallBarring(int cbType, int enable, String[] barrList) {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Try to update the call barring with the type: " + cbType + ", enabled: "
                    + enable + ", barrList: " + Utilities.getString(barrList));
        }

        return ImsUtInterface.INVALID;
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

        return ImsUtInterface.INVALID;
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
        Log.w(TAG, "Do not support update CLIR now.");
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

        UTAction action = new UTAction("setCallForwardingOption",
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

        UTAction action = new UTAction("getCallForwardingOption",
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

	UTAction action = new UTAction("setFacilityLock",
                MSG_ACTION_SET_FACILITY_LOCK, CMD_TIMEOUT,
               facility, Boolean.valueOf(lockState), password);
        return mCmdManager.addCmd(action);
    }

     public int queryFacilityLock(String facility, String password, int serviceClass){
        if (Utilities.DEBUG) {
            Log.i(TAG, "queryFacilityLock, reason: " + facility
                    + ", password: " + password + ", serviceclass: " + serviceClass);
        }

	UTAction action = new UTAction("queryFacilityLock",
                MSG_ACTION_QUERY_FACILITY_LOCK, CMD_TIMEOUT,
               facility, password, Integer.valueOf(serviceClass));
        return mCmdManager.addCmd(action);
    }

    public int changeBarringPassword(String facility, String oldPwd, String newPwd){

		if (Utilities.DEBUG) {
            Log.i(TAG, "changeBarringPassword, reason: " + facility
                    + ", old password: " + oldPwd + ", new password: " + newPwd);
        }
	UTAction action = new UTAction("changeBarringPassword",
                MSG_ACTION_CHANGE_LOCK_PWD, CMD_TIMEOUT,
               facility, oldPwd, newPwd);
        return mCmdManager.addCmd(action);
    }

    private void nativeQueryCallBarring(int condition) {
        if (Utilities.DEBUG) Log.i(TAG, "Native query the call barring options.");

        boolean success = false;
        try {
            if (mICall != null) {
                int res = mICall.queryCallBarring(condition);
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
            if (mICall != null) {
                int res = mICall.queryCallForward();
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
            if (mICall != null) {
                int res = mICall.queryCallWaiting();
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

    private void nativeUpdateCallForward(int action, int condition, String number, int serviceClass,
            int timeSeconds) {
        if (Utilities.DEBUG) {
            Log.i(TAG, "Native update the call forward options, action: " + action
                    + ", condition: " + condition + ", number: " + number + ", service class: "
                    + serviceClass + ", timeSeconds: " + timeSeconds);
        }

        boolean success = false;
        try {
            if (mICall != null) {
                int res = mICall.updateCallForward(
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
            if (mICall != null) {
                int res = mICall.updateCallWaiting(enabled);
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
                case JSONUtils.EVENT_CODE_UT_UPDATE_CF_OK:
                case JSONUtils.EVENT_CODE_UT_UPDATE_CW_OK: {
                    handleUpdateActionOK();
                    break;
                }
                case JSONUtils.EVENT_CODE_UT_QUERY_CF_FAILED:
                case JSONUtils.EVENT_CODE_UT_QUERY_CW_FAILED:
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
        if (Utilities.DEBUG) Log.i(TAG, "Query call forward finished, the result is OK.");

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
                info.mServiceClass = SERVICE_CLASS_VOICE | SERVICE_CLASS_VIDEO;
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
		status =  enabled?1:0;
		 if (conditions != null && conditions.length() == 1) {
                // This is the normal result.
               condition = conditions.getInt(0);
            } else {
                Log.w(TAG, "The condition is abnormal, please check rule: " + rule.toString());
            }
		CallBarringInfo info =  new CallBarringInfo();
		info.setCondition(condition);
		info.setStatus(status);
		infoList.add(info);
	}
	 return infoList;
   }

    private ImsCallForwardInfo findCallForwardInfo(ArrayList<ImsCallForwardInfo> infoList,
            int condition, String number, int serviceClass, String ruleset) {
        // TODO ruleset removed on 7.0

        ImsCallForwardInfo matchedInfo = null;
        for (ImsCallForwardInfo info : infoList) {
            if (info.mCondition == condition
                    && (TextUtils.isEmpty(number) || number.equals(info.mNumber))
                    && (serviceClass < 0 || (serviceClass & info.mServiceClass) > 0)) {
                matchedInfo = info;
            }
        }

        if (matchedInfo != null) {
            Log.d(TAG, "Found the matched CF info: " + matchedInfo);
            return matchedInfo;
        } else {
            // Do not found the matched CF info, we'd like to give the result as deactivate.
            ImsCallForwardInfo newInfo = new ImsCallForwardInfo();
            newInfo.mToA = 0x81; // 0x81 means Unknown.
            newInfo.mTimeSeconds = 20;
            newInfo.mCondition = condition;
            newInfo.mServiceClass = serviceClass;
            newInfo.mNumber = number;
            newInfo.mStatus = 0; // Set it as deactivate
            Log.d(TAG, "Build the CF info: " + newInfo);
            return newInfo;
        }
    }

    private ImsCallForwardInfoEx findCallForwardInfoEx(ArrayList<ImsCallForwardInfo> infoList,
            int condition, String number, int serviceClass, String ruleset) {
        // TODO ruleset removed on 7.0

        ImsCallForwardInfo matchedInfo = null;
	 ImsCallForwardInfoEx newInfo = new ImsCallForwardInfoEx();
        for (ImsCallForwardInfo info : infoList) {
            if (info.mCondition == condition
                    && (TextUtils.isEmpty(number) || number.equals(info.mNumber))
                    && (serviceClass < 0 || (serviceClass & info.mServiceClass) > 0)) {
                matchedInfo = info;
            }
        }

        if (matchedInfo != null) {
            Log.d(TAG, "Found the matched CF infoEx: " + matchedInfo);
	     newInfo.mToA = matchedInfo.mToA; // 0x81 means Unknown.
            newInfo.mTimeSeconds = matchedInfo.mTimeSeconds;
            newInfo.mCondition = matchedInfo.mCondition;
            newInfo.mServiceClass = matchedInfo.mServiceClass;
            newInfo.mNumber = matchedInfo.mNumber;
            newInfo.mStatus = matchedInfo.mStatus; // Set it as deactivate
            newInfo.mNumberType =  0; 
            newInfo.mRuleset = ruleset; 
            Log.d(TAG, "Build the CF infoEx: " + newInfo);
            return newInfo;
        } else {
            // Do not found the matched CF info, we'd like to give the result as deactivate
            newInfo.mToA = 0x81; // 0x81 means Unknown.
            newInfo.mTimeSeconds = 20;
            newInfo.mCondition = condition;
            newInfo.mServiceClass = serviceClass;
            newInfo.mNumber = number;
            newInfo.mStatus = 0; // Set it as deactivate
            newInfo.mNumberType = 0; 
            newInfo.mRuleset = ruleset; 
            Log.d(TAG, "Build the CF infoEx: " + newInfo);
            return newInfo;
        }
    }

   private int[] findCallBarringInfo(ArrayList<CallBarringInfo> infoList, int condition)
   {
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
	     infos[1] = matchedInfo.mCondition;
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
            throw new RuntimeException ("invalid call barring sc");
        }
	Log.d(TAG, "getConditionFromCBReason the reason is : " + sc);

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
       switch(cfAction) {
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

    private class CmdManager {
        private static final int ACTION_TYPE_QUERY = 0;
        private static final int ACTION_TYPE_UPDATE = 1;
        private static final int ACTION_TYPE_QUERY_EX = 2;
        private static final int ACTION_TYPE_UPDATE_EX = 3;
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

            processPendingAction();
            return key;
        }

        private void processPendingAction() {
            if (mHandleCmd) {
                Log.d(TAG, "There is cmd in processing, can not process the other cmds.");
                return;
            }

            if (mCmds.size() < 1) {
                Log.e(TAG, "There isn't any pending action, do nothing.");
                return;
            }

            // Get the first cmd, send the pending action to handler.
            mHandleCmd = true;

            Integer key = mCmds.getFirst();
            UTAction action = mUTActions.get(key);

            Message msg = new Message();
            msg.what = action._action;
            msg.obj = action;

            mHandler.sendMessage(msg);
            Log.d(TAG, "The cmd " + action._name + " will be handled now.");

            if (HANDLE_TIMEOUT && action._timeoutMillis > 0) {
                Message timeoutMsg = new Message();
                timeoutMsg.what = MSG_CMD_TIMEOUT;
                timeoutMsg.arg1 = key;
                mHandler.sendMessageDelayed(timeoutMsg, action._timeoutMillis);
            }
        }

        public Integer getFirstCmd() {
            return mCmds.size() > 0 ? mCmds.getFirst() : ImsUtInterface.INVALID;
        }

        public void onActionFailed(ImsReasonInfo error) {
            if (!mHandleCmd) {
                Log.e(TAG, "Do not handle any cmd now, shouldn't action failed.");
                return;
            }

            if (mCmds.size() < 1) {
                Log.e(TAG, "There isn't any pending action, shouldn't action failed.");
                return;
            }

            synchronized (mCmds) {
                Integer key = mCmds.getFirst();
                 int actionType = getActionType(mUTActions.get(key)._action);
		   Log.d(TAG, "actionType is : " + actionType);
		   try {
		    // Notify the action failed.
  		       switch(actionType)
  		       {
  		           case ACTION_TYPE_QUERY:
  			       if (mListener != null) {
  			            mListener.utConfigurationQueryFailed(ImsUtImpl.this, key, error);
  			       }
  			       break;
  		             case ACTION_TYPE_QUERY_EX:
  			      if (mListenerEx!= null) {
  			           mListenerEx.utConfigurationQueryFailed(ImsUtImpl.this, key, error);
  			       }
  			       break;
  		            case ACTION_TYPE_UPDATE:
  			      if (mListener!= null) {
  			            mListener.utConfigurationUpdateFailed(ImsUtImpl.this, key, error);
  			       }
  			       break;	
  		            case ACTION_TYPE_UPDATE_EX:
  			       if (mListenerEx!= null) {
  			            mListenerEx.utConfigurationUpdateFailed(ImsUtImpl.this, key, error);
  			      }
  			      break;				   
  		         }
		    }catch (RemoteException e) {
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
            if (!mHandleCmd) {
                Log.e(TAG, "Do not handle any cmd now, shouldn't query CF finished.");
                return;
            }

            if (mCmds.size() < 1) {
                Log.e(TAG, "There isn't any pending action, shouldn't query CF finished.");
                return;
            }

            synchronized (mCmds) {
                Integer key = mCmds.getFirst();
                UTAction action = mUTActions.get(key);
                ImsCallForwardInfo info = null;
		  ImsCallForwardInfoEx infoEx = null;
		  Log.d(TAG, "action._action is : " + action._action);
                if (action._action == MSG_ACTION_QUERY_CALL_FORWARD) {
                    info = findCallForwardInfo(callForwardInfos,
                            (Integer) action._params.get(0), // condition
                            (String) action._params.get(1), // number
                            -1, // For query call forward action, do not give the service class
                            null);
		      if (info != null) {
                        // Find the call forward info for this action.
                         Log.d(TAG, "Success to query the call forward info: " + info);
                        if (mListener != null) {
                            mListener.utConfigurationCallForwardQueried(
                                ImsUtImpl.this, key, new ImsCallForwardInfo[] { info });
                        }
                    }else {
                    // Can not find the call forward info for this action.
                        Log.w(TAG, "Failed to query call forward as can not found matched item.");
                        if (mListener != null) {
                            mListener.utConfigurationQueryFailed(
                                ImsUtImpl.this, key, new ImsReasonInfo());
                        }
                   }
                } else if (action._action == MSG_ACTION_QUERY_CALL_FORWARDING_OPTION) {
                    infoEx = findCallForwardInfoEx(callForwardInfos,
                            getConditionFromCFReason((Integer) action._params.get(0)),
                            null,
                            (Integer) action._params.get(1),
                            (String) action._params.get(2));
		      if (infoEx != null) {
                        // Find the call forward info for this action.
                         Log.d(TAG, "Success to query the call forward infoEx: " + infoEx);
                        if (mListenerEx!= null) {
                            mListenerEx.utConfigurationCallForwardQueried(
                                ImsUtImpl.this, key, new ImsCallForwardInfoEx[] { infoEx });
                        }
                    }else {
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

        public void onQueryCallBarringFinished(ArrayList<CallBarringInfo> infolist) throws RemoteException {
           if (!mHandleCmd) {
                Log.e(TAG, "Do not handle any cmd now, shouldn't query CB finished.");
                return;
            }

            if (mCmds.size() < 1) {
                Log.e(TAG, "There isn't any pending action, shouldn't query CB finished.");
                return;
            }		
	     synchronized (mCmds) {
                Integer key = mCmds.getFirst();
                UTAction action = mUTActions.get(key);
                int[] info;
		  Log.d(TAG, "action._action is : " + action._action);
	         info = findCallBarringInfo(infolist,
                            getConditionFromCBReason((String) action._params.get(0)));
                        // Find the call barring info for this action.
                Log.d(TAG, "Success to query the call barring info: " + info);
		  if(info != null)
		  {
                    if (mListenerEx!= null) {
                           mListenerEx.utConfigurationCallBarringResult(key, info);
                    }
		  }
		  else 
		  {
		       mListenerEx.utConfigurationCallBarringResult(key, null);
		  }
                mUTActions.remove(key);
                mCmds.remove(key);
                mHandleCmd = false;
	     	}	
                // After action finished, we need try to start the next pending action.
                processPendingAction();
	 }
	
        public void onQueryCallWaitingFinished(boolean enabled) throws RemoteException {
            if (!mHandleCmd) {
                Log.e(TAG, "Do not handle any cmd now, shouldn't query CW finished.");
                return;
            }

            if (mCmds.size() < 1) {
                Log.e(TAG, "There isn't any pending action, shouldn't query CW finished.");
                return;
            }

            synchronized (mCmds) {
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
            if (mCmds.size() < 1) {
                Log.e(TAG, "There isn't any pending action, shouldn't update action finished.");
                return;
            }

            synchronized (mCmds) {
                Integer key = mCmds.getFirst();
                if (mListener != null) {
                    mListener.utConfigurationUpdated(ImsUtImpl.this, key);
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
                    return ACTION_TYPE_QUERY;
		  case MSG_ACTION_QUERY_CALL_FORWARDING_OPTION:
		  case MSG_ACTION_QUERY_FACILITY_LOCK:
		      return ACTION_TYPE_QUERY_EX;
                case MSG_ACTION_UPDATE_CALL_BARRING:
                case MSG_ACTION_UPDATE_CALL_FORWARD:
                case MSG_ACTION_UPDATE_CALL_WAITING:
                    return ACTION_TYPE_UPDATE;
		  case MSG_ACTION_UPDATE_CALL_FORWARDING_OPTION:
		  case MSG_ACTION_SET_FACILITY_LOCK:
		  	return ACTION_TYPE_UPDATE_EX;
            }

            Log.e(TAG, "Shouldn't be here, do not defined this action now.");
            return ACTION_TYPE_UPDATE;
        }
    }

    private class UTAction extends PendingAction {
        public int _timeoutMillis;

        public UTAction(String name, int action, int timeout, Object... params) {
            super(name, action, params);
            _timeoutMillis = timeout;
        }
    }

    private class MySerServiceCallback extends IVoWifiSerServiceCallback.Stub {
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

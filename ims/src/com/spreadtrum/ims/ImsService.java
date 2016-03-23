package com.spreadtrum.ims;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.ServiceManager;
import android.telephony.ServiceState;
import android.telephony.TelephonyManager;
import android.util.Log;

import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneFactory;

import com.android.ims.ImsException;
import com.android.ims.ImsManager;
import com.android.ims.ImsReasonInfo;
import com.android.ims.ImsServiceClass;
import com.android.ims.ImsCallProfile;
import com.android.ims.internal.IImsCallSession;
import com.android.ims.internal.IImsCallSessionListener;
import com.android.ims.internal.IImsRegistrationListener;
import com.android.ims.internal.IImsEcbm;
import com.android.ims.internal.IImsService;
import com.android.ims.internal.IImsUt;
import com.android.ims.internal.IImsUtEx;
import com.android.ims.internal.IImsConfig;
import com.android.ims.internal.ImsCallSession;
import com.spreadtrum.ims.vt.VTManagerProxy;


public class ImsService extends Service {
    private static final String TAG = ImsService.class.getSimpleName();

    /** event code. */
    private static final int EVENT = 100;

    private Map<Integer, ImsServiceImpl> mImsServiceImplMap = new HashMap<Integer, ImsServiceImpl>();

    /**
     * Used to listen to events.
     */
    private final Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case EVENT:
                    break;
                default:
                    break;
            }
        }
    };

    @Override
    public IBinder onBind(Intent intent) {
        iLog("Ims Service onBind.");
        return mImsBinder;
    }

    @Override
    public void onCreate() {
        iLog("Ims Service onCreate.");
        super.onCreate();
        ServiceManager.addService("ims", mImsBinder);
        ServiceManager.addService("imsutex", mImsUtExBinder);
        Phone[] phones = PhoneFactory.getPhones();
        VTManagerProxy.init(this);
        if(phones != null){
            for(Phone phone : phones){
                ImsServiceImpl impl = new ImsServiceImpl(phone,this);
                mImsServiceImplMap.put(impl.getServiceId(), impl);
                VTManagerProxy.getInstance().registerForImsVideoQos(phone);
            }
        }
    }

    @Override
    public void onDestroy() {
        iLog("Ims Service Destroyed.");
        super.onDestroy();
    }

    /*
     * Implement the methods of the IImsService interface in this stub
     */
    private final IImsService.Stub mImsBinder = new IImsService.Stub() {
        @Override
        public int open(int phoneId, int serviceClass, PendingIntent incomingCallIntent,
                IImsRegistrationListener listener) {
            ImsServiceImpl impl = mImsServiceImplMap.get(Integer.valueOf(phoneId+1));
            if(impl != null){
                return impl.open(serviceClass, incomingCallIntent, listener);
            }
            Log.d (TAG, "Open returns serviceId " + phoneId + " ImsServiceImpl:"+impl);
            return 0;
        }


        @Override
        public void close(int serviceId) {
            ImsServiceImpl impl = mImsServiceImplMap.get(Integer.valueOf(serviceId));
            if(impl != null){
                impl.close();
            }
            Log.d (TAG, "close returns serviceId:" + serviceId + " ImsServiceImpl:"+impl);
        }

        @Override
        public boolean isConnected(int serviceId, int serviceType, int callType) {
            return true; //TODO:
        }


        @Override
        public boolean isOpened(int serviceId) {
            ImsServiceImpl impl = mImsServiceImplMap.get(Integer.valueOf(serviceId));
            if(impl != null){
                return impl.isOpened();
            }
            return false;
        }

        @Override
        public void setRegistrationListener(int serviceId, IImsRegistrationListener listener) {
            ImsServiceImpl service = mImsServiceImplMap.get(new Integer(serviceId));
            if (service == null) {
                Log.e (TAG, "Invalid ServiceId ");
                return;
            }
            service.setRegistrationListener(listener);
        }

        @Override
        public ImsCallProfile createCallProfile(int serviceId, int serviceType, int callType) {
            ImsServiceImpl service = mImsServiceImplMap.get(new Integer(serviceId));
            if (service == null) {
                Log.e (TAG, "Invalid ServiceId ");
                return null;
            }
            return service.createCallProfile(serviceType, callType);
        }

        @Override
        public IImsCallSession createCallSession(int serviceId, ImsCallProfile profile,
                IImsCallSessionListener listener) {
            ImsServiceImpl service = mImsServiceImplMap.get(new Integer(serviceId));
            if (service == null) {
                Log.e (TAG, "Invalid ServiceId ");
                return null;
            }
            return service.createCallSession(serviceId, profile, listener);
        }

        @Override
        public IImsCallSession getPendingCallSession(int serviceId, String callId){
            ImsServiceImpl service = mImsServiceImplMap.get(new Integer(serviceId));
            if (service == null || callId == null) {
                Log.e (TAG, "Invalid arguments " + service + " " + callId);
                return null;
            }
           return service.getPendingCallSession(callId);
        }

        /**
         * Ut interface for the supplementary service configuration.
         */
        @Override
        public IImsUt getUtInterface(int serviceId){
            ImsServiceImpl service = mImsServiceImplMap.get(new Integer(serviceId));
            if (service == null) {
                Log.e (TAG, "Invalid ServiceId " + serviceId);
                return null;
            }
            return service.getUtInterface();
        }

        /**
         * Config interface to get/set IMS service/capability parameters.
         */
        @Override
        public IImsConfig getConfigInterface(int phoneId) {
            ImsServiceImpl service = mImsServiceImplMap.get(new Integer(phoneId+1));
            if (service == null) {
                Log.e (TAG, "Invalid phoneId " + phoneId);
                return null;
            }
            return service.getConfigInterface();
        }

        /**
         * Used for turning on IMS when its in OFF state.
         */
        @Override
        public void turnOnIms(int phoneId){
            ImsServiceImpl service = mImsServiceImplMap.get(new Integer(phoneId+1));
            if (service == null) {
                Log.e (TAG, "Invalid phoneId " + phoneId);
            }
            service.turnOnIms();
        }

        /**
         * Used for turning off IMS when its in ON state.
         * When IMS is OFF, device will behave as CSFB'ed.
         */
        @Override
        public void turnOffIms(int phoneId) {
            ImsServiceImpl service = mImsServiceImplMap.get(new Integer(phoneId+1));
            if (service == null) {
                Log.e (TAG, "Invalid phoneId " + phoneId);
            }
            service.turnOffIms();
        }

        /**
         * ECBM interface for Emergency callback notifications
         */
        @Override
        public IImsEcbm getEcbmInterface(int serviceId) {
            ImsServiceImpl service = mImsServiceImplMap.get(new Integer(serviceId));
            if (service == null) {
                Log.e(TAG, "getEcbmInterface: Invalid argument " + service);
                return null;
            }
            return service.getEcbmInterface();
        }

        /**
         * Used to set current TTY Mode.
         */
        @Override
        public void setUiTTYMode(int serviceId, int uiTtyMode, Message onComplete) {
            ImsServiceImpl service = mImsServiceImplMap.get(new Integer(serviceId));
            if (service == null) {
                Log.e (TAG, "Invalid arguments " + serviceId);
                return;
            }
            service.setUiTTYMode(serviceId, uiTtyMode, onComplete);
        }
    };

    private final IImsUtEx.Stub mImsUtExBinder = new IImsUtEx.Stub() {
        /**
         * Retrieves the configuration of the call forward.
         */
        public int setCallForwardingOption(int serviceId, int commandInterfaceCFAction,
                int commandInterfaceCFReason,int serviceClass, String dialingNumber,
                int timerSeconds, String ruleSet){
            ImsServiceImpl service = mImsServiceImplMap.get(new Integer(serviceId));
            if (service == null) {
                Log.e (TAG, "Invalid serviceId " + serviceId);
                return -1;
            }
            ImsUtImpl ut = service.getUtImpl();
            return ut.setCallForwardingOption(commandInterfaceCFAction, commandInterfaceCFReason,
                    serviceClass, dialingNumber, timerSeconds, ruleSet);
        }

        /**
         * Updates the configuration of the call forward.
         */
        public int getCallForwardingOption(int serviceId, int commandInterfaceCFReason, int serviceClass,
                String ruleSet){
            ImsServiceImpl service = mImsServiceImplMap.get(new Integer(serviceId));
            if (service == null) {
                Log.e (TAG, "Invalid serviceId " + serviceId);
                return -1;
            }
            ImsUtImpl ut = service.getUtImpl();
            return ut.getCallForwardingOption(commandInterfaceCFReason, serviceClass, ruleSet);
        }
    };

    public IImsConfig getConfigInterface(int serviceId) {
        ImsServiceImpl service = mImsServiceImplMap.get(new Integer(serviceId));
        if (service == null) {
            Log.e (TAG, "getConfigInterface->Invalid serviceId " + serviceId);
            return null;
        }
        return service.getConfigInterface();
    }

    private void iLog(String log){
        Log.i(TAG, log);
    }
}

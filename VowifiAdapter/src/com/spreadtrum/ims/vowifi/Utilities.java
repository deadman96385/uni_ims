package com.spreadtrum.ims.vowifi;

import android.content.Context;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.database.CursorWrapper;
import android.os.SystemProperties;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Log;

import com.android.ims.ImsCallProfile;
import com.spreadtrum.ims.ImsConfigImpl;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map.Entry;

public class Utilities {
    private static final String TAG = getTag(Utilities.class.getSimpleName());

    // This value will be false if release.
    public static final boolean DEBUG = true;

    // Used to get the primary card id.
    private static final int DEFAULT_PHONE_ID   = 0;
    private static final int SLOTTWO_PHONE_ID   = 1;
    private static final String PROP_MODEM_WORKMODE = "persist.radio.modem.workmode";

    public static HashMap<Integer, VideoQuality> sVideoQualitys =
            new HashMap<Integer, VideoQuality>();
    static {
        // Refer to ImsConfigImpl#VT_RESOLUTION_720P}
        sVideoQualitys.put(ImsConfigImpl.VT_RESOLUTION_720P, new VideoQuality(
                31, 1280, 720, 30, 3000 * 1000, 4000 * 1000, 200 * 1000, 30, 1));
        // Refer to ImsConfigImpl#VT_RESOLUTION_VGA_REVERSED_15
        sVideoQualitys.put(ImsConfigImpl.VT_RESOLUTION_VGA_REVERSED_15, new VideoQuality(
                22, 480, 640, 15, 400 * 1000, 660 * 1000, 150 * 1000, 15, 1));
        // Refer to ImsConfigImpl#VT_RESOLUTION_VGA_REVERSED_30
        sVideoQualitys.put(ImsConfigImpl.VT_RESOLUTION_VGA_REVERSED_30, new VideoQuality(
                30, 480, 640, 30, 600 * 1000, 980 * 1000, 150 * 1000, 30, 1));
        // Refer to ImsConfigImpl#VT_RESOLUTION_QVGA_REVERSED_15
        sVideoQualitys.put(ImsConfigImpl.VT_RESOLUTION_QVGA_REVERSED_15, new VideoQuality(
                12, 240, 320, 15, 256 * 1000, 320 * 1000, 100 * 1000, 15, 1));
        // Refer to ImsConfigImpl#VT_RESOLUTION_QVGA_REVERSED_30
        sVideoQualitys.put(ImsConfigImpl.VT_RESOLUTION_QVGA_REVERSED_30, new VideoQuality(
                13, 240, 320, 30, 384 * 1000, 512 * 1000, 100 * 1000, 30, 1));
        // Refer to ImsConfigImpl#VT_RESOLUTION_CIF
        sVideoQualitys.put(ImsConfigImpl.VT_RESOLUTION_CIF, new VideoQuality(
                14, 352, 288, 30, 300 * 1000, 400 * 1000, 100 * 1000, 30, 1));
        // Refer to ImsConfigImpl#VT_RESOLUTION_QCIF
        sVideoQualitys.put(ImsConfigImpl.VT_RESOLUTION_QCIF, new VideoQuality(
                11, 176, 144, 30, 100 * 1000, 300 * 1000, 60 * 1000, 30, 1));
    }

    public static String getTag(String tag) {
        return "[Adapter]" + tag;
    }

    public static String getString(String[] items) {
        if (items == null || items.length < 1) {
            return "null";
        }

        StringBuilder builder = new StringBuilder("[");
        builder.append(items[0]);
        for (int i = 1; i < items.length; i++) {
            builder.append(",");
            builder.append(items[i]);
        }
        builder.append("]");
        return builder.toString();
    }

    public static int getPrimaryCard(Context context) {
        TelephonyManager tm =
                (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
        int phoneCount = tm.getPhoneCount();
        if (phoneCount == 1) {
            return DEFAULT_PHONE_ID;
        }

        String prop = SystemProperties.get(PROP_MODEM_WORKMODE);
        if (!TextUtils.isEmpty(prop)) {
            String values[] = prop.split(",");
            int[] workMode = new int[phoneCount];
            for (int i = 0; i < phoneCount; i++) {
                workMode[i] = Integer.parseInt(values[i]);
            }

            if (workMode[DEFAULT_PHONE_ID] == 10
                    && workMode[SLOTTWO_PHONE_ID] != 10
                    && workMode[SLOTTWO_PHONE_ID] != 254) {
                return SLOTTWO_PHONE_ID;
            } else if (workMode[DEFAULT_PHONE_ID] == 254
                    && workMode[SLOTTWO_PHONE_ID] != 254) {
                return SLOTTWO_PHONE_ID;
            }
        }

        return DEFAULT_PHONE_ID;
    }

    public static int getPrimaryCardSubId(Context context) {
        int phoneId = Utilities.getPrimaryCard(context);
        int[] subId = SubscriptionManager.getSubId(phoneId);
        if (subId == null || subId.length == 0) {
            Log.e(TAG, "Can not get the sub id from the phone id: " + phoneId);
            return -1;
        }

        return subId[0];
    }

    public static boolean isVideoCall(int callType) {
        return callType != ImsCallProfile.CALL_TYPE_VOICE;
    }

    public static VideoQuality getDefaultVideoQuality(SharedPreferences preference) {
        int resolution = ImsConfigImpl.VT_RESOLUTION_VGA_REVERSED_30;
        if (preference != null) {
            resolution = preference.getInt(ImsConfigImpl.VT_RESOLUTION_VALUE,
                    ImsConfigImpl.VT_RESOLUTION_VGA_REVERSED_30);
            // As do not accept none reversed resolution, need adjust to reversed resolution.
            switch (resolution) {
                case ImsConfigImpl.VT_RESOLUTION_VGA_15:
                    resolution = ImsConfigImpl.VT_RESOLUTION_VGA_REVERSED_15;
                    break;
                case ImsConfigImpl.VT_RESOLUTION_VGA_30:
                    resolution = ImsConfigImpl.VT_RESOLUTION_VGA_REVERSED_30;
                    break;
                case ImsConfigImpl.VT_RESOLUTION_QVGA_15:
                    resolution = ImsConfigImpl.VT_RESOLUTION_QVGA_REVERSED_15;
                    break;
                case ImsConfigImpl.VT_RESOLUTION_QVGA_30:
                    resolution = ImsConfigImpl.VT_RESOLUTION_QVGA_REVERSED_30;
                    break;
            }
        }

        return sVideoQualitys.get((Integer) resolution);
    }

    public static VideoQuality findVideoQuality(float videoLevel) {
        Iterator<Entry<Integer, VideoQuality>> it = sVideoQualitys.entrySet().iterator();
        while (it.hasNext()) {
            VideoQuality quality = it.next().getValue();
            if (quality._level == videoLevel) {
                return quality;
            }
        }

        return null;
    }

    public static boolean isSameCallee(String calleeNumber, String phoneNumber) {
        if (TextUtils.isEmpty(calleeNumber)
                || TextUtils.isEmpty(phoneNumber)) {
            return false;
        }

        if (phoneNumber.indexOf(calleeNumber) >= 0
                || calleeNumber.indexOf(phoneNumber) >= 0) {
            return true;
        } else if (calleeNumber.startsWith("0")) {
            // Sometimes, the phone number will be start will 0, we'd like to sub the string.
            String tempCallee = calleeNumber.substring(1);
            if (phoneNumber.indexOf(tempCallee) >= 0
                    || tempCallee.indexOf(phoneNumber) >= 0) {
                return true;
            }
        }

        return false;
    }

    // This defined is match the error used by CM. Please do not change.
    public static class UnsolicitedCode {
        public static final int SECURITY_DPD_DISCONNECTED = 1;
        public static final int SIP_TIMEOUT               = 2;
        public static final int SIP_LOGOUT                = 3;
        public static final int SECURITY_REKEY_FAILED     = 4;
        public static final int SECURITY_STOP             = 5;
    }

    public static class CallType {
        public static final int CALL_TYPE_UNKNOWN = 0;
        public static final int CALL_TYPE_VOICE   = 1;
        public static final int CALL_TYPE_VIDEO   = 2;
    }

    public static class VolteNetworkType {
        public static final int IEEE_802_11 = -1;
        public static final int E_UTRAN_FDD = 4; // 3gpp
        public static final int E_UTRAN_TDD = 5; // 3gpp
    }

    public static class VowifiNetworkType {
        public static final int IEEE_802_11 = 1;
        public static final int E_UTRAN_FDD = 9;  // 3gpp
        public static final int E_UTRAN_TDD = 10; // 3gpp
    }

    public static class NativeErrorCode {
        public static final int IKE_INTERRUPT_STOP   = 0xD200 + 198;
        public static final int IKE_HANDOVER_STOP    = 0xD200 + 199;
        public static final int DPD_DISCONNECT       = 0xD200 + 15;
        public static final int IPSEC_REKEY_FAIL     = 0xD200 + 10;
        public static final int IKE_REKEY_FAIL       = 0xD200 + 11;
        public static final int REG_TIMEOUT          = 0xE100 + 5;
        public static final int SERVER_TIMEOUT       = 0xE100 + 6;
        public static final int REG_SERVER_FORBIDDEN = 0xE100 + 8;
        public static final int REG_EXPIRED_TIMEOUT  = 0xE100 + 17;
        public static final int REG_EXPIRED_OTHER    = 0xE100 + 18;
    }

    public static class RegisterState {
        public static final int STATE_IDLE        = 0;
        public static final int STATE_PROGRESSING = 1;
        public static final int STATE_CONNECTED   = 2;
    }

    public static class Result {
        public static final int INVALID_ID = -1;

        public static final int FAIL       = 0;
        public static final int SUCCESS    = 1;
    }

    public static class S2bType {
        public static final int NORMAL = 1;
        public static final int SOS    = 2;
        public static final int XCAP   = 4;
        public static final int MMS    = 8;
    }

    public static class RegisterType {
        public static final int NORMAL = 1;
        public static final int SOS    = 2;
    }

    public static class CallStateForDataRouter {
        public static final int VOLTE  = 0;
        public static final int VOWIFI = 1;
        public static final int NONE   = 2;
    }

    public static class IPVersion {
        public static final int NONE = -1;
        public static final int IP_V4 = 0;
        public static final int IP_V6 = 1;
    }

    // Note: Do not change this defined value, as it matched the call state used in CP.
    public static class SRVCCSyncInfo {

        public static class CallState {
            public static final int IDLE_STATE           = 0;
            public static final int DIALING_STATE        = 1;
            public static final int OUTGOING_STATE       = 2;
            public static final int ACTIVE_STATE         = 3;
            public static final int INCOMING_STATE       = 4;
            public static final int ACCEPT_STATE         = 5;
            public static final int MODIFY_PENDING_STATE = 6;
            public static final int RELEASE_STATE        = 7;
            public static final int CCBS_RECALL_STATE    = 8;
            public static final int MT_CSFB_STATE        = 9;
            public static final int MAX_STATE            = 10;
        }

        public static class HoldState {
            public static final int IDLE = 0;
            public static final int HELD = 2;
        }

        public static class MultipartyState {
            public static final int NO  = 0;
            public static final int YES = 2;
        }

        public static class CallDirection {
            public static final int MO = 0;
            public static final int MT = 1;
        }

        public static class CallType {
            public static final int NORMAL    = 0;
            public static final int EMERGENCY = 1;
            public static final int VIDEO     = 2;
        }

        public static class PhoneNumberType {
            public static final int INTERNATIONAL = 1;
            public static final int NATIONAL = 2;
            public static final int NETWORK = 3;
        }
    }

    public static class SRVCCResult {
        public static final int SUCCESS = 1;
        public static final int CANCEL  = 2;
        public static final int FAILURE  = 3;
    }

    public static class SecurityConfig {
        public String _pcscf4;
        public String _pcscf6;
        public String _dns4;
        public String _dns6;
        public String _ip4;
        public String _ip6;
        public boolean _prefIPv4;
        public boolean _isSupportMobike;

        public int _useIPVersion = IPVersion.NONE;

        public SecurityConfig(String pcscf4, String pcscf6, String dns4, String dns6, String ip4,
                String ip6, boolean prefIPv4, boolean isSupportMobike) {
            _pcscf4 = pcscf4;
            _pcscf6 = pcscf6;
            _dns4 = dns4;
            _dns6 = dns6;
            _ip4 = ip4;
            _ip6 = ip6;
            _prefIPv4 = prefIPv4;
            _isSupportMobike = isSupportMobike;
        }

        @Override
        public String toString() {
            return "[pcscf4=" + _pcscf4 + ", pcscf6=" + _pcscf6 + ", dns4=" + _dns4 + ", dns6="
                    + _dns6 + ", ip4=" + _ip4 + ", ip6=" + _ip6 + ", prefIPv4=" + _prefIPv4
                    + ", supportMobike=" + _isSupportMobike + "]";
        }
    }

    public static class RegisterConfig {
        private static final String JSON_PCSCF_SEP = ";";

        private String mLocalIPv4;
        private String mLocalIPv6;
        private String[] mPcscfIPv4;
        private String[] mPcscfIPv6;
        private String mDnsSerIPv4;
        private String mDnsSerIPv6;
        private int mIPv4Index = 0;
        private int mIPv6Index = 0;

        private String mUsedLocalIP;
        private String mUsedPcscfIP;
        private String mUsedDnsSerlIP;

        public static RegisterConfig getInstance(String localIPv4, String localIPv6,
                String pcscfIPv4, String pcscfIPv6, String usedPcscfAddr, String dns4,
                String dns6) {
            if (Utilities.DEBUG) {
                Log.i(TAG, "Get the s2b ip address from localIPv4: " + localIPv4 + ", localIPv6: "
                        + localIPv6 + ", pcscfIPv4: " + pcscfIPv4 + ", pcscfIPv6: "
                        + pcscfIPv6 + ", usedPcscfAddr: " + usedPcscfAddr + ", dns4: " + dns4
                        + ", dns6: " + dns6);
            }

            if ((TextUtils.isEmpty(localIPv4) || TextUtils.isEmpty(pcscfIPv4))
                    && (TextUtils.isEmpty(localIPv6) || TextUtils.isEmpty(pcscfIPv6))
                    && TextUtils.isEmpty(usedPcscfAddr)) {
                Log.e(TAG,
                        "Can not get the ip address: localIPv4: " + localIPv4 + ", localIPv6: "
                                + localIPv6 + ", pcscfIPv4: " + pcscfIPv4 + ", pcscfIPv6: "
                                + pcscfIPv6 + ", usedPcscfAddr: " + usedPcscfAddr);
                return null;
            }

            String[] pcscfIPv4s =
                    TextUtils.isEmpty(pcscfIPv4) ? null : pcscfIPv4.split(JSON_PCSCF_SEP);
            String[] pcscfIPv6s =
                    TextUtils.isEmpty(pcscfIPv6) ? null : pcscfIPv6.split(JSON_PCSCF_SEP);
            if (TextUtils.isEmpty(usedPcscfAddr)) {
                return new RegisterConfig(
                        localIPv4, localIPv6, pcscfIPv4s, pcscfIPv6s, dns4, dns6);
            } else {
                String[] newPcscfIPv4s = rebuildAddr(pcscfIPv4s, usedPcscfAddr, true);
                String[] newPcscfIPv6s = rebuildAddr(pcscfIPv6s, usedPcscfAddr, false);
                return new RegisterConfig(
                        localIPv4, localIPv6, newPcscfIPv4s, newPcscfIPv6s, dns4, dns6);
            }
        }

        private RegisterConfig(String localIPv4, String localIPv6, String[] pcscfIPv4,
                String[] pcscfIPv6, String dns4, String dns6) {
            mLocalIPv4 = localIPv4;
            mLocalIPv6 = localIPv6;
            mPcscfIPv4 = pcscfIPv4;
            mPcscfIPv6 = pcscfIPv6;
            mDnsSerIPv4 = dns4;
            mDnsSerIPv6 = dns6;
        }

        public boolean isCurUsedIPv4() {
            return isIPv4(mUsedLocalIP);
        }

        public String getCurUsedLocalIP() {
            return mUsedLocalIP;
        }

        public String getCurUsedPcscfIP() {
            return mUsedPcscfIP;
        }

        public String getCurUsedDnsSerIP() {
            return mUsedDnsSerlIP;
        }

        public String getLocalIP(boolean isIPv4) {
            mUsedLocalIP = isIPv4 ? getLocalIPv4() : getLocalIPv6();
            return mUsedLocalIP;
        }

        public String getDnsSerIP(boolean isIPv4) {
            mUsedDnsSerlIP = isIPv4 ? getDnsSerIPv4() : getDnsSerIPv6();
            return mUsedDnsSerlIP;
        }

        public String getPcscfIP(boolean isIPv4) {
            mUsedPcscfIP = isIPv4 ? getPcscfIPv4() : getPcscfIPv6();
            return mUsedPcscfIP;
        }

        public int getValidIPVersion(boolean prefIPv4) {
            if (prefIPv4) {
                if (isIPv4Valid()) return IPVersion.IP_V4;
                if (isIPv6Valid()) return IPVersion.IP_V6;
            } else {
                if (isIPv6Valid()) return IPVersion.IP_V6;
                if (isIPv4Valid()) return IPVersion.IP_V4;
            }

            return IPVersion.NONE;
        }

        private String getLocalIPv4() {
            return mLocalIPv4;
        }

        private String getDnsSerIPv4() {
            return mDnsSerIPv4;
        }

        private String getLocalIPv6() {
            return mLocalIPv6;
        }

        private String getDnsSerIPv6() {
            return mDnsSerIPv6;
        }

        private String getPcscfIPv4() {
            String pcscfIPv4 = null;
            if (mIPv4Index < mPcscfIPv4.length) {
                pcscfIPv4 = mPcscfIPv4[mIPv4Index];
                mIPv4Index = mIPv4Index + 1;
            }
            return pcscfIPv4;
        }

        private String getPcscfIPv6() {
            String pcscfIPv6 = null;
            if (mIPv6Index < mPcscfIPv6.length) {
                pcscfIPv6 = mPcscfIPv6[mIPv6Index];
                mIPv6Index = mIPv6Index + 1;
            }
            return pcscfIPv6;
        }

        private boolean isIPv4Valid() {
            return !TextUtils.isEmpty(mLocalIPv4)
                    && mPcscfIPv4 != null
                    && mPcscfIPv4.length > 0
                    && mIPv4Index < mPcscfIPv4.length;
        }

        private boolean isIPv6Valid() {
            return !TextUtils.isEmpty(mLocalIPv6)
                    && mPcscfIPv6 != null
                    && mPcscfIPv6.length > 0
                    && mIPv6Index < mPcscfIPv6.length;
        }

        private static boolean isIPv4(String ipAddr) {
            return ipAddr.contains(".");
        }

        private static String[] rebuildAddr(String[] oldAddrs, String firstAddr, boolean asIPv4) {
            if (oldAddrs == null
                    || oldAddrs.length == 1
                    || TextUtils.isEmpty(firstAddr)
                    || isIPv4(firstAddr) != asIPv4) {
                return oldAddrs;
            }

            for (int i = 0; i < oldAddrs.length; i++) {
                if (firstAddr.equals(oldAddrs[i])) {
                    String oldFirstAddr = oldAddrs[0];
                    oldAddrs[0] = firstAddr;
                    oldAddrs[i] = oldFirstAddr;
                    break;
                }
            }
            return oldAddrs;
        }

        @Override
        public String toString() {
            StringBuilder builder = new StringBuilder();
            builder.append("localIPv4[" + mLocalIPv4 + "]");
            builder.append(", localIPv6[" + mLocalIPv6 + "]");
            if (mPcscfIPv4 != null) {
                for (int i = 0; i < mPcscfIPv4.length; i++) {
                    builder.append(", pcscfIPv4[" + i + "] = " + mPcscfIPv4[i]);
                }
            }
            if (mPcscfIPv6 != null) {
                for (int i = 0; i < mPcscfIPv6.length; i++) {
                    builder.append(", pcscfIPv6[" + i + "] = " + mPcscfIPv6[i]);
                }
            }
            builder.append(", pcscfDnsSerIPv4[" + mDnsSerIPv4 + "]");
            builder.append(", pcscfDnsSerIPv6[" + mDnsSerIPv6 + "]");
            return builder.toString();
        }
    }

    public static class Camera {
        private static final String CAMERA_FRONT = "1";
        private static final String CAMERA_BACK = "0";

        private static final String CAMERA_NAME_NULL = "null";
        private static final String CAMERA_NAME_FRONT = "front";
        private static final String CAMERA_NAME_BACK = "back";

        public static boolean isFront(String cameraId) {
            return CAMERA_FRONT.equals(cameraId);
        }

        public static boolean isBack(String cameraId) {
            return CAMERA_BACK.equals(cameraId);
        }

        public static String toString(String cameraId) {
            String cameraName = CAMERA_NAME_NULL;
            if (!TextUtils.isEmpty(cameraId)) {
                cameraName = isFront(cameraId) ? CAMERA_NAME_FRONT : CAMERA_NAME_BACK;
            }
            return cameraName;
        }
    }

    /**
     * For emergency service please refer to 3GPP TS 22.101 clause 10.
     */
    public static class EMUtils {
        // The default based emergency service urn format
        public static final String DEFAULT_EMERGENCY_SERVICE_URN = "urn:service:sos";
        // Police
        public static final String EMERGENCY_SERVICE_CATEGORY_BIT1 = "police";
        // Ambulance
        public static final String EMERGENCY_SERVICE_CATEGORY_BIT2 = "ambulance";
        // Fire Brigade
        public static final String EMERGENCY_SERVICE_CATEGORY_BIT3 = "fire";
        // Marine Guard
        public static final String EMERGENCY_SERVICE_CATEGORY_BIT4 = "marine";
        // Mountain Rescue
        public static final String EMERGENCY_SERVICE_CATEGORY_BIT5 = "mountain";

        /**
         * byte to inverted bit
         */
        public static String byteToInvertedBit(byte b) {
            return "" + (byte) ((b >> 0) & 0x1) + (byte) ((b >> 1) & 0x1) + (byte) ((b >> 2) & 0x1)
                    + (byte) ((b >> 3) & 0x1) + (byte) ((b >> 4) & 0x1) + (byte) ((b >> 5) & 0x1)
                    + (byte) ((b >> 6) & 0x1) + (byte) ((b >> 7) & 0x1);
        }

        public static String getEmergencyCallUrn(String category) {
            if (DEBUG)
                Log.i(TAG, "Get the emergency call's urn, category: " + category);
            if (TextUtils.isEmpty(category)) {
                Log.w(TAG, "The category is empty, return the default urn.");
                return DEFAULT_EMERGENCY_SERVICE_URN;
            }

            String urnUri = DEFAULT_EMERGENCY_SERVICE_URN;
            try {
                int categoryValue = Integer.parseInt(category);
                if ((categoryValue > 0) && (categoryValue < 128)) {
                    byte categoryByte = (byte) categoryValue;
                    String categoryBitString = byteToInvertedBit(categoryByte);
                    if (categoryBitString.charAt(0) == '1') {
                        urnUri = urnUri.concat(".").concat(EMERGENCY_SERVICE_CATEGORY_BIT1);
                    }
                    if (categoryBitString.charAt(1) == '1') {
                        urnUri = urnUri.concat(".").concat(EMERGENCY_SERVICE_CATEGORY_BIT2);
                    }
                    if (categoryBitString.charAt(2) == '1') {
                        urnUri = urnUri.concat(".").concat(EMERGENCY_SERVICE_CATEGORY_BIT3);
                    }
                    if (categoryBitString.charAt(3) == '1') {
                        urnUri = urnUri.concat(".").concat(EMERGENCY_SERVICE_CATEGORY_BIT4);
                    }
                    if (categoryBitString.charAt(4) == '1') {
                        urnUri = urnUri.concat(".").concat(EMERGENCY_SERVICE_CATEGORY_BIT5);
                    }
                }
            } catch (NumberFormatException e) {
                Log.e(TAG, "Failed to get emergency urn as catch the ex: " + e.toString());
            }

            return urnUri;
        }

        public static String getEmergencyCallCategory(String urnUri) {
            if (Utilities.DEBUG)
                Log.i(TAG, "Get the emergency call's category, urn: " + urnUri);
            if (TextUtils.isEmpty(urnUri)) {
                Log.w(TAG, "The urn uri is empty, return null.");
                return null;
            }

            int category = 0;
            String urnLowerCase = urnUri.toLowerCase();
            if (urnLowerCase.startsWith(DEFAULT_EMERGENCY_SERVICE_URN)) {
                if (urnLowerCase.indexOf(EMERGENCY_SERVICE_CATEGORY_BIT1) > 0) {
                    category += 1;
                }
                if (urnLowerCase.indexOf(EMERGENCY_SERVICE_CATEGORY_BIT2) > 0) {
                    category += 2;
                }
                if (urnLowerCase.indexOf(EMERGENCY_SERVICE_CATEGORY_BIT3) > 0) {
                    category += 4;
                }
                if (urnLowerCase.indexOf(EMERGENCY_SERVICE_CATEGORY_BIT4) > 0) {
                    category += 8;
                }
                if (urnLowerCase.indexOf(EMERGENCY_SERVICE_CATEGORY_BIT5) > 0) {
                    category += 16;
                }
            }

            return String.valueOf(category);
        }
    }

    public static class ECBMRequest {
        public static final int ECBM_STEP_INVALID = 0;

        // For ECBM normal step.
        public static final int ECBM_STEP_DEREGISTER_NORMAL = 1;
        public static final int ECBM_STEP_DEATTACH_NORMAL = 2;
        public static final int ECBM_STEP_ATTACH_SOS = 3;
        public static final int ECBM_STEP_REGISTER_SOS = 4;
        public static final int ECBM_STEP_START_EMERGENCY_CALL = 5;
        public static final int ECBM_STEP_DEREGISTER_SOS = 6;
        public static final int ECBM_STEP_DEATTACH_SOS = 7;
        public static final int ECBM_STEP_ATTACH_NORMAL = 8;
        public static final int ECBM_STEP_REGISTER_NORMAL = 9;

        // For ECBM error step.
        public static final int ECBM_STEP_FORCE_RESET = 10;

        private ArrayList<Integer> mRequestSteps;
        private int mIndex;

        private ECBMRequest(int... steps) {
            mIndex = -1;
            mRequestSteps = new ArrayList<Integer>();
            for (int step : steps) {
                mRequestSteps.add(step);
            }
        }

        public static ECBMRequest get(boolean asNormalCall, boolean needRemoveOldS2b) {
            // TODO: Do not support needn't remove old s2b now.
            if (asNormalCall) {
                return new ECBMRequest(ECBM_STEP_INVALID);
            } else if (true /*needRemoveOldS2b*/) {
                return new ECBMRequest(
                        ECBM_STEP_DEREGISTER_NORMAL,
                        ECBM_STEP_DEATTACH_NORMAL,
                        ECBM_STEP_ATTACH_SOS,
                        ECBM_STEP_REGISTER_SOS,
                        ECBM_STEP_START_EMERGENCY_CALL,
                        ECBM_STEP_DEREGISTER_SOS,
                        ECBM_STEP_DEATTACH_SOS,
                        ECBM_STEP_ATTACH_NORMAL,
                        ECBM_STEP_REGISTER_NORMAL);
            } else {
                return new ECBMRequest(
                        ECBM_STEP_ATTACH_SOS,
                        ECBM_STEP_REGISTER_SOS,
                        ECBM_STEP_START_EMERGENCY_CALL,
                        ECBM_STEP_DEREGISTER_SOS,
                        ECBM_STEP_DEATTACH_SOS);
            }
        }

        public int getEnterECBMStep() {
            mIndex = 0;
            return mRequestSteps.get(mIndex);
        }

        public int getCurStep() {
            return mRequestSteps.get(mIndex);
        }

        public int getNextStep() {
            mIndex = mIndex + 1;
            if (mIndex >= mRequestSteps.size()) {
                return ECBM_STEP_INVALID;
            } else {
                return mRequestSteps.get(mIndex);
            }
        }

        public int getExitECBMStep() {
            if (mRequestSteps.size() == 1) {
                mIndex = 0;
            } else {
                mIndex = (mRequestSteps.size() / 2) + 1;
            }
            return mRequestSteps.get(mIndex);
        }
    }

    public static class PendingAction {
        public String _name;
        public int _action;
        public ArrayList<Object> _params;

        public PendingAction(String name, int action, Object... params) {
            _name = name;
            _action = action;
            _params = new ArrayList<Object>();
            if (params != null) {
                for (Object param : params) {
                    _params.add(param);
                }
            } else {
                Log.d(TAG, "The action '" + _name + "' do not contains the params.");
            }
        }
    }

    public static class CallBarringInfo {
        // Refer to ImsUtInterface#CDIV_CF_XXX
        public int mCondition;
        // 0: disabled, 1: enabled
        public int mStatus;

        public CallBarringInfo() {
        }

        public void setCondition(int conditon) {
            mCondition = conditon;
        }

        public void setStatus(int status) {
            mStatus = status;
        }
    }

    public static class VideoQuality {
        public int _level;
        public int _width;
        public int _height;
        public int _frameRate;
        public int _bitRate;
        public int _brHi;
        public int _brLo;
        public int _frHi;
        public int _frLo;

        public VideoQuality(int level, int width, int height, int frameRate, int bitRate, int brHi,
                int brLo, int frHi, int frLo) {
            _level = level;
            _width = width;
            _height = height;
            _frameRate = frameRate;
            _bitRate = bitRate;
            _brHi = brHi;
            _brLo = brLo;
            _frHi = frHi;
            _frLo = frLo;
        }

        @Override
        public String toString() {
            return "[level=" + _level + ", width=" + _width + ", height=" + _height
                    + ", frameRate=" + _frameRate + ", bitRate=" + _bitRate + ", BrHi=" + _brHi
                    + ", BrLo=" + _brLo + ", FrHi=" + _frHi + ", FrLo=" + _frLo + "]";
        }
    }

    public static class ProviderUtils {
        public static final String CONTENT_URI = "content://com.spreadtrum.vowifi.accountsettings";

        // Defined support get configuration items.
        public static final String FUN_SECURITY = "security";
        public static final String FUN_REGISTER = "register";
        public static final String FUN_CALL     = "call";
        public static final String FUN_MESSAGE  = "message";
        public static final String FUN_XCAP     = "xcap";
    }

    public static class CallCursor extends CursorWrapper {
        private Cursor mCursor;

        private static final String COL_WITH_LOCATION      = "withLocation";
        private static final String COL_SOS_AS_NORMAL_CALL = "sosAsNormal";
        private static final String COL_SOS_REMOVE_OLD_S2B = "sosRemoveOldS2b";

        private static int sIndexWithLocation = -1;
        private static int sIndexSosAsNormalCall = -1;
        private static int sIndexSosRemoveOldS2b = -1;

        public CallCursor(Cursor cursor) {
            super(cursor);
            mCursor = cursor;

            if (sIndexWithLocation < 0) {
                sIndexWithLocation = mCursor.getColumnIndexOrThrow(COL_WITH_LOCATION);
                sIndexSosAsNormalCall = mCursor.getColumnIndexOrThrow(COL_SOS_AS_NORMAL_CALL);
                sIndexSosRemoveOldS2b = mCursor.getColumnIndexOrThrow(COL_SOS_REMOVE_OLD_S2B);
            }
        }

        public boolean needWithLocation() {
            // Will be only one item.
            mCursor.moveToFirst();
            return mCursor.getInt(sIndexWithLocation) > 0;
        }

        public boolean sosAsNormalCall() {
            // Will be only one item.
            mCursor.moveToFirst();
            return mCursor.getInt(sIndexSosAsNormalCall) > 0;
        }

        public boolean sosNeedRemoveOldS2b() {
            // Will be only one item.
            mCursor.moveToFirst();
            return mCursor.getInt(sIndexSosRemoveOldS2b) > 0;
        }
    }

    public static class JSONUtils {
        // Callback constants
        public static final String KEY_EVENT_CODE = "event_code";
        public static final String KEY_EVENT_NAME = "event_name";

        // Security
        public static final String KEY_SESSION_ID = "session_id";

        public static final int STATE_CODE_SECURITY_INVALID_ID       = -1;
        public static final int STATE_CODE_SECURITY_AUTH_FAILED      = -2;
        public static final int STATE_CODE_SECURITY_LOCAL_IP_IS_NULL = -3;
        public static final int STATE_CODE_SECURITY_NO_REQUEST       = -4;
        public static final int STATE_CODE_SECURITY_STOP_TIMEOUT     = -5;

        public final static int SECURITY_EVENT_CODE_BASE = 0;
        public final static int EVENT_CODE_ATTACH_SUCCESSED = SECURITY_EVENT_CODE_BASE + 1;
        public final static int EVENT_CODE_ATTACH_FAILED = SECURITY_EVENT_CODE_BASE + 2;
        public final static int EVENT_CODE_ATTACH_PROGRESSING = SECURITY_EVENT_CODE_BASE + 3;
        public final static int EVENT_CODE_ATTACH_STOPPED = SECURITY_EVENT_CODE_BASE + 4;

        public final static String EVENT_ATTACH_SUCCESSED = "attach_successed";
        public final static String EVENT_ATTACH_FAILED = "attach_failed";
        public final static String EVENT_ATTACH_PROGRESSING = "attach_progressing";
        public final static String EVENT_ATTACH_STOPPED = "attach_stopped";

        // Keys for security callback
        public final static String KEY_PROGRESS_STATE = "progress_state";
        public final static String KEY_LOCAL_IP4 = "local_ip4";
        public final static String KEY_LOCAL_IP6 = "local_ip6";
        public final static String KEY_PCSCF_IP4 = "pcscf_ip4";
        public final static String KEY_PCSCF_IP6 = "pcscf_ip6";
        public final static String KEY_DNS_IP4 = "dns_ip4";
        public final static String KEY_DNS_IP6 = "dns_ip6";
        public final static String KEY_PREF_IP4 = "pref_ip4";
        public final static String KEY_TYPE = "type";
        public final static String KEY_SUPPORT_MOBIKE = "support_mobike";

        public final static int USE_IP4 = 0;
        public final static int USE_IP6 = 1;

        // Register
        public static final String KEY_STATE_CODE = "state_code";
        public static final String KEY_RETRY_AFTER = "retry_after";

        public static final int STATE_CODE_REG_PING_FAILED = 1;
        public static final int STATE_CODE_REG_NATIVE_FAILED = 2;
        public static final int STATE_CODE_REG_AUTH_FAILED = 3;

        public static final int REGISTER_EVENT_CODE_BASE = 50;
        public static final int EVENT_CODE_LOGIN_OK = REGISTER_EVENT_CODE_BASE + 1;
        public static final int EVENT_CODE_LOGIN_FAILED = REGISTER_EVENT_CODE_BASE + 2;
        public static final int EVENT_CODE_LOGOUTED = REGISTER_EVENT_CODE_BASE + 3;
        public static final int EVENT_CODE_REREGISTER_OK = REGISTER_EVENT_CODE_BASE + 4;
        public static final int EVENT_CODE_REREGISTER_FAILED = REGISTER_EVENT_CODE_BASE + 5;
        public static final int EVENT_CODE_REGISTER_STATE_UPDATE = REGISTER_EVENT_CODE_BASE + 6;

        public static final String EVENT_LOGIN_OK = "login_ok";
        public static final String EVENT_LOGIN_FAILED = "login_failed";
        public static final String EVENT_LOGOUTED = "logouted";
        public static final String EVENT_REREGISTER_OK = "refresh_ok";
        public static final String EVENT_REREGISTER_FAILED = "refresh_failed";
        public static final String EVENT_REGISTER_STATE_UPDATE = "state_update";

        // Call & Conference
        public static final String KEY_ID = "id";
        public static final String KEY_ALERT_TYPE = "alert_type";
        public static final String KEY_IS_VIDEO = "is_video";
        public static final String KEY_PHONE_NUM = "phone_num";
        public static final String KEY_SIP_URI = "sip_uri";
        public static final String KEY_VIDEO_HEIGHT = "video_height";
        public static final String KEY_VIDEO_WIDTH = "video_width";
        public static final String KEY_VIDEO_LEVEL = "video_level";
        public static final String KEY_RTP_RECEIVED = "rtp_received";
        public static final String KEY_RTCP_LOSE = "rtcp_lose";
        public static final String KEY_RTCP_JITTER = "rtcp_jitter";
        public static final String KEY_RTCP_RTT = "rtcp_rtt";
        public static final String KEY_CONF_PART_NEW_STATUS = "conf_part_new_status";
        public static final String KEY_ECALL_IND_URN_URI = "emergency_call_ind_urn_uri";
        public static final String KEY_ECALL_IND_REASON = "emergency_call_ind_reason";
        public static final String KEY_ECALL_IND_ACTION_TYPE = "emergency_call_ind_action_type";
        public static final String KEY_USSD_INFO_RECEIVED = "ussd_info_received";
        public static final String KEY_USSD_MODE = "ussd_mode";

        // Call
        public static final int CALL_EVENT_CODE_BASE = 100;
        public static final int EVENT_CODE_CALL_INCOMING = CALL_EVENT_CODE_BASE + 1;
        public static final int EVENT_CODE_CALL_OUTGOING = CALL_EVENT_CODE_BASE + 2;
        public static final int EVENT_CODE_CALL_ALERTED = CALL_EVENT_CODE_BASE + 3;
        public static final int EVENT_CODE_CALL_TALKING = CALL_EVENT_CODE_BASE + 4;
        public static final int EVENT_CODE_CALL_TERMINATE = CALL_EVENT_CODE_BASE + 5;
        public static final int EVENT_CODE_CALL_HOLD_OK = CALL_EVENT_CODE_BASE + 6;
        public static final int EVENT_CODE_CALL_HOLD_FAILED = CALL_EVENT_CODE_BASE + 7;
        public static final int EVENT_CODE_CALL_RESUME_OK = CALL_EVENT_CODE_BASE + 8;
        public static final int EVENT_CODE_CALL_RESUME_FAILED = CALL_EVENT_CODE_BASE + 9;
        public static final int EVENT_CODE_CALL_HOLD_RECEIVED = CALL_EVENT_CODE_BASE + 10;
        public static final int EVENT_CODE_CALL_RESUME_RECEIVED = CALL_EVENT_CODE_BASE + 11;
        public static final int EVENT_CODE_CALL_ADD_VIDEO_OK = CALL_EVENT_CODE_BASE + 12;
        public static final int EVENT_CODE_CALL_ADD_VIDEO_FAILED = CALL_EVENT_CODE_BASE + 13;
        public static final int EVENT_CODE_CALL_REMOVE_VIDEO_OK = CALL_EVENT_CODE_BASE + 14;
        public static final int EVENT_CODE_CALL_REMOVE_VIDEO_FAILED = CALL_EVENT_CODE_BASE + 15;
        public static final int EVENT_CODE_CALL_ADD_VIDEO_REQUEST = CALL_EVENT_CODE_BASE + 16;
        public static final int EVENT_CODE_CALL_ADD_VIDEO_CANCEL = CALL_EVENT_CODE_BASE + 17;
        public static final int EVENT_CODE_CALL_RTP_RECEIVED = CALL_EVENT_CODE_BASE + 18;
        public static final int EVENT_CODE_CALL_RTCP_CHANGED = CALL_EVENT_CODE_BASE + 19;
        public static final int EVENT_CODE_CALL_IS_FOCUS = CALL_EVENT_CODE_BASE + 20;
        public static final int EVENT_CODE_CALL_IS_EMERGENCY = CALL_EVENT_CODE_BASE + 21;
        public static final int EVENT_CODE_USSD_INFO_RECEIVED = CALL_EVENT_CODE_BASE + 22;

        public static final String EVENT_CALL_INCOMING = "call_incoming";
        public static final String EVENT_CALL_OUTGOING = "call_outgoing";
        public static final String EVENT_CALL_ALERTED = "call_alerted";
        public static final String EVENT_CALL_TALKING = "call_talking";
        public static final String EVENT_CALL_TERMINATE = "call_terminate";
        public static final String EVENT_CALL_HOLD_OK = "call_hold_ok";
        public static final String EVENT_CALL_HOLD_FAILED = "call_hold_failed";
        public static final String EVENT_CALL_RESUME_OK = "call_resume_ok";
        public static final String EVENT_CALL_RESUME_FAILED = "call_resume_failed";
        public static final String EVENT_CALL_HOLD_RECEIVED = "call_hold_received";
        public static final String EVENT_CALL_RESUME_RECEIVED = "call_resume_received";
        public static final String EVENT_CALL_ADD_VIDEO_OK = "call_add_video_ok";
        public static final String EVENT_CALL_ADD_VIDEO_FAILED = "call_add_video_failed";
        public static final String EVENT_CALL_REMOVE_VIDEO_OK = "call_remove_video_ok";
        public static final String EVENT_CALL_REMOVE_VIDEO_FAILED = "call_remove_video_failed";
        public static final String EVENT_CALL_ADD_VIDEO_REQUEST = "call_add_video_request";
        public static final String EVENT_CALL_ADD_VIDEO_CANCEL = "call_add_video_cancel";
        public static final String EVENT_CALL_RTP_RECEIVED = "call_rtp_received";
        public static final String EVENT_CALL_RTCP_CHANGED = "call_rtcp_changed";
        public static final String EVENT_CALL_IS_FOCUS = "call_is_focus";
        public static final String EVENT_CALL_IS_EMERGENCY = "call_is_emergency";
        public static final String EVENT_USSD_INFO_RECEIVED = "ussd_info_received";

        // Conference
        public static final int CONF_EVENT_CODE_BASE = 200;
        public static final int EVENT_CODE_CONF_OUTGOING = CONF_EVENT_CODE_BASE + 1;
        public static final int EVENT_CODE_CONF_ALERTED = CONF_EVENT_CODE_BASE + 2;
        public static final int EVENT_CODE_CONF_CONNECTED = CONF_EVENT_CODE_BASE + 3;
        public static final int EVENT_CODE_CONF_DISCONNECTED = CONF_EVENT_CODE_BASE + 4;
        public static final int EVENT_CODE_CONF_INVITE_ACCEPT = CONF_EVENT_CODE_BASE + 5;
        public static final int EVENT_CODE_CONF_INVITE_FAILED = CONF_EVENT_CODE_BASE + 6;
        public static final int EVENT_CODE_CONF_KICK_ACCEPT = CONF_EVENT_CODE_BASE + 7;
        public static final int EVENT_CODE_CONF_KICK_FAILED = CONF_EVENT_CODE_BASE + 8;
        public static final int EVENT_CODE_CONF_PART_UPDATE = CONF_EVENT_CODE_BASE + 9;
        public static final int EVENT_CODE_CONF_HOLD_OK = CONF_EVENT_CODE_BASE + 10;
        public static final int EVENT_CODE_CONF_HOLD_FAILED = CONF_EVENT_CODE_BASE + 11;
        public static final int EVENT_CODE_CONF_RESUME_OK = CONF_EVENT_CODE_BASE + 12;
        public static final int EVENT_CODE_CONF_RESUME_FAILED = CONF_EVENT_CODE_BASE + 13;
        public static final int EVENT_CODE_CONF_HOLD_RECEIVED = CONF_EVENT_CODE_BASE + 14;
        public static final int EVENT_CODE_CONF_RESUME_RECEIVED = CONF_EVENT_CODE_BASE + 15;
        public static final int EVENT_CODE_CONF_RTP_RECEIVED = CONF_EVENT_CODE_BASE + 16;
        public static final int EVENT_CODE_CONF_RTCP_CHANGED = CONF_EVENT_CODE_BASE + 17;

        public static final String EVENT_CONF_OUTGOING = "conf_outgoing";
        public static final String EVENT_CONF_ALERTED = "conf_alerted";
        public static final String EVENT_CONF_CONNECTED = "conf_connected";
        public static final String EVENT_CONF_DISCONNECTED = "conf_disconnected";
        public static final String EVENT_CONF_INVITE_ACCEPT = "conf_invite_accept";
        public static final String EVENT_CONF_INVITE_FAILED = "conf_invite_failed";
        public static final String EVENT_CONF_KICK_ACCEPT = "conf_kick_accept";
        public static final String EVENT_CONF_KICK_FAILED = "conf_kick_failed";
        public static final String EVENT_CONF_PART_UPDATE = "conf_part_update";
        public static final String EVENT_CONF_HOLD_OK = "conf_hold_ok";
        public static final String EVENT_CONF_HOLD_FAILED = "conf_hold_failed";
        public static final String EVENT_CONF_RESUME_OK = "conf_resume_ok";
        public static final String EVENT_CONF_RESUME_FAILED = "conf_resume_failed";
        public static final String EVENT_CONF_HOLD_RECEIVED = "conf_hold_received";
        public static final String EVENT_CONF_RESUME_RECEIVED = "conf_resume_received";
        public static final String EVENT_CONF_RTP_RECEIVED = "conf_rtp_received";
        public static final String EVENT_CONF_RTCP_CHANGED = "conf_rtcp_changed";

        // Video resize
        public static final int VIDEO_EVENT_CODE_BASE = 300;
        public static final int EVENT_CODE_LOCAL_VIDEO_RESIZE = VIDEO_EVENT_CODE_BASE + 1;
        public static final int EVENT_CODE_REMOTE_VIDEO_RESIZE = VIDEO_EVENT_CODE_BASE + 2;
        public static final int EVENT_CODE_LOCAL_VIDEO_LEVEL_UPDATE = VIDEO_EVENT_CODE_BASE + 3;

        public static final String EVENT_LOCAL_VIDEO_RESIZE = "local_video_resize";
        public static final String EVENT_REMOTE_VIDEO_RESIZE = "remote_video_resize";
        public static final String EVENT_LOCAL_VIDEO_LEVEL_UPDATE = "local_video_level_update";

        // UT
        public static final String KEY_UT_CF_TIME_SECONDS = "ut_cf_time_seconds";
        public static final String KEY_UT_CF_RULES = "ut_cf_rules";
        public static final String KEY_UT_CF_RULE_ENABLED = "ut_cf_rule_enabled";
        public static final String KEY_UT_CF_RULE_MEDIA = "ut_cf_rule_media";
        public static final String KEY_UT_CF_CONDS = "ut_cf_conditions";
        public static final String KEY_UT_CF_ACTION_TARGET = "ut_cf_action_target";
        public static final String KEY_UT_CW_ENABLED = "ut_cw_enabled";
        public static final String KEY_UT_CB_RULES = "ut_cb_rules";
        public static final String KEY_UT_CB_RULE_ENABLED = "ut_cb_rule_enabled";
        public static final String KEY_UT_CB_CONDS = "ut_cb_conditions";

        public static final int UT_EVENT_CODE_BASE = 350;
        public static final int EVENT_CODE_UT_QUERY_CB_OK = UT_EVENT_CODE_BASE + 1;
        public static final int EVENT_CODE_UT_QUERY_CB_FAILED = UT_EVENT_CODE_BASE + 2;
        public static final int EVENT_CODE_UT_QUERY_CF_OK = UT_EVENT_CODE_BASE + 3;
        public static final int EVENT_CODE_UT_QUERY_CF_FAILED = UT_EVENT_CODE_BASE + 4;
        public static final int EVENT_CODE_UT_QUERY_CW_OK = UT_EVENT_CODE_BASE + 5;
        public static final int EVENT_CODE_UT_QUERY_CW_FAILED = UT_EVENT_CODE_BASE + 6;
        public static final int EVENT_CODE_UT_UPDATE_CB_OK = UT_EVENT_CODE_BASE + 7;
        public static final int EVENT_CODE_UT_UPDATE_CB_FAILED = UT_EVENT_CODE_BASE + 8;
        public static final int EVENT_CODE_UT_UPDATE_CF_OK = UT_EVENT_CODE_BASE + 9;
        public static final int EVENT_CODE_UT_UPDATE_CF_FAILED = UT_EVENT_CODE_BASE + 10;
        public static final int EVENT_CODE_UT_UPDATE_CW_OK = UT_EVENT_CODE_BASE + 11;
        public static final int EVENT_CODE_UT_UPDATE_CW_FAILED = UT_EVENT_CODE_BASE + 12;

        public static final String EVENT_UT_QUERY_CB_OK = "ut_query_call_barring_ok";
        public static final String EVENT_UT_QUERY_CB_FAILED = "ut_query_call_barring_failed";
        public static final String EVENT_UT_QUERY_CF_OK = "ut_query_call_forward_ok";
        public static final String EVENT_UT_QUERY_CF_FAILED = "ut_query_call_forward_failed";
        public static final String EVENT_UT_QUERY_CW_OK = "ut_query_call_waiting_ok";
        public static final String EVENT_UT_QUERY_CW_FAILED = "ut_query_call_waiting_failed";
        public static final String EVENT_UT_UPDATE_CB_OK = "ut_update_call_barring_ok";
        public static final String EVENT_UT_UPDATE_CB_FAILED = "ut_update_call_barring_failed";
        public static final String EVENT_UT_UPDATE_CF_OK = "ut_update_call_forward_ok";
        public static final String EVENT_UT_UPDATE_CF_FAILED = "ut_update_call_forward_failed";
        public static final String EVENT_UT_UPDATE_CW_OK = "ut_update_call_waiting_ok";
        public static final String EVENT_UT_UPDATE_CW_FAILED = "ut_update_call_waiting_failed";

        // Query call forward result of media
        public static final String RULE_MEDIA_AUDIO = "audio";
        public static final String RULE_MEDIA_VIDEO = "video";
    }

}

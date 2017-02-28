package com.spreadtrum.ims.vowifi;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.SystemProperties;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Log;

import com.android.ims.ImsCallProfile;
import com.spreadtrum.ims.ImsConfigImpl;

import java.util.ArrayList;

public class Utilities {
    private static final String TAG = getTag(Utilities.class.getSimpleName());

    // This value will be false if release.
    public static final boolean DEBUG = true;

    // Used to get the primary card id.
    private static final int DEFAULT_PHONE_ID   = 0;
    private static final int SLOTTWO_PHONE_ID   = 1;
    private static final String PROP_MODEM_WORKMODE = "persist.radio.modem.workmode";

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

    public static boolean isVideoCall(int callType) throws UnsupportedCallTypeException {
        if (callType < ImsCallProfile.CALL_TYPE_VOICE_N_VIDEO
                || callType > ImsCallProfile.CALL_TYPE_VS_RX) {
            Log.e(TAG, "The call type " + callType + " do not exist. Please check!");
            throw new UnsupportedCallTypeException(callType);
        }
        return callType != ImsCallProfile.CALL_TYPE_VOICE;
    }

    public static int getDefaultVideoQuality(SharedPreferences preference) {
        if (preference == null) {
            // If the preferences is null, return the default value.
            return ImsConfigImpl.VT_RESOLUTION_VGA_REVERSED_30;
        }

        int quality = preference.getInt(ImsConfigImpl.VT_RESOLUTION_VALUE,
                ImsConfigImpl.VT_RESOLUTION_VGA_REVERSED_30);
        // TODO: As there is some problem when we use the quality as VGA 30 now, hard code first.
        quality = ImsConfigImpl.VT_RESOLUTION_QVGA_REVERSED_15;
        return quality;
    }

    public static ArrayList<VideoQuality> sVideoQualityList = new ArrayList<VideoQuality>();
    static {
        // Refer to ImsConfigImpl#VT_RESOLUTION_720P}
        sVideoQualityList.add(
                new VideoQuality(1280, 720, 30, 3000 * 1000, 4000 * 1000, 200 * 1000, 30, 1));
        // Refer to ImsConfigImpl#VT_RESOLUTION_VGA_REVERSED_15
        sVideoQualityList.add(
                new VideoQuality(480, 640, 15, 400 * 1000, 660 * 1000, 150 * 1000, 15, 1));
        // Refer to ImsConfigImpl#VT_RESOLUTION_VGA_REVERSED_30
        sVideoQualityList.add(
                new VideoQuality(480, 640, 30, 600 * 1000, 980 * 1000, 150 * 1000, 30, 1));
        // Refer to ImsConfigImpl#VT_RESOLUTION_QVGA_REVERSED_15
        sVideoQualityList.add(
                new VideoQuality(240, 320, 15, 256 * 1000, 320 * 1000, 100 * 1000, 15, 1));
        // Refer to ImsConfigImpl#VT_RESOLUTION_QVGA_REVERSED_30
        sVideoQualityList.add(
                new VideoQuality(240, 320, 30, 384 * 1000, 512 * 1000, 100 * 1000, 30, 1));
        // Refer to ImsConfigImpl#VT_RESOLUTION_CIF
        sVideoQualityList.add(
                new VideoQuality(352, 288, 30, 300 * 1000, 400 * 1000, 100 * 1000, 30, 1));
        // Refer to ImsConfigImpl#VT_RESOLUTION_QCIF
        sVideoQualityList.add(
                new VideoQuality(176, 144, 30, 100 * 1000, 300 * 1000, 60 * 1000, 30, 1));
        // Refer to ImsConfigImpl#VT_RESOLUTION_VGA_15
        sVideoQualityList.add(
                new VideoQuality(640, 480, 15, 400 * 1000, 660 * 1000, 150 * 1000, 15, 1));
        // Refer to ImsConfigImpl#VT_RESOLUTION_VGA_30
        sVideoQualityList.add(
                new VideoQuality(640, 480, 30, 600 * 1000, 980 * 1000, 150 * 1000, 30, 1));
        // Refer to ImsConfigImpl#VT_RESOLUTION_QVGA_15
        sVideoQualityList.add(
                new VideoQuality(320, 240, 15, 256 * 1000, 320 * 1000, 100 * 1000, 15, 1));
        // Refer to ImsConfigImpl#VT_RESOLUTION_QVGA_30
        sVideoQualityList.add(
                new VideoQuality(320, 240, 30, 384 * 1000, 512 * 1000, 100 * 1000, 30, 1));
    }

    // This defined is match the error used by CM. Please do not change.
    public static class UnsolicitedCode {
        public static final int SECURITY_DPD_DISCONNECTED = 1;
        public static final int SIP_TIMEOUT = 2;
        public static final int SIP_LOGOUT = 3;
        public static final int SECURITY_REKEY_FAILED = 4;
        public static final int SECURITY_STOP = 5;
    }

    public static class CallType {
        public static final int CALL_TYPE_UNKNOWN = 0;
        public static final int CALL_TYPE_VOICE = 1;
        public static final int CALL_TYPE_VIDEO = 2;
    }

    public static class VolteNetworkType {
        public static final int IEEE_802_11 = -1;
        public static final int E_UTRAN_FDD = 4; // 3gpp
        public static final int E_UTRAN_TDD = 5; // 3gpp
    }

    public static class VowifiNetworkType {
        public static final int IEEE_802_11 = 1;
        public static final int E_UTRAN_FDD = 9; // 3gpp
        public static final int E_UTRAN_TDD = 10; // 3gpp
    }

    public static class AttachState {
        public static final int STATE_IDLE = 0;
        public static final int STATE_PROGRESSING = 1;
        public static final int STATE_CONNECTED = 2;
    }

    public static class NativeErrorCode {
        public static final int IKE_INTERRUPT_STOP = 0xD200 + 198;
        public static final int DPD_DISCONNECT = 0xD200 + 15;
        public static final int IPSEC_REKEY_FAIL = 0xD200 + 10;
        public static final int IKE_REKEY_FAIL = 0xD200 + 11;
        public static final int REG_TIMEOUT = 0xE100 + 5;
        public static final int REG_SERVER_FORBIDDEN = 0xE100 + 8;
    }

    public static class RegisterState {
        public static final int STATE_IDLE = 0;
        public static final int STATE_PROGRESSING = 1;
        public static final int STATE_CONNECTED = 2;
    }

    public static class Result {
        public static final int FAIL = 0;
        public static final int SUCCESS = 1;

        public static final int INVALID_ID = -1;
    }

    public static class CallStateForDataRouter {
        public static final int VOLTE = 0;
        public static final int VOWIFI = 1;
        public static final int NONE = 2;
    }

    public static class IPVersion {
        public static final int NONE = -1;
        public static final int IP_V4 = 0;
        public static final int IP_V6 = 1;
    }

    public static class SecurityConfig {
        public String _pcscf4;
        public String _pcscf6;
        public String _dns4;
        public String _dns6;
        public String _ip4;
        public String _ip6;
        public boolean _prefIPv4;

        public int _useIPVersion = IPVersion.NONE;

        public SecurityConfig(String pcscf4, String pcscf6, String dns4, String dns6, String ip4,
                String ip6, boolean prefIPv4) {
            _pcscf4 = pcscf4;
            _pcscf6 = pcscf6;
            _dns4 = dns4;
            _dns6 = dns6;
            _ip4 = ip4;
            _ip6 = ip6;
            _prefIPv4 = prefIPv4;
        }

        @Override
        public String toString() {
            return "[pcscf4=" + _pcscf4 + ", pcscf6=" + _pcscf6 + ", dns4=" + _dns4 + ", dns6="
                    + _dns6 + ", ip4=" + _ip4 + ", ip6=" + _ip6 + ", pref IPv4=" + _prefIPv4 + "]";
        }
    }

    public static class RegisterIPAddress {
        private static final String JSON_PCSCF_SEP = ";";

        private String mLocalIPv4;
        private String mLocalIPv6;
        private String[] mPcscfIPv4;
        private String[] mPcscfIPv6;

        private int mIPv4Index = 0;
        private int mIPv6Index = 0;

        private String mUsedLocalIP;
        private String mUsedPcscfIP;

        public static RegisterIPAddress getInstance(String localIPv4, String localIPv6,
                String pcscfIPv4, String pcscfIPv6) {
            if (Utilities.DEBUG) {
                Log.i(TAG, "Get the s2b ip address from localIPv4: " + localIPv4 + ", localIPv6: "
                        + localIPv6 + ", pcscfIPv4: " + pcscfIPv4 + ", pcscfIPv6: " + pcscfIPv6);
            }
            if ((TextUtils.isEmpty(localIPv4) || TextUtils.isEmpty(pcscfIPv4))
                    && (TextUtils.isEmpty(localIPv6) || TextUtils.isEmpty(pcscfIPv6))) {
                Log.e(TAG, "Can not get the ip address: localIPv4: " + localIPv4 + ", localIPv6: "
                        + localIPv6 + ", pcscfIPv4: " + pcscfIPv4 + ", pcscfIPv6: " + pcscfIPv6);
                return null;
            }

            String[] pcscfIPv4s =
                    TextUtils.isEmpty(pcscfIPv4) ? null : pcscfIPv4.split(JSON_PCSCF_SEP);
            String[] pcscfIPv6s =
                    TextUtils.isEmpty(pcscfIPv6) ? null : pcscfIPv6.split(JSON_PCSCF_SEP);
            return new RegisterIPAddress(localIPv4, localIPv6, pcscfIPv4s, pcscfIPv6s);
        }

        private RegisterIPAddress(String localIPv4, String localIPv6, String[] pcscfIPv4,
                String[] pcscfIPv6) {
            mLocalIPv4 = localIPv4;
            mLocalIPv6 = localIPv6;
            mPcscfIPv4 = pcscfIPv4;
            mPcscfIPv6 = pcscfIPv6;
        }

        public String getCurUsedLocalIP() {
            return mUsedLocalIP;
        }

        public String getCurUsedPcscfIP() {
            return mUsedPcscfIP;
        }

        public String getLocalIP(boolean isIPv4) {
            mUsedLocalIP = isIPv4 ? getLocalIPv4() : getLocalIPv6();
            return mUsedLocalIP;
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

        private String getLocalIPv6() {
            return mLocalIPv6;
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

        @Override
        public String toString() {
            StringBuilder builder = new StringBuilder();
            builder.append("localIPv4 = " + mLocalIPv4);
            builder.append(", localIPv6 = " + mLocalIPv6);
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

    public static class UnsupportedCallTypeException extends Exception {
        public UnsupportedCallTypeException(int callType) {
            super("The call type " + callType + " do not exist.");
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

    public static class VideoQuality {
        public int _width;
        public int _height;
        public int _frameRate;
        public int _bitRate;
        public int _brHi;
        public int _brLo;
        public int _frHi;
        public int _frLo;

        public VideoQuality(int width, int height, int frameRate, int bitRate, int brHi, int brLo,
                int frHi, int frLo) {
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
            return "[width=" + _width + ", height=" + _height + ", frameRate=" + _frameRate
                    + ", bitRate=" + _bitRate + ", BrHi=" + _brHi + ", BrLo=" + _brLo
                    + ", FrHi=" + _frHi + ", FrLo=" + _frLo + "]";
        }
    }

    public static class JSONUtils {
        // Callback constants
        public static final String KEY_EVENT_CODE = "event_code";
        public static final String KEY_EVENT_NAME = "event_name";

        // Register
        public static final String KEY_STATE_CODE = "state_code";
        public static final String KEY_RETRY_AFTER = "retry_after";

        public static final int REGISTER_EVENT_CODE_BASE = 0;
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
        public static final String KEY_VIDEO_ORIENTATION = "video_orientation";
        public static final String KEY_RTP_RECEIVED = "rtp_received";
        public static final String KEY_RTCP_LOSE = "rtcp_lose";
        public static final String KEY_RTCP_JITTER = "rtcp_jitter";
        public static final String KEY_RTCP_RTT = "rtcp_rtt";
        public static final String KEY_CONF_PART_NEW_STATUS = "conf_part_new_status";

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
        public static final int EVENT_CODE_CALL_IS_FOCUS = CALL_EVENT_CODE_BASE + 19;
        public static final int EVENT_CODE_CALL_RTCP_CHANGED = CALL_EVENT_CODE_BASE + 20;

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
        public static final String EVENT_CALL_IS_FOCUS = "call_is_focus";
        public static final String EVENT_CALL_RTCP_CHANGED = "call_rtcp_changed";

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
        public static final int RESIZE_EVENT_CODE_BASE = 300;
        public static final int EVENT_CODE_LOCAL_VIDEO_RESIZE = RESIZE_EVENT_CODE_BASE + 1;
        public static final int EVENT_CODE_REMOTE_VIDEO_RESIZE = RESIZE_EVENT_CODE_BASE + 2;

        public static final String EVENT_LOCAL_VIDEO_RESIZE = "local_video_resize";
        public static final String EVENT_REMOTE_VIDEO_RESIZE = "remote_video_resize";

        // UT
        public static final String KEY_UT_CF_TIME_SECONDS = "ut_cf_time_seconds";
        public static final String KEY_UT_CF_RULES = "ut_cf_rules";
        public static final String KEY_UT_CF_RULE_ENABLED = "ut_cf_rule_enabled";
        public static final String KEY_UT_CF_RULE_MEDIA = "ut_cf_rule_media";
        public static final String KEY_UT_CF_CONDS = "ut_cf_conditions";
        public static final String KEY_UT_CF_ACTION_TARGET = "ut_cf_action_target";
        public static final String KEY_UT_CW_ENABLED = "ut_cw_enabled";

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
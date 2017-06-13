package com.spreadtrum.ims;

import android.hardware.radio.V1_0.Carrier;
import android.hardware.radio.V1_0.CarrierRestrictions;
import android.hardware.radio.V1_0.CdmaBroadcastSmsConfigInfo;
import android.hardware.radio.V1_0.CdmaSmsAck;
import android.hardware.radio.V1_0.CdmaSmsMessage;
import android.hardware.radio.V1_0.CdmaSmsWriteArgs;
import android.hardware.radio.V1_0.CellInfoCdma;
import android.hardware.radio.V1_0.CellInfoGsm;
import android.hardware.radio.V1_0.CellInfoLte;
import android.hardware.radio.V1_0.CellInfoType;
import android.hardware.radio.V1_0.CellInfoWcdma;
import android.hardware.radio.V1_0.DataProfileInfo;
import android.hardware.radio.V1_0.Dial;
import android.hardware.radio.V1_0.GsmBroadcastSmsConfigInfo;
import android.hardware.radio.V1_0.GsmSmsMessage;
import android.hardware.radio.V1_0.HardwareConfigModem;
import android.hardware.radio.V1_0.IRadio;
import android.hardware.radio.V1_0.IccIo;
import android.hardware.radio.V1_0.ImsSmsMessage;
import android.hardware.radio.V1_0.LceDataInfo;
import android.hardware.radio.V1_0.MvnoType;
import android.hardware.radio.V1_0.NvWriteItem;
import android.hardware.radio.V1_0.RadioError;
import android.hardware.radio.V1_0.RadioIndicationType;
import android.hardware.radio.V1_0.RadioResponseInfo;
import android.hardware.radio.V1_0.RadioResponseType;
import android.hardware.radio.V1_0.ResetNvType;
import android.hardware.radio.V1_0.SelectUiccSub;
import android.hardware.radio.V1_0.SetupDataCallResult;
import android.hardware.radio.V1_0.SimApdu;
import android.hardware.radio.V1_0.SmsWriteArgs;
import android.hardware.radio.V1_0.UusInfo;
import android.os.AsyncResult;
import android.os.Message;
import android.os.Parcel;
import android.telephony.PhoneNumberUtils;
import com.android.ims.internal.ImsCallForwardInfoEx;

import com.android.internal.telephony.*;
import com.android.internal.telephony.uicc.*;

import java.util.ArrayList;
import java.util.Collections;

import vendor.sprd.hardware.radio.V1_0.CallForwardInfoUri;
import vendor.sprd.hardware.radio.V1_0.CallVoLTE;
import vendor.sprd.hardware.radio.V1_0.ExtPersoSubstate;
import vendor.sprd.hardware.radio.V1_0.ExtRadioErrno;
import vendor.sprd.hardware.radio.V1_0.IExtRadio;
import vendor.sprd.hardware.radio.V1_0.IExtRadioIndication;
import vendor.sprd.hardware.radio.V1_0.IExtRadioResponse;
import vendor.sprd.hardware.radio.V1_0.IIMSRadioIndication;
import vendor.sprd.hardware.radio.V1_0.IIMSRadioResponse;
import vendor.sprd.hardware.radio.V1_0.ImsHandoverToVoWifiResult;
import vendor.sprd.hardware.radio.V1_0.ImsHandoverType;
import vendor.sprd.hardware.radio.V1_0.ImsNetworkInfo;
import vendor.sprd.hardware.radio.V1_0.ImsPdnStatus;
import vendor.sprd.hardware.radio.V1_0.ImsPhoneCMCCSI;
import vendor.sprd.hardware.radio.V1_0.NetworkList;
import vendor.sprd.hardware.radio.V1_0.StkCallControlResult;
import vendor.sprd.hardware.radio.V1_0.VideoPhoneCodec;
import vendor.sprd.hardware.radio.V1_0.VideoPhoneDial;
import vendor.sprd.hardware.radio.V1_0.VideoPhoneDSCI;

public class IMSRadioResponse extends IIMSRadioResponse.Stub {
    private ImsRIL mRil;

    public IMSRadioResponse(ImsRIL ril){
        mRil = ril;
    }

    /**
     * Helper function to send response msg
     * @param msg Response message to be sent
     * @param ret Return object to be included in the response message
     */
    static void sendMessageResponse(Message msg, Object ret) {
        if (msg != null) {
            AsyncResult.forMessage(msg, ret, null);
            msg.sendToTarget();
        }
    }

    /**
     * Acknowledge the receipt of radio request sent to the vendor. This must be sent only for
     * radio request which take long time to respond.
     * For more details, refer https://source.android.com/devices/tech/connect/ril.html
     *
     * @param serial Serial no. of the request whose acknowledgement is sent.
     */
    public void acknowledgeRequest(int serial) {
        mRil.processRequestAck(serial);
    }

    /*Interface implements*/
    public void getIMSCurrentCallsResponse(RadioResponseInfo info, java.util.ArrayList<CallVoLTE> calls){
        responseImsCallList(info, calls);
    }

    private void responseImsCallList(RadioResponseInfo responseInfo, ArrayList<CallVoLTE> calls) {
        RILRequest rr = mRil.processResponse(responseInfo);
        if (rr != null) {
            int num = calls.size();
            ArrayList<ImsDriverCall> dcCalls = new ArrayList<ImsDriverCall>(num);
            ImsDriverCall dc;

            for (int i = 0; i < num; i++) {
                dc = new ImsDriverCall();
            /* parameter from +CLCCS:
             * [+CLCCS: <ccid1>,<dir>,<neg_status_present>,<neg_status>,<SDP_md>,
             * <cs_mode>,<ccstatus>,<mpty>,[,<numbertype>,<ton>,<number>
             * [,<priority_present>,<priority>[,<CLI_validity_present>,<CLI_validity>]]]
             * @{*/
                dc.index = calls.get(i).index;
                dc.isMT = (calls.get(i).isMT != 0);
                dc.negStatusPresent = calls.get(i).negStatusPresent;
                dc.negStatus = calls.get(i).negStatus;
                dc.mediaDescription = calls.get(i).mediaDescription;
                dc.csMode = calls.get(i).csMode;
                dc.state = ImsDriverCall.stateFromCLCCS(calls.get(i).state);
                boolean videoMediaPresent = false;
                if(dc.mediaDescription != null && dc.mediaDescription.contains("video") && !dc.mediaDescription.contains("cap:")){
                    videoMediaPresent = true;
                }
                boolean videoMode = videoMediaPresent &&
                        ((dc.negStatusPresent == 1 && dc.negStatus == 1)
                                ||(dc.negStatusPresent == 1 && dc.negStatus == 2 && dc.state == ImsDriverCall.State.INCOMING)
                                ||(dc.negStatusPresent == 1 && dc.negStatus == 3)
                                ||(dc.negStatusPresent == 0 && dc.csMode == 0));
                if(videoMode || dc.csMode == 2 || dc.csMode >= 7){
                    dc.isVoice = false;
                } else {
                    dc.isVoice = true;
                }
                int mpty = calls.get(i).mpty;
                dc.mptyState = mpty;
                dc.isMpty = (0 != mpty);
                dc.numberType = calls.get(i).numberType;
                dc.TOA = calls.get(i).toa;
                dc.number = calls.get(i).number;
                dc.prioritypresent = calls.get(i).prioritypresent;
                dc.priority = calls.get(i).priority;
                dc.cliValidityPresent = calls.get(i).cliValidityPresent;
                dc.numberPresentation = ImsDriverCall.presentationFromCLIP(calls.get(i).numberPresentation);

                dc.als = calls.get(i).als;
                dc.isVoicePrivacy = (calls.get(i).isVoicePrivacy == 1);
                dc.name = calls.get(i).name;
                dc.namePresentation = calls.get(i).namePresentation;

                // Make sure there's a leading + on addresses with a TOA of 145
                dc.number = PhoneNumberUtils.stringFromStringAndTOA(dc.number, dc.TOA);
                mRil.riljLog("responseCallListEx: dc=" +dc.toString());
                dcCalls.add(dc);

                if (dc.isVoicePrivacy) {
                    mRil.riljLog("InCall VoicePrivacy is enabled");
                } else {
                    mRil.riljLog("InCall VoicePrivacy is disabled");
                }
            }

            Collections.sort(dcCalls);

            if (responseInfo.error == RadioError.NONE) {
                sendMessageResponse(rr.mResult, dcCalls);
            }
            mRil.processResponseDone(rr, responseInfo, dcCalls);
        }
    }


    public void setIMSVoiceCallAvailabilityResponse(RadioResponseInfo info){
        responseVoid(info);
    }

    public void getIMSVoiceCallAvailabilityResponse(RadioResponseInfo info, int state){
        responseInts(info, state);
    }

    public void initISIMResponse(RadioResponseInfo info, int response){
        responseInts(info, response);
    }

    public void requestVolteCallMediaChangeResponse(RadioResponseInfo info){
        responseVoid(info);
    }

    public void responseVolteCallMediaChangeResponse(RadioResponseInfo info){
        responseVoid(info);
    }

    public void setIMSSmscAddressResponse(RadioResponseInfo info){
        responseVoid(info);
    }

    public void volteCallFallBackToVoiceResponse(RadioResponseInfo info){
        responseVoid(info);
    }

    public void setIMSInitialAttachApnResponse(RadioResponseInfo info){
        responseVoid(info);
    }

    public void queryCallForwardStatusResponse(RadioResponseInfo info, ArrayList<CallForwardInfoUri> callForwardInfos){
        responseCallForwardInfo(info, callForwardInfos);
    }
    private void responseCallForwardInfo(RadioResponseInfo responseInfo,
                                         ArrayList<CallForwardInfoUri> callForwardInfos) {
        RILRequest rr = mRil.processResponse(responseInfo);
        if (rr != null) {
            ImsCallForwardInfoEx[] ret = new ImsCallForwardInfoEx[callForwardInfos.size()];
            for (int i = 0; i < callForwardInfos.size(); i++) {
                ret[i] = new ImsCallForwardInfoEx();
                ret[i].mStatus = callForwardInfos.get(i).status;
                ret[i].mCondition = callForwardInfos.get(i).reason;
                ret[i].mNumberType = callForwardInfos.get(i).numberType;
                ret[i].mToA = callForwardInfos.get(i).ton;
                ret[i].mNumber = callForwardInfos.get(i).number;
                ret[i].mServiceClass = callForwardInfos.get(i).serviceClass;
                ret[i].mRuleset = callForwardInfos.get(i).ruleset;
                ret[i].mTimeSeconds = callForwardInfos.get(i).timeSeconds;
                mRil.riljLog("responseCallForwardUri> "
                        + " ret[i].number:" + ret[i].mNumber);
            }
            if (responseInfo.error == RadioError.NONE) {
                sendMessageResponse(rr.mResult, ret);
            }
            mRil.processResponseDone(rr, responseInfo, ret);
        }
    }

    public void setCallForwardUriResponse(RadioResponseInfo info){
        responseVoid(info);
    }

    public void IMSInitialGroupCallResponse(RadioResponseInfo info){
        responseVoid(info);
    }

    public void IMSAddGroupCallResponse(RadioResponseInfo info){
        responseVoid(info);
    }

    public void enableIMSResponse(RadioResponseInfo info){
        responseVoid(info);
    }

    public void disableIMSResponse(RadioResponseInfo info){
        responseVoid(info);
    }

    public void getIMSBearerStateResponse(RadioResponseInfo info, int state){
        responseInts(info, state);
    }

    public void setInitialAttachSOSApnResponse(RadioResponseInfo info){
        responseVoid(info);
    }

    public void IMSHandoverResponse(RadioResponseInfo info){
        responseVoid(info);
    }

    public void notifyIMSHandoverStatusUpdateResponse(RadioResponseInfo info){
        responseVoid(info);
    }

    public void notifyIMSNetworkInfoChangedResponse(RadioResponseInfo info){
        responseVoid(info);
    }

    public void notifyIMSCallEndResponse(RadioResponseInfo info){
        responseVoid(info);
    }

    public void getTPMRStateResponse(RadioResponseInfo info, int state){
        responseInts(info, state);
    }

    public void setTPMRStateResponse(RadioResponseInfo info){
        responseVoid(info);
    }

    public void notifyVoWifiEnableResponse(RadioResponseInfo info){
        responseVoid(info);
    }

    public void notifyVoWifiCallStateChangedResponse(RadioResponseInfo info){
        responseVoid(info);
    }

    public void notifyDataRouterUpdateResponse(RadioResponseInfo info){
        responseVoid(info);
    }

    public void IMSHoldSingleCallResponse(RadioResponseInfo info){
        responseVoid(info);
    }

    public void IMSMuteSingleCallResponse(RadioResponseInfo info){
        responseVoid(info);
    }

    public void IMSSilenceSingleCallResponse(RadioResponseInfo info){
        responseVoid(info);
    }

    public void IMSEnableLocalConferenceResponse(RadioResponseInfo info){
        responseVoid(info);
    }

    public void notifyHandoverCallInfoResponse(RadioResponseInfo info){
        responseVoid(info);
    }

    public void getSrvccCapbilityResponse(RadioResponseInfo info, int response){
        responseInts(info, response);
    }

    public void getIMSPcscfAddressResponse(RadioResponseInfo info, String addr){
        responseString(info, addr);
    }

    public void setIMSPcscfAddressResponse(RadioResponseInfo info){
        responseVoid(info);
    }

    /*response method*/
    private void responseInts(RadioResponseInfo responseInfo, int ...var) {
        final ArrayList<Integer> ints = new ArrayList<>();
        for (int i = 0; i < var.length; i++) {
            ints.add(var[i]);
        }
        responseIntArrayList(responseInfo, ints);
    }

    private void responseIntArrayList(RadioResponseInfo responseInfo, ArrayList<Integer> var) {
        RILRequest rr = mRil.processResponse(responseInfo);

        if (rr != null) {
            int[] ret = new int[var.size()];
            for (int i = 0; i < var.size(); i++) {
                ret[i] = var.get(i);
            }
            if (responseInfo.error == RadioError.NONE) {
                sendMessageResponse(rr.mResult, ret);
            }
            mRil.processResponseDone(rr, responseInfo, ret);
        }
    }

    private void responseVoid(RadioResponseInfo responseInfo) {
        RILRequest rr = mRil.processResponse(responseInfo);

        if (rr != null) {
            Object ret = null;
            if (responseInfo.error == RadioError.NONE) {
                sendMessageResponse(rr.mResult, ret);
            }
            mRil.processResponseDone(rr, responseInfo, ret);
        }
    }

    private void responseString(RadioResponseInfo responseInfo, String str) {
        RILRequest rr = mRil.processResponse(responseInfo);

        if (rr != null) {
            if (responseInfo.error == RadioError.NONE) {
                sendMessageResponse(rr.mResult, str);
            }
            mRil.processResponseDone(rr, responseInfo, str);
        }
    }

    private void responseStrings(RadioResponseInfo responseInfo, String ...str) {
        ArrayList<String> strings = new ArrayList<>();
        for (int i = 0; i < str.length; i++) {
            strings.add(str[i]);
        }
        responseStringArrayList(mRil, responseInfo, strings);
    }

    static void responseStringArrayList(ImsRIL ril, RadioResponseInfo responseInfo,
                                        ArrayList<String> strings) {
        RILRequest rr = ril.processResponse(responseInfo);

        if (rr != null) {
            String[] ret = new String[strings.size()];
            for (int i = 0; i < strings.size(); i++) {
                ret[i] = strings.get(i);
            }
            if (responseInfo.error == RadioError.NONE) {
                sendMessageResponse(rr.mResult, ret);
            }
            ril.processResponseDone(rr, responseInfo, ret);
        }
    }

}

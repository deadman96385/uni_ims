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

import java.util.ArrayList;

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

public class IMSRadioIndication extends IIMSRadioIndication.Stub {
    private ImsRIL mRil;

    public IMSRadioIndication(ImsRIL ril){
        mRil = ril;
    }

    public void IMSCallStateChangedInd(int type){
        mRil.processIndication(type);
        mRil.unsljLog(ImsRILConstants.RIL_UNSOL_RESPONSE_IMS_CALL_STATE_CHANGED);
        if(mRil.mImsCallStateRegistrants != null) {
            mRil.mImsCallStateRegistrants.notifyRegistrants();
        }
    }

    public void videoQualityInd(int type, java.util.ArrayList<Integer> data){
        mRil.processIndication(type);
        mRil.unsljLog(ImsRILConstants.RIL_UNSOL_RESPONSE_VIDEO_QUALITY);
        if(mRil.mImsVideoQosRegistrant != null) {
            mRil.mImsVideoQosRegistrant.notifyRegistrant(new AsyncResult(null, arrayListToPrimitiveArray(data), null));
        }
    }

    public void IMSBearerEstablished(int type, int status){
        mRil.processIndication(type);
        mRil.unsljLog(ImsRILConstants.RIL_UNSOL_RESPONSE_IMS_BEARER_ESTABLISTED);
        if(mRil.mImsBearerStateRegistrant != null) {
            mRil.mImsBearerStateRegistrant.notifyRegistrants(new AsyncResult(null, status, null));
        }
    }

    public void IMSHandoverRequestInd(int type, int status){
        mRil.processIndication(type);
        mRil.unsljLog(ImsRILConstants.RIL_UNSOL_IMS_HANDOVER_REQUEST);
        if(mRil.mImsHandoverRequestRegistrant != null) {
            mRil.mImsHandoverRequestRegistrant.notifyRegistrant(new AsyncResult(null, status, null));
        }
    }

    public void IMSHandoverStatusChangedInd(int type, int status){
        mRil.processIndication(type);
        mRil.unsljLog(ImsRILConstants.RIL_UNSOL_IMS_HANDOVER_STATUS_CHANGE);
        if(mRil.mImsHandoverStatusRegistrant != null) {
            mRil.mImsHandoverStatusRegistrant.notifyRegistrant(new AsyncResult(null, status, null));
        }
    }

    public void IMSNetworkInfoChangedInd(int type, ImsNetworkInfo nwInfo){
        mRil.processIndication(type);
        mRil.unsljLog(ImsRILConstants.RIL_UNSOL_IMS_NETWORK_INFO_CHANGE);
        if(mRil.mImsNetworkInfoRegistrant != null) {
            mRil.mImsNetworkInfoRegistrant.notifyRegistrant(new AsyncResult(null, nwInfo, null));
        }
    }

    public void IMSRegisterAddressChangedInd(int type,  ArrayList<String> addr){
        mRil.processIndication(type);
        mRil.unsljLog(ImsRILConstants.RIL_UNSOL_IMS_REGISTER_ADDRESS_CHANGE);
        if(mRil.mImsRegAddressRegistrant != null) {
            mRil.mImsRegAddressRegistrant.notifyRegistrant(new AsyncResult(null, arrayListToStringArray(addr), null));
        }
    }

    public void IMSWifiParamInd(int type, java.util.ArrayList<Integer> data){
        mRil.processIndication(type);
        mRil.unsljLog(ImsRILConstants.RIL_UNSOL_IMS_WIFI_PARAM);
        if(mRil.mImsWiFiParamRegistrant != null) {
            mRil.mImsWiFiParamRegistrant.notifyRegistrant(new AsyncResult(null, arrayListToPrimitiveArray(data), null));
        }
    }

    public void IMSNetworkStateChangedInd(int type, int status){
        mRil.processIndication(type);
        mRil.unsljLog(ImsRILConstants.RIL_UNSOL_IMS_NETWORK_STATE_CHANGED);
        if(mRil.mImsNetworkStateChangedRegistrants != null) {
            mRil.mImsNetworkStateChangedRegistrants.notifyRegistrants(new AsyncResult(null, status, null));
        }
    }

    public static int[] arrayListToPrimitiveArray(ArrayList<Integer> ints) {
        int[] ret = new int[ints.size()];
        for (int i = 0; i < ret.length; i++) {
            ret[i] = ints.get(i);
        }
        return ret;
    }

    public static String[] arrayListToStringArray(ArrayList<String> data){
        String[] ret = new String[data.size()];
        for(int i = 0; i<ret.length; i++){
            ret[i] = data.get(i);
        }
        return  ret;
    }
}

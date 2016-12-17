package com.spreadtrum.ims;

public class ImsNetworkInfo {
    private static String TAG = ImsNetworkInfo.class.getSimpleName();
    public ImsNetworkInfo(){
    }

    public int mType;
    public String mInfo;
    @Override
    public String
    toString() {
        return "ImsNetworkInfo: mType:" + mType + " mInfo:" + mInfo;
    }

}

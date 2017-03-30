package com.spreadtrum.ims;

public class ImsServiceState {
    public boolean mImsRegistered;
    public int mRegState;
    // SPRD:ADD for bug 660338/625186
    public int mSrvccState = -1;

    public ImsServiceState(boolean imsRegistered, int regState){
        mImsRegistered = imsRegistered;
        mRegState = regState;
    }
}

package com.spreadtrum.ims;

public class ImsServiceState {
    public boolean mImsRegistered;
    public int mRegState;

    public ImsServiceState(boolean imsRegistered, int regState){
        mImsRegistered = imsRegistered;
        mRegState = regState;
    }
}

package com.spreadtrum.ims;

import com.android.ims.internal.IImsEcbm;
import com.android.ims.internal.IImsEcbmListener;
import com.android.internal.telephony.CommandsInterface;

public class ImsEcbmImpl extends IImsEcbm.Stub {

    private CommandsInterface mCi;

    public ImsEcbmImpl(CommandsInterface ci){
        mCi = ci;
    }

    /**
     * Sets the listener.
     */
    @Override
    public void setListener(IImsEcbmListener listener){

    }

    /**
     * Requests Modem to come out of ECBM mode
     */
    @Override
    public void exitEmergencyCallbackMode(){

    }
}

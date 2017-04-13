// ISecurityS2b.aidl
package com.juphoon.sprd.security;

import com.juphoon.sprd.security.ISecurityS2bCallback;

// Declare any non-default types here with import statements

interface ISecurityS2b {

    void registerCallback(ISecurityS2bCallback callback);

    void unregisterCallback(ISecurityS2bCallback callback);

    void Mtc_S2bStart(String config);

    void Mtc_S2bStop(boolean handover);

    int Mtc_S2bGetState();

    boolean Mtc_S2bSwitchLogin(int type);

    void Mtc_S2bDeleteTunelIpsec();
}

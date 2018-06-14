package com.spreadtrum.vowifi.service;

import com.spreadtrum.vowifi.service.ISecurityServiceCallback;

interface ISecurityService {

    void registerCallback(ISecurityServiceCallback callback);

    void unregisterCallback(ISecurityServiceCallback callback);

    int start(boolean isHandover, int type, int subId, String imsi, String hplmn, String vplmn, String imei);

    void stop(int sessionId, boolean forHandover);

    int getState(int sessionId);

    boolean switchLoginIpVersion(int sessionId, int ipVersion);

    boolean deleteTunelIpsec(int sessionId);

    void setVolteUsedLocalAddr(String addr);
}

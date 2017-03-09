package com.spreadtrum.vowifi.service;

import com.spreadtrum.vowifi.service.IRegisterServiceCallback;

interface IRegisterService {

    void registerCallback(IRegisterServiceCallback callback);

    void unregisterCallback(IRegisterServiceCallback callback);

    int cliOpen(String accountName);

    int cliStart();

    int cliUpdateSettings(int subId, String spn, String imei, String userName, String authName,
            String authPass, String realm, String impu, boolean isSRVCCSupport);

    /**
     * Start the SIP register process. Before login, you need open, start and update
     * account settings first.
     *
     * @return {@link Utils#RESULT_FAIL} as fail. If login failed, please handle it.
     *         {@link Utils#RESULT_SUCCESS} as success.
     */
    int cliLogin(boolean forSos, boolean isIPv4, String localIP, String pcscfIP);

    /**
     * Start the SIP re-register process.
     *
     * @return {@link Utils#RESULT_FAIL} as fail. If re-register failed, please handle it.
     *         {@link Utils#RESULT_SUCCESS} as success.
     */
    int cliRefresh(int type, String info);

    /**
     * Start the logout process.
     *
     * @return {@link Utils#RESULT_FAIL} as fail. If logout failed, please handle it.
     *         {@link Utils#RESULT_SUCCESS} as success.
     */
    int cliLogout();

    int cliReset();
}

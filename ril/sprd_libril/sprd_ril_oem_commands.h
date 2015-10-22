//vendor/sprd/proprietories-source/ril/sprd_libril/sprd_ril_oem_commands.h

#if defined (RIL_SPRD_EXTENSION)
    {OEM_REQ_FUNCID_UNKNOWN, NULL, NULL},                   //none
    {OEM_REQ_FUNCID_CALL_BLACKLIST, dispatchCallBlack, responseVoid},
    {OEM_REQ_FUNCID_SIM_POWER, dispatchInts, responseVoid},
    {OEM_REQ_FUNCID_GET_REMAIN_TIMES, dispatchInts, responseInts},
    {OEM_REQ_FUNCID_STK_DIAL, dispatchVoid, responseVoid},
    {OEM_REQ_FUNCID_QUERY_COLP_COLR, dispatchVoid, responseInts},
    {OEM_REQ_FUNCID_MMI_ENTER_SIM, dispatchString, responseVoid},
    {OEM_REQ_FUNCID_VIDEOPHONE, dispatchVideoPhone, responseVideoPhone},
    {OEM_REQ_FUNCID_MBBMS, dispatchMBBMS, responseString},
    {OEM_REQ_FUNCID_GET_SIM_CAPACITY, dispatchVoid, responseStrings},
    {OEM_REQ_FUNCID_END_ALL_CONNECTIONS, dispatchVoid, responseVoid},
    {OEM_REQ_FUNCID_SET_CMMS, dispatchInts, responseVoid},
    {OEM_REQ_FUNCID_CSFALLBACK, dispatchVoid, responseVoid},
    {OEM_REQ_FUNCID_PRIORITY_NETWORK, dispatchPrioNet, responsePrioNet},
    {OEM_REQ_FUNCID_GET_BAND_INFO, dispatchVoid, responseString},
    {OEM_REQ_FUNCID_SWITCH, dispatchInts, responseVoid},
    {OEM_REQ_FUNCID_STOP_QUERY_AVAILABLE_NETWORKS, dispatchVoid, responseVoid},
    {OEM_REQ_FUNCID_INIT_ISIM, dispatchVoid, responseInts},
    {OEM_REQ_FUNCID_IMS, dispatchIMS, responseIMS},
    {OEM_REQ_FUNCID_VOLTE, dispatchVolte, responseVolte},
    {OEM_REQ_FUNCID_ENABLE_BROADCAST_SMS, dispatchInts, responseVoid},
    {OEM_REQ_FUNCID_SIMLOCK, dispatchSimlock, responseSimlock}

#endif



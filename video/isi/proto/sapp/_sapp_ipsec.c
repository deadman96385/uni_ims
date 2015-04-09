/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30369 $ $Date: 2014-12-11 19:09:13 +0800 (Thu, 11 Dec 2014) $
 */

#include <osal_types.h>
#include <osal.h>
#include <osal_msg.h>
#include <osal_string.h>

#include <ezxml.h>
#include <auth_b64.h>
#include <milenage.h>

#include <sip_sip.h>
#include <sip_xport.h>
#include <sip_ua.h>
#include <sip_app.h>
#include <sip_debug.h>
#include <sip_auth.h>

#include "isi.h"
#include "isip.h"

#include "mns.h"

#ifdef INCLUDE_SIMPLE
#include "xcap.h"
#include "_simple_types.h"
#endif

#include "_sapp.h"
#include "_sapp_parse_helper.h"
#include "_sapp_ipsec.h"

#include <ims_net.h>
#include <sr.h>
const char *_SAPP_ipsecAlgStrings[OSAL_IPSEC_AUTH_ALG_LAST + 1] = {
    "hmac-md5-96",
    "hmac-sha-1-96",
    ""
};

const char *_SAPP_ipsecProtStrings[OSAL_IPSEC_PROTOCOL_LAST + 1] = {
    "esp",
    "ah",
};

const char *_SAPP_ipsecModStrings[OSAL_IPSEC_SP_MODE_LAST + 1] = {
    "trans",
    "tunnel"
};

const char *_SAPP_ipsecEAlgStrings[OSAL_IPSEC_ENC_ALG_LAST + 1] = {
    "",
    "des-ede3-cbc",
    "aes-cbc"
};

/*
 * ======== _SAPP_ipsecSendIsiEvt() ========
 * This function is used to notify IPSec protected ports and SPIs to CSM.
 */
void _SAPP_ipsecSendIsiEvt(
    SAPP_ServiceObj *service_ptr,
    SAPP_Event      *evt_ptr,
    ISIP_Status      status)
{

    /* Populate isi event */
    SAPP_serviceIsiEvt(service_ptr->isiServiceId, service_ptr->protocolId,
            ISIP_SERVICE_REASON_IPSEC, status, &evt_ptr->isiMsg);

    /* Fill in ports and spis */
    evt_ptr->isiMsg.msg.service.settings.ipsec.resp.portUc =
            OSAL_netNtohs(service_ptr->ipsecObj.outboundSAc.srcAddr.port);
    evt_ptr->isiMsg.msg.service.settings.ipsec.resp.portUs =
            OSAL_netNtohs(service_ptr->ipsecObj.outboundSAs.srcAddr.port);
    evt_ptr->isiMsg.msg.service.settings.ipsec.resp.portPc =
            OSAL_netNtohs(service_ptr->ipsecObj.outboundSAc.dstAddr.port);
    evt_ptr->isiMsg.msg.service.settings.ipsec.resp.portPs =
            OSAL_netNtohs(service_ptr->ipsecObj.outboundSAs.dstAddr.port);
    evt_ptr->isiMsg.msg.service.settings.ipsec.resp.spiUc  =
            service_ptr->ipsecObj.inboundSAc.spi;
    evt_ptr->isiMsg.msg.service.settings.ipsec.resp.spiUs  =
            service_ptr->ipsecObj.inboundSAs.spi;
    evt_ptr->isiMsg.msg.service.settings.ipsec.resp.spiPc  =
            service_ptr->ipsecObj.outboundSAc.spi;
    evt_ptr->isiMsg.msg.service.settings.ipsec.resp.spiPs  =
            service_ptr->ipsecObj.outboundSAs.spi;

    /* Send event */
    SAPP_sendEvent(evt_ptr);
}

/*
 * ======== _SAPP_ipsecGetProtectedPort() ========
 * This function is used to get the protected port from port pool.
 *
 * Returns:
 *  protected port
 */
uint16 _SAPP_ipsecGetProtectedPort(
    SAPP_ServiceObj *service_ptr)
{
    uint16 lastPort;

    lastPort = service_ptr->ipsecObj.lastProtectedPort + 1;
    if (lastPort > (service_ptr->ipsecObj.startProtectedPort +
            service_ptr->ipsecObj.pPortPoolSize)) {
        lastPort = service_ptr->ipsecObj.startProtectedPort;
    }
    service_ptr->ipsecObj.lastProtectedPort = lastPort;

    return lastPort;
}

/*
 * ======== _SAPP_ipsecGetSpi() ========
 * This function is used to get SPI from spi pool.
 *
 * Returns:
 *  SPI
 */
vint _SAPP_ipsecGetSpi(
    SAPP_ServiceObj *service_ptr)
{
    int lastSpi;

    lastSpi = service_ptr->ipsecObj.lastSpi + 1;
    if (lastSpi > (service_ptr->ipsecObj.startSpi +
            service_ptr->ipsecObj.spiPoolSize)) {
        lastSpi = service_ptr->ipsecObj.startSpi;
    }
    service_ptr->ipsecObj.lastSpi = lastSpi;

    return lastSpi;
}

/*
 * ======== _SAPP_ipsecSetAlgorithm() ========
 * This function is to set SA AH algorithm to SAs in SAPP_ServiceObj.
 *
 * Return Values:
 *   SAPP_OK    Successful.
 *   SAPP_ERR   Failed.
 */
vint _SAPP_ipsecSetAlgorithm(
    SAPP_ServiceObj *service_ptr,
    char *alg_ptr)
{
    int i;

    for (i = 0; i < OSAL_IPSEC_AUTH_ALG_LAST; i++) {
        if (0 == OSAL_strcmp(alg_ptr, &_SAPP_ipsecAlgStrings[i][0])) {
            service_ptr->ipsecObj.outboundSAc.algAh = (OSAL_IpsecAuthAlg)i;
            service_ptr->ipsecObj.outboundSAs.algAh = (OSAL_IpsecAuthAlg)i;
            service_ptr->ipsecObj.inboundSAc.algAh = (OSAL_IpsecAuthAlg)i;
            service_ptr->ipsecObj.inboundSAs.algAh = (OSAL_IpsecAuthAlg)i;
            return (SAPP_OK);
        }
    }

    return (SAPP_ERR);
}

/*
 * ======== _SAPP_ipsecSetProtocol() ========
 * This function is to set SA protocol to SAs in SAPP_ServiceObj.
 *
 * Return Values:
 *   SAPP_OK    Successful.
 *   SAPP_ERR   Failed.
 */
vint _SAPP_ipsecSetProtocol(
    SAPP_ServiceObj *service_ptr,
    const char *alg_ptr)
{
    int i;

    for (i = 0; i <= OSAL_IPSEC_PROTOCOL_LAST; i++) {
        if (0 == OSAL_strcmp(alg_ptr, &_SAPP_ipsecProtStrings[i][0])) {
            service_ptr->ipsecObj.outboundSAc.protocol = (OSAL_IpsecProtocol)i;
            service_ptr->ipsecObj.outboundSAs.protocol = (OSAL_IpsecProtocol)i;
            service_ptr->ipsecObj.inboundSAc.protocol = (OSAL_IpsecProtocol)i;
            service_ptr->ipsecObj.inboundSAs.protocol = (OSAL_IpsecProtocol)i;
            return (SAPP_OK);
        }
    }

    return (SAPP_ERR);
}

/*
 * ======== _SAPP_ipsecSetMode() ========
 * This function is to set SA mode to SAs in SAPP_ServiceObj.
 *
 * Return Values:
 *   SAPP_OK    Successful.
 *   SAPP_ERR   Failed.
 */
vint _SAPP_ipsecSetMode(
    SAPP_ServiceObj *service_ptr,
    const char *alg_ptr)
{
    int i;

    for (i = 0; i <= OSAL_IPSEC_SP_MODE_LAST; i++) {
        if (0 == OSAL_strcmp(alg_ptr, &_SAPP_ipsecModStrings[i][0])) {
            service_ptr->ipsecObj.outboundSAc.mode = (OSAL_IpsecMode)i;
            service_ptr->ipsecObj.outboundSAs.mode = (OSAL_IpsecMode)i;
            service_ptr->ipsecObj.inboundSAc.mode = (OSAL_IpsecMode)i;
            service_ptr->ipsecObj.inboundSAs.mode = (OSAL_IpsecMode)i;
            return (SAPP_OK);
        }
    }

    return (SAPP_ERR);
}

/*
 * ======== _SAPP_ipsecSetEncAlgorithm() ========
 * This function is to set encryption algorithm to SAs in
 * SAPP_ServiceObj.
 *
 * Return Values:
 *   SAPP_OK    Successful.
 *   SAPP_ERR   Failed.
 */
vint _SAPP_ipsecSetEncAlgorithm(
    SAPP_ServiceObj *service_ptr,
    const char *alg_ptr)
{
    int i;

    for (i = 0; i <= OSAL_IPSEC_ENC_ALG_LAST; i++) {
        if (0 == OSAL_strcmp(alg_ptr, &_SAPP_ipsecEAlgStrings[i][0])) {
            service_ptr->ipsecObj.outboundSAc.algEsp = (OSAL_IpsecEncryptAlg)i;
            service_ptr->ipsecObj.outboundSAs.algEsp = (OSAL_IpsecEncryptAlg)i;
            service_ptr->ipsecObj.inboundSAc.algEsp = (OSAL_IpsecEncryptAlg)i;
            service_ptr->ipsecObj.inboundSAs.algEsp = (OSAL_IpsecEncryptAlg)i;
            return (SAPP_OK);
        }
    }

    return (SAPP_ERR);
}

/*
 * ======== SAPP_ipsecParseParamValue() ========
 * This function is used to search for parameters with the parameter
 *
 * Return Values:
 *  SAPP_OK: The value has been found and is pointed to by value_ptr.
 *  SAPP_ERR: The parameter specified in pName_ptr could not be found.
 *
 */
vint SAPP_ipsecParseParamValue(
    const char   *target_ptr,
    const char   *param_ptr,
    char        **value_ptr,
    vint         *valueLen_ptr)
{
    char *pos_ptr;
    char *end_ptr;

    if (NULL != (pos_ptr = OSAL_strscan(target_ptr, param_ptr))) {
        /*
         * In case wrong match for alg to ealg, last char should be ';' or ' '
         */
        if ((' ' != *(pos_ptr - 1)) && (';' != *(pos_ptr - 1))) {
            /* find again from next char */
            if (NULL == (pos_ptr = OSAL_strscan(pos_ptr + 1, param_ptr))) {
                return (SAPP_ERR);
            }
        }
        /* found it, now look for the 'value' */
        pos_ptr += OSAL_strlen(param_ptr);
        /* Remove any white space */
        while (*pos_ptr == ' ') {
            pos_ptr++;
        }
        *value_ptr = pos_ptr;
        /* Find ';' */
        if (NULL != (end_ptr = OSAL_strchr(pos_ptr, ';'))) {
            *valueLen_ptr = end_ptr - pos_ptr;
        }
        else {
            *valueLen_ptr = OSAL_strlen(pos_ptr);
        }

        return (SAPP_OK);
    }

    return (SAPP_ERR);
}

/*
 * ======== _SAPP_ipsecSetLocalAddress() ========
 * This function is to set the local address to the SA objects.
 *
 * Return Values:
 * SAPP_OK      Successfully set the local address.
 * SAPP_ERR     There is error setting the local address.
 */
vint _SAPP_ipsecSetLocalAddress(
    SAPP_ServiceObj *service_ptr,
    OSAL_NetAddress *addr_ptr)
{
    OSAL_IpsecSa *outSA_ptr;
    OSAL_IpsecSp *outSP_ptr;
    OSAL_IpsecSa *inSA_ptr;
    OSAL_IpsecSp *inSP_ptr;

    /* Set protected client port SAs */
    outSA_ptr = &service_ptr->ipsecObj.outboundSAc;
    outSP_ptr = &service_ptr->ipsecObj.outboundSPc;
    inSA_ptr = &service_ptr->ipsecObj.inboundSAc;
    inSP_ptr = &service_ptr->ipsecObj.inboundSPc;

    OSAL_netAddrCpy(&outSA_ptr->srcAddr, addr_ptr);
    OSAL_netAddrCpy(&outSP_ptr->srcAddr, addr_ptr);
    OSAL_netAddrCpy(&inSA_ptr->dstAddr, addr_ptr);
    OSAL_netAddrCpy(&inSP_ptr->dstAddr, addr_ptr);

    /* Set protected server port SAs */
    outSA_ptr = &service_ptr->ipsecObj.outboundSAs;
    outSP_ptr = &service_ptr->ipsecObj.outboundSPs;
    inSA_ptr = &service_ptr->ipsecObj.inboundSAs;
    inSP_ptr = &service_ptr->ipsecObj.inboundSPs;

    OSAL_netAddrCpy(&outSA_ptr->srcAddr, addr_ptr);
    OSAL_netAddrCpy(&outSP_ptr->srcAddr, addr_ptr);
    OSAL_netAddrCpy(&inSA_ptr->dstAddr, addr_ptr);
    OSAL_netAddrCpy(&inSP_ptr->dstAddr, addr_ptr);

    return (SAPP_OK);
}

/*
 * ======== _SAPP_ipsecSetRemoteAddress() ========
 * This function is to set the PCSCF's address to the SA objects.
 *
 * Return Values:
 * SAPP_OK      Successfully set the PCSCF's address.
 * SAPP_ERR     There is error setting the PCSCF's address.
 */
vint _SAPP_ipsecSetRemoteAddress(
    SAPP_ServiceObj *service_ptr)
{
    OSAL_IpsecSa       *outSA_ptr;
    OSAL_IpsecSp       *outSP_ptr;
    OSAL_IpsecSa       *inSA_ptr;
    OSAL_IpsecSp       *inSP_ptr;
    OSAL_NetAddress     addr;

    char *pos_ptr;
    char *end_ptr;
    char *char_ptr;
    char *pcscf_ptr;
    char  szPcscf[SIP_URI_STRING_MAX_SIZE];
    char  dnsBuf[512];

    /*
     * Parsing proxy or outbound proxy to get remote ip address.
     */
    if (0 !=  service_ptr->registration.pcscfList[
            service_ptr->registration.pcscfIndex][0]) {
        /* Outbound proxy is not empty, consider it as PCSCF */
        pcscf_ptr = &service_ptr->registration.pcscfList[
                service_ptr->registration.pcscfIndex][0];
    }
    else {
        /* Outbound proxy is empty, consider proxy as PCSCF */
        pcscf_ptr = service_ptr->sipConfig.config.szProxy;
    }

    /* Check if "sip:" is included in the proxy string */
    if (NULL != (pos_ptr = OSAL_strncasescan(pcscf_ptr, SIP_URI_STRING_MAX_SIZE,
            SAPP_SIP_SCHEME))) {
        /* Advance off "sip:" */
        pos_ptr += OSAL_strlen(SAPP_SIP_SCHEME);
    }
    else {
        pos_ptr = pcscf_ptr;
    }

    pcscf_ptr = pos_ptr;
    /* Get rid of the rest parameters */
    if ((NULL != OSAL_strscan(pcscf_ptr, "[")) &&
            (NULL != OSAL_strscan(pcscf_ptr, "]"))) {
        /* IPv6 */
        end_ptr  = OSAL_strscan(pcscf_ptr, "]");
        pos_ptr  = OSAL_strscan(pcscf_ptr, "[");
        /* Copy to local string for later process */
        OSAL_memCpy(szPcscf, (pos_ptr + 1), (end_ptr - pos_ptr -1));
        szPcscf[end_ptr - pos_ptr - 1] = 0;
    }
    else {
        /* Copy to local string for later process */
        OSAL_snprintf(szPcscf, SIP_URI_STRING_MAX_SIZE, "%s", pos_ptr);

        /* None IPv6 */
        OSAL_strtok(szPcscf, ";:");
    }

    /*
     * If interface is IPv6, need to query by OSAL_NET_RESOLVE_AAAA.
     * Otherwise, query by OSAL_NET_RESOLVE_A.
     */
    if ((OSAL_NET_SOCK_UDP_V6 == service_ptr->sipInfc.type) ||
            (OSAL_NET_SOCK_TCP_V6 == service_ptr->sipInfc.type)) {
        if (OSAL_SUCCESS != IMS_NET_RESOLVE((int8 *)szPcscf, (int8 *)dnsBuf, 
                sizeof(dnsBuf),
                2, 4, OSAL_NET_RESOLVE_AAAA)) {
            /* Cannot get remote address for ipsec */
            return (SAPP_ERR);
        }
    }
    else {
        if (OSAL_SUCCESS != IMS_NET_RESOLVE((int8 *)szPcscf, (int8 *)dnsBuf, 
                sizeof(dnsBuf),
                2, 4, OSAL_NET_RESOLVE_A)) {
            /* Cannot get remote address for ipsec */
            return (SAPP_ERR);
        }
    }

    pos_ptr = dnsBuf;
    /* Get the first answer */
    while (NULL != (char_ptr = OSAL_strscan(pos_ptr, "\n"))) {
        *char_ptr = 0;
        pos_ptr = char_ptr + 1;
    }

    /* Covert string to address, it's network byte order */
    if (OSAL_SUCCESS != OSAL_netStringToAddress((int8 *)dnsBuf, &addr)) {
        return (SAPP_ERR);
    }

    /* Set protected client port SAs */
    outSA_ptr = &service_ptr->ipsecObj.outboundSAc;
    outSP_ptr = &service_ptr->ipsecObj.outboundSPc;
    inSA_ptr = &service_ptr->ipsecObj.inboundSAc;
    inSP_ptr = &service_ptr->ipsecObj.inboundSPc;

    OSAL_netAddrCpy(&outSA_ptr->dstAddr, &addr);
    OSAL_netAddrCpy(&outSP_ptr->dstAddr, &addr);
    OSAL_netAddrCpy(&inSA_ptr->srcAddr, &addr);
    OSAL_netAddrCpy(&inSP_ptr->srcAddr, &addr);

    /* Set protected server port SAs */
    outSA_ptr = &service_ptr->ipsecObj.outboundSAs;
    outSP_ptr = &service_ptr->ipsecObj.outboundSPs;
    inSA_ptr = &service_ptr->ipsecObj.inboundSAs;
    inSP_ptr = &service_ptr->ipsecObj.inboundSPs;

    OSAL_netAddrCpy(&outSA_ptr->dstAddr, &addr);
    OSAL_netAddrCpy(&outSP_ptr->dstAddr, &addr);
    OSAL_netAddrCpy(&inSA_ptr->srcAddr, &addr);
    OSAL_netAddrCpy(&inSP_ptr->srcAddr, &addr);

    return (SAPP_OK);
}

/*
 * ======== _SAPP_ipsecParseSecSrvParam() ========
 * This function is to parse the parameters in Security-Server header field
 * and store the value of all parameters in the SA objects.
 *
 * Return Values:
 * SAPP_OK      Successfully parsed the the parameters in Security-Server 
 *                           header field
 * SAPP_ERR     There is error in parsing the parameters in Security-Server 
 *                           header field
 */
vint _SAPP_ipsecParseSecSrvParam(
    SAPP_ServiceObj *service_ptr, 
    char            *ss_ptr)
{
    char   tmpStr[SAPP_STRING_SZ];
    char  *value_ptr;
    vint   valueLen;
    uint16 port;

    /* Find if there is "3gpp-ipsec" machanism name */
    if (NULL == OSAL_strscan(ss_ptr, SAPP_IPSEC_3GPP_ARG)) {
        SAPP_dbgPrintf("%s %d: No machanism \"ipsec-3gpp\" exists in "
                "Security-Server.\n", __FILE__, __LINE__);
        return (SAPP_ERR);
    }

    /* Find "alg" value */
    if (SAPP_OK != SAPP_ipsecParseParamValue(ss_ptr, SAPP_ALG_ARG, 
            &value_ptr, &valueLen)) {
        SAPP_dbgPrintf("%s %d: No algorithim exists in Security-Server.\n",
                __FILE__, __LINE__);
        return (SAPP_ERR);
    }
    SAPP_parseCopy(tmpStr, sizeof(tmpStr), value_ptr, valueLen);

    if (SAPP_OK != _SAPP_ipsecSetAlgorithm(service_ptr, tmpStr)) {
        SAPP_dbgPrintf("%s %d: Set algorithm failed. alg=%s\n",
                __FILE__, __LINE__, tmpStr);
        return (SAPP_ERR);
    }

    /* Find "prot" value */ 
    if (SAPP_OK != SAPP_ipsecParseParamValue(ss_ptr, SAPP_PROT_ARG, 
            &value_ptr, &valueLen)) {
        /* prot default is esp */
        value_ptr = "esp";
        valueLen = OSAL_strlen(value_ptr);
    }
    SAPP_parseCopy(tmpStr, sizeof(tmpStr), value_ptr, valueLen);
    if (SAPP_OK != _SAPP_ipsecSetProtocol(service_ptr, tmpStr)) {
        SAPP_dbgPrintf("%s %d: Set protocol failed. prot=%s\n",
                __FILE__, __LINE__, tmpStr);
        return (SAPP_ERR);
    }

    /* Find "mod" value */
    if (SAPP_OK != SAPP_ipsecParseParamValue(ss_ptr, SAPP_MOD_ARG, 
            &value_ptr, &valueLen)) {
        /* mod default is trans */
        value_ptr = "trans";
        valueLen = OSAL_strlen(value_ptr);
    }
    SAPP_parseCopy(tmpStr, sizeof(tmpStr), value_ptr, valueLen);
    if (SAPP_OK != _SAPP_ipsecSetMode(service_ptr, tmpStr)) {
        SAPP_dbgPrintf("%s %d: Set mode failed. mod=%s\n",
                __FILE__, __LINE__, tmpStr);
        return (SAPP_ERR);
    }

    /* Find "ealg" value */
    if (SAPP_OK != SAPP_ipsecParseParamValue(ss_ptr, SAPP_EALG_ARG, 
            &value_ptr, &valueLen)) {
        SAPP_dbgPrintf("%s %d: No ealg exists in Security-Server.\n",
                __FILE__, __LINE__);
        /* If no ealg is given, consider it as "null" */
        OSAL_strcpy(tmpStr, "null");
    }
    else {
        SAPP_parseCopy(tmpStr, sizeof(tmpStr), value_ptr, valueLen);
    }

    if (SAPP_OK != _SAPP_ipsecSetEncAlgorithm(service_ptr, tmpStr)) {
        SAPP_dbgPrintf("%s %d: Set encrypt-algorithm failed. ealg=%s\n",
                __FILE__, __LINE__, tmpStr);
        return (SAPP_ERR);
    }

    /* Find "spi-c" value */
    if (SAPP_OK != SAPP_ipsecParseParamValue(ss_ptr, SAPP_SPI_C_ARG, 
            &value_ptr, &valueLen)) {
        SAPP_dbgPrintf("%s %d: No spi-c exists in Security-Server.\n",
                __FILE__, __LINE__);
        return (SAPP_ERR);
    }
    SAPP_parseCopy(tmpStr, sizeof(tmpStr), value_ptr, valueLen);

    /* Set spi-c. It's outbound SA on UE's protected server port */
    service_ptr->ipsecObj.outboundSAs.spi = OSAL_atoi(tmpStr);

    /* Find "spi-s" value */
    if (SAPP_OK != SAPP_ipsecParseParamValue(ss_ptr, SAPP_SPI_S_ARG, 
            &value_ptr, &valueLen)) {
        SAPP_dbgPrintf("%s %d: No spi-s exists in Security-Server.\n",
                __FILE__, __LINE__);
        return (SAPP_ERR);
    }
    SAPP_parseCopy(tmpStr, sizeof(tmpStr), value_ptr, valueLen);

    /* Set spi-s. I'ts outbound SA on UE's protected client port */
    service_ptr->ipsecObj.outboundSAc.spi = OSAL_atoi(tmpStr);

    /* Find "port-c" value */
    if (SAPP_OK != SAPP_ipsecParseParamValue(ss_ptr, SAPP_PORT_C_ARG, 
            &value_ptr, &valueLen)) {
        SAPP_dbgPrintf("%s %d: No port-c exists in Security-Server.\n",
                __FILE__, __LINE__);
        return (SAPP_ERR);
    }
    SAPP_parseCopy(tmpStr, sizeof(tmpStr), value_ptr, valueLen);

    port = OSAL_netHtons(OSAL_atoi(tmpStr));
    /* Set PCSCF's client port */
    service_ptr->ipsecObj.outboundSAs.dstAddr.port = port;
    service_ptr->ipsecObj.outboundSPs.dstAddr.port = port;
    service_ptr->ipsecObj.inboundSAs.srcAddr.port = port;
    service_ptr->ipsecObj.inboundSPs.srcAddr.port = port;

    /* Find "port-s" value */
    if (SAPP_OK != SAPP_ipsecParseParamValue(ss_ptr, SAPP_PORT_S_ARG, 
            &value_ptr, &valueLen)) {
        SAPP_dbgPrintf("%s %d: No port-c exists in Security-Server.\n",
                __FILE__, __LINE__);
        return (SAPP_ERR);
    }
    SAPP_parseCopy(tmpStr, sizeof(tmpStr), value_ptr, valueLen);
    
    port = OSAL_netHtons(OSAL_atoi(tmpStr));
    /* Set PCSCF's server port */
    service_ptr->ipsecObj.outboundSAc.dstAddr.port = port;
    service_ptr->ipsecObj.outboundSPc.dstAddr.port = port;
    service_ptr->ipsecObj.inboundSAc.srcAddr.port = port;
    service_ptr->ipsecObj.inboundSPc.srcAddr.port = port;

    return (SAPP_OK);
}


/*
 * ======== _SAPP_ipsecParseSecurityServer() ========
 * This function is to find the highest q-value and parse the Security-Server 
 * header field with the highest q-value. If it parse the Security-Server 
 * header field with the highest q-value fails, find the next higher q-value 
 * and parse the header field again.
 *
 * Return Values:
 * SAPP_OK      Successfully parsed the Security-Server header field
 * SAPP_ERR     There is error in parsing the Security-Server header field
 */
vint _SAPP_ipsecParseSecurityServer(
    SAPP_ServiceObj *service_ptr)
{
    int    i;
    char  *ss_ptr;
    char   tmpStr[SAPP_STRING_SZ];
    char  *value_ptr;
    vint   valueLen;
    int    ssHfIdx = -1;
    int    qValue = -1;
    int    tmpValue;
    char  *pos_ptr;

    /* Looking for the entry with highest q value. */
    for (i = 0; i < SAPP_SEC_SRV_HF_MAX_NUM; i++) {
        if ((service_ptr->secSrvHfs[i][0] != 0)) {
            ss_ptr = &service_ptr->secSrvHfs[i][0];
            /* Find the highest "q=" value. */
            if (SAPP_OK != SAPP_ipsecParseParamValue(ss_ptr, SAPP_Q_ARG, 
                    &value_ptr, &valueLen)) {
                /* 
                * If no q value is present, a value of 1 is assumed.
                * For comparing conveniently, append zero to q-value as 
                * five digits and thus the value become 10000.
                */
                tmpValue = 10000;
            }
            else {
                /*
                * For comparing conveniently, if there is '0' exist, use 
                * '0' to replace the '.'.
                */
                if (NULL != (pos_ptr = OSAL_strscan(value_ptr, "."))) {
                    pos_ptr[0] = '0';
                }
                SAPP_parseCopy(tmpStr, sizeof(tmpStr), value_ptr, valueLen);
                /* Recovery the original header field string. */
                pos_ptr[0] = '.';
                /*
                * For comparing conveniently, append zero to q-value as 
                * five digits.
                */
                for (i = 0; i < (SAPP_Q_VALUE_STRING_SZ - valueLen); i++) {
                    tmpStr[valueLen + i] = '0';
                }
                tmpStr[SAPP_Q_VALUE_STRING_SZ] = '\0';
                tmpValue = OSAL_atoi(tmpStr);
            }
            /*
            * Store the hightest q-value and idex of secSrvHfs 
            * currently.
            */
            if (tmpValue > qValue) {
                /* This entry is now the highest, check if we support it. */
                if (SAPP_OK == _SAPP_ipsecParseSecSrvParam(service_ptr,
                        ss_ptr)) {
                    /* We support it. Set to the highest supported one. */
                    qValue = tmpValue;
                    ssHfIdx = i;
                }
            }
        }
    }

    if (-1 == ssHfIdx) {
        SAPP_dbgPrintf("%s %d: No algorithim exists in Security-Server.\n",
            __FILE__, __LINE__);
        return (SAPP_ERR);
    }

    return (SAPP_OK);

}

/*
 * ======== _SAPP_ipsecBindSockets() ========
 * This function is to bind local sip port.
 *
 * Return Values:
 *   SAPP_OK    Successful.
 *   SAPP_ERR   Failed.
 */
vint _SAPP_ipsecBindSockets(
    SAPP_ServiceObj *service_ptr,
    uint16           port,
    OSAL_NetSockId  *fd_ptr)
{
    OSAL_NetAddress     addr;
    char                ipAddrStr[OSAL_NET_IPV6_STR_MAX];

    /* Set the type of address */
    if (eTransportUdp == service_ptr->sipTransportType) {
        if (OSAL_netIsAddrIpv6(&service_ptr->sipInfc)) {
            service_ptr->sipInfc.type = OSAL_NET_SOCK_UDP_V6;
        }
        else {
            service_ptr->sipInfc.type = OSAL_NET_SOCK_UDP;
        }
    }
    else if (eTransportTcp == service_ptr->sipTransportType) {
        if (OSAL_netIsAddrIpv6(&service_ptr->sipInfc)) {
            service_ptr->sipInfc.type = OSAL_NET_SOCK_TCP_V6;
        }
        else {
            service_ptr->sipInfc.type = OSAL_NET_SOCK_TCP;
        }
    }
    else if (eTransportTls == service_ptr->sipTransportType) {
        if (OSAL_netIsAddrIpv6(&service_ptr->sipInfc)) {
            service_ptr->sipInfc.type = OSAL_NET_SOCK_TCP_V6;
        }
        else {
            service_ptr->sipInfc.type = OSAL_NET_SOCK_TCP;
        }
    }
    else {
        /* Other transport type */
        SAPP_dbgPrintf("%s %d: Unsupported tranport type %d.\n",
                __FILE__, __LINE__, service_ptr->sipTransportType);
        return (SAPP_ERR);
    }

    if (OSAL_SUCCESS == OSAL_netIsSocketIdValid(fd_ptr)) {
        SAPP_dbgPrintf("%s %d: Close protected socket fd:%d\n",
            __FILE__, __LINE__, *fd_ptr);
        IMS_NET_CLOSE_SOCKET(fd_ptr);
        *fd_ptr = OSAL_NET_SOCK_INVALID_ID;
    }

    OSAL_netAddrHton(&addr, &service_ptr->sipInfc);
    addr.port = OSAL_netHtons(port);

    /* Create a socket for the SIP network interface */
    if (OSAL_SUCCESS != IMS_NET_SOCKET(fd_ptr, addr.type)) {
        SAPP_dbgPrintf("%s %d: Protected socket create failed.\n",
                __FILE__, __LINE__);
        return (SAPP_ERR);
    }

    /* Bind the port */
    OSAL_netAddressToString((int8 *)ipAddrStr, &addr);
    if (OSAL_SUCCESS != IMS_NET_BIND_SOCKET(fd_ptr, &addr)) {
        SAPP_dbgPrintf("%s %d: Bind protected socket failed. addr:%s,"
                "port:%d(network byte order).\n",
                __FILE__, __LINE__, ipAddrStr, OSAL_netNtohs(addr.port));
        IMS_NET_CLOSE_SOCKET(fd_ptr);
        *fd_ptr = OSAL_NET_SOCK_INVALID_ID;
        return (SAPP_ERR);
    }

    /* Get the port we bound */
    IMS_NET_GET_SOCKET_ADDRESS(fd_ptr, &addr);

    SAPP_dbgPrintf("%s %d: Bind protected socket done. fd:%d port:%d\n",
                __FILE__, __LINE__, *fd_ptr, OSAL_netNtohs(addr.port));
    return (SAPP_OK);

}

/* 
 * ======== _SAPP_ipsecInit() ========
 * 
 * This function is to initialize four SAs(as below figure).
 *
 *
 *       port-uc | ---------outboundSAc-------->| port-ps
 *               | <--------inboundSAc----------| 
 *               |                              |
 *               |                              |
 *       port-us | ---------outboundSAs-------->| port-pc
 *               | <--------inboundSAs----------| 
 *               |                              |
 *
 * Returns: 
 *  SAPP_ERR: Ipsec initialize failed.
 *  SAPP_OK: Ipsec initialized successfully.
 */
vint _SAPP_ipsecInit(
    SAPP_ServiceObj *service_ptr,
    SAPP_SipObj     *sip_ptr)
{
    OSAL_IpsecSa *outSA_ptr;
    OSAL_IpsecSp *outSP_ptr;
    OSAL_IpsecSa *inSA_ptr;
    OSAL_IpsecSp *inSP_ptr;
    uint16    portUC;
    uint16    portUS;
    OSAL_NetAddress addr;
    uint16    portTmp;
    tLocalIpConn    conn;

    /* Get next UE client port and UE server port */
    service_ptr->ipsecObj.pPortUS = _SAPP_ipsecGetProtectedPort(service_ptr);
    service_ptr->ipsecObj.pPortUC = _SAPP_ipsecGetProtectedPort(service_ptr);

    /* UE client and server create and bind socket*/
    if (SAPP_OK != _SAPP_ipsecBindSockets(service_ptr,
            service_ptr->ipsecObj.pPortUS,
            &service_ptr->sipInfcProtectedServerFd)) {
        return (SAPP_ERR);
    }
    if (SAPP_OK != _SAPP_ipsecBindSockets(service_ptr,
            service_ptr->ipsecObj.pPortUC,
            &service_ptr->sipInfcProtectedClientFd)) {
        return (SAPP_ERR);
    }

    /*
     * If mtu is set to  a value, create & bind TCP socket.
     */
    if (sip_ptr->mtu &&
        eTransportUdp == service_ptr->sipTransportType) {
        service_ptr->sipTransportType = eTransportTcp;
        if (SAPP_OK != _SAPP_ipsecBindSockets(service_ptr,
                service_ptr->ipsecObj.pPortUS,
                &service_ptr->sipInfcProtectedTcpServerFd)) {
            return (SAPP_ERR);
        }
        if (SAPP_OK != _SAPP_ipsecBindSockets(service_ptr,
                service_ptr->ipsecObj.pPortUC,
                &service_ptr->sipInfcProtectedTcpClientFd)) {
            return (SAPP_ERR);
        }
        portTmp = service_ptr->sipInfc.port;
        service_ptr->sipInfc.port = service_ptr->ipsecObj.pPortUC;

        /* Convert to host byte order */
        OSAL_netAddrPortNtoh(&conn.addr, &service_ptr->sipInfc);
        conn.fd = service_ptr->sipInfcProtectedTcpClientFd;
        if (SIP_FAILED == SR_clientConnect(
                service_ptr->sipConfig.uaId, &conn, eTransportTcp)) {
                return (SAPP_ERR);
        }

        /* Revert to original transport type and port */
        service_ptr->sipInfc.port = portTmp;
        service_ptr->sipTransportType = eTransportUdp; 
        if (OSAL_netIsAddrIpv6(&service_ptr->sipInfc)) {
            service_ptr->sipInfc.type = OSAL_NET_SOCK_UDP_V6;
        }
        else {
            service_ptr->sipInfc.type = OSAL_NET_SOCK_UDP;
        }        
    }

    portUS = OSAL_netHtons(service_ptr->ipsecObj.pPortUS);
    portUC = OSAL_netHtons(service_ptr->ipsecObj.pPortUC);

    /* Set outbound SA on protected client port SAs */
    outSA_ptr = &service_ptr->ipsecObj.outboundSAc;
    outSP_ptr = &service_ptr->ipsecObj.outboundSPc;

    /* Set default outbound SA parameters */
    outSA_ptr->protocol = _SAPP_IPSEC_DEFAULT_PROTOCOL;
    outSA_ptr->algAh    = _SAPP_IPSEC_DEFAULT_AUTH_ALG;
    outSA_ptr->algEsp   = _SAPP_IPSEC_DEFAULT_ENC_ALG;
    outSA_ptr->mode     = _SAPP_IPSEC_DEFAULT_MODE;
    outSA_ptr->reqId    = 1;
    outSA_ptr->srcAddr.port = portUC;
    /* Set default outbound SP parameters */
    outSP_ptr->protocol = _SAPP_IPSEC_DEFAULT_PROTOCOL;
    outSP_ptr->mode     = _SAPP_IPSEC_DEFAULT_MODE;
    outSP_ptr->dir      = OSAL_IPSEC_DIR_OUT;
    outSP_ptr->level    = OSAL_IPSEC_SP_LEVEL_UNIQUE;
    outSP_ptr->reqId    = 1;
    outSP_ptr->srcAddr.port = portUC;

    /* Set inbound SA on protected client port SAs */
    inSA_ptr = &service_ptr->ipsecObj.inboundSAc;
    inSP_ptr = &service_ptr->ipsecObj.inboundSPc;

    /* Set default inbound SA parameters */
    inSA_ptr->protocol = _SAPP_IPSEC_DEFAULT_PROTOCOL;
    inSA_ptr->algAh    = _SAPP_IPSEC_DEFAULT_AUTH_ALG;
    inSA_ptr->algEsp   = _SAPP_IPSEC_DEFAULT_ENC_ALG;
    inSA_ptr->mode     = _SAPP_IPSEC_DEFAULT_MODE; 
    inSA_ptr->reqId    = 0;
    inSA_ptr->spi      = _SAPP_ipsecGetSpi(service_ptr);
    inSA_ptr->dstAddr.port = portUC;

    /* Set default inbound SP parameters */
    inSP_ptr->protocol = _SAPP_IPSEC_DEFAULT_PROTOCOL;
    inSP_ptr->mode     = _SAPP_IPSEC_DEFAULT_MODE;
    inSP_ptr->dir      = OSAL_IPSEC_DIR_IN;
    inSP_ptr->level    = OSAL_IPSEC_SP_LEVEL_REQUIRE;
    inSP_ptr->dstAddr.port = portUC;

    /* Set outbound on protected server port SAs */
    outSA_ptr = &service_ptr->ipsecObj.outboundSAs;
    outSP_ptr = &service_ptr->ipsecObj.outboundSPs;

    /* Set default outbound SA parameters */
    outSA_ptr->protocol = _SAPP_IPSEC_DEFAULT_PROTOCOL;
    outSA_ptr->algAh    = _SAPP_IPSEC_DEFAULT_AUTH_ALG;
    outSA_ptr->algEsp   = _SAPP_IPSEC_DEFAULT_ENC_ALG; 
    outSA_ptr->mode     = _SAPP_IPSEC_DEFAULT_MODE;
    outSA_ptr->reqId    = 2;
    outSA_ptr->srcAddr.port = portUS;

    /* Set default outbound SP parameters */
    outSP_ptr->protocol = _SAPP_IPSEC_DEFAULT_PROTOCOL;
    outSP_ptr->mode     = _SAPP_IPSEC_DEFAULT_MODE;
    outSP_ptr->dir      = OSAL_IPSEC_DIR_OUT;
    outSP_ptr->level    = OSAL_IPSEC_SP_LEVEL_UNIQUE;
    outSP_ptr->reqId    = 2;
    outSP_ptr->srcAddr.port = portUS;

    /* Set inbound on protected server port SAs */
    inSA_ptr = &service_ptr->ipsecObj.inboundSAs;
    inSP_ptr = &service_ptr->ipsecObj.inboundSPs;
    /* Set default inbound SA parameters */
    inSA_ptr->protocol = _SAPP_IPSEC_DEFAULT_PROTOCOL;
    inSA_ptr->algAh    = _SAPP_IPSEC_DEFAULT_AUTH_ALG;
    inSA_ptr->algEsp   = _SAPP_IPSEC_DEFAULT_ENC_ALG;
    inSA_ptr->mode     = _SAPP_IPSEC_DEFAULT_MODE;
    inSA_ptr->spi      = _SAPP_ipsecGetSpi(service_ptr);
    inSA_ptr->reqId    = 0;
    inSA_ptr->dstAddr.port = portUS;

    /* Set default inbound SP parameters */
    inSP_ptr->protocol = _SAPP_IPSEC_DEFAULT_PROTOCOL;
    inSP_ptr->mode     = _SAPP_IPSEC_DEFAULT_MODE;
    inSP_ptr->dir      = OSAL_IPSEC_DIR_IN;
    inSP_ptr->level    = OSAL_IPSEC_SP_LEVEL_REQUIRE;
    inSP_ptr->dstAddr.port = portUS;

    /* Convert to OSAL_NetAddress */
    OSAL_netAddrHton(&addr, &service_ptr->sipConfig.localConn.addr);
    /* Set local ip address */
    if (SAPP_OK != _SAPP_ipsecSetLocalAddress(service_ptr, &addr)) {
        SAPP_dbgPrintf("%s %d:Cannot set local address for ipsec\n",
                __FILE__, __LINE__);
        return (SAPP_ERR);
    }


    /* Set remote ip address */
    if (SAPP_OK != _SAPP_ipsecSetRemoteAddress(service_ptr)) {
        SAPP_dbgPrintf("%s %d:Cannot get remote address for ipsec\n",
                __FILE__, __LINE__);
        return (SAPP_ERR);
    }

    return (SAPP_OK);
}

/*
 * ======== _SAPP_ipsecPrintSA() ========
 * Helper function to print SA information.
 *
 * Return Values:
 *  None.
 */
void _SAPP_ipsecPrintSA(
    OSAL_IpsecSa *sa_ptr) 
{

    char srcAddrStr[SAPP_STRING_SZ];
    char dstAddrStr[SAPP_STRING_SZ];

    OSAL_netAddressToString((int8 *)srcAddrStr, &sa_ptr->srcAddr);
    OSAL_netAddressToString((int8 *)dstAddrStr, &sa_ptr->dstAddr);
    SAPP_dbgPrintf("---Ipsec SA:\n"
            "---src addr:%s, port:%d\n"
            "---dst addr:%s, port:%d\n"
            "---esp alg:%s, esp key:%s\n"
            "---ah alg:%s, ah key:%s\n"
            "---spi:%u\n"
            "---reqId:%d\n",
            srcAddrStr, OSAL_netNtohs(sa_ptr->srcAddr.port),
            dstAddrStr, OSAL_netNtohs(sa_ptr->dstAddr.port),
            _SAPP_ipsecEAlgStrings[sa_ptr->algEsp],
            sa_ptr->keyEsp,
            _SAPP_ipsecAlgStrings[sa_ptr->algAh],
            sa_ptr->keyAh,
            sa_ptr->spi,
            sa_ptr->reqId);

}

/*
 * ======== _SAPP_ipsecCreateSAs() ========
 * This function is to create the IPSEC SAs between UE and PCSCF
 *
 * Return Values:
 * SAPP_OK      Create SAs successfully
 * SAPP_ERR     Create SAs failed
 */
vint _SAPP_ipsecCreateSAs(
    SAPP_ServiceObj *service_ptr,
    SAPP_Event      *evt_ptr)
{

    SAPP_dbgPrintf("%s %d: Creating SAs...\n",
                __FILE__, __LINE__);

    /*
     * TS 33.203
     * UDP case: the UE receives requests and responses protected with ESP
     * on the port port_us (the"protected server port"). The UE sends requests
     *  and responses protected with ESP on the port port_uc (the "protected
     * client port").
     */

    /* Create inbound SA on protected server port */
    _SAPP_ipsecPrintSA(&service_ptr->ipsecObj.inboundSAs);
    if (OSAL_SUCCESS != OSAL_ipsecCreateSA(&service_ptr->ipsecObj.inboundSAs,
            &service_ptr->ipsecObj.inboundSPs)) {
        SAPP_dbgPrintf("%s %d: Create SA failed.\n",
                __FILE__, __LINE__);
        _SAPP_ipsecClearSAs(service_ptr, evt_ptr);
        return (SAPP_ERR);
    }

    /* Create outbound SA on protected client port */
    _SAPP_ipsecPrintSA(&service_ptr->ipsecObj.outboundSAc);
    if (OSAL_SUCCESS != OSAL_ipsecCreateSA(&service_ptr->ipsecObj.outboundSAc,
            &service_ptr->ipsecObj.outboundSPc)) {
        SAPP_dbgPrintf("%s %d: Create SA failed.\n",
                __FILE__, __LINE__);
        _SAPP_ipsecClearSAs(service_ptr, evt_ptr);
        return (SAPP_ERR);
    }

    /* Create inbound SA on protected client port */
    _SAPP_ipsecPrintSA(&service_ptr->ipsecObj.inboundSAc);
    if (OSAL_SUCCESS != OSAL_ipsecCreateSA(&service_ptr->ipsecObj.inboundSAc,
            &service_ptr->ipsecObj.inboundSPc)) {
        SAPP_dbgPrintf("%s %d: Create SA failed.\n",
                __FILE__, __LINE__);
        _SAPP_ipsecClearSAs(service_ptr, evt_ptr);
        return (SAPP_ERR);
    }

    /* Create outbound SA on protected server port */
    _SAPP_ipsecPrintSA(&service_ptr->ipsecObj.outboundSAs);
    if (OSAL_SUCCESS != OSAL_ipsecCreateSA(&service_ptr->ipsecObj.outboundSAs,
            &service_ptr->ipsecObj.outboundSPs)) {
        SAPP_dbgPrintf("%s %d: Create SA failed.\n",
                __FILE__, __LINE__);
        _SAPP_ipsecClearSAs(service_ptr, evt_ptr);
        return (SAPP_ERR);
    }

    /*
     * IPSec is enable, after create IPSec SAs successfully.
     * And sending event, IPSec information, to notify ISI
     * IPSec has been created.
     */
    service_ptr->ipsecObj.isEnable = OSAL_TRUE;
    _SAPP_ipsecSendIsiEvt(service_ptr, evt_ptr, ISIP_STATUS_DONE);

    return (SAPP_OK);
}

/*
 * ======== _SAPP_ipsecDeleteSAs() ========
 * This function is to delete all the IPSEC SAs
 *
 * Return Values:
 * SAPP_OK      Clear SAs successfully
 * SAPP_ERR     Clear SAs failed
 */
vint _SAPP_ipsecDeleteSAs(
    SAPP_ServiceObj *service_ptr,
    SAPP_Event      *evt_ptr)
{
    if (0 == service_ptr->sipConfig.useIpSec) {
        return (SAPP_OK);
    }

    if (OSAL_TRUE != service_ptr->ipsecObj.isEnable) {
        /* No Ipsec created */
        return (SAPP_OK);
    }

    SAPP_dbgPrintf("%s %d: Deleting SAs...\n",
                __FILE__, __LINE__);

    OSAL_ipsecDeleteSA(&service_ptr->ipsecObj.inboundSAs,
            &service_ptr->ipsecObj.inboundSPs);

    OSAL_ipsecDeleteSA(&service_ptr->ipsecObj.outboundSAc,
            &service_ptr->ipsecObj.outboundSPc);

    OSAL_ipsecDeleteSA(&service_ptr->ipsecObj.inboundSAc,
            &service_ptr->ipsecObj.inboundSPc);

    OSAL_ipsecDeleteSA(&service_ptr->ipsecObj.outboundSAs,
            &service_ptr->ipsecObj.outboundSPs);

    /*
     * If IPSec is enable, set the flag as disable after delete SAs.
     * And sending event, IPSec information, to notify ISI
     * IPSec has been deleted.
     */
    service_ptr->ipsecObj.isEnable = OSAL_FALSE;
    _SAPP_ipsecSendIsiEvt(service_ptr, evt_ptr,
            ISIP_STATUS_INVALID);

    return (SAPP_OK);
}

/*
 * ======== _SAPP_ipsecClearSAs() ========
 * This function is to delete all the IPSEC SAs and clear related memory
 *
 * Return Values:
 * SAPP_OK      Clear SAs successfully
 * SAPP_ERR     Clear SAs failed
 */
vint _SAPP_ipsecClearSAs(
    SAPP_ServiceObj *service_ptr,
    SAPP_Event      *evt_ptr)
{
    if (0 == service_ptr->sipConfig.useIpSec) {
        return (SAPP_OK);
    }

    /* Delete SAs */
    _SAPP_ipsecDeleteSAs(service_ptr, evt_ptr);

    OSAL_memSet(&service_ptr->ipsecObj.inboundSAc, 0, sizeof(OSAL_IpsecSa));
    OSAL_memSet(&service_ptr->ipsecObj.inboundSAs, 0, sizeof(OSAL_IpsecSa));
    OSAL_memSet(&service_ptr->ipsecObj.outboundSAc, 0, sizeof(OSAL_IpsecSa));
    OSAL_memSet(&service_ptr->ipsecObj.outboundSAs, 0, sizeof(OSAL_IpsecSa));
    OSAL_memSet(&service_ptr->ipsecObj.inboundSPc, 0, sizeof(OSAL_IpsecSp));
    OSAL_memSet(&service_ptr->ipsecObj.inboundSPs, 0, sizeof(OSAL_IpsecSp));
    OSAL_memSet(&service_ptr->ipsecObj.outboundSPc, 0, sizeof(OSAL_IpsecSp));
    OSAL_memSet(&service_ptr->ipsecObj.outboundSPs, 0, sizeof(OSAL_IpsecSp));

    return (SAPP_OK);
}

/*
 * ======== _SAPP_ipsecDumpKey() ========
 * Helper function to dump key.
 *
 * Return Values:
 *  None.
 */
void _SAPP_ipsecDumpKey(
    char  *name_ptr,
    uint8 *k_ptr,
    vint   len)
{
    int i;

    SAPP_dbgPrintf("%s:\t", name_ptr);
    for (i = 0; i < len; i++ ) {
        SAPP_dbgPrintf("%2x ", k_ptr[i]);
    }
    SAPP_dbgPrintf("\n");
}

/*
 * ======== _SAPP_ipsecSetKey() ========
 * This function is to set ck and ik to SAs.
 *
 * Return Values:
 * SAPP_OK      Set key successfully
 * SAPP_ERR     Set key failed
 */
vint _SAPP_ipsecSetKey(
    SAPP_ServiceObj *service_ptr,
    const uint8     *ik_ptr,
    vint             ikLen,
    const uint8     *ck_ptr,
    vint             ckLen)
{
    char key[OSAL_IPSEC_KEY_LENGTH_MAX];
    char keyLen;
    char cm1[_SAPP_IPSEC_CK_SUB_SZ + 1], cm2[_SAPP_IPSEC_CK_SUB_SZ + 1];

    /* Verify key length */
    if (_SAPP_IPSEC_IK_SZ != ikLen || _SAPP_IPSEC_CK_SZ != ckLen) {
        SAPP_dbgPrintf("%s %d: Key length is not correct.\n",
                __FILE__, __LINE__);
        return (SAPP_ERR);
    }

    OSAL_memSet(key, 0, OSAL_IPSEC_KEY_LENGTH_MAX);
    keyLen = 0;
    /*
     * Refer to TS 33.203 Annex I for key expansion.
     */
    switch (service_ptr->ipsecObj.inboundSAc.algAh) {
        case OSAL_IPSEC_AUTH_ALG_HMAC_MD5_96:
            keyLen = OSAL_IPSEC_KEY_LENGTH_HMAC_MD5 + 1;
            OSAL_memCpy(key, ik_ptr, ikLen);
            break;
        case OSAL_IPSEC_AUTH_ALG_HMAC_SHA1_96:
            /* Appends 32 zero bits to create 160-bit string */
            keyLen = OSAL_IPSEC_KEY_LENGTH_HMAC_SHA1 + 1;
            /* Leave last 4 bytes as zero. */
            OSAL_memCpy(key, ik_ptr, ikLen);
            break;
        case OSAL_IPSEC_AUTH_ALG_NULL:
        default:
            keyLen = 0;
            break;
    }

    OSAL_memCpy(service_ptr->ipsecObj.inboundSAc.keyAh, key, keyLen);
    OSAL_memCpy(service_ptr->ipsecObj.inboundSAs.keyAh, key, keyLen);
    OSAL_memCpy(service_ptr->ipsecObj.outboundSAc.keyAh, key, keyLen);
    OSAL_memCpy(service_ptr->ipsecObj.outboundSAs.keyAh, key, keyLen);

    OSAL_memSet(key, 0, OSAL_IPSEC_KEY_LENGTH_MAX);
    keyLen = 0;

    switch (service_ptr->ipsecObj.inboundSAc.algEsp) {
        case OSAL_IPSEC_ENC_ALG_DES_EDE3_CBC:
            /*
             * Divide CKim into two 64-bit blocks.
             * CKim = CKim1 || CKim2
             */
            OSAL_memCpy(cm1, ck_ptr, _SAPP_IPSEC_CK_SUB_SZ);
            OSAL_memCpy(cm2, ck_ptr + _SAPP_IPSEC_CK_SUB_SZ, _SAPP_IPSEC_CK_SUB_SZ);

            /* CKesp = CKim1 || CKim2 || CKim1 */
            keyLen = OSAL_IPSEC_KEY_LENGTH_3DES_CBC + 1;
            OSAL_memCpy(key, cm1, _SAPP_IPSEC_CK_SUB_SZ);
            OSAL_memCpy(key + _SAPP_IPSEC_CK_SUB_SZ, cm2, _SAPP_IPSEC_CK_SUB_SZ);
            OSAL_memCpy(key + 2 * _SAPP_IPSEC_CK_SUB_SZ, cm1, _SAPP_IPSEC_CK_SUB_SZ);
            break; 
        case OSAL_IPSEC_ENC_ALG_AES_CBC:
            keyLen = OSAL_IPSEC_KEY_LENGTH_AES_CBC + 1;
            OSAL_memCpy(key, ck_ptr, keyLen);
            break; 
        case OSAL_IPSEC_ENC_ALG_NULL:
        default:
            keyLen = 0;
            break;
    }

    OSAL_memCpy(service_ptr->ipsecObj.inboundSAc.keyEsp, key, keyLen);
    OSAL_memCpy(service_ptr->ipsecObj.inboundSAs.keyEsp, key, keyLen);
    OSAL_memCpy(service_ptr->ipsecObj.outboundSAc.keyEsp, key, keyLen);
    OSAL_memCpy(service_ptr->ipsecObj.outboundSAs.keyEsp, key, keyLen);
    
    return (SAPP_OK);
}

/*
 * ======== _SAPP_ipsecClientSocketDestroy() ========
 * This function is to destroy client sockets.
 * It closes the client socket and set it to zero.
 *
 * Return Values:
 *   SAPP_OK    Successful.
 *   SAPP_ERR   Failed.
 */
vint _SAPP_ipsecClientSocketDestroy(
    SAPP_ServiceObj *service_ptr)
{
    OSAL_NetSockId   *fd_ptr;

    fd_ptr = &service_ptr->sipInfcProtectedClientFd;

    /* If old fd exists, close it */
    if (OSAL_SUCCESS == OSAL_netIsSocketIdValid(fd_ptr)) {
        SAPP_dbgPrintf("%s %d: Close protected client socket fd:%d\n",
            __FILE__, __LINE__, *fd_ptr);
        IMS_NET_CLOSE_SOCKET(fd_ptr);
        *fd_ptr = OSAL_NET_SOCK_INVALID_ID;
    }

    fd_ptr = &service_ptr->sipInfcProtectedTcpClientFd;

    /* If old fd exists, close it */
    if (OSAL_SUCCESS == OSAL_netIsSocketIdValid(fd_ptr)) {
        SAPP_dbgPrintf("%s %d: Close protected client socket fd:%d\n",
            __FILE__, __LINE__, *fd_ptr);
        IMS_NET_CLOSE_SOCKET(fd_ptr);
        *fd_ptr = OSAL_NET_SOCK_INVALID_ID;
    }

    return (SAPP_OK);
}

/*
 * ======== _SAPP_ipsecReplaceServerSocket() ========
 * This function is to replace sip server socket.
 * Local sip server port might change when using ipsec, this function is
 * to update local sip server port when it changed.
 *
 * Return Values:
 *   SAPP_OK    Successful.
 *   SAPP_ERR   Failed.
 */
vint _SAPP_ipsecReplaceServerSocket(
    SAPP_ServiceObj *service_ptr,
    OSAL_NetSockId  *fd_ptr,
    uint16           portUS)
{
    OSAL_NetAddress   addr;

    /* Create new one if server port is given */
    if (0 == portUS) {
        return (SAPP_OK);
    }

    if (eTransportTcp == service_ptr->sipTransportType) {
        if (OSAL_SUCCESS != IMS_NET_LISTEN_ON_SOCKET(fd_ptr)) {
            SAPP_dbgPrintf("%s %d: Listen socket failed. fd:%d\n",
                    __FILE__, __LINE__, *fd_ptr);
            IMS_NET_CLOSE_SOCKET(fd_ptr);
            return (SAPP_ERR);
        }
    }

    SAPP_dbgPrintf("%s %d: Bind/Listen ipsec server socket done. fd:%d port:%d\n",
            __FILE__, __LINE__, *fd_ptr, portUS);

    OSAL_netAddrCpy(&addr, &service_ptr->sipInfc);
    addr.port = portUS;
    /* Replace sip server socket */
    if (SIP_OK != SIP_replaceServerSocket(
            *fd_ptr, &addr, service_ptr->sipTransportType,
            &service_ptr->sipConfig.localConn.nwAccess)) {
        SAPP_dbgPrintf("%s %d: _SAPP_ipsecReplaceServerSocket. Replace sip "
                "server socket failed\n", __FILE__, __LINE__);
        return (SAPP_ERR);
    }

    return (SAPP_OK);

}

/*
 * ======== _SAPP_ipsecSetLocalPort() ========
 * This function is to set local sip port to sip configuration.
 * Local sip might change when using ipsec, this function is
 * to update local sip port when it changed.
 *
 * Return Values:
 * SAPP_OK      Set proxy port successfully
 * SAPP_ERR     Set proxy port failed
 */
vint _SAPP_ipsecSetLocalPort(
    SAPP_ServiceObj *service_ptr,
    SAPP_SipObj     *sip_ptr,
    uint16           portUC,
    uint16           portUS)
{
    OSAL_NetSockId   *fd_ptr;
    OSAL_NetAddress   addr;
    SAPP_RegObj      *reg_ptr;

    SAPP_dbgPrintf("%s %d: _SAPP_ipsecSetLocalPort port_uc:%d port_us:%d "
            "transport type:%d\n",
            __FILE__, __LINE__, portUC, portUS, service_ptr->sipInfc.type);

    if (SAPP_OK != _SAPP_ipsecReplaceServerSocket(service_ptr,
        &service_ptr->sipInfcProtectedServerFd, portUS)) {
        SAPP_dbgPrintf("%s %d: _SAPP_ipsecSetLocalPort. Rplace server socket "
                "failed\n", __FILE__, __LINE__);
        return (SAPP_ERR);
    }
    if (0 < service_ptr->sipInfcProtectedServerFd) {
        if (SAPP_OK != _SAPP_ipsecReplaceServerSocket(service_ptr,
            &service_ptr->sipInfcProtectedServerFd, portUS)) {
            SAPP_dbgPrintf("%s %d: _SAPP_ipsecSetLocalPort. Rplace TCP "
                    "server socket failed\n", __FILE__, __LINE__);
            return (SAPP_ERR);
        }
    }

    service_ptr->sipInfc.port = portUC;

    OSAL_netAddrPortCpy(&addr, &service_ptr->sipInfc);
    /*
     * If portUS euqals 0, it means set to default port then close the socket
     * if exists.
     */
    if (0 != portUS) {
        /*
         * From TS 33.203 the port in the Contact and Via is the protected UE
         * server port, that means it will be different from the port we send
         * REGISTER(UE client port).
         * So we have to create the socket in sapp and put different port
         * number in localConn of registration.
         */

        /* Convert to network byte order */
        OSAL_netAddrPortHton(&addr, &service_ptr->sipInfc);

        fd_ptr = &service_ptr->sipInfcProtectedClientFd;
    }
    else {
        /* Set the default port */
        fd_ptr = &service_ptr->sipInfcFd;
    }

    OSAL_netAddrPortCpy(&service_ptr->sipConfig.localConn.addr,
            &service_ptr->sipInfc);

    service_ptr->sipConfig.localConn.fd = *fd_ptr;

    /* Set local connection in regObj */
    reg_ptr = &service_ptr->registration;
    reg_ptr->lclConn = service_ptr->sipConfig.localConn;
    if (0 != portUS) {
        /* Set the registration's localConn's port to server port */
        reg_ptr->lclConn.addr.port = portUS;
        /* Set the sip's localConn's port to server port */
        service_ptr->sipConfig.localConn.addr.port = portUS;
    }

    return (SAPP_OK);
}

/*
 * ======== _SAPP_ipsecSetProxyPort() ========
 * This function is to set proxy's sip port to sip configuration.
 * Proxy's port might change when using ipsec, this function is
 * to update proxy's sip port when it changed.
 *
 * Return Values:
 * SAPP_OK      Set proxy port successfully
 * SAPP_ERR     Set proxy port failed
 */
vint _SAPP_ipsecSetProxyPort(
    SAPP_ServiceObj *service_ptr,
    SAPP_SipObj     *sip_ptr,
    uint16           port)
{
    char             *pcscf_ptr;
    char             *pos_ptr;
    char              tmpStr[SIP_URI_STRING_MAX_SIZE];
    char              param[SIP_URI_STRING_MAX_SIZE];

    /* Update PCSCF's URI */
    if (0 !=  service_ptr->registration.pcscfList[
            service_ptr->registration.pcscfIndex][0]) {
        /* Outbound proxy is not empty, consider it as PCSCF */
        pcscf_ptr = &service_ptr->registration.pcscfList[
                service_ptr->registration.pcscfIndex][0];
    }
    else {
        /* Outbound proxy is empty, consider proxy as PCSCF */
        pcscf_ptr = service_ptr->sipConfig.config.szProxy;
    }
 
    param[0] = '\0';
    /* Find the parameters */
    if (NULL != (pos_ptr = OSAL_strchr(pcscf_ptr, ';'))) {
        /* Copy the parameters */
        OSAL_strcpy(param, pos_ptr + 1);
        *pos_ptr = '\0';
    }

    /* Find the port */
    if (NULL == (pos_ptr = OSAL_strchr(pcscf_ptr, ']'))) {
        /* PCSCF is IPv4 or domain name */
        if (NULL != (pos_ptr = OSAL_strchr(pcscf_ptr, ':'))) {
            /* Remove the port */
            *pos_ptr = '\0';
        }
    }
    else {
        /* PCSCF is IPv6 */
        *(pos_ptr + 1) = '\0';
    }

    if (0 != port) {
        OSAL_snprintf(tmpStr, SIP_URI_STRING_MAX_SIZE, "%s:%d;%s",
                pcscf_ptr, port, param);
    }
    else {
        /* Don't put the port if the port is not assigned */
        OSAL_snprintf(tmpStr, SIP_URI_STRING_MAX_SIZE, "%s;%s",
                pcscf_ptr, param);
    }

    OSAL_strcpy(pcscf_ptr, tmpStr);

    if (service_ptr->sipConfig.uaId != 0) {
        /* Then UA is currently active so call UA_Modify() */
        UA_Modify(service_ptr->sipConfig.uaId,
                service_ptr->sipConfig.config.szProxy,
                &service_ptr->registration.pcscfList[
                service_ptr->registration.pcscfIndex][0], NULL,
                NULL, NULL, NULL, NULL, NULL, 20, 0);
    }

    return (SAPP_OK);
}

/*
 * ======== _SAPP_ipsecSetProtectedPort() ========
 * This function is to set protected port to
 * local sip infc and prxoy or outbound proxy string. 
 * This function also set ipsec keys and create ipsec SAs.
 *
 * Return Values:
 * SAPP_OK      Set default port successfully
 * SAPP_ERR     Set default port failed
 */
vint _SAPP_ipsecSetProtectedPort(
    SAPP_ServiceObj *service_ptr,
    SAPP_SipObj     *sip_ptr)
{
    
    /* If useIpSec is not used or current port is protect port then no need to change port */
    if ((0 == service_ptr->sipConfig.useIpSec)) {
        return (SAPP_OK);
    }

    /* Always delete old SAs if exists before create new ones */
    _SAPP_ipsecDeleteSAs(service_ptr, &sip_ptr->event);

    /* Set ipsec keys */
    if (SAPP_OK != _SAPP_ipsecSetKey(service_ptr,
            service_ptr->akaAuthIk, SAPP_AKA_AUTH_CK_SZ,
            service_ptr->akaAuthCk, SAPP_AKA_AUTH_IK_SZ)) {
        /* Set ipsec key failed so do go thru protected port */
        SAPP_dbgPrintf("%s %d: Set ipsec keys failed.\n",
                __FILE__, __LINE__);
        return (SAPP_ERR);
    }

    /* Update local sip port */
    if (SAPP_OK != _SAPP_ipsecSetLocalPort(service_ptr, sip_ptr,
            service_ptr->ipsecObj.pPortUC, service_ptr->ipsecObj.pPortUS)) {
        SAPP_dbgPrintf("%s %d: Update local port failed.\n",
                __FILE__, __LINE__);        
        return (SAPP_ERR);
    }

    /* Update PCSCF's port */
    if (SAPP_OK != _SAPP_ipsecSetProxyPort(service_ptr, sip_ptr,
            OSAL_netNtohs(service_ptr->ipsecObj.outboundSAc.dstAddr.port))) {
        SAPP_dbgPrintf("%s %d: Update PCSCF's port failed.\n",
                __FILE__, __LINE__);
        return (SAPP_ERR);
    }

    /* Create IPSEC SAs between UE and PCSCF */
    if (SAPP_OK != _SAPP_ipsecCreateSAs(service_ptr, &sip_ptr->event)) {
        SAPP_dbgPrintf("%s %d: Create IPSEC SAs failed.\n",
                __FILE__, __LINE__);
        /* Tell ISI IPSec SAs create fail */
        OSAL_snprintf(sip_ptr->event.isiMsg.msg.service.reasonDesc,
                ISI_EVENT_DESC_STRING_SZ,
                "REG FAILED: CODE:%d REASON:%s", 0,
                "FAIL TO CREATE IPSEC SAs");
        SAPP_serviceIsiEvt(service_ptr->isiServiceId, service_ptr->protocolId,
                    ISIP_SERVICE_REASON_NET, ISIP_STATUS_FAILED,
                    &sip_ptr->event.isiMsg);
        SAPP_sendEvent(&sip_ptr->event);

        return (SAPP_ERR);
    }

    return (SAPP_OK);
}

/*
 * ======== _SAPP_ipsecSetDefaultPort() ========
 * This function is to set default unprotected port to
 * local sip infc and prxoy or outbound proxy string. 
 *
 * Return Values:
 * SAPP_OK      Set default port successfully
 * SAPP_ERR     Set default port failed
 */
vint _SAPP_ipsecSetDefaultPort(
    SAPP_ServiceObj *service_ptr,
    SAPP_SipObj     *sip_ptr)
{
    /* If useIpSec is not used or current port is deafult port then no need to change port */
    if (0 == service_ptr->sipConfig.useIpSec) {
        return (SAPP_OK);
    }

    /* Update local sip port, no server port is required */
    if (SAPP_OK != _SAPP_ipsecSetLocalPort(service_ptr, sip_ptr,
            service_ptr->ipsecObj.defaultPort, 0)) {
        SAPP_dbgPrintf("%s %d: Update local port failed.\n",
                __FILE__, __LINE__);
        /* We still need to set proxy to default port, so don't return */
    }

    /* Update PCSCF's port */
    if (SAPP_OK != _SAPP_ipsecSetProxyPort(service_ptr, sip_ptr,
            service_ptr->ipsecObj.defaultProxyPort)) {
        SAPP_dbgPrintf("%s %d: Update PCSCF's port failed.\n",
                __FILE__, __LINE__);
        return (SAPP_ERR);
    }

    return (SAPP_OK);
}

/*
 * ======== _SAPP_ipsecAddSecurityVerify() ========
 * This function is to add Security-Verify header field 
 * bases on the SAs information in the SAPP_ServiceObj.
 *
 * Return Values:
 *
 */
vint _SAPP_ipsecAddSecurityVerify(
    SAPP_ServiceObj *service_ptr,
    char            *hdrFlds_ptr[],
    vint            *numHdrFlds)
{
    vint     numSecV;

    numSecV = 0;
    
    /*
         * Add Security-Verify header field, it's stored in first entry of
         * service_ptr->secSrvHfs already.
         */
    while ((numSecV < SAPP_SEC_SRV_HF_MAX_NUM) &&
            (service_ptr->secSrvHfs[numSecV][0] != 0)) {
        hdrFlds_ptr[*numHdrFlds] = &service_ptr->secSrvHfs[numSecV][0];
        numSecV++;
        (*numHdrFlds)++;
    }
    return (numSecV);
}

/*
 * ======== _SAPP_ipsecAddSecAgreeRequire() ========
 * This function is to add sec-agree header field 
 * bases on the SAs information in the SAPP_ServiceObj.
 *
 * Return Values:
 *
 */
void _SAPP_ipsecAddSecAgreeRequire(
    SAPP_ServiceObj *service_ptr,
    char            *hdrFlds_ptr[],
    vint            *numHdrFlds)
{
    /* Set up the "Require" header field for IPSEC */
    OSAL_snprintf(&service_ptr->hfStratch[*numHdrFlds][0],
            SAPP_STRING_SZ, "Require: %s", "sec-agree");
    hdrFlds_ptr[*numHdrFlds] = &service_ptr->hfStratch[*numHdrFlds][0];
    (*numHdrFlds)++;
    /* Set up the "Proxy-Require" header field for IPSEC */
    OSAL_snprintf(&service_ptr->hfStratch[*numHdrFlds][0],
            SAPP_STRING_SZ, "Proxy-Require: %s", "sec-agree");
    hdrFlds_ptr[*numHdrFlds] = &service_ptr->hfStratch[*numHdrFlds][0];
    (*numHdrFlds)++;
}

/*
 * ======== _SAPP_ipsecAddSecurityClient() ========
 * This function is to add Security-Client header field 
 * bases on the SAs information in the SAPP_ServiceObj.
 *
 * Return Values:
 *
 */
void _SAPP_ipsecAddSecurityClient(
    SAPP_ServiceObj *service_ptr,
    char            *target_ptr,
    vint             targetLen)
{
    OSAL_IpsecSa  *inSAc_ptr;
    OSAL_IpsecSa  *inSAs_ptr;
    OSAL_IpsecSa  *outSAc_ptr;

    inSAc_ptr = &service_ptr->ipsecObj.inboundSAc;
    inSAs_ptr = &service_ptr->ipsecObj.inboundSAs;
    outSAc_ptr = &service_ptr->ipsecObj.outboundSAc;

    OSAL_snprintf(target_ptr, targetLen,
            "%s%s;%s%s;%s%s;%s%s;%s%s;%s%u;%s%u;%s%d;%s%d",
            SAPP_SECURITY_CLIENT_HF, SAPP_IPSEC_3GPP_ARG,
            SAPP_ALG_ARG, _SAPP_ipsecAlgStrings[inSAc_ptr->algAh],
            SAPP_PROT_ARG, _SAPP_ipsecProtStrings[inSAc_ptr->protocol],
            SAPP_MOD_ARG, _SAPP_ipsecModStrings[inSAc_ptr->mode],
            SAPP_EALG_ARG, _SAPP_ipsecEAlgStrings[inSAc_ptr->algEsp],
            SAPP_SPI_C_ARG, inSAc_ptr->spi,
            SAPP_SPI_S_ARG, inSAs_ptr->spi,
            SAPP_PORT_C_ARG, OSAL_netNtohs(outSAc_ptr->srcAddr.port),
            SAPP_PORT_S_ARG, OSAL_netNtohs(inSAs_ptr->dstAddr.port));
}

/*
 * ======== _SAPP_ipsecCalculateAKAResp() ========
 * This function calculates the response for AKA challenge.
 *
 * Returns:
 */
vint _SAPP_ipsecCalculateAKAResp(
    SAPP_ServiceObj *service_ptr,
    uint8           *rand_ptr,
    uint8           *autn_ptr,
    SAPP_SipObj     *sip_ptr)
{
    tUaConfig *config_ptr;

    vint    keyLen;
    vint    i;
    uint8  *sqn_ptr;
    uint8  *mac_ptr;
    uint8   k[SIP_AUTH_KEY_SIZE + 1];
    uint8   op[SIP_AUTH_OP_SIZE];
    uint8   amf[SIP_AUTH_AMF_SIZE + 1];
    uint8   ck[SIP_AUTH_AKA_CKLEN];
    uint8   ik[SIP_AUTH_AKA_IKLEN];
    uint8   ak[SIP_AUTH_AKA_AKLEN];
    uint8   sqn[SIP_AUTH_AKA_SQNLEN + 1];
    uint8   xmac[SIP_AUTH_AKA_MACLEN + 1];
    uint8   res[SIP_AUTH_AKA_RESLEN];

    config_ptr = &service_ptr->sipConfig.config;

    sqn_ptr = autn_ptr;
    mac_ptr = autn_ptr + SIP_AUTH_AKA_SQNLEN + SIP_AUTH_AMF_SIZE;

    /* Copy k. op, and amf */
    OSAL_memSet(k, 0, sizeof(k));
    OSAL_memSet(op, 0, sizeof(op));
    OSAL_memSet(amf, 0, sizeof(amf));

    keyLen = OSAL_strlen((char*)config_ptr->authCred[0].u.szAuthPassword);
    /* Copy the key from password */
    OSAL_memCpy(k, config_ptr->authCred[0].u.szAuthPassword,
            (keyLen > SIP_AUTH_KEY_SIZE)? SIP_AUTH_KEY_SIZE: keyLen);

    /* Optional 'op' and 'amf' code, leave them empty */

    /* Given key K and random challenge RAND, compute response RES,
     * confidentiality key CK, integrity key IK and anonymity key AK.
     */
    f2345(k, rand_ptr, res, ck, ik, ak, op);

    /* Compute sequence number SQN */
    for (i = 0 ; i < SIP_AUTH_AKA_SQNLEN; ++i) {
        sqn[i] = (char) (sqn_ptr[i] ^ ak[i]);
    }

    /* Verify MAC in the challenge */
    f1(k, rand_ptr, sqn, amf, xmac, op);

    if (0 != OSAL_memCmp(mac_ptr, xmac, SIP_AUTH_AKA_MACLEN)) {
        service_ptr->akaAuthRespSet = 1;
        service_ptr->akaAuthResLength = 0;
        OSAL_memSet(service_ptr->akaAuthResp, 0, SAPP_AKA_AUTH_RESP_SZ);
        return (SAPP_ERR);
    }

    service_ptr->akaAuthRespSet = 1;
    service_ptr->akaAuthResLength = 8;
    OSAL_memCpy(service_ptr->akaAuthResp, res, SIP_AUTH_AKA_RESLEN);
    OSAL_memCpy(service_ptr->akaAuthCk, ck, SAPP_AKA_AUTH_CK_SZ);
    OSAL_memCpy(service_ptr->akaAuthIk, ik, SAPP_AKA_AUTH_IK_SZ);

    return (SAPP_OK);
}

vint _SAPP_ipsecClean(
    SAPP_ServiceObj *service_ptr,
    SAPP_SipObj     *sip_ptr)
{
    _SAPP_ipsecClearSAs(service_ptr, &sip_ptr->event);

    /* Close protected socket */
    if (OSAL_SUCCESS ==
            OSAL_netIsSocketIdValid(&service_ptr->sipInfcProtectedServerFd)) {
        SIP_CloseConnection(service_ptr->sipInfcProtectedServerFd);
        IMS_NET_CLOSE_SOCKET(&service_ptr->sipInfcProtectedServerFd);
        service_ptr->sipInfcProtectedServerFd = OSAL_NET_SOCK_INVALID_ID;
    }
    if (OSAL_SUCCESS ==
            OSAL_netIsSocketIdValid(&service_ptr->sipInfcProtectedClientFd)) {
        SIP_CloseConnection(service_ptr->sipInfcProtectedClientFd);
        IMS_NET_CLOSE_SOCKET(&service_ptr->sipInfcProtectedClientFd);
        service_ptr->sipInfcProtectedClientFd = OSAL_NET_SOCK_INVALID_ID;
    }
    if (OSAL_SUCCESS ==
            OSAL_netIsSocketIdValid(
            &service_ptr->sipInfcProtectedTcpServerFd)) {
        SIP_CloseConnection(service_ptr->sipInfcProtectedTcpServerFd);
        IMS_NET_CLOSE_SOCKET(&service_ptr->sipInfcProtectedTcpServerFd);
        service_ptr->sipInfcProtectedTcpServerFd = OSAL_NET_SOCK_INVALID_ID;
    }
    if (OSAL_SUCCESS ==
            OSAL_netIsSocketIdValid(
            &service_ptr->sipInfcProtectedTcpClientFd)) {
        SIP_CloseConnection(service_ptr->sipInfcProtectedTcpClientFd);
        IMS_NET_CLOSE_SOCKET(&service_ptr->sipInfcProtectedTcpClientFd);
        service_ptr->sipInfcProtectedTcpClientFd = OSAL_NET_SOCK_INVALID_ID;
    }

    _SAPP_ipsecSetDefaultPort(service_ptr, sip_ptr);
    return (SAPP_OK);
}

/*
 * ======== _SAPP_ipsecResetPort() ========
 * This function is to reset protected port to
 * local sip infc and prxoy or outbound proxy string. 
 *
 * Return Values:
 * SAPP_OK      Set default port successfully
 * SAPP_ERR     Set default port failed
 */
vint _SAPP_ipsecResetPort(
    SAPP_ServiceObj *service_ptr,
    SAPP_SipObj     *sip_ptr,
    uint16           portUC,
    uint16           portUS)
{

    /* reset portUS */
    if (SAPP_OK != _SAPP_ipsecBindSockets(service_ptr, portUS,
            &service_ptr->sipInfcProtectedServerFd)) {
        return (SAPP_ERR);
    }

    /* reset portUC */
    if (SAPP_OK != _SAPP_ipsecBindSockets(service_ptr, portUC,
            &service_ptr->sipInfcProtectedClientFd)) {
        return (SAPP_ERR);
    }

    /* reset local port */
    if (SAPP_OK != _SAPP_ipsecSetLocalPort(service_ptr, sip_ptr,
            portUC, portUS)) {
        SAPP_dbgPrintf("%s %d: Update local port failed.\n",
                __FILE__, __LINE__);        
        return (SAPP_ERR);
    }

    /* reset PCSCF's port */
    if (SAPP_OK != _SAPP_ipsecSetProxyPort(service_ptr, sip_ptr,
            OSAL_netNtohs(service_ptr->ipsecObj.outboundSAc.dstAddr.port))) {
        SAPP_dbgPrintf("%s %d: Update PCSCF's port failed.\n",
                __FILE__, __LINE__);
        return (SAPP_ERR);
    }
    return (SAPP_OK);
}


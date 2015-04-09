/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28728 $ $Date: 2014-09-05 17:01:31 +0800 (Fri, 05 Sep 2014) $
 */

#include "sip_sip.h"
#include <osal_net.h>

/******************************************************************************
 * ================VoIP_IpV4Ext2Int()===================
 *
 * This function takes an IPV4 address in string form and converts it to
 * to a unsigned long IN HOST BYTE ORDER.
 *
 * pStr = A pointer to the IP address in string form (i.e. "10.1.1.1")
 *          (Must be zero terminated)
 *
 * len = The length of the string (NO LONGER USED)
 *
 * pAddr = A pointer to the tIPAddr object that will be populated with the
 *         ip address in host byte order.
 *
 * RETURNS:
 *         SIP_OK:  The string was successfully converted
 *         SIP_FAILED: The string was not a valid IP address and the
 *                     conversion failed.
 ******************************************************************************
 */
vint VoIP_IpV4Ext2Int(
    const char *pStr,
    uint32      len,
    tIPAddr    *pAddr)
{
    OSAL_NetAddress addrObj;

    if (OSAL_SUCCESS == OSAL_netStringToAddress((int8 *)pStr, &addrObj)) {
        pAddr->v4.ul = OSAL_netNtohl(addrObj.ipv4);
        return (SIP_OK);
    }

    return (SIP_FAILED);
}

/******************************************************************************
 * ================VoIP_IpV6Ext2Int()===================
 *
 * This function takes an IPV6 address in string form and converts it to
 * to a 8-word data IN NETWORK BYTE ORDER!
 *
 * pStr = A pointer to the IP address in string form (i.e. "10.1.1.1")
 *
 * len = The length of the string
 *
 * pAddr = A pointer to the tIPAddr object that will be populated with the
 *         ip address in netwrok byte order.
 *
 * RETURNS:
 *         SIP_OK:  The string was successfully converted
 *         SIP_FAILED: The string was not a valid IP address and the
 *                     conversion failed.
 ******************************************************************************
 */
vint VoIP_IpV6Ext2Int(
    const char *pStr,
    uint32      len,
    tIPAddr    *pAddr)
{
    OSAL_NetAddress addrObj;

    if (OSAL_SUCCESS == OSAL_netStringToAddress((int8 *)pStr, &addrObj) &&
            OSAL_NET_SOCK_UDP_V6 == addrObj.type) {
        OSAL_netIpv6Ntoh(pAddr->v6, addrObj.ipv6);
        return (SIP_OK);
    }

    return (SIP_FAILED);
}
/******************************************************************************
 * ================VoIP_IpAddrExt2Int()===================
 *
 * This function converts a network address in string form and converts it to
 * to a tNetworkAddr object.  This function identifies if the string
 * is a domain name or an IP address.  For example, the string could be
 * "d2tech.com" or "192.168.1.1".
 *
 * pStr = A pointer to the IP address\domain name in string form
 *
 * len = The length of the string
 *
 * port = A port number
 *
 * pAddr = A pointer to the tNetworkAddress object that will be populated
 *
 * RETURNS:
 *         SIP_OK:  Always
 *
 ******************************************************************************
 */
vint VoIP_IpAddrExt2Int(
     const char     *pStr,
     uint32          len,
     uint16          port,
    tNetworkAddress *pAddr)
{
    char addrBuf[MAX_IPV6_STR_LEN + 1];
    pAddr->port = OSAL_netHtons(port);

    if (len <= MAX_IPV6_STR_LEN) {
        OSAL_memCpy(addrBuf, pStr, len);
        addrBuf[len] = 0;
        /* Try if it's IPv6 Address */
        if (SIP_OK == VoIP_IpV6Ext2Int(addrBuf, len, &pAddr->x.ip)) {
            pAddr->addressType = eNwAddrIPv6;
            return (SIP_OK);
        }
        else if (SIP_OK == VoIP_IpV4Ext2Int(addrBuf, len, &pAddr->x.ip)) {
            pAddr->addressType = eNwAddrIPv4;
            return (SIP_OK);
        }
    }

    /* it's a domain name */
    if (len >= MAX_DOMAIN_NAME_LEN) {
        len = (MAX_DOMAIN_NAME_LEN - 1);
    }
    OSAL_memCpy(pAddr->x.domainName, pStr, len);
    pAddr->x.domainName[len] = 0;
    pAddr->addressType = eNwAddrDomainName;
    return (SIP_OK);
}

/******************************************************************************
 * ================VoIP_IpV4Int2Ext()===================
 *
 * This function converts a tIPAddr object to string form.
 * This function identifies if the tIPAddr is a domain name or an IP address.
 * For example, the output could be "d2tech.com" or "192.168.1.1".
 *
 * IpV4 = The tIPAddr to convert
 *
 * pIpAddr = A pointer to the string to be populated
 *
 * RETURNS:
 *         SIP_OK:  The conversion was successfull
 *         SIP_FAILED: The conversion failed
 *
 ******************************************************************************
 */
vint VoIP_IpV4Int2Ext(
    tIPAddr ip,
    char   *pIpAddr)
{
    OSAL_NetAddress addrObj;

    addrObj.type = OSAL_NET_SOCK_UDP;
    addrObj.ipv4 = OSAL_netHtonl(ip.v4.ul);
    if (OSAL_SUCCESS == OSAL_netAddressToString((int8 *)pIpAddr, &addrObj)) {
        return (SIP_OK);
    }
    return (SIP_FAILED);
}

/******************************************************************************
 * ================VoIP_IpV6Int2Ext()===================
 *
 * This function converts a tIPAddr obejct to string form.
 *
 * IpV4 = The tIPAddr to convert
 *
 * pIpAddr = A pointer to the string to be populated
 *
 * RETURNS:
 *         SIP_OK:  The conversion was successfull
 *         SIP_FAILED: The conversion failed
 *
 ******************************************************************************
*/
vint VoIP_IpV6Int2Ext(
    tIPAddr ip,
    char   *pIpAddr)
{
    OSAL_NetAddress addrObj;

    addrObj.type = OSAL_NET_SOCK_UDP_V6;
    OSAL_netIpv6Hton(addrObj.ipv6, ip.v6);
    if (OSAL_SUCCESS == OSAL_netAddressToString((int8 *)pIpAddr, &addrObj)) {
        return (SIP_OK);
    }
    return (SIP_FAILED);
}

/*
 *****************************************************************************
 * ================VoIP_IsEqualHost()===================
 *
 * This function works like an overloaded "=" operator.  It returns
 * true if two 'tNetworkAddress' objects are equal.
 *
 * pA = A pointer to a 'tNetworkAddress' object
 *
 * pB = A pointer to a 'tNetworkAddress' object
 *
 * RETURNS:
 *         TRUE:  They are equal
 *         FALSE: These are different
 *
 ******************************************************************************
 */
vint VoIP_IsEqualHost(
    tNetworkAddress *pA,
    tNetworkAddress *pB)
{
    if (pA->addressType == pB->addressType) {
        if (pA->addressType == eNwAddrDomainName) {
            if(OSAL_strcasecmp(pA->x.domainName, pB->x.domainName) != 0) {
                return (FALSE);
            }
        }
        else if (pA->addressType == eNwAddrIPv4) {
            if (pA->x.ip.v4.ul != pB->x.ip.v4.ul) {
                return (FALSE);
            }
        }
        else if (pA->addressType == eNwAddrIPv6) {
            if (OSAL_memCmp(pA->x.ip.v6, pB->x.ip.v6, sizeof(pA->x.ip.v6))) {
                return (FALSE);
            }
        }

        /* we are looking good if we are here, check the rest */
        if (pA->port == pB->port) {
            return (TRUE);
        }
    }
    return (FALSE);
}

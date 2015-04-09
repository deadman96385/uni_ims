/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29711 $ $Date: 2014-11-06 12:42:22 +0800 (Thu, 06 Nov 2014) $
 */

#ifndef _SIP_VOIPNET_H_
#define _SIP_VOIPNET_H_

/* Network type */
typedef enum eNetworkType
{
   eNetworkLocal,               /* Local connection */
   eNetworkIN,                  /* Internet */
   eNetworkATM                  /* ATM */
} tNetworkType;


/* Transport type */
typedef enum eTransportType
{
   eTransportNone,
   eTransportLocal,
   eTransportRtpAvp,
   eTransportRtpSavp,
   eTransportRtpAvpf,
   eTransportRtpSavpf,
   eTransportAtmAvp,
   eTransportUdp,
   eTransportTcp,
   eTransportTls,
   eTransportSip,
   eTransportUdptl,
   eTransportMsrpTcp,
   eTransportMsrpTls,
   eTransportLast
} tTransportType ;


/* Network address type */
typedef enum eNetworkAddrType
{
   eNwAddrNonSpecified,         /* Network address isn't specified */
   eNwAddrLocal,                /* Local address. Usually endpoint name */
   eNwAddrDomainName,           /* Domain name. Can be used with IPv4 and IPv6 */
   eNwAddrIPv4,                 /* IPv4 */
   eNwAddrIPv6,                 /* IPv6 */
   eNwAddrLast
}  tNetworkAddrType;

typedef union uIPAddr
{
    union {
        uint32 ul;
        uint8  b[4];    
    } v4;                        /* ipv4 */
    uint16 v6[8];                /* ipv6 */
}tIPAddr;

/* Network address */
typedef struct sNetworkAddress
{
   tNetworkAddrType addressType;
   union
   {
      char        epName[MAX_DOMAIN_NAME_LEN];
      char        domainName[MAX_DOMAIN_NAME_LEN];
      tIPAddr     ip;            /* ipv6 or ipv4 */
   } x;
   uint16            port;
} tNetworkAddress;

typedef struct sPacketRate
{
    uint16 target;
    uint16 high;
    uint16 low;
}tPacketRate;

/******************************************************************************
 * ================VoIP_IpV4Ext2Int()===================
 *
 * This function takes an IPV4 address in string form and converts it to
 * to a unsigned long IN NETWORK BYTE ORDER!
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
vint VoIP_IpV4Ext2Int(
    const char *pStr, 
    uint32      len, 
    tIPAddr    *pAddr);

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
    tIPAddr    *pAddr);

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
    tNetworkAddress *pAddr);

/******************************************************************************
 * ================VoIP_IpV4Int2Ext()===================
 *
 * This function converts a tIPAddr obejct to string form.
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
    char   *pIpAddr);

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
    char   *pIpAddr);

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
    tNetworkAddress *pB);


#endif   /* #ifndef VOIP_NW_H */


/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 1381 $ $Date: 2006-12-05 14:44:53 -0800 (Tue, 05 Dec 2006) $
 */

#include <osal_ipsec.h>
#include <osal_mem.h>
#include <osal_log.h>

#ifdef OSAL_IPSEC_ENABLE
/* Defines */

static char ifSetInBound = 0;
static uint32 inBoundSpi;
static char   inKeyAh[OSAL_IPSEC_KEY_LENGTH_MAX + 1];
static char   inKeyEsp[OSAL_IPSEC_KEY_LENGTH_MAX + 1];

#endif // OSAL_IPSEC_ENABLE

/*
 * ======== OSAL_ipsecCreateSA() ========
 *
 * This function is used to create SA.
 *  ***NOTE :: MUST create SA inbound then outbound when using this API.
                    Because the API provide in AC502 would create inbound and outbound SA 
                    at the same time, we cache inbound data and 
                    SA would really create when creating outbound.
 * Return:
 *   OSAL_SUCCESS: successfully.
 *   OSAL_FAIL:    failed
 */
OSAL_Status OSAL_ipsecCreateSA(
    OSAL_IpsecSa *sa_ptr,
    OSAL_IpsecSp *sp_ptr)
{
#ifdef OSAL_IPSEC_ENABLE
    uint8   encr_alg;
    uint8   encr_keylen = 0;
    uint8   auth_alg;
    uint8   auth_keylen = 0;
    char    srcAddrStr[OSAL_NET_IP_STR_MAX];
    char    dstAddrStr[OSAL_NET_IP_STR_MAX];
    char    srcAddrStrwithPort[OSAL_IPSEC_STRING_SZ];
    char    dstAddrStrwithPort[OSAL_IPSEC_STRING_SZ];

    if(NULL == sa_ptr || NULL == sp_ptr){
        OSAL_logMsg("%s %d: SA or SP is null.\n", __FUNCTION__,
                __LINE__);
        return OSAL_FAIL;
    }

    /* Set esp key */
    if (OSAL_IPSEC_ENC_ALG_DES_EDE3_CBC == sa_ptr->algEsp){
        encr_alg = IPSEC_ALG_ESP_DES_EDE3_CBC;
        encr_keylen = OSAL_IPSEC_KEY_LENGTH_3DES_CBC;
    }
    else if (OSAL_IPSEC_ENC_ALG_AES_CBC == sa_ptr->algEsp) {
        encr_alg = IPSEC_ALG_ESP_AES_CBC;
        encr_keylen = OSAL_IPSEC_KEY_LENGTH_AES_CBC;
    }
    else {
        encr_alg = IPSEC_ALG_ESP_NULL;
        OSAL_logMsg("%s %d: ESP algorithm is not set.\n", __FUNCTION__,
                __LINE__);
    }
    /* Set AH key */
    if (OSAL_IPSEC_AUTH_ALG_HMAC_MD5_96 == sa_ptr->algAh) {
        auth_alg = IPSEC_ALG_AUTH_HMAC_MD5_96;
        auth_keylen = OSAL_IPSEC_KEY_LENGTH_HMAC_MD5;
    }
    else if (OSAL_IPSEC_AUTH_ALG_HMAC_SHA1_96 == sa_ptr->algAh) {
        auth_alg = IPSEC_ALG_AUTH_HMAC_SHA1_96;
        auth_keylen = OSAL_IPSEC_KEY_LENGTH_HMAC_SHA1;
    }
    else {
        auth_alg = IPSEC_ALG_AUTH_HMAC_NULL;
        OSAL_logMsg("%s %d: AH algorithm is not set.\n", __FUNCTION__,
                __LINE__);
    }
    if (0 == ifSetInBound) {
        OSAL_strncpy(inKeyAh, sa_ptr->keyAh, OSAL_IPSEC_KEY_LENGTH_MAX);
        OSAL_strncpy(inKeyEsp, sa_ptr->keyEsp, OSAL_IPSEC_KEY_LENGTH_MAX);
        inBoundSpi = sa_ptr->spi;
        ifSetInBound = 1;
    }
    else {
        /* Get ipaddr string*/
        OSAL_netAddressToString((int8 *)srcAddrStr, &sa_ptr->srcAddr);
        OSAL_netAddressToString((int8 *)dstAddrStr, &sa_ptr->dstAddr);
        /* add port */
        OSAL_snprintf(srcAddrStrwithPort, OSAL_IPSEC_STRING_SZ,"%s,%d",
                srcAddrStr, OSAL_netNtohs(sa_ptr->srcAddr.port));
        OSAL_snprintf(dstAddrStrwithPort, OSAL_IPSEC_STRING_SZ,"%s,%d",
                dstAddrStr, OSAL_netNtohs(sa_ptr->dstAddr.port));

        IPSEC_AddManualSA(
                SPRD_IPSEC_MODE_TRANSPORT,  /* TRANSPORT or TUNNEL */
                sp_ptr->transport,    /* Transport Protocol */
                srcAddrStrwithPort,    /* Source Group */
                dstAddrStrwithPort,    /* Dest Group */
                dstAddrStrwithPort,    /* Remote end-point */
                encr_alg,       /* Encryption Alg (IPSEC_ALG_ESP_*) */
                encr_keylen,    /* Encryption Key Length (bytes) */
                inKeyEsp,
                sa_ptr->keyEsp, /* Outbound Encryption Key */
                auth_alg,       /* Auth Alg (IPSEC_ALG_AUTH_HMAC_*)  */
                auth_keylen,    /* Auth Key Length (bytes) */
                inKeyAh,        /* Inbound Auth Key */
                sa_ptr->keyAh,  /* Outbound Auth Key */
                inBoundSpi,     /* ESP Inbound SPI  (host byte order)*/
                sa_ptr->spi,    /* ESP Outbound SPI  (host byte order)*/
                inBoundSpi,     /* AH Inbound SPI  (host byte order)*/
                sa_ptr->spi);   /* AH Outbound SPI  (host byte order)*/   
        ifSetInBound = 0;
    }

    return (OSAL_SUCCESS);
#else
    return (OSAL_FAIL);
#endif // OSAL_IPSEC_ENABLE
}

/*
 * ======== OSAL_ipsecDeleteSA() ========
 *
 * This function is used to delete SA.
 *
 * Return:
 *   OSAL_SUCCESS: successfully.
 *   OSAL_FAIL:    failed
 */
OSAL_Status OSAL_ipsecDeleteSA(
    OSAL_IpsecSa *sa_ptr,
    OSAL_IpsecSp *sp_ptr)
{
#ifdef OSAL_IPSEC_ENABLE
    char    srcAddrStr[OSAL_NET_IP_STR_MAX];
    char    dstAddrStr[OSAL_NET_IP_STR_MAX];
    char    srcAddrStrwithPort[OSAL_IPSEC_STRING_SZ];
    char    dstAddrStrwithPort[OSAL_IPSEC_STRING_SZ];

    if(NULL == sa_ptr || NULL == sp_ptr){
        OSAL_logMsg("%s %d: SA or SP is null.\n", __FUNCTION__,
                __LINE__);
        return OSAL_FAIL;
    }    
    /* Get ipaddr string*/
    OSAL_netAddressToString((int8 *)srcAddrStr, &sa_ptr->srcAddr);
    OSAL_netAddressToString((int8 *)dstAddrStr, &sa_ptr->dstAddr);
    /* add port */
    OSAL_snprintf(srcAddrStrwithPort, OSAL_IPSEC_STRING_SZ,"%s,%d",
                srcAddrStr, OSAL_netNtohs(sa_ptr->srcAddr.port));
    OSAL_snprintf(dstAddrStrwithPort, OSAL_IPSEC_STRING_SZ,"%s,%d",
                dstAddrStr, OSAL_netNtohs(sa_ptr->dstAddr.port));

    IPSEC_DeleteManualSA(
            srcAddrStr, /* Source Group */
            dstAddrStr, /* Dest Group */
            sp_ptr->transport);  /* Transport Protocol */
    
    return (OSAL_SUCCESS);
#else
    return (OSAL_FAIL);
#endif // OSAL_IPSEC_ENABLE
}


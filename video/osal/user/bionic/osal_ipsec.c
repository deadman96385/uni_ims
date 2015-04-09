/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 1381 $ $Date: 2006-12-05 14:44:53 -0800 (Tue, 05 Dec 2006) $
 */

#include <osal_ipsec.h>
#include <osal_mem.h>
#include <osal_sys.h>
#include <osal_log.h>

#ifdef OSAL_IPSEC_ENABLE
/* Defines for private use */
#define _OSAL_IPSEC_ALIGN8(a)     (1 + (((a) - 1) | (8 - 1)))
#define _OSAL_IPSEC_UNUNIT64(a)   ((a) << 3)
#define _OSAL_IPSEC_UNIT64(a)     ((a) >> 3)

/* ========= _OSAL_ipsecSetExtVar() =========
 * Private function to set extension and variables to buffer
 *
 * Return:
 *   OSAL_SUCCESS: success to add ext var into message.
 *   OSAL_FAIL:    fail to add ext var into message.
 */
OSAL_Status _OSAL_ipsecSetExtVar(
    char            *buf_ptr,
    int             *offset,
    struct sadb_ext *ext_ptr,
    int              eLen,
    const void      *var_ptr,
    int              vLen)
{
    /* If the remaining length of message is not enough, return OSAL_FAIL */
    if ((eLen + vLen) > (OSAL_IPSEC_PFKEY_MSG_SIZE_MAX - (*offset))) {
        return (OSAL_FAIL);
    }

    OSAL_memSet(buf_ptr + *offset, 0,
            _OSAL_IPSEC_UNUNIT64(ext_ptr->sadb_ext_len));
    OSAL_memCpy(buf_ptr + *offset, (caddr_t)ext_ptr, eLen);
    OSAL_memCpy(buf_ptr + *offset + eLen, var_ptr, vLen);
    (*offset) += _OSAL_IPSEC_ALIGN8(eLen + vLen);
    return (OSAL_SUCCESS);
}

/*
 * ======== _OSAL_ipsecInitMsg() ========
 *
 * Private function to initialize sadb_msg.
 *
 * Return:
 */
static void _OSAL_ipsecInitMsg(
    struct sadb_msg *msg_ptr,
    unsigned int     type,
    unsigned int     satype,
    int              length)
{
    msg_ptr->sadb_msg_version  = PF_KEY_V2;
    msg_ptr->sadb_msg_type     = type;
    msg_ptr->sadb_msg_errno    = 0;
    msg_ptr->sadb_msg_satype   = satype;
    msg_ptr->sadb_msg_reserved = 0;
    msg_ptr->sadb_msg_seq      = 0;
    msg_ptr->sadb_msg_pid      = OSAL_sysGetPid();
    msg_ptr->sadb_msg_len      = _OSAL_IPSEC_UNIT64(length);
}

/*
 * ======== _OSAL_ipsecMsgSetSadb() ========
 *
 * Private function to set sadb.
 *
 * Return:
 *   OSAL_SUCCESS: success to add sadb into message.
 *   OSAL_FAIL:    fail to add sadb into message.
 */
OSAL_Status _OSAL_ipsecMsgSetSadb(
    char         *msg_ptr,
    OSAL_IpsecSa *sa_ptr,
    int          *msgLen_ptr,
    int           authAlg,
    int           encAlg)
{
    struct sadb_sa    m_sa;
    struct sadb_x_sa2 m_sa2;
    int len;

    OSAL_memSet(&m_sa, 0, sizeof(struct sadb_sa));
    len = sizeof(struct sadb_sa);
    m_sa.sadb_sa_len     = _OSAL_IPSEC_UNIT64(len);
    m_sa.sadb_sa_exttype = SADB_EXT_SA;
    m_sa.sadb_sa_spi     = htonl(sa_ptr->spi);
    m_sa.sadb_sa_replay  = 0;
    m_sa.sadb_sa_state   = 0;
    m_sa.sadb_sa_auth    = authAlg;
    m_sa.sadb_sa_encrypt = encAlg;
    m_sa.sadb_sa_flags   = 0;

    if (len > (OSAL_IPSEC_PFKEY_MSG_SIZE_MAX - *msgLen_ptr)) {
        return (OSAL_FAIL);
    }
    OSAL_memCpy((msg_ptr + *msgLen_ptr), &m_sa, len);
    *msgLen_ptr += len;

    OSAL_memSet(&m_sa2, 0, sizeof(struct sadb_x_sa2));
    len = sizeof(struct sadb_x_sa2);
    m_sa2.sadb_x_sa2_len     = _OSAL_IPSEC_UNIT64(len);
    m_sa2.sadb_x_sa2_exttype = SADB_X_EXT_SA2;
    /* only support transport for now */
    m_sa2.sadb_x_sa2_mode    = IPSEC_MODE_TRANSPORT;
    m_sa2.sadb_x_sa2_reqid   = sa_ptr->reqId;                      

    if (len > (OSAL_IPSEC_PFKEY_MSG_SIZE_MAX - *msgLen_ptr)) {
        return (OSAL_FAIL);
    }
    OSAL_memCpy((msg_ptr + *msgLen_ptr), &m_sa2, len);
    *msgLen_ptr += len;

    return (OSAL_SUCCESS);
}

/*
 * ======== _OSAL_ipsecMsgSetAddrExt() ========
 *
 * Private function to set address to sadb msg.
 *
 * Return:
 *   OSAL_SUCCESS: success to add address into message.
 *   OSAL_FAIL:    fail to add address into message.
 */
OSAL_Status _OSAL_ipsecMsgSetAddrExt(
    char            *msg_ptr,
    OSAL_NetAddress *addr_ptr,
    unsigned int     extType,
    int             *msgLen_ptr)
{
    struct sadb_address  sadbAddr;
    struct sockaddr_in   ipv4Addr;
    struct sockaddr_in6  ipv6Addr;
    struct sockaddr     *sa_ptr;
    int                  saLen;

    OSAL_memSet(&sadbAddr, 0, sizeof(struct sadb_address));
    if ((OSAL_NET_SOCK_TCP_V6 == addr_ptr->type) ||
            (OSAL_NET_SOCK_UDP_V6 == addr_ptr->type)) {
        saLen    = sizeof(struct sockaddr_in6);
        ipv6Addr.sin6_family = AF_INET6;
        ipv6Addr.sin6_port   = addr_ptr->port;
        OSAL_memCpy(&ipv6Addr.sin6_addr, addr_ptr->ipv6,
                sizeof(addr_ptr->ipv6));
        sa_ptr = (struct sockaddr *)&ipv6Addr;
    }
    else {
        saLen = sizeof(struct sockaddr_in);
        ipv4Addr.sin_family      = AF_INET;
        ipv4Addr.sin_port        = addr_ptr->port;
        ipv4Addr.sin_addr.s_addr = addr_ptr->ipv4;
        sa_ptr = (struct sockaddr *)&ipv4Addr;
    }

    sadbAddr.sadb_address_len       =
            _OSAL_IPSEC_UNIT64(sizeof(struct sadb_address) +
            _OSAL_IPSEC_ALIGN8(saLen));
    sadbAddr.sadb_address_proto     = IPSEC_ULPROTO_ANY;
    sadbAddr.sadb_address_prefixlen = 0;
    sadbAddr.sadb_address_reserved  = 0;
    sadbAddr.sadb_address_exttype   = extType;

    return (_OSAL_ipsecSetExtVar(msg_ptr, msgLen_ptr,
            (struct sadb_ext *)&sadbAddr, sizeof(sadbAddr), sa_ptr, saLen));
}

/* ========= _OSAL_ipsecMsgSetKey() ==========
 *  Private function to set key data to sadb msg.
 *
 *  Return:
 *    OSAL_SUCCESS: success to add key into message.
 *    OSAL_FAIL:    fail to add key into message.
 */
OSAL_Status _OSAL_ipsecMsgSetKey(
    char         *msg_ptr,
    OSAL_IpsecSa *sa_ptr,
    int          *msgLen_ptr,
    int           keyLen,
    int           extType,
    char         *key_ptr)
{
    union {
        struct sadb_key key;
        struct sadb_ext ext;
    } kMsg;

    kMsg.key.sadb_key_len      =
            _OSAL_IPSEC_UNIT64(sizeof(kMsg.key) +
            _OSAL_IPSEC_ALIGN8(keyLen));
    kMsg.key.sadb_key_exttype  = extType;
    kMsg.key.sadb_key_bits     = keyLen * 8;
    kMsg.key.sadb_key_reserved = 0;

    return (_OSAL_ipsecSetExtVar(msg_ptr, msgLen_ptr, &kMsg.ext,
            sizeof(kMsg.key), key_ptr, keyLen));
}

/* ========= _OSAL_ipsecMsgSetPolicyExt() ==========
 * Private function to set policy extension to message
 *
 * Return:
 *   OSAL_SUCCESS: successfully.
 *   OSAL_FAIL:    failed
 */
OSAL_Status _IPSEC_pfkeyMsgSetPolicyExt(
    char         *msg_ptr,
    OSAL_IpsecSp *sp_ptr,
    int          *msgLen_ptr)   
{
    struct sadb_x_policy        xp;
    struct sadb_x_ipsecrequest *xr_ptr;

    char   rules[OSAL_IPSEC_PFKEY_REQUEST_BYTE_MAX];
    int    rulesLen;
    int    dir;

    rulesLen = 0;
    xr_ptr = (struct sadb_x_ipsecrequest*)rules; 
    /* rule for ESP */
    if (OSAL_IPSEC_PROTOCOL_ESP == sp_ptr->protocol) {
        xr_ptr->sadb_x_ipsecrequest_len       =
                sizeof(struct sadb_x_ipsecrequest);
        xr_ptr->sadb_x_ipsecrequest_proto     = IPPROTO_ESP;
        /* Always use transport */
        xr_ptr->sadb_x_ipsecrequest_mode      = IPSEC_MODE_TRANSPORT;
        xr_ptr->sadb_x_ipsecrequest_level     = sp_ptr->level;
        xr_ptr->sadb_x_ipsecrequest_reserved1 = 0;
        xr_ptr->sadb_x_ipsecrequest_reqid     = sp_ptr->reqId;
        xr_ptr->sadb_x_ipsecrequest_reserved2 = 0;
        rulesLen += sizeof(struct sadb_x_ipsecrequest);
        xr_ptr = (struct sadb_x_ipsecrequest*)(rules + rulesLen);
    }

    /* rule for AH */
    if (OSAL_IPSEC_PROTOCOL_AH == sp_ptr->protocol) {
        xr_ptr->sadb_x_ipsecrequest_len       =
                sizeof(struct sadb_x_ipsecrequest);
        xr_ptr->sadb_x_ipsecrequest_proto     = IPPROTO_AH;
        /* Always use transport */
        xr_ptr->sadb_x_ipsecrequest_mode      = IPSEC_MODE_TRANSPORT;
        /* Always Required */
        xr_ptr->sadb_x_ipsecrequest_level     = IPSEC_LEVEL_REQUIRE;
        xr_ptr->sadb_x_ipsecrequest_reserved1 = 0;
        xr_ptr->sadb_x_ipsecrequest_reqid     = sp_ptr->reqId;
        xr_ptr->sadb_x_ipsecrequest_reserved2 = 0;
        rulesLen += sizeof(struct sadb_x_ipsecrequest);
    }

    if (OSAL_IPSEC_DIR_OUT == sp_ptr->dir) {
        dir = IPSEC_DIR_OUTBOUND;
    }
    else if (OSAL_IPSEC_DIR_IN == sp_ptr->dir) {
        dir = IPSEC_DIR_INBOUND;
    }
    else {
        OSAL_logMsg("%s %d Incorrect direction.\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Policy ext of SADB. See <linux/pfkeyv2.h> */
    xp.sadb_x_policy_len      = _OSAL_IPSEC_UNIT64(sizeof(xp) +
        _OSAL_IPSEC_ALIGN8(rulesLen));
    xp.sadb_x_policy_exttype  = SADB_X_EXT_POLICY;
    xp.sadb_x_policy_type     = IPSEC_POLICY_IPSEC;
    xp.sadb_x_policy_dir      = dir;
    xp.sadb_x_policy_reserved = 0;
    xp.sadb_x_policy_id       = 0;
    xp.sadb_x_policy_priority = 0;

    /* Write to buffer */ 
    return (_OSAL_ipsecSetExtVar(msg_ptr, msgLen_ptr, (struct sadb_ext *)&xp,
            sizeof(xp), rules, rulesLen));
}

/*
 * ======== _OSAL_ipsecSendSaMsg() ========
 *
 * This function is used to send SA message.
 *
 * Return:
 *   OSAL_SUCCESS: successfully.
 *   OSAL_FAIL:    failed
 */
static OSAL_Status _OSAL_ipsecSendSaMsg(
    OSAL_NetSockId *socket_ptr,
    OSAL_IpsecSa   *sa_ptr,
    int             msgType)
{
    struct sadb_msg *msg;
    char             msgStrach[OSAL_IPSEC_PFKEY_MSG_SIZE_MAX];
    unsigned int     saType;
    int              msgLen;
    int              encAlg;
    int              authAlg;
    int              encKeyLen;
    int              authKeyLen;

    OSAL_memSet(msgStrach, 0, OSAL_IPSEC_PFKEY_MSG_SIZE_MAX);
    msg    = (struct sadb_msg*)msgStrach;
    msgLen = 0;

    /* Set sa type */
    if (OSAL_IPSEC_PROTOCOL_ESP == sa_ptr->protocol) {
        saType = SADB_SATYPE_ESP;
    }
    else if (OSAL_IPSEC_PROTOCOL_AH == sa_ptr->protocol) {
        saType = SADB_SATYPE_AH;
    }
    else {
        OSAL_logMsg("%s %d: SA protocol is not set correctly.\n",
                __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Prepare message header */
    if ((int)sizeof(struct sadb_msg) > (OSAL_IPSEC_PFKEY_MSG_SIZE_MAX - msgLen)) {
        return (OSAL_FAIL);
    }
    _OSAL_ipsecInitMsg((struct sadb_msg*)msgStrach, msgType, saType, msgLen);
    msgLen = sizeof(struct sadb_msg);

    /* Set encrypt key */
    if (OSAL_IPSEC_ENC_ALG_DES_EDE3_CBC == sa_ptr->algEsp) {
        encAlg    = SADB_EALG_3DESCBC;
        encKeyLen = OSAL_IPSEC_KEY_LENGTH_3DES_CBC;
    }
    else if (OSAL_IPSEC_ENC_ALG_AES_CBC ==  sa_ptr->algEsp) {
        encAlg    = SADB_X_EALG_AESCBC;
        encKeyLen = OSAL_IPSEC_KEY_LENGTH_AES_CBC;
    }
    else {
        /* No ESP */
        encAlg = SADB_EALG_NONE;
        encKeyLen = 0;
        /* Is it ok to do ESP without algorithm? Return fail for now */
        OSAL_logMsg("%s %d: ESP algorithm is not set.\n", __FUNCTION__,
                __LINE__);
    }
    if (OSAL_SUCCESS != _OSAL_ipsecMsgSetKey(msgStrach, sa_ptr, &msgLen,
            encKeyLen, SADB_EXT_KEY_ENCRYPT, sa_ptr->keyEsp)) {
        return (OSAL_FAIL);
    } 

    /* Set AH key */
    if (OSAL_IPSEC_AUTH_ALG_HMAC_MD5_96 == sa_ptr->algAh) {
        authAlg    = SADB_AALG_MD5HMAC;
        authKeyLen = OSAL_IPSEC_KEY_LENGTH_HMAC_MD5;
    }
    else if (OSAL_IPSEC_AUTH_ALG_HMAC_SHA1_96 == sa_ptr->algAh) {
        authAlg    = SADB_AALG_SHA1HMAC;
        authKeyLen = OSAL_IPSEC_KEY_LENGTH_HMAC_SHA1;
    }
    else {
        authAlg = SADB_AALG_NONE;
        authKeyLen = 0;
        OSAL_logMsg("%s %d: AH algorithm is not set.\n", __FUNCTION__,
                __LINE__);
    }
    if (OSAL_SUCCESS != _OSAL_ipsecMsgSetKey(msgStrach, sa_ptr, &msgLen,
            authKeyLen, SADB_EXT_KEY_AUTH, sa_ptr->keyAh)) {
        return (OSAL_FAIL);
    }

    /* Set spi, encryption and authentication algorithm to SADB */
    if (OSAL_SUCCESS != _OSAL_ipsecMsgSetSadb(msgStrach, sa_ptr, &msgLen,
            authAlg, encAlg)) {
        return (OSAL_FAIL);
    }

    /* Set source address */
    if (OSAL_SUCCESS != _OSAL_ipsecMsgSetAddrExt(msgStrach, &sa_ptr->srcAddr,
            SADB_EXT_ADDRESS_SRC, &msgLen)) {
        return (OSAL_FAIL);
    }

    /* Set destination address */
    if (OSAL_SUCCESS != _OSAL_ipsecMsgSetAddrExt(msgStrach, &sa_ptr->dstAddr,
            SADB_EXT_ADDRESS_DST, &msgLen)) {
        return (OSAL_FAIL);
    }

    /* Set msg length */
    msg->sadb_msg_len = _OSAL_IPSEC_UNIT64(msgLen);

    /* Send message */
    if ((OSAL_SUCCESS != OSAL_netSocketSend(socket_ptr, msgStrach, &msgLen))) {
        OSAL_logMsg("%s %d send failed\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== _OSAL_ipsecSendSpMsg() ========
 *
 * Private function to send SP message.
 *
 * Return:
 *   OSAL_SUCCESS: successfully.
 *   OSAL_FAIL:    failed
 */
static OSAL_Status _OSAL_ipsecSendSpMsg(
    OSAL_NetSockId *socket_ptr,
    OSAL_IpsecSp   *sp_ptr,
    int             msgType)
{
    struct sadb_msg *msg;
    char             msgStrach[OSAL_IPSEC_PFKEY_MSG_SIZE_MAX];
    unsigned int     saType;
    int              msgLen;

    OSAL_memSet(msgStrach, 0, OSAL_IPSEC_PFKEY_MSG_SIZE_MAX);
    msg    = (struct sadb_msg*)msgStrach;
    msgLen = 0;

    /* Set sa type */
    saType = SADB_SATYPE_UNSPEC;
    /* Prepare message header */
    if ((int)sizeof(struct sadb_msg) > (OSAL_IPSEC_PFKEY_MSG_SIZE_MAX - msgLen)) {
        return (OSAL_FAIL);
    }
    _OSAL_ipsecInitMsg((struct sadb_msg*)msgStrach, msgType, saType, msgLen);
    msgLen = sizeof(struct sadb_msg);

    if (OSAL_SUCCESS != _IPSEC_pfkeyMsgSetPolicyExt(msgStrach, sp_ptr,
            &msgLen)) {
        OSAL_logMsg("%s %d: Set policy failed.\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Set source address */
    if (OSAL_SUCCESS != _OSAL_ipsecMsgSetAddrExt(msgStrach, &sp_ptr->srcAddr,
            SADB_EXT_ADDRESS_SRC, &msgLen)) {
        return (OSAL_FAIL);
    }

    /* Set destination address */
    if (OSAL_SUCCESS != _OSAL_ipsecMsgSetAddrExt(msgStrach, &sp_ptr->dstAddr,
            SADB_EXT_ADDRESS_DST, &msgLen)) {
        return (OSAL_FAIL);
    }

    /* Set msg length */
    msg->sadb_msg_len = _OSAL_IPSEC_UNIT64(msgLen);

    /* Send message */
    if ((OSAL_SUCCESS != OSAL_netSocketSend(socket_ptr, msgStrach, &msgLen))) {
        OSAL_logMsg("%s %d send failed\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

#endif // OSAL_IPSEC_ENABLE

/*
 * ======== OSAL_ipsecCreateSA() ========
 *
 * This function is used to create SA.
 *
 * Return:
 *   OSAL_SUCCESS: successfully.
 *   OSAL_FAIL:    failed
 */
OSAL_Status OSAL_ipsecCreateSA(
    OSAL_IpsecSa *sa_ptr,
    OSAL_IpsecSp *sp_ptr)
{
#ifdef OSAL_IPSEC_ENABLE
    OSAL_NetSockId socketId;    /* Socket id for PFKEY socket */

    if((NULL == sa_ptr) || (NULL == sp_ptr)) {
        return (OSAL_FAIL);
    }
    
    /* Open PFKEY socket */
    if (OSAL_SUCCESS != OSAL_netSocket(&socketId, OSAL_NET_SOCK_IPSEC)) {
        OSAL_logMsg("%s %d: Open socket failed.\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* add SA */
    if (OSAL_SUCCESS != _OSAL_ipsecSendSaMsg(&socketId, sa_ptr, SADB_ADD)) {
        OSAL_logMsg("%s %d: Add SA failed.\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* add SP */
    if (OSAL_SUCCESS != _OSAL_ipsecSendSpMsg(&socketId, sp_ptr, SADB_X_SPDADD)) {
        OSAL_logMsg("%s %d: Add SP failed.\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Close PFKEY socket*/
    if (OSAL_SUCCESS != OSAL_netCloseSocket(&socketId)) {
        OSAL_logMsg("%s %d: Close socket failed.\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
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
    OSAL_NetSockId socketId;    /* Socket id for PFKEY socket */

    if((NULL == sa_ptr) || (NULL == sp_ptr)) {
        return (OSAL_FAIL);
    }
    
    /* Open PFKEY socket */
    if (OSAL_SUCCESS != OSAL_netSocket(&socketId, OSAL_NET_SOCK_IPSEC)) {
        OSAL_logMsg("%s %d: Open socket failed.\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* delete SA */
    if (OSAL_SUCCESS != _OSAL_ipsecSendSaMsg(&socketId, sa_ptr, SADB_DELETE)) {
        OSAL_logMsg("%s %d: Delete SP failed.\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* delete SP */
    if (OSAL_SUCCESS != _OSAL_ipsecSendSpMsg(&socketId, sp_ptr, SADB_X_SPDDELETE)) {
        OSAL_logMsg("%s %d: Delete SP failed.\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Close PFKEY socket*/
    if (OSAL_SUCCESS != OSAL_netCloseSocket(&socketId)) {
        OSAL_logMsg("%s %d: Close socket failed.\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
#else
    return (OSAL_FAIL);
#endif // OSAL_IPSEC_ENABLE
}


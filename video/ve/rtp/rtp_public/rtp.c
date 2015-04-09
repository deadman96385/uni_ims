/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2008 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28394 $ $Date: 2014-08-21 10:24:05 +0800 (Thu, 21 Aug 2014) $
 */

/*
 * Includes.
 */
#include <rtp.h>

#ifdef VTSP_ENABLE_SRTP
# include "../rtp_private/include/srtp.h"
# include "../rtp_private/include/hmac.h" /* for hmac_ctx_t */
# include "../rtp_private/include/datatypes.h"

/*
 * Internal structure stores SRTP context information
 */
typedef struct {
    srtp_ctx_t              srtpContext;
    srtp_policy_t           srtpPolicyContext;

    srtp_stream_ctx_t       srtpStreamContext;
    cipher_t                srtpCipher;
    cipher_t                srtcpCipher;
    
    /* For the moment only aes_icm is used, null_cipher_ctx is a dummy */
    /* aes_cbc_ctx_t       srtpAesCbcCipherContext; */
    aes_icm_ctx_t           srtpAesIcmCipherContext;
    uint8                   srtpNullCipherContext;

    auth_t                  srtpAuth;
    auth_t                  srtcpAuth; 

    hmac_ctx_t              srtpHmacAuthContext;
    uint8                   srtpNullAuthContext;

    key_limit_ctx_t         srtpKeyLimit;

    kernel_cipher_type_t    srtpKernelNullCipehr;
    kernel_cipher_type_t    srtpKernelAesIcmCipehr;
    kernel_cipher_type_t    srtpKernelAesCbcCipehr;

    kernel_auth_type_t      srtpKernelNullAuth;
    kernel_auth_type_t      srtpKernelHmacAuth;

    kernel_debug_module_t   srtpCryptoKernelModule;
    kernel_debug_module_t   srtpAuthModule;
    kernel_debug_module_t   srtpCipherModule;
    kernel_debug_module_t   srtpStatModule;
    kernel_debug_module_t   srtpAllocModule;
    kernel_debug_module_t   srtpSrtpModule;
    kernel_debug_module_t   srtpNullCipherModule;
    kernel_debug_module_t   srtpAesIcmCipherModule;
    kernel_debug_module_t   srtpAesCbcCipherModule;
    kernel_debug_module_t   srtpNullAuthModule;
    kernel_debug_module_t   srtpHmacAuthModule;

} _SRTP_Internal;

/*
 * The srtp module has a global init function that should be called only once.
 * This variable keeps track of this init status.
 * XXX can this be moved somewhere else?
 */
vint _RTP_SRTP_GLOBAL_INIT = OSAL_FALSE;
#endif

/* 
 * ======== _RTP_makePreamble() ======== 
 * Packs the RTP header bits V, P, X, CC, M in one 32-bit word.
 * Preamble does not change frequently, hence computing and storing it 
 * increases efficiency.
 * 
 * Return Values:
 *
 */
static void _RTP_makePreamble (
    RTP_Obj *rtp_ptr)  /* RTP object pointer */
{
    
    uint32      temp32;
    RTP_MinHdr *hdr_ptr;
 
    hdr_ptr = &rtp_ptr->pkt.rtpMinHdr;
   
    /*
     * Pack options. Bit locations not expected to change hence not declared as
     * macros.
     */
    temp32 =  ((hdr_ptr->vers      & 0x3) << 30);   /* Version */
    temp32 |= ((hdr_ptr->padding   & 0x1) << 29);   /* Padding */
    temp32 |= ((hdr_ptr->extension & 0x1) << 28);   /* Extension */
    temp32 |= ((hdr_ptr->csrcCount & 0xF) << 24);   /* CSRC count */
    temp32 |= ((hdr_ptr->marker    & 0x1) << 23);   /* Marker */

    /*
     * Store for later use when forming packets.
     */
    rtp_ptr->preamble = temp32;
}



/* 
 * ======== _RTP_parsePreamble() ======== 
 * Unpacks the RTP header bits V, P, X, CC, M from one 32-bit preamble word.
 * 
 * Return Values:
 *
 */
static void _RTP_parsePreamble(
    RTP_Obj *rtp_ptr) /* RTP object pointer */
{
    
    uint32      temp32;
    RTP_MinHdr *hdr_ptr;
 
    hdr_ptr = &rtp_ptr->pkt.rtpMinHdr;
   
    /*
     * Unpack options. 
     * Bit locations not expected to change hence not declared as macros.
     */
    temp32 = rtp_ptr->preamble;
    
    hdr_ptr->vers      = (temp32 >> 30) & 0x3;  /* Version */
    hdr_ptr->padding   = (temp32 >> 29) & 0x1;  /* Padding */
    hdr_ptr->extension = (temp32 >> 28) & 0x1;  /* Extension */
    hdr_ptr->csrcCount = (temp32 >> 24) & 0xF;  /* CSRC count */
    hdr_ptr->marker    = (temp32 >> 23) & 0x1;  /* Marker */
}



/* 
 * ======== _RTP_packPkt() ======== 
 * Packs the RTP packet in RTP object pointed by rtp_ptr.
 * Packed packet is ready for xmit.
 * Required: &rtp_ptr->pkt != dst_ptr.
 * 
 * Return Values:
 *
 */
static void _RTP_packPkt(
    RTP_Obj *rtp_ptr,    /* RTP object pointer */
    uint8   *dst_ptr)    /* Packed packet destination pointer */
{
    RTP_RdnObj   *rdnObj_ptr;
    RTP_RdnCache *cache_ptr;
    RTP_MinHdr   *hdr_ptr;
    uint32       *packed_ptr;
    uint32       *csrcList_ptr;
    uint32       *payload_ptr;
    uint8        *packedData_ptr;
    uint8        *rdnHdr_ptr;
    int           i;
    uint32        word;
    int           csrcCount;
    int8          padSize;
    int8          payloadRemainder;
    int           idx;
    uint16        length;
    uint16        primDataSize;
    uint8         rdnEnabled;

    hdr_ptr      = &rtp_ptr->pkt.rtpMinHdr;
    csrcCount    = hdr_ptr->csrcCount;
    packed_ptr   = (uint32 *)dst_ptr;
    csrcList_ptr = rtp_ptr->pkt.csrcList;
    payload_ptr  = (uint32 *)rtp_ptr->pkt.payload;

    rdnEnabled = 0;
    if (NULL != rtp_ptr->rdnCache_ptr) {
        rdnEnabled = rtp_ptr->rdnCache_ptr->level;
    }
    /*
     * In order to copy primary payload (at the buttom of this function),
     * 'prtp_ptr->ayloadSize' must be kept as 'primDataSize',
     * since the total payload size would be enlarge by RFC2198 redundant data.
     */
    primDataSize = rtp_ptr->payloadSize;

    /*
     * Create packed packet, 32-bits at a time.
     */
    word  = (rtp_ptr->preamble);
    /* If RFC2198 enabled, PT is rdnDynType, otherwise regular type */
    word |= (((rdnEnabled ? rtp_ptr->rdnDynType : hdr_ptr->type) & 0x7F) << 16);
    word |= (hdr_ptr->seqn);
    *packed_ptr++ = OSAL_netHtonl(word);
    *packed_ptr++ = OSAL_netHtonl(hdr_ptr->ts);
    *packed_ptr++ = OSAL_netHtonl(hdr_ptr->ssrc);

    /*
     * Pack csrc list.
     */
    for (i = 0; i < csrcCount; i++) {
        word = *csrcList_ptr;
        *packed_ptr = OSAL_netHtonl(word);
        packed_ptr++;
        csrcList_ptr++;
    }
    packedData_ptr = NULL;

    if (rdnEnabled) {    /* RFC2198 enabled, pack redundant hdr & data */
        /* Book the start point of redundant headers */
        rdnHdr_ptr = (uint8*) packed_ptr;

        cache_ptr = rtp_ptr->rdnCache_ptr;
        rdnObj_ptr = cache_ptr->rdnObj_ary;
        /*
         * Get the start point of the primary encoding block header.
         * Thus we can pack redundant headers & data blocks simultaneously.
         */
        packedData_ptr = (uint8*) (packed_ptr + (int)cache_ptr->avail);

        /* Put the primary encoding block header :  |F=0| primary PT |   */
        *packedData_ptr++ = (0x7f & hdr_ptr->type); /* mask out MSB (F = 0) */

        /* looping to collect up to 'Level' elements from the cache buffer */
        idx = (int) cache_ptr->curr - cache_ptr->hop;
        for (i = 0; i < cache_ptr->avail; i++) {
            if (idx < 0) { /* ring buffer index goes over array's head */
                idx += RTP_REDUN_CACHE_MAX; /* wrap to the tail */
            }
            length = rdnObj_ptr[idx].hdr.blkLen;

            /* Pack redundant headers : |F=1| blkPT | TSoffset | blkLen | */
            word  = (0x03ff & length);
            word |= (0x00003fff & 
                    (hdr_ptr->ts - rdnObj_ptr[idx].hdr.tStamp)) << 10;
            word |= ((uint32) (0x80 | rdnObj_ptr[idx].hdr.blkPT) << 24); /* F=1 */
            *packed_ptr++ = OSAL_netHtonl(word);

            /* Copy redundant data blocks */
            RTP_MEMCPY(packedData_ptr, rdnObj_ptr[idx].data, length);
            /* leap to the location of next redundant data block */
            packedData_ptr += length;
            /* Hop to next candidate in the cache buffer */
            idx -= cache_ptr->hop;
        }
        /* Updata the total payload size */
        rtp_ptr->payloadSize += (int)(packedData_ptr - rdnHdr_ptr);
    }

    /*
     * Padding the payload.
     */
    if (hdr_ptr->padding) {
        payloadRemainder = rtp_ptr->payloadSize & 0x3;
        if (payloadRemainder) {
            padSize = 4 - payloadRemainder;
            for (i = 0; i < padSize; i++) {
                rtp_ptr->pkt.payload[rtp_ptr->payloadSize + i - 1] = 0;
            }
            rtp_ptr->pkt.payload[rtp_ptr->payloadSize + padSize - 1] = padSize;
            rtp_ptr->payloadSize += padSize;
        }
        else {
            hdr_ptr->padding = 0;
        }
    }

    /*
     * Copy primary payload which starts at payload_ptr.
     */
    if (rdnEnabled) {   /* Copy primary data block */
        RTP_MEMCPY(packedData_ptr, (uint8 *)payload_ptr, primDataSize);
    }
    else {   /* RFC2198 disabled, just copy primary data as payload */
        RTP_MEMCPY((uint8 *)packed_ptr, (uint8 *)payload_ptr, primDataSize);
    }

    /*
     * Store packed packet size in object.
     */
    rtp_ptr->packetSize = (3 << 2) + (csrcCount << 2) + rtp_ptr->payloadSize;
}



/* 
 * ======== _RTP_unpackPkt() ======== 
 * Unpacks the RTP packet in RTP object pointed by rtp_ptr.
 * Packed packet is received in network byte order.
 * rtp_ptr->packetSize must be set to size of src_ptr data before calling.
 * Required: &rtp_ptr->pkt != src_ptr.
 * 
 * Return Values:
 * RTP_OK:  Unpack successful.
 * RTP_ERR: Unpack not done due to error in packet.
 *
 */
static int _RTP_unpackPkt(
    RTP_Obj *rtp_ptr,         /* RTP object pointer */
    uint8   *src_ptr)         /* Packed packet source data pointer */
{
    RTP_MinHdr   *hdr_ptr;
    RTP_RdnObj   *rdnObj_ptr;
    RTP_RdnCache *cache_ptr;
    uint8        *rdnHdr_ptr;
    uint32       *packed_ptr;
    uint32       *csrcList_ptr;
    uint32       *payload_ptr;
    uint8        *packedData_ptr;
    uint32        temp32;
    int           ii;
    int           csrcCount;
    uint8         padSize;
    uint16        idx;
    uint16        rdnByteCnt;
    uint16        rdnBlkLen;
    uint16        primSize;
    uint8         rdnEnabled;
 
    hdr_ptr      = &rtp_ptr->pkt.rtpMinHdr;
    packed_ptr   = (uint32 *)src_ptr;
    csrcList_ptr = rtp_ptr->pkt.csrcList;
    payload_ptr  = (uint32 *)rtp_ptr->pkt.payload;
   
    /*
     * Get from packed packet, 32-bits at a time.
     */
    temp32            = OSAL_netNtohl(*packed_ptr);
    packed_ptr++;
    rtp_ptr->preamble = temp32 & 0xFF800000;
    hdr_ptr->type     = (temp32 >> 16) & 0x7F;
    hdr_ptr->seqn     = temp32 & 0xFFFF;
    hdr_ptr->ts       = OSAL_netNtohl(*packed_ptr);
    packed_ptr++;
    hdr_ptr->ssrc     = OSAL_netNtohl(*packed_ptr);
    packed_ptr++;
    _RTP_parsePreamble(rtp_ptr);

    /*
     * Return error if not a valid RTP packet version.
     */
    if (RTP_VERSION != hdr_ptr->vers) {
        return (RTP_ERR);
    }
    /*
     * Unpack csrc list.
     */
    csrcCount = hdr_ptr->csrcCount;
    for (ii = 0; ii < csrcCount; ii++) {
        *csrcList_ptr++ = OSAL_netNtohl(*packed_ptr);
        packed_ptr++;
    }

    rdnEnabled = 0;
    if (NULL != rtp_ptr->rdnCache_ptr) {
        rdnEnabled = rtp_ptr->rdnCache_ptr->avail;
    }
    /* If RFC2198 enabled, check is pkt's PT matthe dynamic redundant PT ? */
    if (rdnEnabled && (hdr_ptr->type == rtp_ptr->rdnDynType)) {
        rdnHdr_ptr  = (uint8*) packed_ptr;   /* start of redundant headers */
        rdnByteCnt  = 0;
        cache_ptr   = rtp_ptr->rdnCache_ptr;
        rdnObj_ptr  = cache_ptr->rdnObj_ary;
        idx         = 0;    /* Number of redundant data retrieved */
        temp32      = OSAL_netNtohl(*packed_ptr);
        while (0x80000000 & temp32) {       /*  is  MSB 'F' bit == 1  ??  */
            rdnBlkLen = (uint16) (temp32 & 0x3ff);
            if (idx < cache_ptr->avail) {
                /*
                 * Parse the info carried by the redundant header
                 * and store into the cache buffer.
                 */
                rdnObj_ptr[idx].hdr.blkLen = rdnBlkLen;
                rdnObj_ptr[idx].hdr.tStamp = (uint16) ((temp32 >> 10) & 0x3fff);
                rdnObj_ptr[idx].hdr.blkPT  = (uint8)  ((temp32 >> 24) & 0x7f);
                idx++;
            }

            /* Accumulate total size of redundant headers & data blocks */
            rdnByteCnt += (rdnBlkLen + RTP_REDUN_HEADER_LEN);
            temp32 = OSAL_netNtohl(*(++packed_ptr));
        }

        rdnByteCnt += RTP_REDUN_PRIMARY_HEADER_LEN;
        /* Calculate primary data block length */
        rtp_ptr->payloadSize = primSize =
                rtp_ptr->packetSize - (3 << 2) - (csrcCount << 2) - rdnByteCnt;

        packedData_ptr = (uint8*) packed_ptr;
        /* Get the primary payload type  (MSB 'F' bit must be 0) */
        hdr_ptr->type = *packedData_ptr++;

        /* Retrieve redundant data blocks */
        for (ii = 0; ii < idx; ii++) {
            rdnBlkLen = rdnObj_ptr[ii].hdr.blkLen;
            RTP_MEMCPY(rdnObj_ptr[ii].data, packedData_ptr, rdnBlkLen);
            packedData_ptr += rdnBlkLen;
        }
        cache_ptr->avail = idx;

        /* Move to the start point of primary payload, then copy primary data */
        packedData_ptr = rdnHdr_ptr + rdnByteCnt;
        RTP_MEMCPY((uint8 *)payload_ptr, packedData_ptr, primSize);
    }
    else {      /* Not an RFC2198 RTP packet, just pick the payload */
        /*
         * Store payload size in object.
         */
        rtp_ptr->payloadSize = 
            rtp_ptr->packetSize - (3 << 2) - (csrcCount << 2);
        /*
         * Copy payload.
         */
        RTP_MEMCPY((uint8 *)payload_ptr, (uint8 *)packed_ptr,
                rtp_ptr->payloadSize);
    }

    /*
     * Adjust for padding
     */
    if (hdr_ptr->padding) {
        padSize = rtp_ptr->pkt.payload[rtp_ptr->payloadSize - 1];
        if (padSize > rtp_ptr->payloadSize) {
            return (RTP_ERR);
        }
        else {
            rtp_ptr->payloadSize -= padSize;
        }
    }

    return (RTP_OK);
}



/* 
 * ======== RTP_setOpts() ======== 
 * Sets RTP options in the RTP header field.
 * opts = RTP_PADDING | RTP_EXTENSION | RTP_MARKER | CSRCCNT
 * 
 * Return Values:
 *
 */
void RTP_setOpts(
    RTP_Obj *rtp_ptr,  /* RTP object pointer */
    uint32   opts)     /* RTP options */
{
    RTP_MinHdr *hdr_ptr;
    
    hdr_ptr = &rtp_ptr->pkt.rtpMinHdr;

    /*
     * Clear previosuly set options.
     */
    hdr_ptr->padding   = 0;
    hdr_ptr->extension = 0;
    hdr_ptr->marker    = 0;

    /*
     * Padding, last octet must have padding length.
     */
    if (0 != (opts & RTP_PADDING)) {
        hdr_ptr->padding = 1;
    }
    
    /*
     * Header extension. Could be treated as a part of payload.
     */
    if (0 != (opts & RTP_EXTENSION)) {
        hdr_ptr->extension = 1;
    }
    
    /*
     * Set marker bits.
     */
    if (0 != (opts & RTP_MARKER)) {
        hdr_ptr->marker = 1;
    }

    /*
     * Set csrc count placed in lower 4 bits of the opts.
     * If csrcCnt != 0, csrcList in RTP object will be placed in the RTP 
     * header.
     */
    hdr_ptr->csrcCount = opts & 0x0F;

    /*
     * Update preamble for packets xmit.
     */
    _RTP_makePreamble(rtp_ptr);
}



/* 
 * ======== RTP_setCsrc() ======== 
 * Copies RTP csrc list from csrc_ptr to rtp_ptr->pkt.csrcList.
 * Entries copied will be equal to rtp_ptr->pkt.rtpMinHdr.csrcCount.
 * 
 * Return Values:
 *
 */
void RTP_setCsrc(
    RTP_Obj *rtp_ptr,
    uint32  *csrc_ptr)
{
    uint32 *list_ptr;
    int     i;
    int     count;

    count    = rtp_ptr->pkt.rtpMinHdr.csrcCount;
    list_ptr = rtp_ptr->pkt.csrcList;
    for (i = 0; i < count; i++) {
        *list_ptr++ = *csrc_ptr++;
    }
}



/* 
 * ======== RTP_getCsrc() ======== 
 * Copies RTP csrc list in rtp_ptr->pkt.csrcList to csrc_ptr.
 * Entries copied will be equal to rtp_ptr->pkt.rtpMinHdr.csrcCount.
 * 
 * Return Values:
 *
 */
void RTP_getCsrc(
    RTP_Obj *rtp_ptr,
    uint32  *csrc_ptr)
{
    uint32 *list_ptr;
    int     i;
    int     count;

    count    = rtp_ptr->pkt.rtpMinHdr.csrcCount;
    list_ptr = rtp_ptr->pkt.csrcList;
    for (i = 0; i < count; i++) {
        *csrc_ptr++ = *list_ptr++;
    }
}

#ifdef VTSP_ENABLE_SRTP
/*
 * ======== RTP_srtpInit ========
 */
static vint _RTP_srtpInit(
    void           *context_ptr,
    _SRTP_Policy   *policy_ptr)
{
    _SRTP_Internal  *srtpInternal_ptr;
    srtp_policy_t   *srtpPolicy_ptr;
    cipher_t        *srtpCipher_ptr;
    cipher_t        *srtcpCipher_ptr;
    auth_t          *srtpAuth_ptr;
    auth_t          *srtcpAuth_ptr;
    sec_serv_t       sec_servs;
    vint             srtpStatus;

    srtpInternal_ptr = (_SRTP_Internal *)context_ptr;

    srtpCipher_ptr = &srtpInternal_ptr->srtpCipher;
    srtcpCipher_ptr = &srtpInternal_ptr->srtcpCipher;
    srtpAuth_ptr = &srtpInternal_ptr->srtpAuth;
    srtcpAuth_ptr = &srtpInternal_ptr->srtcpAuth;

    /*
     * Initialize the security policy
     */
    srtpPolicy_ptr  = &srtpInternal_ptr->srtpPolicyContext;
    sec_servs       = sec_serv_none;

    if (RTP_SECURITY_SERVICE_NONE != policy_ptr->serviceType) {
        /*
         * Set up the security policy for the srtp software.
         * XXX Note that the implementation of RTCP is incomplete
         * and the initialization of RTCP structures and contexts
         * has been commented out, as it conflicts with the static
         * memory model as it has been written.
         */
        switch (policy_ptr->serviceType) {
            case RTP_SECURITY_SERVICE_CONF_AUTH_80:
                crypto_policy_set_rtp_default(&srtpPolicy_ptr->rtp);
                /* crypto_policy_set_rtcp_default(&srtpPolicy_ptr->rtcp);*/
                sec_servs = sec_serv_conf_and_auth;
                srtpCipher_ptr->state = 
                    &srtpInternal_ptr->srtpAesIcmCipherContext;
                srtpAuth_ptr->state = &srtpInternal_ptr->srtpHmacAuthContext;
                break;
            case RTP_SECURITY_SERVICE_CONF_AUTH_32:
                crypto_policy_set_aes_cm_128_hmac_sha1_32(&srtpPolicy_ptr->rtp);
                /* crypto_policy_set_rtcp_default(&srtpPolicy_ptr->rtcp);*/
                sec_servs = sec_serv_conf_and_auth;
                srtpCipher_ptr->state = 
                    &srtpInternal_ptr->srtpAesIcmCipherContext;
                srtpAuth_ptr->state = &srtpInternal_ptr->srtpHmacAuthContext;
                break;
#if 0
            case RTP_SECURITY_SERVICE_CONF:
                crypto_policy_set_aes_cm_128_null_auth(&srtpPolicy_ptr->rtp);
                /* crypto_policy_set_rtcp_default(&srtpPolicy_ptr->rtcp); */     
                sec_servs = sec_serv_conf;
                srtpCipher_ptr->state = 
                        &srtpInternal_ptr->srtpAesIcmCipherContext;
                srtpAuth_ptr->state = &srtpInternal_ptr->srtpNullAuthContext;
                break;
            case RTP_SECURITY_SERVICE_AUTH_80:
                crypto_policy_set_null_cipher_hmac_sha1_80(
                                                &srtpPolicy_ptr->rtp);
                /* crypto_policy_set_rtcp_default(&srtpPolicy_ptr->rtcp); */
                sec_servs = sec_serv_auth;
                srtpCipher_ptr->state = 
                        &srtpInternal_ptr->srtpNullCipherContext;
                srtpAuth_ptr->state = &srtpInternal_ptr->srtpHmacAuthContext;
                break;
            case RTP_SECURITY_SERVICE_AUTH_32:
                crypto_policy_set_null_cipher_hmac_sha1_32(
                        &srtpPolicy_ptr->rtp);
                /* crypto_policy_set_rtcp_default(&srtpPolicy_ptr->rtcp); */
                sec_servs = sec_serv_auth;
                srtpCipher_ptr->state = 
                        &srtpInternal_ptr->srtpNullCipherContext;
                srtpAuth_ptr->state = &srtpInternal_ptr->srtpHmacAuthContext;
                break;
#endif
            default:
                /* XXX error condition */
                break;
        } 
        srtpPolicy_ptr->ssrc.type  = ssrc_specific;
        srtpPolicy_ptr->ssrc.value = policy_ptr->ssrcInit;
        srtpPolicy_ptr->key  = policy_ptr->key;
        srtpPolicy_ptr->next = NULL;
        srtpPolicy_ptr->rtp.sec_serv = sec_servs;
        /* RTCP not supported in this implementation */
        srtpPolicy_ptr->rtcp.sec_serv = sec_serv_none;
        srtcpCipher_ptr->state = &srtpInternal_ptr->srtpNullCipherContext;
        srtcpAuth_ptr->state = &srtpInternal_ptr->srtpNullAuthContext;
    }
    else {
        /*
         * Set up NULL policy services
         * This is not really complaint, because RTCP is suppose to
         * always have authentication according to the RFC.
         */
        srtpPolicy_ptr->key                 = policy_ptr->key;
        srtpPolicy_ptr->ssrc.type           = ssrc_specific;
        srtpPolicy_ptr->ssrc.value          = policy_ptr->ssrcInit;
        srtpPolicy_ptr->rtp.cipher_type     = NULL_CIPHER;
        srtpPolicy_ptr->rtp.cipher_key_len  = 0; 
        srtpPolicy_ptr->rtp.auth_type       = NULL_AUTH;
        srtpPolicy_ptr->rtp.auth_key_len    = 0;
        srtpPolicy_ptr->rtp.auth_tag_len    = 0;
        srtpPolicy_ptr->rtp.sec_serv        = sec_serv_none;   
        srtpPolicy_ptr->rtcp.cipher_type    = NULL_CIPHER;
        srtpPolicy_ptr->rtcp.cipher_key_len = 0; 
        srtpPolicy_ptr->rtcp.auth_type      = NULL_AUTH;
        srtpPolicy_ptr->rtcp.auth_key_len   = 0;
        srtpPolicy_ptr->rtcp.auth_tag_len   = 0;
        srtpPolicy_ptr->rtcp.sec_serv       = sec_serv_none;   
        srtpPolicy_ptr->next                = NULL;
        srtpCipher_ptr->state = &srtpInternal_ptr->srtpNullCipherContext;
        srtpAuth_ptr->state = &srtpInternal_ptr->srtpNullAuthContext;
        srtcpCipher_ptr->state = &srtpInternal_ptr->srtpNullCipherContext; 
        srtcpAuth_ptr->state = &srtpInternal_ptr->srtpNullAuthContext;
    }

    srtpInternal_ptr->srtpStreamContext.rtp_cipher = srtpCipher_ptr;
    srtpInternal_ptr->srtpStreamContext.rtcp_cipher = srtcpCipher_ptr;
    srtpInternal_ptr->srtpStreamContext.rtp_auth = srtpAuth_ptr;
    srtpInternal_ptr->srtpStreamContext.rtcp_auth = srtcpAuth_ptr;
    srtpInternal_ptr->srtpStreamContext.limit = &srtpInternal_ptr->srtpKeyLimit;

    /*
     * Init the srtp module with the srtp policy parameters set up above
     */
    srtpStatus = srtp_create(
            (srtp_t) &srtpInternal_ptr->srtpContext, srtpPolicy_ptr,
            &srtpInternal_ptr->srtpStreamContext);


    if (srtpStatus) {
        /* XXX add error check */
    }

    return (RTP_OK);

}
#endif

/* 
 * ======== RTP_redunInit() ========
 *   Allocate/initialize resources for RFC2198
 */
void RTP_redunInit(
    RTP_Obj *rtp_ptr)
{
#ifdef VTSP_ENABLE_RTP_REDUNDANT
    rtp_ptr->rdnCache_ptr = (RTP_RdnCache *)
            OSAL_memCalloc(1, sizeof(RTP_RdnCache), 0);

    if (NULL == rtp_ptr->rdnCache_ptr) { /* In case of allocation failure */
        OSAL_logMsg("%s:%d", __FILE__, __LINE__);
    }
    else {
        /* Disable RFC2198 run-time by default */
        rtp_ptr->rdnCache_ptr->level = 0;
        rtp_ptr->rdnCache_ptr->hop   = 0;
    }
#else
    rtp_ptr->rdnCache_ptr = NULL; /* No 2198 support */
#endif
}

/*
 * ======== RTP_redunShutdown() ========
 *   Free resources for RFC2198
 */
void RTP_redunShutdown(
    RTP_Obj *rtp_ptr)
{
#ifdef VTSP_ENABLE_RTP_REDUNDANT
    if (rtp_ptr->rdnCache_ptr) {
        OSAL_memFree(rtp_ptr->rdnCache_ptr, 0);
    }
#endif
}

/*
 * ======== RTP_redunReset() ========
 *
 * Return Values:
 *
 */
void RTP_redunReset(
    RTP_Obj *rtp_ptr,
    uint8    rdnDynType)
{
    /*
     * Assign the dynamic payload type for RFC2198.
     */
    if ((RTP_REDUN_DYN_TYPE_MIN <= rdnDynType) &&
            (rdnDynType <= RTP_REDUN_DYN_TYPE_MAX)) {
        rtp_ptr->rdnDynType = rdnDynType;
    }
    else {  /* dynamic type out of range, use default */
        rtp_ptr->rdnDynType = RTP_REDUN_DYN_TYPE_DEFAULT;
    }

    /* Reset the redundant cache buffer */
    if (rtp_ptr->rdnCache_ptr) {
        rtp_ptr->rdnCache_ptr->curr  = 0;
        rtp_ptr->rdnCache_ptr->avail = 0;
        rtp_ptr->rdnCache_ptr->rem   = 0;
    }
}

/*
 * ======== RTP_init() ======== 
 * Inits the RTP object.
 * Init one object per direction per session.
 * 
 * Return Values:
 *
 */
void RTP_init(
    RTP_Obj *rtp_ptr)
{
    RTP_MinHdr         *hdr_ptr;
    
#ifdef VTSP_ENABLE_SRTP
    _SRTP_Internal     *srtpInternal_ptr;
#endif
    volatile uint32     random;
        
    hdr_ptr = &rtp_ptr->pkt.rtpMinHdr;

#ifdef VTSP_ENABLE_SRTP
    srtpInternal_ptr = (_SRTP_Internal *)rtp_ptr->context_ptr;
#endif
    /*
     * Set RTP version.
     */
    hdr_ptr->vers = RTP_VERSION;

    /*
     * Set initial sequence number.
     */
    if (rtp_ptr->seqRandom) {
        OSAL_randomGetOctets((char *)&random, sizeof(random));
        hdr_ptr->seqn = random;
    }
    else {
        hdr_ptr->seqn = 0;
    }
   
    /*
     * Set initial timestamp.
     */
    if (rtp_ptr->tsRandom) {
        OSAL_randomGetOctets((char *)&random, sizeof(random));
        hdr_ptr->ts = random;
        rtp_ptr->tStamp = random;
    }
    else {
        hdr_ptr->ts = 0;
        rtp_ptr->tStamp = 0;
    }
    /*
     * Init RTP options at init.
     */
    hdr_ptr->padding   = 0;
    hdr_ptr->marker    = 0;
    hdr_ptr->extension = 0;
    hdr_ptr->csrcCount = 0;

    /*
     * Update preamble for packets xmit.
     */
    _RTP_makePreamble(rtp_ptr);

    /*
     * Set packet and payload sizes to zero.
     */
    rtp_ptr->packetSize  = 0;
    rtp_ptr->payloadSize = 0;
    
#ifdef VTSP_ENABLE_SRTP
    
    if (OSAL_FALSE == _RTP_SRTP_GLOBAL_INIT) {
        /* Call the one time global initialization function */
        _RTP_SRTP_GLOBAL_INIT = OSAL_TRUE;
        srtp_init(&srtpInternal_ptr->srtpKernelNullCipehr,
                  &srtpInternal_ptr->srtpKernelAesIcmCipehr,
                  &srtpInternal_ptr->srtpKernelAesCbcCipehr,
                  &srtpInternal_ptr->srtpKernelNullAuth,
                  &srtpInternal_ptr->srtpKernelHmacAuth,
                  &srtpInternal_ptr->srtpCryptoKernelModule,
                  &srtpInternal_ptr->srtpAuthModule,
                  &srtpInternal_ptr->srtpCipherModule,
                  &srtpInternal_ptr->srtpStatModule,
                  &srtpInternal_ptr->srtpAllocModule,
                  &srtpInternal_ptr->srtpSrtpModule,
                  &srtpInternal_ptr->srtpNullCipherModule,
                  &srtpInternal_ptr->srtpAesIcmCipherModule,
                  &srtpInternal_ptr->srtpAesCbcCipherModule,
                  &srtpInternal_ptr->srtpNullAuthModule,
                  &srtpInternal_ptr->srtpHmacAuthModule);
    }
#endif
    /*
     * Set SSRC.
     * XXX: Do collision detection by checking RTP objects of other connections
     * for same ssrc number.
     */
    OSAL_randomGetOctets((char *)&random, sizeof(random));
    hdr_ptr->ssrc = random;   
}

/*
 * ======== RTP_shutdown() ========
 *   Shutdown RTP module
 */
void RTP_shutdown(
    RTP_Obj *rtp_ptr)
{
    RTP_redunShutdown(rtp_ptr);
}


#ifdef VTSP_ENABLE_SRTP
void RTP_srtpinit(
    RTP_Obj          *rtp_ptr,
    RTP_SecurityType    serviceType,
    char               *key_ptr,
    uvint               keyLen)
{

    RTP_MinHdr         *hdr_ptr;
    int                 count;
    
    _SRTP_Policy       *policy_ptr;
    
    hdr_ptr = &rtp_ptr->pkt.rtpMinHdr;

    policy_ptr = &rtp_ptr->srtpPolicy;
        
    /*
     * Prepare the security key
     * and copy it to SRTP internal
     */
    if (RTP_KEY_STRING_MAX_LEN < keyLen) {
        keyLen = RTP_KEY_STRING_MAX_LEN;
        OSAL_logMsg("%s:%d keyLen is too large\n", __FILE__, __LINE__);
    }

    for (count = 0; count < keyLen; count++) {
        policy_ptr->key[count] = key_ptr[count];
    }

    /*
     * Initiialize SRTP
     * First, get policy parameters from RTP_init parameters
     */
    if (serviceType != RTP_SECURITY_SERVICE_NONE &&
            serviceType != RTP_SECURITY_SERVICE_CONF_AUTH_80 &&
            serviceType != RTP_SECURITY_SERVICE_CONF_AUTH_32) {
        
        /* Set null cipher if policy is not valid */
        serviceType = RTP_SECURITY_SERVICE_NONE;
    }
    policy_ptr->serviceType = serviceType;
    
    policy_ptr->ssrcInit = hdr_ptr->ssrc;
    
    _RTP_srtpInit(rtp_ptr->context_ptr, policy_ptr);

    /* The SSRC must come from the SRTP policy */
    hdr_ptr->ssrc = policy_ptr->ssrcInit;
}
#endif

/* 
 * ======== _RTP_redunCache() ========
 * Caching some current sending packet's infomation into cache buffer.
 *      (RFC2198 support)
 *
 * Return Values:
 *
 */
static void _RTP_redunCache(
    RTP_RdnCache *cache_ptr,   /* RTP redundant cache */
    uint8         payloadType, /* RTP payload type   */
    uint32        timestamp,   /* RTP timestamp      */
    uint16        payloadSize, /* RTP payload size   */
    uint8        *payload)     /* RTP payload data   */
{
    RTP_RdnObj  *rdnObj_ptr;

    rdnObj_ptr = &(cache_ptr->rdnObj_ary[cache_ptr->curr]);
    rdnObj_ptr->hdr.blkLen = payloadSize;
    rdnObj_ptr->hdr.blkPT  = payloadType;
    rdnObj_ptr->hdr.tStamp = timestamp;
    OSAL_memCpy(rdnObj_ptr->data, payload, payloadSize);

    /*  Move to next element in the ring buffer  */
    if (++cache_ptr->curr >= RTP_REDUN_CACHE_MAX) {
        cache_ptr->curr = 0;    /* reach end of array, wrap to the head */
    }

    if (cache_ptr->avail < cache_ptr->level) {
        if (++(cache_ptr->rem) >= cache_ptr->hop) {
            (cache_ptr->avail)++;
            cache_ptr->rem = 0;
        }
    }
}

/*
 * ======== RTP_xmitPkt() ======== 
 * Xmits the RTP packet (to UDP etc.).
 * 
 * Return Values:
 *
 */
vint RTP_xmitPkt(
    RTP_Obj *rtp_ptr,       /* RTP object pointer */
    uint8   *buf_ptr,     /* Scratch space or a network buffer location */
    uint16   primDataSize,  /* RTP primary payload size */
    uint8    payloadType)   /* RTP primary payload type */
{
    RTP_MinHdr     *hdr_ptr;
    RTP_RdnCache   *cache_ptr;
    
#ifdef VTSP_ENABLE_SRTP
    vint            srtpStatus;
    vint            srtpPacketLen;

    _SRTP_Internal *srtpInternal_ptr;
    srtpInternal_ptr = (_SRTP_Internal *)rtp_ptr->context_ptr;
#endif

    hdr_ptr = &rtp_ptr->pkt.rtpMinHdr;
    /*
     * Set payload size.
     */
    rtp_ptr->payloadSize = primDataSize;

    /*
     * Set payload type.
     */
    hdr_ptr->type = payloadType;

    /*
     * Set the timestamp.
     */
    hdr_ptr->ts = rtp_ptr->tStamp;
    
    /*
     * Pack packet in network buffer pointed by buf_ptr. Usually buf_ptr points
     * to starting location of UDP payload.
     */
    _RTP_packPkt(rtp_ptr, buf_ptr);

    /*
     * Cache packet into cache buffer. (RFC2198 support)
     */
    cache_ptr = rtp_ptr->rdnCache_ptr;
    if (NULL != cache_ptr) { /* RFC2198 enabled */
        if (cache_ptr->level) {
            _RTP_redunCache(cache_ptr, payloadType, rtp_ptr->tStamp,
                    primDataSize, rtp_ptr->pkt.payload);
        }
    }
    /*
     * Increment sequence number.
     */
    hdr_ptr->seqn++;

#ifdef VTSP_ENABLE_SRTP
    /*
     * Encript the packet.
     * 'srtpPacketLen' must base on 'rtp_ptr->payloadSize', it includes
     * the primary payload size  &  the size of all RFC2198 redundant data
     * that was accumulated by _RTP_packPkt() when doing RFC2198 process.
     */
    srtpPacketLen = rtp_ptr->payloadSize + 12;   /* XXX check this */
    srtpStatus = srtp_protect((srtp_t) &srtpInternal_ptr->srtpContext,
                    buf_ptr, &srtpPacketLen);
    
    if (srtpStatus) {
        OSAL_logMsg("error: srtp protection failed with code %d%s\n",
            srtpStatus,
            srtpStatus == err_status_cipher_fail ? " (encryption failed)" :
            srtpStatus == err_status_auth_fail ? " (authorization failed)" : "");
    }

    /*
     * Update packet length
     */
    rtp_ptr->packetSize = (uint16)srtpPacketLen;
#endif

    /*
     * UDP can handle packet from here on.
     */

    /* 
     * SRTP determines the size because:
     *      - additional fields can be added
     *      - the encripted packet can be the same size or larger than original
     */
    return (rtp_ptr->packetSize);
}



/* 
 * ======== RTP_recvPkt() ======== 
 * Receives and parses an RTP packet (from UDP etc.).
 * 
 * Return Values:
 * RTP_OK:  Recv successful.
 * RTP_ERR: Recv not done due to error in packet.
 *
 */
vint RTP_recvPkt(
    RTP_Obj *rtp_ptr,     /* RTP object pointer */
    uint8   *buf_ptr,     /* Network or scratch buffer that has RTP packet */
    uint16   packetSize)  /* RTP packet size */
{
    vint            retVal;

#ifdef VTSP_ENABLE_SRTP    
    vint            srtpStatus;
    vint            srtpPacketLen;
    _SRTP_Internal *srtpInternal_ptr;
    _SRTP_Policy   *policy_ptr;
     srtp_hdr_t    *packedHdr_ptr;
#endif     


#ifdef VTSP_ENABLE_SRTP
    srtpInternal_ptr = (_SRTP_Internal *)rtp_ptr->context_ptr;
    packedHdr_ptr = (srtp_hdr_t *)buf_ptr;
    policy_ptr = &rtp_ptr->srtpPolicy;
    /*
     * If a new ssrc has been encountered, re-init recvObj with new ssrc
     */
    if (OSAL_netNtohl(packedHdr_ptr->ssrc) != policy_ptr->ssrcInit) {
        policy_ptr->ssrcInit = OSAL_netNtohl(packedHdr_ptr->ssrc);
        _RTP_srtpInit(rtp_ptr->context_ptr, policy_ptr);
    }
#endif

    /*
     * Set packet size.
     */
    rtp_ptr->packetSize = packetSize;

#ifdef VTSP_ENABLE_SRTP
    srtpPacketLen = packetSize;
    srtpStatus = srtp_unprotect((srtp_t) &srtpInternal_ptr->srtpContext,
                    buf_ptr, &srtpPacketLen);

    /* 
     * SRTP determines the size because:
     *      - additional fields may have been removed
     *      - the encripted packet can be the same size or larger than original
     */
    rtp_ptr->packetSize = (uint16)srtpPacketLen;
#endif

    /*
     * Unpack packet in network buffer pointed by buf_ptr. 
     * Usually buf_ptr points to starting location of UDP payload.
     */
    retVal = _RTP_unpackPkt(rtp_ptr, buf_ptr);
     
#ifdef VTSP_ENABLE_SRTP
    if (srtpStatus) {
        OSAL_logMsg("error: srtp unprotection failed with code %d%s\n",
            srtpStatus,
            srtpStatus == err_status_replay_fail ? " (replay check failed)" :
            srtpStatus == err_status_auth_fail ? " (auth check failed)" : "");

        return RTP_ERR;
    }
#endif
    return (retVal);
}

/* 
 * ======== RTP_getContextSize() ======== 
 * Returns the size of the internal structure holding context information.
 * 
 * Return Values:
 * 
 */

uvint RTP_getContextSize(
        void)
{
#ifdef VTSP_ENABLE_SRTP
    return ((uvint)sizeof(_SRTP_Internal));
#else
    return (0);
#endif
}


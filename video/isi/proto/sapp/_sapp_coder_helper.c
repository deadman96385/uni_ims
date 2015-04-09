/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 15584 $ $Date: 2011-08-29 14:04:45 +0800 (Mon, 29 Aug 2011) $
 */
#include <osal_types.h>
#include <osal.h>
#include <osal_msg.h>

#include <auth_b64.h>

#include <ezxml.h>

#include <sip_sip.h>
#include <sip_xport.h>
#include <sip_ua.h>
#include <sip_app.h>
#include <sip_debug.h>
#include <sip_session.h>
#include <sip_cfg.h>

#include "isi.h"
#include "isip.h"

#include "_sapp.h"
#include "_sapp_coder_helper.h"
#include "_sapp_dialog.h"

#define GAMRNB_MAX_MODE_SET             8
#define GAMRWB_MAX_MODE_SET             9
#define GAMRNB_BE_IPV4_MODE_SET_0       22
#define GAMRNB_BE_IPV4_MODE_SET_1       22
#define GAMRNB_BE_IPV4_MODE_SET_2       23
#define GAMRNB_BE_IPV4_MODE_SET_3       24
#define GAMRNB_BE_IPV4_MODE_SET_4       24
#define GAMRNB_BE_IPV4_MODE_SET_5       25
#define GAMRNB_BE_IPV4_MODE_SET_6       27
#define GAMRNB_BE_IPV4_MODE_SET_7       29
#define GAMRNB_OA_IPV4_MODE_SET_0       22
#define GAMRNB_OA_IPV4_MODE_SET_1       22
#define GAMRNB_OA_IPV4_MODE_SET_2       23
#define GAMRNB_OA_IPV4_MODE_SET_3       24
#define GAMRNB_OA_IPV4_MODE_SET_4       25
#define GAMRNB_OA_IPV4_MODE_SET_5       25
#define GAMRNB_OA_IPV4_MODE_SET_6       28
#define GAMRNB_OA_IPV4_MODE_SET_7       30
#define GAMRWB_BE_IPV4_MODE_SET_0       24
#define GAMRWB_BE_IPV4_MODE_SET_1       26
#define GAMRWB_BE_IPV4_MODE_SET_2       30
#define GAMRWB_BE_IPV4_MODE_SET_3       31
#define GAMRWB_BE_IPV4_MODE_SET_4       33
#define GAMRWB_BE_IPV4_MODE_SET_5       35
#define GAMRWB_BE_IPV4_MODE_SET_6       37
#define GAMRWB_BE_IPV4_MODE_SET_7       40
#define GAMRWB_BE_IPV4_MODE_SET_8       41
#define GAMRWB_OA_IPV4_MODE_SET_0       24
#define GAMRWB_OA_IPV4_MODE_SET_1       26
#define GAMRWB_OA_IPV4_MODE_SET_2       30
#define GAMRWB_OA_IPV4_MODE_SET_3       32
#define GAMRWB_OA_IPV4_MODE_SET_4       33
#define GAMRWB_OA_IPV4_MODE_SET_5       36
#define GAMRWB_OA_IPV4_MODE_SET_6       37
#define GAMRWB_OA_IPV4_MODE_SET_7       40
#define GAMRWB_OA_IPV4_MODE_SET_8       41
#define GAMRNB_BE_IPV6_MODE_SET_0       30
#define GAMRNB_BE_IPV6_MODE_SET_1       30
#define GAMRNB_BE_IPV6_MODE_SET_2       31
#define GAMRNB_BE_IPV6_MODE_SET_3       32
#define GAMRNB_BE_IPV6_MODE_SET_4       32
#define GAMRNB_BE_IPV6_MODE_SET_5       33
#define GAMRNB_BE_IPV6_MODE_SET_6       35
#define GAMRNB_BE_IPV6_MODE_SET_7       37
#define GAMRNB_OA_IPV6_MODE_SET_0       30
#define GAMRNB_OA_IPV6_MODE_SET_1       30
#define GAMRNB_OA_IPV6_MODE_SET_2       31
#define GAMRNB_OA_IPV6_MODE_SET_3       32
#define GAMRNB_OA_IPV6_MODE_SET_4       33
#define GAMRNB_OA_IPV6_MODE_SET_5       33
#define GAMRNB_OA_IPV6_MODE_SET_6       36
#define GAMRNB_OA_IPV6_MODE_SET_7       38
#define GAMRWB_BE_IPV6_MODE_SET_0       32
#define GAMRWB_BE_IPV6_MODE_SET_1       34
#define GAMRWB_BE_IPV6_MODE_SET_2       38
#define GAMRWB_BE_IPV6_MODE_SET_3       39
#define GAMRWB_BE_IPV6_MODE_SET_4       41
#define GAMRWB_BE_IPV6_MODE_SET_5       43
#define GAMRWB_BE_IPV6_MODE_SET_6       45
#define GAMRWB_BE_IPV6_MODE_SET_7       48
#define GAMRWB_BE_IPV6_MODE_SET_8       49
#define GAMRWB_OA_IPV6_MODE_SET_0       32
#define GAMRWB_OA_IPV6_MODE_SET_1       34
#define GAMRWB_OA_IPV6_MODE_SET_2       38
#define GAMRWB_OA_IPV6_MODE_SET_3       40
#define GAMRWB_OA_IPV6_MODE_SET_4       41
#define GAMRWB_OA_IPV6_MODE_SET_5       44
#define GAMRWB_OA_IPV6_MODE_SET_6       45
#define GAMRWB_OA_IPV6_MODE_SET_7       48
#define GAMRWB_OA_IPV6_MODE_SET_8       49

static vint _gamrwb_bandwidthTableIpv4[][GAMRWB_MAX_MODE_SET] = {{
     GAMRWB_BE_IPV4_MODE_SET_0, 
     GAMRWB_BE_IPV4_MODE_SET_1,
     GAMRWB_BE_IPV4_MODE_SET_2,
     GAMRWB_BE_IPV4_MODE_SET_3,
     GAMRWB_BE_IPV4_MODE_SET_4,
     GAMRWB_BE_IPV4_MODE_SET_5,
     GAMRWB_BE_IPV4_MODE_SET_6,
     GAMRWB_BE_IPV4_MODE_SET_7,
     GAMRWB_BE_IPV4_MODE_SET_8},{
     GAMRWB_OA_IPV4_MODE_SET_0, 
     GAMRWB_OA_IPV4_MODE_SET_1,
     GAMRWB_OA_IPV4_MODE_SET_2,
     GAMRWB_OA_IPV4_MODE_SET_3,
     GAMRWB_OA_IPV4_MODE_SET_4,
     GAMRWB_OA_IPV4_MODE_SET_5,
     GAMRWB_OA_IPV4_MODE_SET_6,
     GAMRWB_OA_IPV4_MODE_SET_7,
     GAMRWB_OA_IPV4_MODE_SET_8}};
static vint _gamrwb_bandwidthTableIpv6[][GAMRWB_MAX_MODE_SET] = {{
     GAMRWB_BE_IPV6_MODE_SET_0, 
     GAMRWB_BE_IPV6_MODE_SET_1,
     GAMRWB_BE_IPV6_MODE_SET_2,
     GAMRWB_BE_IPV6_MODE_SET_3,
     GAMRWB_BE_IPV6_MODE_SET_4,
     GAMRWB_BE_IPV6_MODE_SET_5,
     GAMRWB_BE_IPV6_MODE_SET_6,
     GAMRWB_BE_IPV6_MODE_SET_7,
     GAMRWB_BE_IPV6_MODE_SET_8},{
     GAMRWB_OA_IPV6_MODE_SET_0, 
     GAMRWB_OA_IPV6_MODE_SET_1,
     GAMRWB_OA_IPV6_MODE_SET_2,
     GAMRWB_OA_IPV6_MODE_SET_3,
     GAMRWB_OA_IPV6_MODE_SET_4,
     GAMRWB_OA_IPV6_MODE_SET_5,
     GAMRWB_OA_IPV6_MODE_SET_6,
     GAMRWB_OA_IPV6_MODE_SET_7,
     GAMRWB_OA_IPV6_MODE_SET_8}};

static vint _gamrnb_bandwidthTableIpv4[][GAMRNB_MAX_MODE_SET] = {{
     GAMRNB_BE_IPV4_MODE_SET_0, 
     GAMRNB_BE_IPV4_MODE_SET_1,
     GAMRNB_BE_IPV4_MODE_SET_2,
     GAMRNB_BE_IPV4_MODE_SET_3,
     GAMRNB_BE_IPV4_MODE_SET_4,
     GAMRNB_BE_IPV4_MODE_SET_5,
     GAMRNB_BE_IPV4_MODE_SET_6,
     GAMRNB_BE_IPV4_MODE_SET_7},{
     GAMRNB_OA_IPV4_MODE_SET_0, 
     GAMRNB_OA_IPV4_MODE_SET_1,
     GAMRNB_OA_IPV4_MODE_SET_2,
     GAMRNB_OA_IPV4_MODE_SET_3,
     GAMRNB_OA_IPV4_MODE_SET_4,
     GAMRNB_OA_IPV4_MODE_SET_5,
     GAMRNB_OA_IPV4_MODE_SET_6,
     GAMRNB_OA_IPV4_MODE_SET_7}};
static vint _gamrnb_bandwidthTableIpv6[][GAMRNB_MAX_MODE_SET] = {{
     GAMRNB_BE_IPV6_MODE_SET_0, 
     GAMRNB_BE_IPV6_MODE_SET_1,
     GAMRNB_BE_IPV6_MODE_SET_2,
     GAMRNB_BE_IPV6_MODE_SET_3,
     GAMRNB_BE_IPV6_MODE_SET_4,
     GAMRNB_BE_IPV6_MODE_SET_5,
     GAMRNB_BE_IPV6_MODE_SET_6,
     GAMRNB_BE_IPV6_MODE_SET_7},{
     GAMRNB_OA_IPV6_MODE_SET_0, 
     GAMRNB_OA_IPV6_MODE_SET_1,
     GAMRNB_OA_IPV6_MODE_SET_2,
     GAMRNB_OA_IPV6_MODE_SET_3,
     GAMRNB_OA_IPV6_MODE_SET_4,
     GAMRNB_OA_IPV6_MODE_SET_5,
     GAMRNB_OA_IPV6_MODE_SET_6,
     GAMRNB_OA_IPV6_MODE_SET_7}};     

static vint _SAPP_isH263(
    char *coderName_ptr,
    vint  maxCoderLen)
{
    if (!OSAL_strncasecmp(_SAPP_neg_h263_1998, coderName_ptr, maxCoderLen) ||
            !OSAL_strncasecmp(_SAPP_neg_h263_2000, coderName_ptr, maxCoderLen) ||
            !OSAL_strncasecmp(_SAPP_neg_h263, coderName_ptr, maxCoderLen)) {
        return SAPP_OK;
    }
    return (SAPP_ERR);
}

static void _SAPP_decodeEnumRate(
    char       *desc_ptr,
    SAPP_Coder *coder_ptr,
    char       *delimiter_ptr)
{
    char   *s_ptr;
    int    *encInt_ptr;
    int    *decInt_ptr;
    char   *enc_ptr;

    s_ptr = OSAL_strtok(desc_ptr, delimiter_ptr);
    while (NULL != s_ptr) {
        encInt_ptr = decInt_ptr = NULL;
        
        if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_enum)) {
            encInt_ptr = &coder_ptr->coderNum;
            decInt_ptr = &coder_ptr->decoderNum;
        }
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_rate)) {
            encInt_ptr = decInt_ptr = &coder_ptr->rate;
        }
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_maxptime)) {
            encInt_ptr = decInt_ptr = &coder_ptr->maxPacketRate;
        }

        /*
         * We got token, let's get the value.
         */
        s_ptr = OSAL_strtok(NULL, delimiter_ptr);
        if (NULL == s_ptr) {
            break;
        }

        if (NULL != encInt_ptr) {
            /*
             * Check if it contains '/' for asymmetric payload type.
             * enum=encPT/decPT
             */
            enc_ptr = OSAL_strchr(s_ptr, '/');
            if (NULL != enc_ptr) {
                *enc_ptr = '\0';
                /* encoder payload type */
                *encInt_ptr = OSAL_atoi(s_ptr);
                /* decoder payload type */
                *decInt_ptr = OSAL_atoi(enc_ptr + 1);
            }
            else {
                /* It's symmetric, put same value for encode and decode */
                *encInt_ptr = *decInt_ptr = OSAL_atoi(s_ptr);
            }
        }
    }
}

static void _SAPP_decodeAudioParams(
    char       *desc_ptr,
    SAPP_Coder *coder_ptr,
    char       *delimiter_ptr)
{
    char   *s_ptr;
    int    *int_ptr;
    char   *m_ptr;
    int     checkYes = 0;
    int     checkModeSet;

    s_ptr = OSAL_strtok(desc_ptr, delimiter_ptr);
    while (NULL != s_ptr) {
        int_ptr = NULL;
        checkModeSet = 0;

        if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_annexb)) {
            int_ptr = &coder_ptr->props.g729.annexb;
            checkYes = 1;
        }
        else if (!OSAL_strncasecmp(s_ptr, _SAPP_neg_modeset,
                OSAL_strlen(_SAPP_neg_modeset))) {
            int_ptr = &coder_ptr->props.amr.modeSet;
            checkModeSet = 1;
        }
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_oa)) {
            /* Found "octet-align" */
            int_ptr = &coder_ptr->props.amr.octetAlign;
        }
        else {
            s_ptr = OSAL_strtok(NULL, delimiter_ptr);
            continue;
        }
        /*
         * We got token, let's get the value.
         */
        s_ptr = OSAL_strtok(NULL, delimiter_ptr);
        if (NULL == s_ptr) {
            break;
        }

        if (checkYes) {
            if (!OSAL_strcasecmp(s_ptr, "yes")) {
                *int_ptr = 1;
            }
            else {
                *int_ptr = 0;
            }
        }
        else {
            /* Must be AMR/AMR-WB parameters. */
            if (checkModeSet) {
                /* set bit rate of modeSet bitmask */
                *int_ptr = 0;
                m_ptr = s_ptr;
                while (m_ptr) {
                    *int_ptr |= (1 << OSAL_atoi(m_ptr));
                    m_ptr = OSAL_strscan(m_ptr, ",");
                    if (NULL != m_ptr) {
                        m_ptr++;
                    }
                }
            }
            else {
                *int_ptr = OSAL_atoi(s_ptr);
            }
        }
    }
}

/*
 * ======== _SAPP_decodeH264Params() ========
 *
 * This function will decode a given "desc_ptr"
 * and populates H264 related properties in SAPP_Coder.
 *
 * Typically IsiCoder description can contain fmtp and extmap related parameters
 * This method is designed to handle both.
 *
 * Returns:
 *   Nothing.
 */
static void _SAPP_decodeH264Params(
    char       *desc_ptr,
    SAPP_Coder *coder_ptr)
{
    char   *s_ptr;
    int    *int_ptr;
    char   *str_ptr;
    int     strLen;

    s_ptr = OSAL_strtok(desc_ptr, "=");

    while (NULL != s_ptr) {
        int_ptr = NULL;
        str_ptr = NULL;
        strLen = 0;

        if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_pmode)) {
            int_ptr = &coder_ptr->props.h264.pmode;
        }
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_plevel)) {
            str_ptr = coder_ptr->props.h264.plevel;
            strLen = sizeof(coder_ptr->props.h264.plevel);
        }
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_extmapid)) {
            int_ptr = &coder_ptr->props.h264.extmap.id;
        }
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_extmapuri)) {
            str_ptr = coder_ptr->props.h264.extmap.uri;
            strLen = sizeof(coder_ptr->props.h264.extmap.uri);
        }
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_maxbr) ||
                !OSAL_strcasecmp(s_ptr, _SAPP_neg_maxbr2)) {
            int_ptr = &coder_ptr->props.h264.maxBr;
        }
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_maxmbps) ||
            !OSAL_strcasecmp(s_ptr, _SAPP_neg_maxmbps2)) {
            int_ptr = &coder_ptr->props.h264.maxMbps;
        }
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_framesize)) {
            str_ptr = coder_ptr->props.h264.framesize;
            strLen = sizeof(coder_ptr->props.h264.framesize);
        }        
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_framerate)) {
            int_ptr = &coder_ptr->props.h264.framerate;
        }
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_sps) || 
            !OSAL_strcasecmp(s_ptr, _SAPP_neg_sps_long)) {
            str_ptr = coder_ptr->props.h264.sps;
            strLen = sizeof(coder_ptr->props.h264.sps);
        }

        /*
         * We got token, let's get the value.
         */
        s_ptr = OSAL_strtok(NULL, ";");
        if (NULL == s_ptr) {
            break;
        }

        if (NULL != int_ptr) {
            *int_ptr = OSAL_atoi(s_ptr);
        }
        else if (0 != strLen) {
            OSAL_snprintf(str_ptr, strLen, "%s", s_ptr);
        }
        s_ptr = OSAL_strtok(NULL, "= ");
    }
}

static void _SAPP_decodeH263Params(
    char       *desc_ptr,
    SAPP_Coder *coder_ptr,
    char       *delimter_ptr)
{
    char   *s_ptr;
    uint8  *uint8_ptr;
    int    *int_ptr;
    char   *str_ptr;
    char    buf[129];
    int     strLen;
    
    H263_Params *p_ptr = &coder_ptr->props.h263;

    /* Bug 6247, desc */
    OSAL_strncpy(buf, desc_ptr, 128);
    s_ptr = OSAL_strtok(buf, "=");

    while (NULL != s_ptr) {
        int_ptr = NULL;
        uint8_ptr = NULL;
        str_ptr = NULL;
        strLen = 0;
        
        if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_SQCIF)) {
            uint8_ptr = &p_ptr->SQCIF;
        }
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_QCIF)) {
            uint8_ptr = &p_ptr->QCIF;
        }
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_CIF)) {
            uint8_ptr = &p_ptr->CIF;
        }
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_CIF4)) {
            uint8_ptr = &p_ptr->CIF4;
        }
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_CIF16)) {
            uint8_ptr = &p_ptr->CIF16;
        }
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_framesize)) {
            str_ptr = p_ptr->framesize;
            strLen = sizeof(p_ptr->framesize);
        }
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_PAR)) {
             if (NULL == (s_ptr = OSAL_strtok(NULL, ":"))) break;
             p_ptr->PAR_ratio.left = (uint8)OSAL_atoi(s_ptr);
             if (NULL == (s_ptr = OSAL_strtok(NULL, delimter_ptr))) break;
             p_ptr->PAR_ratio.right = (uint8)OSAL_atoi(s_ptr);
             continue;
        }
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_CPCF)) { /* Load the 8 comma delimited values */
            if (NULL == (s_ptr = OSAL_strtok(NULL, ","))) break;
            p_ptr->CPCF.cd = (uint8)OSAL_atoi(s_ptr);
            if (NULL == (s_ptr = OSAL_strtok(NULL, ","))) break;
            p_ptr->CPCF.cf = OSAL_atoi(s_ptr);
            if (NULL == (s_ptr = OSAL_strtok(NULL, ","))) break;
            p_ptr->CPCF.SQCIFMPI = OSAL_atoi(s_ptr);
            if (NULL == (s_ptr = OSAL_strtok(NULL, ","))) break;
            p_ptr->CPCF.QCIFMPI = OSAL_atoi(s_ptr);
            if (NULL == (s_ptr = OSAL_strtok(NULL, ","))) break;
            p_ptr->CPCF.CIFMPI = OSAL_atoi(s_ptr);
            if (NULL == (s_ptr = OSAL_strtok(NULL, ","))) break;
            p_ptr->CPCF.CIF4MPI = OSAL_atoi(s_ptr);
            if (NULL == (s_ptr = OSAL_strtok(NULL, ","))) break;
            p_ptr->CPCF.CIF16MPI = OSAL_atoi(s_ptr);
            if (NULL == (s_ptr = OSAL_strtok(NULL, delimter_ptr))) break;
            p_ptr->CPCF.CUSTOMMPI = OSAL_atoi(s_ptr);
            continue;
        }
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_BPP)) {
            int_ptr = &p_ptr->BPP;
        }
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_HRD)) {
            uint8_ptr = &p_ptr->HRD;
        }
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_CUSTOM)) { /* Load the 3 comma delimited values */
            if (NULL == (s_ptr = OSAL_strtok(NULL, ","))) break;
            p_ptr->CUSTOM.Xmax = OSAL_atoi(s_ptr);
            if (NULL == (s_ptr = OSAL_strtok(NULL, ","))) break;
            p_ptr->CUSTOM.Ymax = OSAL_atoi(s_ptr);
            if (NULL == (s_ptr = OSAL_strtok(NULL, delimter_ptr))) break;
            p_ptr->CUSTOM.MPI = OSAL_atoi(s_ptr);
            continue;
        }
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_PROFILE)) {
            uint8_ptr = &p_ptr->PROFILE;
        }
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_LEVEL)) {
            uint8_ptr = &p_ptr->LEVEL;
        }
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_INTERLACE)) {
            uint8_ptr = &p_ptr->INTERLACE;
        }
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_F)) {
            uint8_ptr = &p_ptr->annex.F;
        }
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_I)) {
            uint8_ptr = &p_ptr->annex.I;
        }
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_J)) {
            uint8_ptr = &p_ptr->annex.J;
        }
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_T)) {
            uint8_ptr = &p_ptr->annex.T;
        }
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_K)) {
            uint8_ptr = &p_ptr->annex.K;
        }
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_N)) {
            uint8_ptr = &p_ptr->annex.K;
        }
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_P)) {
            uint8_ptr = &p_ptr->annex.K;
        }
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_maxbr) ||
                !OSAL_strcasecmp(s_ptr, _SAPP_neg_maxbr2)) {
            int_ptr = &coder_ptr->props.h263.maxBr;
        }
        else if (!OSAL_strcasecmp(s_ptr, _SAPP_neg_maxmbps) ||
                !OSAL_strcasecmp(s_ptr, _SAPP_neg_maxmbps2)) {
            int_ptr = &coder_ptr->props.h263.maxMbps;
        }
        
        /*
         * We got token, let's get the value.
         */
        s_ptr = OSAL_strtok(NULL, ";");
        if (NULL == s_ptr) {
            break;
        }
        if (NULL != int_ptr) {
            *int_ptr = OSAL_atoi(s_ptr);
        }
        else if (NULL != uint8_ptr) {
            *uint8_ptr = (uint8)OSAL_atoi(s_ptr);
        }
        else if (0 != strLen) {
            OSAL_snprintf(str_ptr, strLen, "%s", s_ptr);
        }
        s_ptr = OSAL_strtok(NULL, "= ");
    }
}

static vint _SAPP_encodeH263Param(
   char      **buffer_ptr,
   vint       *bufferLen_ptr,
   const char *paramName_ptr,
   vint        paramValue,
   char       *delimiter_ptr)
{
    int bytes;
    int len;
    char *str_ptr;
    if (0 != paramValue) {
        len = *bufferLen_ptr;
        str_ptr = *buffer_ptr;
        bytes = OSAL_snprintf(str_ptr, len, "%s=%d%s",
                paramName_ptr, paramValue, delimiter_ptr);
        if (bytes > len) return -1;
        *buffer_ptr = str_ptr + bytes;
        *bufferLen_ptr = len - bytes;
    }
    return 0;
}

static void _SAPP_encodeH263Params(
    SAPP_Coder  *c_ptr,
    char        *fmtp_ptr,
    vint         fmtpSize,
    char        *delimiter_ptr)
{
    char *str_ptr = fmtp_ptr;
    int   len = fmtpSize - 1;
    int   bytes;

    H263_Params *p_ptr = &c_ptr->props.h263;

    if (0 != _SAPP_encodeH263Param(&str_ptr, &len, _SAPP_neg_CIF, p_ptr->CIF, delimiter_ptr)) {
        return;
    }

    if (0 != _SAPP_encodeH263Param(&str_ptr, &len, _SAPP_neg_QCIF, p_ptr->QCIF, delimiter_ptr)) {
        return;
    }

    if (0 != _SAPP_encodeH263Param(&str_ptr, &len, _SAPP_neg_SQCIF, p_ptr->SQCIF, delimiter_ptr)) {
        return;
    }

    if (0 != _SAPP_encodeH263Param(&str_ptr, &len, _SAPP_neg_CIF4, p_ptr->CIF4, delimiter_ptr)) {
        return;
    }

    if (0 != _SAPP_encodeH263Param(&str_ptr, &len, _SAPP_neg_CIF16, p_ptr->CIF16, delimiter_ptr)) {
        return;
    }

    if (0 != p_ptr->PAR_ratio.left && 0 != p_ptr->PAR_ratio.right) {
        bytes = OSAL_snprintf(str_ptr, len, "%s=%d:%d%s", _SAPP_neg_PAR,
                p_ptr->PAR_ratio.left, p_ptr->PAR_ratio.right, delimiter_ptr);
        if (bytes > len) return;
        str_ptr += bytes;
        len -= bytes;
    }

    if (0 != p_ptr->CPCF.cd) { /* MAke sure you have room */
        bytes = OSAL_snprintf(str_ptr, len, "%s=%d,%d,%d,%d,%d,%d,%d,%d%s", _SAPP_neg_CPCF,
                p_ptr->CPCF.cd, p_ptr->CPCF.cf, p_ptr->CPCF.SQCIFMPI,
                p_ptr->CPCF.QCIFMPI , p_ptr->CPCF.CIFMPI , p_ptr->CPCF.CIF4MPI,
                p_ptr->CPCF.CIF16MPI, p_ptr->CPCF.CUSTOMMPI, delimiter_ptr);
        if (bytes > len) return;
        str_ptr += bytes;
        len -= bytes;
    }

    if (0 != _SAPP_encodeH263Param(&str_ptr, &len, _SAPP_neg_BPP, p_ptr->BPP, delimiter_ptr)) {
        return;
    }

    if (0 != _SAPP_encodeH263Param(&str_ptr, &len, _SAPP_neg_HRD, p_ptr->HRD, delimiter_ptr)) {
        return;
    }

    if (0 != p_ptr->CUSTOM.Xmax) {
        bytes = OSAL_snprintf(str_ptr, len, "%s=%d,%d,%d%s", _SAPP_neg_CUSTOM,
                p_ptr->CUSTOM.Xmax, p_ptr->CUSTOM.Ymax, p_ptr->CUSTOM.MPI, delimiter_ptr);
        if (bytes > len) return;
        str_ptr += bytes;
        len -= bytes;
    }
    if (0 != _SAPP_encodeH263Param(&str_ptr, &len, _SAPP_neg_PROFILE, p_ptr->PROFILE, delimiter_ptr)) {
        return;
    }
    if (0 != _SAPP_encodeH263Param(&str_ptr, &len, _SAPP_neg_LEVEL, p_ptr->LEVEL, delimiter_ptr)) {
        return;
    }

    if (0 != _SAPP_encodeH263Param(&str_ptr, &len, _SAPP_neg_INTERLACE, p_ptr->INTERLACE, delimiter_ptr)) {
        return;
    }

    if (0 != _SAPP_encodeH263Param(&str_ptr, &len, _SAPP_neg_F, p_ptr->annex.F, delimiter_ptr)) {
        return;
    }

    if (0 != _SAPP_encodeH263Param(&str_ptr, &len, _SAPP_neg_I, p_ptr->annex.I, delimiter_ptr)) {
        return;
    }

    if (0 != _SAPP_encodeH263Param(&str_ptr, &len, _SAPP_neg_J, p_ptr->annex.J, delimiter_ptr)) {
        return;
    }

    if (0 != _SAPP_encodeH263Param(&str_ptr, &len, _SAPP_neg_T, p_ptr->annex.T, delimiter_ptr)) {
        return;
    }

    if (0 != _SAPP_encodeH263Param(&str_ptr, &len, _SAPP_neg_K, p_ptr->annex.K, delimiter_ptr)) {
        return;
    }

    if (0 != _SAPP_encodeH263Param(&str_ptr, &len, _SAPP_neg_N, p_ptr->annex.K, delimiter_ptr)) {
        return;
    }

    if (0 != _SAPP_encodeH263Param(&str_ptr, &len, _SAPP_neg_P, p_ptr->annex.K, delimiter_ptr)) {
        return;
    }
    
    if (0 != _SAPP_encodeH263Param(&str_ptr, &len, _SAPP_neg_maxbr2, p_ptr->maxBr, delimiter_ptr)) {
        return;
    }

    if (0 != _SAPP_encodeH263Param(&str_ptr, &len, _SAPP_neg_maxmbps2, p_ptr->maxMbps, delimiter_ptr)) {
        return;
    }

    if (str_ptr != fmtp_ptr) {
        /*
         * Then something wass written, let's get rid of the last delimiter.
         */
        str_ptr -= OSAL_strlen(delimiter_ptr);

    }
    *str_ptr = 0;
    return;
}

/*
 * ======== _SAPP_encodeH264Fmtp() ========
 *
 * This function will encode a fmtp string for H264 using
 * H264 related properties in SAPP_Coder
 *
 * Note: Even though props.h264 contains extmap related parameters
 * This function ignores them as they are not part of fmtp.
 *
 * Returns:
 *   Nothing.
 */
static void _SAPP_encodeH264Fmtp(
    SAPP_Coder  *c_ptr,
    char        *fmtp_ptr,
    vint         fmtpSize,
    char        *delimiter_ptr)
{
    int bytes;
    char *str_ptr = fmtp_ptr;
    int i = 0;

    /* Check if plevel has something */
    if (0 != c_ptr->props.h264.plevel[0]) {
        /* Then we have a plevel to write */
        bytes = OSAL_snprintf(str_ptr, fmtpSize, "%s=%s%s",
                _SAPP_neg_plevel, c_ptr->props.h264.plevel, delimiter_ptr);
        str_ptr += bytes;
        fmtpSize -= bytes;
    }
    if (0 <= c_ptr->props.h264.pmode) {
        bytes = OSAL_snprintf(str_ptr, fmtpSize, "%s=%d%s",
                _SAPP_neg_pmode, c_ptr->props.h264.pmode, delimiter_ptr);
        str_ptr += bytes;
        fmtpSize -= bytes;
    }
    if (0 != c_ptr->props.h264.maxBr) {
        bytes = OSAL_snprintf(str_ptr, fmtpSize, "%s=%d%s",
                _SAPP_neg_maxbr, c_ptr->props.h264.maxBr, delimiter_ptr);
        str_ptr += bytes;
        fmtpSize -= bytes;
    }
    if (0 != c_ptr->props.h264.maxMbps) {
        bytes = OSAL_snprintf(str_ptr, fmtpSize, "%s=%d%s",
                _SAPP_neg_maxmbps, c_ptr->props.h264.maxMbps, delimiter_ptr);
        str_ptr += bytes;
        fmtpSize -= bytes;
    }
    if (0 != c_ptr->props.h264.sps[0]) {
        bytes = OSAL_snprintf(str_ptr, fmtpSize, "%s=%s%s",
                _SAPP_neg_sps_long, c_ptr->props.h264.sps, delimiter_ptr);
        str_ptr += bytes;
        fmtpSize -= bytes;
    }

    if (str_ptr != fmtp_ptr) {
        /*
         * Then something wass written, let's get rid of the last delimiter.
         */
        for (i = 0; str_ptr[i] != *delimiter_ptr; i--){        
        }
    }
    str_ptr[i] = '\0';
    return;
}

/*
 * ======== _SAPP_encodeModeSet() ========
 *
 * Private function encode modeset from bitmask to fmtp string.
 *
 * Return:
 *   SAPP_OK: Encode mode set done.
 *   SAPP_ERR: Ecnode mode set failed.
 */
static vint _SAPP_encodeModeSet(
    vint  modeSetBitMask,
    char *s_ptr,
    vint  sLen)
{
    vint first;
    vint len;
    vint index;

    if ((NULL == s_ptr) || (0 >= sLen)) {
        return (SAPP_ERR);
    }

    /* Empty the string if no modeset is configured. */
    if (0 == modeSetBitMask) {
        s_ptr[0] = 0;
        return (SAPP_OK);
    }

    first = 1;
    len = 0;

    len += OSAL_snprintf(s_ptr, sLen, "%s=", _SAPP_neg_modeset);
    for (index = 0; index <= _SAPP_MAX_MODE_SET; index++) {
        if (0x1 & (modeSetBitMask >> index)) {
            if (!first) {
                len += OSAL_snprintf(s_ptr + len, sLen - len, ",%d", index);
            }
            else {
                len += OSAL_snprintf(s_ptr + len, sLen - len, "%d", index);
            }
            first = 0;
        }
    }
    
    return (SAPP_OK);
}

static vint _SAPP_encodeCoderSapp2SipHelper(
    SAPP_Coder        *c_ptr,
    tMediaCoder       *t_ptr,
    tNetworkAddrType   addressType)
{
    vint   clockRate;
    char  *cName_ptr;
    int    cNameSz;
    uint32 bandwidth;
    char   modesetStr[_SAPP_MODE_SET_STRING_SIZE];
    char  *fmtp_ptr = t_ptr->fmtp;
    vint   fmtpSize = MAX_SESSION_MEDIA_LARGE_STR;
    vint   index;

    clockRate = 0;
    bandwidth = 0;
    cName_ptr = c_ptr->szCoderName;
    cNameSz = sizeof(c_ptr->szCoderName);

    if (!OSAL_strncasecmp(_SAPP_neg_g729, cName_ptr, cNameSz)) {
        if (c_ptr->props.g729.annexb) {
            OSAL_snprintf(fmtp_ptr, fmtpSize, "%s=%s", _SAPP_neg_annexb, "yes");
        }
        clockRate = 8000;
        /* Set the bandwidth to 24kbps for b=AS */
        bandwidth = 24;
    }
    else if (!OSAL_strncasecmp(_SAPP_neg_ilbc, cName_ptr, cNameSz)) {
        if (c_ptr->rate == 20) {
            /*
             * Check for 20 ms packet times for iLBC, if it is then set a
             * param to reflect that. Anything else defaults to 30ms.
             */

            OSAL_snprintf(fmtp_ptr, fmtpSize, "%s=%s", _SAPP_neg_mode, "20");
        }
        else {
            OSAL_snprintf(fmtp_ptr, fmtpSize, "%s=%s", _SAPP_neg_mode, "30");
        }
        clockRate = 8000;
    }
    else if (!OSAL_strncasecmp(_SAPP_neg_tel_evt, cName_ptr, cNameSz)) {
        OSAL_snprintf(fmtp_ptr, fmtpSize, "%s", "0-15");
        clockRate = 8000;
    }
    else if (!OSAL_strncasecmp(_SAPP_neg_tel_evt_16k, cName_ptr, cNameSz)) {
        OSAL_strcpy(cName_ptr, _SAPP_neg_tel_evt);
        OSAL_snprintf(fmtp_ptr, fmtpSize, "%s", "0-15");
        clockRate = 16000;
    }
    else if (!OSAL_strncasecmp(_SAPP_neg_pcmu, cName_ptr, cNameSz)) {
        clockRate = 8000;
        /* Set the bandwidth to 80kbps for b=AS */
        bandwidth = 80;
    }
    else if (!OSAL_strncasecmp(_SAPP_neg_pcma, cName_ptr, cNameSz)) {
        clockRate = 8000;
        /* Set the bandwidth to 80kbps for b=AS */
        bandwidth = 80;
    }
    else if (!OSAL_strncasecmp(_SAPP_neg_cn, cName_ptr, cNameSz)) {
        clockRate = 8000;
    }
    else if (!OSAL_strncasecmp(_SAPP_neg_g726_32, cName_ptr, cNameSz)) {
        clockRate = 8000;
    }
    else if (!OSAL_strncasecmp(_SAPP_neg_g722, cName_ptr, cNameSz)) {
        clockRate = 8000;
    }
    else if (!OSAL_strncasecmp(_SAPP_neg_silk_24k, cName_ptr, cNameSz)) {
        OSAL_snprintf(cName_ptr, ISI_CODER_STRING_SZ, "%s",  "SILK");
        clockRate = 24000;
    }
    else if (!OSAL_strncasecmp(_SAPP_neg_silk_16k, cName_ptr, cNameSz)) {
        OSAL_snprintf(cName_ptr, ISI_CODER_STRING_SZ, "%s",  "SILK");
        clockRate = 16000;
    }
    else if (!OSAL_strncasecmp(_SAPP_neg_silk_8k, cName_ptr, cNameSz)) {
        OSAL_snprintf(cName_ptr, ISI_CODER_STRING_SZ, "%s",  "SILK");
        clockRate = 8000;
    }
    else if (!OSAL_strncasecmp(_SAPP_neg_h264, cName_ptr, cNameSz)) {
        /*
         * ZK: For ethereal, that uses H264 for decoding name rather than h264.
         */
        *cName_ptr = 'H';
        clockRate = 90000;
        _SAPP_encodeH264Fmtp(c_ptr, fmtp_ptr, fmtpSize, "; ");

        /* Check if we have some extmap parameters. */
        if (0 != c_ptr->props.h264.extmap.id) {
            t_ptr->extmap.id = c_ptr->props.h264.extmap.id;
        }
        if (0 != c_ptr->props.h264.extmap.uri[0]) {
            OSAL_snprintf(t_ptr->extmap.uri, MAX_SDP_ATTR_STR_LEN, "%s",  c_ptr->props.h264.extmap.uri);
        }
        if (0 != c_ptr->props.h264.framesize[0]) {
            char* end_ptr = NULL;
            t_ptr->width = OSAL_strtoul(c_ptr->props.h264.framesize, &end_ptr, 10);
            if (end_ptr != NULL)
                t_ptr->height = OSAL_strtoul(end_ptr+1, NULL, 10);
        }
        if (0 != c_ptr->props.h264.framerate) {
            t_ptr->framerate = c_ptr->props.h264.framerate;
        }
        /* Set the bandwidth to 1024kbps for b=AS */
        bandwidth = 1024;
    }
    else if (SAPP_OK == _SAPP_isH263(cName_ptr, cNameSz)) {
        *cName_ptr = 'H';
        clockRate = 90000;
        _SAPP_encodeH263Params(c_ptr, fmtp_ptr, fmtpSize, ";");
        /* Set the bandwidth to 1024kbps for b=AS */
        bandwidth = 1024;
        if (0 != c_ptr->props.h263.framesize[0]) {
            char* end_ptr = NULL;
            t_ptr->width = OSAL_strtoul(c_ptr->props.h263.framesize, &end_ptr, 10);
            if (end_ptr != NULL)
                t_ptr->height = OSAL_strtoul(end_ptr+1, NULL, 10);
        }
    }
    else if (!OSAL_strncasecmp(_SAPP_neg_amrnb, cName_ptr, cNameSz)) {
        OSAL_snprintf(cName_ptr, ISI_CODER_STRING_SZ, "%s",  "AMR");
        /* Set the bandwidth to 39kbps for b=AS */
        bandwidth = 39;
        if (0 < c_ptr->props.amr.modeSet) {
            _SAPP_encodeModeSet(c_ptr->props.amr.modeSet, modesetStr,
                    sizeof(modesetStr));
            OSAL_snprintf(fmtp_ptr, fmtpSize, "%s=%d;%s;%s",
                    _SAPP_neg_oa, c_ptr->props.amr.octetAlign,
                    _SAPP_neg_mcc, modesetStr);
            for (index = GAMRNB_MAX_MODE_SET; index >= 0; index--) {
                if (c_ptr->props.amr.modeSet & (0x1 << index)) {
                    if (addressType == eNwAddrIPv6) {
                        bandwidth = _gamrnb_bandwidthTableIpv6
                                [c_ptr->props.amr.octetAlign][index];
                    }
                    else {
                        bandwidth = _gamrnb_bandwidthTableIpv4
                                [c_ptr->props.amr.octetAlign][index];
                    }
                    break;
                }
            }
        }
        else {
            OSAL_snprintf(fmtp_ptr, fmtpSize, "%s=%d;%s",
                    _SAPP_neg_oa, c_ptr->props.amr.octetAlign, _SAPP_neg_mcc);
        }
        clockRate = 8000;
    }
    else if (!OSAL_strncasecmp(_SAPP_neg_amrwb, cName_ptr, cNameSz)) {
        OSAL_snprintf(cName_ptr, ISI_CODER_STRING_SZ, "%s",  "AMR-WB");
        /* Set the bandwidth to 41kbps for b=AS */
        bandwidth = 41;
        if (0 < c_ptr->props.amr.modeSet) {
            _SAPP_encodeModeSet(c_ptr->props.amr.modeSet, modesetStr,
                    sizeof(modesetStr));
            OSAL_snprintf(fmtp_ptr, fmtpSize, "%s=%d;%s;%s",
                    _SAPP_neg_oa, c_ptr->props.amr.octetAlign,
                    _SAPP_neg_mcc, modesetStr);
            for (index = GAMRWB_MAX_MODE_SET; index >= 0; index--) {
                if (c_ptr->props.amr.modeSet & (0x1 << index)) {
                    if (addressType == eNwAddrIPv6) {
                        bandwidth = _gamrwb_bandwidthTableIpv6
                                [c_ptr->props.amr.octetAlign][index];
                    }
                    else {
                        bandwidth = _gamrwb_bandwidthTableIpv4
                                [c_ptr->props.amr.octetAlign][index];
                    }
                    break;
                }
            }
        }
        else {
            OSAL_snprintf(fmtp_ptr, fmtpSize, "%s=%d;%s",
                    _SAPP_neg_oa, c_ptr->props.amr.octetAlign, _SAPP_neg_mcc);
        }
        clockRate = 16000;
    }
    else if (!OSAL_strncasecmp(_SAPP_neg_g7221, cName_ptr, cNameSz)) {
        OSAL_snprintf(cName_ptr, ISI_CODER_STRING_SZ, "%s",  "G7221");
        clockRate = 16000;
        /* Set the bandwidth to 32kbps for b=AS */
        bandwidth = 32;
    }

    else {
        return(SAPP_ERR);
    }

    /* The RTP coder number */
    t_ptr->payloadType = c_ptr->coderNum;
    t_ptr->decodePayloadType = c_ptr->decoderNum;

    /* Set the clock rate */
    t_ptr->clockRate = clockRate;

    /* Set the bandwidth */
    t_ptr->bandwidth = bandwidth;

    /*
     * Copy coder name.
     */
    OSAL_snprintf(t_ptr->encodingName, MAX_SESSION_MEDIA_STR, "%s", cName_ptr);
    return (SAPP_OK);
}

/*
 * ======== _SAPP_mapDirSip2Isi() ========
 *
 * This function is used to map SIP/SDP defined enumerated values that
 * represent a call's direction to an enumerated value that ISI understands.
 *
 * Returns:
 *   An Enumerated value representing a direction as defined in ISI.
 */
ISI_SessionDirection _SAPP_mapDirSip2Isi(
    tSdpAttrType direction)
{
    switch (direction) {
    case eSdpAttrSendOnly:
        return (ISI_SESSION_DIR_SEND_ONLY);
    case eSdpAttrRecvOnly:
        return (ISI_SESSION_DIR_RECV_ONLY);
    case eSdpAttrSendRecv:
        return (ISI_SESSION_DIR_SEND_RECV);
    case eSdpAttrInactive:
    default:
        return (ISI_SESSION_DIR_INACTIVE);
    }
}

/*
 * ======== _SAPP_mapDirIsi2Sip() ========
 *
 * This function is used to map ISI defined enumerated values that represent
 * a call's direction to an enumerated value that SIP/SDP understands.
 *
 * Returns:
 *   An Enumerated value representing a direction as defined in SIP/SDP.
 */
tSdpAttrType _SAPP_mapDirIsi2Sip(
    ISI_SessionDirection direction)
{
    switch (direction) {
    case ISI_SESSION_DIR_SEND_ONLY:
        return (eSdpAttrSendOnly);
    case ISI_SESSION_DIR_RECV_ONLY:
        return (eSdpAttrRecvOnly);
    case ISI_SESSION_DIR_SEND_RECV:
        return (eSdpAttrSendRecv);
    case ISI_SESSION_DIR_INVALID:
    case ISI_SESSION_DIR_INACTIVE:
    default:
        return (eSdpAttrInactive);
    }
}


/*
 * ======== _SAPP_encodeCoder() ========
 * This function will translate a SIP/SDP coder to an ISI coder description
 *
 * Return Values:
 *  SAPP_OK :  Coder was successfully translated.
 *  SAPP_ERR : Coder is unknown.
 *
 */
vint _SAPP_encodeCoder(
    ISIP_Coder  *c_ptr,
    tMediaCoder *t_ptr,
    uint8        packetRate)
{
    char        *coderName_ptr = NULL;
    int         len, bytes;
    SAPP_Coder  coder;
    char       *str_ptr;
    char        coderNumStr[16];
    uint8       coderNum = t_ptr->payloadType;
    uint8       decoderNum = t_ptr->decodePayloadType;
    char       *name_ptr = t_ptr->encodingName;
    char       *fmtp_ptr = t_ptr->fmtp;
    char       *extmapUri_ptr = t_ptr->extmap.uri;
    vint        clockRate = t_ptr->clockRate;
    char        modesetStr[_SAPP_MODE_SET_STRING_SIZE];
    char        fmtpTmp[MAX_SESSION_MEDIA_LARGE_STR + 1];

    /*
     * For asymmetric payload type.
     * If encode and decode payload typle is different, set enum=encPT/decPT
     */
    if (coderNum != decoderNum) {
        OSAL_snprintf(coderNumStr, sizeof(coderNumStr),
                "%d/%d", coderNum, decoderNum);
    }
    else {
        OSAL_snprintf(coderNumStr, sizeof(coderNumStr),
                "%d", coderNum);
    }

    if (OSAL_strncasecmp(name_ptr, _SAPP_neg_g729, 
            sizeof(_SAPP_neg_g729)) == 0) {
        coderName_ptr = name_ptr;
        /* We must determine what kind of G729 */
        if (OSAL_strncasecmp(fmtp_ptr, "annexb=yes", 10) == 0) {
            OSAL_snprintf(c_ptr->description, sizeof(c_ptr->description),
                    "enum=%s;rate=%d;annexb=yes",
                    coderNumStr, packetRate);
        }
        else {
            /* If we are here then it's not 'AB'
             */
            OSAL_snprintf(c_ptr->description, sizeof(c_ptr->description),
                    "enum=%s;rate=%d",
                    coderNumStr, packetRate);
        }
        c_ptr->relates = ISI_SESSION_TYPE_AUDIO;
    }
    else if (OSAL_strncasecmp(name_ptr, _SAPP_neg_ilbc, 
            sizeof(_SAPP_neg_ilbc)) == 0) {
        coderName_ptr = name_ptr;
        if (OSAL_strncasecmp(fmtp_ptr, "mode=20", 7) == 0) {
            packetRate = 20;
        }
        else {
            /*
             * If we are here then we didn't find any parameter
             * setting so use default of 30ms
             */
            packetRate = 30;
        }
        OSAL_snprintf(c_ptr->description, sizeof(c_ptr->description),
                "enum=%s;rate=%d",
                coderNumStr, packetRate);
        c_ptr->relates = ISI_SESSION_TYPE_AUDIO;
    }
    else if (OSAL_strncasecmp(name_ptr, _SAPP_neg_pcmu, 
            sizeof(_SAPP_neg_pcmu)) == 0) {
        coderName_ptr = name_ptr;
        OSAL_snprintf(c_ptr->description, sizeof(c_ptr->description),
                "enum=%s;rate=%d",
                coderNumStr, packetRate);
        c_ptr->relates = ISI_SESSION_TYPE_AUDIO;
    }
    else if (OSAL_strncasecmp(name_ptr, _SAPP_neg_pcma, 
            sizeof(_SAPP_neg_pcma)) == 0) {
        coderName_ptr = name_ptr;
        OSAL_snprintf(c_ptr->description, sizeof(c_ptr->description),
                "enum=%s;rate=%d",
                coderNumStr, packetRate);
        c_ptr->relates = ISI_SESSION_TYPE_AUDIO;
    }
    else if (OSAL_strncasecmp(name_ptr, _SAPP_neg_cn, 
            sizeof(_SAPP_neg_cn)) == 0) {
        coderName_ptr = name_ptr;
        OSAL_snprintf(c_ptr->description, sizeof(c_ptr->description),
                "enum=%s;rate=%d",
                coderNumStr, packetRate);
        c_ptr->relates = ISI_SESSION_TYPE_AUDIO;
    }
    else if (OSAL_strncasecmp(name_ptr, _SAPP_neg_g726_32, 
            sizeof(_SAPP_neg_g726_32)) == 0) {
        coderName_ptr = name_ptr;
        OSAL_snprintf(c_ptr->description, sizeof(c_ptr->description),
                "enum=%s;rate=%d",
                coderNumStr, packetRate);
        c_ptr->relates = ISI_SESSION_TYPE_AUDIO;
    }
    else if (OSAL_strncasecmp(name_ptr, _SAPP_neg_g722,
            sizeof(_SAPP_neg_g722)) == 0) {
        coderName_ptr = name_ptr;
        OSAL_snprintf(c_ptr->description, sizeof(c_ptr->description),
                "enum=%s;rate=%d", coderNumStr, packetRate);
        c_ptr->relates = ISI_SESSION_TYPE_AUDIO;
    }
    else if ((OSAL_strncasecmp(name_ptr, _SAPP_neg_amrnb, 
            sizeof(_SAPP_neg_amrnb)) == 0) ||
            (OSAL_strncasecmp(name_ptr, _SAPP_neg_amrwb,
            sizeof(_SAPP_neg_amrwb)) == 0)) {
        coderName_ptr = name_ptr;
        OSAL_strcpy(fmtpTmp, fmtp_ptr);
        /* set default value. */
        coder.props.amr.modeSet    = 0;
        coder.props.amr.octetAlign = 0;
        _SAPP_decodeAudioParams(fmtpTmp, &coder, ";= ");
        _SAPP_encodeModeSet(coder.props.amr.modeSet, modesetStr,
                sizeof(modesetStr));
        OSAL_snprintf(c_ptr->description, sizeof(c_ptr->description),
                "enum=%s;rate=%d;%s=%d;%s", coderNumStr, packetRate,
                _SAPP_neg_oa, coder.props.amr.octetAlign, modesetStr);
        c_ptr->relates = ISI_SESSION_TYPE_AUDIO;
    }
    else if (OSAL_strncasecmp(name_ptr, _SAPP_neg_g7221, sizeof(_SAPP_neg_g7221)) == 0) {
        coderName_ptr = name_ptr;
        OSAL_snprintf(c_ptr->description, sizeof(c_ptr->description),
                "enum=%s;rate=%d",
                coderNumStr, packetRate);
        c_ptr->relates = ISI_SESSION_TYPE_AUDIO;
    }
    else if (OSAL_strncasecmp(name_ptr, _SAPP_neg_silk,
            sizeof(_SAPP_neg_silk)) == 0) {
        /*
         * Check the sample rate for the specific silk coder ISI/vTSP type
         */
        if (24000 == clockRate) {
            OSAL_snprintf(name_ptr, ISI_CODER_STRING_SZ, "%s",
                    _SAPP_neg_silk_24k);
        }
        else if (16000 == clockRate) {
            OSAL_snprintf(name_ptr, ISI_CODER_STRING_SZ, "%s",
                    _SAPP_neg_silk_16k);
        }
        else /*(8000== clockRate) */ {
            OSAL_snprintf(name_ptr, ISI_CODER_STRING_SZ, "%s",
                    _SAPP_neg_silk_8k);
        }
        coderName_ptr = name_ptr;
        OSAL_snprintf(c_ptr->description, sizeof(c_ptr->description),
                "enum=%s;rate=%d", coderNumStr, packetRate);
        c_ptr->relates = ISI_SESSION_TYPE_AUDIO;
        OSAL_logMsg("clockRate=%d, coderName=%s\n", clockRate, coderName_ptr);
    }
    else if (OSAL_strncasecmp(name_ptr, _SAPP_neg_tel_evt, 
            sizeof(_SAPP_neg_tel_evt)) == 0) {
        if (16000 == clockRate) {
            OSAL_snprintf(name_ptr, ISI_CODER_STRING_SZ, "%s",
                    _SAPP_neg_tel_evt_16k);
        }
        else /*(8000== clockRate) */ {
            OSAL_snprintf(name_ptr, ISI_CODER_STRING_SZ, "%s",
                    _SAPP_neg_tel_evt);
        }
        coderName_ptr = name_ptr;
        OSAL_snprintf(c_ptr->description, sizeof(c_ptr->description),
                "enum=%s;rate=%d",
                coderNumStr, packetRate);
        c_ptr->relates = ISI_SESSION_TYPE_AUDIO;
    }
    else if (OSAL_strncasecmp(name_ptr, _SAPP_neg_h264,
            sizeof(_SAPP_neg_h264)) == 0) {
        coderName_ptr = name_ptr;
        len = sizeof(c_ptr->description) - 1;
        str_ptr = c_ptr->description;
        str_ptr[len] = '\0';
        bytes = OSAL_snprintf(str_ptr, len, "enum=%s;rate=%d;", coderNumStr, 
                packetRate);
        str_ptr += bytes;
        len -= bytes;

        /* Add extmap related params to description if present. */
        if (0 != *extmapUri_ptr) {
            bytes = OSAL_snprintf(str_ptr, len, "%s=%d;%s=%s;",
                    _SAPP_neg_extmapid, t_ptr->extmap.id, _SAPP_neg_extmapuri, extmapUri_ptr);
            str_ptr += bytes;
            len -= bytes;
        }

        /* Add framerate and framesize parameter to description if present. */
        if (0 != t_ptr->width) {
            bytes = OSAL_snprintf(str_ptr, len, "%s=%d-%d;",
                    _SAPP_neg_framesize, t_ptr->width, t_ptr->height);
            str_ptr += bytes;
            len -= bytes;
        }

        if (0 != t_ptr->framerate) {
            bytes = OSAL_snprintf(str_ptr, len, "%s=%d;",
                    _SAPP_neg_framerate, t_ptr->framerate);
            str_ptr += bytes;
            len -= bytes;            
        }

        if (0 != *fmtp_ptr) {            
            /*
             * Some code reuse here, Let's decode the fmtp and then
             * re-encode it for ISI
             */
            OSAL_memSet(&coder, 0, sizeof(SAPP_Coder));
            _SAPP_decodeH264Params(fmtp_ptr, &coder);
            _SAPP_encodeH264Fmtp(&coder, str_ptr, len, ";");
        }
        c_ptr->relates = ISI_SESSION_TYPE_VIDEO;
    }
    else if (SAPP_OK == _SAPP_isH263(name_ptr, OSAL_strlen(name_ptr))) {
        coderName_ptr = name_ptr;
        len = sizeof(c_ptr->description) - 1;
        str_ptr = c_ptr->description;
        bytes = OSAL_snprintf(str_ptr, len, "enum=%s;rate=%d;", coderNumStr, 
                packetRate);
        str_ptr += bytes;
        len -= bytes;

        /* Add framerate and framesize parameter to description if present. */
        if (0 != t_ptr->width) {
            bytes = OSAL_snprintf(str_ptr, len, "%s=%d-%d;",
                    _SAPP_neg_framesize, t_ptr->width, t_ptr->height);
            str_ptr += bytes;
            len -= bytes;
        }

        if (0 != t_ptr->framerate) {
            bytes = OSAL_snprintf(str_ptr, len, "%s=%d;",
                    _SAPP_neg_framerate, t_ptr->framerate);
            str_ptr += bytes;
            len -= bytes;            
        }

        if (0 != *fmtp_ptr) {
            /*
             * Some code reuse here, Let's decode the fmtp and then
             * re-encode it for ISI
             */
            OSAL_memSet(&coder, 0, sizeof(SAPP_Coder));
            _SAPP_decodeH263Params(fmtp_ptr, &coder, ";=");
            _SAPP_encodeH263Params(&coder, str_ptr, len, ";");
        }
        c_ptr->relates = ISI_SESSION_TYPE_VIDEO;
    }
    else {
        return (SAPP_ERR);
    }

    OSAL_snprintf(c_ptr->szCoderName, ISI_CODER_STRING_SZ, "%s",
            coderName_ptr);
    return (SAPP_OK);
}

/*
 * ======== _SAPP_generateSrtpKeys() ========
 * This function will create 2 keys (random strings) for RTP Security (SRTP)
 * and then populate a SIP/SDP object with the base64 encoded representation
 * of those key values.
 *
 * Return Values:
 *  Nothing
 */
void _SAPP_generateSrtpKeys(
    char              *aes80_ptr,
    char              *aes32_ptr,
    tMedia            *media_ptr)
{
    int x;
    /* Generate random keys */
    OSAL_randomGetOctets(aes80_ptr, SRTP_SECURITY_KEY_SZ);
    OSAL_randomGetOctets(aes32_ptr, SRTP_SECURITY_KEY_SZ);

    /* Make sure no value is over 7F */
    for (x = 0 ; x < SRTP_SECURITY_KEY_SZ ; x++) {
        aes80_ptr[x] &= 0x7f;
        aes32_ptr[x] &= 0x7f;
    }

    /* Base64 encode keys */
    OSAL_memSet(media_ptr->srtpKeyParamsAes80, 0,
            sizeof(media_ptr->srtpKeyParamsAes80));
    b64encode(aes80_ptr, media_ptr->srtpKeyParamsAes80,
            SRTP_SECURITY_KEY_SZ);

    OSAL_memSet(media_ptr->srtpKeyParamsAes32, 0,
            sizeof(media_ptr->srtpKeyParamsAes32));
    b64encode(aes32_ptr, media_ptr->srtpKeyParamsAes32,
            SRTP_SECURITY_KEY_SZ);
}

/*
 * ======== _SAPP_negEncodeCoderSapp2Sip() ========
 *
 * This function will populate a SIP/SDP media object with coder details
 * specified by an the array of SAPP coders ("coders[]").
 * This function will basically translate coders found in SAPP "coders" to
 * the SIP/SDP format defined in the tMedia object.
 *
 * Returns:
 *   Nothing.
 */
void _SAPP_updateMediaSapp2Sip(
    SAPP_CallObj    *call_ptr,
    ISIP_Call       *c_ptr)
{
    tSession      *sess_ptr;
    tMedia        *media_ptr;
    vint           x;

    sess_ptr = &call_ptr->mnsSession.session;

    /* Always set session-level direction to sendrecv */
    sess_ptr->lclDirection = eSdpAttrSendRecv;

    /* Set resource status */
    call_ptr->mnsSession.rsrcStatus = c_ptr->rsrcStatus;

    for (x = 0; x < sess_ptr->numMedia; ++x) {
        media_ptr = &sess_ptr->media[x];
        _SAPP_encodeCoderSapp2Sip(call_ptr->coders, media_ptr, 
                sess_ptr->lclAddr.addressType);
        if (eSdpMediaAudio == media_ptr->mediaType) {
            /* Set direction from ISI cmd */
            media_ptr->lclOriDir = media_ptr->lclDirection;
            media_ptr->lclDirection = _SAPP_mapDirIsi2Sip(c_ptr->audioDirection);
            /* RS and RR */
            media_ptr->bwRs = 0;
            media_ptr->bwRr = 1600; /* XXX To be updated. */
            /* Set rtp port to zero if the media is diabled */
            if (!(c_ptr->type & ISI_SESSION_TYPE_AUDIO)) {
                media_ptr->lclRtpPort = 0;
            }
        }
        else if (eSdpMediaVideo == media_ptr->mediaType) {
            /*
             * Local Video AS bandwidth is specified at media level (not at the coder level).
             * Copy the video bandwidth from ISIP_Call to tMedia.
             */
            media_ptr->lclAsBwKbps = c_ptr->lclVideoAsBwKbps;
            /* Set direction from ISI cmd */
            media_ptr->lclOriDir = media_ptr->lclDirection;
            media_ptr->lclDirection = _SAPP_mapDirIsi2Sip(c_ptr->videoDirection);
            /* RS and RR */
            media_ptr->bwRs = 0;
            media_ptr->bwRr = 19200; /* XXX To be updated. */
            /* Set rtp port to zero if the media is diabled */
            if (!(c_ptr->type & ISI_SESSION_TYPE_VIDEO)) {
                media_ptr->lclRtpPort = 0;
            }
        }
    }
}

void _SAPP_encodeCoderSapp2Sip(
    SAPP_Coder         coders[],
    tMedia            *targetMedia_ptr,
    tNetworkAddrType   addressType)
{
    vint x;
    vint y;
    /* Load the coders.  Translate from ISI format to SIP format */
    y = 0;
    for (x = 0 ; x < ISI_CODER_NUM && y < SYSDB_MAX_NUM_CODERS; x++) {
        /*
         * Only fill audio coders for audio media and video coders for video
         * media.
         */
        if (eSdpMediaAudio == targetMedia_ptr->mediaType) {
            if (eSdpMediaAudio != coders[x].relates) {
                continue;
            }
        }
        else if (eSdpMediaVideo == targetMedia_ptr->mediaType) {
            if (eSdpMediaVideo != coders[x].relates) {
                continue;
            }
        }
        if (coders[x].szCoderName[0] != 0) {
            if (targetMedia_ptr->packetRate == 0) {
                /*
                 * Let's load the packet rates to some default value
                 * Just make it the first valid coder.
                 */
                targetMedia_ptr->packetRate = coders[x].rate;

            }
            if (coders[x].maxPacketRate != 0) {
                targetMedia_ptr->maxPacketRate = coders[x].maxPacketRate;
            }
            /* Then we have a valid coder, load it */
            if (_SAPP_encodeCoderSapp2SipHelper(&coders[x],
                    &targetMedia_ptr->aCoders[y], addressType) == SAPP_OK) {
                y++;
            }
        }
    }
    /* Set the number of valid coders set */
    targetMedia_ptr->numCoders = y;
    return;
}

/*
 * ======== _SAPP_decodeCoderIsi2Sapp() ========
 *
 * This function is called when we need to load coder definitions from ISI
 * commands into an array of SAPP_Coder objects.  SAPP_Coder arrays are
 * typically in SAPP_Service or SAPP_Call objects.
 *
 *
 * Returns:
 *   Nothing.
 */
void _SAPP_decodeCoderIsi2Sapp(
        ISIP_Coder  from[],
        vint        fromSize,
        SAPP_Coder  to[],
        vint        toSize)
{
    int   x;
    char *desc_ptr;
    char *cName_ptr;
    char buffer[ISI_CODER_DESCRIPTION_STRING_SZ + 1];

    // Let's clean the target before the copy
    OSAL_memSet(to, 0, (sizeof(SAPP_Coder) * toSize));

    for (x = 0; x < fromSize && x < toSize; x++) {
        cName_ptr = from[x].szCoderName;
        if (*cName_ptr != 0) {

            OSAL_snprintf(to[x].szCoderName, sizeof(to[x].szCoderName),
                    "%s", cName_ptr);

            desc_ptr = from[x].description;
            /* Make a copy since we will parse it twice */
            OSAL_snprintf(buffer, ISI_CODER_DESCRIPTION_STRING_SZ, "%s", desc_ptr);
            
            if (!OSAL_strncasecmp(_SAPP_neg_h264, cName_ptr, ISI_CODER_STRING_SZ)) {
                _SAPP_decodeEnumRate(desc_ptr, &to[x], ";=");
                _SAPP_decodeH264Params(buffer, &to[x]);
            }
            else if (SAPP_OK == _SAPP_isH263(cName_ptr, ISI_CODER_STRING_SZ)) {
                _SAPP_decodeEnumRate(desc_ptr, &to[x], ";=");
                _SAPP_decodeH263Params(buffer, &to[x], ";=");
            }
            else if (!OSAL_strncasecmp(_SAPP_neg_amrnb, cName_ptr,
                    ISI_CODER_STRING_SZ) ||
                    !OSAL_strncasecmp(_SAPP_neg_amrwb, cName_ptr,
                    ISI_CODER_STRING_SZ)) {
                /* It's AMR or AMR-WB */
                _SAPP_decodeEnumRate(desc_ptr, &to[x], ";=");
                /* Clear mode-set and octetAlign */
                to[x].props.amr.modeSet = 0;
                to[x].props.amr.octetAlign = 0;
                _SAPP_decodeAudioParams(buffer, &to[x], ";=");
            }
            else {
                _SAPP_decodeEnumRate(desc_ptr, &to[x], ";=");
                _SAPP_decodeAudioParams(buffer, &to[x], ";=");
            }
            
            if (ISI_SESSION_TYPE_AUDIO == from[x].relates) {
                to[x].relates =  eSdpMediaAudio;
            }
            if (ISI_SESSION_TYPE_VIDEO == from[x].relates) {
                to[x].relates =  eSdpMediaVideo;
            }
        }
    }
}

/*
 * ========_SAPP_addrSip2Sapp()========
 * 
 * This function is call to translate sip network address(tNetworkAddress) to
 * sapp network address(OSAL_NetAddress)
 *
 * Returns:
 *   Nothing.
 */
void _SAPP_addrSip2Sapp(
    tNetworkAddress *sipAddr_ptr,
    OSAL_NetAddress *sappAddr_ptr)
{
    if (eNwAddrIPv6 == sipAddr_ptr->addressType) {
        sappAddr_ptr->type = OSAL_NET_SOCK_UDP_V6;
        OSAL_netIpv6AddrCpy(sappAddr_ptr->ipv6, sipAddr_ptr->x.ip.v6);
        sappAddr_ptr->port = sipAddr_ptr->port;
    }
    else {
        /* ipv4 */
        sappAddr_ptr->type = OSAL_NET_SOCK_UDP;
        sappAddr_ptr->ipv4 = sipAddr_ptr->x.ip.v4.ul;
        sappAddr_ptr->port = sipAddr_ptr->port;
    }
}

/*
 * ========_SAPP_addrSapp2Sip()========
 * 
 * This function is call to translate sapp network address(OSAL_NetAddress) to
 * sip network address(tNetworkAddress).
 *
 * Returns:
 *   Nothing.
 */
void _SAPP_addrSapp2Sip(
    OSAL_NetAddress *sappAddr_ptr,
    tNetworkAddress *sipAddr_ptr)
{
    if (OSAL_netIsAddrIpv6(sappAddr_ptr)) {
        sipAddr_ptr->addressType = eNwAddrIPv6;
        OSAL_netIpv6AddrCpy(sipAddr_ptr->x.ip.v6, sappAddr_ptr->ipv6);
    }
    else {
        sipAddr_ptr->addressType = eNwAddrIPv4;
        sipAddr_ptr->x.ip.v4.ul = sappAddr_ptr->ipv4;
    }
}

/*
 * ======== _SAPP_encodeAudioSapp2Sip() ========
 * 
 * Load audio media from ISI to MNS session
 *
 * Returns:
 *   Nothing.
 */
void _SAPP_encodeAudioSapp2Sip(
    SAPP_ServiceObj *service_ptr,
    SAPP_CallObj    *call_ptr,
    ISIP_Call       *c_ptr,
    tMedia          *media_ptr)
{
    tSession       *sess_ptr;
    OSAL_NetAddress rtpInfc;
    OSAL_NetAddress rtcpInfc;

    sess_ptr = &call_ptr->mnsSession.session;

    /* Set local ip address for audio rtp */
    rtpInfc = rtcpInfc = service_ptr->mnsService.aAddr;
    /* Set local ip address for audio rtcp */
    rtcpInfc.port = service_ptr->mnsService.aRtcpPort;

    /* Convert sapp address to sip address */
    _SAPP_addrSapp2Sip(&rtpInfc, &sess_ptr->lclAddr);

    media_ptr->mediaType = eSdpMediaAudio;
    media_ptr->lclDirection = _SAPP_mapDirIsi2Sip(c_ptr->audioDirection);
    /* Set the RTP port we will use for audio */
    media_ptr->lclRtpPort = rtpInfc.port;
    media_ptr->lclRtcpPort = rtcpInfc.port;
    /* RS and RR */
    media_ptr->bwRs = 0;
    media_ptr->bwRr = 1600; /* XXX To be updated. */

    /* Set up any security stuff if it's enabled */
    if (c_ptr->type & ISI_SESSION_TYPE_SECURITY_AUDIO) {
        media_ptr->supportSrtp = 1;
        /*
         * Offer both auth 32 and auth 80 by setting cryptoSuite to an
         * empty string.
         */
        media_ptr->cryptoSuite[0] = 0;
        call_ptr->audioKeys.type = (SRTP_AES_80 | SRTP_AES_32);
        _SAPP_generateSrtpKeys(
                call_ptr->audioKeys.lclAes80, call_ptr->audioKeys.lclAes32,
                media_ptr);
    }

    /*
     * Now load the coders.  They will first need to be translated from ISI
     * format to SIP format
     */
    _SAPP_encodeCoderSapp2Sip(call_ptr->coders, media_ptr, 
            sess_ptr->lclAddr.addressType);

    /* Set the rtp port to zero if video is not set */
    if (!(c_ptr->type & ISI_SESSION_TYPE_AUDIO)) {
        media_ptr->lclRtpPort = 0;
    }

}

/*
 * ======== _SAPP_encodeVideoSapp2Sip() ========
 * 
 * Load video media from ISI to MNS session
 *
 * Returns:
 *   Nothing.
 */
void _SAPP_encodeVideoSapp2Sip(
    SAPP_ServiceObj *service_ptr,
    SAPP_CallObj    *call_ptr,
    ISIP_Call       *c_ptr,
    tMedia          *media_ptr)
{
    tSession       *sess_ptr;
    OSAL_NetAddress rtpInfc;
    OSAL_NetAddress rtcpInfc;

    sess_ptr = &call_ptr->mnsSession.session;

    /* Set local ip address for audio rtp */
    rtpInfc = rtcpInfc = service_ptr->mnsService.vAddr;
    /* Set local ip address for audio rtcp */
    rtcpInfc.port = service_ptr->mnsService.vRtcpPort;

    /* Set the local RTP interface the result of the "stun" */
    _SAPP_addrSapp2Sip(&rtpInfc, &sess_ptr->lclAddr);

    media_ptr->mediaType = eSdpMediaVideo;
    media_ptr->lclDirection = _SAPP_mapDirIsi2Sip(c_ptr->videoDirection);
    /* Set the RTP port we will use for video */
    media_ptr->lclRtpPort = rtpInfc.port;
    media_ptr->lclRtcpPort = rtcpInfc.port;
    /* RS and RR */
    media_ptr->bwRs = 0;
    media_ptr->bwRr = 19200; /* XXX To be updated. */

    /* Set up any security stuff if it's enabled */
    if (c_ptr->type & ISI_SESSION_TYPE_SECURITY_VIDEO) {
        media_ptr->supportSrtp = 1;
        /*
         * Offer both auth 32 and auth 80 by setting cryptoSuite to an
         * empty string.
         */
        media_ptr->cryptoSuite[0] = 0;
        call_ptr->videoKeys.type = (SRTP_AES_80 | SRTP_AES_32);
        _SAPP_generateSrtpKeys(
                call_ptr->videoKeys.lclAes80, call_ptr->videoKeys.lclAes32,
                media_ptr);
    }

    /* Set supports of AVP Feedback. */
    media_ptr->supportAVPF = 1;

    /*
     * Local Video AS bandwidth is specified at media level (not at the coder level).
     * Copy the video bandwidth from ISIP_Call to tMedia.
     */
    media_ptr->lclAsBwKbps = c_ptr->lclVideoAsBwKbps;
    /*
     * Now load the coders.  They will first need to be translated from ISI
     * format to SIP format
     */
    _SAPP_encodeCoderSapp2Sip(call_ptr->coders, media_ptr, 
            sess_ptr->lclAddr.addressType);

    /* Set the rtp port to zero if video is not set */
    if (!(c_ptr->type & ISI_SESSION_TYPE_VIDEO)) {
        media_ptr->lclRtpPort = 0;
    }
}

vint _SAPP_encodeMediaSapp2Sip(
    SAPP_ServiceObj *service_ptr,
    SAPP_CallObj    *call_ptr,
    ISIP_Call       *c_ptr)
{
    tSession       *sess_ptr;
    tMedia         *media_ptr;
    vint            x;

    sess_ptr = &call_ptr->mnsSession.session;
    /* Always set session-level direction to sendrecv */
    sess_ptr->lclDirection = eSdpAttrSendRecv;

    /* Load media */
    if (0 != sess_ptr->numMedia) {
        /*
         * There is already media exists in session,
         * check if there is new media added
         */
        if (c_ptr->type & ISI_SESSION_TYPE_VIDEO) {
            for (x = 0; x < sess_ptr->numMedia; ++x) {
                /* Loop thru all media and set it if it's used */
                media_ptr = &sess_ptr->media[x];
                if (eSdpMediaVideo == media_ptr->mediaType) {
                    if (0 == media_ptr->lclRtpPort) {
                        /*
                         * This media is not eabled, enable it by
                         * set valid rtp/rtcp port.
                         */
                        media_ptr->lclRtpPort =
                                service_ptr->mnsService.vAddr.port;
                        media_ptr->lclRtcpPort =
                                service_ptr->mnsService.vRtcpPort;
                        media_ptr->lclDirection =
                                _SAPP_mapDirIsi2Sip(c_ptr->videoDirection);
                    }
                    break;
                }
            }

            if (x == sess_ptr->numMedia) {
                /* No media type found, add new one */
                MNS_loadDefaultVideo(&service_ptr->mnsService,
                        &call_ptr->mnsSession);
                /* Direction may changed by ISI, set it */
                media_ptr = &sess_ptr->media[sess_ptr->numMedia];
                media_ptr->lclDirection =
                        _SAPP_mapDirIsi2Sip(c_ptr->videoDirection);
            }
        }
        else {
            /* Video is not eabled */
            for (x = 0; x < sess_ptr->numMedia; ++x) {
                /* Loop thru all media and set it if it's used */
                media_ptr = &sess_ptr->media[x];
                if (eSdpMediaVideo == media_ptr->mediaType) {
                    /* Disable the media by setting the port to zero. */
                    media_ptr->lclRtpPort = 0;
                    media_ptr->lclRtcpPort = 0;
                    break;
                }
            }
        }

        if (c_ptr->type & ISI_SESSION_TYPE_AUDIO) {
            for (x = 0; x < sess_ptr->numMedia; ++x) {
                /* Loop thru all media and set it if it's used */
                media_ptr = &sess_ptr->media[x];
                if (eSdpMediaAudio == media_ptr->mediaType) {
                    if (0 == media_ptr->lclRtpPort) {
                        /*
                         * This media is not eabled, enable it by
                         * set valid rtp/rtcp port.
                         */
                        media_ptr->lclRtpPort =
                                service_ptr->mnsService.aAddr.port;
                        media_ptr->lclRtcpPort =
                                service_ptr->mnsService.aRtcpPort;
                        media_ptr->lclDirection =
                                _SAPP_mapDirIsi2Sip(c_ptr->audioDirection);
                    }
                    break;
                }
            }

            if (x == sess_ptr->numMedia) {
                /* No media type found, add new one */
                MNS_loadDefaultAudio(&service_ptr->mnsService,
                        &call_ptr->mnsSession);
                /* Direction may changed by ISI, set it */
                media_ptr = &sess_ptr->media[sess_ptr->numMedia];
                media_ptr->lclDirection =
                        _SAPP_mapDirIsi2Sip(c_ptr->audioDirection);
            }
        }
        else {
            /* Audio is not eabled */
            for (x = 0; x < sess_ptr->numMedia; ++x) {
                /* Loop thru all media and set it if it's used */
                media_ptr = &sess_ptr->media[x];
                if (eSdpMediaAudio == media_ptr->mediaType) {
                    /* Disable the media by setting the port to zero. */
                    media_ptr->lclRtpPort = 0;
                    media_ptr->lclRtcpPort = 0;
                    break;
                }
            }
        }
    }
    else {
        /* Media number is zero, load default media */
        /* Populate audio if the media type is enabled in the session */
        if (0 != (c_ptr->type & ISI_SESSION_TYPE_AUDIO)) {
            /* Populate Audio */
            media_ptr = &sess_ptr->media[sess_ptr->numMedia++];
            _SAPP_encodeAudioSapp2Sip(service_ptr, call_ptr, c_ptr, media_ptr);
        }

        /* Populate video if the media type is enabled in the session */
        if (c_ptr->type & ISI_SESSION_TYPE_VIDEO) {
            /* Populate Video */
            media_ptr = &sess_ptr->media[sess_ptr->numMedia++];
            _SAPP_encodeVideoSapp2Sip(service_ptr, call_ptr, c_ptr, media_ptr);
        }
    }
    return (SAPP_OK);
}



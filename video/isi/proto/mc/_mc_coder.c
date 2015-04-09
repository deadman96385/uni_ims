/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29807 $ $Date: 2014-11-12 18:48:10 +0800 (Wed, 12 Nov 2014) $
 */
#include <osal_types.h>
#include <osal.h>

#include "isi.h"
#include "isip.h"
#include "vtsp.h"
#include "_mc.h"
#include "_mc_coder.h"

static const char _MC_neg_h263_1998[] = "h263-1998";
static const char _MC_neg_h263_2000[] = "h263-2000";
static const char _MC_neg_h263[]      = "h263";
static const char _MC_neg_h264[]      = "h264";
static const char _MC_neg_amrnb[]     = "AMR";
static const char _MC_neg_amrwb[]     = "AMR-WB";
static const char _MC_neg_enum[]      = "enum";
static const char _MC_neg_rate[]      = "rate";
static const char _MC_neg_pmode[]     = "packetization-mode";
static const char _MC_neg_plevel[]    = "profile-level-id";
static const char _MC_neg_extmapid[]  = "extmapId";
static const char _MC_neg_extmapuri[] = "extmapUri";
static const char _MC_neg_annexb[]    = "annexb";
static const char _MC_neg_oa[]        = "octet-align";
static const char _MC_neg_modeset[]   = "mode-set";
static const char _MC_neg_maxbr[]     = "MAX-BR";
static const char _MC_neg_maxmbps[]   = "MAX-MBPS";
static const char _MC_neg_maxbr2[]    = "MAXBR";
static const char _MC_neg_maxmbps2[]  = "MAXMBPS";

static const char _MC_neg_SQCIF[]     = "SQCIF";
static const char _MC_neg_QCIF[]      = "QCIF";
static const char _MC_neg_CIF[]       = "CIF";
static const char _MC_neg_CIF4[]      = "CIF4";
static const char _MC_neg_CIF16[]     = "CIF16";
static const char _MC_neg_PAR[]       = "PAR";
static const char _MC_neg_CPCF[]      = "CPCF";
static const char _MC_neg_BPP[]       = "BPP";
static const char _MC_neg_HRD[]       = "HRD";
static const char _MC_neg_CUSTOM[]    = "CUSTOM";
static const char _MC_neg_PROFILE[]   = "PROFILE";
static const char _MC_neg_LEVEL[]     = "LEVEL";
static const char _MC_neg_INTERLACE[] = "INTERLACE";
static const char _MC_neg_F[]         = "F";
static const char _MC_neg_I[]         = "I";
static const char _MC_neg_J[]         = "J";
static const char _MC_neg_T[]         = "T";
static const char _MC_neg_K[]         = "K";
static const char _MC_neg_N[]         = "N";
static const char _MC_neg_P[]         = "P";

vint MC_isH263(
    char *coderName_ptr,
    vint  maxCoderLen)
{
    if (!OSAL_strncasecmp(_MC_neg_h263_1998, coderName_ptr, maxCoderLen) ||
            !OSAL_strncasecmp(_MC_neg_h263_2000, coderName_ptr, maxCoderLen) ||
            !OSAL_strncasecmp(_MC_neg_h263, coderName_ptr, maxCoderLen)) {
        return MC_OK;
    }
    return (MC_ERR);
}

vint MC_isH264(
    char *coderName_ptr,
    vint  maxCoderLen)
{
    if (!OSAL_strncasecmp(_MC_neg_h264, coderName_ptr, maxCoderLen)) {
        return MC_OK;
    }
    return (MC_ERR);
}

static void _MC_negDecodeEnumRate(
    char     *desc_ptr,
    MC_Coder *encoder_ptr,
    MC_Coder *decoder_ptr,
    char     *delimiter_ptr)
{
    char   *s_ptr;
    char   *enc_ptr;
    int    *encInt_ptr;
    int    *decInt_ptr;

    s_ptr = OSAL_strtok(desc_ptr, delimiter_ptr);
    while (NULL != s_ptr) {
        encInt_ptr = NULL;
        decInt_ptr = NULL;

        if (!OSAL_strcasecmp(s_ptr, _MC_neg_enum)) {
            encInt_ptr = &encoder_ptr->coderNum;
            decInt_ptr = &decoder_ptr->coderNum;
        }
        else if (!OSAL_strcasecmp(s_ptr, _MC_neg_rate)) {
            encInt_ptr = &encoder_ptr->rate;
            decInt_ptr = &decoder_ptr->rate;
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

static void _MC_negDecodeAudioParams(
    char     *desc_ptr,
    MC_Coder *coder_ptr,
    char     *delimiter_ptr)
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

        if (!OSAL_strcasecmp(s_ptr, _MC_neg_annexb)) {
            int_ptr = &coder_ptr->props.g729.annexb;
            checkYes = 1;
        }
        else if (!OSAL_strncasecmp(s_ptr, _MC_neg_modeset,
                OSAL_strlen(_MC_neg_modeset))) {
            int_ptr = &coder_ptr->props.amr.modeSet;
            checkModeSet = 1;
        }
        else if (!OSAL_strcasecmp(s_ptr, _MC_neg_oa)) {
            /* Found "octet-align */
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

        if (checkYes ) {
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

static void _MC_negDecodeH264Params(
    char       *desc_ptr,
    MC_Coder   *coder_ptr,
    char       *delimiter_ptr)
{
    char   *s_ptr;
    int    *int_ptr;
    char   *str_ptr;
    int     strLen;

    s_ptr = OSAL_strtok(desc_ptr, delimiter_ptr);

    while (NULL != s_ptr) {
        int_ptr = NULL;
        str_ptr = NULL;
        strLen = 0;

        if (!OSAL_strcasecmp(s_ptr, _MC_neg_pmode)) {
            int_ptr = &coder_ptr->props.h264.pmode;
        }
        else if (!OSAL_strcasecmp(s_ptr, _MC_neg_plevel)) {
            str_ptr = coder_ptr->props.h264.plevel;
            strLen = sizeof(coder_ptr->props.h264.plevel);
        }
        else if (!OSAL_strcasecmp(s_ptr, _MC_neg_extmapid)) {
            int_ptr = &coder_ptr->props.h264.extmap.id;
        }
        else if (!OSAL_strcasecmp(s_ptr, _MC_neg_extmapuri)) {
            str_ptr = coder_ptr->props.h264.extmap.uri;
            strLen = sizeof(coder_ptr->props.h264.extmap.uri);
        }
        else if (!OSAL_strcasecmp(s_ptr, _MC_neg_maxbr) ||
                !OSAL_strcasecmp(s_ptr, _MC_neg_maxbr2)) {
            int_ptr = &coder_ptr->props.h264.maxBr;
        }
        else if (!OSAL_strcasecmp(s_ptr, _MC_neg_maxmbps) ||
                !OSAL_strcasecmp(s_ptr, _MC_neg_maxmbps2)) {
            int_ptr = &coder_ptr->props.h264.maxMbps;
        }

        /*
         * We got token, let's get the value.
         */
        s_ptr = OSAL_strtok(NULL, delimiter_ptr);
        if (NULL == s_ptr) {
            break;
        }

        if (NULL != int_ptr) {
            *int_ptr = OSAL_atoi(s_ptr);
        }
        else if (0 != strLen) {
            OSAL_snprintf(str_ptr, strLen, "%s", s_ptr);
        }
    }
}

static void _MC_negDecodeH263Params(
    char       *desc_ptr,
    MC_Coder   *coder_ptr,
    char       *delimter_ptr)
{
    char   *s_ptr;
    uint8  *uint8_ptr;
    int    *int_ptr;

    H263_Params *p_ptr = &coder_ptr->props.h263;

    s_ptr = OSAL_strtok(desc_ptr, delimter_ptr);

    while (NULL != s_ptr) {
        int_ptr = NULL;
        uint8_ptr = NULL;

        if (!OSAL_strcasecmp(s_ptr, _MC_neg_SQCIF)) {
            uint8_ptr = &p_ptr->SQCIF;
        }
        else if (!OSAL_strcasecmp(s_ptr, _MC_neg_QCIF)) {
            uint8_ptr = &p_ptr->QCIF;
        }
        else if (!OSAL_strcasecmp(s_ptr, _MC_neg_CIF)) {
            uint8_ptr = &p_ptr->CIF;
        }
        else if (!OSAL_strcasecmp(s_ptr, _MC_neg_CIF4)) {
            uint8_ptr = &p_ptr->CIF4;
        }
        else if (!OSAL_strcasecmp(s_ptr, _MC_neg_CIF16)) {
            uint8_ptr = &p_ptr->CIF16;
        }
        else if (!OSAL_strcasecmp(s_ptr, _MC_neg_PAR)) {
             if (NULL == (s_ptr = OSAL_strtok(NULL, ":"))) break;
             p_ptr->PAR_ratio.left = (uint8)OSAL_atoi(s_ptr);
             if (NULL == (s_ptr = OSAL_strtok(NULL, delimter_ptr))) break;
             p_ptr->PAR_ratio.right = (uint8)OSAL_atoi(s_ptr);
             continue;
        }
        else if (!OSAL_strcasecmp(s_ptr, _MC_neg_CPCF)) { /* Load the 8 comma delimited values */
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
        else if (!OSAL_strcasecmp(s_ptr, _MC_neg_BPP)) {
            int_ptr = &p_ptr->BPP;
        }
        else if (!OSAL_strcasecmp(s_ptr, _MC_neg_HRD)) {
            uint8_ptr = &p_ptr->HRD;
        }
        else if (!OSAL_strcasecmp(s_ptr, _MC_neg_CUSTOM)) { /* Load the 3 comma delimited values */
            if (NULL == (s_ptr = OSAL_strtok(NULL, ","))) break;
            p_ptr->CUSTOM.Xmax = OSAL_atoi(s_ptr);
            if (NULL == (s_ptr = OSAL_strtok(NULL, ","))) break;
            p_ptr->CUSTOM.Ymax = OSAL_atoi(s_ptr);
            if (NULL == (s_ptr = OSAL_strtok(NULL, delimter_ptr))) break;
            p_ptr->CUSTOM.MPI = OSAL_atoi(s_ptr);
            continue;
        }
        else if (!OSAL_strcasecmp(s_ptr, _MC_neg_PROFILE)) {
            uint8_ptr = &p_ptr->PROFILE;
        }
        else if (!OSAL_strcasecmp(s_ptr, _MC_neg_LEVEL)) {
            uint8_ptr = &p_ptr->LEVEL;
        }
        else if (!OSAL_strcasecmp(s_ptr, _MC_neg_INTERLACE)) {
            uint8_ptr = &p_ptr->INTERLACE;
        }
        else if (!OSAL_strcasecmp(s_ptr, _MC_neg_F)) {
            uint8_ptr = &p_ptr->annex.F;
        }
        else if (!OSAL_strcasecmp(s_ptr, _MC_neg_I)) {
            uint8_ptr = &p_ptr->annex.I;
        }
        else if (!OSAL_strcasecmp(s_ptr, _MC_neg_J)) {
            uint8_ptr = &p_ptr->annex.J;
        }
        else if (!OSAL_strcasecmp(s_ptr, _MC_neg_T)) {
            uint8_ptr = &p_ptr->annex.T;
        }
        else if (!OSAL_strcasecmp(s_ptr, _MC_neg_K)) {
            uint8_ptr = &p_ptr->annex.K;
        }
        else if (!OSAL_strcasecmp(s_ptr, _MC_neg_N)) {
            uint8_ptr = &p_ptr->annex.K;
        }
        else if (!OSAL_strcasecmp(s_ptr, _MC_neg_P)) {
            uint8_ptr = &p_ptr->annex.K;
        }
        else if (!OSAL_strcasecmp(s_ptr, _MC_neg_maxbr) ||
                !OSAL_strcasecmp(s_ptr, _MC_neg_maxbr2)) {
            int_ptr = &p_ptr->maxBr;
        }
        else if (!OSAL_strcasecmp(s_ptr, _MC_neg_maxmbps) ||
                !OSAL_strcasecmp(s_ptr, _MC_neg_maxmbps2)) {
            int_ptr = &p_ptr->maxMbps;
        }

        /*
         * We got token, let's get the value.
         */
        s_ptr = OSAL_strtok(NULL, delimter_ptr);
        if (NULL == s_ptr) {
            break;
        }

        if (NULL != int_ptr) {
            *int_ptr = OSAL_atoi(s_ptr);
        }
        else if (NULL != uint8_ptr) {
            *uint8_ptr = (uint8)OSAL_atoi(s_ptr);
        }
    }
}

void MC_parseCoder(
    char     *name_ptr,
    char     *desc_ptr,    
    MC_Coder *encoder_ptr,
    MC_Coder *decoder_ptr)
{
    char      buffer[ISI_CODER_DESCRIPTION_STRING_SZ + 1];
    
    /* Make a copy of the description since we need to parse it twice */
    OSAL_snprintf(buffer, ISI_CODER_DESCRIPTION_STRING_SZ, "%s", desc_ptr);
    
    if (MC_OK == MC_isH264(name_ptr, ISI_CODER_STRING_SZ)) {
        _MC_negDecodeH264Params(buffer, encoder_ptr, ";=");
        /* Copy parameters from encoder to decoder */
        *decoder_ptr = *encoder_ptr;
        _MC_negDecodeEnumRate(desc_ptr, encoder_ptr, decoder_ptr, ";=");
    }
    else if (MC_OK == MC_isH263(name_ptr, ISI_CODER_STRING_SZ)) {
        _MC_negDecodeH263Params(buffer, encoder_ptr, ";=");
        /* Copy parameters from encoder to decoder */
        *decoder_ptr = *encoder_ptr;
        _MC_negDecodeEnumRate(desc_ptr, encoder_ptr, decoder_ptr, ";=");
    }
    else if (!OSAL_strncasecmp(_MC_neg_amrnb, name_ptr, ISI_CODER_STRING_SZ) ||
            !OSAL_strncasecmp(_MC_neg_amrwb, name_ptr, ISI_CODER_STRING_SZ)) {
        /* Clear mode-set and octetAlign */
        encoder_ptr->props.amr.modeSet    = 0;
        encoder_ptr->props.amr.octetAlign = 0;
        _MC_negDecodeAudioParams(buffer, encoder_ptr, ";=");
        /* Copy parameters from encoder to decoder */
        *decoder_ptr = *encoder_ptr;
        _MC_negDecodeEnumRate(desc_ptr, encoder_ptr, decoder_ptr, ";=");
    }
    else {
        _MC_negDecodeAudioParams(buffer, encoder_ptr, ";=");
        /* Copy parameters from encoder to decoder */
        *decoder_ptr = *encoder_ptr;
        _MC_negDecodeEnumRate(desc_ptr, encoder_ptr, decoder_ptr, ";=");
    }
    return;
}

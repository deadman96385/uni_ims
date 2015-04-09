/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 14555 $ $Date: 2011-04-21 03:24:35 +0800 (Thu, 21 Apr 2011) $
 */

#ifndef _SAPP_CODER_HELPER_H_
#define _SAPP_CODER_HELPER_H_

#include "_sapp.h"

static const char _SAPP_neg_h263_1998[]   = "H263-1998";
static const char _SAPP_neg_h263_2000[]   = "H263-2000";
static const char _SAPP_neg_h263[]        = "H263";
static const char _SAPP_neg_h264[]        = "H264";
static const char _SAPP_neg_g729[]        = "G729";
static const char _SAPP_neg_ilbc[]        = "iLBC";
static const char _SAPP_neg_silk_24k[]    = "SILK-24k";
static const char _SAPP_neg_silk_16k[]    = "SILK-16k";
static const char _SAPP_neg_silk_8k[]     = "SILK-8k";
static const char _SAPP_neg_silk[]        = "SILK";
static const char _SAPP_neg_g722[]        = "G722";
static const char _SAPP_neg_g7221[]       = "G7221";
static const char _SAPP_neg_pcmu[]        = "PCMU";
static const char _SAPP_neg_pcma[]        = "PCMA";
static const char _SAPP_neg_cn[]          = "CN";
static const char _SAPP_neg_g726_32[]     = "G726-32";
static const char _SAPP_neg_mode[]        = "mode";
static const char _SAPP_neg_tel_evt[]     = "telephone-event";
static const char _SAPP_neg_tel_evt_16k[] = "telephone-event-16k";
static const char _SAPP_neg_enum[]        = "enum";
static const char _SAPP_neg_rate[]        = "rate";
static const char _SAPP_neg_maxptime[]    = "maxptime";
static const char _SAPP_neg_framesize[]   = "framesize";
static const char _SAPP_neg_framerate[]   = "framerate";
static const char _SAPP_neg_sps[]         = "sps";
static const char _SAPP_neg_sps_long[]    = "sprop-parameter-sets";
static const char _SAPP_neg_pmode[]       = "packetization-mode";
static const char _SAPP_neg_plevel[]      = "profile-level-id";
static const char _SAPP_neg_extmapid[]    = "extmapId";
static const char _SAPP_neg_extmapuri[]   = "extmapUri";
static const char _SAPP_neg_annexb[]      = "annexb";
static const char _SAPP_neg_maxbr[]       = "MAX-BR";
static const char _SAPP_neg_maxmbps[]     = "MAX-MBPS";
static const char _SAPP_neg_maxbr2[]      = "MAXBR";
static const char _SAPP_neg_maxmbps2[]    = "MAXMBPS";
static const char _SAPP_neg_amrnb[]       = "AMR";
static const char _SAPP_neg_amrwb[]       = "AMR-WB";
static const char _SAPP_neg_oa[]          = "octet-align";
static const char _SAPP_neg_modeset[]     = "mode-set";
static const char _SAPP_neg_mcc[]         = "mode-change-capability=2";
static const char _SAPP_neg_SQCIF[]       = "SQCIF";
static const char _SAPP_neg_QCIF[]        = "QCIF";
static const char _SAPP_neg_CIF[]         = "CIF";
static const char _SAPP_neg_CIF4[]        = "CIF4";
static const char _SAPP_neg_CIF16[]       = "CIF16";
static const char _SAPP_neg_PAR[]         = "PAR";
static const char _SAPP_neg_CPCF[]        = "CPCF";
static const char _SAPP_neg_BPP[]         = "BPP";
static const char _SAPP_neg_HRD[]         = "HRD";
static const char _SAPP_neg_CUSTOM[]      = "CUSTOM";
static const char _SAPP_neg_PROFILE[]     = "PROFILE";
static const char _SAPP_neg_LEVEL[]       = "LEVEL";
static const char _SAPP_neg_INTERLACE[]   = "INTERLACE";
static const char _SAPP_neg_F[]           = "F";
static const char _SAPP_neg_I[]           = "I";
static const char _SAPP_neg_J[]           = "J";
static const char _SAPP_neg_T[]           = "T";
static const char _SAPP_neg_K[]           = "K";
static const char _SAPP_neg_N[]           = "N";
static const char _SAPP_neg_P[]           = "P";
static const char _SAPP_neg_yes[]         = "yes";

tSdpAttrType _SAPP_mapDirIsi2Sip(
    ISI_SessionDirection direction);

ISI_SessionDirection _SAPP_mapDirSip2Isi(
    tSdpAttrType direction);

vint _SAPP_encodeCoder(
    ISIP_Coder  *c_ptr,
    tMediaCoder *t_ptr,
    uint8        packetRate);

void _SAPP_decodeCoderIsi2Sapp(
        ISIP_Coder  from[],
        vint        fromSize,
        SAPP_Coder  to[],
        vint        toSize);

tSdpAttrType SAPP_negMapDirIsi2Sip(
    ISI_SessionDirection direction);

void _SAPP_encodeCoderSapp2Sip(
    SAPP_Coder         coders[],
    tMedia            *targetMedia_ptr,
    tNetworkAddrType   addressType);

void _SAPP_addrSip2Sapp(
    tNetworkAddress *sipAddr_ptr,
    OSAL_NetAddress *sappAddr_ptr);

void _SAPP_addrSapp2Sip(
    OSAL_NetAddress *sappAddr_ptr,
    tNetworkAddress *sipAddr_ptr);

void _SAPP_updateMediaSapp2Sip(
    SAPP_CallObj    *call_ptr,
    ISIP_Call       *c_ptr);

vint _SAPP_encodeMediaSapp2Sip(
    SAPP_ServiceObj *service_ptr,
    SAPP_CallObj    *call_ptr,
    ISIP_Call       *c_ptr);

#endif

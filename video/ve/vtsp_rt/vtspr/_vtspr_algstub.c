/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28394 $ $Date: 2014-08-21 10:24:05 +0800 (Thu, 21 Aug 2014) $
 *
 */
#include "vtspr.h"

#ifndef VTSP_ENABLE_ALGS

#if 0
void COMM_a2lin(
    vint  *dst_ptr,  /* destination buffer pointer */
    vint  *src_ptr,  /* source buffer pointer */
    uvint  length)   /* length */
{
    return;
}


void COMM_attn(
    vint  *dst_ptr,  /* destination buffer pointer */
    vint  *src_ptr,  /* source buffer pointer may be same as dst_ptr */
    vint   attn,     /* Attn value in dBm, 1/2 dB steps (0 to -159) */
    uvint  length)   /* length */
{
    return;
}


void COMM_copy(
    vint  *dst_ptr,  /* destination buffer pointer */
    vint  *src_ptr,  /* source buffer pointer */
    uvint  length)   /* length */
{
    return;
}


uvint COMM_db2lin(
    uvint db)        /* (dBm value - 20) dBm in 0.5 dB units */ 
{
    return (0);
}


void COMM_fill(
    vint  *dst_ptr,  /* destination buffer pointer */
    vint   fill,     /* Fill pattern */
    uvint  length)   /* length */
{
    return;
}


void COMM_lin2a(
    vint  *dst_ptr,  /* destination buffer pointer */
    vint  *src_ptr,  /* source buffer pointer */
    uvint  length)   /* length */
{
    return;
}


vint COMM_lin2db(
    uvint lin)       /* the linear value, in range 1 to 32767 */
{
    return (0);
}


void COMM_lin2mu(
    vint  *dst_ptr,  /* destination buffer pointer */
    vint  *src_ptr,  /* source buffer pointer */
    uvint  length)   /* length */
{
    return;
}


uvint COMM_lrms(
    vint  *src_ptr,  /* source buffer pointer */
    uvint  length)   /* length */
{
    return (0);
}


void COMM_mu2lin(
    vint  *dst_ptr,  /* destination buffer pointer */
    vint  *src_ptr,  /* source buffer pointer */
    uvint  length)   /* length */
{
    return;
}


void COMM_scale(
    vint  *dst_ptr,  /* destination buffer pointer */
    vint  *src_ptr,  /* source buffer pointer may be same as dst_ptr */
    vint   value,    /* linear scale value */
    uvint  length)   /* length */
{
    return;
}


void COMM_shiftLeft(
    vint  *dst_ptr,  /* destination buffer pointer */
    vint  *src_ptr,  /* source buffer pointer may be same as dst_ptr */
    uvint  value,    /* number of bits to shift */
    uvint  length)   /* length */
{
    return;
}


void COMM_shiftRight(
    vint  *dst_ptr,  /* destination buffer pointer */
    vint  *src_ptr,  /* source buffer pointer may be same as dst_ptr */
    uvint  value,    /* number of bits to shift */
    uvint  length)   /* length */

{
    return;
}


uvint COMM_sqrt(
    uint32 num)       /* number */
{
    return (0);
}

void COMM_sum(
    vint  *dst_ptr,  /* destination buffer pointer */
    vint  *src_ptr,  /* source buffer pointer may be same as dst_ptr */
    uvint  length)   /* length */
{
    return;
}

void COMM_pack32(
    uint32 *dst_ptr,  /* destination buffer pointer */
    vint   *src_ptr,  /* source buffer pointer */
    uvint   length)   /* length */
{
    return;
}

void COMM_unpack32(
    vint   *dst_ptr,  /* destination buffer pointer */
    uint32 *src_ptr,  /* source buffer pointer */
    uvint   length)   /* length */
{
    return;
}

void COMM_octetCopy(
    int8  *dst_ptr,  /* destination buffer pointer */
    int8  *src_ptr,  /* source buffer pointer */
    uvint  length)   /* length in octets */
{
    return;
}

void COMM_octetFill(
    int8  *dst_ptr,  /* destination buffer pointer */
    int8   fill,     /* fill pattern */
    uvint  length)   /* length in octets */
{
    return;
}
#endif
#ifdef VTSP_ENABLE_DTMF
void DTMF_init(DTMF_Handle obj_ptr, GLOBAL_Params *p, DTMF_Params *dp)
{
    return;
}

void DTMF_detect(DTMF_Handle obj_ptr)
{
    return;
}

void DTMF_remove(DTMF_Handle obj_ptr)
{
    return;
}
#endif

#ifdef VTSP_ENABLE_ECSR
void ECSR_init(
    const EC_Handle  ecObj_ptr, 
    const vint       p0DBIN, 
    const EC_Params *ecParams_ptr, 
    const vint      mode) 
{
    return;
}

void ECSR_isrRun(
    EC_Handle ecObj_ptr)
{
    return;
}

void ECSR_run(
    const EC_Handle  ecObj_ptr, 
    const NFE_Handle nfeObj_ptr) 
{
    return;
}
#endif

#ifdef VTSP_ENABLE_FMTD
void FMTD_init(                      /* For FMTD object initialization */
        FMTD_Handle    fmtd_ptr,     /* pointer to struct of FMTD object */
        GLOBAL_Params *global_ptr,   /* ref. input/output level for 0 dBm */
        FMTD_Params   *local_ptr)
{
    return;
}


void FMTD_detect(
        FMTD_Handle    fmtd_ptr)
{
    return;
}


void FMTD_fullDuplexDetect(
        FMTD_Handle    fmtd1_ptr, 
        FMTD_Handle    fmtd2_ptr)
{
    return;
}
#endif


#ifdef VTSP_ENABLE_G726
void G726_init(
    G726_Obj *g726Obj_ptr,  /* pointer to G726_Obj */
    vint      mode)         /* data rate and data type */
{
    return;
}

vint  G726_encode(
    G726_Obj *g726Obj_ptr)  /* pointer to G726_Obj */
{
    return (0);
}

vint  G726_decode(
    G726_Obj *g726Obj_ptr)  /* pointer to G726_Obj */
{
    return (0);
}

void G726_init_encode(
    vint input,
    G726_Obj *g726Obj_ptr)
{
    return;
}

void G726_init_decode(
    vint input,
    G726_Obj *g726Obj_ptr)
{
    return;
}

#endif

void NFE_init(
    NFE_Handle     nfeObject_ptr,
    GLOBAL_Params *globalParam_ptr,
    NFE_Local     *nfeLocalParam_ptr, 
    int32          startMode)
{
    return;
}


void NFE_run(
    NFE_Handle     nfeObject_ptr)
{
    return;
}
#if 0
void NLP_init(
    NLP_Obj       *nlp_ptr, 
    GLOBAL_Params *global_ptr, 
    NLP_Params    *nlpParams_ptr)
{
    return;
}

void NLP_run(
    NLP_Obj    *nlp_ptr, 
    void       *ec_ptr, 
    NFE_Handle  nfe_ptr)
{
    return;
}


void NSE_init(
    NSE_Obj       *nseObj_ptr,  /* Pointer to NSE object struct */
    GLOBAL_Params *globals_ptr, /* Pointer to global parameters struct */
    NSE_Params    *locals_ptr)
{
    return;
}


void NSE_generate(
    NSE_Obj *nseObj_ptr)
{
    return;
}
void PLC_init(
    PLC_Obj *plcObj_ptr)
{
    return;
}


void PLC_run(
    PLC_Obj *plcObj_ptr)
{
    return;
}

void TONE_init(
    TONE_Obj      *toneObj_ptr,  /* Pointer to TONE object struct */
    GLOBAL_Params *globals_ptr,  /* Pointer to global parameters struct */
    TONE_Params   *locals_ptr)
{
    return;
}


int TONE_generate(
    TONE_Obj *toneObj_ptr)
{
    return (0);
}
#endif
#ifdef VTSP_ENABLE_G729
void G729AB_encode_init(
        G729AB_EncObj *encObj_ptr)
{
    return;
}

void G729AB_decode_init(
        G729AB_DecObj *decObj_ptr)
{
    return;
}

vint G729AB_encode(
        G729AB_EncObj *encObj_ptr,
        vint           speechIn[],
        int16          packedWords[])
{
    return (0);
}

void G729AB_decode(
        G729AB_DecObj *decObj_ptr,
        int16          parameters[],
        vint           speechOut[],
        vint           nBytes,
        vint           bfi)
{
    return;
}

void G729AB_pack (Word16 prm[], Word16 packedWords[], Word16 nBytes)
{
    return;
}

void G729AB_unpack (Word16 packedWords[], Word16 prm[], Word16 nBytes)
{
    return;
}
#endif
#ifdef VTSP_ENABLE_G722
void G722_encode_init(
        G722_EncObj *encObj_ptr)
{
    return;
}

void G729AB_decode_init(
        G722_DecObj *decObj_ptr)
{
    return;
}

vint G722_encode(
        G722_EncObj *encObj_ptr,
        vint           speechIn[],
        int16          packedWords[])
{
    return (0);
}

void G722_decode(
        G729AB_DecObj *decObj_ptr,
        int16          parameters[],
        vint           speechOut[],
        vint           nBytes,
        vint           bfi)
{
    return;
}
#endif
#ifdef VTSP_ENABLE_CIDS
void FSKS_init(
    FSKS_Handle    fsksObj_ptr, 
    GLOBAL_Params *global_ptr, 
    FSKS_LParams  *fsksParams_ptr, 
    uint8         *src_ptr, 
    int16          messageLen)
{
    return;
}

int16  FSKS_run(FSKS_Handle fsksObj_ptr)
{
    return (0);
}
#endif

#if 0
vint DCRM_init(
    DCRM_Obj    *dcrmObj_ptr, /* Pointer to DCRM object */
    DCRM_Params *locals_ptr)  /* Pointer to DCRM local parameters struct */
{
    return (0);
}

vint DCRM_run(
    DCRM_Obj *dcrmObj_ptr)  /* Pointer to DCRM object */
{
    return (0);
}
#endif
#ifdef VTSP_ENABLE_FXO
void UTD_translate(
    void           *userTone_ptr,
    void           *dspTone_ptr)
{
    return;
}

void UTD_init(
    UTD_Obj        *utd_ptr,
    int16         **toneParams_ptr,
    GLOBAL_Params  *globals_ptr)
{
    return;
}

void UTD_detect(
    UTD_Obj        *utd_ptr,
    NFE_Object     *nfe_ptr)
{
    return;
}


void CAS_init(
    CAS_Obj       *casObj_ptr,
    GLOBAL_Params *globals_ptr,
    vint           warmInit)
{
    return;
}

vint CAS_run(
    CAS_Obj *casObj_ptr)
{
    return (0);
}

void FSKR_init(
    FSKR_Handle     fskrObj_ptr,
    GLOBAL_Params  *globals_ptr,
    FSKR_Params    *fskrParams_ptr,
    vint            mode)
{
    return;
}

vint FSKR_run(
    FSKR_Handle     fskrObj_ptr)
{
    return (0);
}
#endif


#ifdef VTSP_ENABLE_G723
void G723A_encodeInit(
        G723A_EncObj *enc_ptr)
{
    return;
}
void G723A_decodeInit(
        G723A_DecObj *dec_ptr)
{
    return;
}
vint G723A_encode(     
        G723A_EncObj *enc_ptr)
{
    static vint encCount;

    encCount++;
    encCount &= 0x0f;
    switch (encCount) { 
        case 0:
        case 1:
        case 2:
        case 3:
            return (20);    /* 6.3kb pktSz */
        case 4:
        case 5:
        case 6:
        case 7:
            return (1);     /* CN pktSz */
        case 8:
        case 9:
        case 10:
        case 11:
            return (24);    /* 5.3kb pktSz */
        case 12:
        case 13:
        case 14:
        case 15:
            return (1);     /* CN pktSz */
    }

    return (0);
}
void G723A_decode(     
        G723A_DecObj *dec_ptr)
{
    return;
}
#endif

#endif
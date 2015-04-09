/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * +D2Tech+ Rev: 3238  Date: 2008-10-30 12:15:07 -0700 (Thu, 30 Oct 2008) 
 * +D2Tech+ Release Version: alg_core3-g723a
 */
/*
**
** File:        "g723a.h"
**
** Description:
**    This file contains D2's public and private definition of the
**    SG15 LBC Coder for 6.3/5.3 kbps.
**    Specification: ITU-T G.723.1 Annex A
**
*/
#ifndef _G723A_H_
#define _G723A_H_

#include "d2types.h"

/* 
 * Processor-specific information MIPS 24KE and MIPS 34K (DSP instructions)
 * Worst-case Enc+Dec      Instructions (MIPS)   Cycles (MHz.) 
 *   Rate = 5.3 kbit/s            35.8              46.7
 *   Rate = 6.3 kbit/s            45.7              61.5
 *
 * Processor-specific information MIPS 4Kc or other MIPS32R2 (no DSP/ASE)
 * Worst-case Enc+Dec      Instructions (MIPS)   Cycles (MHz.) 
 *   Rate = 5.3 kbit/s            49.7              62.1
 *   Rate = 6.3 kbit/s            61.8              78.6
 *
 * Worst-case encoder on all ITU test vectors, plus worst-case decoder on
 * all ITU test vectors.
 */

/* 
 * **************************************************
 * *  Beginning of "private" ITU header information *
 * *    (unreferenced by test or example programs)  *
 * **************************************************
 */

/*
** Types definitions - replaces g723a_typedef.h
*/
typedef int32  Word32;
typedef int16  Word16;
typedef uint16 UWord16;
typedef int16  Flag;

/* Coder rate */
enum  G723A_Crate { Rate63, Rate53 };   /* D2's API requires that Rate63=0 */

/* Coder global constants */
#define G723A_Frame       240
#define G723A_LpcFrame    180
#define G723A_SubFrames   4
#define G723A_SubFrLen    (G723A_Frame/G723A_SubFrames)

/* LPC constants */
#define G723A_LpcOrder    10

/* LTP constants */
#define G723A_PitchMin    18
#define G723A_PitchMax    (G723A_PitchMin+127)

/* CNG constants */
#define G723A_NbAvAcf     3  /* Nb of frames for Acf average */
#define G723A_NbAvGain    3  /* Nb of frames for gain average */
#define G723A_NbPulsBlk   11 /* Nb of pulses in 2-subframes blocks */

#define G723A_LpcOrderP1  (G723A_LpcOrder+1)
/* size of array Acf */
#define G723A_SizAcf      ((G723A_NbAvAcf+1)*G723A_LpcOrderP1)

/* Taming constants */
#define G723A_SizErr      5

/*
   Used structures
*/
    /* VAD static variables */
typedef struct {
    Word32  Penr ;
    Word32  Nlev ;
    Word16  Hcnt ;
    Word16  Vcnt ;
    Word16  Polp[4] ;
    Word16  Aen ;
#ifdef OSAL_WINCE
    __declspec(align(4)) Word16  NLpc[G723A_LpcOrder];
#else
    Word16  NLpc[G723A_LpcOrder] __attribute__((aligned(4)));
#endif
} VADSTATDEF ;
typedef VADSTATDEF G723A_VadStat;  /* adhere to D2 naming conventions */


/* CNG features */

/* Coder part */
typedef struct {
    Word16 CurGain;
    Word16 PastFtyp;
    Word16 ShRC;
    Word16 NbEner;
    Word16 IRef;
    Word16 SidGain;
    Word16 RandSeed;
#ifdef OSAL_WINCE
    __declspec(align(4)) Word16 Ener[G723A_NbAvGain];
    __declspec(align(4)) Word16 LspSid[G723A_LpcOrder];
#else
    Word16 Ener[G723A_NbAvGain] __attribute__((aligned(4)));
    Word16 LspSid[G723A_LpcOrder] __attribute__((aligned(4)));
#endif
    Word16 SidLpc[G723A_LpcOrder] ;
    Word16 RC[G723A_LpcOrderP1];
#ifdef OSAL_WINCE
    __declspec(align(4)) Word16 ShAcf[G723A_NbAvAcf + 1];
    __declspec(align(4)) Word16 Acf[G723A_SizAcf];
#else
    Word16 ShAcf[G723A_NbAvAcf + 1] __attribute__((aligned(4)));
    Word16 Acf[G723A_SizAcf] __attribute__((aligned(4)));
#endif
} CODCNGDEF;
typedef CODCNGDEF G723A_CodCng;  /* adhere to D2 naming conventions */

/* Decoder part */
typedef struct {
    Word16 CurGain;
    Word16 PastFtyp;
    Word16 SidGain;
    Word16 RandSeed;
#ifdef OSAL_WINCE
    __declspec(align(4)) Word16  LspSid[G723A_LpcOrder];
#else
    Word16 LspSid[G723A_LpcOrder] __attribute__((aligned(4)));
#endif
} DECCNGDEF;
typedef DECCNGDEF G723A_DecCng;  /* adhere to D2 naming conventions */


typedef struct {
   /* Taming procedure errors */
   Word32 Err[G723A_SizErr];

   /* High pass variables */
   Word32   HpfPdl   ;
   Word16   HpfZdl   ;

   /* Sine wave detector */
   Word16   SinDet   ;

   /* Lsp previous vector */
   Word16   PrevLsp[G723A_LpcOrder] ;

   /* Used delay lines */
   Word16   WghtFirDl[G723A_LpcOrder] ;
   Word16   WghtIirDl[G723A_LpcOrder] ;
   Word16   RingFirDl[G723A_LpcOrder] ;
   Word16   RingIirDl[G723A_LpcOrder] ;

   /* Required memory for the delay */
   Word16   PrevDat[G723A_LpcFrame-G723A_SubFrLen] ;

   /* All pitch operation buffers */
#ifdef OSAL_WINCE
    __declspec(align(4)) Word16   PrevWgt[G723A_PitchMax];
    __declspec(align(4)) Word16   PrevErr[G723A_PitchMax];
    __declspec(align(4)) Word16   PrevExc[G723A_PitchMax];
#else
   Word16   PrevWgt[G723A_PitchMax] __attribute__((aligned(4)));
   Word16   PrevErr[G723A_PitchMax] __attribute__((aligned(4)));
   Word16   PrevExc[G723A_PitchMax] __attribute__((aligned(4)));
#endif

} CODSTATDEF  ;
typedef CODSTATDEF G723A_CodStat;  /* adhere to D2 naming conventions */

typedef  struct   {
   Word16   Ecount ;
   Word16   InterGain ;
   Word16   InterIndx ;
   Word16   Rseed ;
   Word16   Park  ;
   Word16   Gain  ;
   /* Lsp previous vector */
#ifdef OSAL_WINCE
    __declspec(align(4)) Word16   PrevLsp[G723A_LpcOrder];
#else
   Word16   PrevLsp[G723A_LpcOrder] __attribute__((aligned(4)));
#endif

   /* Used delay lines */
#ifdef OSAL_WINCE
    __declspec(align(4)) Word16   SyntIirDl[G723A_LpcOrder];
    __declspec(align(4)) Word16   PostIirDl[G723A_LpcOrder];
    __declspec(align(4)) Word16   PostFirDl[G723A_LpcOrder];
#else
   Word16   SyntIirDl[G723A_LpcOrder] __attribute__((aligned(4)));
   Word16   PostIirDl[G723A_LpcOrder] __attribute__((aligned(4)));
   Word16   PostFirDl[G723A_LpcOrder] __attribute__((aligned(4)));
#endif

   /* All pitch operation buffers */
   Word16   PrevExc[G723A_PitchMax] ;

} DECSTATDEF;
typedef DECSTATDEF G723A_DecStat;  /* adhere to D2 naming conventions */

/* 
 * **************************************************
 * *     End of "private" ITU header information    *
 * **************************************************
 */

/* 
 * Coder rate
 */
#define G723A_WRKRATE_53       (1)            /* Rate53, 5.3 kb/s */
#define G723A_WRKRATE_63       (0)            /* Rate63, 6.3 kb/s */

/* 
 * Coder global constants
 */
#define G723A_FRAME            (240)          /* G723A_Frame */
#define G723A_MAX_CODED_BYTES  (24)           /* MAX( 24, 20) */
#define G723A_USEHP_ENABLE     (1)
#define G723A_USEHP_DISABLE    (0)
#define G723A_USEPF_ENABLE     (1)
#define G723A_USEPF_DISABLE    (0)
#define G723A_USEVX_ENABLE     (1)
#define G723A_USEVX_DISABLE    (0)
#define G723A_CRC_PACKETLOSS   (1)
#define G723A_CRC_NORMAL       (0)

/* 
 * Total of 107 words (428 bytes) for decoder object (x86 or MIPS32)
 */
typedef struct {
    G723A_DecStat  decStat;
    G723A_DecCng   decCng;
    vint           wrkRate;     /* output, ITU enum WkrRate */
    int16          usePf;       /* input, ITU UsePf */
    int16          crc;         /* input, ITU Crc */
    uint8         *src_ptr;
    vint          *dst_ptr;
} G723A_DecObj;

/*
 * Total of 372 words (1488 bytes) for encoder object (x86 or MIPS32)
 */
typedef struct {
#ifdef OSAL_WINCE
    __declspec(align(4)) G723A_CodStat  codStat;
    __declspec(align(4)) G723A_VadStat  vadStat;
    __declspec(align(4)) G723A_CodCng   codCng;
#else
    G723A_CodStat  codStat      __attribute__((aligned(4)));
    G723A_VadStat  vadStat      __attribute__((aligned(4)));
    G723A_CodCng   codCng       __attribute__((aligned(4)));
#endif
    vint           wrkRate;     /* input, ITU enum WrkRate */
    int16          useVx;       /* input, ITU UseVx */
    int16          useHp;       /* input, ITU UseHp */
    vint          *src_ptr;
    uint8         *dst_ptr;
} G723A_EncObj;

/* 
 * Private function interfaces for direct testing of ITU algorithms
 */
Flag G723A_ITU_encode(G723A_EncObj *encObj, Word16 *DataBuff, char *Vout );
Flag G723A_ITU_decode(G723A_DecObj *decObj, Word16 *DataBuff, char *Vinp);

/* 
 * Public function interfaces for D2's API
 */
void G723A_encodeInit(
        G723A_EncObj *enc_ptr);
void G723A_decodeInit(
        G723A_DecObj *dec_ptr);
vint G723A_encode(     
        G723A_EncObj *enc_ptr);
void G723A_decode(     
        G723A_DecObj *dec_ptr);

#endif /* ifndef _G723A_H_ */

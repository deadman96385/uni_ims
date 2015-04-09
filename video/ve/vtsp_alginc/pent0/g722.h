/*                                                   Version: 2.00 - 01.Jul.95
  ============================================================================

                          U    U   GGG    SSSS  TTTTT
                          U    U  G       S       T
                          U    U  G  GG   SSSS    T
                          U    U  G   G       S   T
                           UUU     GG     SSS     T

                   ========================================
                    ITU-T - USER'S GROUP ON SOFTWARE TOOLS
                   ========================================


       =============================================================
       COPYRIGHT NOTE: This source code, and all of its derivations,
       is subject to the "ITU-T General Public License". Please have
       it  read  in    the  distribution  disk,   or  in  the  ITU-T
       Recommendation G.191 on "SOFTWARE TOOLS FOR SPEECH AND  AUDIO
       CODING STANDARDS". 
       ** This code has  (C) Copyright by CNET Lannion A TSS/CMC **
       =============================================================


MODULE:         USER-LEVEL FUNCTIONS FOR THE UGST G.722 MODULE

ORIGINAL BY:
   Simao Ferraz de Campos Neto
   COMSAT Laboratories                    Tel:    +1-301-428-4516
   22300 Comsat Drive                     Fax:    +1-301-428-9287
   Clarksburg MD 20871 - USA              E-mail: simao.campos@labs.comsat.com
    
   History:
   14.Mar.95    v1.0    Released for use ITU-T UGST software package Tool
                        based on the CNET's 07/01/90 version 2.00
   01.Jul.95    v2.0    Changed function declarations to work with 
                        many compilers; reformated <simao@ctd.comsat.com>
  ============================================================================
*/
/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * +D2Tech+ Rev: 2052  Date: 2007-07-24 13:55:31 -0700 (Tue, 24 Jul 2007) 
 * +D2Tech+ Release Version: alg_core3-g722
 */


#ifndef _G722_H_
#define _G722_H_

#include <d2types.h>

#define _G722_D2MOD                 /* Run optimized code */

#define G722_SPEECH_NSAMPLES_10MS   (160)
#define G722_CODED_BYTES_10MS       (G722_SPEECH_NSAMPLES_10MS / 2)

/* Modes of operation */
#define G722_BIT_RATE_64KBPS            (1)
#define G722_BIT_RATE_48KBPS            (2)
#define G722_BIT_RATE_32KBPS            (3)

/* Define type for G.722 state structure */
typedef struct {
  int16          al[3];
  int16          bl[7];
  int16          detl;
  int16          dlt[7]; /* dlt[0]=dlt */
  int16          nbl;
  int16          plt[3]; /* plt[0]=plt */
  int16          rlt[3];
  int16          ah[3];
  int16          bh[7];
  int16          deth;
  int16          dh[7]; /* dh[0]=dh */
  int16          ph[3]; /* ph[0]=ph */
  int16          rh[3];
  int16          sl;
  int16          spl;
  int16          szl;
  int16          nbh;
  int16          sh;
  int16          sph;
  int16          szh;
  int16          init_qmf_tx;
  int16          qmf_tx_delayx[24];
  int16          init_qmf_rx;
  int16          qmf_rx_delayx[24];
  vint           mode;
} G722_Obj;

typedef G722_Obj G722_EncObj;
typedef G722_Obj G722_DecObj;

/* 
 * Function Prototypes
 */
void g722_encodeInit(
    G722_EncObj *encObj_ptr);

void g722_decodeInit(
    G722_DecObj *decObj_ptr);

vint g722_encode(
    G722_EncObj *encObj_ptr,
    vint        *speechIn_ptr,
    uint8       *packetOut_ptr);

vint g722_decode(
    G722_DecObj *decObj_ptr,
    uint8       *packetIn_ptr,
    vint        *speechOut_ptr);

#endif /* _G722_H_ */

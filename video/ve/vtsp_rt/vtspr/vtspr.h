/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30131 $ $Date: 2014-12-01 14:45:03 +0800 (Mon, 01 Dec 2014) $
 */

#ifndef _VTSPR_H_
#define _VTSPR_H_

#include "osal.h"
#include "vtsp.h"
#include "../vtsp/vtsp_private/_vtsp_private.h"

#include "tic.h"
#include "vhw.h"

#include "comm.h"
#ifndef VTSP_ENABLE_MP_LITE
#include "tone.h"
#endif
#ifdef VTSP_ENABLE_AEC
#include "aec.h"
#endif
#ifndef VTSP_ENABLE_MP_LITE
#include "bnd.h"
#endif
#ifdef VTSP_ENABLE_ECSR
#include "ecsr.h"
#include "nlp.h"
#endif
#ifdef VTSP_ENABLE_NFE
#include "nfe.h"
#endif
#ifndef VTSP_ENABLE_MP_LITE
#include "nse.h"
#endif
#ifdef VTSP_ENABLE_DTMF
#include "dtmf.h"
#endif
#if defined(VTSP_ENABLE_STREAM_16K) || defined(VTSP_ENABLE_AUDIO_SRC)
#ifndef VTSP_ENABLE_MP_LITE
#include "uds.h"
#endif
#endif

#ifdef VTSP_ENABLE_FMTD
#include "fmtd.h"
#endif
#ifndef VTSP_ENABLE_MP_LITE
#include "plc.h"
#endif
#include "jb.h"
#ifdef VTSP_ENABLE_T38
#include "fr38v3.h"
#endif
#ifdef VTSP_ENABLE_UTD
#include "utd.h"
#endif
#ifdef VTSP_ENABLE_CIDR
#include "cas.h"
#include "fskr.h"
#endif
#ifndef VTSP_ENABLE_MP_LITE
#include "dcrm.h"
#endif
#ifdef VTSP_ENABLE_TONE_QUAD
#include "genf.h"
#endif

#ifdef VTSP_ENABLE_CIDS
#include "../cid/cids.h"
#include "../cid/cidcws.h"
#endif

#ifdef VTSP_ENABLE_DTMFR
#include "../dr/vtsp_rt_dr.h"
#endif

#if (defined(VTSP_ENABLE_G729) && !defined(VTSP_ENABLE_G729_ACCELERATOR))
#include "g729ab.h"
#endif

#ifdef VTSP_ENABLE_G722
#include "g722.h"
#endif

#if (defined(VTSP_ENABLE_G726) && !defined(VTSP_ENABLE_G726_ACCELERATOR))
#include "g726.h"
#endif

#ifdef VTSP_ENABLE_ILBC
#include "ilbc.h"
#endif

#ifdef VTSP_ENABLE_G723
#include "g723a.h"
#endif

#ifdef VTSP_ENABLE_G722
#include "g722.h"
#endif

#ifdef VTSP_ENABLE_G722P1
#include "g722p1.h"
#endif

#ifdef VTSP_ENABLE_SILK
#include "silk.h"
#endif

#ifdef VTSP_ENABLE_GAMRNB
#include "gamrnb.h"
#endif

#ifdef VTSP_ENABLE_GAMRWB
#include "gamrwb.h"
#endif

#if (defined(VTSP_ENABLE_G711P1) && !defined(VTSP_ENABLE_G711P1_ACCELERATOR))
#include "g711p1.h"
#endif

#include "vtspr_const.h"
#include "vtspr_struct.h"

#include "_vtspr_rtp.h"
#include "_vtspr_rtcp.h"
#include "_vtspr_net.h"
#include "_vtspr_stun.h"

/* Public Externs 
 */
extern VTSPR_Obj      VTSPR_obj;

extern char const D2_Release_VTSP_RT[];

/* Public Functions for VTSP Management
 * --------
 */

vint VTSPR_postInit(
    void);

void VTSPR_shutdown(
    void);

void VTSPR_task(
    VTSPR_Obj *vtpsr_ptr);
    
vint _VTSPR_samples10msToUdsMode(
    vint nSamples10ms);

#endif


/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28422 $ $Date: 2014-08-22 11:55:05 +0800 (Fri, 22 Aug 2014) $
 *
 */

/*
 * This is the ipc interface for recv VTSP->VTSPR
 */
#include "vtspr.h"
#include "_vtspr_private.h"

/*
 * ======== VTSPR_recvAllCmd() ========
 *
 * Get each cmd in VTSP->VTSPR cmd queue
 * and run each cmd, until queue is empty
 */
void _VTSPR_recvAllCmd(
    VTSPR_Obj     *vtspr_ptr,
    VTSPR_Queues  *q_ptr,
    VTSPR_DSP     *dsp_ptr) 
{
    vint            infc;
    VTSPR_ChanObj  *chan_ptr;
#ifdef VTSP_ENABLE_BENCHMARK
    _VTSPR_benchmarkStart(vtspr_ptr, _VTSPR_BENCHMARK_CMD, 1);
#endif
    /* 
     * Drain the callerId-Send queue for each FXS interface
     */
#ifdef VTSP_ENABLE_CIDS

    _VTSPR_FOR_ALL_FXS(infc) {
        chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);

        if ((VTSPR_ALG_CHAN_CIDS & chan_ptr->algChannelState) ||
                (VTSPR_ALG_CHAN_CIDCWS & chan_ptr->algChannelState)) { 
            /* Do not process cidQ on infc while CIDS or CIDCWS is running
             * on the infc.
             */
            continue;
        }
        while (OSAL_msgQRecv(q_ptr->cidQ[infc], 
                &chan_ptr->cidsData, sizeof(_VTSP_CIDData),
                OSAL_NO_WAIT, NULL) >= 0) { 
            /* drain queue */
        }
    }
    /* 
     * Drain the callerId-Send queue for each AUDIO interface
     */
    _VTSPR_FOR_ALL_AUDIO(infc) {
        chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);

        while (OSAL_msgQRecv(q_ptr->cidQ[infc], 
                &chan_ptr->cidsData, sizeof(_VTSP_CIDData),
                OSAL_NO_WAIT, NULL) >= 0) { 
            /* drain queue */
        }
    }
#endif
    /*
     * Clear all DTMF relay events
     * Bug 2976. drEventObj.event is trigger from DTMF detection event,
     * or from VTSP command.
     */
#ifdef VTSP_ENABLE_DTMFR
    _VTSPR_FOR_ALL_INFC(infc) {
        chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
        chan_ptr->drEventObj.event = 0;
    }
#endif

    /* Drain the command queue
     * in non-block
     */
    while (OSAL_msgQRecv(q_ptr->cmdQ, &q_ptr->cmdMsg,
            _VTSP_Q_CMD_MSG_SZ, OSAL_NO_WAIT, NULL) >= 0) { 
        /* Process the command
         */
        _VTSPR_runDnCmd(vtspr_ptr, q_ptr, dsp_ptr, &q_ptr->cmdMsg);
    }

#ifdef VTSPR_ENABLE_AUTOSTART
    if (1 == vtspr_ptr->autoStartCmd) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        _VTSPR_runDnCmd(vtspr_ptr, q_ptr, dsp_ptr, 0);
    }
#endif
#ifdef VTSP_ENABLE_BENCHMARK
    _VTSPR_benchmarkStop(vtspr_ptr, _VTSPR_BENCHMARK_CMD, 1);
#endif
}


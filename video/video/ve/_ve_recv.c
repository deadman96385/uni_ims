/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 9111 $ $Date: 2009-03-11 20:59:56 -0400 (Wed, 11 Mar 2009) $
 *
 */

/*
 * This is the ipc interface for recv VTSP->VE
 */
#include "_ve_private.h"

/*
 * ======== VE_recvAllCmd() ========
 *
 * Get each cmd in VTSP->VE cmd queue
 * and run each cmd, until queue is empty
 */
void _VE_recvAllCmd(
    _VE_Obj     *ve_ptr,
    _VE_Queues  *q_ptr,
    _VE_Dsp     *dsp_ptr)
{
    vint            size;
    /* 
     * Drain the callerId-Send queue for each FXS interface
     */

    /* Drain the command queue
     * in non-block
     */
    while ((size = OSAL_msgQRecv(q_ptr->cmdQ, 
                    &q_ptr->cmdMsg, 
                    _VTSP_Q_CMD_MSG_SZ, OSAL_NO_WAIT, NULL)) >= 0) {
        /* Process the command
         */
        _VE_runDnCmd(ve_ptr, q_ptr, dsp_ptr, &q_ptr->cmdMsg);
    }
}


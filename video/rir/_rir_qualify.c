/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 988 $ $Date: 2006-11-02 15:47:08 -0800 (Thu, 02 Nov 2006) $
 */

#include <osal.h>
#include "_rir.h"
#include "_rir_log.h"

/*
 * ======== _RIR_qualifyEvaluateInterfacesForBest() ========
 * 
 * Find the interface thats best in case a call need to be handed off.
 * 
 * Returns:
 * 
 */
void _RIR_qualifyEvaluateInterfacesForBest(
    _RIR_Obj *obj_ptr)
{
    _RIR_Interface *cur_ptr = NULL;
    
    /*
     * Start sorting interfaces.
     * Already sorted by prio.
     */    
    for (cur_ptr = obj_ptr->infc_ptr; cur_ptr; ) {

        /*
         * If not up, then not best.
         */
        if (!cur_ptr->up) {
        }
        /*
         * If invalid interface, then not best
         */
        else if (RIR_INTERFACE_TYPE_INVALID == cur_ptr->type) {
        }
        /*
         * If IPv4 not available for IP interface, then its not best
         */
        else if ((RIR_INTERFACE_TYPE_CMRS != cur_ptr->type) && 
            (OSAL_netIsAddrZero(&cur_ptr->addr))) {
        }
        else if (obj_ptr->curInfc_ptr == cur_ptr) {
            /*
             * This is the current interface. Cannot handoff to it, so it
             * cannot be the next best interface.
             */
        }
        /*
         * This is the best interface, thats highest prio and is up and available.
         */
        else {
            break;
        }
        
        cur_ptr = cur_ptr->next_ptr;
    }
    
    /*
     * return.
     */
    if (obj_ptr->bestInfc_ptr != cur_ptr) {
        obj_ptr->bestInfc_ptr = cur_ptr;
        if (NULL != cur_ptr) {
            _RIR_logMsg("%s %d: Next Best interface is now %s\n",
                    __FILE__, __LINE__, cur_ptr->name);
        }
    }
}

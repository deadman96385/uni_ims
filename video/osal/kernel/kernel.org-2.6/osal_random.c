/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL  
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE  
 * APPLIES: "COPYRIGHT 2004-2006 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"  
 *  
 * $D2Tech$ $Rev: 988 $ $Date: 2006-11-02 15:47:08 -0800 (Thu, 02 Nov 2006) $
 * 
 */

#include <osal_random.h>

/*
 * ======== OSAL_randomGetOctets() ========
 *
 * Gets a random string.
 *
 * Returns
 *  OSAL_SUCCESS
 */
void OSAL_randomGetOctets(
    char *buf_ptr,
    int   size)
{
    get_random_bytes(buf_ptr, size);
}

EXPORT_SYMBOL(OSAL_randomGetOctets);

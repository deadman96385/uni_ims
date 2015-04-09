/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL  
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE  
 * APPLIES: "COPYRIGHT 2004-2006 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"  
 *  
 * $D2Tech$ $Rev: 5475 $ $Date: 2008-03-12 16:20:56 -0400 (Wed, 12 Mar 2008) $
 */

#include <osal_types.h>
#include <osal_platform.h>

/*
 * ======== OSAL_archCountInit() ========
 *
 * Initialize counter for OSAL use.
 *
 * XXX If this function is not necessary on a Platform, 
 * it should be implemented as an empty function.
 *
 */
        
OSAL_INLINE void OSAL_archCountInit(void)
{
    return;
}

/*
 * ======== OSAL_archCountGet() ========
 *
 * Returns an SOC core Count register
 *
 * Returns:     uint32 Count
 *
 */

OSAL_INLINE uint32 OSAL_archCountGet(void)
{
    return (0);
}

/*
 * ======== OSAL_archCountDelta() ========
 *
 * Returns the difference between two times for SOC Count register
 *
 * Returns:     uint32 delta
 *
 */

OSAL_INLINE uint32 OSAL_archCountDelta(
        uint32 start,
        uint32 stop)
{
    /*
     * A1100 platform implements count-down timer
     * So subtract stop from start.
     */
    return (stop-start);
}

/*
 * ======== OSAL_archCountCyclesToUsec(count)() ========
 *
 * Returns cycle count in terms of microseconds
 *
 * Returns:     uint32 Usecs
 *
 */

OSAL_INLINE uint32 OSAL_archCountCyclesToUsec(
        vint count)
{
    return (0);
}
/*
 * ======== OSAL_archCountCyclesToMHz(count)() ========
 *
 * Returns cycle count in terms of cycles per second
 *
 * Returns:     uint32 MHz
 *
 */

OSAL_INLINE uint32 OSAL_archCountCyclesToMHz(
    uint32 count)
{
    return (0);
}


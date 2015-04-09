/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL  
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE  
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"  
 *  
 * $D2Tech$ $Rev: 12352 $ $Date: 2010-06-22 11:24:28 -0700 (Tue, 22 Jun 2010) $
 */

#include <osal_types.h>
#include <osal_platform.h>

static vint init = 0;

#define OSAL_ARCH_CPU_COUNTS_PER_CYCLE (1)

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
    init = 1;
    
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
    vint count = 0;

    return (count);
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
 * ======== OSAL_archCountCyclesToKHz(count)() ========
 *
 * Returns cycle count in terms of cycles per second
 *
 * Returns:     uint32 KHz
 *
 */

OSAL_INLINE uint32 OSAL_archCountCyclesToKHz(
    uint32 count)
{
    uint32 cyclesPerSec;
    /* 
     * Computes the number of Kilo- cycles in 0.01 seconds
     */
    cyclesPerSec = (OSAL_ARCH_CPU_COUNTS_PER_CYCLE * count) / 10;

    return (cyclesPerSec);
}


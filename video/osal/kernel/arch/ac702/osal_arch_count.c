/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL  
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE  
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"  
 *  
 * $D2Tech$ $Rev: 29023 $ $Date: 2014-09-25 17:28:52 +0800 (Thu, 25 Sep 2014) $
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
   /* Enable Perf Mon Cntl Register */
    asm("MRC p15, 0, r0, c9, c12, 0");
    asm("ORR r0, r0, #1 << 0");

    asm("MCR p15, 0, r0, c9, c12, 0");
    asm("MRC p15, 0, r0, c9, c12, 1");

    asm("LDR r1, = 0x8000000F");
    asm("ORR r0, r0, r1");
    asm("MCR p15, 0, r0, c9, c12, 1");

    /* Set tick rate to 1 */
    asm("MRC p15, 0, r0, c9, c12, 0");
    asm("BIC r0, r0, #1 << 3");
    asm("MCR p15, 0, r0, c9, c12, 0");        

    /* Reset the Cycle Counter Register */
    asm("MRC p15, 0, r0, c9, c12, 0");
    asm("ORR r0, r0, #1 << 2");
    asm("MCR p15, 0, r0, c9, c12, 0");        

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
#ifdef OSAL_ARCH_ENABLE_COUNT
    if (!init) {
        OSAL_archCountInit();
    }

    /* Read the cycle counter register */
    asm("MRC p15, 0, %0, c9, c13, 0" : "=r" (count));
#endif
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
#ifndef OSAL_KERNEL_EMULATION
EXPORT_SYMBOL(OSAL_archCountInit);
EXPORT_SYMBOL(OSAL_archCountGet);
EXPORT_SYMBOL(OSAL_archCountDelta);
EXPORT_SYMBOL(OSAL_archCountCyclesToKHz);
#endif

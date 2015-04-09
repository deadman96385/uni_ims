/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 988 $ $Date: 2006-11-02 15:47:08 -0800 (Thu, 02 Nov 2006) $
 *
 */

#include <osal_random.h>

#include "osa_interface.h"

static uint32 seed = 1;         /* Start the seed at 1 */

static uint16 _OSAL_random(void)
{
    seed = seed * 1103413245 + 12345;
    return (uint16) (seed/65536) % 32768;
}

/*
 * ======== OSAL_randomReseed() ========
 *
 * Seeds random number generator.
 *
 * Returns
 */
void OSAL_randomReseed(
    void)
{
    _OSAL_random();
}

/*
 * ======== OSAL_randomGetOctets() ========
 *
 * Gets a random string.
 * Note: this routine is NOT re-entrant.  If multiple threads call this
 * routine at the same time they may get the same value.
 *
 * If you need a random generator that can be called between
 * different threads to produce different values then see
 * OSAL_randomGetOctetsReEntrant().
 *
 * Returns
 *    Nothing.
 */
void OSAL_randomGetOctets(
    char *buf_ptr,
    int   size)
{
    uint32 sec, v;
    uint16 msec;
    uint16 _r;
    while(size > 0)
    {
        _r = _OSAL_random();
        osa_get_time ( &sec, &msec );
        v = (sec + msec + _r)%26;
        *buf_ptr++ = (char)('a' + v);
        size--;
    }
}

/*
 * ======== OSAL_randomGetOctetsReEntrant() ========
 *
 * Gets a random string.
 * Note: This routine is re-entrant. Different threads
 * may call this routine with
 *
 * The seed_ptr argument is a pointer to an unsigned int that
 * is used to store state between calls. If this routine is
 * called with the same initial value for the integer pointed
 * to by seed_ptr, and that value is not modified between calls,
 * then the same pseudo-random sequence will result.
 *
 * Returns
 *    Nothing.
 */
void OSAL_randomGetOctetsReEntrant(
    uint32 *seed_ptr,
    char   *buf_ptr,
    int     size)
{
    OSAL_randomGetOctets(buf_ptr, size);
}

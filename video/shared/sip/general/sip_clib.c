/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 25287 $ $Date: 2014-03-24 16:37:42 +0800 (Mon, 24 Mar 2014) $
 */

#include "sip_clib.h"
#include <osal_types.h>
#include <osal_sem.h>
#include <osal_random.h>
#include "sip_sip_const.h"

static OSAL_SemId _SIP_cLibMutex = NULL;
static uint32     _SIP_cLibSeed = 0;

/* 
 ******************************************************************************
 * ================SIP_initCLib()===================
 *
 * This function used to initilize the resources for getting a random string.
 *
 * seed = A number used to initialize a pseudo-random number generator.
 *
 * RETURNS:
 *        Nothing.
 *
 ******************************************************************************
 */
void SIP_initCLib(uint32 seed)
{
    if (NULL == _SIP_cLibMutex) {
         _SIP_cLibMutex = OSAL_semMutexCreate();
    }

     _SIP_cLibSeed = seed;
}

/* 
 ******************************************************************************
 * ================SIP_destroyCLib()===================
 *
 * This function used to destroy the resources for getting a random string.
 *
 * RETURNS:
 *        Nothing.
 *
 ******************************************************************************
 */
void SIP_destroyCLib(void)
{
    if (NULL != _SIP_cLibMutex) {
        OSAL_semDelete(_SIP_cLibMutex);
        _SIP_cLibMutex = NULL;
    }
}

/* 
 ******************************************************************************
 * ================SIP_randInt()===================
 *
 * This function returns a pseudo-random integer in the range l to h exclusive.
 *
 * l = The lower bound of random range.
 *
 * h = The upper bound of random range.
 *
 * RETURNS:
 *        A pseudo-random integer.
 *
 ******************************************************************************
 */
uint32 SIP_randInt(uint32 l, uint32 h)
{
    uint32 x;
    uint32 mod;

    if (l < h) {
        OSAL_semAcquire(_SIP_cLibMutex, OSAL_WAIT_FOREVER);
        OSAL_randomGetOctetsReEntrant(&_SIP_cLibSeed, (void *)&x, sizeof(x));
        OSAL_semGive(_SIP_cLibMutex);
        mod = (h - l);
        x %= mod;
        x += l;
        return x;
    }
    else {
        return l;
    }
}

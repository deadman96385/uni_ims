/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28728 $ $Date: 2014-09-05 17:01:31 +0800 (Fri, 05 Sep 2014) $
 */

#ifndef __MC_MAIN_H__
#define __MC_MAIN_H__

#ifdef __cplusplus
extern "C" {
#endif

int MC_init(void *cfg_ptr);
void MC_shutdown(void);

vint MC_start(void);

void MC_destroy(void);

vint MC_allocate(void);

vint MC_start(void);

void MC_destroy(void);

#ifdef __cplusplus
}
#endif

#endif

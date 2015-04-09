/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28728 $ $Date: 2014-09-05 17:01:31 +0800 (Fri, 05 Sep 2014) $
 */

#ifndef __CSM_MAIN_H__
#define __CSM_MAIN_H__

#ifdef __cplusplus
extern "C" {
#endif

int CSM_init(void *cfg_ptr);
void CSM_shutdown(void);

vint CSM_allocate(void);

vint CSM_start(
     void *obj_ptr);

vint CSM_destroy(
     void *obj_ptr);

vint CSM_vport4gInit(void);
void CSM_vport4gShutdown(void);


#ifdef __cplusplus
}
#endif

#endif

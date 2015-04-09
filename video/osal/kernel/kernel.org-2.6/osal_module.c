/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL  
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE  
 * APPLIES: "COPYRIGHT 2004-2008 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"  
 *  
 * $D2Tech$ $Rev: 7834 $ $Date: 2008-10-15 20:03:59 -0400 (Wed, 15 Oct 2008) $
 *
 */

#include <osal_log.h>

/* 
 * Module License and Information
 * Not a GPL Module
 */
MODULE_AUTHOR("D2 Technologies, Inc.");
MODULE_LICENSE("D2 Technologies, Inc. Proprietary Software");
MODULE_DESCRIPTION("COPYRIGHT 2008 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
      "OSAL module");

/*
 * ======== init_module() ========
 *
 * Purpose:  Invoked when the module is dynamically loaded via insmod
 *
 * Params:   void
 * 
 * Returns:  int - negative indicates init failure
 *
 * Notes:    creates proc and devices entries.
 */
int init_module(void)
{
    OSAL_logMsg("%s:%d OSAL module_init OK\n", __FILE__, __LINE__, 0, 0);
    return (0);
}

/*
 * ======== cleanup_module() ========
 *
 * Purpose:  Invoked when the module is dynamically unloaded via rmmod
 *
 * Params:   void
 * 
 * Returns:  void
 *
 * Notes:    cleans up proc entries and devices.
 */
void cleanup_module(void)
{
    OSAL_logMsg("%s: %d OSAL module_exit OK\n", __FILE__, __LINE__, 0, 0);
    return;
}


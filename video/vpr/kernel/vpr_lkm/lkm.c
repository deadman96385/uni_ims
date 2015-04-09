/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 */

/*
 * Voice driver Linux Kernel Module (LKM) interface
 */

#include <osal.h>
#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/init.h>

extern OSAL_Status VPR_kernInit(void);
extern void VPR_kernShutdown(void);

/*
 * Module License and Information
 * Not a GPL Module
 */
MODULE_AUTHOR("D2 Technologies, Inc.");
MODULE_LICENSE("D2 Technologies, Inc. Proprietary Software");
MODULE_DESCRIPTION("COPYRIGHT 2008 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
      "VTSPR module");

/*
 * This function is called by when this driver is invoked with
 * insmod command. This function can be used to initialize data structures.
 * Another important use of this function is to register the device.
 */
vint LKM_initModule(
    void)
{
    /*
     * Start the VPR KERNEL
     */
    if (OSAL_SUCCESS != VPR_kernInit()) {
        OSAL_logMsg("%s:%d VPR kernel init FAILED.\n", __FUNCTION__, __LINE__);
        return (-1);
    }

    return (0);
}


/*
 * This function is called when this driver is removed with the rmmod command.
 * This function can be used to free data structures allocated by the
 * init_module function.  Another important use of this function is to
 * unregister the device, Kill any threads that may have been created.
 */
void LKM_cleanupModule(
    void)
{
    VPR_kernShutdown();
}

module_init(LKM_initModule);
module_exit(LKM_cleanupModule);

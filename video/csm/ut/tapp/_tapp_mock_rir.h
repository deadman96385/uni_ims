/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 20785 $ $Date: 2013-05-23 07:23:39 +0800 (Thu, 23 May 2013) $
 *
 */
 /* TAPP mock RIR */

int RIR_init(
    char *xmlDoc_ptr,
    int   xmlDocLen);

int RIR_shutdown(void);
   
vint TAPP_mockRirShutdown(
    TAPP_GlobalObj *global_ptr);    

vint TAPP_mockRirInit(
    TAPP_GlobalObj *global_ptr);

vint TAPP_mockRirIssueCsm(
    TAPP_GlobalObj *global_ptr,
    TAPP_Action    *action_ptr);

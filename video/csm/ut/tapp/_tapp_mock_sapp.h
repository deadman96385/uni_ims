/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28271 $ $Date: 2014-08-18 15:24:33 +0800 (Mon, 18 Aug 2014) $
 *
 */
 
 /* TAPP mock SAPP */
int SAPP_init(
    void *cfg_ptr,
    char *hostname_ptr);

vint TAPP_mockSappInit(
    void           *cfg_ptr,
    TAPP_GlobalObj *global_ptr);

vint TAPP_mockSappShutdown(
    TAPP_GlobalObj *global_ptr);

vint TAPP_mockSappIssueIsip(
    TAPP_GlobalObj *global_ptr,
    ISIP_Message   *isip_ptr);

vint TAPP_mockSappValidateIsip(
    TAPP_GlobalObj *global_ptr,
    TAPP_Action    *action_ptr);

vint _TAPP_mockSappGetServiceIdByIndex(
    TAPP_GlobalObj *global_ptr,
    vint index);

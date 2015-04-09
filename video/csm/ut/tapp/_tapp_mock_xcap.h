/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 *
 */
 
/* TAPP mock Xcap */
#ifndef __TAPP_MOCK_XCAP_H_
#define __TAPP_MOCK_XCAP_H_

vint TAPP_mockXcapInit(
    TAPP_GlobalObj *global_ptr,
    XCAP_Obj *obj_ptr);

vint TAPP_mockXcapShutdown(
    TAPP_GlobalObj *global_ptr,
    XCAP_Obj *obj_ptr);

vint TAPP_mockXcapIssueEvt(
    TAPP_GlobalObj *global_ptr,
    TAPP_mockXcapEvt *mockXcapEvt_ptr);

vint TAPP_mockXcapValidateCmd(
    TAPP_GlobalObj *global_ptr,
    TAPP_Action    *action_ptr);
#endif

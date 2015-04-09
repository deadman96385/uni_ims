/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 19840 $ $Date: 2013-02-14 15:06:18 -0800 (Thu, 14 Feb 2013) $
 *
 */

#define TAPP_MOCK_VPMD_WRITE_Q_NAME "mock-vmpd-writeQ"
#define TAPP_MOCK_VPMD_READ_Q_NAME  "mock-vmpd-readQ"
#define TAPP_MOCK_VPMD_WEVT_Q_NAME  "mock-vmpd-W-EvtQ"
#define TAPP_MOCK_VPMD_REVT_Q_NAME  "mock-vmpd-R-EvtQ"
 /* TAPP mock SAPP */
OSAL_Status VPMD_init(
    );

vint TAPP_mockVpmdIssueIsiRpc(
    TAPP_GlobalObj *global_ptr,
    TAPP_Action    *action_ptr);

vint TAPP_mockVpmdValidateIsiRpc (
    TAPP_GlobalObj *global_ptr,
    TAPP_Action    *action_ptr);

OSAL_Status VPMD_readIsiRpc(
    void *buf_ptr,
    vint  size,
    vint  timeout);

OSAL_Status VPMD_writeIsiRpc(
    void *buf_ptr,
    vint  size);

vint TAPP_mockVpmdValidateIsiGetEvt (
    TAPP_GlobalObj *global_ptr,
    TAPP_Action    *action_ptr);

vint TAPP_mockVpmdCleanIsiEvt (
    TAPP_GlobalObj *global_ptr,
    TAPP_Action    *action_ptr);

int VPMD_destroy();

/* VPMD interfaces for Video streams */
OSAL_Status VPMD_writeVideoStream(
    void *buf_ptr,
    vint  size);

OSAL_Status VPMD_readVideoStream(
    void *buf_ptr,
    vint  size,
    vint  timeout);

/* VPMD interfaces for Voice streams */
OSAL_Status VPMD_writeVoiceStream(
    void *buf_ptr,
    vint  size);

OSAL_Status VPMD_readVoiceStream(
    void *buf_ptr,
    vint  size,
    vint  timeout);

OSAL_MsgQId VPMD_getVoiceStreamReadQueue(
    void);

OSAL_MsgQId VPMD_getVideoCmdEvtReadQueue(
    void);

OSAL_MsgQId VPMD_getVideoStreamReadQueue(
    void);

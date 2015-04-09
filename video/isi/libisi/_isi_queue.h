/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 27616 $ $Date: 2014-07-18 11:02:53 +0800 (Fri, 18 Jul 2014) $
 *
 */


#define ISIQ_PROTO_MAX_MSG_SZ  (sizeof(ISIP_Message))
#define ISIQ_APP_MAX_MSG_SZ    (sizeof(ISI_EventMessage))
#define ISIQ_MAX_DEPTH         (16)

typedef struct {
    vint           protocol;
    OSAL_Boolean   isValid;
    char           protocolIpc[ISI_ADDRESS_STRING_SZ + 1];
    char           mediaIpc[ISI_ADDRESS_STRING_SZ + 1];
    char           streamIpc[ISI_ADDRESS_STRING_SZ + 1];
    OSAL_MsgQId    protocolQId;
    OSAL_MsgQId    mediaQId;
    OSAL_MsgQId    streamQId;
} ISIQ_Ipc;

typedef struct {
    ISI_Id     serviceId;
    ISI_Id     id;
    ISI_IdType idType;
    ISI_Event  event;
    char       eventDesc[ISI_EVENT_DESC_STRING_SZ + 1];
} ISI_EventMessage;

ISI_Return ISIQ_init(
    void);

void ISIQ_destroyProto(
    void);

void ISIQ_destroyApp(
    void);

void ISIQ_destroyGroupQ(
    void);

ISI_Return ISIQ_writeAppQueue(
    char   *data_ptr,
    vint    length);

ISI_Return ISIQ_writeProtocolQueue(
    ISIP_Message  *msg_ptr);

ISI_Return ISIQ_getEvent(
    char  *data_ptr,
    vint  *len_ptr,
    vint  *isApplication_ptr,
    vint   timeout);

ISI_Return ISIQ_addProtocol(
    vint  protocol,
    char *protocolIpc_ptr,
    char *mediaIpc_ptr,
    char *streamIpc_ptr);

ISIQ_Ipc* ISIQ_getProtocol(
    vint  protocol);

ISI_Return ISIQ_clearProtocol(
    vint  protocol);

void ISIQ_wakeAppQueue(void);


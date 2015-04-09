/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29904 $ $Date: 2014-11-19 02:54:52 +0800 (Wed, 19 Nov 2014) $
 *
 */

/** \file
 * \brief Top level application programming interface.
 *  
 * API in this file is used to exchange XCAP data with an XCAP server.
 * Client sends commands to the XCAP task, then receives events from the XCAP
 * task.
 * All data is pass through i.e. no attempt is made to parse data.
 *
 * Application uses function #XCAP_sendCmd to send a command to the XCAP task,
 * the XCAP task performs XCAP transaction described in that command, and
 * generates an event to the application based on result of transaction of that
 * command. Application calls function #XCAP_getEvt to read the event.
 * After reading, application uses parser helper functions to parse the event
 * contents.
 * A typical sequence of calls for application is:\n
 *  #XCAP_init\n
 *  #XCAP_helperMakeUri\n
 *  #XCAP_sendCmd\n
 *  #XCAP_getEvt\n
 *  #XCAP_httpParseHeader\n
 *  #XCAP_xmlParulesParseDocument\n
 *  #XCAP_disposeEvt\n
 */

#ifndef _XCAP_H_
#define _XCAP_H_

#include <osal.h>

#ifdef INCLUDE_GBA
#include "gba.h"
#include "gaa.h"
#endif

/** \enum XCAP_Operation
 * \brief Enumeration for XCAP operation to be performed.
 *
 * Used in XCAP_Cmd.op element for performing operations on server.
 */
typedef enum {
    XCAP_OPERATION_NONE = 0,       
    XCAP_OPERATION_DELETE,         /**< Delete something */
    XCAP_OPERATION_CREATE_REPLACE, /**< Create or replace something */
    XCAP_OPERATION_FETCH,          /**< Fetch something from server */
    XCAP_OPERATION_INVALID
} XCAP_Operation;

/** \enum XCAP_OperationType
 * \brief Enumeration for XCAP operation type.
 *
 * Used in XCAP_Cmd.opType for type of operation on server. Used in conjuction
 * with #XCAP_Operation type.
 * Note: URI must match and GET(PUT(x))==x as in RFC 4825 must be satisfied.
 */
typedef enum {
    XCAP_OPERATION_TYPE_NONE = 0,
    XCAP_OPERATION_TYPE_DOCUMENT,  /**< Document operation type */
    XCAP_OPERATION_TYPE_ELEMENT,   /**< Element operation type */
    XCAP_OPERATION_TYPE_ATTRIBUTE, /**< Attribute operation type */
    XCAP_OPERATION_TYPE_INVALID
} XCAP_OperationType;

/** \enum XCAP_Condition
 * \brief Enumeration for XCAP etag conditional operations.
 *
 * Used in XCAP_Cmd.cond element for performing conditional operations on
 * server. This is required for ETag If-Match and If-None-Match conditional
 * updates.
 */
typedef enum {
    XCAP_CONDITION_NONE = 0,
    XCAP_CONDITION_IF_MATCH,        /**< If-Match */
    XCAP_CONDITION_IF_NONE_MATCH,   /**< If-None-Match */
    XCAP_CONDITION_INVALID
} XCAP_Condition;

/** \enum XCAP_EvtErr
 * \brief Event error.
 *
 * This is received in event XCAP_Evt.error to indicate a particular failure.
 */
typedef enum {
    XCAP_EVT_ERR_NONE  = 0, /**< OK, no error */
    XCAP_EVT_ERR_NOMEM,     /**< Memory allocation error */
    XCAP_EVT_ERR_NET,       /**< Network timeout */
    XCAP_EVT_ERR_HTTP,      /**< Error geting http resources */
    XCAP_EVT_ERR_LAST
} XCAP_EvtErr;

/** \struct XCAP_Cmd
 * \brief XCAP command structure.
 * 
 * Application fills this structure and sends to XCAP task as a command using
 * call to function #XCAP_sendCmd.\n
 * Note: We pass pointers in queue for zero copy.
 */
typedef struct {
    XCAP_Operation      op;         /**< Set an op */
    XCAP_OperationType  opType;     /**< Set an op type */
    XCAP_Condition      cond;       /**< Set a condition */
    char               *uri_ptr;    /**< Set target URI */
    char               *username_ptr; /**< Set username for authentcation */
    char               *password_ptr; /**< Set password for authentcation */
    char               *userAgent_ptr; /**< Set the user agent value */
    char               *x3gpp_ptr;    /**< Set ID for the 3GPP network */
    char               *auid_ptr;   /**< Set to AUID as in above uri_ptr */
    char               *etag_ptr;   /**< ETag if .cond is set */
    char               *src_ptr;    /**< Document buffer pointer */
    int                 srcSz;      /**< Document buffer size */
    OSAL_NetAddress    *infcAddr_ptr; /* the supsrv radio to do the xcap transactin */
} XCAP_Cmd;

/** \struct XCAP_Evt
 * \brief XCAP event structure.
 * 
 * XCAP task fills this structure and sends to application as an event.
 * Application gets this event using call to function #XCAP_getEvt.
 * Application reads from event then disposes the event using call to
 * function #XCAP_disposeEvt.\n
 * Note: 
 *  - We pass pointers in queue for zero copy.
 *  - If application does not call function #XCAP_disposeEvt after event is
 *    processed and useless, application will leak memory.
 */
typedef struct {
    XCAP_EvtErr   error;    /**< Error code */
    char         *hdr_ptr;  /**< Pointer to HTTP header, unparsed */
    char         *body_ptr; /**< Pointer to HTTP body, unparsed */
} XCAP_Evt;

/** \struct XCAP_Obj
 * \brief XCAP internal structure.
 * 
 * Used in XCAP API here. This has no application data in it.
 */
typedef struct {
    OSAL_TaskId tid;            /**< OSAL task ID for XCAP task */
    OSAL_MsgQId cmdq;           /**< Command queue ID */
    OSAL_MsgQId evtq;           /**< Event queue ID */
    int         exit;           /**< Condition for exitting the XCAP task */
    int         timeoutsec;     /**< Time out for every XCAP transaction */
#ifdef INCLUDE_GBA
    GAA_NafSession   *nafSession_ptr;
#endif
} XCAP_Obj;

/*
 * Function prototypes
 */

/** \fn int XCAP_init(
 *          XCAP_Obj *obj_ptr,
 *          int       timeoutsec)
 * \brief Init function.
 *
 * This function must be called first before calling any other functions.
 * Note: Application cannot init more than one XCAP object.
 * @param obj_ptr Application must pass and retain it for future calls to API
 * @param timeoutsec Time out in seconds for each XCAP transaction. Set it high
 * as transactions can take a while to complete.
 * @return 0: failed, 1: passed
 */
int XCAP_init(
    XCAP_Obj *obj_ptr,  
    int       timeoutsec);

/** \fn int XCAP_shutdown(
 *          XCAP_Obj *obj_ptr)
 * \brief Shutdown function.
 *
 * This function must be called last before exitting an application to free
 * resources.
 * @param obj_ptr XCAP object initialized by #XCAP_init
 * @return 0: failed, 1: passed
 */
int XCAP_shutdown(
    XCAP_Obj *obj_ptr);

/** \fn int XCAP_sendCmd(
 *          XCAP_Obj *obj_ptr,
 *          XCAP_Cmd *cmd_ptr)
 * \brief Send a command to XCAP task.
 *
 * This function is called to send a command to the XCAP task.
 * Each command starts an XCAP transaction with a server.
 * @param obj_ptr XCAP object initialized by #XCAP_init
 * @param cmd_ptr A pointer to an XCAP_Cmd structure populated as,\n
 * XCAP_Cmd.op: operation\n
 * XCAP_Cmd.opType: type of operation\n
 * XCAP_Cmd.cond: condition (must also set etag_ptr to an ETag) - optional\n
 * XCAP_Cmd.uri_ptr: pointer to the HTTP URI, use function #XCAP_helperMakeUri\n
 * XCAP_Cmd.auid_ptr: pointer to the AUID, must match AUID in uri_ptr\n
 * XCAP_Cmd.etag_ptr: pointer to ETag if XCAP_Cmd.cond is set\n
 * XCAP_Cmd.src_ptr: pointer to document buffer - required for
 *  #XCAP_OPERATION_CREATE_REPLACE, optional for other ops\n
 * XCAP_Cmd.srcSz: size of the document pointed by XCAP_Cmd.src_ptr
 * @return 0: failed, 1: passed
 */
int XCAP_sendCmd(
     XCAP_Obj *obj_ptr,
     XCAP_Cmd *cmd_ptr);

/** \fn int XCAP_getEvt(
 *          XCAP_Obj *obj_ptr,
 *          XCAP_Evt *evt_ptr,
 *          int       msTimeout)
 * \brief Get an event from XCAP task.
 *
 * This function is called to get an event from an XCAP task.
 * Each command sent by function #XCAP_sendCmd starts an XCAP transaction 
 * with a server, which generates exactly one event.
 * Application reads from event then disposes the event using call to
 * function #XCAP_disposeEvt.
 * @param obj_ptr XCAP object initialized by #XCAP_init
 * @param evt_ptr Location where event will be written on return
 * @param msTimeout Wait for the event in millieconds, -1 forever
 * @return 0: failed, 1: passed
 */
int XCAP_getEvt(
     XCAP_Obj *obj_ptr,
     XCAP_Evt *evt_ptr,
     int       msTimeout);

/** \fn int XCAP_disposeEvt(
 *          XCAP_Evt *evt_ptr)
 * \brief Dispose an event.
 * 
 * Dispose an event we got from function call #XCAP_getEvt.
 * Note: dispose event only if function returns success and, 
 * XCAP_Evt.error == #XCAP_EVT_ERR_NONE.
 * @param evt_ptr As passed in #XCAP_getEvt call.
 * @return 0: failed, 1: passed
 */
int XCAP_disposeEvt(
     XCAP_Evt *evt_ptr);

vint XCAP_allocate(
     XCAP_Obj *obj_ptr,
     int       timeoutsec);

vint XCAP_start(
     XCAP_Obj *obj_ptr);

vint XCAP_destroy(
    XCAP_Obj *obj_ptr);

vint _XCAP_deallocate(
     XCAP_Obj *obj_ptr);

vint _XCAP_stop(
     XCAP_Obj *obj_ptr);
#endif

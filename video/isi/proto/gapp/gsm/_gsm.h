/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 25157 $ $Date: 2014-03-17 00:59:26 +0800 (Mon, 17 Mar 2014) $
 */

#ifndef _GSM_H_
#define _GSM_H_

#include "_gsm_cfg.h"

/* Defines AT commands */
#define  _GSM_CMD_CDU                ("AT+CDU")
#define  _GSM_CMD_CDEFMP             ("AT+CDEFMP")
#define  _GSM_CMD_CCMMD              ("AT+CCMMD")

typedef int GSM_Fd;

typedef struct {
    OSAL_TaskId  id;
    uvint        stackSz;
    uvint        pri;
    void        *func_ptr;
    char         name[16];
    void        *arg_ptr;
} GSM_Task;

typedef struct {
    OSAL_SemId mutex;
    GSM_Id     id;
} GSM_IdMngr;

typedef enum {
    GSM_CMD_TYPE_CMD = 0,
    GSM_CMD_TYPE_TMR = 1,
} GSM_CmdType;

typedef enum {
    GSM_TMR_TYPE_NONE   = 0,
    GSM_TMR_TYPE_REPORT = 2,
} GSM_TmrType;

typedef struct {
    GSM_CmdType type;
    union {
        struct {
            char        cmd[GSM_BUFFER_SZ + 1];
            char        result[GSM_BUFFER_SZ + 1];
            vint        timeoutSecs;
            GSM_Id      id;
        } cmd;
        struct {
            GSM_TmrType type;
            vint        arg;
        } tmr;
        /* XXX Add any other command structures here */
    }u;
} GSM_Cmd;

typedef struct {
    GSM_Task       task;
    GSM_IdMngr     idMngr;
    OSAL_MsgQId    evtQId;
    GSM_Fd         cmdPipe[2];
    GSM_Fd         devFd;
    OSAL_SelectSet fdSet;
    GSM_Event      event;
    struct {
        OSAL_TmrId   tmrId;
        OSAL_Boolean timerOn;
        GSM_Cmd      command;
        char         result[GSM_BUFFER_SZ + 1];
        char         report[GSM_BUFFER_SZ + 1];
    } report;
    union {
        char    result[GSM_BUFFER_SZ + 1];
        GSM_Cmd command;
    } readScratchPad;
    char writeScratchPad[GSM_BUFFER_SZ + 1];
    OSAL_Boolean   extDialCmdEnabled;
} GSM_Obj;

vint GSM_snprintf(
    char       *buf_ptr,
    size_t      size,
    const char *format_ptr,
    ...);

GSM_Return GSM_getUniqueId(
    GSM_IdMngr *mngr_ptr, 
    GSM_Id     *id_ptr);

GSM_Return GSM_initTask(
    GSM_Obj *gsm_ptr);

GSM_Return GSM_destroyTask(
    GSM_Obj *gsm_ptr);

GSM_Return GSM_initAllResources(
    GSM_Obj *gsm_ptr);

GSM_Return GSM_destroyAllResources(
    GSM_Obj *gsm_ptr);

GSM_Return GSM_initDev(
    GSM_Obj    *gsm_ptr, 
    const char *configFile_ptr);

GSM_Return GSM_destroyDev(
    GSM_Obj *gsm_ptr);

GSM_Return GSM_writeApiCommand(
    GSM_Obj *gsm_ptr,
    GSM_Cmd *cmd_ptr);

GSM_Return GSM_readEvent(
    GSM_Obj   *gsm_ptr,
    GSM_Event *event_ptr,
    uint32     timeout);

#endif

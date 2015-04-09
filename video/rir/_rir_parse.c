/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 988 $ $Date: 2006-11-02 15:47:08 -0800 (Thu, 02 Nov 2006) $
 */

#include <settings.h>
#include <osal.h>
#include "_rir.h"
#include "_rir_log.h"
#include <arpa/inet.h>

/*
 IR_parseInputIR_parseInput() ========
 *
 * This function reads RIR XML config file named rir.xml.
 * Then it parses it and places it in RIR object for program needs. 
 * 
 * Returns:
 * -1 : Failed to parse
 *  0 : Parsed.
 * 
 */
int _RIR_parseInput(
    _RIR_Obj *obj_ptr,
    void     *cfg_ptr)
{
    char      *value_ptr;
    char      *name_ptr;
    int        tmp;
    int        profileFound;
    _RIR_Interface *cur_ptr;


    /*
     * Log file name.
     */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_RIR,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            NULL, NULL, SETTINGS_PARM_LOG_FILE))) {
        OSAL_strncpy(obj_ptr->logfile, value_ptr, sizeof(obj_ptr->logfile));
    }
    /*
     * Ping server name, dont include if ping test is not required.
     */
    obj_ptr->serverIp = 0;
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_RIR,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            NULL, NULL, SETTINGS_PARM_PING_SERVER))) {
        obj_ptr->serverIp = OSAL_netNtohl(inet_addr(value_ptr));
    }

    /*
     * Which protocols to notify in case of a command to change the IP address.
     */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_RIR,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            NULL, NULL, SETTINGS_PARM_NOTIFY))) {
        OSAL_strncpy(obj_ptr->protocols, value_ptr, sizeof(obj_ptr->protocols));
    }

    /*
     * WLU like operation where all interfaces are accepted and treated the same.
     */
    obj_ptr->all = 0;
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_RIR,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            NULL, NULL, SETTINGS_PARM_ACCEPT_ALL))) {
        if ('0' != value_ptr[0]) {
            obj_ptr->all = 1;
            return (0);
        }
    }
    /*
     * Reset delay.  
     * This is used to delay reset when RIR_warmInit is called during 
     * initialization.  This allows the netlink layer to settle on boot up.
     */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_RIR,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            NULL, NULL, SETTINGS_PARM_RESET_DELAY))) {
        obj_ptr->delayTimeMs = OSAL_atoi(value_ptr) * 1000;
    }

    /*
     * Find the Interface Profile
     */
    profileFound = 0;
    if (NULL != (value_ptr = SETTINGS_getAttrValue(SETTINGS_TYPE_RIR,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_PROFILE,
            NULL, NULL, SETTINGS_ATTR_NAME))) {
        if (0 == OSAL_strncasecmp(value_ptr, "default",
            sizeof(obj_ptr->profile))) {
            profileFound = 1;
        }
    }

    /* Return if we did not find a profile */
    if (0 == profileFound) {
        _RIR_logMsg("RIR: error default profile NOT FOUND");
        return (-1);
    }

    if (NULL == (name_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_RIR,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROFILE,
            SETTINGS_TAG_INTERFACE, NULL, SETTINGS_PARM_INTERFACE_NAME))) {
        _RIR_logMsg("RIR: error interface name NOT FOUND\n");
        return (-1);
    }
    else {
        OSAL_logMsg("interface name : %s\n", name_ptr);
    }

    if (NULL == (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_RIR,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROFILE,
            SETTINGS_TAG_INTERFACE, NULL, SETTINGS_PARM_INTERFACE_ENABLED))) {
        _RIR_logMsg("RIR: error interface enabled NOT FOUND\n");
        return (-1);
    }
    else {
        if ('1' == value_ptr[0]) {
            if (NULL == (cur_ptr = _RIR_findInterfaceorAdd(obj_ptr, name_ptr))) {
                _RIR_logMsg("RIR: error to add interface");
                return (-1);
            }
            if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_RIR,
                    SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROFILE,
                    SETTINGS_TAG_INTERFACE, NULL, SETTINGS_PARM_INTERFACE_TYPE))) {
                OSAL_strcpy(cur_ptr->typeName, value_ptr);
            }
            if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_RIR,
                    SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROFILE,
                    SETTINGS_TAG_INTERFACE, NULL, SETTINGS_PARM_INTERFACE_PRIORITY))) {
                tmp = atoi(value_ptr);
                if (tmp < 0) {
                    tmp = 0;
                }
                cur_ptr->prio = tmp;
            }

        }
        else {
            _RIR_logMsg("RIR: error interface is not enabled");
            return (-1);
        }
    }
    return (0);
}

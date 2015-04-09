/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 20467 $ $Date: 2013-04-23 06:55:24 +0800 (Tue, 23 Apr 2013) $
 */

#include <osal.h>
#include <isi.h>
#include <isip.h>
#include "_iut_app.h"
#include "_iut.h"
#include "_iut_cfg.h"

static void _IUT_cfgNone(
    _IUT_Cfg *cfg_ptr, 
    char     *value_ptr);

static void _IUT_cfgName(
    _IUT_Cfg *cfg_ptr, 
    char     *value_ptr);

static void _IUT_cfgProxy(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgStun(
    _IUT_Cfg *cfg_ptr, 
    char     *value_ptr);

static void _IUT_cfgRelay(
    _IUT_Cfg *cfg_ptr, 
    char     *value_ptr);

static void _IUT_cfgXcap(
    _IUT_Cfg *cfg_ptr, 
    char     *value_ptr);

static void _IUT_cfgChat(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgOutbound(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgUsername(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgPassword(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgRealm(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgUri(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgCidPrivate(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgSecurity(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgTo(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgToMsg(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgForward(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgXfer(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgPcmu(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgPcma(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgG729(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgG726(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgiLBC(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgSilk(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgG722(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgDtmfR(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgCn(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgH264(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgH263(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgAmrNb(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgAmrWb(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgPPcmu(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgPPcma(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgPG729(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgPG726(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgPiLBC(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgPSilk(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgPG722(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgPDtmfR(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgPCn(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgPH264(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgPH263(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgPAmrNb(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgPAmrWb(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgEmergency(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgImeiUri(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgAudioPort(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgVideoPort(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static void _IUT_cfgIfAddress(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

static _IUT_Cfg _IUT_Config[IUT_CFG_MAX_SERVICE];

char _IUT_cfgFileName[IUT_STR_SIZE + 1];

static _IUT_CfgEntry _IUT_cfgTable[IUT_CFG_LAST_F] = {
    { IUT_CFG_ID_F,         IUT_CFG_NONE_STR,       _IUT_cfgNone        },
    { IUT_CFG_NAME_F,       IUT_CFG_NAME_STR,       _IUT_cfgName        },
    { IUT_CFG_PROXY_F,      IUT_CFG_PROXY_STR,      _IUT_cfgProxy       },
    { IUT_CFG_STUN_F,       IUT_CFG_STUN_STR,       _IUT_cfgStun        },
    { IUT_CFG_RELAY_F,      IUT_CFG_RELAY_STR,      _IUT_cfgRelay       },
    { IUT_CFG_XCAP_F,       IUT_CFG_XCAP_STR,       _IUT_cfgXcap        },
    { IUT_CFG_CHAT_F,       IUT_CFG_CHAT_STR,       _IUT_cfgChat        },
    { IUT_CFG_OUTBOUND_F,   IUT_CFG_OUTBOUND_STR,   _IUT_cfgOutbound    },
    { IUT_CFG_USERNAME_F,   IUT_CFG_USERNAME_STR,   _IUT_cfgUsername    },
    { IUT_CFG_PASSWORD_F,   IUT_CFG_PASSWORD_STR,   _IUT_cfgPassword    },
    { IUT_CFG_REALM_F,      IUT_CFG_REALM_STR,      _IUT_cfgRealm       },
    { IUT_CFG_URI_F,        IUT_CFG_URI_STR,        _IUT_cfgUri         },
    { IUT_CFG_CID_PRIVATE_F,IUT_CFG_CID_PRIVATE_STR,_IUT_cfgCidPrivate  },
    { IUT_CFG_SECURITY_F,   IUT_CFG_SECURITY_STR,   _IUT_cfgSecurity    },
    { IUT_CFG_TO_URI_F,     IUT_CFG_TO_URI_STR,     _IUT_cfgTo          },
    { IUT_CFG_TO_MSG_URI_F, IUT_CFG_TO_MSG_URI_STR, _IUT_cfgToMsg       },
    { IUT_CFG_FORWARD_URI_F,IUT_CFG_FORWARD_URI_STR,_IUT_cfgForward     },
    { IUT_CFG_XFER_URI_F,   IUT_CFG_XFER_URI_STR,   _IUT_cfgXfer        },  
    { IUT_CFG_PCMU_F,       IUT_CFG_PCMU_STR,       _IUT_cfgPcmu        },
    { IUT_CFG_PCMA_F,       IUT_CFG_PCMA_STR,       _IUT_cfgPcma        },
    { IUT_CFG_G729_F,       IUT_CFG_G729_STR,       _IUT_cfgG729        },
    { IUT_CFG_G726_F,       IUT_CFG_G726_STR,       _IUT_cfgG726        },  
    { IUT_CFG_ILBC_F,       IUT_CFG_ILBC_STR,       _IUT_cfgiLBC        },
    { IUT_CFG_SILK_F,       IUT_CFG_SILK_STR,       _IUT_cfgSilk        },
    { IUT_CFG_G722_F,       IUT_CFG_G722_STR,       _IUT_cfgG722        },
    { IUT_CFG_DTMFR_F,      IUT_CFG_DTMFR_STR,      _IUT_cfgDtmfR       },
    { IUT_CFG_CN_F,         IUT_CFG_CN_STR,         _IUT_cfgCn          },
    { IUT_CFG_H264_F,       IUT_CFG_H264_STR,       _IUT_cfgH264        },
    { IUT_CFG_H263_F,       IUT_CFG_H263_STR,       _IUT_cfgH263        },
    { IUT_CFG_AMRNB_F,      IUT_CFG_AMRNB_STR,      _IUT_cfgAmrNb       },
    { IUT_CFG_AMRWB_F,      IUT_CFG_AMRWB_STR,      _IUT_cfgAmrWb       },
    { IUT_CFG_PPCMU_F,      IUT_CFG_PPCMU_STR,      _IUT_cfgPPcmu       },
    { IUT_CFG_PPCMA_F,      IUT_CFG_PPCMA_STR,      _IUT_cfgPPcma       },
    { IUT_CFG_PG729_F,      IUT_CFG_PG729_STR,      _IUT_cfgPG729       },
    { IUT_CFG_PG726_F,      IUT_CFG_PG726_STR,      _IUT_cfgPG726       },  
    { IUT_CFG_PILBC_F,      IUT_CFG_PILBC_STR,      _IUT_cfgPiLBC       },
    { IUT_CFG_PSILK_F,      IUT_CFG_PSILK_STR,      _IUT_cfgPSilk       },
    { IUT_CFG_PG722_F,      IUT_CFG_PG722_STR,      _IUT_cfgPG722       },
    { IUT_CFG_PDTMFR_F,     IUT_CFG_PDTMFR_STR,     _IUT_cfgPDtmfR      },
    { IUT_CFG_PCN_F,        IUT_CFG_PCN_STR,        _IUT_cfgPCn         },
    { IUT_CFG_PH264_F,      IUT_CFG_PH264_STR,      _IUT_cfgPH264       },
    { IUT_CFG_PH263_F,      IUT_CFG_PH263_STR,      _IUT_cfgPH263       },
    { IUT_CFG_PAMRNB_F,     IUT_CFG_PAMRNB_STR,     _IUT_cfgPAmrNb      },
    { IUT_CFG_PAMRWB_F,     IUT_CFG_PAMRWB_STR,     _IUT_cfgPAmrWb      },
    { IUT_CFG_EMERGENCY_F,  IUT_CFG_ISEMERGENCY_STR,_IUT_cfgEmergency   },
    { IUT_CFG_IMEI_F,       IUT_CFG_IMEI_STR,       _IUT_cfgImeiUri     },
    { IUT_CFG_AUDIO_PORT_F, IUT_CFG_AUDIO_PORT_STR, _IUT_cfgAudioPort   },
    { IUT_CFG_VIDEO_PORT_F, IUT_CFG_VIDEO_PORT_STR, _IUT_cfgVideoPort   },
    { IUT_CFG_IF_ADDRESS_F, IUT_CFG_IF_ADDRESS_STR, _IUT_cfgIfAddress   }
};

/* 
 * ======== _IUT_cfgClean() ========
 *
 * This function zero's the memory used to house the array of _IUT_Cfg objects
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgClean(
    _IUT_Cfg *cfg_ptr)
{
    OSAL_memSet(cfg_ptr, 0, sizeof(_IUT_Cfg));
}

/* 
 * ======== _IUT_strnscan() ========
 *
 * This function is a locally defined implementation of strnscan()
 *
 * Returns: 
 *   char * : a pointer to the place in the string where the target
 *            (sub-string) was found.
 *   NULL   : could not find the sub-string specified by target_ptr
 *            in str_ptr.
 *
 */
static char *_IUT_strnscan(
    char       *str_ptr,
    vint        size,
    const char *target_ptr)
{
    vint i;
    vint len = OSAL_strlen(target_ptr);
    
    while (*str_ptr && size--) {
        i = 0;
        if (str_ptr[i] == target_ptr[i]) {
            i++;
            while (i < len) {
                if (!str_ptr[i]) {
                    return (NULL);
                }
                
                if (str_ptr[i] != target_ptr[i]) {
                    break;
                }

                i++;
            }
            if (i == len) {
                return (str_ptr);
            }
        }
        str_ptr++;
    }
    return (NULL);
}

/* 
 * ======== _IUT_cfgGetEntry() ========
 *
 * This function searches the locally defined _IUT_cfgTable table 
 * looking for an entry that has a string value equal to the string 
 * defined in str_ptr. 
 *
 * Returns: 
 *    _IUT_CfgEntry * : A pointer to the entry in the table that matches
 *                      the string specified in str_ptr.
 *   NULL             : No matches were found.
 *
 */
static _IUT_CfgEntry *_IUT_cfgGetEntry(
    char *str_ptr,
    vint  strLen)
{
    /* Search through the field table to find the enum value */
    vint x;
    for (x = 0; x < IUT_CFG_LAST_F; x++) {
        if (OSAL_strncmp(str_ptr, _IUT_cfgTable[x].str_ptr, strLen) == 0) {
            /* Found it */
            return (&_IUT_cfgTable[x]);
        }
    }
    return (NULL);
}

/* 
 * ======== IUT_cfgInit() ========
 *
 * This function initialized the config module.  It is typically called
 * when the application that implements the config module starts up. 
 * looking for an entry that has a string value equal to the string 
 *
 * Returns: 
 *   Nothing.
 *
 */
void IUT_cfgInit(void)
{
    vint  x;
    /* Clean all the objects before we load them */
    for (x = 0; x < IUT_CFG_MAX_SERVICE; x++) {
        _IUT_cfgClean(&_IUT_Config[x]);
    }
    /* Clean the file name area */
    _IUT_cfgFileName[0] = 0;
    return;
}

/* 
 * ======== IUT_cfgSetServiceId() ========
 *
 * This function sets a serviceId inside an available config object.
 * The serviceId is used to associate ISI Services with a
 * specific config object.
 *
 * Returns: 
 *   IUT_OK  : The serviceId was successfully set..
 *   IUT_ERR : No available config objects are available 
 *             for association to a config object.
 */
vint IUT_cfgSetServiceId(
    ISI_Id serviceId)
{
    vint x;
    /* Find an available configuration resource and set the serviceId */
    for (x = 0; x < IUT_CFG_MAX_SERVICE; x++) {
        if (_IUT_Config[x].serviceId == 0) {
            /* Found an available one */
            _IUT_Config[x].serviceId = serviceId;
            return (IUT_OK);
        }
    }
    return (IUT_ERR);
}

/* 
 * ======== IUT_cfgClearServiceId() ========
 *
 * This function searches the array of config objects for one
 * that has the same serviceId as the specified in the "serviceId" 
 * parameter.  Once it finds it, it clears (zeros) the serviceId value
 * for the config object.
 *
 * Returns: 
 *   Nothing.
 *
 */
void IUT_cfgClearServiceId(
    ISI_Id serviceId)
{
    vint x;
    /* Find an available configuration resource and set the serviceId */
    for (x = 0; x < IUT_CFG_MAX_SERVICE; x++) {
        if (_IUT_Config[x].serviceId == serviceId) {
            /* Found it, clear it */
            _IUT_Config[x].serviceId = 0;
            return;
        }
    }
    return;
}

/* 
 * ======== IUT_cfgSetField() ========
 *
 * This function sets the field entry specified by "field" to the value
 * specified in value_ptr.
 *
 * Returns: 
 *   IUT_OK  : the field was successfully set.
 *   IUT_ERR : The field was unknown and thus not set.
 *
 */
vint IUT_cfgSetField(
    ISI_Id       serviceId,
    IUT_CfgField field,
    char        *value_ptr)
{
    vint           x;
    _IUT_CfgEntry *e_ptr;
    _IUT_Cfg      *c_ptr;
    
    if (!(field < IUT_CFG_LAST_F)) {
        return (IUT_ERR);
    }
    
    /* 
     * First find the right configuration object that 
     * pertains to this service ID 
     */
    for (x = 0; x < IUT_CFG_MAX_SERVICE; x++) {
        if (_IUT_Config[x].serviceId == serviceId) {
            break;
        }
    }

    if (x == IUT_CFG_MAX_SERVICE) {
        /* Could not find the service */
        return (IUT_ERR);
    }

    /* get the config object */
    c_ptr = &_IUT_Config[x];
    /* get the field entry */
    e_ptr = &_IUT_cfgTable[field];
    
    /* Set the value */
    e_ptr->handler(c_ptr, value_ptr);
    return (IUT_OK);
}

/* 
 * ======== IUT_cfgGetStrField() ========
 *
 * This function searches and returns the value associated with the
 *  field specified in "field".  Note that this function should only be 
 * called when the field value is a NULL Terminated string.
 * If the value associated with the "field" is a number then use 
 * IUT_cfgGetIntField()
 *
 * Returns: 
 *   char*  : A pointer to a NULL terminated string containing the value
 *   NULL : The field was unknown or the value associated with the value 
 *          is a number.
 *
 */
char* IUT_cfgGetStrField(
    ISI_Id        serviceId,
    IUT_CfgField  field)
{
    vint        x;
    _IUT_Cfg   *c_ptr;
    char       *str_ptr;
    
    /* 
     * First find the right configuration object that 
     * pertains to this service ID 
     */
    for (x = 0; x < IUT_CFG_MAX_SERVICE; x++) {
        if (_IUT_Config[x].serviceId == serviceId) {
            break;
        }
    }

    if (x == IUT_CFG_MAX_SERVICE) {
        /* Could not find the service */
        return (IUT_ERR);
    }

    /* get the config object */
    c_ptr = &_IUT_Config[x];

    switch (field) {
    case IUT_CFG_NAME_F:
        str_ptr = c_ptr->name;
        break;
    case IUT_CFG_PROXY_F:
        str_ptr = c_ptr->proxy;
        break;
    case IUT_CFG_STUN_F:
        str_ptr = c_ptr->stun;
        break;
    case IUT_CFG_RELAY_F:
        str_ptr = c_ptr->relay;
        break;
    case IUT_CFG_XCAP_F:
        str_ptr = c_ptr->xcap;
        break;
    case IUT_CFG_CHAT_F:
        str_ptr = c_ptr->chat;
        break;
    case IUT_CFG_OUTBOUND_F:
        str_ptr = c_ptr->outbound;
        break;
    case IUT_CFG_USERNAME_F:
        str_ptr = c_ptr->username;
        break;
    case IUT_CFG_PASSWORD_F:
        str_ptr = c_ptr->password;
        break;
    case IUT_CFG_REALM_F:
        str_ptr = c_ptr->realm;
        break;
    case IUT_CFG_URI_F:
        str_ptr = c_ptr->uri;
        break;
    case IUT_CFG_TO_URI_F:
        str_ptr = c_ptr->toUri;
        break;
    case IUT_CFG_TO_MSG_URI_F:
        str_ptr = c_ptr->toMsgUri;
        break;
    case IUT_CFG_FORWARD_URI_F:
        str_ptr = c_ptr->forwardUri;
        break;
    case IUT_CFG_XFER_URI_F:
        str_ptr = c_ptr->xferUri;
        break;
    case IUT_CFG_IMEI_F:
        str_ptr = c_ptr->imeiUri;
        break;
    case IUT_CFG_IF_ADDRESS_F:
        str_ptr = c_ptr->interfaceAddress;
        break;
    default:
        str_ptr = IUT_CFG_NONE_STR;
    } /* End of switch statement */
    return (str_ptr);
}

/* 
 * ======== IUT_cfgGetIntField() ========
 *
 * This function searches and returns the value associated with the
 * field specified in "field".  Note that this function should only be 
 * called when the field value is an integer number.
 * If the value associated with the "field" is a string then use 
 * IUT_()
 *
 * Returns: 
 *   -1  : The field specified in "field" is unknown or the value associated
 *         with the "field": is a string and not a number.
 *   !(-1) : The value of field.
 *
 */
vint IUT_cfgGetIntField(
    ISI_Id        serviceId,
    IUT_CfgField  field)
{
    vint        x;
    _IUT_Cfg   *c_ptr;
    vint        value;
    
    /* 
     * First find the right configuration object that 
     * pertains to this service ID 
     */
    for (x = 0; x < IUT_CFG_MAX_SERVICE; x++) {
        if (_IUT_Config[x].serviceId == serviceId) {
            break;
        }
    }

    if (x == IUT_CFG_MAX_SERVICE) {
        /* Could not find the service */
        return (-1);
    }

    /* get the config object */
    c_ptr = &_IUT_Config[x];

    switch (field) {
    case IUT_CFG_CID_PRIVATE_F:
        value = c_ptr->cidPrivate;
        break;
    case IUT_CFG_SECURITY_F:
        value = c_ptr->security;
        break;
    case IUT_CFG_PCMU_F:
        value = c_ptr->pcmu;
        break;
    case IUT_CFG_PCMA_F:
        value = c_ptr->pcma;
        break;
    case IUT_CFG_G729_F:
        value = c_ptr->g729;
        break;
    case IUT_CFG_G726_F:
        value = c_ptr->g726;
        break;
    case IUT_CFG_ILBC_F:
        value = c_ptr->iLBC;
        break;
    case IUT_CFG_SILK_F:
        value = c_ptr->silk;
        break;
    case IUT_CFG_G722_F:
        value = c_ptr->g722;
        break;        
    case IUT_CFG_DTMFR_F:
        value = c_ptr->dtmfr;
        break;
    case IUT_CFG_H264_F:
        value = c_ptr->h264;
        break;
    case IUT_CFG_PPCMU_F:
        value = c_ptr->ppcmu;
        break;
    case IUT_CFG_PPCMA_F:
        value = c_ptr->ppcma;
        break;
    case IUT_CFG_PG729_F:
        value = c_ptr->pg729;
        break;
    case IUT_CFG_PH264_F:
        value = c_ptr->ph264;
        break;
    case IUT_CFG_AMRNB_F:
        value = c_ptr->amrnb;
        break;
    case IUT_CFG_AMRWB_F:
        value = c_ptr->amrwb;
        break;
    case IUT_CFG_PG726_F:
        value = c_ptr->pg726;
        break;
    case IUT_CFG_PILBC_F:
        value = c_ptr->piLBC;
        break;
    case IUT_CFG_PSILK_F:
        value = c_ptr->psilk;
        break;
    case IUT_CFG_PG722_F:
        value = c_ptr->pg722;
        break;
    case IUT_CFG_PDTMFR_F:
        value = c_ptr->pdtmfr;
        break;
    case IUT_CFG_CN_F:
        value = c_ptr->cn;
        break;
    case IUT_CFG_PCN_F:
        value = c_ptr->pcn;
        break;
    case IUT_CFG_H263_F:
        value = c_ptr->h263;
        break;
    case IUT_CFG_PH263_F:
        value = c_ptr->ph263;
        break;
    case IUT_CFG_PAMRNB_F:
        value = c_ptr->pamrnb;
        break;
    case IUT_CFG_PAMRWB_F:
        value = c_ptr->pamrwb;
        break;
    case IUT_CFG_EMERGENCY_F:
        value = c_ptr->isEmergency;
        break;
    case IUT_CFG_AUDIO_PORT_F:
        value = c_ptr->audioPortNumber;
        break;
    case IUT_CFG_VIDEO_PORT_F:
        value = c_ptr->videoPortNumber;
        break;
    default:
        value = -1;
        break;
    } /* End of switch statement */
    return (value);
}

/* 
 * ======== _IUT_cfgWriteOut() ========
 *
 * This function writes the values in the config object pointed to by cfg_ptr
 * to a text file that is already open and specified in by fid_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgWriteOut(
    OSAL_FileId fid,
    _IUT_Cfg *cfg_ptr)
{
    char     outBuff[IUT_CFG_STR_SZ + 1];
    vint     len;
    
    /* Ensure the last byte in the string is NULL */
    outBuff[IUT_CFG_STR_SZ] = 0;

    if ((len = OSAL_snprintf(outBuff, IUT_CFG_STR_SZ, "%s\n",
            IUT_CFG_SERVICE_STR))) {
        OSAL_fileWrite(&fid, outBuff, &len);
    }

    if ((len = OSAL_snprintf(outBuff, IUT_CFG_STR_SZ, IUT_CFG_STR_OUT, 
            IUT_CFG_NAME_STR, IUT_CFG_DELIMITER_STR, cfg_ptr->name))) {
        OSAL_fileWrite(&fid, outBuff, &len);
    }

    if ((len = OSAL_snprintf(outBuff, IUT_CFG_STR_SZ, IUT_CFG_STR_OUT, 
            IUT_CFG_PROXY_STR, IUT_CFG_DELIMITER_STR, cfg_ptr->proxy))) {
        OSAL_fileWrite(&fid, outBuff, &len);
    }

    if ((len = OSAL_snprintf(outBuff, IUT_CFG_STR_SZ, IUT_CFG_STR_OUT, 
            IUT_CFG_STUN_STR, IUT_CFG_DELIMITER_STR, cfg_ptr->stun))) {
        OSAL_fileWrite(&fid, outBuff, &len);
    }

    if ((len = OSAL_snprintf(outBuff, IUT_CFG_STR_SZ, IUT_CFG_STR_OUT, 
            IUT_CFG_RELAY_STR, IUT_CFG_DELIMITER_STR, cfg_ptr->relay))) {
        OSAL_fileWrite(&fid, outBuff, &len);
    }

    if ((len = OSAL_snprintf(outBuff, IUT_CFG_STR_SZ, IUT_CFG_STR_OUT, 
            IUT_CFG_OUTBOUND_STR, IUT_CFG_DELIMITER_STR, cfg_ptr->outbound))) {
        OSAL_fileWrite(&fid, outBuff, &len);
    }

    if ((len = OSAL_snprintf(outBuff, IUT_CFG_STR_SZ, IUT_CFG_STR_OUT, 
            IUT_CFG_USERNAME_STR, IUT_CFG_DELIMITER_STR, cfg_ptr->username))) {
        OSAL_fileWrite(&fid, outBuff, &len);
    }

    if ((len = OSAL_snprintf(outBuff, IUT_CFG_STR_SZ, IUT_CFG_STR_OUT, 
            IUT_CFG_PASSWORD_STR, IUT_CFG_DELIMITER_STR, cfg_ptr->password))) {
        OSAL_fileWrite(&fid, outBuff, &len);
    }

    if ((len = OSAL_snprintf(outBuff, IUT_CFG_STR_SZ, IUT_CFG_STR_OUT, 
            IUT_CFG_REALM_STR, IUT_CFG_DELIMITER_STR, cfg_ptr->realm))) {
        OSAL_fileWrite(&fid, outBuff, &len);
    }

    if ((len = OSAL_snprintf(outBuff, IUT_CFG_STR_SZ, IUT_CFG_STR_OUT, 
            IUT_CFG_URI_STR, IUT_CFG_DELIMITER_STR, cfg_ptr->uri))) {
        OSAL_fileWrite(&fid, outBuff, &len);
    }

    if ((len = OSAL_snprintf(outBuff, IUT_CFG_STR_SZ, IUT_CFG_INT_OUT, 
            IUT_CFG_CID_PRIVATE_STR, IUT_CFG_DELIMITER_STR, 
            cfg_ptr->cidPrivate))) {
        OSAL_fileWrite(&fid, outBuff, &len);
    }
    
    if ((len = OSAL_snprintf(outBuff, IUT_CFG_STR_SZ, IUT_CFG_INT_OUT, 
            IUT_CFG_SECURITY_STR, IUT_CFG_DELIMITER_STR, 
            cfg_ptr->security))) {
        OSAL_fileWrite(&fid, outBuff, &len);
    }

    if ((len = OSAL_snprintf(outBuff, IUT_CFG_STR_SZ, IUT_CFG_STR_OUT, 
            IUT_CFG_TO_URI_STR, IUT_CFG_DELIMITER_STR, cfg_ptr->toUri))) {
        OSAL_fileWrite(&fid, outBuff, &len);
    }

    if ((len = OSAL_snprintf(outBuff, IUT_CFG_STR_SZ, IUT_CFG_STR_OUT, 
            IUT_CFG_TO_MSG_URI_STR, IUT_CFG_DELIMITER_STR,
            cfg_ptr->toMsgUri))) {
        OSAL_fileWrite(&fid, outBuff, &len);
    }

    if ((len = OSAL_snprintf(outBuff, IUT_CFG_STR_SZ, IUT_CFG_STR_OUT, 
            IUT_CFG_FORWARD_URI_STR, IUT_CFG_DELIMITER_STR,
            cfg_ptr->forwardUri))) {
        OSAL_fileWrite(&fid, outBuff, &len);
    }

    if ((len = OSAL_snprintf(outBuff, IUT_CFG_STR_SZ, IUT_CFG_STR_OUT, 
            IUT_CFG_XFER_URI_STR, IUT_CFG_DELIMITER_STR, cfg_ptr->xferUri))) {
        OSAL_fileWrite(&fid, outBuff, &len);
    }

    if ((len = OSAL_snprintf(outBuff, IUT_CFG_STR_SZ, IUT_CFG_INT_OUT, 
            IUT_CFG_PCMU_STR, IUT_CFG_DELIMITER_STR, cfg_ptr->pcmu))) {
        OSAL_fileWrite(&fid, outBuff, &len);
    }

    if ((len = OSAL_snprintf(outBuff, IUT_CFG_STR_SZ, IUT_CFG_INT_OUT, 
            IUT_CFG_PCMA_STR, IUT_CFG_DELIMITER_STR, cfg_ptr->pcma))) {
        OSAL_fileWrite(&fid, outBuff, &len);
    }

    if ((len = OSAL_snprintf(outBuff, IUT_CFG_STR_SZ, IUT_CFG_INT_OUT, 
            IUT_CFG_G729_STR, IUT_CFG_DELIMITER_STR, cfg_ptr->g729))) {
        OSAL_fileWrite(&fid, outBuff, &len);
    }

    if ((len = OSAL_snprintf(outBuff, IUT_CFG_STR_SZ, IUT_CFG_INT_OUT, 
            IUT_CFG_G726_STR, IUT_CFG_DELIMITER_STR, cfg_ptr->g726))) {
        OSAL_fileWrite(&fid, outBuff, &len);
    }

    if ((len = OSAL_snprintf(outBuff, IUT_CFG_STR_SZ, IUT_CFG_INT_OUT, 
            IUT_CFG_ILBC_STR, IUT_CFG_DELIMITER_STR, cfg_ptr->iLBC))) {
        OSAL_fileWrite(&fid, outBuff, &len);
    }

    if ((len = OSAL_snprintf(outBuff, IUT_CFG_STR_SZ, IUT_CFG_INT_OUT, 
            IUT_CFG_SILK_STR, IUT_CFG_DELIMITER_STR, cfg_ptr->silk))) {
        OSAL_fileWrite(&fid, outBuff, &len);
    }

    if ((len = OSAL_snprintf(outBuff, IUT_CFG_STR_SZ, IUT_CFG_INT_OUT, 
            IUT_CFG_G722_STR, IUT_CFG_DELIMITER_STR, cfg_ptr->g722))) {
        OSAL_fileWrite(&fid, outBuff, &len);
    }

    if ((len = OSAL_snprintf(outBuff, IUT_CFG_STR_SZ, IUT_CFG_INT_OUT, 
            IUT_CFG_DTMFR_STR, IUT_CFG_DELIMITER_STR, cfg_ptr->dtmfr))) {
        OSAL_fileWrite(&fid, outBuff, &len);
    }

    if ((len = OSAL_snprintf(outBuff, IUT_CFG_STR_SZ, IUT_CFG_INT_OUT, 
            IUT_CFG_CN_STR, IUT_CFG_DELIMITER_STR, cfg_ptr->cn))) {
        OSAL_fileWrite(&fid, outBuff, &len);
    }
    
    if ((len = OSAL_snprintf(outBuff, IUT_CFG_STR_SZ, IUT_CFG_INT_OUT, 
            IUT_CFG_H264_STR, IUT_CFG_DELIMITER_STR, cfg_ptr->h264))) {
        OSAL_fileWrite(&fid, outBuff, &len);
    }


    if ((len = OSAL_snprintf(outBuff, IUT_CFG_STR_SZ, IUT_CFG_INT_OUT,
            IUT_CFG_AMRNB_STR, IUT_CFG_DELIMITER_STR, cfg_ptr->amrnb))) {
        OSAL_fileWrite(&fid, outBuff, &len);
    }

    if ((len = OSAL_snprintf(outBuff, IUT_CFG_STR_SZ, IUT_CFG_INT_OUT,
            IUT_CFG_AMRWB_STR, IUT_CFG_DELIMITER_STR, cfg_ptr->amrwb))) {
        OSAL_fileWrite(&fid, outBuff, &len);
    }
    return;
}

/* 
 * ======== IUT_cfgWrite() ========
 *
 * This function writes all configurations to the text file specified 
 * in cFile_ptr.  If cFile_ptr is NULL then the function will attempt
 * to write the configuration data to the default path/file specified 
 * by the IUT_CONFIG_FILE_NAME & IUT_CONFIG_FILE_DIR preprocessor
 * directives
 *
 * Returns: 
 *   IUT_OK : The file was successfully written.
 *   IUT_ERR : The file could not be written.  More than likely
 *             the file could not opened due permissions.
 *
 */
vint IUT_cfgWrite(char *cFile_ptr)
{
    OSAL_FileId fid;
    vint   x;
    vint size;

    if (OSAL_SUCCESS != OSAL_fileOpen(&fid, cFile_ptr,
            OSAL_FILE_O_WRONLY | OSAL_FILE_O_TRUNC, 0)) {
        /* Try the default */
        if (OSAL_SUCCESS != OSAL_fileOpen(&fid, _IUT_cfgFileName,
                OSAL_FILE_O_CREATE | OSAL_FILE_O_WRONLY | OSAL_FILE_O_TRUNC, 00755)) {
            /* Failed to open file, check permissions */
            return (IUT_ERR);
        }
    }

    for (x = 0; x < IUT_CFG_MAX_SERVICE; x++) {
        _IUT_cfgWriteOut(fid, &_IUT_Config[x]);
        size = OSAL_strlen(IUT_CFG_BREAK_STR);
        OSAL_fileWrite(&fid, IUT_CFG_BREAK_STR, &size);
    }
    size = 1;
    OSAL_fileWrite(&fid, "\n", &size);
    OSAL_fileClose(&fid);
    return (IUT_OK);
}

static char _IUT_getChar(OSAL_FileId fid)
{
    char c;
    vint size = 1;
    if (OSAL_SUCCESS == OSAL_fileRead(&fid, &c, &size)) {
        return c;
    }
    return 0;
}


static OSAL_Status _IUT_getLine(
    OSAL_FileId   fid,
    char         *target_ptr,
    vint          maxTargetLen,
    vint         *bytesRead_ptr)
{
    char c;
    vint count = 0;
    vint stateComment = 0;
    vint bytesRead = 0;
    while (1) {
        if (0 == (c = _IUT_getChar(fid))) {
            /* Then we are done. */
            bytesRead++;
            break;
        }
        bytesRead++;
        if (0 != stateComment) {
            /* Then we want to skip until the next line. */
            if ('\n' == c) {
                /* Let's go back to the normal state. */
                count = 0;
                target_ptr[count] = 0;
                stateComment = 0;
            }
            continue;
        }

        /*
         * Remove '\r' chars for DOS compatibility.
         */
        if ('\r' == c) {
            continue;
        }
        /*
         * Remove comments
         */
        else if ('#' == c) {
            stateComment = 1;
            continue;
        }
        /*
         * EOL, break and process string
         */
        if ('\n' == c) {
            break;
        }

        if (count < maxTargetLen) {
            target_ptr[count++] = c;
        }
    }
    *bytesRead_ptr = bytesRead;
    if (0 != OSAL_strlen(target_ptr)) {
        return (OSAL_SUCCESS);
    }
    return (OSAL_FAIL);
}

/* 
 * ======== _IUT_cfgReadIn() ========
 *
 * This function reads values from a open text file specified by fid_ptr
 * populates an array of config objects specified in cfgArray.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgReadIn(
    OSAL_FileId  fid,
    _IUT_Cfg     cfgArray[],
    vint         cfgArraySize)
{
    char             inbuf[IUT_CFG_STR_SZ + 1];
    char            *str_ptr;
    _IUT_CfgEntry   *e_ptr;
    vint             cfgIdx;
    vint             bytesRead;
    vint             fileSize;

    cfgIdx = -1;
    if (OSAL_SUCCESS != OSAL_fileGetSize(&fid, &fileSize)) {
        OSAL_logMsg("%s %d: ERROR Could not read the file config file!",
                __FUNCTION__, __LINE__);
        return;
    }
    /* Keep reading/processing until the whole file is done. */
    while(fileSize >= 0) {
        OSAL_memSet(inbuf, 0, sizeof(inbuf));
        if (OSAL_SUCCESS == _IUT_getLine(fid, inbuf, IUT_CFG_STR_SZ, &bytesRead)) {
            /* Whenever we see "Service", we advance the array of configurations */
            if ((str_ptr = _IUT_strnscan(inbuf, IUT_CFG_STR_SZ,
                    IUT_CFG_SERVICE_STR)) != NULL) {
                cfgIdx++;
            }

            if (cfgIdx >= 0 && cfgIdx < cfgArraySize) {
                if ((str_ptr = _IUT_strnscan(inbuf, IUT_CFG_STR_SZ,
                        IUT_CFG_DELIMITER_STR)) != NULL) {
                    /*
                     * NULL terminate and check if the field exists
                     * in the lookup table
                     */
                    *str_ptr++ = 0;
                    if ((e_ptr = _IUT_cfgGetEntry(inbuf, IUT_CFG_STR_SZ)) !=
                            NULL) {
                        /* Then it exists, set it */
                        e_ptr->handler(&cfgArray[cfgIdx], str_ptr);
                    }
                }
            }
        } /* End of first if statement */
        fileSize -= bytesRead;
    }
    return;
}

/* 
 * ======== IUT_cfgRead() ========
 *
 * This function reads configuration info from a text file
 * specified by cFile_ptr and populates config objects with the
 * values provided in the text file.
 * Note, if cFile_ptr is NULL, then the this function will attempt 
 * to search and open a default file specified in the 
 * IUT_CONFIG_FILE_NAME & IUT_CONFIG_FILE_DIR preprocessor directives
 *
 * Returns: 
 *   IUT_OK : The file was successfully read.
 *   IUT_ERR : The file could not be found or opened.
 *
 */
vint IUT_cfgRead(char *cFile_ptr)
{
    OSAL_FileId fid;
    vint    x;
    char    ch;
    vint    size;
    
    if (cFile_ptr && cFile_ptr[0] != 0) {
        /* Then the caller has specified the config file to use */
        OSAL_snprintf(_IUT_cfgFileName, IUT_STR_SIZE, "%s", cFile_ptr);
        if (OSAL_SUCCESS != OSAL_fileOpen(&fid, _IUT_cfgFileName, OSAL_FILE_O_RDONLY, 0)) {
            OSAL_logMsg("No %s file found, create a fresh one? <0/1>\n",
                    IUT_CONFIG_FILE_NAME);
            ch = XXGETCH();
            if ('1' == ch) {
                /* First attempt to create one in the correct directory */
                OSAL_snprintf(_IUT_cfgFileName, IUT_STR_SIZE, "%s",
                        IUT_CONFIG_FILE_NAME);
                if (OSAL_SUCCESS == OSAL_fileOpen(&fid, _IUT_cfgFileName,
                        OSAL_FILE_O_WRONLY | OSAL_FILE_O_CREATE, 00755)) {
                    /* Write something and then close and return */
                    size = 1;
                    OSAL_fileWrite(&fid, "\n", &size);
                    OSAL_fileClose(&fid);
                    return (IUT_OK);
                }
            }
            return (IUT_ERR);
        }
    }
    else {
        return (IUT_ERR);
    }

    /* Clean all the objects before we load them */
    for (x = 0; x < IUT_CFG_MAX_SERVICE; x++) {
        _IUT_cfgClean(&_IUT_Config[x]);
    }
    
    _IUT_cfgReadIn(fid, _IUT_Config, IUT_CFG_MAX_SERVICE);
    
    OSAL_fileClose(&fid);
    return (IUT_OK);
}

/* 
 * ======== _IUT_cfgString() ========
 *
 * This function copies a string from to_ptr to from_ptr
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgString(
    char   *to_ptr,
    vint    maxTo,
    char   *from_ptr)
{
    if (from_ptr && (*from_ptr != 0)) {
        OSAL_snprintf(to_ptr, maxTo, "%s", from_ptr);
    }
    else {
        *to_ptr = 0;
    }
    return;
}

/* 
 * ======== _IUT_cfgInteger() ========
 *
 * This function translates a string containing a number
 * to the integer representation. The integer will be written to the 
 * vint value provided in int_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgInteger(
    vint    *int_ptr,
    char    *value_ptr)
{
    vint x;
    if (value_ptr && (*value_ptr != 0)) {
        x = OSAL_atoi(value_ptr);
    }
    else {
        x = 0;
    }
    *int_ptr = x;
    return;
}

/* 
 * ======== _IUT_cfgInt() ========
 *
 * This function translates a string containing a number
 * to the integer representation. The integer will be written to the 
 * vint value provided in int_ptr and normalized.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgInt(
    vint    *int_ptr,
    char    *value_ptr)
{
    vint x;
    if (value_ptr && (*value_ptr != 0)) {
        x = OSAL_atoi(value_ptr);
    }
    else {
        x = 0;
    }
    /* Normalize it */
    x = (x == 0) ? 0 : 1;
    *int_ptr = x;
    return;
}

/* 
 * ======== _IUT_cfgNone() ========
 *
 * A No-op (dummy) function.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgNone(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    /* No-op */
    return;
}

/* 
 * ======== _IUT_cfgName() ========
 *
 * This function sets the NULL terminated string specified by value_ptr
 * to the "name" field within the config object specified by cfg_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgName(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgString(cfg_ptr->name, IUT_CFG_STR_SZ, value_ptr);
}

/* 
 * ======== _IUT_cfgProxy() ========
 *
 * This function sets the NULL terminated string specified by value_ptr
 * to the "proxy" field within the config object specified by cfg_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgProxy(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgString(cfg_ptr->proxy, IUT_CFG_STR_SZ, value_ptr);
}

/* 
 * ======== _IUT_cfgStun() ========
 *
 * This function sets the NULL terminated string specified by value_ptr
 * to the "stun server" field within the config object specified by cfg_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgStun(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgString(cfg_ptr->stun, IUT_CFG_STR_SZ, value_ptr);
}

/* 
 * ======== _IUT_cfgRelay() ========
 *
 * This function sets the NULL terminated string specified by value_ptr
 * to the "relay server" field within the config object specified by cfg_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgRelay(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgString(cfg_ptr->relay, IUT_CFG_STR_SZ, value_ptr);
}

/* 
 * ======== _IUT_cfgXcap() ========
 *
 * This function sets the NULL terminated string specified by value_ptr
 * to the "xcap root" field within the config object specified by cfg_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgXcap(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgString(cfg_ptr->xcap, IUT_CFG_STR_SZ, value_ptr);
}

/*
 * ======== _IUT_cfgChat() ========
 *
 * This function sets the NULL terminated string specified by value_ptr
 * to the "chat server" field within the config object specified by cfg_ptr.
 *
 * Returns:
 *   Nothing.
 *
 */
static void _IUT_cfgChat(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgString(cfg_ptr->chat, IUT_CFG_STR_SZ, value_ptr);
}

/* 
 * ======== _IUT_cfgOutbound() ========
 *
 * This function sets the NULL terminated string specified by value_ptr
 * to the "outbound proxy" field within the config object specified by cfg_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgOutbound(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgString(cfg_ptr->outbound, IUT_CFG_STR_SZ, value_ptr);
}

/* 
 * ======== _IUT_cfgUsername() ========
 *
 * This function sets the NULL terminated string specified by value_ptr
 * to the "username" field within the config object specified by cfg_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgUsername(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgString(cfg_ptr->username, IUT_CFG_STR_SZ, value_ptr);
}

/* 
 * ======== _IUT_cfgPassword() ========
 *
 * This function sets the NULL terminated string specified by value_ptr
 * to the "password" field within the config object specified by cfg_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgPassword(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgString(cfg_ptr->password, IUT_CFG_STR_SZ, value_ptr);
}

/* 
 * ======== _IUT_cfgRealm() ========
 *
 * This function sets the NULL terminated string specified by value_ptr
 * to the "realm" field within the config object specified by cfg_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgRealm(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgString(cfg_ptr->realm, IUT_CFG_STR_SZ, value_ptr);
}

/* 
 * ======== _IUT_cfgUri() ========
 *
 * This function sets the NULL terminated string specified by value_ptr
 * to the "uri" field within the config object specified by cfg_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgUri(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgString(cfg_ptr->uri, IUT_CFG_STR_SZ, value_ptr);
}

/* 
 * ======== _IUT_cfgImei() ========
 *
 * This function sets the NULL terminated string specified by value_ptr
 * to the "imeiUri" field within the config object specified by cfg_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgImeiUri(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgString(cfg_ptr->imeiUri, IUT_CFG_STR_SZ, value_ptr);
}

/* 
 * ======== _IUT_cfgEmergency() ========
 *
 * This function sets the NULL terminated string specified by value_ptr
 * to the "uri" field within the config object specified by cfg_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgEmergency(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgInt(&cfg_ptr->isEmergency, value_ptr);
}

/* 
 * ======== _IUT_cfgAudioPort() ========
 *
 * This function sets the audio port number.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgAudioPort(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgInt(&cfg_ptr->audioPortNumber, value_ptr);
}
/* 
 * ======== _IUT_cfgVideoPort() ========
 *
 * This function sets the video port number.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgVideoPort(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgInt(&cfg_ptr->videoPortNumber,  value_ptr);
}
/* 
 * ======== _IUT_cfgVideoPort() ========
 *
 * This function sets the interface ip address.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgIfAddress(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgString(cfg_ptr->interfaceAddress, IUT_CFG_STR_SZ, value_ptr);
}

/* 
 * ======== _IUT_cfgCidPrivate() ========
 *
 * This function sets the "cidPrivate" field inside the config object 
 * specified by cfg_ptr, to the integer specified in the NULL terminated 
 * string value_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgCidPrivate(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgInt(&cfg_ptr->cidPrivate, value_ptr);
}

/* 
 * ======== _IUT_cfgSecurity() ========
 *
 * This function sets the "security" field inside the config object 
 * specified by cfg_ptr, to the integer specified in the NULL terminated 
 * string value_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgSecurity(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgInt(&cfg_ptr->security, value_ptr);
}

/* 
 * ======== _IUT_cfgTo() ========
 *
 * This function sets the NULL terminated string specified by value_ptr
 * to the "to URI" field within the config object specified by cfg_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgTo(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgString(cfg_ptr->toUri, IUT_CFG_STR_SZ, value_ptr);
}

/* 
 * ======== _IUT_cfgToMsg() ========
 *
 * This function sets the NULL terminated string specified by value_ptr
 * to the "to Message URI" field within the config object specified by cfg_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgToMsg(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgString(cfg_ptr->toMsgUri, IUT_CFG_STR_SZ, value_ptr);
}

/* 
 * ======== _IUT_cfgForward() ========
 *
 * This function sets the NULL terminated string specified by value_ptr
 * to the "forward URI" field within the config object specified by cfg_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgForward(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgString(cfg_ptr->forwardUri, IUT_CFG_STR_SZ, value_ptr);
}

/* 
 * ======== _IUT_cfgXfer() ========
 *
 * This function sets the NULL terminated string specified by value_ptr
 * to the "transfer target URI" field within the config object specified by
 * cfg_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgXfer(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgString(cfg_ptr->xferUri, IUT_CFG_STR_SZ, value_ptr);
}

/* 
 * ======== _IUT_cfgPcmu() ========
 *
 * This function sets the "pcmu" field inside the config object 
 * specified by cfg_ptr, to the integer specified in the NULL terminated 
 * string value_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgPcmu(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgInt(&cfg_ptr->pcmu, value_ptr);
}

/* 
 * ======== _IUT_cfgPPcmu() ========
 *
 * This function sets the coder's priority field inside the config object 
 * specified by cfg_ptr, to the integer specified in the NULL terminated 
 * string value_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgPPcmu(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgInteger(&cfg_ptr->ppcmu, value_ptr);
}

/* 
 * ======== _IUT_cfgPcma() ========
 *
 * This function sets the "pcma" field inside the config object 
 * specified by cfg_ptr, to the integer specified in the NULL terminated 
 * string value_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgPcma(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgInt(&cfg_ptr->pcma, value_ptr);
}

/* 
 * ======== _IUT_cfgPPcma() ========
 *
 * This function sets the coder's priority field inside the config object 
 * specified by cfg_ptr, to the integer specified in the NULL terminated 
 * string value_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgPPcma(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgInteger(&cfg_ptr->ppcma, value_ptr);
}

/* 
 * ======== _IUT_cfgG729() ========
 *
 * This function sets the "g729" field inside the config object 
 * specified by cfg_ptr, to the integer specified in the NULL terminated 
 * string value_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgG729(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgInt(&cfg_ptr->g729, value_ptr);
}

/* 
 * ======== _IUT_cfgPPG729() ========
 *
 * This function sets the coder's priority field inside the config object 
 * specified by cfg_ptr, to the integer specified in the NULL terminated 
 * string value_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgPG729(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgInteger(&cfg_ptr->pg729, value_ptr);
}

/* 
 * ======== _IUT_cfgG726() ========
 *
 * This function sets the "g726" field inside the config object 
 * specified by cfg_ptr, to the integer specified in the NULL terminated 
 * string value_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgG726(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgInt(&cfg_ptr->g726, value_ptr);
}

/* 
 * ======== _IUT_cfgPG726() ========
 *
 * This function sets the coder's priority field inside the config object 
 * specified by cfg_ptr, to the integer specified in the NULL terminated 
 * string value_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgPG726(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgInteger(&cfg_ptr->pg726, value_ptr);
}

/* 
 * ======== _IUT_cfgiLBC() ========
 *
 * This function sets the "iLBC" field inside the config object 
 * specified by cfg_ptr, to the integer specified in the NULL terminated 
 * string value_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgiLBC(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgInt(&cfg_ptr->iLBC, value_ptr);
}

/* 
 * ======== _IUT_cfgPiLBC() ========
 *
 * This function sets the coder's priority field inside the config object 
 * specified by cfg_ptr, to the integer specified in the NULL terminated 
 * string value_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgPiLBC(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgInteger(&cfg_ptr->piLBC, value_ptr);
}

/* 
 * ======== _IUT_cfgSilk() ========
 *
 * This function sets the "silk" field inside the config object 
 * specified by cfg_ptr, to the integer specified in the NULL terminated 
 * string value_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgSilk(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgInt(&cfg_ptr->silk, value_ptr);
}

/* 
 * ======== _IUT_cfgPSilk() ========
 *
 * This function sets the coder's priority field inside the config object 
 * specified by cfg_ptr, to the integer specified in the NULL terminated 
 * string value_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgPSilk(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgInteger(&cfg_ptr->psilk, value_ptr);
}

/* 
 * ======== _IUT_cfgG722() ========
 *
 * This function sets the "silk" field inside the config object 
 * specified by cfg_ptr, to the integer specified in the NULL terminated 
 * string value_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgG722(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgInt(&cfg_ptr->g722, value_ptr);
}

/* 
 * ======== _IUT_cfgPG722() ========
 *
 * This function sets the coder's priority field inside the config object 
 * specified by cfg_ptr, to the integer specified in the NULL terminated 
 * string value_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgPG722(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgInteger(&cfg_ptr->pg722, value_ptr);
}

/*
 * ======== _IUT_cfgDtmfR() ========
 *
 * This function sets the "dtmf relay" field inside the config object 
 * specified by cfg_ptr, to the integer specified in the NULL terminated 
 * string value_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgDtmfR(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgInt(&cfg_ptr->dtmfr, value_ptr);
}

/* 
 * ======== _IUT_cfgPDtmfR() ========
 *
 * This function sets the coder's priority field inside the config object 
 * specified by cfg_ptr, to the integer specified in the NULL terminated 
 * string value_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgPDtmfR(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgInteger(&cfg_ptr->pdtmfr, value_ptr);
}

/* 
 * ======== _IUT_cfgCn() ========
 *
 * This function sets the "comfort noise" field inside the config object 
 * specified by cfg_ptr, to the integer specified in the NULL terminated 
 * string value_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgCn(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgInt(&cfg_ptr->cn, value_ptr);
}

/* 
 * ======== _IUT_cfgPCn() ========
 *
 * This function sets the coder's priority field inside the config object 
 * specified by cfg_ptr, to the integer specified in the NULL terminated 
 * string value_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgPCn(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgInteger(&cfg_ptr->pcn, value_ptr);
}

/* 
 * ======== _IUT_cfgH264() ========
 *
 * This function sets the "h264" field inside the config object 
 * specified by cfg_ptr, to the integer specified in the NULL terminated 
 * string value_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgH264(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgInt(&cfg_ptr->h264, value_ptr);
}

/* 
 * ======== _IUT_cfgPH264() ========
 *
 * This function sets the coder's priority field inside the config object 
 * specified by cfg_ptr, to the integer specified in the NULL terminated 
 * string value_ptr.
 *
 * Returns: 
 *   Nothing.
 *
 */
static void _IUT_cfgPH264(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgInteger(&cfg_ptr->ph264, value_ptr);
}


/*
 * ======== _IUT_cfgH263() ========
 *
 * This function sets the "h263" field inside the config object
 * specified by cfg_ptr, to the integer specified in the NULL terminated
 * string value_ptr.
 *
 * Returns:
 *   Nothing.
 *
 */
static void _IUT_cfgH263(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgInt(&cfg_ptr->h263, value_ptr);
}

/*
 * ======== _IUT_cfgPH263() ========
 *
 * This function sets the coder's priority field inside the config object
 * specified by cfg_ptr, to the integer specified in the NULL terminated
 * string value_ptr.
 *
 * Returns:
 *   Nothing.
 *
 */
static void _IUT_cfgPH263(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgInteger(&cfg_ptr->ph263, value_ptr);
}

/*
 * ======== _IUT_cfgAmrNb() ========
 *
 * This function sets the "amrnb" field inside the config object
 * specified by cfg_ptr, to the integer specified in the NULL terminated
 * string value_ptr.
 *
 * Returns:
 *   Nothing.
 *
 */
static void _IUT_cfgAmrNb(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgInt(&cfg_ptr->amrnb, value_ptr);
}

/*
 * ======== _IUT_cfgPAmrNb() ========
 *
 * This function sets the coder's priority field inside the config object
 * specified by cfg_ptr, to the integer specified in the NULL terminated
 * string value_ptr.
 *
 * Returns:
 *   Nothing.
 *
 */
static void _IUT_cfgPAmrNb(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgInteger(&cfg_ptr->pamrnb, value_ptr);
}

/*
 * ======== _IUT_cfgAmrWb() ========
 *
 * This function sets the "amrwb" field inside the config object
 * specified by cfg_ptr, to the integer specified in the NULL terminated
 * string value_ptr.
 *
 * Returns:
 *   Nothing.
 *
 */
static void _IUT_cfgAmrWb(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgInt(&cfg_ptr->amrwb, value_ptr);
}

/*
 * ======== _IUT_cfgPAmrWb() ========
 *
 * This function sets the coder's priority field inside the config object
 * specified by cfg_ptr, to the integer specified in the NULL terminated
 * string value_ptr.
 *
 * Returns:
 *   Nothing.
 *
 */
static void _IUT_cfgPAmrWb(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr)
{
    _IUT_cfgInteger(&cfg_ptr->pamrwb, value_ptr);
}

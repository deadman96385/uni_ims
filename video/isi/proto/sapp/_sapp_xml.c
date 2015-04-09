/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30298 $ $Date: 2014-12-09 19:05:49 +0800 (Tue, 09 Dec 2014) $
 */
#include <osal_types.h>
#include <osal.h>
#include <osal_msg.h>

#include <ezxml.h>
#include <ezxml_mem.h>

#include <sip_sip.h>
#include <sip_xport.h>
#include <sip_ua.h>
#include <sip_app.h>
#include <sip_debug.h>

#include "isi.h"
#include "isip.h"

#include "mns.h"

#ifdef INCLUDE_SIMPLE
#include "xcap.h"
#include "_simple_types.h"
#endif

#include "_sapp.h"
#include "_sapp_xml.h"
#include "_sapp_capabilities.h"

static const char _SAPP_XML_TAG_PRESENCE[]         = "presence-info";
static const char _SAPP_XML_TAG_REG_INFO[]         = "reginfo";
static const char _SAPP_XML_TAG_REGISTRATION[]     = "registration";
static const char _SAPP_XML_TAG_CONTACT[]          = "contact";
static const char _SAPP_XML_TAG_URI[]              = "uri";
static const char _SAPP_XML_ATTR_AOR[]             = "aor";
static const char _SAPP_XML_ATTR_STATE[]           = "state";
static const char _SAPP_XML_ATTR_ARG_TERMINATED[]  = "terminated";
static const char _SAPP_XML_ATTR_ARG_REJECTED[]    = "rejected";
static const char _SAPP_XML_ATTR_ARG_DEACTIVATED[] = "deactivated";
static const char _SAPP_XML_ATTR_ARG_ACTIVE[]      = "active";
static const char _SAPP_XML_ATTR_ARG_SHORTENED[]   = "shortened";
static const char _SAPP_XML_ATTR_EXPIRES[]         = "expires";
static const char _SAPP_XML_ATTR_EVENT[]           = "event";
static const char _SAPP_XML_TAG_FROM[]             = "from";
static const char _SAPP_XML_TAG_SHOW[]             = "show";
static const char _SAPP_XML_TAG_STATUS[]           = "status";
static const char _SAPP_XML_TAG_PRIORITY[]         = "priority";
static const char _SAPP_XML_TAG_CAPABILITIES[]     = "capabilities";
static const char _SAPP_XML_TAG_CAPABILITY_NAME[]  = "feature";
static const char _SAPP_XML_ATTR_CAPABILITY_NAME[] = "name";
static const char _SAPP_XML_TAG_USERS[]            = "users";
static const char _SAPP_XML_TAG_USER[]             = "user";
static const char _SAPP_XML_TAG_CONF_INFO[]        = "conference-info";
static const char _SAPP_XML_TAG_IMS_3GPP[]         = "ims-3gpp";
static const char _SAPP_XML_TAG_TYPE[]             = "type";
static const char _SAPP_XML_TAG_ALT_SERVICE[]      = "alternative-service";
static const char _SAPP_XML_TAG_ACTION[]           = "action";
static const char _SAPP_XML_TAG_REASON[]           = "reason";

const char _SAPP_XML_ATTR_ENTITY[]          = "entity";
const char _SAPP_XML_TAG_ENDPOINT[]         = "endpoint";

const char _SAPP_XML_SHOW_AVAIL[]           = "Available";
const char _SAPP_XML_SHOW_UNAVAIL[]         = "Unavailable";
const char _SAPP_XML_SHOW_DND[]             = "DND";
const char _SAPP_XML_SHOW_CHAT[]            = "Chat";
const char _SAPP_XML_SHOW_AWAY[]            = "Away";
const char _SAPP_XML_SHOW_XAWAY[]           = "Extended Away";
const char _SAPP_XML_SHOW_OTP[]             = "On The Phone";

const char _SAPP_XML_STATUS_CONNECTED[]     = "connected";
const char _SAPP_XML_STATUS_DISCONNECTED[]  = "disconnected";
const char _SAPP_XML_STATUS_ONHOLD[]        = "on-hold";
const char _SAPP_XML_STATUS_MUTEDVIAFOCUS[] = "muted-via-focus";
const char _SAPP_XML_STATUS_PENDING[]       = "pending";
const char _SAPP_XML_STATUS_ALERTING[]      = "alerting";
const char _SAPP_XML_STATUS_DIALING[]       = "dialing-in";
const char _SAPP_XML_STATUS_DISCONNECTING[] = "disconnecting";

static const char _SAPP_XML_TYPE_EMERGENCY[]        = "emergency";
static const char _SAPP_XML_TYPE_RESTORATION[]      = "restoration";
static const char _SAPP_XML_ACTION_EMERGENCY_REG[]  = "emergency-registration";
static const char _SAPP_XML_ACTION_INITIAL_REG[]    = "initial-registration";

static const char _SAPP_XML_HEADER[]             = "<?xml version='1.0' encoding='UTF-8'?>";

static const char _SAPP_XML_TAG_LIST[]           = "list";
static const char _SAPP_XML_TAG_ENTRY[]          = "entry";
static const char _SAPP_XML_TAG_RESOURCE_LISTS[] = "resource-lists";

static const char _SAPP_XML_VALUE_TO[]           = "to";
static const char _SAPP_XML_VALUE_XMLNS_RLISTS[] = "urn:ietf:params:xml:ns:resource-lists";
static const char _SAPP_XML_VALUE_XMLNS_CP[]    = "urn:ietf:params:xml:ns:copycontrol";

static const char _SAPP_XML_ATTR_XMLNS[]         = "xmlns";
static const char _SAPP_XML_ATTR_URI[]           = "uri";
static const char _SAPP_XML_ATTR_XMLNS_CP[]      = "xmlns:cp";
static const char _SAPP_XML_ATTR_COPY_CONTROL[]  = "cp:copyControl";

/*
 * Unused parameters:
 *static const char _SAPP_XML_TAG_NAME[]            = "name";        
 *static const char _SAPP_XML_TAG_RESOURCE[]        = "resource";    
 *static const char _SAPP_XML_VALUE_PRESENCE[]      = "presence";    
 *static const char _SAPP_XML_TAG_EMERGENCY[]       = "emergency";  
 */

#define _SAPP_XML_MAX_RESOURCE_LIST_SIZE (10)
#define _SAPP_XML_STRING_SZ             (128)
#define _SAPP_XML_END_OF_LINE            "\r\n"

typedef struct {
    char    entry[_SAPP_XML_MAX_RESOURCE_LIST_SIZE][_SAPP_XML_STRING_SZ];
    vint    length;
} _SAPP_rlsObj;

#if 0
<?xml version="1.0"?>
       <reginfo xmlns="urn:ietf:params:xml:ns:reginfo"
           xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
                    version="0" state="full">
         <registration aor="sip:user@example.com" id="as9"
                       state="active">
           <contact id="76" state="active" event="registered"
                    duration-registered="7322"
                    q="0.8">
                    <uri>sip:user@pc887.example.com</uri>
           </contact>
           <contact id="77" state="terminated" event="expired"
                    duration-registered="3600"
                    q="0.5">
                    <uri>sip:user@university.edu</uri>
           </contact>
         </registration>
       </reginfo>
#endif
        
static vint _SAPP_xmlDecodeRegEventContactNode(
    ezxml_t            xml_ptr,
    char              *contact_ptr,
    vint              *isUs_ptr,
    SAPP_xmlRegStatus *regStatus_ptr,
    vint              *expires_ptr
    )
{
    const char *value_ptr;
    SAPP_xmlRegStatus   regStatus = SAPP_XML_REG_STATUS_ON;
    vint                isUs = 0;
    vint                len;
    
    /* Check for the "terminated" state */
    if (NULL == (value_ptr = ezxml_attr(xml_ptr, _SAPP_XML_ATTR_STATE))) {
        /* no state so return */
        return (SAPP_ERR);
    }
    if (0 == OSAL_strncasecmp(value_ptr, _SAPP_XML_ATTR_ARG_TERMINATED,
            sizeof(_SAPP_XML_ATTR_ARG_TERMINATED) - 1)) {
        /* Then this contact entry is 'terminated' */
        regStatus = SAPP_XML_REG_STATUS_TERMINATED;
        /*
         * Let's ssee if we can get more info, like the event that triggered
         * the cancellation of the registration.
         */
        /* 
               * if the event = "unregistered" the reg status is 
               * same with SAPP_XML_REG_STATUS_TERMINATED.
               */
        if (NULL != (value_ptr = ezxml_attr(xml_ptr,
                _SAPP_XML_ATTR_EVENT))) {
            if (0 == OSAL_strncasecmp(value_ptr, _SAPP_XML_ATTR_ARG_REJECTED,
                    sizeof(_SAPP_XML_ATTR_ARG_REJECTED) - 1)) {
                /* Then this contact entry was 'rejected' */
                regStatus = SAPP_XML_REG_STATUS_REJECTED;
            }
        }
        /* if event = " deactivated" the, should restart the initial registration procedure */
        else if (NULL != (value_ptr = ezxml_attr(xml_ptr,
                _SAPP_XML_ATTR_EVENT))) {
            if (0 == OSAL_strncasecmp(value_ptr,
                    _SAPP_XML_ATTR_ARG_DEACTIVATED,
                    sizeof(_SAPP_XML_ATTR_ARG_DEACTIVATED) - 1)) {
                /* Then this contact entry was 'deactivted' */
                regStatus = SAPP_XML_REG_STATUS_DEACTIVATED;
            }
        }
    }
    else if (0 == OSAL_strncasecmp(value_ptr, _SAPP_XML_ATTR_ARG_ACTIVE,
            sizeof(_SAPP_XML_ATTR_ARG_ACTIVE) - 1)) {
        if (NULL != (value_ptr = ezxml_attr(xml_ptr,
                _SAPP_XML_ATTR_EVENT))) {
             /* check if this contact evnet entry was 'shortened' */
            if (0 == OSAL_strncasecmp(value_ptr,
                    _SAPP_XML_ATTR_ARG_SHORTENED,
                    sizeof(_SAPP_XML_ATTR_ARG_SHORTENED) - 1)) {
                 /* get the expires */
                 if (NULL != (value_ptr = ezxml_attr(xml_ptr,
                    _SAPP_XML_ATTR_EXPIRES))) {
                    *expires_ptr = atoi(value_ptr);
                    regStatus = SAPP_XML_REG_STATUS_ACTIVE_SHORTENED;
                }
            }
        }
    }
    /* Get the nested uri tag and compare to the contact_ptr parameter */
    if (NULL == (xml_ptr = ezxml_child(xml_ptr, _SAPP_XML_TAG_URI))) {
        /* not valid, return error */
        return (SAPP_ERR);
    }
    if (NULL == (value_ptr = ezxml_txt(xml_ptr))) {
        /* not valid, return error */
        return (SAPP_ERR);
    }
    /* 
     * Compare it to see if this is the one we are looking for.
     * Note, some of the contact values may not have ports in them
     * so compare only up to the shortest of the 2 contacts.
     */
    len = OSAL_strlen(value_ptr);
    if (OSAL_strlen(contact_ptr) < len) {
        len = OSAL_strlen(contact_ptr);
    }

    if (0 == OSAL_strncasecmp(value_ptr, contact_ptr, len)) {
        /* Then this contact entry pertains to this endpoint */
        isUs = 1;
    }
    
    /* Return 'ok' since we have a valid value */
    *isUs_ptr = isUs;
    *regStatus_ptr = regStatus;
    return (SAPP_OK);
}
        
/*
 * ======== SAPP_xmlDecodeRegEventDoc() ========
 * This function parses an XML document from SIP (specified in the message body) 
 * that represents the registration state of this endpoint.
 *
 * Returns:
 *  SAPP_OK: The registration state of the aor_ptr and related contact_ptr has
 *            been terminated.
 * SAPP_ERR: The registration state of the aor_ptr and related contact_ptr is 
 *            still active.
 */
vint SAPP_xmlDecodeRegEventDoc(
    char              *aor_ptr,
    char              *contact_ptr,
    char              *doc_ptr,
    vint               docLen,
    SAPP_xmlRegStatus *regStatus_ptr,
    vint              *numDev_ptr,
    vint              *expires_ptr)
{
    ezxml_t             xml_ptr;
    ezxml_t             child_ptr;
    const char         *value_ptr;
    vint                numReg;
    SAPP_xmlRegStatus   ourReg;
    vint                isUs;
    SAPP_xmlRegStatus   regStatus;
    vint                rptFlag;
    
    if (NULL == (xml_ptr = ezxml_parse_str(doc_ptr,  docLen))) {
        return (SAPP_ERR);
    }
    
    /* Check for the mandatory 'reginfo' root tag */
    if (NULL == (value_ptr = ezxml_name(xml_ptr))) {
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }
    if (0 != OSAL_strncmp(value_ptr, _SAPP_XML_TAG_REG_INFO, 
            sizeof(_SAPP_XML_TAG_REG_INFO - 1))) {
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }
    
    /* Get the mandatory "Registration" child element */
    if (NULL == (child_ptr = ezxml_child(xml_ptr,
            _SAPP_XML_TAG_REGISTRATION))) {
        /* can't find it, return error */
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }
    
    /* Verify the 'aor' attr matches the aor_ptr parameter */
    if (NULL == (value_ptr = ezxml_attr(child_ptr, _SAPP_XML_ATTR_AOR))) {
        /* This is mandatory so return error if it doesn't exist */
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }
    if (0 != OSAL_strncmp(value_ptr, aor_ptr, SAPP_STRING_SZ)) {
        /* Then this doesn't belong to us */
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }
    
    /* 
    * Loop and get all the "contact" entries and find the one that matches 
    * The "contact_ptr" parameter.
    */
    numReg = 0;
    rptFlag = 0;
    ourReg = SAPP_XML_REG_STATUS_ON;
    child_ptr = ezxml_child(child_ptr, _SAPP_XML_TAG_CONTACT);
    while (NULL != child_ptr) {
        if (SAPP_OK == _SAPP_xmlDecodeRegEventContactNode(child_ptr,
                contact_ptr, &isUs, &regStatus, expires_ptr)) {
            rptFlag = 1;
            if (SAPP_XML_REG_STATUS_ON == regStatus) {
                /*
                 * If we have a valid contact entry and it's not terminated,
                 * then we count it as a registered device.
                 */
                numReg++;
            }
            else if (1 == isUs) {
                /*
                 * Then we found our own entry and it's terminated.
                 * Indicate that there are no active registrations.
                 * Since our registration is dead then we don't care about other
                 * registrations.  So let's return OK indicating that we have status
                 * and let's indicate the status as having no valid registration.
                 */
                ourReg = regStatus;
            }
        }
        child_ptr = ezxml_next(child_ptr);
    }
    if (0 == rptFlag) {
        /* Then there is no report */
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }

    *regStatus_ptr = ourReg;
    *numDev_ptr = numReg;

    /* Free the xml object since we don't need it anymore */
    ezxml_free(xml_ptr);
    return (SAPP_OK);
}



/*
 * ======== _SAPP_xmlDecodeIsiCapabilitiesDoc() ========
 * Decodes an XML document containing device capabilities and
 * determines the bitmap and strings to represent the
 * capabilities.
 *
 * doc_ptr - pointer to the XML document
 *
 * docLen - string length of the XML document
 *
 * capsBitmap_ptr - the memory pointed to by this ptr will be
 *     filled in with a bitmap with each of the bits set
 *     corresponding to those in the XML document.
 *
 * capsString_ptr - the memory pointed to by this ptr will be
 *     filled with a string in the format specified by the
 *     RCS 5.0 specification that will indicate the capabilities
 *     specified in the XML document.
 *
 * e.g. -
 *    <capabilities>
 *        <feature name="discovery via presence"/>
 *        <feature name="ip video call"/>
 *        <feature name="sms"/>
 *    </capabilities>
 *
 * Returns:
 *  SAPP_ERR: The subscription request could not be made.
 *  SAPP_OK : The subscription request was successful.
*/
vint _SAPP_xmlDecodeIsiCapabilitiesDoc(
    char               *doc_ptr,
    vint                docLen,
    uint32             *capsBitmap_ptr)
{
    ezxml_t xml_ptr;

    /* initialize the string */
    *capsBitmap_ptr = (uint32)SAPP_CAPS_NONE;

    /*
     * parse the XML string and return a structure we can work with to
     * determine the capabilities.
     */
    if (NULL == (xml_ptr = ezxml_parse_str(doc_ptr,  docLen))) {
        SAPP_dbgPrintf("%s: failed to parse XML string\n", __FUNCTION__);
        return (SAPP_ERR);
    }

    if (SAPP_OK != SAPP_xmlDecodeIsiCapabilitiesHelper(xml_ptr, capsBitmap_ptr)) {
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }

    /* Free the xml object since we don't need it anymore */
    ezxml_free(xml_ptr);
    return (SAPP_OK);
}


/*
 * ======== SAPP_xmlDecodeIsiCapabilitiesHelper() ========
 * Decodes an XML document containing device capabilities and
 * determines the bitmap and strings to represent the
 * capabilities given the data pre-parsed into an EZ-XML type (ezxml_t).
 *
 * xml_ptr - pointer to the XML document parsed by EZ-XML
 *
 * capsBitmap_ptr - the memory pointed to by this ptr will be
 *     filled in with a bitmap with each of the bits set
 *     corresponding to those in the XML document.
 *
 * NOTE: This function neither creates nor destroys the EZ-XML data
 *       structure.
 *
 * Returns:
 *  SAPP_OK, or SAPP_ERR for failure.
*/
vint SAPP_xmlDecodeIsiCapabilitiesHelper(
    ezxml_t xml_ptr,
    uint32 *capsBitmap_ptr)
{
    ezxml_t                         child_ptr;
    const char                     *value_ptr;
    const _SAPP_CapabilitiesTableEntry   *entry_ptr;

    *capsBitmap_ptr = (uint32)SAPP_CAPS_NONE;

    /* Check for the mandatory <capabilities> root tag */
    if (NULL == (value_ptr = ezxml_name(xml_ptr))) {
        SAPP_dbgPrintf("%s: name not found\n", __FUNCTION__);
        return (SAPP_ERR);
    }
    if (0 != OSAL_strncmp(value_ptr, _SAPP_XML_TAG_CAPABILITIES,
            sizeof(_SAPP_XML_TAG_CAPABILITIES - 1))) {
        SAPP_dbgPrintf("%s: capabilities root tag not found\n", __FUNCTION__);
        return (SAPP_ERR);
    }


    /*
     * Loop and get all the <feature name="xxx"> entries and keep track of the
     * "xxx" capabilities that were provided.
     */
    child_ptr = ezxml_child(xml_ptr, _SAPP_XML_TAG_CAPABILITY_NAME);
    if (child_ptr == NULL) {
        SAPP_dbgPrintf("%s: child node <feature> not found\n", __FUNCTION__);
    }

    while (NULL != child_ptr) {
        /* determine which capability is named by this feature tag */
        if (NULL != (value_ptr = ezxml_attr(child_ptr,
                _SAPP_XML_ATTR_CAPABILITY_NAME))) {

            /*
             * obtain the _SAPP_CapabilitiesTableEntry for the
             * associated feature, which contains the bitmap value
             */
            entry_ptr = _SAPP_findCapabilityInfoByName(value_ptr);
            if (entry_ptr != NULL) {
                *capsBitmap_ptr |= entry_ptr->maskValue;
            }
            else {
                /*
                 * normally quietly ignore unhandled capabilities provided by
                 * ISI, but show a debug message if enabled.
                 */
                SAPP_dbgPrintf("Unknown capability feature provided by ISI: %s\n", value_ptr);
            }
        }
        else {
            SAPP_dbgPrintf("%s: feature name attribute not detected\n", value_ptr);
        }

        child_ptr = ezxml_next(child_ptr);
    }
    return (SAPP_OK);

}

/*
 * ======== _SAPP_xmlEncodeCapabilitiesIsiDoc() ========
 * Encodes an XML document containing device capabilities
 * given the bitmap represent the capabilities.
 *
 * capsBitmap -
 *     filled in with a bitmap with each of the bits set
 *     corresponding to the supported capabilities.
 *
 * doc_ptr -
 *     pointer to memory that will contain the resulting XML,
 *     which will be NULL terminated
 *
 * maxDocLen -
 *     size of doc_ptr buffer
 *
 * Returns:
 *  nothing
 */
void _SAPP_xmlEncodeCapabilitiesIsiDoc(
    const uint32  capsBitmap,
    const char   *status_ptr,
    char         *doc_ptr,
    vint          maxDocLen)
{
    vint        x = 0;
    uint32      maskValue = SAPP_CAPS_FIRST;
    const char *xmlAttrName;

    /* Add required opening <capabilities> tag */
    x += OSAL_snprintf(doc_ptr + x, maxDocLen, "<%s>",
            _SAPP_XML_TAG_CAPABILITIES);
    maxDocLen -= x;

    /* Set the 'Status' string. This is optional */
    if (NULL != status_ptr && status_ptr[0] != 0) {
        x += OSAL_snprintf(doc_ptr + x, maxDocLen, "<%s>%s</%s>",
                _SAPP_XML_TAG_STATUS, status_ptr,
                _SAPP_XML_TAG_STATUS);
        maxDocLen -= x;
    }

    /*
     * loop through bits in bitmap and add any optional
     * <feature name="xxx"/> tags, where "xxx" depends on
     * the particular feature, for any bits set in the bitmap
     */
    for (maskValue = SAPP_CAPS_FIRST; maskValue <= SAPP_CAPS_LAST; ) {
        if ((maskValue & capsBitmap) == maskValue) {
            /* SAPP_dbgPrintf("0x%08X - bit set!\n", maskValue); */
            /*
             * bit is set -
             * lookup associated string fragment and update structure used
             * to build the result string in proper order
             */
            xmlAttrName = _SAPP_findCapabilityXmlAttrNameByBitmask(
                    (tCapabilityBitmapType)maskValue);
            if (xmlAttrName != NULL)
            {
                x += OSAL_snprintf(doc_ptr + x, maxDocLen, "<%s %s=\"%s\"/>",
                        _SAPP_XML_TAG_CAPABILITY_NAME,
                        _SAPP_XML_ATTR_CAPABILITY_NAME,
                        xmlAttrName);
                maxDocLen -= x;
            }
        }

        maskValue = maskValue << 1;
    }

    /* Add required closing </capabilities> tag */
    x += OSAL_snprintf(doc_ptr + x, maxDocLen, "</%s>",
            _SAPP_XML_TAG_CAPABILITIES);
    maxDocLen -= x;

    return;
}




static void _SAPP_xmlEncodeIsiDoc(
    const char *from_ptr,
    const char *show_ptr,
    const char *status_ptr,
    const char *priority_ptr,
    char       *doc_ptr,
    vint        maxDocLen)
{
    vint x;

    /*
     * 'Show' strings must be one of the following:
     * "Available", "Away", "Chat", "DND", "Extended Away", "Unavailable".
     */

    x = OSAL_snprintf(doc_ptr, maxDocLen, "<%s>%s</%s>",
            _SAPP_XML_TAG_FROM, from_ptr, _SAPP_XML_TAG_FROM);
    maxDocLen -= x;

    /* Set the 'Show' state. This is mandatory */
    x += OSAL_snprintf(doc_ptr + x, maxDocLen, "<%s>%s</%s>",
            _SAPP_XML_TAG_SHOW, show_ptr,_SAPP_XML_TAG_SHOW);
    maxDocLen -= x;

    /* Set the 'Status' string. This is optional */
    if (NULL != status_ptr && status_ptr[0] != 0) {
        x += OSAL_snprintf(doc_ptr + x, maxDocLen, "<%s>%s</%s>",
                _SAPP_XML_TAG_STATUS, status_ptr,
                _SAPP_XML_TAG_STATUS);
        maxDocLen -= x;
    }

    /* Set the 'Priority' of this presence report. This is optional */
    if (NULL != priority_ptr && priority_ptr[0] != 0) {
        x += OSAL_snprintf(doc_ptr + x, maxDocLen, "<%s>%s</%s>",
                _SAPP_XML_TAG_PRIORITY, priority_ptr,
                _SAPP_XML_TAG_PRIORITY);
        maxDocLen -= x;
    }
    /* Add any more here in the future */
    return;
}

void SAPP_xmlEncodeRegEventDoc(
    const char *aor_ptr,
    const vint  status,
    char       *doc_ptr,
    vint        maxDocLen)
{
    char number[16];
    /*
     * Encode details about all the contacts registered for the
     * aor associated with endpoint
     */

    OSAL_itoa(status, number, sizeof(number));
    _SAPP_xmlEncodeIsiDoc(aor_ptr, _SAPP_XML_TAG_REG_INFO, number, NULL,
            doc_ptr, maxDocLen);
    /* Add any more here in the future */
    return;
}



static vint _SAPP_xmlDecodeUserNode(
    SAPP_ServiceObj *service_ptr,
    ISI_Id           chatId,
    const char      *from_ptr,
    ezxml_t          xml_ptr,
    ISIP_Message    *isi_ptr)
{
    const char *entity_ptr;
    const char *note_ptr;
    const char *status_ptr;
    ezxml_t child_ptr;
    

     /*
     * Note the from_ptr parameter is currently unused but may be needed in the
     * future.  WE may need to include it in the ISI event that this function
     * generates.  We void it here to avoid compiler warnings.
     */
    (void)from_ptr;

    /* Get the mandatory 'entity' attribute in the 'user' tag */
    if (NULL == (entity_ptr = ezxml_attr(xml_ptr, _SAPP_XML_ATTR_ENTITY))) {
        return (SAPP_ERR);
    }

    /* Get the 'endpoint' */
    if (NULL == (child_ptr = ezxml_child(xml_ptr, _SAPP_XML_TAG_ENDPOINT))) {
        /* Nothing, assume the dude is unavailable */
        return (SAPP_ERR);
    }

    /* Get the 'status' */
    if (NULL == (child_ptr = ezxml_child(child_ptr, _SAPP_XML_TAG_STATUS))) {
        /* Nothing, assume the dude is available */
        return (SAPP_ERR);
    }

    /* 
     * Get the value of the 'status' and store it as the 'note' as that's what
     * we will use as the note
     */
    if (NULL == (note_ptr = ezxml_txt(child_ptr))) {
        return (SAPP_ERR);
    }

    /* Figure out the dude's status from all posible values defined 
     * in RFC4575 */
    if (0 == OSAL_strncasecmp(note_ptr, _SAPP_XML_STATUS_CONNECTED,
            sizeof(_SAPP_XML_STATUS_CONNECTED) - 1)) {
        status_ptr = _SAPP_XML_SHOW_AVAIL;
    }
    else if (0 == OSAL_strncasecmp(note_ptr, _SAPP_XML_STATUS_DISCONNECTED,
            sizeof(_SAPP_XML_STATUS_DISCONNECTED) - 1)) {
        status_ptr = _SAPP_XML_SHOW_UNAVAIL;
    }
    else if (0 == OSAL_strncasecmp(note_ptr, _SAPP_XML_STATUS_ONHOLD,
            sizeof(_SAPP_XML_STATUS_ONHOLD) - 1)) {
        status_ptr = _SAPP_XML_SHOW_AVAIL;
    }
    else if (0 == OSAL_strncasecmp(note_ptr, _SAPP_XML_STATUS_MUTEDVIAFOCUS,
            sizeof(_SAPP_XML_STATUS_MUTEDVIAFOCUS) - 1)) {
        status_ptr = _SAPP_XML_SHOW_AVAIL;
    }
    else if (0 == OSAL_strncasecmp(note_ptr, _SAPP_XML_STATUS_PENDING,
            sizeof(_SAPP_XML_STATUS_PENDING) - 1)) {
        status_ptr = _SAPP_XML_SHOW_UNAVAIL;
    }
    else if (0 == OSAL_strncasecmp(note_ptr, _SAPP_XML_STATUS_ALERTING,
            sizeof(_SAPP_XML_STATUS_ALERTING) - 1)) {
        status_ptr = _SAPP_XML_SHOW_UNAVAIL;
    }
    else if (0 == OSAL_strncasecmp(note_ptr, _SAPP_XML_STATUS_DIALING,
            sizeof(_SAPP_XML_STATUS_DIALING) - 1)) {
        status_ptr = _SAPP_XML_SHOW_UNAVAIL;
    }
    else if (0 == OSAL_strncasecmp(note_ptr, _SAPP_XML_STATUS_DISCONNECTING,
            sizeof(_SAPP_XML_STATUS_DISCONNECTING) - 1)) {
        status_ptr = _SAPP_XML_SHOW_UNAVAIL;
    }
    else {
        note_ptr = _SAPP_XML_STATUS_DISCONNECTED;
        status_ptr = _SAPP_XML_STATUS_DISCONNECTED;
    }

    /* Start to populate the ISI event. */
    isi_ptr->id = 0;
    isi_ptr->code = ISIP_CODE_PRESENCE;
    isi_ptr->protocol = service_ptr->protocolId;
    isi_ptr->msg.presence.reason = ISIP_PRES_REASON_PRESENCE;
    isi_ptr->msg.presence.serviceId = service_ptr->isiServiceId;
    OSAL_snprintf(isi_ptr->msg.presence.to, ISI_ADDRESS_STRING_SZ, "%s",
           entity_ptr);
    OSAL_snprintf(isi_ptr->msg.presence.from, ISI_ADDRESS_STRING_SZ, "%s",
            entity_ptr);

    /* For presence related to chat conference's let's set the chatId */
    isi_ptr->msg.presence.chatId = chatId;
    if (0 < isi_ptr->msg.presence.chatId) {
        OSAL_snprintf(isi_ptr->msg.presence.reasonDesc, 
                ISI_EVENT_DESC_STRING_SZ, "ChatId:%d",
                isi_ptr->msg.presence.chatId);
    }

    SAPP_xmlEncodePresenceDoc(entity_ptr, status_ptr, note_ptr,
            NULL, isi_ptr->msg.presence.presence, ISI_PRESENCE_STRING_SZ);

    return (SAPP_OK);
}

/*
 * ======== SAPP_xmlDecodeConferenceInfoDoc() ========
 * This routine will process the message bodies of SIP requests or responses
 * related to the conference event package RFC4575. ISI events are typically
 * generated from calling this routine.
 *
 * Returns:
 *  Nothing.
 */
vint SAPP_xmlDecodeConferenceInfoDoc(
    ISI_Id           isiCallId,
    SAPP_ServiceObj *service_ptr,
    const char      *from_ptr,
    char            *contentType_ptr,
    char            *doc_ptr,
    vint             docLen,
    SAPP_Event      *evt_ptr)
{
    ezxml_t     xml_ptr;
    ezxml_t     user_ptr;
    ezxml_t     next_ptr;
    const char *tag_ptr;


    /* Check if it's "application/conference-info+xml" only */
    if (0 != OSAL_strncasecmp(contentType_ptr, _SAPP_CONF_EVENT_ARG,
            sizeof(_SAPP_CONF_EVENT_ARG) - 1)) {
        return (SAPP_ERR);
    }
   
    if (NULL == (xml_ptr = ezxml_parse_str(doc_ptr,  docLen))) {
        return (SAPP_ERR);
    }

    /* Check for the mandatory '<conference-info>' tag */
    if (NULL == (tag_ptr = ezxml_name(xml_ptr))) {
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }
    if (0 != OSAL_strncmp(tag_ptr, _SAPP_XML_TAG_CONF_INFO,
            sizeof(_SAPP_XML_TAG_CONF_INFO - 1))) {
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }

    /* Get the <users> tag */
    if (NULL == (user_ptr = ezxml_child(xml_ptr, _SAPP_XML_TAG_USERS))) {
        /* Nothing to really process..!!?? */
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }

    /* Ge the first <user> tag */
    if (NULL == (user_ptr = ezxml_child(user_ptr, _SAPP_XML_TAG_USER))) {
        /* Nothing to really process..!!?? */
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }

    /* Loop and get all the "user" entries */
    next_ptr = user_ptr;
    while (NULL != next_ptr) {
        if (SAPP_OK == _SAPP_xmlDecodeUserNode(
                service_ptr,
                isiCallId,
                from_ptr,
                next_ptr,
                &evt_ptr->isiMsg)) {
            /* Send the event to ISI */
            SAPP_sendEvent(evt_ptr);
        }
        /* Reset the ISI event object */
        evt_ptr->isiMsg.code = ISIP_CODE_INVALID;
        next_ptr = ezxml_next(next_ptr);
    }

    /* Free the xml object since we don't need it anymore */
    ezxml_free(xml_ptr);

    return (SAPP_OK);
}

/*
 * ======== SAPP_xmlEncodePresenceDoc() ========
 * This function prepares a presence string destined for ISI and the application
 * that implements ISI.  The end result is written to 'doc_ptr'.
 *
 * Returns:
 *  Nothing
 */
void SAPP_xmlEncodePresenceDoc(
    const char *from_ptr,
    const char *show_ptr,
    const char *status_ptr,
    const char *priority_ptr,
    char       *doc_ptr,
    vint        maxDocLen)
{
    vint x;

    /* 
     * 'Show' strings must be one of the following:
     * "Available", "Away", "Chat", "DND", "On The Phone", "Extended Away", 
     * "Unavailable".
     */
    
    /* Let's first add an informative header tag.  */
    x = OSAL_snprintf(doc_ptr, maxDocLen, "<%s>", _SAPP_XML_TAG_PRESENCE);
    doc_ptr += x;
    maxDocLen -= x;

    x = OSAL_snprintf(doc_ptr, maxDocLen, "<%s>%s</%s>",
            _SAPP_XML_TAG_FROM, from_ptr, _SAPP_XML_TAG_FROM);
    doc_ptr += x;
    maxDocLen -= x;

    /* Set the 'Show' state. This is mandatory */
    x = OSAL_snprintf(doc_ptr, maxDocLen, "<%s>%s</%s>",
            _SAPP_XML_TAG_SHOW, show_ptr,_SAPP_XML_TAG_SHOW);
    doc_ptr += x;
    maxDocLen -= x;
    
    /* Set the 'Status' string. This is optional */
    if (NULL != status_ptr && status_ptr[0] != 0) {
        x = OSAL_snprintf(doc_ptr, maxDocLen, "<%s>%s</%s>",
                _SAPP_XML_TAG_STATUS, status_ptr,
                _SAPP_XML_TAG_STATUS);
        doc_ptr += x;
        maxDocLen -= x;
    }
    
    /* Set the 'Priority' of this presence report. This is optional */
    if (NULL != priority_ptr && priority_ptr[0] != 0) {
        x = OSAL_snprintf(doc_ptr, maxDocLen, "<%s>%s</%s>",
                _SAPP_XML_TAG_PRIORITY, priority_ptr,
                _SAPP_XML_TAG_PRIORITY);
        doc_ptr += x;
        maxDocLen -= x;
    }

    /* End the informative header 'presence' tag. */
    x = OSAL_snprintf(doc_ptr, maxDocLen, "</%s>", _SAPP_XML_TAG_PRESENCE);
    doc_ptr += x;
    maxDocLen -= x;

    /* Add any more here in the future */
    return;
}

/*
 * ======== _SAPP_xmlDecode3gppImsDoc() ========
 * Decodes an XML document containing 3GPP IMS xml document
 * which specified in 3GPP TS 24.229 Section 7.6.
 * Currently it decodes if emergency registration is required.
 *
 * doc_ptr - pointer to the XML document
 *
 * docLen - string length of the XML document
 *
 * reason_ptr - pointer to a char pointer for filling reason string from the
 *     xml doc.
 *
 * action - pointer to the 3gpp Ims action if required.
 *
 * e.g. -
 *    <ims-3gpp>
 *        <alternative-service>
 *            <reason>Here is the reason.</reason>
 *            <type>emergency</type>
 *            <action>emergency-registration</action>
 *        </alternative-service>
 *    </ims-3gpp>
 *
 * Returns:
 *  SAPP_ERR: The subscription request could not be made.
 *  SAPP_OK : The subscription request was successful.
*/
vint _SAPP_xmlDecode3gppImsDoc(
    char                *doc_ptr,
    vint                 docLen,
    char               **reason_ptr,
    SAPP_3gppImsAction  *action)
{
    ezxml_t  xml_ptr;
    char    *value_ptr;

    /* initialize the string */
    *reason_ptr = NULL;
    *action     = SAPP_XML_ACTION_NONE;

    /*
     * parse the XML string and return a structure we can work with to
     * determine the capabilities.
     */
    if (NULL == (xml_ptr = ezxml_parse_str(doc_ptr,  docLen))) {
        SAPP_dbgPrintf("%s: failed to parse XML string\n", __FUNCTION__);
        return (SAPP_ERR);
    }

    /* Get the alternative service text from XML */
    if (NULL == (value_ptr = SAPP_getXmlNestedTagText(xml_ptr,
            _SAPP_XML_TAG_ALT_SERVICE,
            _SAPP_XML_TAG_TYPE))) {
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }
    
    /* Check the type if it is emergency or restoration. */
    if ((0 != OSAL_strncmp(value_ptr, _SAPP_XML_TYPE_EMERGENCY,
                sizeof(_SAPP_XML_TYPE_EMERGENCY) - 1)) && 
                (0 != OSAL_strncmp(value_ptr, _SAPP_XML_TYPE_RESTORATION,
                sizeof(_SAPP_XML_TYPE_RESTORATION) - 1))) {
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }

    /* Get the action text from XML */
    if (NULL == (value_ptr = SAPP_getXmlNestedTagText(xml_ptr,
            _SAPP_XML_TAG_ALT_SERVICE,
            _SAPP_XML_TAG_ACTION))) {
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }

    /* Check the action if it is emergency-reg or initial-reg. */
    if (0 == OSAL_strncmp(value_ptr, _SAPP_XML_ACTION_EMERGENCY_REG,
            sizeof(_SAPP_XML_ACTION_EMERGENCY_REG) - 1)) {
        *action = SAPP_XML_ACTION_EMERGENCY_REG;
    }
    else if (0 == OSAL_strncmp(value_ptr, _SAPP_XML_ACTION_INITIAL_REG,
            sizeof(_SAPP_XML_ACTION_INITIAL_REG) - 1)) {
        *action = SAPP_XML_ACTION_INITIAL_REG;
    }
    else {
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }

    /* Get the alternative service text from XML */
    if (NULL != (value_ptr = SAPP_getXml2NestedTagText(xml_ptr,
            _SAPP_XML_TAG_IMS_3GPP, _SAPP_XML_TAG_ALT_SERVICE,
            _SAPP_XML_TAG_REASON))) {
        *reason_ptr = value_ptr;
    }

    /* Free the xml object since we don't need it anymore */
    ezxml_free(xml_ptr);
    return (SAPP_OK);
}


/*
 * ======== _SAPP_xmlDecode3gppCwiHelper() ========
 * Decodes an XML document containing 3GPP CWI indication according to
 * 3GPP TS 24.615 Section 4.4.1.
 *
 * Returns:
 *  SAPP_ERR: The parsing is not done or failed, e.g. no such xml or invalid format
 *  SAPP_OK : The parsing is successful and hasCwiAltSrv
*/
vint _SAPP_xmlDecode3gppCwiHelper(
    char         *doc_ptr,
    vint          docLen,
    OSAL_Boolean *hasCwiAltSrv)
{
    /* XXX urn:3gpp:ns:cw:1.0 */
    *hasCwiAltSrv = OSAL_FALSE;
    return (SAPP_ERR);
}

static void _SAPP_xmlLoadInvitationList(
    char            *participants_ptr,
    _SAPP_rlsObj    *rls_ptr)
{
    vint x = 0;
    char *c_ptr;

    /* Add entries */
    while ((NULL != (c_ptr = OSAL_strchr(participants_ptr, ','))) && 
            (x < _SAPP_XML_MAX_RESOURCE_LIST_SIZE)) {
        *c_ptr = 0;
        OSAL_strcpy(rls_ptr->entry[x], participants_ptr);
        participants_ptr = c_ptr + 1;
        x++;
    }
    if (x < _SAPP_XML_MAX_RESOURCE_LIST_SIZE) {
        /* Still more for the last one */
        OSAL_strcpy(rls_ptr->entry[x], participants_ptr);
        x++;
    }
    rls_ptr->length = x;
    return;
}

/*
 * ======== _SAPP_xmlEncodeRcRlsDoc() ========
 * This function prepares an XML document used in the SIP community
 * to convey a contact list within a SUBSCRIBE request
 * (a.k.a. "Request-Contained Resource Lists").
 *
 * Returns:
 *  The bytes written to doc_ptr.  If the bytes returned are more
 *  than the size of the doc buffer provided in maxDocLen then that means that
 *  The buffer was not big enough to accomidate the request.  However
 *  up to 'maxDocLen' bytes will be written into doc_ptr.
 */


/*
 * <?xml version="1.0" encoding="UTF-8"?>
 * <resource-lists 
 *      xmlns="urn:ietf:params:xml:ns:resource-lists" 
 *      xmlns:cp="urn:ietf:params:xml:ns:copyControl">
 * <list>
 * <entry uri="01080801954" cp:copyControl="to" />
 * </list>
 * </resource-lists>
*/
vint _SAPP_xmlEncodeRcRlsDoc(
    char    *participants_ptr,
    char    *doc_ptr,
    vint     maxDocLen)
{
    char           *str_ptr;
    ezxml_t         xml_ptr;
    ezxml_t         l_ptr;
    ezxml_t         c_ptr;
    vint            x;
    _SAPP_rlsObj    rlsList;

    _SAPP_xmlLoadInvitationList(participants_ptr, &rlsList);

    xml_ptr = ezxml_new(_SAPP_XML_TAG_RESOURCE_LISTS);
    /* The 'name space' MUST exist */
    ezxml_set_attr(xml_ptr, _SAPP_XML_ATTR_XMLNS,
            _SAPP_XML_VALUE_XMLNS_RLISTS);
    ezxml_set_attr(xml_ptr, _SAPP_XML_ATTR_XMLNS_CP,
            _SAPP_XML_VALUE_XMLNS_CP);

    /* Add the List tag */
    l_ptr = ezxml_add_child(xml_ptr, _SAPP_XML_TAG_LIST, 0);

    /* Add participients */
    for (x = 0 ; x < rlsList.length ; x++) {
        if (0 != rlsList.entry[x][0]) {
            /* Then add the contact */
            c_ptr = ezxml_add_child(l_ptr, _SAPP_XML_TAG_ENTRY, 0);
            if (NULL != c_ptr) {
                ezxml_set_attr(c_ptr, _SAPP_XML_ATTR_URI,
                        rlsList.entry[x]);

                /* add cp:copyControl="to" */
                ezxml_set_attr(c_ptr, _SAPP_XML_ATTR_COPY_CONTROL,
                               _SAPP_XML_VALUE_TO);
            }
        }
    }

    str_ptr = ezxml_toxml(xml_ptr);
    /* Always free it here, even if it couldn't be constructed */
    ezxml_free(xml_ptr);
    if (str_ptr == NULL) {
        return (0);
    }

    /* Copy to the target */
    x = OSAL_snprintf(doc_ptr, maxDocLen, "%s%s%s", _SAPP_XML_HEADER,
            _SAPP_XML_END_OF_LINE, str_ptr);
    EZXML_memFree(str_ptr, OSAL_MEM_ARG_DYNAMIC_ALLOC);
    return (x);
}

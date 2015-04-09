#include <osal_types.h>
#include <osal.h>
#include <osal_msg.h>

#include <ezxml.h>


#include "_isi_port.h"
#include "isi.h"
#include "isip.h"
#include "_isi_db.h"
#include "_isi_queue.h"
#include "_isi_msg.h"
#include "_isi_service.h"
#include "_isi_call.h"
#include "_isi_gchat.h"
#include "_isi_text.h"
#include "_isi_file.h"
#include "_isi_evt.h"
#include "_isi_conf.h"
#include "_isi_pres.h"
#include "_isi_sys.h"
#include "_isi_media.h"
#include "_isi_dbg.h"
#include "_isi_diag.h"
#include "_isi_ussd.h"

#include "_isi_xml.h"

/*
 * ======== ISI_decodeAudioSessionAttributes() ========
 * Decodes XML tags and attributes specified for "audio" Media. *
 *
 * xml_ptr  : EZ-XML structure Pointer to the "audio" tag.
 *
 * call_ptr : The ISI Call pointer.
 *
 * NOTE: This function neither creates nor destroys the EZ-XML data
 *       structure.
 */
static void ISI_decodeAudioSessionAttributes(ezxml_t child_ptr,
        ISID_CallId *call_ptr) {
    const char *value_ptr;

    /* Fetch the "enabled" attribute value .*/
    value_ptr = ezxml_attr(child_ptr, _ISI_XML_ATTR_ENABLED);
    if (NULL != value_ptr) {
        /* Check if audio is enabled. */
        if (0 == OSAL_strncmp(value_ptr, _ISI_XML_VALUE_TRUE,
                        sizeof(_ISI_XML_VALUE_TRUE))) {
            /* Audio is enabled. Set the audio bit in session type bit mask. */
            call_ptr->type |= ISI_SESSION_TYPE_AUDIO;
        }
        else {
            /* 
             * Audio is disabled. Clear the audio bit in 
             * session type bit mask.
             */
            call_ptr->type &= ~ISI_SESSION_TYPE_AUDIO;
        }
    }

    /* Fetch the "direction" attribute value. */
    value_ptr = ezxml_attr(child_ptr, _ISI_XML_ATTR_DIRECTION);
    if (NULL != value_ptr) {
        if (0 == OSAL_strncmp(value_ptr, _ISI_XML_VALUE_SENDRECV,
                sizeof(_ISI_XML_VALUE_SENDRECV))) {
            /* Direction is SEND RECV. */
            call_ptr->audioDir = ISI_SESSION_DIR_SEND_RECV;
        }
        else if (0 == OSAL_strncmp(value_ptr, _ISI_XML_VALUE_SENDONLY,
                sizeof(_ISI_XML_VALUE_SENDONLY))) {
            /* Direction is SEND ONLY. */
            call_ptr->audioDir = ISI_SESSION_DIR_SEND_ONLY;
        }
        else if (0 == OSAL_strncmp(value_ptr, _ISI_XML_VALUE_RECVONLY,
                sizeof(_ISI_XML_VALUE_RECVONLY))) {
            /* Direction is RECV ONLY. */
            call_ptr->audioDir = ISI_SESSION_DIR_RECV_ONLY;
        }
        else if (0 == OSAL_strncmp(value_ptr, _ISI_XML_VALUE_INACTIVE,
                sizeof(_ISI_XML_VALUE_INACTIVE))) {
            /* Direction is INACTIVE. */
            call_ptr->audioDir = ISI_SESSION_DIR_INACTIVE;
        }
        else {
            /* Invalid Direction. */
            OSAL_logMsg("Audio media invalid direction \n");
        }
    }

    /* Fetch the "secure" attribute value. */
    value_ptr = ezxml_attr(child_ptr, _ISI_XML_ATTR_SECURE);
    if (NULL != value_ptr) {
        /* Check if audio should be secured. */
        if (0 == OSAL_strncmp(value_ptr, _ISI_XML_VALUE_TRUE,
                        sizeof(_ISI_XML_VALUE_TRUE))) {
            /* Set the secure audio bit in session type bit mask. */
            call_ptr->type |= ISI_SESSION_TYPE_SECURITY_AUDIO;
        }
        else {
            /* Clear the secure audio bit in session type bit mask. */
            call_ptr->type &= ~ISI_SESSION_TYPE_SECURITY_AUDIO;
        }
    }

    /* Fetch the "emergency" attribute value. */
    value_ptr = ezxml_attr(child_ptr, _ISI_XML_ATTR_EMERGENCY);
    if (NULL != value_ptr) {
        /* Check if its emergency. */
        if (0 == OSAL_strncmp(value_ptr, _ISI_XML_VALUE_TRUE,
                        sizeof(_ISI_XML_VALUE_TRUE))) {
            /* Set the emergency bit in session type bit mask. */
            call_ptr->type |= ISI_SESSION_TYPE_EMERGENCY;
        }
        else {
            /* Clear the emergency bit in session type bit mask. */
            call_ptr->type &= ~ISI_SESSION_TYPE_EMERGENCY;
        }
    }
}

/*
 * ======== ISI_decodeVideoSessionAttributes() ========
 * Decodes XML tags and attributes specified for "video" Media. *
 *
 * xml_ptr  : EZ-XML structure Pointer to the "video" tag.
 *
 * call_ptr : The ISI Call pointer.
 *
 * NOTE: This function neither creates nor destroys the EZ-XML data
 *       structure.
 */
static void ISI_decodeVideoSessionAttributes(ezxml_t child_ptr,
        ISID_CallId *call_ptr) {
    const char *value_ptr;

    /* Fetch the "enabled" attribute value. */
    value_ptr = ezxml_attr(child_ptr, _ISI_XML_ATTR_ENABLED);
    if (NULL != value_ptr) {
        /* Check if video is enabled. */
        if (0 == OSAL_strncmp(value_ptr, _ISI_XML_VALUE_TRUE,
                sizeof(_ISI_XML_VALUE_TRUE))) {
            /* Video is enabled. Set the audio bit in session type bit mask. */
            call_ptr->type |= ISI_SESSION_TYPE_VIDEO;
        }
        else {
            /* 
             * Video is disabled. Clear the audio bit in session
             * type bit mask. 
             */
            call_ptr->type &= ~ISI_SESSION_TYPE_VIDEO;
        }
    }

    /* Fetch the "direction" attribute value. */
    value_ptr = ezxml_attr(child_ptr, _ISI_XML_ATTR_DIRECTION);
    if (NULL != value_ptr) {
        if (0 == OSAL_strncmp(value_ptr, _ISI_XML_VALUE_SENDRECV,
                sizeof(_ISI_XML_VALUE_SENDRECV))) {
            /* Direction is SEND RECV. */
            call_ptr->videoDir = ISI_SESSION_DIR_SEND_RECV;
        }
        else if (0 == OSAL_strncmp(value_ptr, _ISI_XML_VALUE_SENDONLY,
                sizeof(_ISI_XML_VALUE_SENDONLY))) {
            /* Direction is SEND ONLY. */
            call_ptr->videoDir = ISI_SESSION_DIR_SEND_ONLY;
        }
        else if (0 == OSAL_strncmp(value_ptr, _ISI_XML_VALUE_RECVONLY,
                sizeof(_ISI_XML_VALUE_RECVONLY))) {
            /* Direction is RECV ONLY. */
            call_ptr->videoDir = ISI_SESSION_DIR_RECV_ONLY;
        }
        else if (0 == OSAL_strncmp(value_ptr, _ISI_XML_VALUE_INACTIVE,
                sizeof(_ISI_XML_VALUE_INACTIVE))) {
            /* Direction is INACTIVE. */
            call_ptr->videoDir = ISI_SESSION_DIR_INACTIVE;
        }
        else {
            /* Invalid Direction. */
            OSAL_logMsg("Video media invalid direction \n");
        }
    }

    /* Fetch the "secure" attribute value. */
    value_ptr = ezxml_attr(child_ptr, _ISI_XML_ATTR_SECURE);
    if (NULL != value_ptr) {
        /* Check if video should be secured. */
        if (0 == OSAL_strncmp(value_ptr, _ISI_XML_VALUE_TRUE,
                sizeof(_ISI_XML_VALUE_TRUE))) {
            /* Set the secure video bit in session type bit mask. */
            call_ptr->type |= ISI_SESSION_TYPE_SECURITY_VIDEO;
        }
        else {
            /* Clear the secure video bit in session type bit mask. */
            call_ptr->type &= ~ISI_SESSION_TYPE_SECURITY_VIDEO;
        }
    }

    /* Fetch the "emergency" attribute value. */
    value_ptr = ezxml_attr(child_ptr, _ISI_XML_ATTR_EMERGENCY);
    if (NULL != value_ptr) {
        /* Check if its emergency. */
        if (0 == OSAL_strncmp(value_ptr, _ISI_XML_VALUE_TRUE,
                        sizeof(_ISI_XML_VALUE_TRUE))) {
            /* Set the emergency bit in session type bit mask. */
            call_ptr->type |= ISI_SESSION_TYPE_EMERGENCY;
        }
        else {
            /* Clear the emergency bit in session type bit mask. */
            call_ptr->type &= ~ISI_SESSION_TYPE_EMERGENCY;
        }
    }

    /* Fetch the "maxBandwidth" attribute value for video. */
    value_ptr = ezxml_attr(child_ptr, _ISI_XML_ATTR_MAXBANDWIDTH);
    if (NULL != value_ptr) {
        call_ptr->rtpVideoLcl.videoAsBwKbps = OSAL_atoi(value_ptr);
    }
    else {
        /* Video maxBandwidth not set. Use default 1024 kbps. */
        call_ptr->rtpVideoLcl.videoAsBwKbps = 1024;
    }
}

/*
 * ======== ISI_decodeCallSessionAttributeXMLDocHelper() ========
 * Decodes an XML document containing session attributes and
 * determines the session Type to represent the session attributes given the
 * data pre-parsed into an EZ-XML type (ezxml_t).
 *
 * xml_ptr  : Pointer to the XML document to be parsed by EZ-XML.
 * call_ptr : The ISI Call pointer.
 *
 * NOTE: This function neither creates nor destroys the EZ-XML data
 *       structure.
 */
static ISI_Return ISI_decodeCallSessionAttributeXMLDocHelper(ezxml_t xml_ptr,
        ISID_CallId *call_ptr) {

    ezxml_t child_ptr;

    /* Check for the mandatory <media> root tag. */
    if (NULL == ezxml_name(xml_ptr)) {
        ISIG_log((char *)__FUNCTION__, "Failed to parse \n",
                0, 0 ,0);
        return (ISI_RETURN_FAILED);
    }

    /* Check for child tag "audio" and set the session type. */
    child_ptr = ezxml_child(xml_ptr, _ISI_XML_TAG_AUDIO);
    if (child_ptr != NULL) {
        /* Found audio media. Decode the attributes in audio media session.*/
        ISI_decodeAudioSessionAttributes(child_ptr, call_ptr);
    }

    /* Check for child tag "video" and set the session type. */
    child_ptr = ezxml_child(xml_ptr, _ISI_XML_TAG_VIDEO);
    if (child_ptr != NULL) {
        /* Found video media. Decode the attributes in video media session.*/
        ISI_decodeVideoSessionAttributes(child_ptr, call_ptr);
    }

    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_decodeMediaAttributeXMLDoc() ========
 * This function is a helper that is used to update the ISI call for
 * adding or removing video media, change the direction of a particular media
 * flow.
 *
 * doc_ptr : A null terminated string in xml format. 
 *           This string represents the changes to be done at the media level
 *           to the call session.
 *          Please refer to _isi_xml.h for more details on various tags and 
 *          attributes used.
 * e.g -
 * <media>
 *     <audio enabled="true"
 *         direction="sendrecv"
 *         secure="false"
 *         emergency="false"
 *         useRtpAVPF="false"
 *         maxBandwidth="0"/>
 *     <video enabled="true"
 *         direction="sendrecv"
 *         secure="false"
 *         emergency="false"
 *         useRtpAVPF="true"
 *         maxBandwidth="1024"/>
 * </media>
 *
 * call_ptr : Pointer to the ISI Call object.
 *
 * Return Values:
 * ISI_RETURN_OK         : Success.
 * ISI_RETURN_FAILED     : Failed to parse the xml string.
 */
ISI_Return ISI_decodeMediaAttributeXMLDoc(char *doc_ptr,
        ISID_CallId *call_ptr) {

    ezxml_t xml_ptr;
    ISI_Return ret;
    vint docLen;

    /* Find the length of the call session XML document. */
    docLen = OSAL_strlen(doc_ptr);

    /*
     * Parse the XML string and return a structure we can work with to
     * determine the session Attributes.
     */
    if (NULL == (xml_ptr = ezxml_parse_str(doc_ptr, docLen))) {
        ISIG_log((char *)__FUNCTION__, "Failed to parse \n",
                0, 0 ,0);
        return (ISI_RETURN_FAILED);
    }

    /*
     * Use ISI_decodeCallSessionAttributeXMLDocHelper method as a helper
     * to set session type and session direction.
     */
    ret = ISI_decodeCallSessionAttributeXMLDocHelper(xml_ptr, call_ptr);

    if (ret != ISI_RETURN_OK) {
        ISIG_log((char *)__FUNCTION__, "Failed to parse \n",
                0, 0 ,0);

        /* 
         * Free the memory allocated for the ezxml structure.
         * we don't need it anymore.
         */
        ezxml_free(xml_ptr);
        return ret;
    }

    /* 
     * Free the memory allocated for the ezxml structure.  
     * we don't need it anymore. 
     */
    ezxml_free(xml_ptr);
    return (ISI_RETURN_OK);
}

/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 14395 $ $Date: 2011-04-01 19:23:22 -0500 (Fri, 01 Apr 2011) $
 */
#include <osal_types.h>
#include <osal.h>
#include <osal_msg.h>

#include <ezxml.h>

#include <sip_sip.h>
#include <sip_xport.h>
#include <sip_ua.h>
#include <sip_app.h>
#include <sip_debug.h>
#include <sip_sdp_msg.h>

#include "isi.h"
#include "isip.h"

#include "mns.h"

#include "_sapp.h"
#include "_sapp_dialog.h"
#include "_sapp_reg.h"
#include "_sapp_mwi.h"
#include "_sapp_te.h"
#include "_sapp_im_page.h"
#include "_sapp_call_settings.h"

static SAPP_IntExt _SAPP_ContentTypes[SAPP_CONTENT_LAST] = {
    { SAPP_CONTENT_NONE,        ""                },
    { SAPP_CONTENT_TEXT_PLAIN,  "text/plain"      },
    { SAPP_CONTENT_PHOTO_JPEG,  "image/jpeg"      },
    { SAPP_CONTENT_PHOTO_GIF,   "image/gif"       },
    { SAPP_CONTENT_PHOTO_BMP,   "image/bmp"       },
    { SAPP_CONTENT_PHOTO_PNG,   "image/png"       },
    { SAPP_CONTENT_VIDEO_3GP,   "video/3gp"       },
    { SAPP_CONTENT_VIDEO_MP4,   "video/mp4"       },
    { SAPP_CONTENT_VIDEO_WMV,   "video/wmv"       },
    { SAPP_CONTENT_VIDEO_AVC,   "video/avc"       },
    { SAPP_CONTENT_VIDEO_MPEG,  "video/mpeg"      },
    { SAPP_CONTENT_AUDIO_AMR,   "audio/amr"       },
    { SAPP_CONTENT_AUDIO_AAC,   "audio/aac"       },
    { SAPP_CONTENT_AUDIO_MP3,   "audio/mpeg"      },
    { SAPP_CONTENT_AUDIO_WMA,   "audio/wma"       },
    { SAPP_CONTENT_AUDIO_M4A,   "audio/mp4"       },
    { SAPP_CONTENT_AUDIO_3GPP,  "audio/3gpp"      },
    { SAPP_CONTENT_MSG_CPIM,    "Message/CPIM"    },
    { SAPP_CONTENT_MULTI_PART,  "multipart/mixed" },
    { SAPP_CONTENT_COMPOSING,   "application/im-iscomposing+xml" },
    { SAPP_CONTENT_SMS_3GPP,    "application/vnd.3gpp.sms" },
    { SAPP_CONTENT_SMS_3GPP2,   "application/vnd.3gpp2.sms" },
};

SAPP_ContentType SAPP_parseIntFileType(const char *type_ptr, vint len)
{
    vint x;
   
    for (x = 0 ; x < SAPP_CONTENT_LAST ; x++) {
        if (NULL != OSAL_strncasescan(type_ptr, len, _SAPP_ContentTypes[x].ext_ptr)) {
            return (SAPP_ContentType)x;
        }
    }
    return (SAPP_CONTENT_NONE);
}

const char* SAPP_parseExtFileType(SAPP_ContentType type)
{
    if (SAPP_CONTENT_LAST <= type) {
        type = SAPP_CONTENT_NONE;
    }
    return (_SAPP_ContentTypes[type].ext_ptr);
}

/*
 * lookup table to convert between the string and integer representation
 * of the file disposition options.
 */
static SAPP_IntExt _SAPP_FileDispositionValues[SAPP_FILE_ATTR_LAST] = {
    { SAPP_FILE_ATTR_NONE,        ""                },
    { SAPP_FILE_ATTR_RENDER,      "render"          },
    { SAPP_FILE_ATTR_ATTACHMENT,  "attachment"      },
};


/*
 * ======== SAPP_parseFileDisposition() ========
 * Given a SAPP_FileAttribute type, returns a pointer to a string representing
 * that attribute.
 */
const char* SAPP_parseFileDisposition(SAPP_FileAttribute attribute)
{
    if (SAPP_FILE_ATTR_LAST <= attribute) {
        attribute = SAPP_FILE_ATTR_NONE;
    }
    return (_SAPP_FileDispositionValues[attribute].ext_ptr);
}

/*
 * ======== SAPP_parseFileAttribute() ========
 * Given a string and it's length, returns the associated SAPP_FileAttribute
 * type.
 */
SAPP_FileAttribute SAPP_parseFileAttribute(const char *disposition, vint len)
{
    vint x;

    for (x = 0 ; x < SAPP_FILE_ATTR_LAST ; x++) {
        if (NULL != OSAL_strncasescan(disposition, len, _SAPP_FileDispositionValues[x].ext_ptr)) {
            return (SAPP_FileAttribute)x;
        }
    }
    return (SAPP_FILE_ATTR_NONE);
}

/*
 * ======== SAPP_stripDelimters() ========
 * This function name is spelled incorrectly and could be renamed as
 * SAPP_stripDelimiters()
 * This function is used to update a pointer and size of a string
 * with new values without any "delimiters".  For example...
 * if value_ptr is '<sip:steve@d2tech.com>' and you want to strip
 * '<' and '>' then this routine would return a value_ptr pointing to 
 * sip:steve@d2tech.com and the size would be 20.
 *
 * Return Values:
 *  SAPP_OK: A delimiter was found. value_ptr and size_ptr were updated
 *  SAPP_ERR: No change.  value_ptr and size_ptr are unchanged.
 *
 */
vint SAPP_parseStripDelimters(
    char **value_ptr,
    vint  *size_ptr,
    char   begin,
    char   end)
{
    char *pos_ptr = *value_ptr;
    vint size = *size_ptr;
    char *search_ptr;
    vint  x;

    /* Get the first delimiter */
    search_ptr = pos_ptr;
    for (x = 0 ; x < size ; x++, search_ptr++) {
        if (begin == *search_ptr) {
            search_ptr++;
            size -= (search_ptr - pos_ptr);
            /* Update the position to one past the first delimiter */
            pos_ptr = search_ptr;
            break;
        }
    }

    /* Let's look for the last delimiter if the first one was found */
    if (pos_ptr != *value_ptr) {
        search_ptr = pos_ptr;
        for (x = 0 ; x < size ; x++, search_ptr++) {
            if (end == *search_ptr) {
                size = x;
                break;
            }
        }
        *value_ptr = pos_ptr;
        *size_ptr = size;
        return (SAPP_OK);
    }
    return (SAPP_ERR);
}


/*
 * ======== SAPP_getPayloadValue() ========
 * This function is used to search for parameters with the payload of
 * SIP events.
 *
 * Return Values:
 *  SAPP_OK: The value has been found and is pointed to by value_ptr.
 *  SAPP_ERR: The parameter specified in pName_ptr could not be found.
 *
 */
vint SAPP_parsePayloadValue(
    char         *payload_ptr,
    const char   *pName_ptr,
    char        **value_ptr,
    vint         *valueLen_ptr)
{
    char *start_ptr;
    char *end_ptr;
    char *p_ptr;
    vint  len;

    /*
     * Loop through all lines (delimited by "carriage-return & linefeed")
     * and search for the parameter name specified in pName_ptr.
     * If the pName_ptr is found return a pointer to the value.
     */

    /* Get the length of the parameter name to look for */
    len = OSAL_strlen(pName_ptr);

    start_ptr = payload_ptr;
    end_ptr = OSAL_strscan(start_ptr, SAPP_END_OF_LINE);
    while (end_ptr) {
        /*
         * Then we have a line from the payload. Search for the parameter name.
         */
        if (NULL != (p_ptr = OSAL_strncasescan(start_ptr, end_ptr - start_ptr,
                pName_ptr))) {
            /* Found it, Now get the value */
            p_ptr += len;
            /* Go past white space */
            while (' ' == *p_ptr) {
                p_ptr++;
            }
            *value_ptr = p_ptr;
            *valueLen_ptr = (end_ptr - p_ptr);
            return (SAPP_OK);
        }
        /* Otherwise go to the next line */
        start_ptr = end_ptr + 2;
        end_ptr = OSAL_strscan(start_ptr, SAPP_END_OF_LINE);
    }
    /* The last line will not be "\r\n" terminated but rather NULL terminated */
    if (NULL != (p_ptr = OSAL_strncasescan(start_ptr, OSAL_strlen(start_ptr),
            pName_ptr))) {
        /* Found it, Now get the value */
        p_ptr += len;
        /* Go past white space */
        while (' ' == *p_ptr) {
            p_ptr++;
        }
        *value_ptr = p_ptr;
        *valueLen_ptr = OSAL_strlen(p_ptr);
        return (SAPP_OK);
    }
    return (SAPP_ERR);
}

/*
 * ======== SAPP_parseHfExist() ========
 * This function identifies if the header field specified in hf_ptr
 * in the list of header field values of a
 * SIP event.
 *
 * Returns:
 *  0 = The header field does NOT exist
 *  1 = The header field exist.
 */
vint SAPP_parseHfExist(
    const char  *hf_ptr,
    tUaAppEvent *arg_ptr)
{
    vint x = 0;

    while (arg_ptr->szHeaderFields[x][0] != 0) {
        if (NULL != OSAL_strncasescan(&arg_ptr->szHeaderFields[x][0],
                SIP_EVT_STR_SIZE_BYTES, hf_ptr)) {
            return (1);
        }
        x++;
    }
    return (0);
}

/*
 * ======== SAPP_parseHfValueExist() ========
 * This function identifies if the header field specified in hf_ptr and the
 * value specified in value_ptr exist in the list of header field values of a
 * SIP event.
 *
 * Returns:
 *  0 = The header field and or value do NOT exist
 *  1 = The header field and value do exist.
 */
vint SAPP_parseHfValueExist(
    const char  *hf_ptr,
    const char  *value_ptr,
    tUaAppEvent *arg_ptr)
{
    vint x = 0;

    while (arg_ptr->szHeaderFields[x][0] != 0) {
        if (NULL != OSAL_strncasescan(&arg_ptr->szHeaderFields[x][0],
                SIP_EVT_STR_SIZE_BYTES, hf_ptr)) {
            /* found it, now look for the 'value' */
            if (NULL != OSAL_strncasescan(&arg_ptr->szHeaderFields[x][0],
                    SIP_EVT_STR_SIZE_BYTES, value_ptr)) {
                return (1);
            }
        }
        x++;
    }
    return (0);
}

/*
 * ======== SAPP_parseHfValue() ========
 * This function searches for a header field in a SIP event and returns the
 * value of the header field as a NULL terminated string.
 *
 * NOTE: The header field value will be unparsed.  This file contains
 * additional functions which will also parse the header field value
 * for some particular header fields which may be more suited to use
 * depending on the situation.
 *
 * Returns:
 *  char*: A pointer to the header field value.
 *  NULL : The header field does not exist.
 */
char* SAPP_parseHfValue(
    const char  *hf_ptr,
    tUaAppEvent *arg_ptr)
{
    vint  x;
    char *pos_ptr;

    if (arg_ptr == NULL) 
        return (NULL);
    x = 0;
    while (arg_ptr->szHeaderFields[x][0] != 0) {
        if (NULL != OSAL_strncasescan(&arg_ptr->szHeaderFields[x][0],
                SIP_EVT_STR_SIZE_BYTES, hf_ptr)) {
            /* found it, now look for the 'value' */
            if (NULL != (pos_ptr = OSAL_strscan(
                    &arg_ptr->szHeaderFields[x][0], ":"))) {
                /* Then the value is right after the ":" and any white space */
                pos_ptr++;
                while (*pos_ptr == ' ') {
                    pos_ptr++;
                }
                return (pos_ptr);
            }
        }
        x++;
    }
    return (NULL);
}

/*
 * ======== SAPP_parseSecSrvHfValue() ========
 * This function to search for a "Security-Server:" header field in a SIP 
 * event and store the header field for re-register. Finally, returns the max 
 * ssHfIdx it found.
 *
 * Returns:
 *  SAPP_OK Parsed completed.
 *  SAPP_ERR Failed to parse.
 */
int SAPP_parseSecSrvHfValue(
    SAPP_ServiceObj  *service_ptr,
    tUaAppEvent *arg_ptr)
{
    vint  x;
    char *pos_ptr;
    int   ssHfIdx = 0;

    x = 0;
    OSAL_memSet(service_ptr->secSrvHfs, 0, 
            sizeof(service_ptr->secSrvHfs));
    while (arg_ptr->szHeaderFields[x][0] != 0) {
        if (NULL != OSAL_strncasescan(&arg_ptr->szHeaderFields[x][0],
                SIP_EVT_STR_SIZE_BYTES, SAPP_SECURITY_SERVER_HF)) {
            /* found it, now look for the 'value' */
            if (NULL != (pos_ptr = OSAL_strscan(
                    &arg_ptr->szHeaderFields[x][0], ":"))) {
                /* Then the value is right after the ":" and any white space */
                pos_ptr++;
                while (*pos_ptr == ' ') {
                    pos_ptr++;
                }
                /* Store the header field to Security-Verify for re-register */
                OSAL_snprintf(&service_ptr->secSrvHfs[ssHfIdx][0],
                        SAPP_LONG_STRING_SZ, "%s%s",
                        SAPP_SECURITY_VERIFY_HF , pos_ptr);
                ssHfIdx++;
                if (SAPP_SEC_SRV_HF_MAX_NUM < ssHfIdx) {
                    /* Security-Server hf number exceeds what we can handle. */
                    OSAL_logMsg("Too many Security-Server hf number.\n");
                    return (SAPP_ERR);
                }
            }
        }
        x++;
    }

    if (0 == ssHfIdx) {
        /* No header filed found. */
        return (SAPP_ERR);
    }
    return (SAPP_OK);
}

/*
 * ======== SAPP_parseRbHfValue() ========
 * This function searches for a "Referred-By" header field with a specific
 * URI scheme. Returns value of the header field as a NULL terminated string.
 *
 * Returns:
 *  char*: A pointer to the header field value.
 *  NULL : The header field does not exist.
 */
char* SAPP_parseRbHfValue(
    tUaAppEvent *arg_ptr)
{
    vint  x;
    vint  size;
    char *pos_ptr;
    char *end_ptr;

    x = 0;
    while (arg_ptr->szHeaderFields[x][0] != 0) {
        if (NULL != (pos_ptr = OSAL_strncasescan(&arg_ptr->szHeaderFields[x][0],
                SIP_EVT_STR_SIZE_BYTES, SAPP_REFERRED_BY_HF))) {
            /* found it, now look for the 'value' */
            pos_ptr += (sizeof(SAPP_REFERRED_BY_HF) - 1);
            /* Remove any white space */
            while (*pos_ptr == ' ') {
                pos_ptr++;
            }
            size = OSAL_strlen(pos_ptr);
            SAPP_parseStripDelimters(&pos_ptr, &size, '<', '>');
            end_ptr = pos_ptr + size;
            *end_ptr = 0;
            return (pos_ptr);
        }
        x++;
    }
    return (NULL);
}

/*
 * ======== SAPP_getPcpiHfValue() ========
 * This function searches for a "P-Called-Party-Id" header field with a specific
 * URI scheme. Returns value of the header field as a NULL terminated string.
 *
 * Returns:
 *  char*: A pointer to the header field value.
 *  NULL : The header field does not exist.
 */
char* SAPP_parsePcpiHfValue(
    tUaAppEvent *arg_ptr)
{
    vint  x;
    vint  size;
    char *pos_ptr;
    char *end_ptr;

    x = 0;
    while (arg_ptr->szHeaderFields[x][0] != 0) {
        if (NULL != (pos_ptr = OSAL_strncasescan(&arg_ptr->szHeaderFields[x][0],
                SIP_EVT_STR_SIZE_BYTES, SAPP_P_CALLED_PARTY_ID_HF))) {
            /* found it, now look for the 'value' */
            pos_ptr += (sizeof(SAPP_P_CALLED_PARTY_ID_HF) - 1);
            /* Remove any white space */
            while (*pos_ptr == ' ') {
                pos_ptr++;
            }
            size = OSAL_strlen(pos_ptr);
            SAPP_parseStripDelimters(&pos_ptr, &size, '<', '>');
            end_ptr = pos_ptr + size;
            *end_ptr = 0;
            return (pos_ptr);
        }
        x++;
    }
    return (NULL);
}

/*
 * ======== SAPP_getPauHfValue() ========
 * This function searches for a "P-Associated-URI" header field, those entries
 * are the URI associated to the user. Store them, except the first one which
 * suppose to be known already, and see if there is tel uri for future use.
 *
 * Returns:
 *  Number of alias URI.
 */
int SAPP_parsePauHfValue(
    SAPP_ServiceObj *service_ptr,
    tUaAppEvent     *arg_ptr,
    char            *uri)
{
    vint  x;
    vint  size;
    char *pos_ptr;
    vint i;

    x = 0;
    i = 0;
    while (arg_ptr->szHeaderFields[x][0] != 0) {
        if (NULL != (pos_ptr = OSAL_strncasescan(&arg_ptr->szHeaderFields[x][0],
                SIP_EVT_STR_SIZE_BYTES, SAPP_P_ASSOCIATED_URI_HF))) {

            /* found it, now look for the 'value' */
            pos_ptr += (sizeof(SAPP_P_ASSOCIATED_URI_HF) - 1);
            /* Remove any white space */
            while (*pos_ptr == ' ') {
                pos_ptr++;
            }
            /* We remove the pattern "P-Associated-URI:" by using the api OSAL_strncasescan
                   * and then we can start to parse case 1/case 2 
                   * Case 1, one P-Associated-URI in one line and the format like
                   * <sip:a@example.net> 
                   * <sip:b@example.net>
                   * <sip:c@example.net>
                   * note:<sip:a@example.net> is primary uri, so we will skip it.
                   * Or case 2, there are many P-Associated-URI in one line and format like
                   * <sip:a@example.net>,<sip:b@example.net>,<sip:c@example.net>
                   * in this case, <sip:a@example.net> is also primary uri, so we will skip it.
                   */
            size = OSAL_strlen(pos_ptr);
            while (SAPP_OK != SAPP_parseStripDelimters(&pos_ptr, &size, '<', '>')) {
                if (0 == i) {
                    OSAL_strncpy(uri, pos_ptr, SIP_URI_STRING_MAX_SIZE);
                    /* It's primary uri, no need to cache it. */
                    i++;
                    pos_ptr += size + 2;
                    size = OSAL_strlen(pos_ptr); 
                    continue;                
                }
                OSAL_strncpy(service_ptr->aliasUriList[i - 1], pos_ptr,
                    SIP_URI_STRING_MAX_SIZE);
                /* NULL terminate the value. */
                service_ptr->aliasUriList[i - 1][size] = 0;
                SAPP_dbgPrintf("Alias URI:%s\n", service_ptr->aliasUriList[i - 1]);
                /* See if it's tel uri */
                if (0 == OSAL_strncmp(pos_ptr, SAPP_TEL_SCHEME,
                        OSAL_strlen(SAPP_TEL_SCHEME))) {
                    /* Pointer the tel uri pointer to alias uri table. */
                    if (NULL == service_ptr->telUri_ptr) {
                        service_ptr->telUri_ptr = service_ptr->aliasUriList[i - 1];
                    }
                }
                if (++i > SAPP_ALIAS_URI_MAX_NUM) {
                    SAPP_dbgPrintf("P-Associated-URI entry exceeds maximum.\n");
                    break;
                }
                pos_ptr += size + 2;
                size = OSAL_strlen(pos_ptr);
            }
        }
        x++;
    }
    return (i - 1);
}


/*
 * ======== SAPP_parsePaiHfCnap() ========
 * This function searches for a customized "P-Asserted-ID" header CNAP part.  It returned
 * the CNAP part of the PAI. It will return NULL if no CNAP string in the PAI at all. 
 * Returns CNAP as a NULL terminated string with \" pair.
 * Sample==> P-Asserted-Identity: "from d2 test" <sip:01022335640@lte-lguplus.co.kr>
 *
 * Returns:
 *  char*: A pointer to the CNAP stripped.
 *  NULL : The header field does not exist.
 */
char* SAPP_parsePaiHfCnap(
    tUaAppEvent *arg_ptr)
{
    vint  x;
    vint  size;
    char *pos_ptr;

    x = 0;
    size = 0;
    while (arg_ptr->szHeaderFields[x][0] != 0) {
        if (NULL != (pos_ptr = OSAL_strncasescan(&arg_ptr->szHeaderFields[x][0],
                SIP_EVT_STR_SIZE_BYTES, SAPP_P_ASSERTED_ID_HF))) {
            /* found it, now look for the 'CNAP' */
            pos_ptr += (sizeof(SAPP_P_ASSERTED_ID_HF) - 1);
            /* Remove any white space */
            while (*pos_ptr == ' ') {
                pos_ptr++;
            }
            size = OSAL_strlen(pos_ptr);
            if (*pos_ptr == '"') {
                SAPP_parseStripDelimters(&pos_ptr, &size, '"', '"');
                pos_ptr[size] = '\0';
                return (pos_ptr);
            }
        }
        x++;
    }
    return (NULL);
}

/*
 * ======== SAPP_getPaiHfValue() ========
 * This function searches for a "P-Asserted-ID" header field.  It will return
 * the PAI with the preferred scheme if it's exists. If preferred scheme is not
 * given, it returns the first one presented in the SIP message.
 * It will return NULL if no PAI exists at all.
 * Returns value of the header field as a NULL terminated string.
 *
 * Returns:
 *  char*: A pointer to the header field value.
 *  NULL : The header field does not exist.
 */
char* SAPP_parsePaiHfValue(
    const char  *preferredScheme_ptr,
    tUaAppEvent *arg_ptr)
{
    vint  x;
    vint  size;
    char *pos_ptr;
    char *end_ptr;

    x = 0;
    size = 0;
    while (arg_ptr->szHeaderFields[x][0] != 0) {
        if (NULL != (pos_ptr = OSAL_strncasescan(&arg_ptr->szHeaderFields[x][0],
                SIP_EVT_STR_SIZE_BYTES, SAPP_P_ASSERTED_ID_HF))) {
            /* found it, now look for the 'value' */
            pos_ptr += (sizeof(SAPP_P_ASSERTED_ID_HF) - 1);
            /* Remove any white space */
            while (*pos_ptr == ' ') {
                pos_ptr++;
            }
            size = OSAL_strlen(pos_ptr);
            SAPP_parseStripDelimters(&pos_ptr, &size, '<', '>');
            /* Check for the scheme if it's given. */
            if ((NULL == preferredScheme_ptr) ||
                    (0 == OSAL_strncmp(pos_ptr, preferredScheme_ptr,
                    OSAL_strlen(preferredScheme_ptr)))) {
                /* Found it, NULL terminate the value and return */
                end_ptr = pos_ptr + size;
                *end_ptr = 0;
                return (pos_ptr);
            }
        }
        x++;
    }
    return (NULL);
}

/*
 * ======== SAPP_parsePathHfValue() ========
 *
 * This function will search the last Path header field and
 * return the value of the last header field.
 *
 * Returns:
 *  char* : A pointer to the header field value.
 *  NULL : The header field does not exist.
 */
char* SAPP_parsePathHfValue(
    tUaAppEvent *arg_ptr)
{
    vint  x;
    vint  size;
    char *pos_ptr;
    char *ret_ptr;

    ret_ptr = NULL;
    x = 0;
    size = 0;
    while (0 != arg_ptr->szHeaderFields[x][0]) {
        if (NULL != (pos_ptr = OSAL_strncasescan(&arg_ptr->szHeaderFields[x][0],
                SIP_EVT_STR_SIZE_BYTES, SAPP_PATH_HF))) {
            /* found it, now look for the 'value' */
            pos_ptr += (sizeof(SAPP_PATH_HF) - 1);

            /* Remove any white space */
            while (*pos_ptr == ' ') {
                pos_ptr++;
            }
            /* find the last item by ',' */
            while (NULL != (ret_ptr = OSAL_strchr(pos_ptr, ','))) {
                pos_ptr += (OSAL_strlen(pos_ptr) - OSAL_strlen(ret_ptr) + 1);
            }

            size = OSAL_strlen(pos_ptr);
            SAPP_parseStripDelimters(&pos_ptr, &size, '<', '>');

            /* Check for the scheme */
            if (0 == OSAL_strncmp(pos_ptr, SAPP_SIP_SCHEME,
                    OSAL_strlen(SAPP_SIP_SCHEME))) {
                /* Found it, NULL terminate the value and return */
                ret_ptr = pos_ptr;
                ret_ptr[size] = 0;
            }
        }
        x++;
    }
    return (ret_ptr);
}

/*
 * ======== SAPP_parseAddHf() ========
 * This function adds a header field to a data buffer.
 * Specify the header field and value you want to add in
 * hdrFld_ptr and value_ptr.  Then specify the data buffer you
 * want to write the data to in target_ptr.  If successful, then
 * the target_ptr and targetLen_ptr are updated to reflect the
 * end of the data in the buffer and the length of the data
 * buffer space available
 *
 * Returns:
 *  SAPP_OK: The function was successful.  The target_ptr and targetLen_ptr
 *           are updated to reflect the state of the databuffer after the
 *           header field and value are added.
 *  SAPP_ERR: Function failed.  There is not enough room to write the
 *            header field and value.
 */
vint SAPP_parseAddHf(
    const char *hdrFld_ptr,
    const char *value_ptr,
    char     **target_ptr,
    vint      *targetLen_ptr)
{
    vint bytes;
    char *pos_ptr = *target_ptr;
    vint posLen = *targetLen_ptr;

    bytes = OSAL_snprintf(pos_ptr, posLen, "%s %s\r\n", hdrFld_ptr, value_ptr);
    if (bytes > posLen) {
        return (SAPP_ERR);
    }
    *targetLen_ptr = posLen - bytes;
    *target_ptr = pos_ptr + bytes;
    return (SAPP_OK);
}

/*
 * ======== SAPP_parseAddAddressHf() ========
 * This function adds a header field that specifies an address to a data buffer.
 * Specify the header field and address you want to add in
 * hdrFld_ptr and address_ptr.  Then specify the data buffer you
 * want to write the data to in target_ptr.  If successful, then
 * the target_ptr and targetLen_ptr are updated to reflect the
 * end of the data in the buffer and the length of the data
 * buffer space available.
 *
 * Note, brackets will be placed around the address if they don't exist.
 * Here are some examples of the output of this routine...
 *
 * input = sip:me@domain.com output = <sip:me@domain.com>
 * input = "steve parrish" <sip:me@domain.com> output = "steve parrish" <sip:me@domain.com>
 * input = <sip:me@domain.com> output = <sip:me@domain.com>
 *
 * Returns:
 *  SAPP_OK: The function was successful.  The target_ptr and targetLen_ptr
 *           are updated to reflect the state of the databuffer after the
 *           header field and value are added.
 *  SAPP_ERR: Function failed.  There is not enough room to write the
 *            header field and value.
 */
vint SAPP_parseAddAddressHf(
    const char *hdrFld_ptr,
    const char *address_ptr,
    char     **target_ptr,
    vint      *targetLen_ptr)
{
    vint bytes;
    char *pos_ptr = *target_ptr;
    vint posLen = *targetLen_ptr;

    /*
     * Check if there are brackets, if there are then use the address verbatim,
     * otherwise place brackets around the address.
     */
    if (NULL == OSAL_strchr(address_ptr, '<') &&
            NULL == OSAL_strchr(address_ptr, '>')) {
        bytes = OSAL_snprintf(pos_ptr, posLen, "%s <%s>\r\n",
                hdrFld_ptr, address_ptr);
    }
    else {
        /* Then use the address verbatim. */
        bytes = OSAL_snprintf(pos_ptr, posLen, "%s %s\r\n",
                hdrFld_ptr, address_ptr);
    }
    if (bytes > posLen) {
        return (SAPP_ERR);
    }
    *targetLen_ptr = posLen - bytes;
    *target_ptr = pos_ptr + bytes;
    return (SAPP_OK);
}

/*
 * ======== SAPP_parseAddBoundry() ========
 * This function adds a 'boundary' field to a data buffer.
 * Specify the boundary value you want to use and whether or not
 * it is a "starting" or an "end boundary that you want to add.
 * Then specify the data buffer you want to write the data to in target_ptr.
 * If successful, then the target_ptr and targetLen_ptr are updated to reflect
 * the end of the data in the buffer and the length of the data
 * buffer space available
 *
 * Returns:
 *  SAPP_OK: The function was successful.  The target_ptr and targetLen_ptr
 *           are updated to reflect the state of the databuffer after the
 *           header field and value are added.
 *  SAPP_ERR: Function failed.  There is not enough room to write the
 *            header field and value.
 */
vint SAPP_parseAddBoundry(
    vint        isEnd,
    const char *value_ptr,
    char      **target_ptr,
    vint       *targetLen_ptr)
{
    vint bytes;
    char *pos_ptr = *target_ptr;
    vint posLen = *targetLen_ptr;

    if (isEnd) {
        bytes = OSAL_snprintf(pos_ptr, posLen, "--%s--\r\n", value_ptr);
    }
    else {
        bytes = OSAL_snprintf(pos_ptr, posLen, "--%s\r\n", value_ptr);
    }
    
    if (bytes > posLen) {
        return (SAPP_ERR);
    }
    *targetLen_ptr = posLen - bytes;
    *target_ptr = pos_ptr + bytes;
    return (SAPP_OK);
}

/*
 * ======== SAPP_parseAddPayload() ========
 * This function adds a 'boundary' field to a data buffer.
 * Specify the boundary value you want to use and whether or not
 * it is a "starting" or an "end boundary that you want to add.
 * Then specify the data buffer you want to write the data to in target_ptr.
 * If successful, then the target_ptr and targetLen_ptr are updated to reflect
 * the end of the data in the buffer and the length of the data
 * buffer space available
 *
 * Returns:
 *  SAPP_OK: The function was successful.  The target_ptr and targetLen_ptr
 *           are updated to reflect the state of the databuffer after the
 *           header field and value are added.
 *  SAPP_ERR: Function failed.  There is not enough room to write the
 *            header field and value.
 */
vint SAPP_parseAddPayload(
    const char *value_ptr,
    char      **target_ptr,
    vint       *targetLen_ptr,
    vint        includeEol)
{
    vint bytes;
    char *pos_ptr = *target_ptr;
    vint posLen = *targetLen_ptr;

    if (0 != includeEol) {
        bytes = OSAL_snprintf(pos_ptr, posLen, "%s\r\n", value_ptr);
    }
    else {
        bytes = OSAL_snprintf(pos_ptr, posLen, "%s", value_ptr);
    }

    if (bytes > posLen) {
        return (SAPP_ERR);
    }
    *targetLen_ptr = posLen - bytes;
    *target_ptr = pos_ptr + bytes;
    return (SAPP_OK);
}

/*
 * ======== SAPP_parseAddEol() ========
 * This function adds a 'EOL' (End of line) sequence to a data buffer.
 *
 * Returns:
 *  SAPP_OK: The function was successful.  The target_ptr and targetLen_ptr
 *           are updated to reflect the state of the databuffer after the
 *           header field and value are added.
 *  SAPP_ERR: Function failed.  There is not enough room to write the
 *            header field and value.
 */
vint SAPP_parseAddEol(
    char      **target_ptr,
    vint       *targetLen_ptr)
{
    vint bytes;
    char *pos_ptr = *target_ptr;
    vint posLen = *targetLen_ptr;

    bytes = OSAL_snprintf(pos_ptr, posLen, "%s", "\r\n");

    if (bytes > posLen) {
        return (SAPP_ERR);
    }
    *targetLen_ptr = posLen - bytes;
    *target_ptr = pos_ptr + bytes;
    return (SAPP_OK);
}

/*
 * ======== SAPP_parseBoundry() ========
 * This function is used to parse out the "boundry" field of a ContentType
 * header field whwen the header field value is "multipart-mixed".
 *
 * Returns:
 *  The size of the boundry field in bytes. If it didn't exist then 0 is
 *  returned: The function was successful.
 */
vint SAPP_parseBoundry(
    char *contentType_ptr,
    vint  contentTypeSize,
    char *boundry_ptr,
    vint  boundrySize)
{
    char *start_ptr;
    char *quote_ptr;
    vint  size;
    
    /* Get the 'boundary=' value in the content-type */
    if (NULL == (start_ptr = OSAL_strnscan(contentType_ptr,
            contentTypeSize, "boundary="))) {
        /* Doesn't exist */
        return (0);
    }
    /* Decrement so we can be sure we are NULL terminated */
    boundrySize--;
    /* Advance off the boundry */
    start_ptr += (sizeof("boundary=") - 1);
    /* Update the size to reflect what's to the left of the boundry= */
    size = start_ptr - contentType_ptr;
    contentTypeSize -= size;
    /* Check for qoutes */
    if (*start_ptr == '\"') {
        start_ptr++;
        /* Then let's get the other qoute */
        if (NULL != (quote_ptr = OSAL_strnscan(start_ptr,
                contentTypeSize, "\""))) {
            size = (quote_ptr - start_ptr) - 1;
            if (size > boundrySize) {
                /* No room */
                return (0);
            }
            OSAL_memCpy(boundry_ptr, start_ptr, size);
            /* Make sure it NULL terminated */
            boundry_ptr[size] = 0;
            return (size);
        }
        /* If we are here then it's malformed */
        return (0);
    }
    /* If we are here then there are no qoutes */
    if (contentTypeSize > boundrySize) {
        /* No room */
        return (0);
    }
    OSAL_memCpy(boundry_ptr, start_ptr, contentTypeSize); /* Copy token */
    boundry_ptr[contentTypeSize] = 0; /* NULL terminate */
    return (contentTypeSize);
}

/*
 * ======== SAPP_parseAdvance() ========
 * This function advances the patload_ptr and updates the payload length after
 * it searches for the escape seqeunce indicated by token_ptr. Note, that
 * the payload_ptr will be advanced to reflect the position in a buffer after
 * the escape sequence indicated in token_ptr.
 *
 * Returns:
 *  SAPP_OK: The function was successful.  The payload_ptr and payloadLen_ptr
 *           are updated to reflect the state of the data buffer after the
 *           specified token.
 *  SAPP_ERR: Function failed.  Could not find the specified token.
 */
vint SAPP_parseAdvance(
    char      **payload_ptr,
    vint       *payloadLen_ptr,
    const char *token_ptr)
{
    char          *start_ptr;
    vint           size;
    
    size = *payloadLen_ptr;
    start_ptr = *payload_ptr;
    /* Get the start and stop of the part of the payload to process */
    if (NULL == (start_ptr = OSAL_strnscan(start_ptr, size, token_ptr))) {
        /* Nothing there */
        return (SAPP_ERR);
    }
    /* Advance off the token */
    start_ptr += OSAL_strlen(token_ptr);
     /* update the payload size */
    size -= (start_ptr - *payload_ptr);

    *payload_ptr = start_ptr;
    *payloadLen_ptr = size;
    return (SAPP_OK);
}

/*
 * ======== SAPP_parseAddPhoneContext() ========
 * This function will add a 'phone-context' URI parameter to the to field
 * if the address is a phone number prefaced with a '+' indicating that it
 * is an international number.
 *
 * Returns:
 *  SAPP_OK: The function added the phone-context value.  
 *           It is an international number.
 *  SAPP_ERR: The function did NOT add the 'phone-context' value.
 */
vint SAPP_parseAddPhoneContext(
    char       *to_ptr,
    vint        maxToLen,
    const char *domain_ptr)
{
    vint offset;
    if (0 != OSAL_strncasecmp(to_ptr, "tel:", sizeof("tel:") - 1)) {
        /* phone-context well not be added */
        return (SAPP_ERR);
    }
    /* If we are here then we have a tel URI */
    if (0 != OSAL_strncasecmp(to_ptr, "tel:+", sizeof("tel:+") - 1)) {
        /* Then the tel is not 'global' */
        if (NULL != domain_ptr && 0 != *domain_ptr) {
            /*
             * Then this is not an international phone number.
             * Add 'phone-context'
             */
            offset = OSAL_strlen(to_ptr);
            OSAL_snprintf(to_ptr + offset, maxToLen - offset,
                    ";phone-context=%s", domain_ptr);
            return (SAPP_OK);
        }
    }
    /* Otherwise it's not an international phone number. No 'phone-context' */
    return (SAPP_ERR);
}

/*
 * ======== SAPP_parseCopy() ========
 * This function will copy a source buffer to a destination and ensure the
 * destination is NULL terminated and will not overflow.
 *
 * Returns:
 *   Nothing
 */
void SAPP_parseCopy(
    char       *dest_ptr,
    vint        maxDestLen,
    const char *src_ptr,
    vint        srcLen)
{
    vint size;
    maxDestLen--;
    size = (srcLen > maxDestLen) ? maxDestLen : srcLen;
    OSAL_memCpy(dest_ptr, src_ptr, size);
    dest_ptr[size] = 0;
    return;
}

/*
 * ======== SAPP_parseUsername() ========
 * This function is used to update a pointer and size of a string to get
 * username of a given URI.
 * Example: "sip:user@domian", the value_ptr will be update to "user@domain"
 * and size will be 4.
 *
 * Returns:
 *   SAPP_OK: Username found.
 *   SAPP_ERR: Username not found.
 */
vint SAPP_parseUsername(
    char **value_ptr,
    vint  *size)
{
    char *end_ptr;

    /* Get rid of scheme. */
    if (0 == OSAL_strncmp(*value_ptr, SAPP_TEL_SCHEME,
            sizeof(SAPP_TEL_SCHEME))) {
        *value_ptr += sizeof(SAPP_TEL_SCHEME);
    }
    else if (0 == OSAL_strncmp(*value_ptr, SAPP_SIP_SCHEME,
            sizeof(SAPP_SIP_SCHEME))) {
        *value_ptr += sizeof(SAPP_SIP_SCHEME);
    }
    else {
        /* No scheme, consider an error. */
        return (SAPP_ERR);
    }

    /* Find username length. */
    if (NULL != (end_ptr = OSAL_strscan(*value_ptr, "@"))) {
        *size = end_ptr - *value_ptr;
    }
    else {
        *size = OSAL_strlen(*value_ptr);
    }

    return (SAPP_OK);
}

/*
 * ======== SAPP_parseCompareUsername() ========
 * This function will compare if the username is the same between two URIs.
 *
 * Returns:
 *   OSAL_TRUE: The username is the same.
 *   OSAL_FALSE: The username is not the same.
 */
OSAL_Boolean SAPP_parseCompareUsername(
    char *uri1_ptr,
    char *uri2_ptr)
{
    vint size1;
    vint size2;

    if ((SAPP_OK == SAPP_parseUsername(&uri1_ptr, &size1)) &&
            (SAPP_OK == SAPP_parseUsername(&uri2_ptr, &size2))) {
        /* Compare size. */
        if (size1 != size2) {
            return (OSAL_FALSE);
        }
        /* Compare the string. */
        if (0 == OSAL_strncmp(uri1_ptr, uri2_ptr, size1)) {
            return (OSAL_TRUE);
        }
    }

    return (OSAL_FALSE);
}


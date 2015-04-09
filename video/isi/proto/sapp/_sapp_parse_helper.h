/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 14395 $ $Date: 2011-04-01 19:23:22 -0500 (Fri, 01 Apr 2011) $
 */

#ifndef _SAPP_PARSE_HELPER_H_
#define _SAPP_PARSE_HELPER_H_

SAPP_ContentType SAPP_parseIntFileType(
   const char *type_ptr,
   vint        len);

const char* SAPP_parseExtFileType(
    SAPP_ContentType type);

vint SAPP_parseStripDelimters(
    char **value_ptr,
    vint  *size_ptr,
    char   begin,
    char   end);

vint SAPP_parsePayloadValue(
    char         *payload_ptr,
    const char   *pName_ptr,
    char        **value_ptr,
    vint         *valueLen_ptr);

vint SAPP_parseHfExist(
    const char  *hf_ptr,
    tUaAppEvent *arg_ptr);

vint SAPP_parseHfValueExist(
    const char  *hf_ptr,
    const char  *value_ptr,
    tUaAppEvent *arg_ptr);

char* SAPP_parseHfValue(
    const char  *hf_ptr,
    tUaAppEvent *arg_ptr);

int SAPP_parseSecSrvHfValue(
    SAPP_ServiceObj  *service_ptr,
    tUaAppEvent *arg_ptr);

char* SAPP_parseRbHfValue(
    tUaAppEvent *arg_ptr);

char* SAPP_parsePcpiHfValue(
    tUaAppEvent *arg_ptr);

int SAPP_parsePauHfValue(
    SAPP_ServiceObj *service_ptr,
    tUaAppEvent     *arg_ptr,
    char            *uri);

char* SAPP_parsePaiHfCnap(
    tUaAppEvent *arg_ptr);

char* SAPP_parsePaiHfValue(
    const char  *preferredScheme_ptr,
    tUaAppEvent *arg_ptr);

char* SAPP_parsePathHfValue(
    tUaAppEvent *arg_ptr);

vint SAPP_parseAddHf(
    const char *hdrFld_ptr,
    const char *value_ptr,
    char     **target_ptr,
    vint      *targetLen_ptr);

vint SAPP_parseAddAddressHf(
    const char *hdrFld_ptr,
    const char *address_ptr,
    char     **target_ptr,
    vint      *targetLen_ptr);

vint SAPP_parseAddBoundry(
    vint        isEnd,
    const char *value_ptr,
    char      **target_ptr,
    vint       *targetLen_ptr);

vint SAPP_parseAddString(
    const char *value_ptr,
    char      **target_ptr,
    vint       *targetLen_ptr);

vint SAPP_parseAddPayload(
    const char *value_ptr,
    char      **target_ptr,
    vint       *targetLen_ptr,
    vint        includeEol);

vint SAPP_parseAddEol(
    char      **target_ptr,
    vint       *targetLen_ptr);

vint SAPP_parseBoundry(
    char *contentType_ptr,
    vint  contentTypeSize,
    char *boundry_ptr,
    vint  boundrySize);

vint SAPP_parseAdvance(
    char      **payload_ptr,
    vint       *payloadLen_ptr,
    const char *token_ptr);

vint SAPP_parseAddPhoneContext(
    char       *to_ptr,
    vint        maxToLen,
    const char *domain_ptr);

const char* SAPP_parseFileDisposition(
    SAPP_FileAttribute attribute);

SAPP_FileAttribute SAPP_parseFileAttribute(
    const char *disposition, vint len);

void SAPP_parseCopy(
    char       *dest_ptr,
    vint        maxDestLen,
    const char *src_ptr,
    vint        srcLen);

vint SAPP_parseUsername(
    char **value_ptr,
    vint  *size);

OSAL_Boolean SAPP_parseCompareUsername(
    char *uri1_ptr,
    char *uri2_ptr);

#endif

/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 24731 $ $Date: 2014-02-21 01:51:50 -0800 (Fri, 21 Feb 2014) $
 */

#ifndef _ISI_XML_H_
#define _ISI_XML_H_

#define _ISI_XML_TAG_MEDIA         "media"
#define _ISI_XML_TAG_AUDIO          "audio"
#define _ISI_XML_TAG_VIDEO          "video"
#define _ISI_XML_ATTR_ENABLED       "enabled"
#define _ISI_XML_ATTR_DIRECTION     "direction"
#define _ISI_XML_ATTR_SECURE        "secure"
#define _ISI_XML_ATTR_EMERGENCY     "emergency"
#define _ISI_XML_ATTR_USERTPAVPF    "useRtpAVPF"
#define _ISI_XML_ATTR_MAXBANDWIDTH  "maxBandwidth"
#define _ISI_XML_VALUE_TRUE         "true"
#define _ISI_XML_VALUE_INACTIVE     "inactive"
#define _ISI_XML_VALUE_SENDONLY     "sendonly"
#define _ISI_XML_VALUE_RECVONLY     "recvonly"
#define _ISI_XML_VALUE_SENDRECV     "sendrecv"

ISI_Return ISI_decodeMediaAttributeXMLDoc(
    char               *doc_ptr,
    ISID_CallId        *call_ptr);

#endif

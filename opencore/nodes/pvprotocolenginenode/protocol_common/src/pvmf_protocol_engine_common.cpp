/* ------------------------------------------------------------------
 * Copyright (C) 1998-2010 PacketVideo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * -------------------------------------------------------------------
 */
#include "pvmf_protocol_engine_common.h"
#include "pvmf_media_msg_format_ids.h" // for PVMF_MEDIA_CMD_EOS_FORMAT_ID
#include "oscl_utf8conv.h" // for oscl_UnicodeToUTF8

#ifndef OSCLCONFIG_IO_H_INCLUDED
#include "osclconfig_io.h"
#endif


////////////////////////////////////////////////////////////////////////////////////
//////  pvHttpDownloadInput implementation
////////////////////////////////////////////////////////////////////////////////////
bool pvHttpDownloadInput::getValidMediaData(INPUT_DATA_QUEUE &aDataInQueue, PVMFSharedMediaDataPtr &aMediaData, bool &isEOS)
{
    isEOS = false; //aMediaData.Bind(iCurrentInputMediaData);

    do
    {
        // There should be at least one media msg in the queue
        if (aDataInQueue.empty()) return false;

        // Check if the next incoming media msg is an EOS or not
        // Introducing a boolean variable aEOSMsg is for simulating connection shutdown cases
        bool aEOSMsg = aDataInQueue[0]->getFormatID() == PVMF_MEDIA_CMD_EOS_FORMAT_ID;
        if (aEOSMsg)
        {
            isEOS = true;
            aDataInQueue.erase(aDataInQueue.begin());
            return true;
        }

        convertToPVMFMediaData(iCurrentInputMediaData, aDataInQueue[0]);
        aDataInQueue.erase(aDataInQueue.begin());
    }
    while (isValidInput() == false);

    aMediaData.Bind(iCurrentInputMediaData);
    return true;
}

bool pvHttpDownloadInput::isValidInput()
{
    if (iCurrentInputMediaData->getTimestamp() == 0xFFFFFFFF ||
            iCurrentInputMediaData->getFilledSize() == 0)
    {
        // Cannot use this input media data since TS is not available
        // Discard and return to retrieve another input media data from the queue
        unbind();
        return false;
        // ANOTHER OPTION TO DEFAULT THE TS TO 0 IN THIS CASE AND CONTINUE ON
    }
    return true;
}

//////  INetURI implementation
////////////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool INetURI::setURI(OSCL_wString &aUri, const bool aRedirectURI)
{
    if (aUri.get_size() == 0) return false;

    OsclMemAllocator alloc;
    char *buf = (char*)alloc.allocate(aUri.get_size() + 1);
    if (!buf) return false;
    uint32 size = oscl_UnicodeToUTF8(aUri.get_cstr(), aUri.get_size(), buf, aUri.get_size() + 1);
    if (size == 0)
    {
        alloc.deallocate(buf);
        return false;
    }
    iURI = OSCL_HeapString<OsclMemAllocator> (buf, size);
    alloc.deallocate(buf);
    // clear iHost
    iHostName.set(NULL, 0);
    iRedirectURI = aRedirectURI;
    return true;
}

OSCL_EXPORT_REF bool INetURI::getHostAndPort(OSCL_String &aSerAdd, uint32 &aSerPort)
{
    if (iURI.get_size() == 0) return false;
    if (iHostName.get_size() == 0)
    {
        if (!parseURL(iURI, iHostName, iHostPort)) return false;
    }
    aSerAdd = iHostName;
    aSerPort = iHostPort;
    return true;
}

bool INetURI::parseURL(OSCL_String &aUrl8, OSCL_String &aSerAdd, uint32 &aSerPort)
{
    OSCL_HeapString<OsclMemAllocator> tmpUrl8(aUrl8);

    typedef char mbchar;
    mbchar* aUrl = tmpUrl8.get_str();

    mbchar *server_ip_ptr = OSCL_CONST_CAST(mbchar*, oscl_strstr(((mbchar*)aUrl), "//"));
    if (server_ip_ptr == NULL) return false;
    server_ip_ptr += 2;

    /* Locate the IP address. */
    mbchar *clip_name = OSCL_CONST_CAST(mbchar*, oscl_strstr(server_ip_ptr, "/"));
    if (clip_name != NULL) *clip_name = '\0';

    mbchar *server_port_ptr = OSCL_CONST_CAST(mbchar*, oscl_strstr(server_ip_ptr, ":"));
    if (server_port_ptr != NULL)
    {
        // find the port number
        *server_port_ptr++ = '\0';
    }

    /* Locate the port number if provided. */
    aSerPort = DEFAULT_HTTP_PORT_NUMBER;
    if (server_port_ptr != NULL)
    {
        uint32 atoi_tmp;
        if (PV_atoi(server_port_ptr, 'd', atoi_tmp)) aSerPort = atoi_tmp;
        else return false;
    }

    /* relocate the server IP address, either stop at ':' or '/' */
    mbchar *server_end_ptr = OSCL_CONST_CAST(mbchar*, oscl_strstr(server_ip_ptr, "/"));
    if (server_end_ptr) *server_end_ptr = '\0';

    OSCL_HeapString<OsclMemAllocator> tmpServerName(server_ip_ptr, oscl_strlen(server_ip_ptr));
    aSerAdd = tmpServerName;
    return true;
}

////////////////////////////////////////////////////////////////////////////////////
//////  HttpParsingBasicObject implementation
////////////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF int32 HttpParsingBasicObject::parseResponse(INPUT_DATA_QUEUE &aDataQueue)
{
    PVMFSharedMediaDataPtr mediaData;
    int32 status = getNextMediaData(aDataQueue, mediaData);
    if (status != PARSE_SUCCESS)
    {
        if (status == PARSE_EOS_INPUT_DATA)
        {
            return validateEOSInput(status);
        }
        return status; // no input data or eos
    }

    OsclRefCounterMemFrag fragIn;
    mediaData->getMediaFragment(0, fragIn);
    HttpParsingBasicObjectAutoCleanup cleanup(this);

    while (status == PARSE_SUCCESS)
    {
        RefCountHTTPEntityUnit entityUnit;
        int32 parsingStatus = iParser->parse(fragIn, entityUnit);
        if (parsingStatus < 0)
        {
            PVMF_PROTOCOL_ENGINE_LOGERRINFODATAPATH((0,
                                                    "HttpParsingBasicObject::parseResponse(), iParser->parse() retval=%d(iHttpHeaderParsed=%d)",
                                                    parsingStatus, (int32)iHttpHeaderParsed));
        }
        else
        {
            // save output data if there is
            iOutputQueue->clear();
            uint32 size = 0;
            if (!saveOutputData(entityUnit, *iOutputQueue, size))
            {
                return PARSE_GENERAL_ERROR;
            }

            if (parsingStatus == HTTPParser::PARSE_HEADER_AVAILABLE)
            {
                iHttpHeaderParsed = true;
                iParser->getContentInfo(iContentInfo);

                if (iContentInfo.iContentLength < 0)
                    return PROCESS_GENERAL_ERROR;

                extractServerVersionNum();

                // update BandWidthEstimationInfo
                iBWEstInfo.update(mediaData, iHttpHeaderParsed);

                // do sanity check for HTTP header
                int32 sanityCheckStatus = iParser->doSanityCheckForResponseHeader();
                if (sanityCheckStatus == HTTPParser::PARSE_TRANSFER_ENCODING_NOT_SUPPORTED)
                {
                    parsingStatus = sanityCheckStatus;
                }
                else
                {
                    // output data
                    status = iObserver->OutputDataAvailable(iOutputQueue, true);
                    if (status < 0) return status;
                }
            }
            else if (iHttpHeaderParsed && size > 0)
            {
                iTotalDLHttpBodySize += size;
                iTotalHttpStreamingSize += size;
                if (iLatestMediaDataTimestamp < mediaData->getTimestamp()) iLatestMediaDataTimestamp = mediaData->getTimestamp();

                // update BandWidthEstimationInfo
                iBWEstInfo.update(mediaData, iHttpHeaderParsed);
                PVMF_PROTOCOL_ENGINE_LOGERRINFODATAPATH((0, "HttpParsingBasicObject::parseResponse() 1:streaming currentSize = %u, 2:download Size = %u", iTotalHttpStreamingSize, iTotalDLHttpBodySize));
                PVMF_PROTOCOL_ENGINE_LOGERRINFODATAPATH((0, "HttpParsingBasicObject::parseResponse() file size = %u, download size = %u, curr_size = %u, new download size = %u",
                                                        iContentInfo.iContentLength, iTotalDLHttpBodySize, size, iBWEstInfo.iTotalSizePerRequest));
            }
        }

        // check the condition of whether parsing the current input is done or not
        // may send out callback for end of message or end of input cases
        if ((status = checkParsingDone(parsingStatus)) != PARSE_SUCCESS)
        {
            if (status != PROCESS_WAIT_FOR_INCOMING_DATA)
            {
                PVMF_PROTOCOL_ENGINE_LOGERRINFODATAPATH((0, "HttpParsingBasicObject::parseResponse() status=checkParsingDone(parsingStatus)); parsingStatus = %d , status = %d",
                                                        parsingStatus, status));
            }
            return status;
        }
    }

    return PARSE_SUCCESS;
}

int32 HttpParsingBasicObject::getNextMediaData(INPUT_DATA_QUEUE &aDataInQueue, PVMFSharedMediaDataPtr &aMediaData)
{
    bool isEOS = false;
    if (!iInput.getValidMediaData(aDataInQueue, aMediaData, isEOS)) return PARSE_NO_INPUT_DATA;
    if (isEOS)
    {
        if (!isRedirectResponse())
        {
            iNumEOSMessagesAfterRequest++;
            iTotalDLSizeForPrevEOS = iTotalDLSizeAtCurrEOS;
            iTotalDLSizeAtCurrEOS = iTotalDLHttpBodySize;
            PVMF_PROTOCOL_ENGINE_LOGERRINFODATAPATH((0, "HttpParsingBasicObject::getNextMediaData() isEOS=%d, iNumEOSMessagesAfterRequest=%d, iTotalDLHttpBodySize=%d, iTotalDLSizeAtCurrEOS=%d, iTotalDLSizeForPrevEOS=%d",
                                                    (uint32)isEOS, iNumEOSMessagesAfterRequest, iTotalDLHttpBodySize, iTotalDLSizeAtCurrEOS, iTotalDLSizeForPrevEOS));

        }
        iInput.unbind();
        return PARSE_EOS_INPUT_DATA;
    }
    //if(iTotalDLSizeAtCurrEOS!=iTotalDLHttpBodySize || !isEOS) {
    if (iTotalDLSizeAtCurrEOS > iTotalDLSizeForPrevEOS || iTotalDLHttpBodySize > iTotalDLSizeAtCurrEOS)
    {
        iNumEOSMessagesAfterRequest = 0;
        PVMF_PROTOCOL_ENGINE_LOGERRINFODATAPATH((0, "HttpParsingBasicObject::getNextMediaData(), non-EOS case iNumEOSMessagesAfterRequest is reset to 0, iTotalDLHttpBodySize=%d, iTotalDLSizeAtCurrEOS=%d, iTotalDLSizeForPrevEOS=%d",
                                                iTotalDLHttpBodySize, iTotalDLSizeAtCurrEOS, iTotalDLSizeForPrevEOS));
    }
    if (iNumEOSMessagesAfterRequest != 0)
    {
        PVMF_PROTOCOL_ENGINE_LOGERRINFODATAPATH((0, "HttpParsingBasicObject::getNextMediaData(), should be non-EOS case and iNumEOSMessagesAfterRequest is NOT reset to 0! isEOS=%d, iNumEOSMessagesAfterRequest=%d, iTotalDLHttpBodySize=%d, iTotalDLSizeAtCurrEOS=%d, iTotalDLSizeForPrevEOS=%d",
                                                (uint32)isEOS, iNumEOSMessagesAfterRequest, iTotalDLHttpBodySize, iTotalDLSizeAtCurrEOS, iTotalDLSizeForPrevEOS));
    }
    return PARSE_SUCCESS;
}

int32 HttpParsingBasicObject::validateEOSInput(int32 parsingStatus)
{
    //if(!iHttpHeaderParsed && iNumEOSMessagesAfterRequest>=MAX_NUM_EOS_MESSAGES_FOR_SAME_REQUEST) { // MAX_NUM_EOS_MESSAGES_FOR_SAME_REQUEST=2
    if (iNumEOSMessagesAfterRequest >= iNumRetry)   // iNumRetry = MAX_NUM_EOS_MESSAGES_FOR_SAME_REQUEST=2
    {
        // if we recieve EOS message as the first response after sending request, and we do socket reconnect and send the request again, if we still
        // get EOS message, that means server always shuts down the connection for this request, and we don't want to try reconnect any more. That
        // indicates the url is probably bad.
        // In general, we treat download size unchange between two adjacent EOSs (even if there is some header parsed) as bad url. Otherwise we'll
        // run into infinite reconnect/disconnect loop
        return PARSE_BAD_URL;
    }
    return parsingStatus;
}

void HttpParsingBasicObject::extractServerVersionNum()
{
    StrCSumPtrLen serverKey = "Server";
    StrPtrLen serverValue;
    if (!iParser->getField(serverKey, serverValue)) return;
    if (serverValue.length() == 0) return;

    // Has Sever header
    char *ptr = (char*)serverValue.c_str();
    for (int32 i = 0; i < serverValue.length(); i++)
    {
        if (!PE_isDigit(*ptr))
        {
            ptr++;
            continue;
        }
        iServerVersionNumber = *ptr++ - '0';
        if (PE_isDigit(*ptr) && ++i < serverValue.length())
        {
            iServerVersionNumber = iServerVersionNumber * 10 + (*ptr - '0');
        }
        break;
    }
}

bool HttpParsingBasicObject::saveOutputData(RefCountHTTPEntityUnit &entityUnit, OUTPUT_DATA_QUEUE &aOutputData, uint32 &aTotalEntityDataSize)
{
    aTotalEntityDataSize = 0;
    int32 err = 0;
    OSCL_TRY(err,
             for (uint32 i = 0; i < entityUnit.getEntityUnit().getNumFragments(); i++)
{
    OsclRefCounterMemFrag memfrag;
    entityUnit.getEntityUnit().getMemFrag(i, memfrag);
        aOutputData.push_back(memfrag);
        aTotalEntityDataSize += memfrag.getMemFragSize();
    }
            );
    return (err == 0);
}

int32 HttpParsingBasicObject::checkParsingDone(const int32 parsingStatus)
{
    // check error case
    if (parsingStatus < 0)
    {
        if (parsingStatus == HTTPParser::PARSE_SYNTAX_ERROR) return PARSE_SYNTAX_ERROR;
        if (parsingStatus == HTTPParser::PARSE_HTTP_VERSION_NOT_SUPPORTED) return PARSE_HTTP_VERSION_NOT_SUPPORTED;
        if (parsingStatus == HTTPParser::PARSE_TRANSFER_ENCODING_NOT_SUPPORTED) return PARSE_TRANSFER_ENCODING_NOT_SUPPORTED;
        return PARSE_GENERAL_ERROR; // error happens;
    }

    if (parsingStatus == HTTPParser::PARSE_SUCCESS_END_OF_MESSAGE ||
            parsingStatus == HTTPParser::PARSE_SUCCESS_END_OF_MESSAGE_WITH_EXTRA_DATA ||
            parsingStatus == HTTPParser::PARSE_SUCCESS_END_OF_INPUT   ||
            parsingStatus == HTTPParser::PARSE_SUCCESS)
    {
        // send output data
        if (iHttpHeaderParsed && !iOutputQueue->empty())
        {
            //if(!iObserver->OutputDataAvailable(iOutputQueue, false)) return PARSE_GENERAL_ERROR;
            int32 status = iObserver->OutputDataAvailable(iOutputQueue, false);
            if (status < 0) return status;
            if (status == PROCESS_SUCCESS_END_OF_MESSAGE) return PARSE_SUCCESS_END_OF_MESSAGE;
            if (status == PROCESS_SUCCESS_END_OF_MESSAGE_TRUNCATED) return status;
        }
    }
    if (parsingStatus == HTTPParser::PARSE_STATUS_LINE_SHOW_NOT_SUCCESSFUL)       return PARSE_STATUS_LINE_SHOW_NOT_SUCCESSFUL; // status code >= 300
    if (parsingStatus == HTTPParser::PARSE_SUCCESS_END_OF_MESSAGE)                return PARSE_SUCCESS_END_OF_MESSAGE;
    if (parsingStatus == HTTPParser::PARSE_SUCCESS_END_OF_MESSAGE_WITH_EXTRA_DATA) return PARSE_SUCCESS_END_OF_MESSAGE_WITH_EXTRA_DATA;
    if (parsingStatus == HTTPParser::PARSE_NEED_MORE_DATA)                        return PARSE_NEED_MORE_DATA;
    if (parsingStatus == HTTPParser::PARSE_SUCCESS_END_OF_INPUT)                      return PARSE_SUCCESS_END_OF_INPUT;

    // HTTPParser::PARSE_SUCCESS or HTTPParser::PARSE_HEADER_AVAILABLE
    return PARSE_SUCCESS;
}

bool HttpParsingBasicObject::isRedirectResponse()
{
    bool aPartOfRedirectResponse = false;
    uint32 httpStatusCode = getStatusCode();
    if (PROTOCOLENGINE_REDIRECT_STATUS_CODE_START <= httpStatusCode && httpStatusCode <= PROTOCOLENGINE_REDIRECT_STATUS_CODE_END)
    {
        aPartOfRedirectResponse = true;
    }
    return aPartOfRedirectResponse;
}


// factory method
OSCL_EXPORT_REF HttpParsingBasicObject* HttpParsingBasicObject::create()
{
    HttpParsingBasicObject *object = OSCL_NEW(HttpParsingBasicObject, ());
    if (!object) return NULL;
    if (!object->construct())
    {
        OSCL_DELETE(object);
        return NULL;
    }
    return object;
}

bool HttpParsingBasicObject::construct()
{
    reset();
    resetForBadConnectionDetection();
    iServerVersionNumber = 0;
    if ((iParser = HTTPParser::create()) == NULL) return false;
    return true;
}

void HttpParsingBasicObject::setDownloadSize(const TOsclFileOffset aInitialSize)
{
    if (aInitialSize > 0)
        iTotalDLHttpBodySize = aInitialSize;
    else
    {
        // sync up with content-range_left
        iTotalDLHttpBodySize = iContentInfo.iContentRangeLeft;
    }

    PVMF_PROTOCOL_ENGINE_LOGERRINFODATAPATH((0, "HttpParsingBasicObject::setDownloadSize %d", iTotalDLHttpBodySize));

    iTotalDLSizeForPrevEOS = iTotalDLSizeAtCurrEOS = iTotalDLHttpBodySize;
}

OSCL_EXPORT_REF uint32 HttpParsingBasicObject::getRedirectURINum()
{
    StrCSumPtrLen aLocation = "Location";
    uint32 numFieldsByKey = iParser->getNumberOfFieldsByKey(aLocation);

    return numFieldsByKey;
}

OSCL_EXPORT_REF bool HttpParsingBasicObject::getRedirectURI(OSCL_String &aRedirectUri, uint32 i)
{
    StrCSumPtrLen aLocation = "Location";
    StrPtrLen url;
    if (iParser->getField(aLocation, url, i))
    {
        if (url.length() > MIN_URL_LENGTH)   // MIN_URL_LENGTH = 1
        {
            // http_parcom views empty header value as ASCII space character 0x20
            // so remove the case of empty header
            aRedirectUri = OSCL_HeapString<OsclMemAllocator> (url.c_str(), url.length());
            return true;
        }
    }
    return false;
}

OSCL_EXPORT_REF bool HttpParsingBasicObject::getContentType(OSCL_String &aContentType)
{
    StrCSumPtrLen aContentTypeKey = "Content-Type";
    StrPtrLen aContentTypeValue;
    if (iParser->getField(aContentTypeKey, aContentTypeValue))
    {
        if (aContentTypeValue.length() > 0)
        {
            aContentType = OSCL_HeapString<OsclMemAllocator> (aContentTypeValue.c_str(), aContentTypeValue.length());
            return true;
        }
    }
    return false;
}

OSCL_EXPORT_REF bool HttpParsingBasicObject::isServerSupportBasicAuthentication()
{
    StrCSumPtrLen aAuthenKey = "WWW-Authenticate";
    uint32 numFieldsByKey = iParser->getNumberOfFieldsByKey(aAuthenKey);
    uint32 i = 0;
    for (i = 0; i < numFieldsByKey; i++)
    {
        StrPtrLen aAuthenValue;
        iParser->getField(aAuthenKey, aAuthenValue, i);
        const char *ptrRealm = aAuthenValue.c_str();
        uint32 len = aAuthenValue.length();
        uint32 length = 0;

        getRealmPtr(ptrRealm, len, length);
        getBasicPtr(aAuthenValue, length);
        if (length >= 6) return true;
    }
    return false;
}

OSCL_EXPORT_REF bool HttpParsingBasicObject::getAuthenInfo(OSCL_String &aRealm)
{
    StrCSumPtrLen aAuthenKey = "WWW-Authenticate";
    uint32 numFieldsByKey = iParser->getNumberOfFieldsByKey(aAuthenKey);
    uint32 i = 0;
    for (i = 0; i < numFieldsByKey; i++)
    {
        StrPtrLen aAuthenValue;
        iParser->getField(aAuthenKey, aAuthenValue, i);
        const char *ptrRealm = aAuthenValue.c_str();
        uint32 len = aAuthenValue.length();
        uint32 length = 0;

        getRealmPtr(ptrRealm, len, length);
        if (len < 6) continue;

        getBasicPtr(aAuthenValue, length);
        if (length < 6) continue;

        ptrRealm += 6;
        len -= 6;
        aRealm = OSCL_HeapString<OsclMemAllocator> (ptrRealm, len);
        return true;
    }
    return false;
}

OSCL_EXPORT_REF void HttpParsingBasicObject::getRealmPtr(const char *&ptrRealm, uint32 &len, uint32 &length)
{
    while (!(((ptrRealm[0]  | OSCL_ASCII_CASE_MAGIC_BIT) == 'r') &&
             ((ptrRealm[1]  | OSCL_ASCII_CASE_MAGIC_BIT) == 'e') &&
             ((ptrRealm[2]  | OSCL_ASCII_CASE_MAGIC_BIT) == 'a') &&
             ((ptrRealm[3]  | OSCL_ASCII_CASE_MAGIC_BIT) == 'l') &&
             ((ptrRealm[4]  | OSCL_ASCII_CASE_MAGIC_BIT) == 'm') &&
             ((ptrRealm[5]  | OSCL_ASCII_CASE_MAGIC_BIT) == '=')) &&
            len >= 6)
    {
        ptrRealm++;
        len--;
        length++;
    }
}

OSCL_EXPORT_REF void HttpParsingBasicObject::getBasicPtr(const StrPtrLen aAuthenValue, uint32 &length)
{
    const char *ptrBasic = aAuthenValue.c_str();
    while (!(((ptrBasic[0]  | OSCL_ASCII_CASE_MAGIC_BIT) == 'b') &&
             ((ptrBasic[1]  | OSCL_ASCII_CASE_MAGIC_BIT) == 'a') &&
             ((ptrBasic[2]  | OSCL_ASCII_CASE_MAGIC_BIT) == 's') &&
             ((ptrBasic[3]  | OSCL_ASCII_CASE_MAGIC_BIT) == 'i') &&
             ((ptrBasic[4]  | OSCL_ASCII_CASE_MAGIC_BIT) == 'c') &&
             ((ptrBasic[5]  | OSCL_ASCII_CASE_MAGIC_BIT) == ' ')) &&
            length >= 6)
    {
        ptrBasic++;
        length--;
    }
}

OSCL_EXPORT_REF bool HttpParsingBasicObject::isServerSendAuthenticationHeader()
{
    StrCSumPtrLen aAuthenKey = "WWW-Authenticate";
    StrPtrLen aAuthenValue;
    if (iParser->getField(aAuthenKey, aAuthenValue))
    {
        if (aAuthenValue.length() > 0)
        {
            return true;
        }
    }
    return false;
}

int32 HttpParsingBasicObject::isNewContentRangeInfoMatchingCurrentOne(const TOsclFileOffset aPrevContentLength)
{
    // First, consider content-length match
    if (aPrevContentLength > 0 && iContentInfo.iContentLength > 0 && aPrevContentLength != iContentInfo.iContentLength)
        return PARSE_CONTENT_LENGTH_NOT_MATCH;

    // if range doesn't support, return false
    if (iContentInfo.iContentRangeRight == 0) return PARSE_CONTENT_RANGE_INFO_NOT_MATCH;

    // Second, consider this case where content-range exists: compare iTotalDLHttpBodySize with content-range_left
    if (iTotalDLHttpBodySize > 0 && iContentInfo.iContentRangeRight > 0)
    {
        if (iTotalDLHttpBodySize != iContentInfo.iContentRangeLeft) return PARSE_CONTENT_RANGE_INFO_NOT_MATCH;
    }

    // for other cases, return true
    return PARSE_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////////
//////  ProtocolState implementation
////////////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF int32 ProtocolState::processMicroState(INPUT_DATA_QUEUE &aDataQueue)
{
    if (iProcessingState == EHttpProcessingMicroState_SendRequest)
    {
        int32 status = doProcessMicroStateSendRequestPreCheck();
        if (status != PROCESS_SUCCESS) return status;
        return doProcessMicroStateSendRequest();
    }
    else if (iProcessingState == EHttpProcessingMicroState_GetResponse)
    {
        int32 status = doProcessMicroStateGetResponsePreCheck();
        if (status != PROCESS_SUCCESS) return status;
        return doProcessMicroStateGetResponse(aDataQueue);
    }
    return PROCESS_SUCCESS;
}

int32 ProtocolState::doProcessMicroStateSendRequestPreCheck()
{
    int32 status = processMicroStateSendRequestPreCheck();
    if (status < 0)
    {
        LOGINFODATAPATH((0, "ProtocolState::processMicroState() processMicroStateSendRequestPreCheck(), error status, errCode=%d", status));
        if (iObserver) iObserver->ProtocolStateError(status);
        return status;
    }
    return PROCESS_SUCCESS;
}

int32 ProtocolState::doProcessMicroStateSendRequest()
{
    int32 status = processMicroStateSendRequest();
    if (status >= 0)
    {
        // SendRequest -> GetResponse automatically
        iProcessingState = EHttpProcessingMicroState_GetResponse;
        // No need to set iNeedGetResponsePreCheck as true.
    }
    if (status != PROCESS_SUCCESS)
    {
        LOGINFODATAPATH((0, "ProtocolState::processMicroState() send request error, errCode=%d", status));
        if (status < 0) iObserver->ProtocolStateError(status);
    }
    return status;
}

int32 ProtocolState::doProcessMicroStateGetResponsePreCheck()
{
    if (iNeedGetResponsePreCheck)
    {
        int32 status = processMicroStateGetResponsePreCheck();
        if (status != PROCESS_SUCCESS)
        {
            LOGINFODATAPATH((0, "ProtocolState::processMicroState() processMicroStateGetResponsePreCheck(), error status, errCode=%d", status));
            iObserver->ProtocolStateError(status);
            return status;
        }
        iNeedGetResponsePreCheck = false;
    }
    return PROCESS_SUCCESS;
}

int32 ProtocolState::doProcessMicroStateGetResponse(INPUT_DATA_QUEUE &aDataQueue)
{
    int32 status = processMicroStateGetResponse(aDataQueue);
    if (isGotEOS(status) || (isHttpHeaderParsed() && is1xxResponse()) || isEndofMessage(status))
    {
        // notify user that data processing at the current state is completely done.
        // user may choose to change to next protocol state
        if (isEndofMessage(status))
        {
            ProtocolStateCompleteInfo aInfo(isDownloadStreamingDoneState(),
                                            isLastState(),
                                            isDownloadStreamingDoneState(),
                                            isMajorProtocolStateComplete());
            iObserver->ProtocolStateComplete(aInfo);
        }
        iNeedGetResponsePreCheck = true;

        if (iRedirect && (!iRedirect->isRedirect()))
        {
            deleteRedirectComposer();
        }

    }

    if (isErrorResponse(status))
    {
        int32 errorCode = status;
        if (status >= 0) errorCode = iParser->getStatusCode();
        storeRedirectUrl(errorCode);
        LOGINFODATAPATH((0, "ProtocolState::processMicroState() error status, errCode=%d", errorCode));
        iObserver->ProtocolStateError(errorCode);
    }
    return status;
}

OSCL_EXPORT_REF int32 ProtocolState::processMicroStateSendRequestPreCheck()
{
    if (!iComposer || !iObserver) return PROCESS_INPUT_OUTPUT_NOT_READY;

    // check good url in terms of parsing correctly to get server address and port
    if (!iURI.isGoodUri()) return PROCESS_BAD_URL;

    // reset composer
    iComposer->reset();

    return PROCESS_SUCCESS;
}

OSCL_EXPORT_REF int32 ProtocolState::processMicroStateSendRequest()
{
    // create output media data to be sent to socket node through port
    PVMFSharedMediaDataPtr mediaData;
    if (!iObserver->GetBufferForRequest(mediaData)) return PROCESS_MEDIA_DATA_CREATE_FAILURE;

    // compose http request
    OsclRefCounterMemFrag fragOut;
    mediaData->getMediaFragment(0, fragOut);
    OsclMemoryFragment memFrag = fragOut.getMemFrag();
    memFrag.len = fragOut.getCapacity();
    int32 status = composeRequest(memFrag);
    if (status != PROCESS_SUCCESS) return status;
    mediaData->setMediaFragFilledLen(0, iComposer->getCurrentRequestLength(iURI.isUseAbsoluteURI())); // don't count NULL

    LOGINFODATAPATH((0, "ProtocolState::processMicroStateSendRequest() HTTP GET :\n------------\n%s\n-----------\n",
                     (char*)memFrag.ptr));

    // send to port
    iObserver->ProtocolRequestAvailable(getProtocolRequestType());

    // set start time for download rate estimation
    iStartTime.set_to_current_time();

    // reset the band width estimation info structure
    // needed for repositioning in progressive streaming
    BandwidthEstimationInfo *pBWEstInfo = iParser->getBandwidthEstimationInfo();
    pBWEstInfo->clear();

    // move to the next state, GetResponse
    iProcessingState = EHttpProcessingMicroState_GetResponse;
    return PROCESS_SUCCESS;
}

int32 ProtocolState::composeRequest(OsclMemoryFragment &aFrag)
{
    // reset composer to compose a new request
    iComposer->reset();

    // set three basic elements: method, url and http version
    setRequestBasics();

    // set fields
    if (!setHeaderFields()) return PROCESS_COMPOSE_HTTP_REQUEST_FAILURE;

    // compose
    return doCompose(aFrag);
}

OSCL_EXPORT_REF int32 ProtocolState::doCompose(OsclMemoryFragment &aFrag)
{
    // compose
    uint32 requestLen = iComposer->getCurrentRequestLength(iURI.isUseAbsoluteURI());
    if (requestLen + 1 > aFrag.len)  return PROCESS_COMPOSE_HTTP_REQUEST_BUFFER_SIZE_NOT_MATCH_REQUEST_SIZE;
    if (iComposer->compose(aFrag, iURI.isUseAbsoluteURI())) return PROCESS_COMPOSE_HTTP_REQUEST_FAILURE;
    return PROCESS_SUCCESS;
}

OSCL_EXPORT_REF int32 ProtocolState::processMicroStateGetResponsePreCheck()
{
    if (!iParser || !iObserver) return PROCESS_INPUT_OUTPUT_NOT_READY;
    iParser->reset();
    return PROCESS_SUCCESS;
}

OSCL_EXPORT_REF int32 ProtocolState::processMicroStateGetResponse(INPUT_DATA_QUEUE &aDataQueue)
{
    if (!iParser->isHttpHeaderParsed())
        setResponseBasics();
    int32 status = iParser->parseResponse(aDataQueue);
    return checkParsingStatus(status);
}

// shared routine for all the download protocols
OSCL_EXPORT_REF int32 ProtocolState::checkParsingStatus(int32 parsingStatus)
{
    // error part
    if (parsingStatus == HttpParsingBasicObject::PARSE_SYNTAX_ERROR) return handleParsingSyntaxError();
    if (parsingStatus == HttpParsingBasicObject::PARSE_GENERAL_ERROR) return PROCESS_GENERAL_ERROR;
    if (parsingStatus == HttpParsingBasicObject::PARSE_BAD_URL) return PROCESS_BAD_URL;
    if (parsingStatus == HttpParsingBasicObject::PARSE_HTTP_VERSION_NOT_SUPPORTED) return PROCESS_HTTP_VERSION_NOT_SUPPORTED;
    if (parsingStatus == HttpParsingBasicObject::PARSE_TRANSFER_ENCODING_NOT_SUPPORTED) return PROCESS_CHUNKED_TRANSFER_ENCODING_NOT_SUPPORT;
    if (parsingStatus < 0)
    {
        return PROCESS_PARSE_HTTP_RESPONSE_FAILURE;
    }

    if (parsingStatus == HttpParsingBasicObject::PARSE_NO_INPUT_DATA) return PROCESS_WAIT_FOR_INCOMING_DATA;
    if (parsingStatus == HttpParsingBasicObject::PARSE_STATUS_LINE_SHOW_NOT_SUCCESSFUL) return PROCESS_SERVER_RESPONSE_ERROR;

    if (parsingStatus == HttpParsingBasicObject::PARSE_SUCCESS ||
            parsingStatus == HttpParsingBasicObject::PARSE_NEED_MORE_DATA ||
            parsingStatus == HttpParsingBasicObject::PARSE_SUCCESS_END_OF_INPUT) return PROCESS_SUCCESS;

    if (parsingStatus == HttpParsingBasicObject::PARSE_SUCCESS_END_OF_MESSAGE) return PROCESS_SUCCESS_END_OF_MESSAGE;
    if (parsingStatus == HttpParsingBasicObject::PARSE_SUCCESS_END_OF_MESSAGE_WITH_EXTRA_DATA) return PROCESS_SUCCESS_END_OF_MESSAGE_WITH_EXTRA_DATA;
    if (parsingStatus == HttpParsingBasicObject::PARSE_EOS_INPUT_DATA) return PROCESS_SUCCESS_GOT_EOS;

    return PROCESS_SUCCESS;
}

int32 ProtocolState::handleParsingSyntaxError()
{
    // If we run into syntax error -2 and the new http header is not available,
    // then ignore the current packet and search the new response packet with http response header
    if (!iParser->isHttpHeaderParsed())
    {
        LOGINFODATAPATH((0, "ProtocolState::handleParsingSyntaxError(), parsing error(-2) + no Http header parsed, IGNORE THE PACKET AND CONTINUE SEARCH!!"));

        // reset the parser
        TOsclFileOffset currDownloadSize = iParser->getDownloadSize();
        iParser->reset();
        iParser->setDownloadSize(currDownloadSize);
        return PROCESS_SUCCESS;
    }

    return PROCESS_PARSE_HTTP_RESPONSE_FAILURE;
}

OSCL_EXPORT_REF uint32 ProtocolState::getDownloadRate()
{
    TimeValue currentTime;
    currentTime.set_to_current_time();

    TimeValue deltaTimeVal = currentTime - iStartTime;
    int32 deltaMilliSec0 = deltaTimeVal.to_msec();

    int32 deltaMilliSec = iParser->getLatestMediaDataTimestamp() - (uint32)iStartTime.to_msec();
    LOGINFODATAPATH((0, "ProtocolState::getDownloadRate(), deltaMilliSec=%d, downloadSize=%d",
                     deltaMilliSec, getDownloadSize()));
    if (deltaMilliSec <= 0) return 0;

    BandwidthEstimationInfo *pBWEstInfo = iParser->getBandwidthEstimationInfo();
    int32 deltaMilliSec1 = pBWEstInfo->iLatestMediaDataTimestamp - pBWEstInfo->iFirstMediaDataTsPerRequest;
    LOGINFODATAPATH((0, "ProtocolState::getDownloadRate(), deltaMilliSec1=%d, downloadSize=%d",
                     deltaMilliSec1, getDownloadSize()));

    if (deltaMilliSec1 <= 0) return 0;

    OsclFloat downloadRate0 = ((OsclFloat)getDownloadSize() / (OsclFloat)deltaMilliSec0) * (OsclFloat)1000.0; // try to avoid overflow problem for 32-bit interger multiplication
    OsclFloat downloadRate  = ((OsclFloat)getDownloadSize() / (OsclFloat)deltaMilliSec) * (OsclFloat)1000.0; // try to avoid overflow problem for 32-bit interger multiplication
    OsclFloat downloadRate1 = ((OsclFloat)(pBWEstInfo->iTotalSizePerRequest) /
                               (OsclFloat)deltaMilliSec1) * (OsclFloat)1000.0; // try to avoid overflow problem for 32-bit interger multiplication

    LOGINFODATAPATH((0, "ProtocolState::getDownloadRate(), deltaMilliSec0=%d, downloadSize=%d, downloadRate0=%ubps",
                     deltaMilliSec0, getDownloadSize(), (uint32)(downloadRate0*8)));

    LOGINFODATAPATH((0, "ProtocolState::getDownloadRate(), deltaMilliSec=%d, downloadSize=%d, downloadRate=%ubps, ",
                     deltaMilliSec, getDownloadSize(), (uint32)(downloadRate*8)));

    LOGINFODATAPATH((0, "ProtocolState::getDownloadRate(), deltaMilliSec1=%d, downloadSize1=%d, downloadRate1=%ubps",
                     deltaMilliSec1, pBWEstInfo->iTotalSizePerRequest, (uint32)(downloadRate1*8)));

    OSCL_UNUSED_ARG(downloadRate0);
    OSCL_UNUSED_ARG(downloadRate); // remove warnings for unused variable
    return (uint32)downloadRate1;
}

uint32 ProtocolState::getDownloadTimeForEstimation()
{
    TimeValue currentTime;
    currentTime.set_to_current_time();

    TimeValue deltaTimeVal = currentTime - iStartTime;
    return (uint32)deltaTimeVal.to_msec();
}

OSCL_EXPORT_REF bool ProtocolState::setExtensionFields(Oscl_Vector<OSCL_HeapString<OsclMemAllocator>, OsclMemAllocator> &aExtensionHeaderKeys,
        Oscl_Vector<OSCL_HeapString<OsclMemAllocator>, OsclMemAllocator> &aExtensionHeaderValues,
        Oscl_Vector<uint32, OsclMemAllocator> &aMaskBitForHTTPMethod,
        Oscl_Vector<bool, OsclMemAllocator> &aExtensionHeadersPurgeOnRedirect,
        const HTTPMethod aMethod)
{
    // no extension headers
    if (aExtensionHeaderKeys.empty() || aExtensionHeaderValues.empty()) return true;
    // number of extension field keys and values doesn't match
    if (aExtensionHeaderKeys.size() != aExtensionHeaderValues.size()) return false;
    if (aMaskBitForHTTPMethod.size() > 0 && (aMaskBitForHTTPMethod.size() != aExtensionHeaderKeys.size())) return false;

    // mask bits for Http method
    uint32 bitMask = getBitMaskForHttpMethod(aMaskBitForHTTPMethod, aMethod);


    for (uint32 i = 0; i < aExtensionHeaderKeys.size(); i++)
    {
        StrCSumPtrLen fieldKey(aExtensionHeaderKeys[i].get_cstr(),
                               aExtensionHeaderKeys[i].get_size());

        StrPtrLen fieldValue(aExtensionHeaderValues[i].get_cstr(),
                             aExtensionHeaderValues[i].get_size());

        bool addExtensionHeader = true;
        if (bitMask > 0) addExtensionHeader = ((aMaskBitForHTTPMethod[i] & bitMask) != 0);
        if (iURI.isRedirectURI() && aExtensionHeadersPurgeOnRedirect[i]) addExtensionHeader = false; // for purge on redirect
        if (addExtensionHeader && !iComposer->setField(fieldKey, &fieldValue)) return false;
    }
    return true;
}

uint32 ProtocolState::getBitMaskForHttpMethod(Oscl_Vector<uint32, OsclMemAllocator> &aMaskBitForHTTPMethod,
        const HTTPMethod aMethod)
{
    uint32 bitMask = 0;
    if (!aMaskBitForHTTPMethod.empty())
    {
        if (aMethod == HTTP_METHOD_GET)  bitMask = MASK_HTTPGET_EXTENSIONHEADER;
        if (aMethod == HTTP_METHOD_POST) bitMask = MASK_HTTPPOST_EXTENSIONHEADER;
        if (aMethod == HTTP_METHOD_HEAD) bitMask = MASK_HTTPHEAD_EXTENSIONHEADER;
    }
    return bitMask;
}

OSCL_EXPORT_REF bool ProtocolState::constructAuthenHeader(OSCL_String &aUserID, OSCL_String &aPasswd)
{
    if (aUserID.get_size() == 0 && aPasswd.get_size() == 0) return true; // empty user and authentication strings

    // set user and authentication for HTTP basic authentication
    const uint32 SIZE = 512;
    char buf[SIZE + 1];
    char *userID = (char*)aUserID.get_cstr();
    char *passwd = (char*)aPasswd.get_cstr();
    oscl_snprintf(buf, SIZE, "%s:%s", userID != NULL ? userID : "", passwd != NULL ? passwd : "");
    char base64buf[(SIZE<<1)+1], *ptr = (char*)base64buf;
    OSCL_FastString basicString(_STRLIT_CHAR("Basic "));
    oscl_memcpy(ptr, basicString.get_cstr(), basicString.get_size());
    ptr += basicString.get_size();
    base64enc(buf, ptr);
    StrCSumPtrLen auth = "Authorization";
    return iComposer->setField(auth, base64buf);
}


// subroutine for base64 encoding (used in HTTP Basic authentification)
const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
int32 ProtocolState::base64enc(char *data, char *out)
{
    int32 len = oscl_strlen(data);

    int32 i, index;
    int32 val;

    for (i = 0, index = 0; i < len; i += 3, index += 4)
    {
        int32 quad = 0, trip = 0;

        val = (0xff & data[i]);
        val <<= 8;
        if ((i + 1) < len)
        {
            val |= (0xff & data[i+1]);
            trip = 1;
        }
        val <<= 8;
        if ((i + 2) < len)
        {
            val |= (0xff & data[i+2]);
            quad = 1;
        }
        out[index+3] = alphabet[(quad ? (val & 0x3f) : 64)];
        val >>= 6;
        out[index+2] = alphabet[(trip ? (val & 0x3f) : 64)];
        val >>= 6;
        out[index+1] = alphabet[val & 0x3f];
        val >>= 6;
        out[index+0] = alphabet[val & 0x3f];
    }

    //out[++index] = 0;
    out[index] = 0;
    return index;
}

void ProtocolState::storeRedirectUrl(int32 errorCode)
{
    if ((PROTOCOLENGINE_REDIRECT_STATUS_CODE_START <= errorCode)
            && (errorCode <= PROTOCOLENGINE_REDIRECT_STATUS_CODE_END))
    {
        if (NULL == iRedirect)
        {
            iRedirect = OSCL_NEW(RedirectComposer, (iParser));
        }
        if (NULL != iRedirect)
        {
            iRedirect->storeRedirectUrl();
        }
        return;
    }
    deleteRedirectComposer();
}

bool ProtocolState::getRedirectURI(OSCL_String &aRedirectUri)
{
    if (NULL != iRedirect)
    {
        return iRedirect->getRedirectURI(aRedirectUri);
    }
    else
    {
        return false;
    }
}

bool ProtocolState::getAllRedirectURI(Oscl_Vector<OSCL_HeapString<OsclMemAllocator>, OsclMemAllocator> &aRedirectVec)
{
    if (NULL != iRedirect)
    {
        return iRedirect->getAllRedirectURI(aRedirectVec);
    }
    else
    {
        return false;
    }
}


OSCL_EXPORT_REF void ProtocolState::deleteRedirectComposer()
{
    if (iRedirect)
    {
        OSCL_DELETE(iRedirect);
        iRedirect = NULL;
    }
}

bool HttpBasedProtocol::updateSeekMode()
{
    /* Set byte-seek param inside PE node, based on HEAD request response check in case when byte-seek
    param was not set by the App inside the context data. */
    if ((iComposer->getMethod() == HTTP_METHOD_HEAD) &&
            (getByteSeekMode() == BYTE_SEEK_NOTSET))
    {
        /* Check whether protocolInfo is present inside the HEAD request's response i.e. we check
           whether the header: 'contentFeatures.dlna.org' is present inside the response. */
        StrCSumPtrLen protocolInfokey = "contentFeatures.dlna.org";
        StrPtrLen protocolInfoValue;
        iParser->getHttpParser()->getField(protocolInfokey, protocolInfoValue);
        if (protocolInfoValue.size() <= 0)
        {
            // If protocolInfo header not present inside the response, we check for the presence of header: "Accept-Ranges".
            StrCSumPtrLen checkRangesHeaderKey = "Accept-Ranges";
            StrPtrLen checkRangesHeaderValue;
            iParser->getHttpParser()->getField(checkRangesHeaderKey, checkRangesHeaderValue);
            if (checkRangesHeaderValue.size() <= 0)
            {
                // If "Accept-Ranges" header also not present, we disable the byte-seek param inside PE node.
                setByteSeekMode(BYTE_SEEK_UNSUPPORTED);
            }
            else
            {
                if (oscl_strcmp((char *)checkRangesHeaderValue.c_str(), "bytes") == 0)
                {
                    // If "Accept-Ranges: bytes" present, we enable the byte-seek param inside the PE node.
                    setByteSeekMode(BYTE_SEEK_SUPPORTED);
                }
                else
                {
                    setByteSeekMode(BYTE_SEEK_UNSUPPORTED);
                }
            }
        }
        else
        {
            if (oscl_strstr((char *)protocolInfoValue.c_str(), "DLNA.ORG_OP=01") != NULL)
            {
                /* If protocolInfo header present with 'DLNA.ORG_OP=01' as one of it's value,
                   we enable the byte-seek param inside the PE node. */
                setByteSeekMode(BYTE_SEEK_SUPPORTED);
            }
            else
            {
                setByteSeekMode(BYTE_SEEK_UNSUPPORTED);
            }

        }

    }
    return true;
}

void RedirectComposer::storeRedirectUrl()
{
    OSCL_HeapString<OsclMemAllocator> newUrl;

    int32 errorCode = iParser->getStatusCode();
    if ((iParser->getHttpVersionNum() == HTTP_V11) && (errorCode == Response305StatusCode))
    {
        iRedirectUrlVec.clear();
        iCntNum = 0;
    }
    uint32 redirectUrlNum = iParser->getRedirectURINum();

    for (uint32 i = 0; i < redirectUrlNum ; i++)
    {
        if (iParser->getRedirectURI(newUrl, i))
        {
            if (validateUrl(newUrl))
            {
                iRedirectUrlVec.push_back(newUrl);
            }
        }
        else
        {
            iRedirectUrlVec.clear();
            iCntNum = 0;
        }
    }

    return;
}

bool RedirectComposer::validateUrl(OSCL_HeapString<OsclMemAllocator> newUrl)
{
    Oscl_Vector<OSCL_HeapString<OsclMemAllocator>, OsclMemAllocator>::iterator it;

    for (it = iRedirectUrlVec.begin(); it < iRedirectUrlVec.end(); it++)
    {
        if (!oscl_strcmp(newUrl.get_str(), it->get_str()))
        {
            return false;
        }
    }
    return true;
}

bool RedirectComposer::getRedirectURI(OSCL_String &aRedirectUri)
{
    if (iCntNum < iRedirectUrlVec.size())
    {
        aRedirectUri = iRedirectUrlVec[iCntNum++];
        return true;
    }
    return false;
}

bool RedirectComposer::getAllRedirectURI(Oscl_Vector<OSCL_HeapString<OsclMemAllocator>, OsclMemAllocator> &aRedirectVec)
{
    if (iCntNum < iRedirectUrlVec.size())
    {
        uint32 i = iCntNum;
        for (; i < iRedirectUrlVec.size(); i++)
        {
            aRedirectVec.push_back(iRedirectUrlVec[i]);
        }
        return true;
    }
    return false;
}

bool RedirectComposer::deleteAllRedirectURI()
{
    if (!iRedirectUrlVec.empty()) iRedirectUrlVec.clear();
    return true;
}

bool RedirectComposer::isRedirect()
{
    uint32 errorCode = iParser->getStatusCode();
    if ((errorCode >= PROTOCOLENGINE_REDIRECT_STATUS_CODE_START) || (PROTOCOLENGINE_REDIRECT_STATUS_CODE_END >= errorCode))
    {
        return true;
    }
    return false;
}

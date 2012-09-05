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
/**
* @file pvmi_mio_fileinput.cpp
* @brief PV Media IO interface implementation using file input
*/

#include "oscl_base.h"
#include "pvmi_mio_fileinput_factory.h"
#include "pvmi_mio_fileinput.h"
#include "pv_mime_string_utils.h"
#include "oscl_dll.h"
#include "media_clock_converter.h"

#ifdef TEXT_TRACK_DESC_INFO
#include "textsampledescinfo.h"
#endif
// Define entry point for this DLL
OSCL_DLL_ENTRY_POINT_DEFAULT()

#define PVMIOFILEIN_MEDIADATA_POOLNUM 8
const uint32 AMR_FRAME_DELAY = 20; // 20ms
#define PVMIOFILEIN_MAX_FSI_SIZE 1024

// Logging macros
#define LOG_STACK_TRACE(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, m)
#define LOG_DEBUG(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, m)
#define LOG_ERR(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_REL,iLogger,PVLOGMSG_ERR,m)

OSCL_EXPORT_REF PvmiMIOControl* PvmiMIOFileInputFactory::Create(const PvmiMIOFileInputSettings& arSettings)
{
    PvmiMIOControl* pmioFilein = (PvmiMIOControl*) new PvmiMIOFileInput(arSettings);

    return pmioFilein;
}

OSCL_EXPORT_REF bool PvmiMIOFileInputFactory::Delete(PvmiMIOControl* apMio)
{
    PvmiMIOFileInput *pmioFilein = (PvmiMIOFileInput*)apMio;
    if (!pmioFilein)
        return false;
    delete pmioFilein;

    pmioFilein = NULL;
    return true;

}

////////////////////////////////////////////////////////////////////////////
PvmiMIOFileInput::~PvmiMIOFileInput()
{
    if (iMediaBufferMemPool)
    {
        OSCL_DELETE(iMediaBufferMemPool);
        iMediaBufferMemPool = NULL;
    }
    if (configinfo)
    {
        oscl_free(configinfo);
        configinfo = NULL;
    }
    DoReset();
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PvmiMIOFileInput::connect(PvmiMIOSession& arSession,
        PvmiMIOObserver* apObserver)
{
    if (!apObserver)
    {
        return PVMFFailure;
    }

    int32 err = 0;
    OSCL_TRY(err, iObservers.push_back(apObserver));
    OSCL_FIRST_CATCH_ANY(err, return PVMFErrNoMemory);
    arSession = (PvmiMIOSession)(iObservers.size() - 1); // Session ID is the index of observer in the vector
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PvmiMIOFileInput::disconnect(PvmiMIOSession aSession)
{
    uint32 index = (uint32)aSession;
    if (index >= iObservers.size())
    {
        // Invalid session ID
        return PVMFFailure;
    }

    iObservers.erase(iObservers.begin() + index);
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PvmiMediaTransfer* PvmiMIOFileInput::createMediaTransfer(PvmiMIOSession& arSession,
        PvmiKvp* pread_formats,
        int32 read_flags,
        PvmiKvp* pwrite_formats,
        int32 write_flags)
{
    OSCL_UNUSED_ARG(pread_formats);
    OSCL_UNUSED_ARG(read_flags);
    OSCL_UNUSED_ARG(pwrite_formats);
    OSCL_UNUSED_ARG(write_flags);

    uint32 index = (uint32)arSession;
    if (index >= iObservers.size())
    {
        // Invalid session ID
        OSCL_LEAVE(OsclErrArgument);
        return NULL;
    }

    return (PvmiMediaTransfer*)this;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmiMIOFileInput::deleteMediaTransfer(PvmiMIOSession& arSession,
        PvmiMediaTransfer* pmedia_transfer)
{
    uint32 index = (uint32)arSession;
    if (!pmedia_transfer || index >= iObservers.size())
    {
        // Invalid session ID
        OSCL_LEAVE(OsclErrArgument);
    }
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmiMIOFileInput::QueryInterface(const PVUuid& arUuid,
        PVInterface*& aInterfacePtr,
        const OsclAny* apContext)
{
    if (arUuid == PVMI_CAPABILITY_AND_CONFIG_PVUUID)
    {
        PvmiCapabilityAndConfig* myInterface = OSCL_STATIC_CAST(PvmiCapabilityAndConfig*, this);
        aInterfacePtr = OSCL_STATIC_CAST(PVInterface*, myInterface);
    }
    else
    {
        aInterfacePtr = NULL;
    }

    return AddCmdToQueue(CMD_QUERY_INTERFACE, apContext, (OsclAny*)&aInterfacePtr);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmiMIOFileInput:: Init(const OsclAny* apContext)
{
    if ((iState != STATE_IDLE) && (iState != STATE_INITIALIZED))
    {
        OSCL_LEAVE(OsclErrInvalidState);
        return -1;
    }

    return AddCmdToQueue(CMD_INIT, apContext);
}


////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmiMIOFileInput::Start(const OsclAny* apContext)
{
    if (iState != STATE_INITIALIZED
            && iState != STATE_PAUSED
            && iState != STATE_STARTED)
    {
        OSCL_LEAVE(OsclErrInvalidState);
        return -1;
    }

    return AddCmdToQueue(CMD_START, apContext);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmiMIOFileInput::Pause(const OsclAny* apContext)
{
    if (iState != STATE_STARTED && iState != STATE_PAUSED)
    {
        OSCL_LEAVE(OsclErrInvalidState);
        return -1;
    }

    return AddCmdToQueue(CMD_PAUSE, apContext);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmiMIOFileInput::Flush(const OsclAny* apContext)
{
    if (iState != STATE_STARTED || iState != STATE_PAUSED)
    {
        OSCL_LEAVE(OsclErrInvalidState);
        return -1;
    }

    return AddCmdToQueue(CMD_FLUSH, apContext);
}

OSCL_EXPORT_REF PVMFCommandId PvmiMIOFileInput::Reset(const OsclAny* apContext)
{
    return AddCmdToQueue(CMD_RESET, apContext);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmiMIOFileInput::DiscardData(PVMFTimestamp aTimestamp,
        const OsclAny* apContext)
{
    OSCL_UNUSED_ARG(apContext);
    OSCL_UNUSED_ARG(aTimestamp);
    OSCL_LEAVE(OsclErrNotSupported);
    return -1;
}

OSCL_EXPORT_REF PVMFCommandId PvmiMIOFileInput::DiscardData(const OsclAny* apContext)
{
    OSCL_UNUSED_ARG(apContext);
    OSCL_LEAVE(OsclErrNotSupported);
    return -1;
}


////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmiMIOFileInput::Stop(const OsclAny* apContext)
{
    if (iState != STATE_STARTED
            && iState != STATE_PAUSED
            && iState != STATE_STOPPED)
    {
        OSCL_LEAVE(OsclErrInvalidState);
        return -1;
    }

    return AddCmdToQueue(CMD_STOP, apContext);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmiMIOFileInput::ThreadLogon()
{
    if (!iThreadLoggedOn)
    {
        AddToScheduler();
        iLogger = PVLogger::GetLoggerObject("PvmiMIOFileInput");
        iThreadLoggedOn = true;
    }
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmiMIOFileInput::ThreadLogoff()
{
    if (iThreadLoggedOn)
    {
        RemoveFromScheduler();
        iLogger = NULL;
        iThreadLoggedOn = false;
    }
}

void PvmiMIOFileInput::CloseInputFile()
{
    if (iFileOpened)
    {
        iInputFile.Close();
        iFileOpened = false;
    }

    if (iFsOpened)
    {
        iFs.Close();
        iFsOpened = false;
    }
}


////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmiMIOFileInput::CancelAllCommands(const OsclAny* apContext)
{
    OSCL_UNUSED_ARG(apContext);
    OSCL_LEAVE(OsclErrNotSupported);
    return -1;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmiMIOFileInput::CancelCommand(PVMFCommandId aCmdId,
        const OsclAny* apContext)
{
    OSCL_UNUSED_ARG(aCmdId);
    OSCL_UNUSED_ARG(apContext);
    OSCL_LEAVE(OsclErrNotSupported);
    return -1;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmiMIOFileInput::setPeer(PvmiMediaTransfer* apPeer)
{
    iPeer = apPeer;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmiMIOFileInput::useMemoryAllocators(OsclMemAllocator* pwrite_alloc)
{
    OSCL_UNUSED_ARG(pwrite_alloc);
    OSCL_LEAVE(OsclErrNotSupported);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmiMIOFileInput::writeAsync(uint8 aFormatType,
        int32 aFormatIndex,
        uint8* aData, uint32 aDataLen,
        const PvmiMediaXferHeader& adata_header_info,
        OsclAny* apContext)
{
    OSCL_UNUSED_ARG(aFormatType);
    OSCL_UNUSED_ARG(aFormatIndex);
    OSCL_UNUSED_ARG(aData);
    OSCL_UNUSED_ARG(aDataLen);
    OSCL_UNUSED_ARG(adata_header_info);
    OSCL_UNUSED_ARG(apContext);
    // This is an active data source. writeAsync is not supported.
    OSCL_LEAVE(OsclErrNotSupported);
    return -1;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmiMIOFileInput::writeComplete(PVMFStatus aStatus,
        PVMFCommandId write_cmd_id,
        OsclAny* apContext)
{
    OSCL_UNUSED_ARG(apContext);
    if ((aStatus != PVMFSuccess) && (aStatus != PVMFErrCancelled))
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "PvmiMIOFileInput::writeComplete: Error - writeAsync failed. aStatus=%d",
                         aStatus));
        OSCL_LEAVE(OsclErrGeneral);
    }

    for (int i = iSentMediaData.size() - 1; i >= 0; i--)
    {
        if (iSentMediaData[i].iId == write_cmd_id)
        {

#ifdef TEXT_TRACK_DESC_INFO
            if (iSentMediaData[i].iNotification)
            {
                PvmiKvp* textKvp = OSCL_STATIC_CAST(PvmiKvp*,
                                                    iSentMediaData[i].iData);
                if (iSettings.iMediaFormat == PVMF_MIME_ISO_AVC_SAMPLE_FORMAT)
                {
                    uint8* textInfo =
                        OSCL_STATIC_CAST(uint8*,
                                         textKvp->value.key_specific_value);
                    OSCL_DELETE(textInfo);
                    textInfo = NULL;
                }
                else
                {
                    PVA_FF_TextSampleDescInfo* textInfo =
                        OSCL_STATIC_CAST(PVA_FF_TextSampleDescInfo*,
                                         textKvp->value.key_specific_value);
                    OSCL_DELETE(textInfo);
                    textInfo = NULL;
                }

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "PvmiMIOFileInput::writeComplete: aStatus=%d, CmdId=%d, Ptr=0x%x",
                                 aStatus, write_cmd_id, iSentMediaData[i].iData));
                iAlloc.deallocate(iSentMediaData[i].iData);
                iSentMediaData.erase(&iSentMediaData[i]);
                return;
            }
            else
#endif

            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "PvmiMIOFileInput::writeComplete: aStatus=%d, CmdId=%d, Ptr=0x%x",
                                 aStatus, write_cmd_id, iSentMediaData[i].iData));
                iMediaBufferMemPool->deallocate(iSentMediaData[i].iData);
                iSentMediaData.erase(&iSentMediaData[i]);
                return;
            }
        }
    }

    // Error: unmatching ID.
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                    (0, "PvmiMIOFileInput::writeComplete: Error - unmatched cmdId %d failed. QSize %d",
                     write_cmd_id, iSentMediaData.size()));
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmiMIOFileInput::readAsync(uint8* apData,
        uint32 amax_data_len,
        OsclAny* apContext, int32* apformats, uint16 anum_formats)
{
    OSCL_UNUSED_ARG(apData);
    OSCL_UNUSED_ARG(amax_data_len);
    OSCL_UNUSED_ARG(apContext);
    OSCL_UNUSED_ARG(apformats);
    OSCL_UNUSED_ARG(anum_formats);
    // This is an active data source. readAsync is not supported.
    OSCL_LEAVE(OsclErrNotSupported);
    return -1;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmiMIOFileInput::readComplete(PVMFStatus aStatus,
        PVMFCommandId read_cmd_id,
        int32 format_index,
        const PvmiMediaXferHeader& ardata_header_info,
        OsclAny* apContext)
{
    OSCL_UNUSED_ARG(aStatus);
    OSCL_UNUSED_ARG(read_cmd_id);
    OSCL_UNUSED_ARG(format_index);
    OSCL_UNUSED_ARG(ardata_header_info);
    OSCL_UNUSED_ARG(apContext);
    // This is an active data source. readComplete is not supported.
    OSCL_LEAVE(OsclErrNotSupported);
    return;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmiMIOFileInput::statusUpdate(uint32 status_flags)
{
    if (status_flags == PVMI_MEDIAXFER_STATUS_WRITE)
    {
        iMicroSecondsPerDataEvent = 0;
        AddDataEventToQueue(iMicroSecondsPerDataEvent);
    }
    else
    {
        // Ideally this routine should update the status of media input component.
        // It should check then for the status. If media input buffer is consumed,
        // media input object should be resheduled.
        // Since the Media fileinput component is designed with single buffer, two
        // asynchronous reads are not possible. So this function will not be required
        // and hence not been implemented.
        OSCL_LEAVE(OsclErrNotSupported);
    }
}


////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmiMIOFileInput::cancelCommand(PVMFCommandId aCmdId)
{
    OSCL_UNUSED_ARG(aCmdId);
    // This cancel command ( with a small "c" in cancel ) is for the media transfer interface.
    // implementation is similar to the cancel command of the media I/O interface.
    OSCL_LEAVE(OsclErrNotSupported);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmiMIOFileInput::cancelAllCommands()
{
    OSCL_LEAVE(OsclErrNotSupported);
}


////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PvmiMIOFileInput::getParametersSync(PvmiMIOSession session,
        PvmiKeyType identifier,
        PvmiKvp*& parameters,
        int& arNumParameterElements,
        PvmiCapabilityContext context)
{
    LOG_STACK_TRACE((0, "PvmiMIOFileInput::getParametersSync"));
    OSCL_UNUSED_ARG(session);
    OSCL_UNUSED_ARG(context);

    parameters = NULL;
    arNumParameterElements = 0;
    PVMFStatus status = PVMFFailure;

    if (pv_mime_strcmp(identifier, OUTPUT_FORMATS_CAP_QUERY) == 0 ||
            pv_mime_strcmp(identifier, OUTPUT_FORMATS_CUR_QUERY) == 0)
    {
        arNumParameterElements = 1;
        status = AllocateKvp(parameters,
                             (PvmiKeyType)OUTPUT_FORMATS_VALTYPE,
                             arNumParameterElements);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PvmiMIOFileInput::GetOutputParametersSync: Error - AllocateKvp failed. status=%d",
                     status));
        }
        else
        {
            parameters[0].value.pChar_value = (char*)iSettings.iMediaFormat.getMIMEStrPtr();
        }
    }
    else if (pv_mime_strcmp(identifier, VIDEO_OUTPUT_WIDTH_CUR_QUERY) == 0)
    {
        arNumParameterElements = 1;
        status = AllocateKvp(parameters,
                             (PvmiKeyType)VIDEO_OUTPUT_WIDTH_CUR_VALUE,
                             arNumParameterElements);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PvmiMIOFileInput::GetOutputParametersSync: Error - AllocateKvp failed. status=%d",
                     status));
            return status;
        }

        parameters[0].value.uint32_value = iSettings.iFrameWidth;
    }
    else if (pv_mime_strcmp(identifier, VIDEO_OUTPUT_HEIGHT_CUR_QUERY) == 0)
    {
        arNumParameterElements = 1;
        status = AllocateKvp(parameters,
                             (PvmiKeyType)VIDEO_OUTPUT_HEIGHT_CUR_VALUE,
                             arNumParameterElements);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PvmiMIOFileInput::GetOutputParametersSync: Error - AllocateKvp failed. status=%d",
                     status));
            return status;
        }

        parameters[0].value.uint32_value = iSettings.iFrameHeight;
    }
    else if (pv_mime_strcmp(identifier, VIDEO_OUTPUT_FRAME_RATE_CUR_QUERY) == 0)
    {
        arNumParameterElements = 1;
        status = AllocateKvp(parameters,
                             (PvmiKeyType)VIDEO_OUTPUT_FRAME_RATE_CUR_VALUE,
                             arNumParameterElements);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PvmiMIOFileInput::GetOutputParametersSync: Error - AllocateKvp failed. status=%d",
                     status));
            return status;
        }

        parameters[0].value.float_value = iSettings.iFrameRate;
    }
    else if (pv_mime_strcmp(identifier, OUTPUT_TIMESCALE_CUR_QUERY) == 0)
    {
        arNumParameterElements = 1;
        status = AllocateKvp(parameters,
                             (PvmiKeyType)OUTPUT_TIMESCALE_CUR_VALUE,
                             arNumParameterElements);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PvmiMIOFileInput::GetOutputParametersSync: Error - AllocateKvp failed. status=%d",
                     status));
            return status;
        }
        else
        {
            if (iSettings.iMediaFormat.isAudio())
            {
                parameters[0].value.uint32_value = iSettings.iSamplingFrequency;
            }
            else
            {
                parameters[0].value.uint32_value = iSettings.iTimescale;
            }
        }
    }
    else if (pv_mime_strcmp(identifier, OUTPUT_BITRATE_CUR_QUERY) == 0)
    {
        arNumParameterElements = 1;
        status = AllocateKvp(parameters,
                             (PvmiKeyType)OUTPUT_BITRATE_CUR_VALUE,
                             arNumParameterElements);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PvmiMIOFileInput::GetOutputParametersSync: Error - AllocateKvp failed. status=%d",
                     status));
            return status;
        }
        parameters[0].value.uint32_value = iSettings.iAverageBitRate;
    }
    else if (pv_mime_strcmp(identifier, PVMF_FORMAT_SPECIFIC_INFO_KEY) == 0)
    {
        status = PVMFSuccess;
        if (iFSIKvp == NULL)
        {
            status = RetrieveFSI();
        }
        if (status != PVMFSuccess)
        {
            return status;
        }

        arNumParameterElements = 1;
        status = AllocateKvp(parameters,
                             (PvmiKeyType)PVMF_FORMAT_SPECIFIC_INFO_KEY,
                             arNumParameterElements);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PvmiMIOFileInput::GetOutputParametersSync: Error - AllocateKvp failed. status=%d",
                     status));
            return status;
        }
        else
        {
            parameters[0].value.key_specific_value = iFSIKvp->value.key_specific_value;
            parameters[0].capacity = iFSIKvp->capacity;
            parameters[0].length = iFSIKvp->length;
        }

    }
    else if (pv_mime_strcmp(identifier, AUDIO_OUTPUT_SAMPLING_RATE_CUR_QUERY) == 0)
    {
        arNumParameterElements = 1;
        status = AllocateKvp(parameters,
                             (PvmiKeyType)AUDIO_OUTPUT_SAMPLING_RATE_CUR_QUERY,
                             arNumParameterElements);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PvmiMIOFileInput::GetOutputParametersSync: Error - AllocateKvp failed. status=%d",
                     status));
            return status;
        }
        parameters[0].value.uint32_value = iSettings.iSamplingFrequency;
    }
    else if (pv_mime_strcmp(identifier, AUDIO_OUTPUT_NUM_CHANNELS_CUR_QUERY) == 0)
    {
        arNumParameterElements = 1;
        status = AllocateKvp(parameters,
                             (PvmiKeyType)AUDIO_OUTPUT_NUM_CHANNELS_CUR_QUERY,
                             arNumParameterElements);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PvmiMIOFileInput::GetOutputParametersSync: Error - AllocateKvp failed. status=%d",
                     status));
            return status;
        }
        parameters[0].value.uint32_value = iSettings.iNumChannels;
    }
    else if (pv_mime_strcmp(identifier,
                            PVMF_AVC_SAMPLE_NAL_SIZE_IN_BYTES_VALUE_KEY) == 0)
    {
        arNumParameterElements = 1;
        status = AllocateKvp(parameters,
                             (PvmiKeyType)PVMF_AVC_SAMPLE_NAL_SIZE_IN_BYTES_VALUE_KEY,
                             arNumParameterElements);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PvmiMIOFileInput::GetOutputParametersSync: Error - AllocateKvp failed. status=%d", status));
            return status;
        }
        if (iNALType == 0)
        {
            RetrieveNALType();
        }
        parameters[0].value.uint32_value = (uint32)iNALType;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PvmiMIOFileInput::releaseParameters(PvmiMIOSession session,
        PvmiKvp* parameters,
        int num_elements)
{
    OSCL_UNUSED_ARG(session);
    OSCL_UNUSED_ARG(num_elements);

    if (parameters)
    {
        iAlloc.deallocate((OsclAny*)parameters);
        return PVMFSuccess;
    }
    else
    {
        return PVMFFailure;
    }
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmiMIOFileInput::setParametersSync(PvmiMIOSession aSession,
        PvmiKvp* apParameters,
        int anum_elements, PvmiKvp*& aret_kvp)
{
    OSCL_UNUSED_ARG(aSession);
    PVMFStatus status = PVMFSuccess;
    aret_kvp = NULL;

    for (int32 i = 0; i < anum_elements; i++)
    {
        status = VerifyAndSetParameter(&(apParameters[i]), true);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PvmiMIOFileInput::setParametersSync: Error - VerifiyAndSetParameter failed on parameter #%d", i));
            aret_kvp = &(apParameters[i]);
            OSCL_LEAVE(OsclErrArgument);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF uint32 PvmiMIOFileInput::GetStreamDuration()
{
    return iStreamDuration;
}
////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmiMIOFileInput::SetAuthoringDuration(uint32 duration)
{
    iAuthoringDuration = duration;
}


////////////////////////////////////////////////////////////////////////////
//                            Private methods
////////////////////////////////////////////////////////////////////////////
PvmiMIOFileInput::PvmiMIOFileInput(const PvmiMIOFileInputSettings& arSettings)
        : OsclTimerObject(OsclActiveObject::EPriorityNominal, "PvmiMIOFileInput"),
        iCmdIdCounter(0),
        iPeer(NULL),
        iThreadLoggedOn(false),
        iSettings(arSettings),
        iFsOpened(false),
        iFileOpened(false),
        iFsOpened_log(false),
        iFileOpened_log(false),
        iFsOpened_text(false),
        iFileOpened_text(false),
        iTimed_Text_configinfo(false),
        iDataEventCounter(0),
        iTotalNumFrames(0),
        iFileHeaderSize(0),
        iTimeStamp(0),
        iReadTimeStamp(0),
        iPreTS(0),
        iCount(0),
        iMediaBufferMemPool(NULL),
        iNotificationID(0),
        iLogger(NULL),
        iState(STATE_IDLE),
        iAuthoringDuration(0),
        iStreamDuration(0),
        iStartTimestamp(0),
        iFormatSpecificInfoSize(0),
        iFSIKvp(NULL),
        iConfigSize(0),
        configinfo(NULL),
        iNALType(0),
        iPeerCapConfig(NULL)
{

}

////////////////////////////////////////////////////////////////////////////
void PvmiMIOFileInput::Run()
{
    if (!iCmdQueue.empty())
    {
        PvmiMIOFileInputCmd cmd = iCmdQueue[0];
        iCmdQueue.erase(iCmdQueue.begin());

        switch (cmd.iType)
        {

            case CMD_INIT:
                DoRequestCompleted(cmd, DoInit());
                break;

            case CMD_START:
                DoRequestCompleted(cmd, DoStart());
                break;

            case CMD_PAUSE:
                DoRequestCompleted(cmd, DoPause());
                break;

            case CMD_FLUSH:
                DoRequestCompleted(cmd, DoFlush());
                break;

            case CMD_RESET:
                DoRequestCompleted(cmd, DoReset());
                break;

            case CMD_STOP:
                DoRequestCompleted(cmd, DoStop());
                break;

            case DATA_EVENT:
                if (PVMFSuccess != DoRead())
                {
                    iStartTimestamp = iTimeStamp;
                    iStartTime.set_to_current_time();
                }
                break;

            case CMD_QUERY_INTERFACE:
                DoRequestCompleted(cmd, PVMFSuccess);
                break;

            case CMD_CANCEL_ALL_COMMANDS:
            case CMD_CANCEL_COMMAND:
                DoRequestCompleted(cmd, PVMFFailure);
                break;

            default:
                break;
        }
    }

    if (!iCmdQueue.empty())
    {
        // Run again if there are more things to process
        RunIfNotReady();
    }
}

////////////////////////////////////////////////////////////////////////////
PVMFCommandId PvmiMIOFileInput::AddCmdToQueue(PvmiMIOFileInputCmdType aType,
        const OsclAny* apContext, OsclAny* apData1)
{
    if (aType == DATA_EVENT)
        OSCL_LEAVE(OsclErrArgument);

    PvmiMIOFileInputCmd cmd;
    cmd.iType = aType;
    cmd.iContext = OSCL_STATIC_CAST(OsclAny*, apContext);
    cmd.iData1 = apData1;
    cmd.iId = iCmdIdCounter;
    ++iCmdIdCounter;
    iCmdQueue.push_back(cmd);
    RunIfNotReady();
    return cmd.iId;
}

////////////////////////////////////////////////////////////////////////////
void PvmiMIOFileInput::AddDataEventToQueue(uint32 aMicroSecondsToEvent)
{
    PvmiMIOFileInputCmd cmd;
    cmd.iType = DATA_EVENT;
    iCmdQueue.push_back(cmd);
    RunIfNotReady(aMicroSecondsToEvent);
}

////////////////////////////////////////////////////////////////////////////
void PvmiMIOFileInput::DoRequestCompleted(const PvmiMIOFileInputCmd& arCmd,
        PVMFStatus aStatus,
        OsclAny* apEventData)
{
    PVMFCmdResp response(arCmd.iId, arCmd.iContext, aStatus, apEventData);

    for (uint32 i = 0; i < iObservers.size(); i++)
        iObservers[i]->RequestCompleted(response);
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmiMIOFileInput::DoInit()
{
    if (STATE_INITIALIZED == iState)
    {
        return PVMFSuccess;
    }

    if (!iFsOpened)
    {
        if (iFs.Connect() != 0)
            return PVMFFailure;
        iFsOpened = true;
    }

    if (iFileOpened ||
            0 != iInputFile.Open(iSettings.iFileName.get_cstr(),
                                 Oscl_File::MODE_READ | Oscl_File::MODE_BINARY, iFs))
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "PvmiMIOFileInput::DoInit: Error - iInputFile.Open failed"));
        return PVMFFailure;
    }

    iFileOpened = true;
    uint32 maxFrameSize = 0;
    uint32 fileSize = 0;
    uint32 fileStart = 0;
    uint32 fileEnd = 0;
    fileStart = iInputFile.Tell();
    iInputFile.Seek(0, Oscl_File::SEEKEND);
    fileEnd = iInputFile.Tell();
    iInputFile.Seek(fileStart, Oscl_File::SEEKSET);
    fileSize = fileEnd - fileStart;
    if (fileSize <= 0)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "PvmiMIOFileInput::DoInit: Error - iInputFile empty"));
        return PVMFFailure;
    }

    iMicroSecondsPerDataEvent = 0;
    iFrameSizeVector.reserve(2500);
    iTimeStampVector.reserve(2500);

    /*
    * For some of the compressed formats we expect a log file to be present.
    * The syntax of the logfile is as follows:
    *********************************************************
    unsigned int(32) total_num_samples;
    unsigned int(32) avg_bitrate; // could be 0 if not available
    unsigned int(32) timescale; //this is the timescale of all sample timestamps below
    unsigned int(32) max_sample_size; // could be 0 if not available
    unsigned int(32) config_size; //could be 0 for streams that have no config, say AMR
    unsigned int(32) height; //zero if it is audio stream
    unsigned int(32) width; //zero if it is audio stream
    unsigned int(32) frame_rate; //zero if audio stream
    for (j=0; j < total_num_samples; j++)
    {
    unsigned int(32) sample_length_in_bytes;
    unsigned int(32) sample_timestamp;
    }
    *********************************************************
    * for some others like AMR & AMR-WB we do not need log files
    * For M4V and H263, log file is optional. Author unit test will
    * always use one, but there could be other test apps that use
    * this media input comp that may not pass the log file.
    */
    if (iSettings.iMediaFormat == PVMF_MIME_ISO_AVC_SAMPLE_FORMAT)
    {
        // Validate settings - even when using logfile we assume the test app
        // has parsed the log file to populate iSettings correcrtly
        if (iSettings.iFrameHeight <= 0 || iSettings.iFrameWidth <= 0 ||
                iSettings.iFrameRate <= 0 || iSettings.iTimescale <= 0)
        {
            CloseInputFile();
            return PVMFErrArgument;
        }
        int32 offset = 0;
        uint32 sample = 0, time = 0;
        int32 numSamplesInTrack = iSettings.iTotalSamples;
        iStreamDuration = 0;
        if (!iFsOpened_log)
        {
            if (iFs_log.Connect() != 0)
                return PVMFFailure;
            iFsOpened_log = true;
        }
        if (iFileOpened_log ||
                0 != iLogFile.Open(iSettings.iVideoLogFileName.get_cstr(),
                                   Oscl_File::MODE_READ | Oscl_File::MODE_BINARY, iFs_log))
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PvmiMIOFileInput::DoInit: Error - iLogFile.Open failed"));
            return PVMFFailure;
        }
        iFileOpened_log = true;
        offset += 4;    //numSamplesInTrack
        offset += 4;    //avgBitRate
        offset += 4;    //TimseScale
        offset += 4;    //MaxBufferSize
        iLogFile.Seek(offset, Oscl_File::SEEKSET);
        iLogFile.Read(&iConfigSize, sizeof(char), 4);
        offset += 4;  //config size
        offset += 4;  //height
        offset += 4;  //width
        offset += 4;  //frame rate
        iLogFile.Seek(offset, Oscl_File::SEEKSET);

        if (iSettings.iFrameRate != 0)
        {
            iMilliSecondsPerDataEvent = (int32)(1000 / iSettings.iFrameRate);
        }
        iMicroSecondsPerDataEvent = iMilliSecondsPerDataEvent * 1000;
        while (numSamplesInTrack)
        {
            iLogFile.Read(&sample, sizeof(char), 4);   //size of the ith frame
            iLogFile.Read(&time, sizeof(char), 4);

            if ((uint32)sample > maxFrameSize)
                maxFrameSize = sample;

            iFrameSizeVector.push_back(sample);
            iTimeStampVector.push_back(time);
            ++iTotalNumFrames;
            numSamplesInTrack--;
            iStreamDuration = time;
        }
        if (iFileOpened_log)
        {
            iLogFile.Close();
            iFileOpened_log = false;
        }
        //extract NALLengthSize here
        offset = 0;
        iInputFile.Seek(offset, Oscl_File::SEEKSET);
        offset += 1;  //ProfileIndication
        offset += 1;  //ProfileCompatibility
        offset += 1;  //LevelIndication
        iInputFile.Seek(offset, Oscl_File::SEEKSET);
        iInputFile.Read(&iNALType, sizeof(uint8), 1);  //the first byte of bitstream is Nal length type
        offset = 0;
        iInputFile.Seek(offset, Oscl_File::SEEKSET);
    }
    else if (iSettings.iMediaFormat == PVMF_MIME_M4V)
    {
        // Validate settings - even when using logfile we assume the test app
        // has parsed the log file to populate iSettings correcrtly
        if (iSettings.iFrameHeight <= 0 || iSettings.iFrameWidth <= 0 ||
                iSettings.iFrameRate <= 0 || iSettings.iTimescale <= 0)
        {
            CloseInputFile();
            return PVMFErrArgument;
        }
        int32 offset = 0;
        uint32 sample = 0, time = 0;
        int32 numSamplesInTrack = iSettings.iTotalSamples;
        iStreamDuration = 0;
        iTotalNumFrames = 0;
        if (iSettings.iFrameRate != 0)
        {
            iMilliSecondsPerDataEvent = (int32)(1000 / iSettings.iFrameRate);
        }
        iMicroSecondsPerDataEvent = iMilliSecondsPerDataEvent * 1000;
        if (!iFsOpened_log)
        {
            if (iFs_log.Connect() != 0)
                return PVMFFailure;
            iFsOpened_log = true;
        }
        if (iFileOpened_log ||
                0 != iLogFile.Open(iSettings.iVideoLogFileName.get_cstr(),
                                   Oscl_File::MODE_READ | Oscl_File::MODE_BINARY, iFs_log))
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PvmiMIOFileInput::DoInit: Error - iLogFile.Open failed"));
            //try to parse the bitstream to locate frame boundaries
            int32  frameSize;
            uint32 bytesProcessed;
            uint8* fileData = NULL;
            uint8* currentFrame;
            int32 skip;
            fileData = (uint8*)iAlloc.allocate(fileSize);
            if (!fileData)
            {
                CloseInputFile();
                return PVMFErrNoMemory;
            }

            // Read the whole file to data buffer then go back to front
            iInputFile.Read((OsclAny*)fileData, sizeof(uint8), fileSize);
            iInputFile.Seek(fileStart, Oscl_File::SEEKSET);

            // Get ready to search for frame sizes
            iFrameSizeVector.reserve(200);
            currentFrame = fileData;
            bytesProcessed = 0;
            while (bytesProcessed < fileSize)
            {
                do
                {
                    skip = 1;
                    frameSize = LocateM4VFrameHeader(currentFrame + skip,
                                                     fileSize - bytesProcessed - skip);
                    if (currentFrame[3] == 0xb3) /* GOV header */
                    {
                        skip += (frameSize + 1);
                        frameSize = LocateM4VFrameHeader(currentFrame + skip,
                                                         fileSize - bytesProcessed - skip);
                    }
                    if (frameSize == 0) skip++;
                }
                while (frameSize == 0);

                if (frameSize > 0)
                {
                    frameSize += skip;
                }
                else
                {
                    frameSize = fileSize - bytesProcessed;
                }

                if (frameSize > (int32)maxFrameSize)
                    maxFrameSize = frameSize;
                iFrameSizeVector.push_back(frameSize);
                currentFrame += frameSize;
                bytesProcessed += frameSize;
                ++iTotalNumFrames;
            }
            iAlloc.deallocate((OsclAny*)fileData);
            iStreamDuration = iTotalNumFrames * (iMicroSecondsPerDataEvent / 1000); //in msec
        }
        else
        {
            iFileOpened_log = true;
            offset += 4;    //numSamplesInTrack
            offset += 4;    //avgBitRate
            offset += 4;    //TimseScale
            offset += 4;    //MaxBufferSize
            iLogFile.Seek(offset, Oscl_File::SEEKSET);
            iLogFile.Read(&iConfigSize, sizeof(char), 4);
            offset += 4;  //config size
            offset += 4;  //height
            offset += 4;  //width
            offset += 4;  //frame rate
            iLogFile.Seek(offset, Oscl_File::SEEKSET);

            while (numSamplesInTrack)
            {
                iLogFile.Read(&sample, sizeof(char), 4);   //size of the ith frame
                iLogFile.Read(&time, sizeof(char), 4);

                if ((uint32)sample > maxFrameSize)
                    maxFrameSize = sample;

                iFrameSizeVector.push_back(sample);
                iTimeStampVector.push_back(time);
                ++iTotalNumFrames;
                numSamplesInTrack--;
                iStreamDuration = time;
            }
            if (iFileOpened_log)
            {
                iLogFile.Close();
                iFileOpened_log = false;
            }
        }
    }
    else if (iSettings.iMediaFormat == PVMF_MIME_H264_VIDEO_RAW)
    {
        // Validate settings - even when using logfile we assume the test app
        // has parsed the log file to populate iSettings correcrtly
        if (iSettings.iFrameHeight <= 0 || iSettings.iFrameWidth <= 0 ||
                iSettings.iFrameRate <= 0 || iSettings.iTimescale <= 0)
        {
            CloseInputFile();
            return PVMFErrArgument;
        }

        iStreamDuration = 0;
        iTotalNumFrames = 0;
        if (iSettings.iFrameRate != 0)
        {
            iMilliSecondsPerDataEvent = (int32)(1000 / iSettings.iFrameRate);
        }
        iMicroSecondsPerDataEvent = iMilliSecondsPerDataEvent * 1000;
        if (!iFsOpened_log)
        {
            if (iFs_log.Connect() != 0)
                return PVMFFailure;
            iFsOpened_log = true;
        }
        if (iFileOpened_log ||
                0 != iLogFile.Open(iSettings.iVideoLogFileName.get_cstr(), Oscl_File::MODE_READ | Oscl_File::MODE_BINARY, iFs_log))
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PvmiMIOFileInput::DoInit: Error - iLogFile.Open failed"));
            //try to parse the bitstream to locate frame boundaries
            int32  frameSize;
            uint32 bytesProcessed;
            uint8* fileData = NULL;
            uint8* currentFrame;
            fileData = (uint8*)iAlloc.allocate(fileSize);
            if (!fileData)
            {
                CloseInputFile();
                return PVMFErrNoMemory;
            }

            // Read the whole file to data buffer then go back to front
            iInputFile.Read((OsclAny*)fileData, sizeof(uint8), fileSize);
            iInputFile.Seek(fileStart, Oscl_File::SEEKSET);

            // Get ready to search for frame sizes
            iFrameSizeVector.reserve(200);
            currentFrame = fileData;
            bytesProcessed = 0;
            while (bytesProcessed < fileSize)
            {

                frameSize = GetNextNALSize(currentFrame, fileSize - bytesProcessed); // get the size of next NAL (including 4 bytes of sync word)

                // don't skip over the sync word, since that is part of the RAW format

                if ((uint32)frameSize > maxFrameSize)
                    maxFrameSize = frameSize;

                iFrameSizeVector.push_back(frameSize);
                currentFrame += frameSize; // move ptr to beginning of next NAL
                bytesProcessed += frameSize;

                ++iTotalNumFrames;


            }
            iAlloc.deallocate((OsclAny*)fileData);
            iStreamDuration = iTotalNumFrames * (iMicroSecondsPerDataEvent / 1000); //in msec
        }
    }
    else if (iSettings.iMediaFormat == PVMF_MIME_H2631998 ||
             iSettings.iMediaFormat == PVMF_MIME_H2632000)
    {
        // Validate settings
        if (iSettings.iFrameHeight <= 0 || iSettings.iFrameWidth <= 0 ||
                iSettings.iFrameRate <= 0 || iSettings.iTimescale <= 0)
        {
            CloseInputFile();
            return PVMFErrArgument;
        }
        int32 offset = 0;
        uint32 sample = 0, time = 0;
        int32 numSamplesInTrack = iSettings.iTotalSamples;
        iStreamDuration = 0;
        iTotalNumFrames = 0;
        if (iSettings.iFrameRate != 0)
        {
            iMilliSecondsPerDataEvent = (int32)(1000 / iSettings.iFrameRate);
        }
        iMicroSecondsPerDataEvent = iMilliSecondsPerDataEvent * 1000;
        int32  frameSize;
        uint32 bytesProcessed;
        uint8* fileData = NULL;
        uint8* currentFrame;
        int32 skip;
        fileData = (uint8*)iAlloc.allocate(fileSize);
        if (!fileData)
        {
            CloseInputFile();
            return PVMFErrNoMemory;
        }
        // Read the whole file to data buffer then go back to front
        iInputFile.Read((OsclAny*)fileData, sizeof(uint8), fileSize);
        iInputFile.Seek(fileStart, Oscl_File::SEEKSET);
        // Get ready to search for frame sizes
        iFrameSizeVector.reserve(200);
        currentFrame = fileData;
        bytesProcessed = 0;
        if (!iFsOpened_log)
        {
            if (iFs_log.Connect() != 0)
                return PVMFFailure;
            iFsOpened_log = true;
        }
        if (iFileOpened_log ||
                0 != iLogFile.Open(iSettings.iVideoLogFileName.get_cstr(),
                                   Oscl_File::MODE_READ | Oscl_File::MODE_BINARY, iFs_log))
        {
            // Find size of each frame iteratively until end of file
            iTotalNumFrames = 0;
            // H263
            while (bytesProcessed < fileSize)
            {

                do
                {
                    skip = 1;
                    frameSize = LocateH263FrameHeader(currentFrame + skip,
                                                      fileSize - bytesProcessed - skip);
                    if (frameSize == 0) skip++;
                }
                while (frameSize == 0);

                if (frameSize > 0)
                {
                    frameSize += skip;
                }
                else
                {
                    frameSize = fileSize - bytesProcessed;
                }

                if (frameSize > (int32)maxFrameSize)
                    maxFrameSize = frameSize;
                iFrameSizeVector.push_back(frameSize);
                currentFrame += frameSize;
                bytesProcessed += frameSize;
                ++iTotalNumFrames;
            }
            iAlloc.deallocate((OsclAny*)fileData);
            iStreamDuration = iTotalNumFrames * (iMicroSecondsPerDataEvent / 1000); //in msec
        }
        else
        {
            iFileOpened_log = true;
            offset += 4;    //numSamplesInTrack
            offset += 4;    //avgBitRate
            offset += 4;    //TimseScale
            offset += 4;    //MaxBufferSize
            iLogFile.Seek(offset, Oscl_File::SEEKSET);
            iLogFile.Read(&iConfigSize, sizeof(char), 4);
            offset += 4;  //config size
            offset += 4;  //height
            offset += 4;  //width
            offset += 4;  //frame rate
            iLogFile.Seek(offset, Oscl_File::SEEKSET);

            while (numSamplesInTrack)
            {
                iLogFile.Read(&sample, sizeof(char), 4);   //size of the ith frame
                iLogFile.Read(&time, sizeof(char), 4);

                if ((uint32)sample > maxFrameSize)
                    maxFrameSize = sample;

                iFrameSizeVector.push_back(sample);
                iTimeStampVector.push_back(time);
                ++iTotalNumFrames;
                numSamplesInTrack--;
                iStreamDuration = time;
            }
            if (iFileOpened_log)
            {
                iLogFile.Close();
                iFileOpened_log = false;
            }
        }
    }
    else if (iSettings.iMediaFormat == PVMF_MIME_MPEG4_AUDIO)
    {
        uint32 sample, time;
        iStreamDuration = 0;
        if (!iFsOpened_log)
        {
            if (iFs_log.Connect() != 0)
                return PVMFFailure;
            iFsOpened_log = true;
        }
        if (iFileOpened_log ||
                0 != iLogFile.Open(iSettings.iAudioLogFileName.get_cstr(),
                                   Oscl_File::MODE_READ | Oscl_File::MODE_BINARY, iFs_log))
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PvmiMIOFileInput::DoInit: Error - iLogFile.Open failed"));
            return PVMFFailure;
        }
        iFileOpened_log = true;
        iTotalNumFrames = 0;
        int32 numSamplesInTrack = iSettings.iTotalSamples;
        int offset = 0;
        offset += 4;    //numSamplesInTrack
        offset += 4;    //avgBitRate
        offset += 4;    //TimseScale
        offset += 4;    //MaxBufferSize
        iLogFile.Seek(offset, Oscl_File::SEEKSET);
        iLogFile.Read(&iConfigSize, sizeof(char), 4);
        offset += 4; //configsize
        offset += 4; //height
        offset += 4; //width
        offset += 4; //frame_rate
        iLogFile.Seek(offset, Oscl_File::SEEKSET);

        while (numSamplesInTrack)
        {
            iLogFile.Read(&sample, sizeof(char), 4);   //size of the ith frame
            iLogFile.Read(&time, sizeof(char), 4);

            if ((uint32)sample > maxFrameSize)
                maxFrameSize = sample;

            MediaClockConverter mcc(iSettings.iTimescale);
            mcc.update_clock(time);
            time = mcc.get_converted_ts(1000);

            iFrameSizeVector.push_back(sample);
            iTimeStampVector.push_back(time);
            ++iTotalNumFrames;
            numSamplesInTrack--;
            iStreamDuration += time;
        }
        if (iFileOpened_log)
        {
            iLogFile.Close();
            iFileOpened_log = false;
        }
    }
    else if (iSettings.iMediaFormat == PVMF_MIME_3GPP_TIMEDTEXT)
    {
        iTotalNumFrames = 0;
        maxFrameSize = 4096; //as per 3GPP spec
        // Validate settings
        if (iSettings.iFrameHeight <= 0 || iSettings.iFrameWidth <= 0 ||
                iSettings.iTimescale <= 0)
        {
            CloseInputFile();
            return PVMFErrArgument;
        }

        if (!iFsOpened_log)
        {
            if (iFs_log.Connect() != 0)
                return PVMFFailure;
            iFsOpened_log = true;
        }

        if (iFileOpened_log ||
                0 != iLogFile.Open(iSettings.iLogFileName.get_cstr(),
                                   Oscl_File::MODE_READ | Oscl_File::MODE_BINARY, iFs_log))
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PvmiMIOFileInput::DoInit: Error - iLogFile.Open for timed text file format failed"));
            return PVMFFailure;
        }

        iFileOpened_log = true;


        int32  frameSize, timestamp;
        uint32 bytesProcessed;
        uint8* fileData = NULL;
        uint8* currentFrame;

        fileData = (uint8*)iAlloc.allocate(fileSize);
        if (!fileData)
        {
            CloseInputFile();
            return PVMFErrNoMemory;
        }

        // Read the whole file to data buffer then go back to front
        iInputFile.Read((OsclAny*)fileData, sizeof(uint8), fileSize);
        iInputFile.Seek(fileStart, Oscl_File::SEEKSET);

        // Get ready to search for frame sizes
        iFrameSizeVector.reserve(2500);
        currentFrame = fileData;
        bytesProcessed = 0;


        int32 ii = 0;
        int32 offset = 0;
        int32 numSamplesInTrack = 0;
        uint32 timescale = 0;
        uint32 bitrate = 0;
        iLogFile.Seek(offset, Oscl_File::SEEKSET);
        iLogFile.Read(&numSamplesInTrack, sizeof(char), 4);
        offset = offset + 4;

        //iLogFile.Seek( offset, Oscl_File::SEEKSET );  //the information
        iLogFile.Read(&bitrate, sizeof(char), 4);       //present in the log file
        offset = offset + 4;

        //iLogFile.Seek( offset, Oscl_File::SEEKSET );
        iLogFile.Read(&timescale, sizeof(char), 4);
        offset = offset + 4;
        iSettings.iTimescale = timescale;
        uint8  isTrackDuration = false;
        uint32 trackDuration = 0;
        iLogFile.Read(&isTrackDuration, sizeof(char), 1);
        offset += 1;
        if (isTrackDuration)
        {
            iLogFile.Read(&trackDuration, sizeof(char), 4);
            offset = offset + 4;
        }
        //skip 4 bytes of maxsamplesize
        offset = offset + 4;
        iTotalNumFrames = numSamplesInTrack;
        while (numSamplesInTrack)
        {
            iLogFile.Seek(offset, Oscl_File::SEEKSET);
            iLogFile.Read(&SampleSizeArray[ii], sizeof(char), 4);    //size of the ith frame

            frameSize = SampleSizeArray[ii];
            if ((uint32)frameSize > maxFrameSize)
                maxFrameSize = frameSize;
            iFrameSizeVector.push_back(frameSize);
            currentFrame += frameSize;
            bytesProcessed += frameSize;
            ii++;
            offset = offset + 4;
            iLogFile.Seek(offset, Oscl_File::SEEKSET);
            iLogFile.Read(&TextTimeStampArray[ii], sizeof(char), 4);
            timestamp = TextTimeStampArray[ii];
            iTimeStampVector.push_back(timestamp);
            offset = offset + 4;
            numSamplesInTrack--;
        }


        //added for the time being, need to be re-evaluated
        iMicroSecondsPerDataEvent = iSettings.iTimescale * 2;
        iAlloc.deallocate((OsclAny*)fileData);
        if (iFileOpened_log)
        {
            iLogFile.Close();
            iFileOpened_log = false;
        }

        if (iFsOpened_log)
        {
            iFs_log.Close();
            iFsOpened_log = false;
        }

        if (iSettings.iTimescale != 0)
            iStreamDuration = trackDuration / iSettings.iTimescale * 1000; //in msec
    }
    else if ((iSettings.iMediaFormat == PVMF_MIME_YUV420) ||
             (iSettings.iMediaFormat == PVMF_MIME_YUV420_SEMIPLANAR))
    {
        // Set bytes per frame
        maxFrameSize = (uint32)(iSettings.iFrameHeight * iSettings.iFrameWidth * 3 / 2);
        iFrameSizeVector.push_back(maxFrameSize);

        //calculate time for a buffer to fill
        iMilliSecondsPerDataEvent = (int32)(1000 / iSettings.iFrameRate);
        iMicroSecondsPerDataEvent = (int32)(1000000 / iSettings.iFrameRate);
        uint32 numFrames = fileSize / maxFrameSize;
        iStreamDuration = numFrames * (iMicroSecondsPerDataEvent) / 1000; //in msec
    }
    else if (iSettings.iMediaFormat == PVMF_MIME_RGB16)
    {
        // Set bytes per frame
        maxFrameSize = (uint32)(iSettings.iFrameHeight * iSettings.iFrameWidth *  2);
        iFrameSizeVector.push_back(maxFrameSize);

        //calculate time for a buffer to fill
        iMilliSecondsPerDataEvent = (int32)(1000 / iSettings.iFrameRate);
        iMicroSecondsPerDataEvent = (int32)(1000000 / iSettings.iFrameRate - 1);
        uint32 numFrames = fileSize / maxFrameSize;
        iStreamDuration = numFrames * (iMicroSecondsPerDataEvent / 1000); //in msec
    }
    else if (iSettings.iMediaFormat == PVMF_MIME_PCM16)
    {
        // Set bytes per frame
        maxFrameSize = AMR_FRAME_DELAY * iSettings.iSamplingFrequency / 1000 * 2 * iSettings.iNum20msFramesPerChunk;
        iFrameSizeVector.push_back(maxFrameSize);

        //calculate time for a buffer to fill
        float chunkrate = (float)(1000 / AMR_FRAME_DELAY) / iSettings.iNum20msFramesPerChunk;
        iMilliSecondsPerDataEvent = (uint32)(1000 / chunkrate);
        iMicroSecondsPerDataEvent = iMilliSecondsPerDataEvent * 1000;
        uint32 numFrames = fileSize / maxFrameSize;
        iStreamDuration = numFrames * (iMicroSecondsPerDataEvent / 1000); //in msec
    }
    else if (iSettings.iMediaFormat == PVMF_MIME_AMR_IF2 ||
             iSettings.iMediaFormat == PVMF_MIME_AMRWB_IETF ||
             iSettings.iMediaFormat == PVMF_MIME_AMR_IETF)
    {
        int32  size, frameSize;
        uint32 bytesProcessed;
        uint32 chunk;
        uint8* fileData = NULL;
        uint8* currentFrame;

        fileData = (uint8*)iAlloc.allocate(fileSize);
        if (!fileData)
        {
            CloseInputFile();
            return PVMFErrNoMemory;
        }

        oscl_memset(fileData, 0, sizeof(fileData));
        // Read the whole file to data buffer then go back to front
        iInputFile.Read((OsclAny*)fileData, sizeof(uint8), fileSize);
        iInputFile.Seek(fileStart, Oscl_File::SEEKSET);

        // Get ready to search for frame sizes
        iFrameSizeVector.reserve(500);
        currentFrame = fileData;
        bytesProcessed = 0;

        //skip AMR file header.
        if (currentFrame[0] == '#')
        {
            iFileHeaderSize = 0;
            if (iSettings.iMediaFormat == PVMF_MIME_AMR_IETF)
            {
                //Skip AMR-NB magic word - "#!AMR\n" (or 0x2321414d520a in hexadecimal) (6 characters)
                iFileHeaderSize = 6;
            }
            else if (iSettings.iMediaFormat == PVMF_MIME_AMRWB_IETF)
            {
                //Skip AMR-WB magic word - "#!AMR-WB\n" (or 0x2321414d522d57420a in hexadecimal) (9 characters)
                iFileHeaderSize = 9;
            }
            currentFrame += iFileHeaderSize;
            iInputFile.Seek(iFileHeaderSize, Oscl_File::SEEKSET);
            bytesProcessed = iFileHeaderSize;
        }

        // Find size of each frame iteratively until end of file
        iTotalNumFrames = 0;
        while (bytesProcessed < fileSize)
        {
            frameSize = 0;
            for (chunk = 0; (chunk < iSettings.iNum20msFramesPerChunk) &&
                    (bytesProcessed < fileSize); chunk++)
            {
                if (iSettings.iMediaFormat == PVMF_MIME_AMR_IF2)
                {
                    size = GetIF2FrameSize(currentFrame[0]);
                    if (size == -1)
                    {
                        return PVMFFailure;
                    }
                }
                else
                {
                    size = GetIETFFrameSize(currentFrame[0],
                                            iSettings.iMediaFormat);
                    if (size == -1)
                    {
                        return PVMFFailure;
                    }
                }
                frameSize += size;
                currentFrame += size;
                bytesProcessed += size;
            }

            if (frameSize > (int32)maxFrameSize)
                maxFrameSize = frameSize;
            iFrameSizeVector.push_back(frameSize);
            ++iTotalNumFrames;
        }

        // Calculate time for a buffer to fill to simulate frame rate
        iMilliSecondsPerDataEvent = 20 * iSettings.iNum20msFramesPerChunk;
        iMicroSecondsPerDataEvent = iMilliSecondsPerDataEvent * 1000;
        iAlloc.deallocate((OsclAny*)fileData);
        iStreamDuration = iTotalNumFrames * (iMicroSecondsPerDataEvent / 1000); //in msec
    }
    else if (iSettings.iMediaFormat == PVMF_MIME_ADTS ||
             iSettings.iMediaFormat == PVMF_MIME_ADIF ||
             iSettings.iMediaFormat == PVMF_MIME_MP3)
    {
        int32 frameSize;
        uint32 bytesProcessed;

        // Get ready to search for frame sizes
        iFrameSizeVector.reserve(500);
        bytesProcessed = 0;

        // Find size of each frame iteratively until end of file
        iTotalNumFrames = 0;
        while (bytesProcessed < fileSize)
        {
            if ((fileSize - bytesProcessed) < 1024)
                frameSize = fileSize - bytesProcessed;
            else
                frameSize = 1024;
            bytesProcessed += frameSize;
            iFrameSizeVector.push_back(frameSize);
            ++iTotalNumFrames;
        }

        // Calculate time for a buffer to fill to simulate frame rate
        maxFrameSize = 1024;
        iMilliSecondsPerDataEvent = 20;
        iMicroSecondsPerDataEvent = iMilliSecondsPerDataEvent * 1000;
        iStreamDuration = iTotalNumFrames * (iMicroSecondsPerDataEvent / 1000); //in msec
    }
    else
    {
        CloseInputFile();
        return PVMFErrArgument;
    }

    if (!iSettings.iLoopInputFile)
    {
        //set default authoring duration
        iAuthoringDuration = iStreamDuration;
    }


    RetrieveFSI(iFormatSpecificInfoSize);

    iDataEventCounter = 0;
    CloseInputFile();

    // Create memory pool for the media data, using the maximum frame size found earlier
    int32 err = 0;
    OSCL_TRY(err,
             if (iMediaBufferMemPool)
{
    OSCL_DELETE(iMediaBufferMemPool);
        iMediaBufferMemPool = NULL;
    }
    iMediaBufferMemPool = OSCL_NEW(OsclMemPoolFixedChunkAllocator,
                                   (PVMIOFILEIN_MEDIADATA_POOLNUM));
    if (!iMediaBufferMemPool)
    OSCL_LEAVE(OsclErrNoMemory);

            );
    OSCL_FIRST_CATCH_ANY(err, return PVMFErrNoMemory);

    // The first allocate call will set the chunk size of the memory pool. Use the max
    // frame size calculated earlier to set the chunk size.  The allocated data will be
    // deallocated automatically as tmpPtr goes out of scope.
    OsclAny* tmpPtr = iMediaBufferMemPool->allocate(maxFrameSize);
    iMediaBufferMemPool->deallocate(tmpPtr);

    iState = STATE_INITIALIZED;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmiMIOFileInput::DoStart()
{
    if (STATE_STARTED == iState)
    {
        return PVMFSuccess;
    }
    iState = STATE_STARTED;

    if (!iFileOpened)
    {
        if (iFs.Connect() != 0)
        {
            return 0;
        }

        iFsOpened = true;

        if (iInputFile.Open(iSettings.iFileName.get_cstr(),
                            Oscl_File::MODE_READ | Oscl_File::MODE_BINARY, iFs))
            return PVMFFailure;
        iFileOpened = true;

        //seek to zero
        if (iInputFile.Seek(0, Oscl_File::SEEKSET))
            return PVMFFailure;

        //skip the header if any
        if (iInputFile.Seek(iFileHeaderSize, Oscl_File::SEEKSET))
            return PVMFFailure;
    }

    iStartTime.set_to_current_time();
    iStartTimestamp = iReadTimeStamp;

    AddDataEventToQueue(0);
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmiMIOFileInput::DoPause()
{
    iState = STATE_PAUSED;
    return PVMFSuccess;
}

PVMFStatus PvmiMIOFileInput::DoReset()
{
    if (iFSIKvp)
    {
        iAlloc.deallocate(iFSIKvp->value.key_specific_value);
        iAlloc.deallocate(iFSIKvp);
        iFSIKvp = NULL;
    }
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmiMIOFileInput::DoFlush()
{
    // This method should stop capturing media data but continue to send captured
    // media data that is already in buffer and then go to stopped state.
    // However, in this case of file input we do not have such a buffer for
    // captured data, so this behaves the same way as stop.
    return DoStop();
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmiMIOFileInput::DoStop()
{
    iDataEventCounter = 0;
    iState = STATE_STOPPED;
    CloseInputFile();
    return PVMFSuccess;
}

int32 PvmiMIOFileInput::GetFrameSize(int32 aIndex)
{
    if (aIndex < 0)
        return 0;
    uint32 num = OSCL_STATIC_CAST(int32, aIndex);
    if (num < iFrameSizeVector.size())
    {
        return iFrameSizeVector[num];
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_WARNING,
                        (0, "PvmiMIOFileInput::GetFrameSize: Attempting to read FrameSize Vector with invalid index.  Returning 0."));
    }
    return 0;
}

int32 PvmiMIOFileInput::GetTimeStamp(int32 aIndex)
{
    if (aIndex < 0)
        return 0;
    uint32 num = OSCL_STATIC_CAST(uint32, aIndex);
    if (num < iTimeStampVector.size())
    {
        return iTimeStampVector[num];
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_WARNING,
                        (0, "PvmiMIOFileInput::DoRead: Attempting to read TimeStamp Vector with invalid index"));
    }
    return 0;
}


////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmiMIOFileInput::DoRead()
{
    // Just copy from PVMFFileInputNode::HandleEventPortActivity.  The only difference
    // is that data buffer is allocated by calling iMediaBufferMemPool->allocate(bytesToRead)
    // and there's no need to wrap it in a PVMFSharedMediaDataPtr.  Also, you'll need to
    // keep track of the data pointer and the write command id received from peer->writeAsync
    // and put it in the iSentMediaData queue

    if (iState != STATE_STARTED)
    {
        return PVMFSuccess;
    }

    uint32 bytesToRead = 0;
    iReadTimeStamp = 0;
    // allocate bytesToRead number of bytes
    uint8* data = NULL;
    uint32 writeAsyncID = 0;

    int32 index = -1;
    if (iTotalNumFrames > 0)
    {
        index = iDataEventCounter % iTotalNumFrames;
    }
    else if (iSettings.iMediaFormat != PVMF_MIME_PCM16) // pcm data is read in fixed sized chunks, no warning required
    {

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_WARNING,
                        (0, "PvmiMIOFileInput::DoRead: iTotalNumFrames is 0!"));
    }
    //Find the frame...
    if ((iSettings.iMediaFormat == PVMF_MIME_M4V) ||
            (iSettings.iMediaFormat == PVMF_MIME_ISO_AVC_SAMPLE_FORMAT))
    {
        bytesToRead = GetFrameSize(index);
        iReadTimeStamp = GetTimeStamp(index);
        ++iDataEventCounter;
    }
    else if (iSettings.iMediaFormat == PVMF_MIME_H2631998 ||
             iSettings.iMediaFormat == PVMF_MIME_H2632000)
    {
        bytesToRead = GetFrameSize(index);
        iReadTimeStamp = (uint32)(iDataEventCounter * 1000 / iSettings.iFrameRate);
        ++iDataEventCounter;
    }
    else if (iSettings.iMediaFormat == PVMF_MIME_3GPP_TIMEDTEXT)
    {
        bytesToRead = GetFrameSize(index);
        uint32 ts = GetTimeStamp(index);
        if ((iDataEventCounter % iTotalNumFrames) == 0)
        {
            ++iCount;
            iPreTS = iPreTS * iCount;
        }
        if (iPreTS > ts)
        {
            iReadTimeStamp = ts + iPreTS;
        }
        else
        {
            iReadTimeStamp = ts;
            iPreTS = iReadTimeStamp;
        }
        ++iDataEventCounter;
    }
    else if (iSettings.iMediaFormat == PVMF_MIME_MPEG4_AUDIO)
    {
        bytesToRead = GetFrameSize(index);
        iReadTimeStamp = GetTimeStamp(index);
        ++iDataEventCounter;
    }
    else if (iSettings.iMediaFormat == PVMF_MIME_AMR_IF2 ||
             iSettings.iMediaFormat == PVMF_MIME_AMR_IETF ||
             iSettings.iMediaFormat == PVMF_MIME_AMRWB_IETF ||
             iSettings.iMediaFormat == PVMF_MIME_ADTS ||
             iSettings.iMediaFormat == PVMF_MIME_ADIF ||
             iSettings.iMediaFormat == PVMF_MIME_MP3)
    {
        bytesToRead = GetFrameSize(index);
        iReadTimeStamp = iTimeStamp;
        iTimeStamp += iMilliSecondsPerDataEvent;
        ++iDataEventCounter;
    }
    else if (iSettings.iMediaFormat == PVMF_MIME_YUV420 ||
             iSettings.iMediaFormat == PVMF_MIME_YUV420_SEMIPLANAR ||
             iSettings.iMediaFormat == PVMF_MIME_RGB16)
    {
        bytesToRead = GetFrameSize(0);
        iReadTimeStamp = (uint32)(iDataEventCounter * 1000 / iSettings.iFrameRate);
        ++iDataEventCounter;
    }
    else if (iSettings.iMediaFormat == PVMF_MIME_PCM16)
    {
        bytesToRead = GetFrameSize(0);
        float chunkrate = (float)(1000 / AMR_FRAME_DELAY) /
                          iSettings.iNum20msFramesPerChunk;
        iReadTimeStamp = (uint32)(iDataEventCounter * 1000 / chunkrate);
        ++iDataEventCounter;
    }
    else if (iSettings.iMediaFormat == PVMF_MIME_H264_VIDEO_RAW)
    {
        if (iDataEventCounter)
        {
            bytesToRead = iFrameSizeVector[iDataEventCounter % iTotalNumFrames];
            iReadTimeStamp = (uint32)(iDataEventCounter * 1000 / iSettings.iFrameRate);
        }
        else
        {
            bytesToRead = iFrameSizeVector[0] + iFrameSizeVector[1];
            ++iDataEventCounter;
            iReadTimeStamp = 0;
        }
        ++iDataEventCounter;
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "PvmiMIOFileInput::HandleEventPortActivity: Error - Unsupported media format"));
        return PVMFFailure;
    }

    // Create new media data buffer
    int32 error = 0;
    data = AllocateMemPool(iMediaBufferMemPool, bytesToRead, error);

    if (error)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "PvmiMIOFileInput::No buffer available, wait till next data event"));

        AddDataEventToQueue(iMicroSecondsPerDataEvent);
        return PVMFPending;
    }


    // Read the frame from file
    uint32 len = 0;
    uint32 stopTimeStamp = 0;

    if (iSettings.iMediaFormat == PVMF_MIME_3GPP_TIMEDTEXT)
    {
        stopTimeStamp = (iReadTimeStamp / iSettings.iTimescale) * 1000; //in msec

    }
    else
    {
        stopTimeStamp = iReadTimeStamp;
    }

    if (iConfigSize)
    {
        if (iSettings.iMediaFormat != PVMF_MIME_ISO_AVC_SAMPLE_FORMAT)
        {
            configinfo = (uint8*)oscl_malloc(iConfigSize);
            if (configinfo)
            {
                len = iInputFile.Read((OsclAny*)configinfo,
                                      sizeof(uint8), iConfigSize);
            }
        }
        else
        {
            if (Send_SPS_PPS_info() != PVMFSuccess)
            {
                return PVMFFailure;
            }
        }

    }

    if (!iSettings.iLoopInputFile)
    {
        if (stopTimeStamp <= iAuthoringDuration)
        {

            len = iInputFile.Read((OsclAny*)data, sizeof(uint8), bytesToRead);
        }
        else
        {
            len = 0;
        }
    }
    else
    {
        len = iInputFile.Read((OsclAny*)data, sizeof(uint8), bytesToRead);
    }


    if (len != bytesToRead)
    {
        if ((iInputFile.EndOfFile() != 0) || (stopTimeStamp >= iAuthoringDuration))
        {
            // Loop or report end of data now...
            if (iSettings.iLoopInputFile)
            {
                iInputFile.Seek(iFileHeaderSize + iFormatSpecificInfoSize,
                                Oscl_File::SEEKSET);
                len = iInputFile.Read(data, sizeof(uint8), bytesToRead);
                if (len != bytesToRead)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PvmiMIOFileInput::HandleEventPortActivity: Error - iInputFile.Read failed"));
                    return PVMFFailure;
                }
            }
            else    //EOS Reached
            {
                //free the allocated data buffer
                iMediaBufferMemPool->deallocate(data);
                data = NULL;

                PvmiMediaXferHeader data_hdr;
                oscl_memset(&data_hdr, 0, sizeof(data_hdr));
                data_hdr.seq_num = iDataEventCounter - 1;
                data_hdr.timestamp = iReadTimeStamp;
                bytesToRead = 0;

                //send EOS information to MIO Node
                error = WriteAsyncDataHdr(writeAsyncID, iPeer,
                                          bytesToRead, data_hdr);

                if (error)
                {
                    //some error occured, retry sending EOS next time.
                    AddDataEventToQueue(iMicroSecondsPerDataEvent);
                    return PVMFSuccess;
                }

                //EOS message was sent so PAUSE MIO Component.
                AddCmdToQueue(CMD_PAUSE, NULL);
                CloseInputFile();

                return PVMFSuccess;
            }
        }
        else
        {
            iState = STATE_STOPPED;
            CloseInputFile();
            AddCmdToQueue(CMD_STOP, NULL);
            return PVMFSuccess;
        }


    }

    if (len == bytesToRead)//Data Read Successfully
    {
        if (iSettings.iMediaFormat == PVMF_MIME_3GPP_TIMEDTEXT &&
                !iTimed_Text_configinfo) //added for timed text support
        {
            if (Get_Timed_Config_Info() != PVMFSuccess)
            {
                return PVMFFailure;
            }
            else
            {
                iTimed_Text_configinfo = true;
            }
        }
        // send data to Peer & store the id
        PvmiMediaXferHeader data_hdr;
        oscl_memset(&data_hdr, 0, sizeof(data_hdr));
        data_hdr.seq_num = iDataEventCounter - 1;
        data_hdr.timestamp = iReadTimeStamp;

        if (iSettings.iMediaFormat == PVMF_MIME_3GPP_TIMEDTEXT)
        {
            data_hdr.flags |= PVMI_MEDIAXFER_MEDIA_DATA_FLAG_DURATION_AVAILABLE_BIT;
            data_hdr.duration = iStreamDuration;
        }
        if (((iSettings.iMediaFormat == PVMF_MIME_MPEG4_AUDIO) ||
                (iSettings.iMediaFormat == PVMF_MIME_M4V)) &&
                (1 == iDataEventCounter))
        {
            data_hdr.private_data_ptr = (OsclAny*)configinfo;
            data_hdr.private_data_length = iConfigSize;
        }
        if ((iSettings.iMediaFormat == PVMF_MIME_ISO_AVC_SAMPLE_FORMAT) &&
                (1 == iDataEventCounter))
        {
            data_hdr.private_data_length = iNALType;
        }
        iConfigSize = 0;
        if (!iPeer)
        {
            iMediaBufferMemPool->deallocate(data);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_NOTICE,
                            (0, "PvmiMIOFileInput::DoRead - Peer missing"));
            return PVMFSuccess;
        }

        // Queue the next data event
        TimeValue currentTime;
        currentTime.set_to_current_time();

        TimeValue deltaTimeVal = currentTime - iStartTime;
        int32 t1 = iReadTimeStamp;
        int32 t2 = iStartTimestamp;
        int32 deltaTimestamp = t1 - t2;
        //int32 deltaTimestamp = iReadTimeStamp - iStartTimestamp;

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                        (0, "PvmiMIOFileInput::DoRead - Calling WriteAsync - Size=%d, TS=%d, Seq=%d, Mime=%s, MicroSecPerDataEvent=%d",
                         bytesToRead, data_hdr.timestamp, data_hdr.seq_num, iSettings.iMediaFormat.getMIMEStrPtr(), iMicroSecondsPerDataEvent));
        OSCL_TRY(error, writeAsyncID = iPeer->writeAsync(PVMI_MEDIAXFER_FMT_TYPE_DATA, 0, data, bytesToRead, data_hdr););

        if (!error)
        {
            // Save the id and data pointer on iSentMediaData queue for writeComplete call
            PvmiMIOFileInputMediaData sentData;
            sentData.iId = writeAsyncID;
            sentData.iData = data;
            iSentMediaData.push_back(sentData);
        }
        else if (error == OsclErrBusy)
        {
            iDataEventCounter--;
            iMediaBufferMemPool->deallocate(data);
        }
        else
        {
            iMediaBufferMemPool->deallocate(data);
        }

        if (deltaTimeVal.to_msec() > deltaTimestamp)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                            (0, "PvmiMIOFileInput::DoRead - We are running late %dms", deltaTimeVal.to_msec() - deltaTimestamp));
            AddDataEventToQueue(0);
        }
        else
        {
            AddDataEventToQueue(iMicroSecondsPerDataEvent);
        }
    }
    return PVMFSuccess;
}

//////////////////////////////////////////////////////////////////////////////////
int32 PvmiMIOFileInput::GetIF2FrameSize(uint8 aFrameType)
{
    // Last 4 bits of first byte of the package is frame type
    uint8 frameType = (uint8)(0x0f & aFrameType);

    // Find frame size for each frame type
    switch (frameType)
    {
        case 0: // AMR 4.75 Kbps
            return 13;
        case 1: // AMR 5.15 Kbps
            return 14;
        case 2: // AMR 5.90 Kbps
            return 16;
        case 3: // AMR 6.70 Kbps
            return 18;
        case 4: // AMR 7.40 Kbps
            return 19;
        case 5: // AMR 7.95 Kbps
            return 21;
        case 6: // AMR 10.2 Kbps
            return 26;
        case 7: // AMR 12.2 Kbps
            return 31;
        case 8: // AMR Frame SID
        case 9: // AMR Frame GSM EFR SID
        case 10: // AMR Frame TDMA EFR SID
        case 11: // AMR Frame PDC EFR SID
            return 6;
        case 15: // AMR Frame No Data
            return 1;
        default: // Error - For Future Use
            return -1;
    }
}

//////////////////////////////////////////////////////////////////////////////////
int32 PvmiMIOFileInput::GetIETFFrameSize(uint8 aFrameType,
        PVMFFormatType aFormat)
{
    uint8 frameType = (uint8)((aFrameType >> 3) & 0x0f);
    if (aFormat == PVMF_MIME_AMR_IETF)
    {
        // Find frame size for each frame type
        switch (frameType)
        {
            case 0: // AMR 4.75 Kbps
                return 13;
            case 1: // AMR 5.15 Kbps
                return 14;
            case 2: // AMR 5.90 Kbps
                return 16;
            case 3: // AMR 6.70 Kbps
                return 18;
            case 4: // AMR 7.40 Kbps
                return 20;
            case 5: // AMR 7.95 Kbps
                return 21;
            case 6: // AMR 10.2 Kbps
                return 27;
            case 7: // AMR 12.2 Kbps
                return 32;
            case 8: // AMR Frame SID
                return 6;
            case 9: // AMR Frame GSM EFR SID
                return 7;
            case 10:// AMR Frame TDMA EFR SID
            case 11:// AMR Frame PDC EFR SID
                return 6;
            case 15: // AMR Frame No Data
                return 1;
            default: // Error - For Future Use
                return -1;
        }
    }
    else if (aFormat == PVMF_MIME_AMRWB_IETF)
    {
        // Find frame size for each frame type
        switch (frameType)
        {
            case 0: // AMR-WB 6.60 Kbps
                return 18;
            case 1: // AMR-WB 8.85 Kbps
                return 24;
            case 2: // AMR-WB 12.65 Kbps
                return 33;
            case 3: // AMR-WB 14.25 Kbps
                return 37;
            case 4: // AMR-WB 15.85 Kbps
                return 41;
            case 5: // AMR-WB 18.25 Kbps
                return 47;
            case 6: // AMR-WB 19.85 Kbps
                return 51;
            case 7: // AMR-WB 23.05 Kbps
                return 59;
            case 8: // AMR-WB 23.85 Kbps
                return 61;
            case 9: // AMR-WB SID Frame
                return 6;
            case 10: //Reserved
            case 11: //Reserved
            case 12: //Reserved
            case 13: //Reserved
                return -1;
            case 14: // AMR-WB Frame Lost
            case 15: // AMR-WB Frame No Data
                return 1;
            default: // Error - For Future Use
                return -1;
        }
    }
    return -1;
}

//////////////////////////////////////////////////////////////////////////////////

uint32 PvmiMIOFileInput::GetNextNALSize(uint8 *video_buffer, uint32 bytes_left)
{
    // locate the next sync word 0x 00 00 00 01
    // for the case of very last NAL, we compare the bytes_left
    video_buffer++;

    // clarity over efficiency !
    uint32 size = 0;
    while (true)
    {
        size++;
        if (((video_buffer[0] == 0) &&
                (video_buffer[1] == 0) &&
                (video_buffer[2] == 0) &&
                (video_buffer[3] == 1)) ||
                (size >= bytes_left))
            break;

        video_buffer++;
    }

    return size;
}


//////////////////////////////////////////////////////////////////////////////////
int32 PvmiMIOFileInput::LocateM4VFrameHeader(uint8* video_buffer, int32 vop_size)
{
    uint8 start_code;
    register int32 idx;
    register uint8* ptr;

    idx = 1;
    ptr = video_buffer + 1;
    vop_size -= 4;
    do
    {
        do
        {
            ptr--;
            idx--;
            for (;;)
            {
                if (ptr[1])
                {
                    ptr += 2;
                    idx += 2;
                }
                else if (ptr[0])
                {
                    ptr++;
                    idx++;
                }
                else
                {
                    break;
                }
                if (idx >= vop_size) return -1;
            }
            ptr += 2;
            idx += 2;
        }
        while (video_buffer[idx] != 0x01);
        ptr++;
        idx++;
        start_code = video_buffer[idx];
    }
    while (start_code != 0xb6 /* VOP */ && start_code != 0xb3 /* GOV */);

    return idx - 3;
}


//////////////////////////////////////////////////////////////////////////////////
int32 PvmiMIOFileInput::LocateH263FrameHeader(uint8 *video_buffer,
        int32 vop_size)
{
    int32 idx;
    uint8 *ptr;

    idx = 1;
    ptr = video_buffer + 1;
    vop_size -= 4;
    do
    {
        ptr--;
        idx--;
        for (;;)
        {
            if (ptr[1])
            {
                ptr += 2;
                idx += 2;
            }
            else if (ptr[0])
            {
                ptr++;
                idx++;
            }
            else
            {
                break;
            }
            if (idx >= vop_size) return -1;
        }
        ptr += 2;
        idx += 2;
    }
    while ((video_buffer[idx] & 0xfc) != 0x80);

    return idx - 2;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmiMIOFileInput::AllocateKvp(PvmiKvp*& aKvp,
        PvmiKeyType aKey,
        int32 aNumParams)
{
    LOG_STACK_TRACE((0, "PvmiMIOFileInput::AllocateKvp"));
    uint8* buf = NULL;
    uint32 keyLen = oscl_strlen(aKey) + 1;
    int32 err = 0;

    OSCL_TRY(err,
             buf = (uint8*)iAlloc.allocate(aNumParams * (sizeof(PvmiKvp) + keyLen));
             if (!buf)
             OSCL_LEAVE(OsclErrNoMemory);
            );
    OSCL_FIRST_CATCH_ANY(err,
                         LOG_ERR((0, "PvmiMIOFileInput::AllocateKvp: Error - kvp allocation failed"));
                         return PVMFErrNoMemory;
                        );

    int32 i = 0;
    PvmiKvp* curKvp = aKvp = new(buf) PvmiKvp;
    buf += sizeof(PvmiKvp);
    for (i = 1; i < aNumParams; i++)
    {
        curKvp += i;
        curKvp = new(buf) PvmiKvp;
        buf += sizeof(PvmiKvp);
    }

    for (i = 0; i < aNumParams; i++)
    {
        aKvp[i].key = (char*)buf;
        oscl_strncpy(aKvp[i].key, aKey, keyLen);
        buf += keyLen;
    }

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmiMIOFileInput::VerifyAndSetParameter(PvmiKvp* apKvp,
        bool aSetParam)
{
    OSCL_UNUSED_ARG(aSetParam);
    LOG_STACK_TRACE((0, "PvmiMIOFileInput::VerifyAndSetParameter: aKvp=0x%x, aSetParam=%d",
                     apKvp, aSetParam));

    if (!apKvp)
    {
        LOG_ERR((0, "PvmiMIOFileInput::VerifyAndSetParameter: Error - Invalid key-value pair"));
        return PVMFFailure;
    }

    if (pv_mime_strcmp(apKvp->key, OUTPUT_FORMATS_VALTYPE) == 0)
    {
        if (iSettings.iMediaFormat == apKvp->value.pChar_value)
        {
            return PVMFSuccess;
        }
        else
        {
            LOG_ERR((0, "PvmiMIOFileInput::VerifyAndSetParameter: Error - Unsupported format %s",
                     apKvp->value.pChar_value));
            return PVMFFailure;
        }
    }
    else if (pv_mime_strcmp(apKvp->key,
                            PVMF_MEDIA_INPUT_NODE_CAP_CONFIG_INTERFACE_KEY) == 0)
    {
        iPeerCapConfig = OSCL_STATIC_CAST(PvmiCapabilityAndConfig*,
                                          apKvp->value.key_specific_value);
        LOG_DEBUG((0, "PvmiMIOFileInput::VerifyAndSetParameter: PVMF_MEDIA_INPUT_NODE_CAP_CONFIG_INTERFACE_KEY - Ptr=0x%x", iPeerCapConfig));
        return PVMFSuccess;
    }

    LOG_ERR((0, "PvmiMIOFileInput::VerifyAndSetParameter: Error - Unsupported parameter"));
    return PVMFFailure;
}

bool PvmiMIOFileInput::DecoderInfo(char* buffer,
                                   char* bufferEnd,
                                   char* res,
                                   uint32 resSize)
{
    uint32 ji = 0;
    if ((NULL == buffer) || (NULL == bufferEnd) || (resSize <= 0))
    {
        return false;
    }

    oscl_memset(res, 0, resSize);

    while (buffer < bufferEnd)
    {
        char* first = OSCL_CONST_CAST(char*, oscl_strstr(buffer, "= "));
        if (NULL == first)
        {
            return false;
        }

        first += 2;
        char* temp = first;
        while (*first != '\n')
        {
            ji++;
            first += 1;

            if ('\r' == *first)
            {
                break;
            }

            if (first >= bufferEnd)
            {
                return false;
            }
        }

        if (ji < resSize)
        {
            oscl_strncpy(res, temp, ji);
        }

        iptextfiledata = first;
        break;
    }   //(buffer < bufferEnd)

    return true;
}

PVMFStatus PvmiMIOFileInput::Get_Timed_Config_Info()
{
#ifdef TEXT_TRACK_DESC_INFO

    if (!iFsOpened_text)
    {
        if (iFs_text.Connect() != 0)
            return PVMFFailure;
        iFsOpened_text = true;
    }

    if (iFileOpened_text ||
            0 != iTextFile.Open(iSettings.iTextFileName.get_cstr(),
                                Oscl_File::MODE_READ | Oscl_File::MODE_BINARY, iFs_text))
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "PvmiMIOFileInput::Get_Timed_Config_Info: Error - iTextFile.Open for timed text file format failed"));
        return PVMFFailure;
    }

    iFileOpened_text = true;

    int32 fileStart, fileEnd, fileSize;


    fileStart = iTextFile.Tell();
    iTextFile.Seek(0, Oscl_File::SEEKEND);

    fileEnd = iTextFile.Tell();
    iTextFile.Seek(fileStart, Oscl_File::SEEKSET);

    fileSize = fileEnd - fileStart;
    iptextfiledata = NULL;
    iptextfiledata = (char*)iAlloc.allocate(fileSize + 1);
    oscl_memset(iptextfiledata, 0, fileSize + 1);
    if (!iptextfiledata)
    {
        return PVMFErrNoMemory;
    }
    // Read the whole file to data buffer then go back to front
    iTextFile.Read((OsclAny*)iptextfiledata, sizeof(uint8), fileSize);
    iTextFile.Seek(fileStart, Oscl_File::SEEKSET);


    char* buff = iptextfiledata;
    uint32 valsize = 10;
    char* val = (char*)OSCL_MALLOC(valsize * sizeof(char));
    uint32 temp = 0;
    while (iptextfiledata < (buff + fileSize))
    {
        PVA_FF_TextSampleDescInfo *ipDecoderinfo;
        ipDecoderinfo = OSCL_NEW(PVA_FF_TextSampleDescInfo, ());
        oscl_memset(val, 0, valsize);

        if (!DecoderInfo(iptextfiledata, (buff + fileSize), val, valsize))
        {
            OSCL_DELETE(ipDecoderinfo);
            break;
        }
        PV_atoi(val, 'd', (uint32&)ipDecoderinfo->start_sample_num);

        if (!DecoderInfo(iptextfiledata, (buff + fileSize), val, valsize))
        {
            OSCL_DELETE(ipDecoderinfo);
            break;
        }
        PV_atoi(val, 'd', (uint32&)ipDecoderinfo->sdindex);

        if (!DecoderInfo(iptextfiledata, (buff + fileSize), val, valsize))
        {
            OSCL_DELETE(ipDecoderinfo);
            break;
        }
        PV_atoi(val, 'd', (uint32&)ipDecoderinfo->display_flags);

        if (!DecoderInfo(iptextfiledata, (buff + fileSize), val, valsize))
        {
            OSCL_DELETE(ipDecoderinfo);
            break;
        }
        PV_atoi(val, 'd', (uint32&)ipDecoderinfo->hJust);


        temp = 0;
        if (!DecoderInfo(iptextfiledata, (buff + fileSize), val, valsize))
        {
            OSCL_DELETE(ipDecoderinfo);
            break;
        }
        PV_atoi(val, 'd', (uint32&) temp);
        ipDecoderinfo->vJust = temp;


        if (!DecoderInfo(iptextfiledata, (buff + fileSize), val, valsize))
        {
            OSCL_DELETE(ipDecoderinfo);
            break;
        }
        temp = 0;
        PV_atoi(val, 'd', (uint32&)temp);
        ipDecoderinfo->bkRgba[0] = temp;

        if (!DecoderInfo(iptextfiledata, (buff + fileSize), val, valsize))
        {
            OSCL_DELETE(ipDecoderinfo);
            break;
        }
        temp = 0;
        PV_atoi(val, 'd', (uint32&)temp);
        ipDecoderinfo->bkRgba[1] = temp;

        if (!DecoderInfo(iptextfiledata, (buff + fileSize), val, valsize))
        {
            OSCL_DELETE(ipDecoderinfo);
            break;
        }
        temp = 0;
        PV_atoi(val, 'd', (uint32&)temp);
        ipDecoderinfo->bkRgba[2] = temp;

        if (!DecoderInfo(iptextfiledata, (buff + fileSize), val, valsize))
        {
            OSCL_DELETE(ipDecoderinfo);
            break;
        }
        temp = 0;
        PV_atoi(val, 'd', (uint32&)temp);
        ipDecoderinfo->bkRgba[3] = temp;

        if (!DecoderInfo(iptextfiledata, (buff + fileSize), val, valsize))
        {
            OSCL_DELETE(ipDecoderinfo);
            break;
        }
        temp = 0;
        PV_atoi(val, 'd', (uint32&)temp);
        ipDecoderinfo->top = temp;

        if (!DecoderInfo(iptextfiledata, (buff + fileSize), val, valsize))
        {
            OSCL_DELETE(ipDecoderinfo);
            break;
        }
        temp = 0;
        PV_atoi(val, 'd', (uint32&)temp);
        ipDecoderinfo->left = temp;

        if (!DecoderInfo(iptextfiledata, (buff + fileSize), val, valsize))
        {
            OSCL_DELETE(ipDecoderinfo);
            break;
        }
        temp = 0;
        PV_atoi(val, 'd', (uint32&)temp);
        ipDecoderinfo->bottom = temp;

        if (!DecoderInfo(iptextfiledata, (buff + fileSize), val, valsize))
        {
            OSCL_DELETE(ipDecoderinfo);
            break;
        }
        temp = 0;
        PV_atoi(val, 'd', (uint32&)temp);
        ipDecoderinfo->right = temp;

        if (!DecoderInfo(iptextfiledata, (buff + fileSize), val, valsize))
        {
            OSCL_DELETE(ipDecoderinfo);
            break;
        }
        temp = 0;
        PV_atoi(val, 'd', (uint32&)temp);
        ipDecoderinfo->startChar = temp;

        if (!DecoderInfo(iptextfiledata, (buff + fileSize), val, valsize))
        {
            OSCL_DELETE(ipDecoderinfo);
            break;
        }
        temp = 0;
        PV_atoi(val, 'd', (uint32&)temp);
        ipDecoderinfo->endChar = temp;

        if (!DecoderInfo(iptextfiledata, (buff + fileSize), val, valsize))
        {
            OSCL_DELETE(ipDecoderinfo);
            break;
        }
        temp = 0;
        PV_atoi(val, 'd', (uint32&)temp);
        ipDecoderinfo->fontID = temp;

        if (!DecoderInfo(iptextfiledata, (buff + fileSize), val, valsize))
        {
            OSCL_DELETE(ipDecoderinfo);
            break;
        }
        temp = 0;
        PV_atoi(val, 'd', (uint32&)temp);
        ipDecoderinfo->fontSizeFlags = temp;

        if (!DecoderInfo(iptextfiledata, (buff + fileSize), val, valsize))
        {
            OSCL_DELETE(ipDecoderinfo);
            break;
        }
        temp = 0;
        PV_atoi(val, 'd', (uint32&)temp);
        ipDecoderinfo->fontSize = temp;

        if (!DecoderInfo(iptextfiledata, (buff + fileSize), val, valsize))
        {
            OSCL_DELETE(ipDecoderinfo);
            break;
        }
        temp = 0;
        PV_atoi(val, 'd', (uint32&)temp);
        ipDecoderinfo->tRgba[0] = temp;

        if (!DecoderInfo(iptextfiledata, (buff + fileSize), val, valsize))
        {
            OSCL_DELETE(ipDecoderinfo);
            break;
        }
        temp = 0;
        PV_atoi(val, 'd', (uint32&)temp);
        ipDecoderinfo->tRgba[1] = temp;

        if (!DecoderInfo(iptextfiledata, (buff + fileSize), val, valsize))
        {
            OSCL_DELETE(ipDecoderinfo);
            break;
        }
        temp = 0;
        PV_atoi(val, 'd', (uint32&)temp);
        ipDecoderinfo->tRgba[2] = temp;

        if (!DecoderInfo(iptextfiledata, (buff + fileSize), val, valsize))
        {
            OSCL_DELETE(ipDecoderinfo);
            break;
        }
        temp = 0;
        PV_atoi(val, 'd', (uint32&)temp);
        ipDecoderinfo->tRgba[3] = temp;

        if (!DecoderInfo(iptextfiledata, (buff + fileSize), val, valsize))
        {
            OSCL_DELETE(ipDecoderinfo);
            break;
        }
        temp = 0;
        PV_atoi(val, 'd', (uint32&)temp);
        ipDecoderinfo->fontListSize = temp;

        if (!DecoderInfo(iptextfiledata, (buff + fileSize), val, valsize))
        {
            OSCL_DELETE(ipDecoderinfo);
            break;
        }
        temp = 0;
        PV_atoi(val, 'd', (uint32&)temp);
        ipDecoderinfo->fontListID = temp;

        if (!DecoderInfo(iptextfiledata, (buff + fileSize), val, valsize))
        {
            OSCL_DELETE(ipDecoderinfo);
            break;
        }
        temp = 0;
        PV_atoi(val, 'd', (uint32&)temp);
        ipDecoderinfo->font_id = temp;

        if (!DecoderInfo(iptextfiledata, (buff + fileSize), val, valsize))
        {
            OSCL_DELETE(ipDecoderinfo);
            break;
        }

        temp = 0;
        PV_atoi(val, 'd', (uint32&)temp);
        ipDecoderinfo->font_length = temp;

        if (ipDecoderinfo->font_length > 0)
        {
            ipDecoderinfo->font_name = (uint8 *)(OSCL_MALLOC(ipDecoderinfo->font_length * sizeof(char) + 1));
            if (!DecoderInfo(iptextfiledata, (buff + fileSize), val, valsize))
            {
                OSCL_DELETE(ipDecoderinfo);
                break;
            }
            oscl_strncpy((char *)ipDecoderinfo->font_name, val,
                         ipDecoderinfo->font_length);
        }

        if (!DecoderInfo(iptextfiledata, (buff + fileSize), val, valsize))
        {
            OSCL_DELETE(ipDecoderinfo);
            break;
        }
        PV_atoi(val, 'd', (uint32&)ipDecoderinfo->end_sample_num);
        uint32 length = sizeof(ipDecoderinfo) + 2 * DEFAULT_RGB_ARRAY_SIZE +
                        ipDecoderinfo->font_length;
        //allocate KVP
        PvmiKvp* aKvp = NULL;
        PVMFStatus status = PVMFSuccess;
        status = AllocateKvp(aKvp, (PvmiKeyType)TIMED_TEXT_OUTPUT_CONFIG_INFO_CUR_VALUE, 1);
        if (status != PVMFSuccess)
        {
            OSCL_DELETE(ipDecoderinfo);
            ipDecoderinfo = NULL;
            OSCL_FREE(val);
            val = NULL;
            return PVMFFailure;
        }
        aKvp->value.key_specific_value = ipDecoderinfo;
        aKvp->capacity = length;
        if (iPeerCapConfig != NULL)
        {
            PvmiKvp* retKvp = NULL; // for return value
            int32 err;
            OSCL_TRY(err, iPeerCapConfig->setParametersSync(NULL,
                     aKvp, 1, retKvp););
            PVA_FF_TextSampleDescInfo* textInfo =
                OSCL_STATIC_CAST(PVA_FF_TextSampleDescInfo*,
                                 aKvp->value.key_specific_value);
            OSCL_DELETE(textInfo);
            iAlloc.deallocate(aKvp);
            if (err != 0)
            {
                OSCL_FREE(val);
                val = NULL;
                return PVMFFailure;
            }
        }
        else
        {
            iAlloc.deallocate(aKvp);
            OSCL_FREE(val);
            val = NULL;
            return PVMFFailure;
        }
    }
    if (val)
    {
        OSCL_FREE(val);
        val = NULL;
    }
    iptextfiledata = buff;
    iAlloc.deallocate((OsclAny*)iptextfiledata);

    //closing text file
    if (iFileOpened_text)
    {
        iTextFile.Close();
        iFileOpened_text = false;
    }

    if (iFsOpened_text)
    {
        iFs_text.Close();
        iFsOpened_text = false;
    }
#endif
    return PVMFSuccess;
}


uint8* PvmiMIOFileInput::AllocateMemPool(OsclMemPoolFixedChunkAllocator*& aMediaBufferMemPool,
        uint32 aDataSize, int32 &arErr)
{
    uint8* data = NULL;
    OSCL_TRY(arErr, data = (uint8*)aMediaBufferMemPool->allocate(aDataSize););
    return data;
}

int32 PvmiMIOFileInput::WriteAsyncDataHdr(uint32& aWriteAsyncID,
        PvmiMediaTransfer*& aPeer,
        uint32& aBytesToRead,
        PvmiMediaXferHeader& aData_hdr)
{
    int err = 0;
    OSCL_TRY(err, aWriteAsyncID = aPeer->writeAsync(PVMI_MEDIAXFER_FMT_TYPE_NOTIFICATION,
                                  PVMI_MEDIAXFER_FMT_INDEX_END_OF_STREAM,
                                  NULL, aBytesToRead, aData_hdr););
    return err;
}

PVMFStatus PvmiMIOFileInput::RetrieveNALType()
{
    if (iSettings.iMediaFormat != PVMF_MIME_ISO_AVC_SAMPLE_FORMAT)
    {
        return PVMFFailure;
    }

    bool close_file_on_return = (!iFsOpened || !iFileOpened);

    if (!iFsOpened)
    {
        if (iFs.Connect() != 0)
            return PVMFFailure;
        iFsOpened = true;
    }

    if (!iFileOpened)
    {
        int32 ret = iInputFile.Open(iSettings.iFileName.get_cstr(),
                                    Oscl_File::MODE_READ | Oscl_File::MODE_BINARY, iFs);
        if (ret != 0)
        {

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PvmiMIOFileInput::DoInit: Error - iInputFile.Open failed, status=%d", ret));
            return PVMFFailure;
        }
        iFileOpened = true;
    }
    //extract NALLengthSize here
    int32 offset = 0;
    iInputFile.Seek(offset, Oscl_File::SEEKSET);
    offset += 1;  //ProfileIndication
    offset += 1;  //ProfileCompatibility
    offset += 1;  //LevelIndication
    iInputFile.Seek(offset, Oscl_File::SEEKSET);
    iInputFile.Read(&iNALType, sizeof(uint8), 1);  //the first byte of bitstream is Nal length type
    offset = 0;
    iInputFile.Seek(offset, Oscl_File::SEEKSET);

    if (close_file_on_return)
        CloseInputFile();
    return PVMFSuccess;
}

PVMFStatus PvmiMIOFileInput::RetrieveFSI(uint32 fsi_size)
{
    if ((iSettings.iMediaFormat != PVMF_MIME_M4V) && (iSettings.iMediaFormat != PVMF_MIME_H264_VIDEO_RAW))
    {
        return PVMFFailure;
    }

    iFormatSpecificInfoSize = 0;
    if (fsi_size == 0)
    {
        fsi_size = PVMIOFILEIN_MAX_FSI_SIZE;
    }

    bool close_file_on_return = (!iFsOpened || !iFileOpened);

    if (!iFsOpened)
    {
        if (iFs.Connect() != 0)
            return PVMFFailure;
        iFsOpened = true;
    }

    if (!iFileOpened)
    {
        int32 ret = iInputFile.Open(iSettings.iFileName.get_cstr(),
                                    Oscl_File::MODE_READ | Oscl_File::MODE_BINARY, iFs);
        if (ret != 0)
        {

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PvmiMIOFileInput::DoInit: Error - iInputFile.Open failed, status=%d", ret));
            return PVMFFailure;
        }
        iFileOpened = true;
    }

    // Allocate memory for VOL header
    uint8* fileData = (uint8*) iAlloc.allocate(fsi_size);
    if (!fileData)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "PvmiMIOFileInput::RetrieveFSI: Error - Failed to allocate memory for FSI"));
        if (close_file_on_return)
            CloseInputFile();
        return PVMFErrNoMemory;
    }

    // Read to data buffer
    if (iInputFile.Read((OsclAny*)fileData, sizeof(uint8),
                        fsi_size) != fsi_size)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "PvmiMIOFileInput::RetrieveFSI: Error - Failed to read from file"));
        if (close_file_on_return)
            CloseInputFile();
        iAlloc.deallocate(fileData);
        return PVMFFailure;
    }

    if (iSettings.iMediaFormat == PVMF_MIME_M4V)
    {
        // Anything before the first frame is assumed to be FSI
        iFormatSpecificInfoSize = LocateM4VFrameHeader(fileData, fsi_size);
    }
    else
    {
        // two first frames include SPS and PPS
        iFormatSpecificInfoSize = GetNextNALSize(fileData, fsi_size);
        iFormatSpecificInfoSize += GetNextNALSize(fileData + iFormatSpecificInfoSize, fsi_size - iFormatSpecificInfoSize);
    }


    if (iFormatSpecificInfoSize == 0)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "PvmiMIOFileInput::RetrieveFSI: Error - Failed to locate M4V Frame Header"));
        iAlloc.deallocate(fileData);
        if (close_file_on_return)
            CloseInputFile();
        return PVMFFailure;
    }

    //allocate KVP
    if (iFSIKvp)
    {
        // if already allocated- need to deallocate before reallocating.
        DoReset();
    }
    PVMFStatus status = AllocateKvp(iFSIKvp,
                                    (PvmiKeyType)PVMF_FORMAT_SPECIFIC_INFO_KEY, 1);
    if (status != PVMFSuccess)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "PvmiMIOFileInput::RetrieveFSI: Error - Failed to allocate KVP"));
        if (close_file_on_return)
            CloseInputFile();
        iAlloc.deallocate(fileData);
        return status;
    }

    iFSIKvp->value.key_specific_value = fileData;
    iFSIKvp->capacity = fsi_size;
    iFSIKvp->length = iFormatSpecificInfoSize;

    if (close_file_on_return)
        CloseInputFile();
    return PVMFSuccess;
}

PVMFStatus PvmiMIOFileInput::Send_SPS_PPS_info()
{
    uint8* my_ptr1;
    uint8* my_ptr2;
    int i = 0;
    PVMFStatus status = PVMFSuccess;
    uint8 SPSsets;
    uint8 PPSsets;
    OsclMemoryFragment iFormat_PPS_Info;
    OsclMemoryFragment iFormat_SPS_Info;
    uint8 NALLength;
    uint16 iFormat_SPS_size, iFormat_PPS_size;
    uint32 offset = 0;
    PvmiKvp* aKvp = NULL;

    iFormat_SPS_Info.len = 0;
    iFormat_PPS_Info.len = 0;

    iInputFile.Seek(offset, Oscl_File::SEEKSET);
    offset += 1;  //ProfileIndication
    offset += 1;  //ProfileCompatibility
    offset += 1;  //LevelIndication
    iInputFile.Seek(offset, Oscl_File::SEEKSET);

    iInputFile.Read(&NALLength, sizeof(uint8), 1);  //the first byte of bitstream is Nal length type
    iNALType = NALLength;
    iInputFile.Read(&SPSsets, sizeof(uint8), 1);
    for (i = 0; i < SPSsets; i++)
    {
        iInputFile.Read(&iFormat_SPS_size, sizeof(uint8), 2);
        my_ptr1 = OSCL_NEW((uint8[iFormat_SPS_size]), ());
        iFormat_SPS_Info.ptr = (OsclAny*)(my_ptr1);
        iFormat_SPS_Info.len = iInputFile.Read((OsclAny*)iFormat_SPS_Info.ptr,
                                               sizeof(uint8), iFormat_SPS_size);
        if (iFormat_SPS_Info.len > 0)
        {
            status = AllocateKvp(aKvp,
                                 (PvmiKeyType)VIDEO_AVC_OUTPUT_SPS_CUR_VALUE, 1);
            if (status != PVMFSuccess)
            {
                OSCL_DELETE(my_ptr1);
                return PVMFFailure;
            }
            aKvp->value.key_specific_value = iFormat_SPS_Info.ptr;
            aKvp->capacity = iFormat_SPS_Info.len;
            if (iPeerCapConfig != NULL)
            {
                PvmiKvp* retKvp = NULL; // for return value
                iPeerCapConfig->setParametersSync(NULL, aKvp, 1, retKvp);
                OSCL_DELETE(my_ptr1);
                iAlloc.deallocate(aKvp);
                if (retKvp != NULL)
                {
                    return PVMFFailure;
                }
            }
            else
            {
                OSCL_DELETE(my_ptr1);
                iAlloc.deallocate(aKvp);
                return PVMFFailure;
            }
        }
    }
    //for PPS information
    iInputFile.Read(&PPSsets, sizeof(uint8), 1);
    offset++;
    for (i = 0; i < PPSsets; i++)
    {
        iInputFile.Read(&iFormat_PPS_size, sizeof(uint8), 2);
        my_ptr2 = OSCL_NEW(uint8[iFormat_PPS_size], ());
        iFormat_PPS_Info.ptr = (OsclAny*)(my_ptr2);
        iFormat_PPS_Info.len = iInputFile.Read((OsclAny*)iFormat_PPS_Info.ptr,
                                               sizeof(int8), iFormat_PPS_size);
        if (iFormat_PPS_Info.len > 0)
        {
            status = AllocateKvp(aKvp,
                                 (PvmiKeyType)VIDEO_AVC_OUTPUT_PPS_CUR_VALUE, 1);
            if (status != PVMFSuccess)
            {
                OSCL_DELETE(my_ptr2);
                return PVMFFailure;
            }
            aKvp->value.key_specific_value = iFormat_PPS_Info.ptr;
            aKvp->capacity = iFormat_PPS_Info.len;
            if (iPeerCapConfig != NULL)
            {
                PvmiKvp* retKvp = NULL; // for return value
                iPeerCapConfig->setParametersSync(NULL, aKvp, 1, retKvp);
                OSCL_DELETE(my_ptr2);
                iAlloc.deallocate(aKvp);
                if (retKvp != NULL)
                {
                    return PVMFFailure;
                }
            }
            else
            {
                OSCL_DELETE(my_ptr2);
                iAlloc.deallocate(aKvp);
                return PVMFFailure;
            }
        }
    }
    return PVMFSuccess;
}


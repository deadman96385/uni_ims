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
#include "pvmf_event_handling.h"

// TODO: header file is bloated with inlined code. Move it here.

// ----------------------------------------------------------------------
// PVMFCmdResp

/**
 * Constructor for PVMFCmdResp
 */
OSCL_EXPORT_REF
PVMFCmdResp::PVMFCmdResp(PVMFCommandId aId,
                         const OsclAny* aContext,
                         PVMFStatus aStatus,
                         OsclAny* aEventData):
        iId(aId),
        iContext(aContext),
        iStatus(aStatus),
        iEventExtInterface(NULL),
        iEventData(aEventData)
{
    iEventDataLengthAvailable = false;
    iEventDataLength = 0;
}

/**
 * Constructor for PVMFCmdResp
 */
OSCL_EXPORT_REF
PVMFCmdResp::PVMFCmdResp(PVMFCommandId aId,
                         const OsclAny* aContext,
                         PVMFStatus aStatus,
                         PVInterface* aEventExtInterface):
        iId(aId),
        iContext(aContext),
        iStatus(aStatus),
        iEventExtInterface(aEventExtInterface),
        iEventData(NULL)
{
    iEventDataLengthAvailable = false;
    iEventDataLength = 0;
}

/**
 * Constructor for PVMFCmdResp
 */
OSCL_EXPORT_REF
PVMFCmdResp::PVMFCmdResp(PVMFCommandId aId,
                         const OsclAny* aContext,
                         PVMFStatus aStatus,
                         PVInterface* aEventExtInterface,
                         OsclAny* aEventData):
        iId(aId),
        iContext(aContext),
        iStatus(aStatus),
        iEventExtInterface(aEventExtInterface),
        iEventData(aEventData)
{
    iEventDataLengthAvailable = false;
    iEventDataLength = 0;
}

OSCL_EXPORT_REF PVMFCmdResp::~PVMFCmdResp()
{
}

OSCL_EXPORT_REF PVMFEventCategory PVMFCmdResp::IsA() const
{
    return PVMFCmdRespEvent;
}

// ----------------------------------------------------------------------
// PVMFAsyncEvent

OSCL_EXPORT_REF
PVMFAsyncEvent::PVMFAsyncEvent(PVMFEventCategory aEventCategory,
                               PVMFEventType aEventType,
                               OsclAny* aContext,
                               OsclAny* aEventData) :
        iEventCategory(aEventCategory)
        , iEventType(aEventType)
        , iEventExtInterface(NULL)
        , iLocalBufferSize(0)
        , iContext(aContext)
        , iEventData(aEventData)
{
    oscl_memset(iLocalBuffer, 0, PVMF_ASYNC_EVENT_LOCAL_BUF_SIZE);
    iEventDataLengthAvailable = false;
    iEventDataLength = 0;
}

OSCL_EXPORT_REF
PVMFAsyncEvent::PVMFAsyncEvent(PVMFEventCategory aEventCategory,
                               PVMFEventType aEventType,
                               OsclAny* aContext,
                               OsclAny* aEventData,
                               const void* aLocalBuffer,
                               const size_t aLocalBufferSize) :
        iEventCategory(aEventCategory)
        , iEventType(aEventType)
        , iEventExtInterface(NULL)
        , iLocalBufferSize(aLocalBufferSize)
        , iContext(aContext)
        , iEventData(aEventData)
{
    if (aLocalBuffer)
    {
        OSCL_ASSERT(iLocalBufferSize <= PVMF_ASYNC_EVENT_LOCAL_BUF_SIZE);
        if (iLocalBufferSize > PVMF_ASYNC_EVENT_LOCAL_BUF_SIZE)
        {
            iLocalBufferSize = PVMF_ASYNC_EVENT_LOCAL_BUF_SIZE;
        }

        oscl_memcpy(iLocalBuffer, aLocalBuffer, iLocalBufferSize);
    }
    else
    {
        oscl_memset(iLocalBuffer, 0, PVMF_ASYNC_EVENT_LOCAL_BUF_SIZE);
    }
    iEventDataLengthAvailable = false;
    iEventDataLength = 0;
}

OSCL_EXPORT_REF
PVMFAsyncEvent::PVMFAsyncEvent(PVMFEventCategory aEventCategory,
                               PVMFEventType aEventType,
                               OsclAny* aContext,
                               PVInterface* aEventExtInterface,
                               OsclAny* aEventData) :
        iEventCategory(aEventCategory)
        , iEventType(aEventType)
        , iEventExtInterface(aEventExtInterface)
        , iLocalBufferSize(0)
        , iContext(aContext)
        , iEventData(aEventData)
{
    oscl_memset(iLocalBuffer, 0, PVMF_ASYNC_EVENT_LOCAL_BUF_SIZE);
    iEventDataLengthAvailable = false;
    iEventDataLength = 0;
}

OSCL_EXPORT_REF
PVMFAsyncEvent::PVMFAsyncEvent(PVMFEventCategory aEventCategory,
                               PVMFEventType aEventType,
                               OsclAny* aContext,
                               PVInterface* aEventExtInterface,
                               OsclAny* aEventData,
                               const void* aLocalBuffer,
                               const size_t aLocalBufferSize) :
        iEventCategory(aEventCategory)
        , iEventType(aEventType)
        , iEventExtInterface(aEventExtInterface)
        , iLocalBufferSize(aLocalBufferSize)
        , iContext(aContext)
        , iEventData(aEventData)
{
    if (aLocalBuffer)
    {
        OSCL_ASSERT(iLocalBufferSize <= PVMF_ASYNC_EVENT_LOCAL_BUF_SIZE);
        if (iLocalBufferSize > PVMF_ASYNC_EVENT_LOCAL_BUF_SIZE)
        {
            iLocalBufferSize = PVMF_ASYNC_EVENT_LOCAL_BUF_SIZE;
        }

        oscl_memcpy(iLocalBuffer, aLocalBuffer, iLocalBufferSize);
    }
    else
    {
        oscl_memset(iLocalBuffer, 0, PVMF_ASYNC_EVENT_LOCAL_BUF_SIZE);
    }
    iEventDataLengthAvailable = false;
    iEventDataLength = 0;
}

OSCL_EXPORT_REF PVMFAsyncEvent::~PVMFAsyncEvent()
{
}

OSCL_EXPORT_REF PVMFEventCategory PVMFAsyncEvent::IsA() const
{
    return iEventCategory;
}

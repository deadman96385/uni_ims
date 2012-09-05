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
// -*- c++ -*-
// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

//           P V F I L E

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =


/**
 * @file pvfile.cpp
 * @brief This include file contains the methods to access a file either
 *    through Oscl Fil I/O or CPM.
 */

#include "pvfile.h"
#include "pvmf_cpmplugin_access_interface_factory.h"
#include "pvmi_data_stream_interface.h"

OSCL_EXPORT_REF TOsclFileOffset PVFile::Seek(TOsclFileOffset offset, Oscl_File::seek_type origin)
{
    if (iFile)
        return iFile->Seek(offset, origin);
    else if (iFilePtr)
        return iFilePtr->Seek(offset, origin);
    else if (iDataStreamAccess)
    {
        PvmiDataStreamSeekType seekType = PVDS_SEEK_CUR;
        if (origin == Oscl_File::SEEKSET)
        {
            seekType = PVDS_SEEK_SET;
        }
        if (origin == Oscl_File::SEEKCUR)
        {
            seekType = PVDS_SEEK_CUR;
        }
        if (origin == Oscl_File::SEEKEND)
        {
            seekType = PVDS_SEEK_END;
        }
        TOsclFileOffset tempOffset = offset;
        PvmiDataStreamStatus status =
            iDataStreamAccess->Seek(iDataStreamSession,
                                    tempOffset,
                                    seekType);
        if (status == PVDS_SUCCESS) return 0;
    }
    return(-1);//error
}

OSCL_EXPORT_REF TOsclFileOffset PVFile::Tell()
{
    if (iFile)
        return iFile->Tell();
    else if (iFilePtr)
        return iFilePtr->Tell();
    else if (iDataStreamAccess)
        return iDataStreamAccess->GetCurrentPointerPosition(iDataStreamSession);
    return (-1);//error
}

OSCL_EXPORT_REF uint32 PVFile::Read(OsclAny *buffer,
                                    uint32 size,
                                    uint32 numelements)
{
    if (iFile)
        return iFile->Read(buffer, size, numelements);
    else if (iFilePtr)
        return iFilePtr->Read(buffer, size, numelements);
    else if (iDataStreamAccess)
    {
        PvmiDataStreamStatus status =
            iDataStreamAccess->Read(iDataStreamSession,
                                    (uint8*)(buffer),
                                    size,
                                    numelements);
        if (status == PVDS_SUCCESS)
            return (numelements);
    }
    return 0; //error
}

OSCL_EXPORT_REF int32 PVFile::Flush()
{
    if (iFile)
        return iFile->Flush();
    else if (iFilePtr)
        return iFilePtr->Flush();
    else if (iDataStreamAccess)
    {
        PvmiDataStreamStatus status =
            iDataStreamAccess->Flush(iDataStreamSession);

        if (status == PVDS_SUCCESS)
            return 0;
    }
    return (-1); //error
}

OSCL_EXPORT_REF int32 PVFile::Close()
{
    int32 result = -1;
    if (iFilePtr)
    {
        result = -1;//Close should not be called for filePtr access.
    }
    else if (iDataStreamAccess)
    {
        iDataStreamAccess->CloseSession(iDataStreamSession);
        PVUuid uuid = PVMIDataStreamSyncInterfaceUuid;
        iCPMAccessFactory->DestroyPVMFCPMPluginAccessInterface(uuid, iDataStreamAccess);
        iDataStreamAccess = NULL;
        result = 0;
    }
    else if (iFile)
    {
        //if using a file handle, don't actually close the
        //file, just flush it.
        if (iFileHandle)
            result = iFile->Flush();

        // call Close in either case.
        result = iFile->Close();

        //delete the file object.
        OSCL_DELETE(iFile);
        iFile = NULL;
    }
    Reset(); // Reset all the internal flags
    return result;
}


OSCL_EXPORT_REF int32 PVFile::Open(const oscl_wchar *filename,
                                   uint32 mode,
                                   Oscl_FileServer& fileserv)
{
    if (iFilePtr)
    {
        return -1;//Open should not be called for filePtr access.
    }
    else if (iCPMAccessFactory)
    {
        if (iDataStreamAccess)
            return (-1);//already open!

        //Create an access interface.
        PVUuid uuid = PVMIDataStreamSyncInterfaceUuid;
        iDataStreamAccess = (PVMIDataStreamSyncInterface*)iCPMAccessFactory->CreatePVMFCPMPluginAccessInterface(uuid);
        if (iDataStreamAccess == NULL)
        {
            return (-1);//error, no access.
        }
        else
        {
            // PVFile will have a DataStream Session on one-to-one basis.
            // For each DataStream Session we need to have a different PVFile Instance.
            PvmiDataStreamRandomAccessType randomAccessType =
                iDataStreamAccess->QueryRandomAccessCapability();
            if (randomAccessType == PVDS_FULL_RANDOM_ACCESS)
            {
                PvmiDataStreamStatus status = PVDS_FAILURE;
                if (mode & Oscl_File::MODE_READWRITE)
                {
                    status =
                        iDataStreamAccess->OpenSession(iDataStreamSession,
                                                       PVDS_READ_WRITE);
                }
                else if (mode & Oscl_File::MODE_READ)
                {
                    status =
                        iDataStreamAccess->OpenSession(iDataStreamSession,
                                                       PVDS_READ_ONLY);
                }
                else if (mode & Oscl_File::MODE_APPEND)
                {
                    status =
                        iDataStreamAccess->OpenSession(iDataStreamSession,
                                                       PVDS_APPEND);
                }

                if (status == PVDS_SUCCESS)
                {
                    return 0;
                }
            }
            iCPMAccessFactory->DestroyPVMFCPMPluginAccessInterface(uuid, OSCL_STATIC_CAST(PVInterface*, iDataStreamAccess));
            iDataStreamAccess = NULL;
            return -1;
        }
    }
    //otherwise open the content using Oscl File I/O.
    else
    {
        if (iFile)
            return (-1); //already open!

        //Create an Oscl_File object for accessing the file, using
        //the optional external file handle.

        iFile = OSCL_NEW(Oscl_File, (iOsclFileCacheParams.iCacheSize, iFileHandle));
        if (!iFile)
            return (-1);//nonzero indicates error.

        // If a filehandle is provided, assume the file is already opened
        // (but still call Oscl_File::Open() with an arbitrary filename, since
        // otherwise it won't be initialized correctly. This filename will
        // be ignored and the handle will be used instead),
        // Otherwise, open it using its filename.

        int32 result;
        if (iFileHandle)
            result =  iFile->Open("", mode, fileserv);
        else
        {
            iFile->SetAsyncReadBufferSize(iOsclFileCacheParams.iAsyncReadBuffSize);
            iFile->SetLoggingEnable(iOsclFileCacheParams.iPVLoggerEnableFlag);


            iFile->SetPVCacheSize(iOsclFileCacheParams.iCacheSize);


            iFile->SetSummaryStatsLoggingEnable(iOsclFileCacheParams.iPVLoggerStateEnableFlag);
            result = iFile->Open(filename, mode, fileserv);
        }

        //If open failed, cleanup the file object
        if (result != 0)
        {
            OSCL_DELETE(iFile);
            iFile = NULL;
        }
        return result;
    }
}

OSCL_EXPORT_REF bool PVFile::GetRemainingBytes(TOsclFileOffset& aNumBytes)
{
    bool result = false;
    if (iFile)
    {
        TOsclFileOffset currPos = iFile->Tell();

        if (iFileSizeAvailable == false)
        {
            iFile->Seek(0, Oscl_File::SEEKEND);
            iFileSize = iFile->Tell();
            iFile->Seek(currPos, Oscl_File::SEEKSET);
            iFileSizeAvailable = true;
        }
        if (currPos <= iFileSize)
        {
            aNumBytes = (iFileSize - currPos);
            result = true;
        }
    }
    else if (iFilePtr)
    {
        TOsclFileOffset currPos = iFilePtr->Tell();

        if (iFileSizeAvailable == false)
        {
            iFilePtr->Seek(currPos, Oscl_File::SEEKSET);
            iFileSize = iFilePtr->Size();
            iFileSizeAvailable = true;
        }
        if (currPos <= iFileSize)
        {
            aNumBytes = (iFileSize - currPos);
            result = true;
        }
    }
    else if (iDataStreamAccess)
    {
        TOsclFileOffset tempNumBytes = 0;
        PvmiDataStreamStatus status =
            iDataStreamAccess->QueryReadCapacity(iDataStreamSession, tempNumBytes);

        aNumBytes = tempNumBytes;
        if ((status == PVDS_SUCCESS) || (status == PVDS_END_OF_STREAM))
        {
            result = true;
        }
    }
    return result;
}

OSCL_EXPORT_REF bool
PVFile::RequestReadCapacityNotification(PvmiDataStreamObserver& aObserver,
                                        TOsclFileOffset aCapacity,
                                        OsclAny* aContextData)
{
    TOsclFileOffset tempCapacity = aCapacity;
    if (iDataStreamAccess)
    {
        int32 errcode = 0;
        OSCL_TRY(errcode,
                 iRequestReadCapacityNotificationID =
                     iDataStreamAccess->RequestReadCapacityNotification(iDataStreamSession,
                             aObserver,
                             tempCapacity,
                             aContextData)
                );
        OSCL_FIRST_CATCH_ANY(errcode, return false);

        return true;
    }
    return false;
}

OSCL_EXPORT_REF TOsclFileOffset
PVFile::GetContentLength()
{
    // content length
    TOsclFileOffset fileSize = 0;
    if (iDataStreamAccess)
    {
        fileSize = iDataStreamAccess->GetContentLength();
    }

    return fileSize;
}


OSCL_EXPORT_REF uint32
PVFile::GetFileBufferingCapacity()
{
    // size of cache
    uint32 bufferSize = 0;
    if (iDataStreamAccess)
    {
        bufferSize = iDataStreamAccess->QueryBufferingCapacity();
    }

    return bufferSize;
}

OSCL_EXPORT_REF bool
PVFile::MakePersistent(TOsclFileOffset offset, uint32 size)
{
    PvmiDataStreamStatus status = PVDS_FAILURE;
    if (iDataStreamAccess)
    {
        status = iDataStreamAccess->MakePersistent(offset, size);
    }

    if (PVDS_SUCCESS == status)
    {
        return true;
    }

    return false;
}

OSCL_EXPORT_REF bool
PVFile::CancelNotificationSync()
{
    if (iDataStreamAccess)
    {
        PvmiDataStreamStatus status = iDataStreamAccess->CancelNotificationSync(iDataStreamSession);

        if (status == PVDS_SUCCESS)
            return true;
    }
    return false;
}

OSCL_EXPORT_REF int32
PVFile::Skip(TOsclFileOffset offset, Oscl_File::seek_type origin)
{
    if (iDataStreamAccess)
    {
        PvmiDataStreamSeekType seekType = PVDS_SEEK_CUR;
        if (origin == Oscl_File::SEEKSET)
        {
            seekType = PVDS_SKIP_SET;
        }
        if (origin == Oscl_File::SEEKCUR)
        {
            seekType = PVDS_SKIP_CUR;
        }
        if (origin == Oscl_File::SEEKEND)
        {
            seekType = PVDS_SKIP_END;
        }

        TOsclFileOffset tempOffset = offset;
        PvmiDataStreamStatus status = iDataStreamAccess->Seek(iDataStreamSession,
                                      tempOffset, seekType);
        if (status == PVDS_SUCCESS) return 0;
    }
    return (-1);//error
}


OSCL_EXPORT_REF void
PVFile::GetCurrentByteRange(TOsclFileOffset& aCurrentFirstByteOffset, TOsclFileOffset& aCurrentLastByteOffset)
{
    aCurrentFirstByteOffset = 0;
    aCurrentLastByteOffset = 0;

    if (iDataStreamAccess)
    {
        iDataStreamAccess->GetCurrentByteRange(aCurrentFirstByteOffset, aCurrentLastByteOffset);
    }
}


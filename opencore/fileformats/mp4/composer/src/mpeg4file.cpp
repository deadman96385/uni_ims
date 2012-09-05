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
/*
    The PVA_FF_Mpeg4File Class fp the class that will construct and maintain all the
    mecessary data structures to be able to render a valid MP4 file to disk.
    Format.
*/


#define IMPLEMENT_Mpeg4File

#ifndef OSCL_STRING_UTILS_H_INCLUDED
#include "oscl_string_utils.h"
#endif

#include "mpeg4file.h"
#include "a_atomdefs.h"
#include "atomutils.h"

#include "pv_gau.h"
#include "oscl_byte_order.h"
#include "oscl_bin_stream.h"

#include "pv_mp4ffcomposer_config.h"

const uint8 aAMRNBZeroSetMask[9] =
{
    0xfe, 0xfe, 0xfc, 0xfc,
    0xf0, 0xfe, 0xf0, 0xf0,
    0xfe
};
//IETF AMR WB Speech Frame Sizes (including zero byte padding but not including TOC)
//FT 0 (6.6 Kbps) - 17 bytes = 136 bits
//FT 1 (8.85 Kbps) - 23 bytes = 184 bits
//FT 2 (12.65 Kbps) - 32 bytes = 256 bits
//FT 3 (14.25 Kbps) - 36 bytes = 288 bits
//FT 4 (15.85 Kbps) - 40 bytes = 320 bits
//FT 5 (18.25 Kbps) - 46 bytes = 368 bits
//FT 6 (19.85 Kbps) - 50 bytes = 400 bits
//FT 7 (23.05 Kbps) - 58 bytes = 464 bits
//FT 8 (23.85 Kbps) - 60 bytes = 480 bits
//FT 9 (SID) - 5 bytes = 40 bits
//FT 10-13 - Reserved
//FT 14 (Lost frame) and FT 15 (NO DATA) - 0 bytes = 0 bits

//IETF AMR WB IF1 Speech Frame Sizes (just Class A, B & C speech bits, does not include FT or any other headers)
//FT 0 (6.6 Kbps) -  132 bits; num-bits-padded = 4
//FT 1 (8.85 Kbps) - 177 bits; num-bits-padded = 7
//FT 2 (12.65 Kbps) - 253 bits; num-bits-padded = 3
//FT 3 (14.25 Kbps) - 285 bits; num-bits-padded = 3
//FT 4 (15.85 Kbps) - 317 bits; num-bits-padded = 3
//FT 5 (18.25 Kbps) - 365 bits; num-bits-padded = 3
//FT 6 (19.85 Kbps) - 397 bits; num-bits-padded = 3
//FT 7 (23.05 Kbps) - 461 bits; num-bits-padded = 3
//FT 8 (23.85 Kbps) - 477 bits; num-bits-padded = 3
//FT 9 (SID) - 5 bytes = 40 bits; num-bits-padded = 0
//FT 10-13 - Reserved
//FT 14 (Lost frame) and FT 15 (NO DATA) - 0 bytes = 0 bits; num-bits-padded = 0

// Difference between IF1 bits and IETF storage bits is padded with zeros to byte align the frame
const uint8 aAMRWBZeroSetMask[9] =
{
    0xf0, 0x80, 0xf8, 0xf8,
    0xf8, 0xf8, 0xf8, 0xf8,
    0xf8
};

typedef Oscl_Vector<PVA_FF_MediaDataAtom*, OsclMemAllocator> PVA_FF_MediaDataAtomVecType;
typedef Oscl_Vector<PVA_FF_MovieFragmentAtom*, OsclMemAllocator> PVA_FF_MovieFragmentAtomVecType;
typedef Oscl_Vector<PVA_FF_InterLeaveBuffer*, OsclMemAllocator> PVA_FF_InterLeaveBufferVecType;

//common to both AMR and AMR-WB
const uint32 AMRModeSetMask[16] =
{
    0x0001, 0x0002, 0x0004, 0x0008,
    0x0010, 0x0020, 0x0040, 0x0080,
    0x0100, 0x0200, 0x0400, 0x0800,
    0x1000, 0x2000, 0x4000, 0x8000
};

// Constructor
PVA_FF_Mpeg4File::PVA_FF_Mpeg4File(int32 mediaType)
{
    OSCL_UNUSED_ARG(mediaType);
    _tempFilePostfix = _STRLIT("");
    _tempOutputPath = _STRLIT("");
    _oUserDataPopulated = true;
    _pmovieAtom = NULL;
    _pmediaDataAtomVec = NULL;
    _puserDataAtom = NULL;
    _pFileTypeAtom = NULL;
    _pCurrentMoofAtom = NULL;
    _pCurrentMediaDataAtom = NULL;
    iCacheSize = 0;
    _oIsFileOpen = false;
    _pInterLeaveBufferVec = NULL;
    _oInterLeaveEnabled = false;
    _oLiveMovieFragmentEnabled = false;
    _aFs = NULL;
    _pMfraAtom = NULL;

}

// Destructor
PVA_FF_Mpeg4File::~PVA_FF_Mpeg4File()
{

    {
        if (_oUserDataPopulated == false)
        {
            populateUserDataAtom();
        }
    }

    // Clean up atoms
    if (_pmovieAtom != NULL)
    {
        PV_MP4_FF_DELETE(NULL, PVA_FF_MovieAtom, _pmovieAtom);
    }

    int32 i;

    // Delete all the atoms in the media data vec
    if (_pmediaDataAtomVec != NULL)
    {
        int32 size = _pmediaDataAtomVec->size();
        for (i = 0; i < size; i++)
        {
            PV_MP4_FF_DELETE(NULL, PVA_FF_MediaDataAtom, (*_pmediaDataAtomVec)[i]);
        }

        // Delete the vectors themselves
        PV_MP4_FF_TEMPLATED_DELETE(NULL, PVA_FF_MediaDataAtomVecType, Oscl_Vector, _pmediaDataAtomVec);
    }

    if ((_oInterLeaveEnabled) && (NULL != _pInterLeaveBufferVec))
    {
        // delete all interleave buffers
        int32 size = _pInterLeaveBufferVec->size();
        for (i = 0; i < size; i++)
        {
            PV_MP4_FF_DELETE(NULL, PVA_FF_MediaDataAtom, (*_pInterLeaveBufferVec)[i]);
        }

        // Delete the vectors themselves
        PV_MP4_FF_TEMPLATED_DELETE(NULL, PVA_FF_InterLeaveBufferVecType, Oscl_Vector, _pInterLeaveBufferVec);
    }

    // in movie fragment mode delete MOOF and MFRA atoms
    if (_oMovieFragmentEnabled == true)
    {
        PV_MP4_FF_DELETE(NULL, PVA_FF_MovieFragmentAtom, _pCurrentMoofAtom);
        PV_MP4_FF_DELETE(NULL, PVA_FF_MediaDataAtom, _pCurrentMediaDataAtom);

        PV_MP4_FF_DELETE(NULL, PVA_FF_MovieFragmentRandomAccessAtom, _pMfraAtom);
    }

    // Delete user data if present
    if (_puserDataAtom != NULL)
    {
        PV_MP4_FF_DELETE(NULL, PVA_FF_UserDataAtom, _puserDataAtom);
    }

    if (_pFileTypeAtom != NULL)
    {
        PV_MP4_FF_DELETE(NULL, PVA_FF_FileTypeAtom, _pFileTypeAtom);
    }

    if (_oPIFFMode == true)
    {
        PV_MP4_FF_DELETE(NULL, PVA_FF_PSSHAtom, _pPSSHAtom);
    }

    if (_aFs)
    {
        PVA_FF_AtomUtils::closeFileSession(OSCL_STATIC_CAST(Oscl_FileServer*, _aFs));
    }
}

void PVA_FF_Mpeg4File::SetCacheSize(uint32 aCacheSize)
{
    iCacheSize = aCacheSize;
}
bool
PVA_FF_Mpeg4File::init(int32 mediaType,
                       void *osclFileServerSession,
                       uint32 fileAuthoringFlags)
{
    OSCL_UNUSED_ARG(mediaType);
    _modifiable = true; // Allow addition of media samples
    _firstFrameInLayer0 = true;
    _firstFrameInLayer1 = true;
    _fileWriteFailed = false;

    _o3GPPTrack = true;
    _oMPEGTrack = false;
    _oAVCTrack  = false;
    _oTextTrack = false;

    _oFileRenderCalled = false;
    _oUserDataPopulated = false;
    _oFtypPopulated = false;

    _baseOffset = 0;
    _oInterLeaveEnabled = false;
    _oMovieAtomUpfront = false;

    _oAuthorASSETINFOAtoms = false;
    _oChunkStart = false;

    // Movie Fragments flags initialised
    _oMovieFragmentEnabled      = false;
    _oLiveMovieFragmentEnabled  = false;
    _oComposeMoofAtom           = false;
    _movieFragmentDuration      = DEFAULT_MOVIE_FRAGMENT_DURATION_IN_MS;
    _pCurrentMoofAtom           = NULL;
    _pCurrentMediaDataAtom      = NULL;
    _currentMoofOffset          = 0;
    _sequenceNumber             = 0;


    _aFs = osclFileServerSession;

    _tempFileIndex = 'a';

    _pmediaDataAtomVec  = NULL;
    _pmovieAtom         = NULL;

    _puserDataAtom      = NULL;
    _pFileTypeAtom      = NULL;

    // PIFF related
    _pPSSHAtom          = NULL;

    _initialUserDataSize     = 0;

    _totalTempFileRemoval = false;
    _oUserDataUpFront     = true;
    _oIsFileOpen          = false;
    _oFirstSampleEditMode = false;

    _fileAuthoringFlags = fileAuthoringFlags;

    if (fileAuthoringFlags & PVMP4FF_SET_MEDIA_INTERLEAVE_MODE)
    {
        _oInterLeaveEnabled = true;
    }

    if (fileAuthoringFlags & PVMP4FF_SET_META_DATA_UPFRONT_MODE)
    {
        _oMovieAtomUpfront = true;
    }

    if ((fileAuthoringFlags & PVMP4FF_3GPP_DOWNLOAD_MODE) ==
            (PVMP4FF_3GPP_DOWNLOAD_MODE))
    {
        //Not possible to remove temp files, without output file name being set
        _oInterLeaveEnabled   = true;
        _totalTempFileRemoval = true;
        _oUserDataUpFront     = false;
    }

    if (fileAuthoringFlags & PVMP4FF_SET_FIRST_SAMPLE_EDIT_MODE)
    {
        /* Supported only if interleaving is enabled */
        if (!_oInterLeaveEnabled)
        {
            return false;
        }
        _oFirstSampleEditMode = true;
    }

    // Movie fragment mode
    if ((fileAuthoringFlags & PVMP4FF_MOVIE_FRAGMENT_MODE) == PVMP4FF_MOVIE_FRAGMENT_MODE)
    {
        if (!_oInterLeaveEnabled)
        {
            return false;
        }
        _oMovieFragmentEnabled = true;
        _totalTempFileRemoval = true;
        _oUserDataUpFront     = false;
    }

    // Live movie fragment mode
    // Note, this mode implies PVMP4FF_MOVIE_FRAGMENT_MODE, so both _oMovieFragmentEnabled and
    // _oLiveMovieFragmentEnabled will be set to true
    if ((fileAuthoringFlags & PVMP4FF_LIVE_MOVIE_FRAGMENT_MODE) == PVMP4FF_LIVE_MOVIE_FRAGMENT_MODE)
    {
        _oLiveMovieFragmentEnabled = true;
    }

    _oPIFFMode = false;
    if ((fileAuthoringFlags & PVMP4FF_PIFF_MODE) == PVMP4FF_PIFF_MODE)
    {
        _oPIFFMode = true;
    }



    // Create user data atom
    PV_MP4_FF_NEW(fp->auditCB, PVA_FF_UserDataAtom, (), _puserDataAtom);

    PV_MP4_FF_NEW(fp->auditCB, PVA_FF_FileTypeAtom, (), _pFileTypeAtom);

    // Create the moov atom
    PV_MP4_FF_NEW(fp->auditCB, PVA_FF_MovieAtom, (fileAuthoringFlags), _pmovieAtom);


    // Movie fragment atom vectors initialised
    if (_oMovieFragmentEnabled)
    {
        // use version 1 of the track fragment rand access box only for PVMP4FF_LIVE_MOVIE_FRAGMENT_MODE
        uint8 tfraversion = (uint8)0;
        if (_oLiveMovieFragmentEnabled)
            tfraversion = (uint8)1;
        PV_MP4_FF_NEW(fp->auditCB, PVA_FF_MovieFragmentRandomAccessAtom, (tfraversion), _pMfraAtom);
    }

    // Create miscellaneous vector of atoms
    PV_MP4_FF_NEW(fp->auditCB, PVA_FF_MediaDataAtomVecType, (), _pmediaDataAtomVec);

    // Create PSSH box only when the authoring mode is PIFF
    if (_oPIFFMode)
    {
        PV_MP4_FF_NEW(fp->auditCB, PVA_FF_PSSHAtom, (), _pPSSHAtom);
    }

    _pparent = NULL;

    /*
     * In interleave mode, create only ONE media atom, to store
     * all the media samples.
     */
    if (_oInterLeaveEnabled)
    {
        PV_MP4_FF_NEW(fp->auditCB, PVA_FF_InterLeaveBufferVecType, (), _pInterLeaveBufferVec);
        PVA_FF_MediaDataAtom *mda = NULL;
        PVA_FF_UNICODE_STRING_PARAM emptyString = PVA_FF_UNICODE_HEAP_STRING(_STRLIT_WCHAR(""));

        if (!_totalTempFileRemoval)
        {
            // Create PVA_FF_MediaDataAtom
            PV_MP4_FF_NEW(fp->auditCB, PVA_FF_MediaDataAtom, (emptyString, NULL,
                          _aFs, iCacheSize,
                          _tempOutputPath,
                          _tempFilePostfix,
                          _tempFileIndex),
                          mda);

            _tempFileIndex++;
        }
        else
        {
            if (_oFileOpenedOutsideAFFLib)
            {
                PV_MP4_FF_NEW(fp->auditCB, PVA_FF_MediaDataAtom, (emptyString, _outputFileHandle, _aFs, iCacheSize), mda);
            }
            else
            {
                PV_MP4_FF_NEW(fp->auditCB, PVA_FF_MediaDataAtom, (_outputFileName, NULL, _aFs, iCacheSize), mda);
            }
        }

        if (mda->_targetFileWriteError)
        {
            PV_MP4_FF_DELETE(NULL, PVA_FF_MediaDataAtom, mda);
            mda = NULL;
            return false;
        }
        addMediaDataAtom(mda);

        _interLeaveDuration = DEFAULT_INTERLEAVE_INTERVAL;
    }
    if ((fileAuthoringFlags & PVMP4FF_USE_EXTN_UDTA) != PVMP4FF_USE_EXTN_UDTA)
    {
        {
            _pmovieAtom->createAssetInfoAtoms();
        }
    }
    recomputeSize();

    return true;
}

bool
PVA_FF_Mpeg4File::setOutputFileName(PVA_FF_UNICODE_STRING_PARAM outputFileName)
{
    _outputFileName           = _STRLIT("");
    _outputFileHandle         = NULL;
    _targetFileHandle         = NULL;
    _oFileOpenedOutsideAFFLib = false;

    if (outputFileName.get_size() > 0)
    {
        _outputFileName   += outputFileName;
        return true;
    }
    return false;
}

bool
PVA_FF_Mpeg4File::setOutputFileHandle(MP4_AUTHOR_FF_FILE_HANDLE outputFileHandle)
{
    _outputFileHandle         = NULL;
    _targetFileHandle         = NULL;
    _oFileOpenedOutsideAFFLib = false;

    if (outputFileHandle != NULL)
    {
        _outputFileHandle  = outputFileHandle;
        _oFileOpenedOutsideAFFLib = true;
        return true;
    }
    return false;
}

uint32
PVA_FF_Mpeg4File::addTrack(int32 mediaType, PVA_FF_MP4_CODEC_TYPE codecType, uint32 aTrackId,
                           uint8 profile, uint8 profileComp, uint8 level)
{
    uint32 TrackID = 0;
    PVA_FF_TrackAtom *pmediatrack = NULL;
    _codecType = codecType;
    PVA_FF_MediaDataAtom *mda = NULL;
    PVA_FF_InterLeaveBuffer *pInterLeaveBuffer = NULL;

    // if aTrackId is specified, make sure a track with the same ID doesn't exist already
    if (aTrackId != 0)
    {
        // How do we fail this method? Returning zero may be interpreted by the caller
        // as a successful operation
        if (_pmovieAtom->IsTrackIdInUse(aTrackId))
            return 0;
    }


    if (!_oInterLeaveEnabled)
    {
        PVA_FF_UNICODE_STRING_PARAM emptyString = PVA_FF_UNICODE_HEAP_STRING(_STRLIT_WCHAR(""));
        //create new track - media will be stored in temp file
        PV_MP4_FF_NEW(fp->auditCB, PVA_FF_MediaDataAtom, (emptyString, NULL,
                      _aFs, iCacheSize,
                      _tempOutputPath,
                      _tempFilePostfix,
                      _tempFileIndex), mda);

        _tempFileIndex++;
        addMediaDataAtom(mda);
    }
    else
    {
        mda = getMediaDataAtomForTrack(0);
    }

    // For track creation, use version 1 of FullBox spec when _oLiveMovieFragmentEnabled is true
    uint8 fbVersion = 0;
    if (_oLiveMovieFragmentEnabled)
        fbVersion = 1;

    uint32 newtrack_Id;
    if (aTrackId != 0)
    {
        newtrack_Id = aTrackId;
        // notify the movie header atom of the next track Id to use if this is
        // not specified in subsequent calls to this method.
        _pmovieAtom->getMutableMovieHeaderAtom().setNextTrackID(aTrackId + 1);
    }
    else
    {
        newtrack_Id = _pmovieAtom->getMutableMovieHeaderAtom().findNextTrackID();
    }

    if ((uint32) mediaType == MEDIA_TYPE_AUDIO)
    {
        // Create default audio track and add it to moov atom
        PV_MP4_FF_NEW(fp->auditCB, PVA_FF_TrackAtom, (MEDIA_TYPE_AUDIO,
                      newtrack_Id,
                      fbVersion,
                      _fileAuthoringFlags,
                      codecType,
                      1, profile, profileComp, level),
                      pmediatrack);

        if (mda)
            mda->setTrackReferencePtr(pmediatrack);
        _pmovieAtom->addTrackAtom(pmediatrack);

        // add audio interleave buffer for track
        if (_oInterLeaveEnabled)
        {
            PV_MP4_FF_NEW(fp->auditCB, PVA_FF_InterLeaveBuffer, (MEDIA_TYPE_AUDIO,
                          codecType,
                          pmediatrack->getTrackID()),
                          pInterLeaveBuffer);

            addInterLeaveBuffer(pInterLeaveBuffer);
        }

        // Returns the index of the reference in the table to which this was
        // just added (with a 1-based index NOT a zero-based index)

        TrackID = pmediatrack->getTrackID();

        if ((codecType == PVA_FF_MP4_CODEC_TYPE_AMR_AUDIO) ||
                (codecType == PVA_FF_MP4_CODEC_TYPE_AMR_WB_AUDIO))
        {
            _o3GPPTrack = true;
        }
        if (codecType == PVA_FF_MP4_CODEC_TYPE_AAC_AUDIO)
        {
            _o3GPPTrack = true;
            _oMPEGTrack = true;
        }
    }

    if ((uint32) mediaType == MEDIA_TYPE_VISUAL)
    {
        if (codecType == PVA_FF_MP4_CODEC_TYPE_BASELINE_H263_VIDEO)
        {
            _o3GPPTrack = true;
        }
        else if (codecType == PVA_FF_MP4_CODEC_TYPE_AVC_VIDEO)
        {
            _o3GPPTrack = true;
            _oAVCTrack  = true;
        }
        else if (codecType == PVA_FF_MP4_CODEC_TYPE_MPEG4_VIDEO)
        {
            _o3GPPTrack = true;
            _oMPEGTrack = true;
        }

        // Create default video track and add it to moov atom
        PV_MP4_FF_NEW(fp->auditCB, PVA_FF_TrackAtom, (MEDIA_TYPE_VISUAL,
                      newtrack_Id,
                      fbVersion,
                      _fileAuthoringFlags,
                      codecType,
                      1, profile, profileComp, level),
                      pmediatrack);

        // add video interleave buffer for track

        if (_oInterLeaveEnabled)
        {
            PV_MP4_FF_NEW(fp->auditCB, PVA_FF_InterLeaveBuffer, (MEDIA_TYPE_VISUAL,
                          codecType,
                          pmediatrack->getTrackID()),
                          pInterLeaveBuffer);

            addInterLeaveBuffer(pInterLeaveBuffer);
        }

        if (mda)
            mda->setTrackReferencePtr(pmediatrack);
        _pmovieAtom->addTrackAtom(pmediatrack);

        // Returns the index of the reference in the table to which this was
        // just added (with a 1-based index NOT a zero-based index)
        TrackID = pmediatrack->getTrackID();
    }

    if ((uint32) mediaType == MEDIA_TYPE_TEXT)//added for the support of timed text track
    {
        if (codecType == PVA_FF_MP4_CODEC_TYPE_TIMED_TEXT)
        {
            _o3GPPTrack = true;
            _oTextTrack = true;
        }
        // Create default video track and add it to moov atom
        PV_MP4_FF_NEW(fp->auditCB, PVA_FF_TrackAtom, (MEDIA_TYPE_TEXT,
                      newtrack_Id,
                      fbVersion,
                      _fileAuthoringFlags,
                      codecType,
                      1,
                      profile, profileComp, level),
                      pmediatrack);

        // add text interleave buffer for track
        if (_oInterLeaveEnabled)
        {
            PV_MP4_FF_NEW(fp->auditCB, PVA_FF_InterLeaveBuffer, (MEDIA_TYPE_TEXT,
                          codecType,
                          pmediatrack->getTrackID()),
                          pInterLeaveBuffer);

            addInterLeaveBuffer(pInterLeaveBuffer);
        }

        mda->setTrackReferencePtr(pmediatrack);
        _pmovieAtom->addTrackAtom(pmediatrack);

        // Returns the index of the reference in the table to which this was
        // just added (with a 1-based index NOT a zero-based index)
        TrackID = pmediatrack->getTrackID();
    }

    // Add a new track fragment rand. access box for live movie fragment mode
    if (_oLiveMovieFragmentEnabled)
        _pMfraAtom->addTrackFragmentRandomAccessAtom(TrackID);

    recomputeSize();
    return (TrackID);
}

void
PVA_FF_Mpeg4File::addTrackReference(uint32 currtrackID, int32 reftrackID)
{
    PVA_FF_TrackAtom *pCurrTrack = _pmovieAtom->getMediaTrack(currtrackID);
    pCurrTrack->addTrackReference(reftrackID);
    return;
}

void
PVA_FF_Mpeg4File::setTargetBitrate(uint32 trackID, uint32 avgBitRate, uint32 maxBitRate, uint32 bufferSizeDB)
{
    _pmovieAtom->setTargetBitrate(trackID, avgBitRate, maxBitRate, bufferSizeDB);
    return;
}

void
PVA_FF_Mpeg4File::setTimeScale(uint32 trackID, uint32 rate)
{
    // Set the sample rate for the specific video track
    _pmovieAtom->setTimeScale(trackID, rate);
    return;
}

bool
PVA_FF_Mpeg4File::setTrackDuration(uint32 trackID, uint64 duration)
{
    PVA_FF_TrackAtom *mediaTrack = _pmovieAtom->getMediaTrack(trackID);
    if (mediaTrack == NULL)
        return false;

    // Track duration can only be set for mode PVMP4FF_LIVE_MOVIE_FRAGMENT_MODE
    if (_oLiveMovieFragmentEnabled == false)
        return false;

    PVA_FF_TrackHeaderAtom *ptkhdr = mediaTrack->getTrackHeaderAtomPtr();
    ptkhdr->setDuration(duration);

    return true;
}

//this will work same as the addsampletotrack but this
//will be called only for timed text file format
bool PVA_FF_Mpeg4File::addTextSampleToTrack(uint32 trackID, PVMP4FFComposerSampleParam *pSampleParam)
{
    if (0 == pSampleParam)
        return false;

    bool retVal = true;
    PVA_FF_TrackAtom* mediaTrack = _pmovieAtom->getMediaTrack(trackID);
    uint32 mediaType  = mediaTrack->getMediaType();
    PVA_FF_MP4_CODEC_TYPE codecType = _pmovieAtom->getCodecType(trackID);

    // Create media sample buffer and size field
    uint32 size = 0;
    if (!pSampleParam->_fragmentList.empty())
    {
        if (mediaType == MEDIA_TYPE_TEXT)//CALCULATES SIZE OF TIMED TEXT SAMPLE
        {
            for (uint32 ii = 0; ii < pSampleParam->_fragmentList.size(); ii++)
                size += pSampleParam->_fragmentList[ii].len;
        }
    }
    pSampleParam->_sampleSize = size;

    PVA_FF_MediaDataAtom *mdatAtom = getMediaDataAtomForTrack(trackID);

    if (mediaType == MEDIA_TYPE_TEXT)
    {
        if (_modifiable)
        {
            // The layer in the flags byte indicates which video track to add to
            // int32 trackNum = (int32)(flags & 0x70) >> 4;

            if (mediaTrack)
            {
                // Add to mdat PVA_FF_Atom for the specified track
                if (codecType == PVA_FF_MP4_CODEC_TYPE_TIMED_TEXT)
                {
                    if (_oInterLeaveEnabled)
                    {
                        if (!addTextMediaSampleInterleave(trackID, pSampleParam))
                        {
                            return false;
                        }
                    }
                    else
                    {
                        if (!mdatAtom->addRawSample((pSampleParam->_fragmentList), (pSampleParam->_sampleSize), mediaType, codecType))
                        {
                            retVal = false;
                        }
                        _pmovieAtom->addTextSampleToTrack(trackID, pSampleParam);
                    }
                }
            }
            else
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }

    return (retVal);
}

// Movie fragment Mode : APIs to set and get duration of each MOOF atom
void
PVA_FF_Mpeg4File::setMovieFragmentDuration(uint32 duration)
{
    _movieFragmentDuration = duration;
    _pmovieAtom->setMovieFragmentDuration(duration);
    return;
}



uint32
PVA_FF_Mpeg4File::getMovieFragmentDuration()
{
    return _movieFragmentDuration;
}


bool
PVA_FF_Mpeg4File::addSampleToTrack(uint32 trackID, PVMP4FFComposerSampleParam *pSampleParam)
{
    if (0 == pSampleParam)
        return false;

    PVA_FF_TrackAtom* mediaTrack = _pmovieAtom->getMediaTrack(trackID);
    if (0 == mediaTrack)
        return false;

    const uint32 mediaType = mediaTrack->getMediaType();
    const PVA_FF_MP4_CODEC_TYPE codecType = _pmovieAtom->getCodecType(trackID);

    bool retVal = true;

    // Create media sample buffer and size field
    uint32 size = 0;
    if (true == pSampleParam->_fragmentList.empty())
        return false;

    // compensate for AVC sample? length + '2' size of NAL unit length field
    const uint8 avc_extra_len = (mediaType == MEDIA_TYPE_VISUAL && codecType == PVA_FF_MP4_CODEC_TYPE_AVC_VIDEO) ? 4 : 0;

    // all memory fragments in the vector combines into one sample
    for (uint32 ii = 0; ii < pSampleParam->_fragmentList.size(); ++ii)
        size += (pSampleParam->_fragmentList[ii].len + avc_extra_len);

    pSampleParam->_sampleSize = size;

    PVA_FF_MediaDataAtom* mdatAtom = getMediaDataAtomForTrack(trackID);
    if (MEDIA_TYPE_AUDIO == mediaType)
    {
        if (!_modifiable)
            return false;

        if (PVA_FF_MP4_CODEC_TYPE_AMR_AUDIO == codecType ||
                PVA_FF_MP4_CODEC_TYPE_AMR_WB_AUDIO == codecType)
        {
            if (1 > size)
                return false;

            PVA_FF_TrackAtom *track = _pmovieAtom->getMediaTrack(trackID);
            if (track != NULL)
            {
                // FT is in the first byte that comes off the encoder
                pSampleParam->_flags = *((uint8*)(pSampleParam->_fragmentList.front().ptr));
                uint32 mode_set = 0;
                if (pSampleParam->_flags < 16)
                {
                    mode_set = AMRModeSetMask[(pSampleParam->_flags&0x0f)];
                }
                if (pSampleParam->_flags < 9)
                {
                    // JUST TO ENSURE THAT THE PADDED BITS ARE ZERO
                    OsclMemoryFragment fragment = pSampleParam->_fragmentList.back();
                    if (PVA_FF_MP4_CODEC_TYPE_AMR_AUDIO == codecType)
                    {
                        ((uint8*)fragment.ptr)[ fragment.len - 1] &= aAMRNBZeroSetMask[(pSampleParam->_flags&0x0f)];
                    }
                    else if (PVA_FF_MP4_CODEC_TYPE_AMR_WB_AUDIO == codecType)
                    {
                        ((uint8*)fragment.ptr)[ fragment.len - 1] &= aAMRWBZeroSetMask[(pSampleParam->_flags&0x0f)];
                    }

                }
                if (_oInterLeaveEnabled)
                {
                    if (!addMediaSampleInterleave(trackID, pSampleParam))
                        return false;
                }
                else
                {
                    // Add to mdat PVA_FF_Atom for the specified track
                    if (!mdatAtom->addRawSample(pSampleParam->_fragmentList,
                                                pSampleParam->_sampleSize,
                                                mediaType, codecType))
                    {
                        retVal = false;
                    }
                    // Add to moov atom (in turn adds to tracks)
                    _pmovieAtom->addSampleToTrack(trackID, pSampleParam);
                }
            }
        }
        else if (PVA_FF_MP4_CODEC_TYPE_AAC_AUDIO == codecType)
        {
            if (size > 0)
            {
                if (_oInterLeaveEnabled)
                {
                    if (!addMediaSampleInterleave(trackID, pSampleParam))
                        return false;
                }
                else
                {
                    // Add to mdat PVA_FF_Atom for the specified track
                    if (!mdatAtom->addRawSample((pSampleParam->_fragmentList), (size), mediaType, codecType))
                        retVal = false;

                    pSampleParam->_flags = 0;

                    // Add to moov atom (in turn adds to tracks)
                    _pmovieAtom->addSampleToTrack(trackID, pSampleParam);
                }
            }
        }
    }

    if (MEDIA_TYPE_VISUAL == mediaType)
    {
        // For the first frame in each layer, pull off the VOL header.  For the base layer
        // (layer=0), this fp the first 28 bytes.  For the enhancememnt(temporal) layer
        // (layer=1), this fp the first 17 bytes (compact version with no repeate headers).
        //
        // Note that this fp making the assumption that the first frame of each layer will
        // contain this VOL header information.  In the current encoder (version 1.0), this
        // fp true.
        //
        // Eventually strip the VOL headers from the first samples of each layer so that
        // there fp no redundancy w.r.t. the VOL headers in the MP4 file.  Currently the
        // VOL headers are remaining in the first frame data

        // uint8 layer = (uint8)((flags & 0x70) >> 4);


        if (PVA_FF_MP4_CODEC_TYPE_BASELINE_H263_VIDEO == codecType)
        {
            if (true == _firstFrameInLayer0)
                _firstFrameInLayer0 = false;
        }

        if (!_modifiable)
            return false;

        // The layer in the flags byte indicates which video track to add to
        // int32 trackNum = (int32)(flags & 0x70) >> 4;

        // Add to mdat PVA_FF_Atom for the specified track
        switch (codecType)
        {
            case PVA_FF_MP4_CODEC_TYPE_MPEG4_VIDEO:
            case PVA_FF_MP4_CODEC_TYPE_BASELINE_H263_VIDEO:
            case PVA_FF_MP4_CODEC_TYPE_AVC_VIDEO:
            {
                if (true == _oInterLeaveEnabled)
                {
                    return addMediaSampleInterleave(trackID, pSampleParam);
                }
                else
                {
                    if (!mdatAtom->addRawSample((pSampleParam->_fragmentList), (size), mediaType, codecType))
                        retVal = false;

                    _pmovieAtom->addSampleToTrack(trackID, pSampleParam);
                }
                break;
            }
            default:
                return false;
        }
    }

    return retVal;
}

// The following methods are used to set the user data
void
PVA_FF_Mpeg4File::setVersion(PVA_FF_UNICODE_STRING_PARAM version, uint16 langCode)
{
    OSCL_UNUSED_ARG(version);
    OSCL_UNUSED_ARG(langCode);
}

void
PVA_FF_Mpeg4File::setTitle(PVA_FF_UNICODE_STRING_PARAM title, uint16 langCode)
{
    if (_pmovieAtom != NULL)
    {
        _pmovieAtom->setTitleInfo(title, langCode);
    }
}

void
PVA_FF_Mpeg4File::setAuthor(PVA_FF_UNICODE_STRING_PARAM author, uint16 langCode)
{
    if (_pmovieAtom != NULL)
    {
        _pmovieAtom->setAuthorInfo(author, langCode);
    }
}

void
PVA_FF_Mpeg4File::setCopyright(PVA_FF_UNICODE_STRING_PARAM copyright, uint16 langCode)
{
    if (_pmovieAtom != NULL)
    {
        _pmovieAtom->setCopyRightInfo(copyright, langCode);
    }
}

void
PVA_FF_Mpeg4File::setDescription(PVA_FF_UNICODE_STRING_PARAM description, uint16 langCode)
{
    if (_pmovieAtom != NULL)
    {
        _pmovieAtom->setDescription(description, langCode);
    }
}

void
PVA_FF_Mpeg4File::setRating(PVA_FF_UNICODE_STRING_PARAM ratingInfo,
                            uint16 langCode,
                            uint32 ratingEntity,
                            uint32 ratingCriteria)
{
    if (_pmovieAtom != NULL)
    {
        _pmovieAtom->setRatingInfo(ratingInfo, ratingEntity, ratingCriteria, langCode);
    }
}

void
PVA_FF_Mpeg4File::setPerformer(PVA_FF_UNICODE_STRING_PARAM performer, uint16 langCode)
{
    if (_pmovieAtom != NULL)
    {
        _pmovieAtom->setPerformerInfo(performer, langCode);
    }
}

void
PVA_FF_Mpeg4File::setGenre(PVA_FF_UNICODE_STRING_PARAM genre, uint16 langCode)
{
    if (_pmovieAtom != NULL)
    {
        _pmovieAtom->setGenreInfo(genre, langCode);
    }
}

void
PVA_FF_Mpeg4File::setClassification(PVA_FF_UNICODE_STRING_PARAM classificationInfo,
                                    uint32 classificationEntity, uint16 classificationTable,
                                    uint16 langCode)
{
    if (_pmovieAtom != NULL)
    {
        _pmovieAtom->setClassificationInfo(classificationInfo, classificationEntity, classificationTable, langCode);
    }
}

void
PVA_FF_Mpeg4File::setKeyWord(uint8 keyWordSize, PVA_FF_UNICODE_HEAP_STRING keyWordInfo, uint16 langCode)
{
    if (_pmovieAtom != NULL)
    {
        _pmovieAtom->setKeyWordsInfo(keyWordSize, keyWordInfo, langCode);
    }
}

void
PVA_FF_Mpeg4File::setLocationInfo(PvmfAssetInfo3GPPLocationStruct *ptr_loc_struct)
{
    if (_pmovieAtom != NULL)
    {
        _pmovieAtom->setLocationInfo(ptr_loc_struct);
    }
}

void
PVA_FF_Mpeg4File::setAlbumInfo(PVA_FF_UNICODE_STRING_PARAM albumInfo, uint16 langCode)
{
    if (_pmovieAtom != NULL)
    {
        _pmovieAtom->setAlbumInfo(albumInfo, langCode);
    }
}

void
PVA_FF_Mpeg4File::setAlbumTrackNumber(uint8 trackNumber)
{
    if (_pmovieAtom != NULL)
    {
        _pmovieAtom->setAlbumTrackNumber(trackNumber);
    }
}

void
PVA_FF_Mpeg4File::setRecordingYear(uint16 recordingYear)
{
    if (_pmovieAtom != NULL)
    {
        _pmovieAtom->setRecordingYearInfo(recordingYear);
    }
}

void
PVA_FF_Mpeg4File::setCreationDate(PVA_FF_UNICODE_STRING_PARAM creationDate)
{
    _creationDate = creationDate;
}

void
PVA_FF_Mpeg4File::setVideoParams(uint32 trackID,
                                 float frate,
                                 uint16 interval,
                                 uint32 frame_width,
                                 uint32 frame_height)
{
    OSCL_UNUSED_ARG(frate);
    OSCL_UNUSED_ARG(interval);
    PVA_FF_TrackAtom *trackAtom;
    trackAtom = _pmovieAtom->getMediaTrack(trackID);

    if (trackAtom != NULL)
        trackAtom->setVideoParams(frame_width, frame_height);

    return;
}

void
PVA_FF_Mpeg4File::setH263ProfileLevel(uint32 trackID,
                                      uint8  profile,
                                      uint8  level)
{
    PVA_FF_TrackAtom *trackAtom;
    trackAtom = _pmovieAtom->getMediaTrack(trackID);
    trackAtom->setH263ProfileLevel(profile, level);
    return;
}

// Methods to get and set the sample rate (i.e. timescales) for the streams and
// the overall Mpeg-4 presentation
void
PVA_FF_Mpeg4File::setPresentationTimescale(uint32 timescale)
{   // Set the overall timescale of the Mpeg-4 presentation
    _pmovieAtom->setTimeScale(timescale);
}

void
PVA_FF_Mpeg4File::addMediaDataAtom(PVA_FF_MediaDataAtom* atom)
{
    if (_modifiable)
    {
        _pmediaDataAtomVec->push_back(atom);
    }
}

//for timed text only
void
PVA_FF_Mpeg4File::setTextDecoderSpecificInfo(PVA_FF_TextSampleDescInfo *header, int32 trackID)
{
    PVA_FF_TextSampleDescInfo *pinfo = NULL;
    pinfo = header;
    _pmovieAtom->addTextDecoderSpecificInfo(pinfo, trackID);
    return;
}


void
PVA_FF_Mpeg4File::setDecoderSpecificInfo(uint8 * header, int32 size, int32 trackID)
{
    if (0 >= size)
        return;

    PVA_FF_DecoderSpecificInfo *pinfo = NULL;
    PV_MP4_FF_NEW(fp->auditCB, PVA_FF_DecoderSpecificInfo, (header, (uint32)size), pinfo);
    _pmovieAtom->addDecoderSpecificInfo(pinfo, trackID);
    PVA_FF_TrackAtom *track = _pmovieAtom->getMediaTrack(trackID);
    if (track->getMediaType() == MEDIA_TYPE_VISUAL)
    {
        if (track->getCodecType() == PVA_FF_MP4_CODEC_TYPE_AVC_VIDEO)
        {
            PV_MP4_FF_DELETE(NULL, PVA_FF_DecoderSpecificInfo, pinfo);
        }
    }
}

bool PVA_FF_Mpeg4File::setPSSHInfo(uint32 systemidlen, uint8* systemid, uint32 datalen, uint8* data)
{
    OsclMemoryFragment systemidfrag;
    systemidfrag.len = systemidlen;
    systemidfrag.ptr = systemid;

    if (! _pPSSHAtom->setSystemId(systemidfrag))
        return false;

    if (! _pPSSHAtom->setData(data, datalen))
        return false;

    // add the pssh atom to the movie atom
    _pmovieAtom->addPSSHAtom(_pPSSHAtom);

    return true;
}

void
PVA_FF_Mpeg4File::recomputeSize()
{
    uint32 i;
    uint32 size = getMovieAtom().getSize();

    for (i = 0; i < getMediaDataAtomVec().size(); i++)
    {
        size += getMediaDataAtomVec()[i]->getSize();
    }
    _size = size;
}

// Rendering the PVA_FF_Mpeg4File in proper format (bitlengths, etc.) to an ostream
bool
PVA_FF_Mpeg4File::renderToFileStream(MP4_AUTHOR_FF_FILE_IO_WRAP *fp)
{
    uint32 metaDataSize = 0;
    /*
     * Setting the major brand in ftyp atom
     */
    if (!_oFtypPopulated)
    {
        if (_o3GPPTrack)
        {
            if (_oMovieFragmentEnabled)
            {
                setMajorBrand(BRAND_3GPP6);
                setMajorBrandVersion(VERSION_3GPP6);
            }
            else if (_oTextTrack)
            {
                setMajorBrand(BRAND_3GPP5);
                setMajorBrandVersion(VERSION_3GPP5);
            }
            else
            {
                setMajorBrand(BRAND_3GPP4);
                setMajorBrandVersion(VERSION_3GPP4);
            }
        }
        else if (_oMPEGTrack)
        {
            setMajorBrand(BRAND_MP41);
            setMajorBrandVersion(VERSION_MP41);
        }
        /*
         * Add compatible brands
         */
        if (_o3GPPTrack)
        {
            addCompatibleBrand(BRAND_3GPP4);

            if (_oTextTrack)
                addCompatibleBrand(BRAND_3GPP5);
        }
        if (_oMPEGTrack)
        {
            addCompatibleBrand(BRAND_MP41);
            addCompatibleBrand(BRAND_MP42);
        }
        if (_oAVCTrack)
        {
            addCompatibleBrand(BRAND_AVC);
        }
        addCompatibleBrand(BRAND_ISOM);
        addCompatibleBrand(BRAND_3G2A);
        addCompatibleBrand(BRAND_3G2B);
        addCompatibleBrand(BRAND_3G2C);
    }

    if (_creationDate.get_size() > 0)
    {
        uint32 time = convertCreationTime(_creationDate);

        _pmovieAtom->getMutableMovieHeaderAtom().setCreationTime(time);
        _pmovieAtom->getMutableMovieHeaderAtom().setModificationTime(time);
    }

    if ((_o3GPPTrack == true) || (_oMPEGTrack == true))
    {
        _pFileTypeAtom->renderToFileStream(fp);

        metaDataSize += _pFileTypeAtom->getSize();
    }
    {
        populateUserDataAtom();
    }
    if (!_fileAuthoringFlags)
    {
        if (_oUserDataUpFront)
        {
            {
                if (!_puserDataAtom->renderToFileStream(fp))
                {
                    return false;
                }
                metaDataSize += _puserDataAtom->getSize();
            }
        }
    }
    if ((_totalTempFileRemoval))
    {
        PVA_FF_AtomUtils::seekFromStart(fp, _offsetDataRenderedToFile);
    }

    _oFileRenderCalled = true;

    uint32 chunkFileOffset  = 0;

    int32 i;
    uint32 size = _pmediaDataAtomVec->size();

    _pmovieAtom->prepareToRender();

    if (_oMovieAtomUpfront)
    {
        metaDataSize += _pmovieAtom->getSize();

        chunkFileOffset = DEFAULT_ATOM_SIZE + metaDataSize;

        // Update all chunk offsets
        for (i = size - 1; i >= 0; i--)
        {
            PVA_FF_MediaDataAtom *mdat = (*_pmediaDataAtomVec)[i];

            if (!_totalTempFileRemoval)
            {
                Oscl_Vector<PVA_FF_TrackAtom*, OsclMemAllocator> *trefVec =
                    (*_pmediaDataAtomVec)[i]->getTrackReferencePtrVec();

                if (trefVec != NULL)
                {
                    for (uint32 trefVecIndex = 0;
                            trefVecIndex < trefVec->size();
                            trefVecIndex++)
                    {
                        (*trefVec)[trefVecIndex]->updateAtomFileOffsets(chunkFileOffset);
                    }
                }
                chunkFileOffset += mdat->getMediaDataSize();
            }
            else
            {
                // not supported - no direct render with media interleaving
                return false;
            }
        }

        // Render the movie atom to the file stream
        if (!_pmovieAtom->renderToFileStream(fp))
        {
            return false;
        }
    }


    // Render all mediaData atoms to the file stream
    for (i = size - 1; i >= 0; i--)
    {
        if (!_totalTempFileRemoval)
        {
            if (!((*_pmediaDataAtomVec)[i]->renderToFileStream(fp)))
            {
                _fileWriteFailed = true;
                return false;
            }
            if ((*_pmediaDataAtomVec)[i]->_targetFileWriteError == true)
            {
                _fileWriteFailed = true;
                return false;
            }

            if (!_oMovieAtomUpfront)
            {
                chunkFileOffset =
                    (*_pmediaDataAtomVec)[i]->getFileOffsetForChunkStart();
                if (chunkFileOffset != 0)
                {
                    // Only true when fp a PVA_FF_MediaDataAtom

                    Oscl_Vector<PVA_FF_TrackAtom*, OsclMemAllocator> *trefVec =
                        (*_pmediaDataAtomVec)[i]->getTrackReferencePtrVec();


                    if (trefVec != NULL)
                    {
                        for (uint32 trefVecIndex = 0;
                                trefVecIndex < trefVec->size();
                                trefVecIndex++)
                        {
                            (*trefVec)[trefVecIndex]->updateAtomFileOffsets(chunkFileOffset);
                        }
                    }
                }
            }
        }
        else
        {
            if (!_oMovieAtomUpfront)
            {
                chunkFileOffset =
                    (*_pmediaDataAtomVec)[i]->getFileOffsetForChunkStart();

                if (chunkFileOffset != 0)
                {
                    // Only true when fp a PVA_FF_MediaDataAtom

                    Oscl_Vector<PVA_FF_TrackAtom*, OsclMemAllocator> *trefVec = (*_pmediaDataAtomVec)[i]->getTrackReferencePtrVec();
                    if (trefVec != NULL)
                    {
                        for (uint32 trefVecIndex = 0;
                                trefVecIndex < trefVec->size();
                                trefVecIndex++)
                        {
                            (*trefVec)[trefVecIndex]->updateAtomFileOffsets(chunkFileOffset);
                        }
                    }
                }
            }
        }
    }
    if (!_fileAuthoringFlags)
    {
        if (!_oUserDataUpFront)
        {
            {
                if (!_puserDataAtom->renderToFileStream(fp))
                {
                    return false;
                }
            }
        }
    }
    //Important: This needs to be done AFTER the user data has been rendered to file
    if (!_oMovieAtomUpfront)
    {
        // Render the movie atom to the file stream
        if (!_pmovieAtom->renderToFileStream(fp))
        {
            return false;
        }
    }

    _tempFileIndex = 'a';

    return true;
}

// Rendering the MP4 file to disk
bool
PVA_FF_Mpeg4File::renderToFile(PVA_FF_UNICODE_STRING_PARAM filename)
{
    MP4_AUTHOR_FF_FILE_IO_WRAP fp;
    fp._filePtr = NULL;
    fp._osclFileServerSession = NULL;
    bool status = true;

    if (!(_oMovieFragmentEnabled && _oComposeMoofAtom))
    {

        _modifiable = false; // Only allow addition of samples BEFORE rendering to disk
        // After render to disk - cannot add more data


        //make sure to flush the interleave buffers, be it to temp files
        //or to target files
        uint32 k = 0;

        for (k = 0; status && k < _pmediaDataAtomVec->size(); k++)
        {
            Oscl_Vector<PVA_FF_TrackAtom*, OsclMemAllocator> *trefVec =
                (*_pmediaDataAtomVec)[k]->getTrackReferencePtrVec();

            if (trefVec != NULL)
            {
                for (uint32 trefVecIndex = 0;
                        status && trefVecIndex < trefVec->size();
                        trefVecIndex++)
                {
                    PVA_FF_TrackAtom* pTrack = (*trefVec)[trefVecIndex];
                    uint32 trackID = pTrack->getTrackID();

                    if (_oInterLeaveEnabled)
                    {
                        if (!flushInterLeaveBuffer(trackID))
                        {
                            status = false;
                        }
                    }

                }
            }
        }

        bool targetRender = false;
        _offsetDataRenderedToFile = 0;

        if (_totalTempFileRemoval)
        {
            for (uint32 k = 0; k < _pmediaDataAtomVec->size(); k++)
            {
                if (targetRender)
                {
                    //Only one track is allowed to be rendered directly onto the target
                    //file
                    status = false;
                }
                else
                {
                    targetRender = true;

                    if (!((*_pmediaDataAtomVec)[k]->closeTargetFile()))
                    {
                        status = false;
                    }

                    fp._filePtr = ((*_pmediaDataAtomVec)[k]->getTargetFilePtr());
                    fp._osclFileServerSession = OSCL_STATIC_CAST(Oscl_FileServer*, _aFs);
                    _offsetDataRenderedToFile =
                        ((*_pmediaDataAtomVec)[k]->getTotalDataRenderedToTargetFile());
                }
            }
        }
        else
        {
            fp._osclFileServerSession = OSCL_STATIC_CAST(Oscl_FileServer*, _aFs);
            PVA_FF_AtomUtils::openFile(&fp, filename, Oscl_File::MODE_READWRITE | Oscl_File::MODE_BINARY);
            _oIsFileOpen = true;
        }

        if (fp._filePtr == NULL)
        {
            status = false;
        }

        if (!renderToFileStream(&fp))
        {
            status = false;
        }

        // Render mfra for _oLiveMovieFragmentEnabled only if it
        // has entries available.
        if (_oLiveMovieFragmentEnabled && _pMfraAtom->hasSampleEntries())
            _pMfraAtom->renderToFileStream(&fp);

        fp._filePtr->Flush();

        if (_oIsFileOpen)
        {
            PVA_FF_AtomUtils::closeFile(&fp);
            _oIsFileOpen = false;
        }

        if (_fileWriteFailed)
        {
            status = false;
        }
    }
    else
    {
        // flush interleave buffers into last TRUN
        for (uint32 k = 0; k < _pmediaDataAtomVec->size(); k++)
        {
            if (_totalTempFileRemoval)
            {
                Oscl_Vector<PVA_FF_TrackAtom*, OsclMemAllocator> *trefVec =
                    (*_pmediaDataAtomVec)[k]->getTrackReferencePtrVec();

                if (trefVec != NULL)
                {
                    for (uint32 trefVecIndex = 0; status && trefVecIndex < trefVec->size(); trefVecIndex++)
                    {
                        PVA_FF_TrackAtom* pTrack = (*trefVec)[trefVecIndex];
                        uint32 trackID = pTrack->getTrackID();

                        if (_oInterLeaveEnabled)
                        {
                            if (!flushInterLeaveBuffer(trackID))
                            {
                                status = false;
                            }
                        }
                    }
                }
            }
        }

        fp._filePtr = _targetFileHandle;
        fp._osclFileServerSession = OSCL_STATIC_CAST(Oscl_FileServer*, _aFs);

        // write movie fragment duration in movie extends atom
        _pmovieAtom->writeMovieFragmentDuration(&fp);

        if (!renderMovieFragments())
        {
            status = false;
        }

        fp._filePtr = _targetFileHandle;
        fp._osclFileServerSession = OSCL_STATIC_CAST(Oscl_FileServer*, _aFs);
        _pMfraAtom->renderToFileStream(&fp);
        _pmovieAtom->writeMaxSampleSize(&fp);
        if (_oIsFileOpen)
        {
            PVA_FF_AtomUtils::closeFile(&fp);
            _oIsFileOpen = false;
        }
    }

    return status;
}


// Access function to set the postfix string for PVA_FF_MediaDataAtom objects
// Set the post fix string for the temporary file in order to support multiple instances,
// the goal fp to create temporary files with different names
void
PVA_FF_Mpeg4File::SetTempFilePostFix(PVA_FF_UNICODE_STRING_PARAM postFix)
{
    _tempFilePostfix = (_STRLIT(""));
    _tempFilePostfix += postFix;
}

// Access function to set the output path string for PVA_FF_MediaDataAtom objects
// Set the output path string for the temporary files in order to generate them at the same location
// as the final mp4 file.
void
PVA_FF_Mpeg4File::SetTempOutputPath(PVA_FF_UNICODE_STRING_PARAM outputPath)
{
    _tempOutputPath = (_STRLIT(""));
    _tempOutputPath += outputPath;
}

PVA_FF_MediaDataAtom*
PVA_FF_Mpeg4File::getMediaDataAtomForTrack(uint32 trackID)
{
    if (_oInterLeaveEnabled)
    {
        if (_pmediaDataAtomVec != NULL)
        {
            if (_pmediaDataAtomVec->size() > MIN_NUM_MEDIA_TRACKS)
            {
                int32 index = MIN_NUM_MEDIA_TRACKS;
                return (*_pmediaDataAtomVec)[index];
            }
        }
    }
    else
    {
        for (uint32 k = 0; k < _pmediaDataAtomVec->size(); k++)
        {
            PVA_FF_TrackAtom *pTrack  = (PVA_FF_TrackAtom *)((*_pmediaDataAtomVec)[k]->getTrackReferencePtr());
            uint32 tID = pTrack->getTrackID();

            if (tID == trackID)
            {
                return (*_pmediaDataAtomVec)[k];
            }
        }
    }
    return (NULL);
}

bool
PVA_FF_Mpeg4File::addMultipleAccessUnitsToTrack(uint32 trackID, GAU *pgau)
{


    PVA_FF_TrackAtom *mediaTrack;
    uint32 mediaType;
    bool retVal = true;

    mediaTrack = _pmovieAtom->getMediaTrack(trackID);

    if (mediaTrack == NULL)
    {
        return false;
    }

    mediaType  = mediaTrack->getMediaType();

    PVA_FF_MediaDataAtom *mdatAtom = getMediaDataAtomForTrack(trackID);

    if (mdatAtom == NULL)
    {
        return false;
    }

    if (mediaType == MEDIA_TYPE_AUDIO)
    {
        if (_modifiable)
        {
            if ((mediaTrack->getCodecType() == PVA_FF_MP4_CODEC_TYPE_AMR_AUDIO) ||
                    (mediaTrack->getCodecType() == PVA_FF_MP4_CODEC_TYPE_AMR_WB_AUDIO))
            {
                int32 index = 0;

                uint8  *frag_ptr = (uint8 *)pgau->buf.fragments[index].ptr;
                int32 frag_len  = pgau->buf.fragments[index].len;

                for (uint32 k = 0; k < pgau->numMediaSamples; k++)
                {
                    uint8 frame_type = (uint8)pgau->info[k].sample_info;

                    frame_type = (uint8)(frame_type << 3);
                    frame_type |= 0x04;

                    // Add to mdat PVA_FF_Atom for the specified track
                    if (!mdatAtom->addRawSample(&frame_type, 1))
                    {
                        retVal = false;
                    }

                    int32 frame_size = pgau->info[k].len;

                    while (frame_size)
                    {
                        if (frag_len >= frame_size)
                        {
                            // Add to mdat PVA_FF_Atom for the specified track
                            if (!mdatAtom->addRawSample(frag_ptr,
                                                        frame_size))
                            {
                                retVal = false;
                            }

                            frag_ptr += frame_size;
                            frag_len -= frame_size;
                            frame_size = 0;
                        }
                        else
                        {
                            // Add to mdat PVA_FF_Atom for the specified track
                            if (!mdatAtom->addRawSample(frag_ptr,
                                                        frag_len))
                            {
                                retVal = false;
                            }

                            frame_size -= frag_len;

                            index++;

                            if (index == pgau->buf.num_fragments)
                            {
                                return false;
                            }

                            frag_ptr = (uint8 *)pgau->buf.fragments[index].ptr;
                            frag_len = pgau->buf.fragments[index].len;
                        }
                    }
                    // Add to moov atom (in turn adds to tracks)

                    PVMP4FFComposerSampleParam sampleParam;
                    sampleParam._sampleSize = pgau->info[k].len + 1;
                    sampleParam._timeStamp = pgau->info[k].ts;
                    sampleParam._flags = (uint8)pgau->info[k].sample_info;
                    _pmovieAtom->addSampleToTrack(trackID, &sampleParam);
                }
            }
            else
            {
                for (int32 k = 0; k < pgau->buf.num_fragments; k++)
                {
                    // Add to mdat PVA_FF_Atom for the specified track
                    if (!mdatAtom->addRawSample(pgau->buf.fragments[k].ptr,
                                                pgau->buf.fragments[k].len))
                    {
                        retVal = false;
                    }
                }

                for (uint32 j = 0; j < pgau->numMediaSamples; j++)
                {
                    // Add to moov atom (in turn adds to tracks)
                    PVMP4FFComposerSampleParam sampleParam;
                    sampleParam._sampleSize = pgau->info[j].len + 1;
                    sampleParam._timeStamp = pgau->info[j].ts;
                    sampleParam._flags = (uint8)pgau->info[j].sample_info;
                    _pmovieAtom->addSampleToTrack(trackID, &sampleParam);
                }
            }
        }
    }
    else if (mediaType == MEDIA_TYPE_VISUAL)
    {
        if (_modifiable)
        {
            for (int32 k = 0; k < pgau->buf.num_fragments; k++)
            {
                // Add to mdat PVA_FF_Atom for the specified track
                if (!mdatAtom->addRawSample(pgau->buf.fragments[k].ptr,
                                            pgau->buf.fragments[k].len))
                {
                    retVal = false;
                }
            }

            for (uint32 j = 0; j < pgau->numMediaSamples; j++)
            {
                // Add to moov atom (in turn adds to tracks)
                PVMP4FFComposerSampleParam sampleParam;
                sampleParam._sampleSize = pgau->info[j].len + 1;
                sampleParam._timeStamp = pgau->info[j].ts;
                sampleParam._flags = (uint8)pgau->info[j].sample_info;
                _pmovieAtom->addSampleToTrack(trackID, &sampleParam);
            }
        }
    }
    else
    {
        return false;
    }

    return (retVal);
}

bool
PVA_FF_Mpeg4File::renderTruncatedFile(PVA_FF_UNICODE_STRING_PARAM filename)
{
    MP4_AUTHOR_FF_FILE_IO_WRAP fp;

    fp._osclFileServerSession = OSCL_STATIC_CAST(Oscl_FileServer*, _aFs);

    PVA_FF_AtomUtils::openFile(&fp, filename, Oscl_File::MODE_READWRITE | Oscl_File::MODE_BINARY);

    if (fp._filePtr == NULL)
    {
        return false;
    }
    /*
     * Setting the major brand in ftyp atom
     */

    if (_o3GPPTrack)
    {
        if (_oMovieFragmentEnabled)
        {
            setMajorBrand(BRAND_3GPP6);
            setMajorBrandVersion(VERSION_3GPP6);
        }
        else if (_oTextTrack)
        {
            setMajorBrand(BRAND_3GPP5);
            setMajorBrandVersion(VERSION_3GPP5);
        }
        else
        {
            setMajorBrand(BRAND_3GPP4);
            setMajorBrandVersion(VERSION_3GPP4);
        }
    }
    else if (_oMPEGTrack)
    {
        setMajorBrand(BRAND_MP41);
        setMajorBrandVersion(VERSION_MP41);
    }
    /*
     * Add compatible brands
     */
    if (_o3GPPTrack)
    {
        addCompatibleBrand(BRAND_3GPP4);

        if (_oTextTrack)
            addCompatibleBrand(BRAND_3GPP5);
    }
    if (_oMPEGTrack)
    {
        addCompatibleBrand(BRAND_MP41);
        addCompatibleBrand(BRAND_MP42);
    }
    if (_oAVCTrack)
    {
        addCompatibleBrand(BRAND_AVC);
    }
    addCompatibleBrand(BRAND_ISOM);
    addCompatibleBrand(BRAND_3G2A);
    addCompatibleBrand(BRAND_3G2B);
    addCompatibleBrand(BRAND_3G2C);

    if ((_o3GPPTrack == true) || (_oMPEGTrack == true))
    {
        _pFileTypeAtom->renderToFileStream(&fp);
    }
    {
        populateUserDataAtom();
        _puserDataAtom->renderToFileStream(&fp);
    }
    _oFileRenderCalled = true;

    PVA_FF_AtomUtils::closeFile(&fp);

    return true;
}

uint32
PVA_FF_Mpeg4File::convertCreationTime(PVA_FF_UNICODE_STRING_PARAM creationDate)
{
    uint32 numSecs = 0;

    uint32 numDaysInMonth[12] =
        {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    uint32 numDaysInLeapFeb = 29;
    uint32 refYear = 1904;

    // (365*4 + 1) * 24 * 3600
    uint32 numSecsInABlkofFourYears = 126230400;

    OSCL_TCHAR *date_ptr = (OSCL_TCHAR *)(creationDate.get_cstr());

    uint32 index = 0;
    uint32 currYear  = 0;
    uint32 month = 0;
    uint32 day   = 0;
    uint32 hour  = 0;
    uint32 minutes = 0;
    uint32 seconds = 0;

    bool nextChar = (date_ptr[index] == 0) ? false : true;

    char *c = (char *)(oscl_malloc(5 * sizeof(char)));

    uint8 s = 0;
    oscl_memset(c, 0, 5);

    while (nextChar && (index < 4))
    {
        c[s++] = (char)(date_ptr[index]);

        index++;

        nextChar = (date_ptr[index] == 0) ? false : true;
    }

    PV_atoi(c, 'd', currYear);

    if (currYear < refYear)
    {
        oscl_free(c);
        return 0;
    }

    if (index != 4)
    {
        oscl_free(c);
        return 0;
    }

    s = 0;
    oscl_memset(c, 0, 5);

    while (nextChar && (index < 6))
    {
        c[s++] = (char)(date_ptr[index]);

        index++;

        nextChar = (date_ptr[index] == 0) ? false : true;
    }

    PV_atoi(c, 'd', month);

    if (index  != 6)
    {
        oscl_free(c);
        return 0;
    }

    s = 0;
    oscl_memset(c, 0, 5);

    while (nextChar && (index < 8))
    {
        c[s++] = (char)(date_ptr[index]);

        index++;

        nextChar = (date_ptr[index] == 0) ? false : true;
    }

    PV_atoi(c, 'd', day);

    if (index  != 8)
    {
        oscl_free(c);
        return 0;
    }

    char val = (char)(date_ptr[index]);

    if (val != 'T')
    {
        oscl_free(c);
        return 0;
    }
    else
    {
        index++;
    }

    s = 0;
    oscl_memset(c, 0, 5);

    while (nextChar && (index < 11))
    {
        c[s++] = (char)(date_ptr[index]);

        index++;

        nextChar = (date_ptr[index] == 0) ? false : true;
    }

    PV_atoi(c, 'd', hour);

    if (index  != 11)
    {
        oscl_free(c);
        return 0;
    }

    s = 0;
    oscl_memset(c, 0, 5);

    while (nextChar && (index < 13))
    {
        c[s++] = (char)(date_ptr[index]);

        index++;

        nextChar = (date_ptr[index] == 0) ? false : true;
    }

    PV_atoi(c, 'd', minutes);

    if (index  != 13)
    {
        oscl_free(c);
        return 0;
    }

    s = 0;
    oscl_memset(c, 0, 5);

    while (nextChar && (index < 15))
    {
        c[s++] = (char)(date_ptr[index]);

        index++;

        nextChar = (date_ptr[index] == 0) ? false : true;
    }

    PV_atoi(c, 'd', seconds);

    uint32 deltaYears = currYear - refYear;

    uint32 numBlks = (deltaYears / 4);

    uint32 numLeftOverYears = (deltaYears - (numBlks * 4));

    numSecs = (numBlks * numSecsInABlkofFourYears);

    uint32 numDays = 0;

    if (numLeftOverYears > 1)
    {
        // Acct for leap year
        numDays = ((numLeftOverYears * 365) + 1);

        for (uint32 i = 0; i < month; i++)
        {
            numDays += numDaysInMonth[i];
        }

        numDays += day;

        uint32 numHours = (numDays * 24);

        numHours += hour;

        uint32 numMins = (numHours * 60);

        numMins += minutes;

        numSecs += ((numMins * 60) + seconds);
    }
    else
    {
        for (uint32 i = 0; i < month; i++)
        {
            if (i != 1)
                numDays += numDaysInMonth[i];
            else
                numDays += numDaysInLeapFeb;
        }

        numDays += day;

        uint32 numHours = (numDays * 24);

        numHours += hour;

        uint32 numMins = (numHours * 60);

        numMins += minutes;

        numSecs += ((numMins * 60) + seconds);
    }

    oscl_free(c);

    return (numSecs);
}

bool
PVA_FF_Mpeg4File::checkInterLeaveDuration(uint32 trackID, uint32 ts)
{
    // get the interleave buffer for the track
    PVA_FF_InterLeaveBuffer *pInterLeaveBuffer = getInterLeaveBuffer(trackID);

    PVA_FF_TrackAtom* mediaTrack = _pmovieAtom->getMediaTrack(trackID);

    uint32 lastChunkEndTime = pInterLeaveBuffer->getLastChunkEndTime();

    uint32 timescale = mediaTrack->getMediaTimeScale();

    uint32 interLeaveDurationInTrackTimeScale =
        (uint32)((_interLeaveDuration) * (timescale / 1000.0f));

    if ((ts - lastChunkEndTime) >= interLeaveDurationInTrackTimeScale)
    {
        pInterLeaveBuffer->setLastChunkEndTime(ts);
        return true;
    }

    return false;
}

bool
PVA_FF_Mpeg4File::flushInterLeaveBuffer(uint32 trackID)
{
    uint32 mediaType;
    PVA_FF_MP4_CODEC_TYPE codecType;


    PVA_FF_TrackAtom* mediaTrack = _pmovieAtom->getMediaTrack(trackID);
    PVA_FF_InterLeaveBuffer *pInterLeaveBuffer = getInterLeaveBuffer(trackID);
    if ((NULL == mediaTrack) || (NULL == pInterLeaveBuffer))
    {
        // Returning true here might sound strange, however this is a valid case
        // where by tracks like odsm or sdsm might not have a valid mediaTrack
        // However if false is returned here, the function renderToFile will
        // break the for loops and return back.
        return true;
    }
    if (!(_oMovieFragmentEnabled && _oComposeMoofAtom))
    {
        PVA_FF_MediaDataAtom *mdatAtom = getMediaDataAtomForTrack(trackID);

        mediaType  = mediaTrack->getMediaType();
        codecType = _pmovieAtom->getCodecType(trackID);

        _oChunkStart = true;

        Oscl_Vector<uint32, OsclMemAllocator> *tsVec = pInterLeaveBuffer->getTimeStampVec();

        Oscl_Vector<uint32, OsclMemAllocator> *sizeVec = pInterLeaveBuffer->getSampleSizeVec();

        Oscl_Vector<uint8, OsclMemAllocator> *flagsVec = pInterLeaveBuffer->getFlagsVec();

        Oscl_Vector<uint32, OsclMemAllocator> *sampleDurationVec = pInterLeaveBuffer->getSampleDurationVec();

        Oscl_Vector<int32, OsclMemAllocator> *indexVec = NULL;

        if (mediaType == MEDIA_TYPE_TEXT && codecType == PVA_FF_MP4_CODEC_TYPE_TIMED_TEXT)
        {
            indexVec = pInterLeaveBuffer->getTextIndexVec();
        }

        int32 numBufferedSamples = tsVec->size();

        if (numBufferedSamples > 0)
        {
            PVMP4FFComposerSampleParam sampleParam;

            sampleParam._baseOffset = _baseOffset;
            for (int32 i = 0; i < numBufferedSamples; i++)
            {
                sampleParam._timeStamp = (*tsVec)[i];
                sampleParam._sampleSize = (*sizeVec)[i];
                sampleParam._flags = (*flagsVec)[i];

                if (mediaType == MEDIA_TYPE_TEXT && codecType == PVA_FF_MP4_CODEC_TYPE_TIMED_TEXT)
                {
                    sampleParam._index = (*indexVec)[i];
                    sampleParam._sampleDuration = (*sampleDurationVec)[i];
                    // Add to moov atom (in turn adds to tracks)
                    _pmovieAtom->addTextSampleToTrack(trackID,
                                                      &sampleParam,
                                                      _oChunkStart);
                }
                else
                {
                    // Add to moov atom (in turn adds to tracks)
                    _pmovieAtom->addSampleToTrack(trackID,
                                                  &sampleParam,
                                                  _oChunkStart);
                }


                _oChunkStart = false;
            }

            //Render chunk
            uint32 chunkSize = 0;
            uint8* ptr = pInterLeaveBuffer->resetInterLeaveBuffer(chunkSize);

            if (!mdatAtom->addRawSample(ptr, chunkSize))
            {
                return false;
            }
            _baseOffset += chunkSize;
        }
    }
    else
    {
        // add remaining samples as last TRUN in current track fragment
        PVA_FF_TrackFragmentAtom *pCurrentTrackFragment;
        pCurrentTrackFragment = _pCurrentMoofAtom->getTrackFragment(trackID);

        // Set trun end time to the last sample TS
        // in the interleave buffer

        _oTrunStart = true;

        Oscl_Vector<uint32, OsclMemAllocator> *tsVec = pInterLeaveBuffer->getTimeStampVec();

        Oscl_Vector<uint32, OsclMemAllocator> *sizeVec = pInterLeaveBuffer->getSampleSizeVec();

        Oscl_Vector<uint8, OsclMemAllocator> *flagsVec = pInterLeaveBuffer->getFlagsVec();

        int32 numBufferedSamples = tsVec->size();

        int32 ii = 0;

        for (ii = 0; ii < numBufferedSamples; ii++)
        {
            uint32 sampleTS   = (*tsVec)[ii];
            uint32 sampleSize = (*sizeVec)[ii];
            uint8 sampleFlag = (*flagsVec)[ii];
            uint32 mediaType  = mediaTrack->getMediaType();

            // Add to moof atom (in turn adds to tracks)
            _pCurrentMoofAtom->addSampleToFragment(trackID, sampleSize, sampleTS,
                                                   sampleFlag,
                                                   _baseOffset, // update data offset for each new trun
                                                   _oTrunStart); // determine to add new trun or not

            // update movie duration in MVEX atom
            _pmovieAtom->updateMovieFragmentDuration(trackID, sampleTS);

            if (mediaType == MEDIA_TYPE_VISUAL)
            {
                // make entry for every sync sample
                uint8 codingType = (uint8)((sampleFlag >> 2) & 0x03);
                if (codingType == CODING_TYPE_I)
                {
                    // add video key frame as random sample entry
                    _pMfraAtom->addSampleEntry(trackID, sampleTS, _currentMoofOffset,
                                               _pCurrentMoofAtom->getTrackFragmentNumber(trackID),
                                               pCurrentTrackFragment->getTrunNumber(),
                                               (ii + 1));
                }
            }
            else if (mediaType == MEDIA_TYPE_AUDIO && _oTrunStart == true)
            {
                // add first audio sample in each TRUN as random sample entry
                _pMfraAtom->addSampleEntry(trackID, sampleTS, _currentMoofOffset,
                                           _pCurrentMoofAtom->getTrackFragmentNumber(trackID),
                                           pCurrentTrackFragment->getTrunNumber(),
                                           (ii + 1));

            }

            _oTrunStart = false;
        }

        // update the last TS entry only if there is 1 sample in buffer

        if (numBufferedSamples == 1)
        {
            uint32 lastSampleTS = pInterLeaveBuffer->getLastSampleTS();
            uint32 ts = pInterLeaveBuffer->getLastChunkEndTime();
            pCurrentTrackFragment->updateLastTSEntry(ts + (ts - lastSampleTS));
            _pmovieAtom->updateMovieFragmentDuration(trackID, ts + (ts - lastSampleTS));
        }
        // make entry for last sample same as duration of second to last sample
        else
        {
            if (tsVec->size() > 1)
            {
                uint32 delta = (*tsVec)[ii -1] - (*tsVec)[ii -2];
                uint32 ts = (*tsVec)[ii -1];
                pCurrentTrackFragment->updateLastTSEntry(ts + delta);
                _pmovieAtom->updateMovieFragmentDuration(trackID, ts + delta);
            }
        }


        if (numBufferedSamples > 0)
        {
            //Render chunk
            uint32 trunSize = 0;
            uint8* ptr = pInterLeaveBuffer->resetInterLeaveBuffer(trunSize);

            if (!_pCurrentMediaDataAtom->addRawSample(ptr, trunSize))
            {
                return false;
            }
            _baseOffset += trunSize;
        }
    }

    return true;
}

bool
PVA_FF_Mpeg4File::getTargetFileSize(uint32 &metaDataSize, uint32 &mediaDataSize)
{
    metaDataSize  = 0;
    mediaDataSize = 0;

    for (uint32 k = 0; k < _pmediaDataAtomVec->size(); k++)
    {
        mediaDataSize += (*_pmediaDataAtomVec)[k]->getMediaDataSize();

        Oscl_Vector<PVA_FF_TrackAtom*, OsclMemAllocator> *trefVec =
            (*_pmediaDataAtomVec)[k]->getTrackReferencePtrVec();

        if (trefVec != NULL)
        {
            for (uint32 trefVecIndex = 0;
                    trefVecIndex < trefVec->size();
                    trefVecIndex++)
            {
                PVA_FF_TrackAtom* pTrack = (*trefVec)[trefVecIndex];

                /*
                 * Account for media data that is remaining in the interleave
                 * buffers
                 */
                if (_oInterLeaveEnabled)
                {
                    uint32 trackID = pTrack->getTrackID();
                    PVA_FF_InterLeaveBuffer *pInterLeaveBuffer = getInterLeaveBuffer(trackID);
                    if (pInterLeaveBuffer)
                    {
                        uint32 currInterLeaveBufferSize = pInterLeaveBuffer->getCurrentInterLeaveBufferSize();

                        mediaDataSize += currInterLeaveBufferSize;
                    }
                }
            }
        }
    }


    if (_pFileTypeAtom != NULL)
    {
        metaDataSize += _pFileTypeAtom->getSize();
    }

    if (_pmovieAtom != NULL)
    {
        metaDataSize += _pmovieAtom->getSize();
    }

    metaDataSize += 1024; //Gaurd Band

    return true;
}

bool
PVA_FF_Mpeg4File::prepareToEncode()
{
    if (_oInterLeaveEnabled)
    {
        uint32 numTracks = _pmovieAtom->getNumberOfTracks();
        if (numTracks == 1)
        {
            _oInterLeaveEnabled = false;
            if (NULL != _pInterLeaveBufferVec)
            {
                // delete all interleave buffers
                int32 size = _pInterLeaveBufferVec->size();
                for (int32 i = 0; i < size; i++)
                {
                    PV_MP4_FF_DELETE(NULL, PVA_FF_MediaDataAtom, (*_pInterLeaveBufferVec)[i]);
                }
                // Delete the vectors themselves
                PV_MP4_FF_TEMPLATED_DELETE(NULL, PVA_FF_InterLeaveBufferVecType, Oscl_Vector, _pInterLeaveBufferVec);
            }
        }
        else if (!_totalTempFileRemoval)
        {
            return true;
        }
    }

    /*
     * Setting the major brand in ftyp atom
     */
    if (_o3GPPTrack)
    {
        if (_oMovieFragmentEnabled)
        {
            setMajorBrand(BRAND_3GPP6);
            setMajorBrandVersion(VERSION_3GPP6);
        }
        else if (_oTextTrack)
        {
            setMajorBrand(BRAND_3GPP5);
            setMajorBrandVersion(VERSION_3GPP5);
        }
        else
        {
            setMajorBrand(BRAND_3GPP4);
            setMajorBrandVersion(VERSION_3GPP4);
        }
    }
    else if (_oMPEGTrack)
    {
        setMajorBrand(BRAND_MP41);
        setMajorBrandVersion(VERSION_MP41);
    }

    /*
     * Add compatible brands
     */
    if (_o3GPPTrack)
    {
        if (_oMovieFragmentEnabled)
            addCompatibleBrand(BRAND_3GPP6);
        else
            addCompatibleBrand(BRAND_3GPP4);
    }
    if (_oMPEGTrack)
    {
        addCompatibleBrand(BRAND_MP41);
        addCompatibleBrand(BRAND_MP42);
    }
    if (!_oMovieFragmentEnabled)
    {
        addCompatibleBrand(BRAND_3GPP6);
    }
    if (_oAVCTrack)
    {
        addCompatibleBrand(BRAND_AVC);
    }
    addCompatibleBrand(BRAND_ISOM);
    addCompatibleBrand(BRAND_3G2A);
    addCompatibleBrand(BRAND_3G2B);
    addCompatibleBrand(BRAND_3G2C);

    _initialUserDataSize += _pFileTypeAtom->getSize();

    _oFtypPopulated = true;

    bool targetRender = false;
    for (uint32 j = 0; j < _pmediaDataAtomVec->size(); ++j)
    {
        if (true == _totalTempFileRemoval)
        {
            // only one track allowed to be rendered directly onto the target file
            if (true == targetRender) // already rendered a track?
                return false;

            targetRender = true;
            ((*_pmediaDataAtomVec)[j]->prepareTargetFile(_initialUserDataSize));
        }
    }

    return true;
}

void
PVA_FF_Mpeg4File::populateUserDataAtom()
{
    _oUserDataPopulated = true;
}

bool
PVA_FF_Mpeg4File::addMediaSampleInterleave(uint32 trackID, PVMP4FFComposerSampleParam *pSampleParam)
{
    if (pSampleParam == NULL)
        return false;

    PVA_FF_TrackAtom *mediaTrack = _pmovieAtom->getMediaTrack(trackID);
    PVA_FF_MediaDataAtom *mdatAtom = getMediaDataAtomForTrack(trackID);
    PVA_FF_InterLeaveBuffer *pInterLeaveBuffer = getInterLeaveBuffer(trackID);
    PVA_FF_MP4_CODEC_TYPE codecType = _pmovieAtom->getCodecType(trackID);
    uint32 mediaType = mediaTrack->getMediaType();

    pSampleParam->_index = 0;
    pSampleParam->_baseOffset = _baseOffset;

    if (true == _oComposeMoofAtom)
        _pmovieAtom->SetMaxSampleSize(trackID, pSampleParam->_sampleSize);
    if (_oFirstSampleEditMode)
    {
        _oChunkStart = true;
        /*
         * In this mode very first sample in each track is authored
         * in a separate chunk. It is easier to go back and edit the
         * sample meta data if we authored it in a seperate chunk by
         * itself.
         */
        if (mediaTrack->IsFirstSample())
        {
            // Add to moov atom (in turn adds to tracks)
            _pmovieAtom->addSampleToTrack(trackID,
                                          pSampleParam,
                                          _oChunkStart);
            _oChunkStart = false;

            if (!mdatAtom->addRawSample(pSampleParam->_fragmentList, pSampleParam->_sampleSize, mediaType, codecType))
            {
                return false;
            }
            _baseOffset += pSampleParam->_sampleSize;
            return true;
        }
    }

    /* Movie Fragment : check if fragment duration reached for MOOV atom. If yes, allocate new
    movie fragment (MOOF) and write data in new media data atom.
    */
    if (_oMovieFragmentEnabled == true && _oComposeMoofAtom == false)
    {
        uint32 duration = _pmovieAtom->getDuration();
        uint32 duration_msec = (uint)((((float)duration / _pmovieAtom->getTimeScale()) * 1000.0f));

        if (duration_msec >= _movieFragmentDuration)
        {
            // render MOOV and MDAT atoms
            renderMoovAtom();

            _oComposeMoofAtom = true;

            // allocate Moof movie fragments
            PVA_FF_MovieFragmentAtom *pMoofAtom;
            _sequenceNumber++;
            PV_MP4_FF_NEW(fp->auditCB, PVA_FF_MovieFragmentAtom, (_sequenceNumber,
                          _movieFragmentDuration,
                          _interLeaveDuration),
                          pMoofAtom);

            _pCurrentMoofAtom = pMoofAtom;

            // set Movie fragment duration
            _pmovieAtom->setMovieFragmentDuration(_movieFragmentDuration);

            // add track fragments
            for (uint32 kk = 0; kk < _pmediaDataAtomVec->size(); kk++)
            {
                // add track fragments from MDAT with interleaved data
                if (_totalTempFileRemoval)
                {
                    Oscl_Vector<PVA_FF_TrackAtom*, OsclMemAllocator> *trefVec =
                        (*_pmediaDataAtomVec)[kk]->getTrackReferencePtrVec();

                    if (trefVec != NULL)
                    {
                        for (uint32 trefVecIndex = 0; trefVecIndex < trefVec->size(); trefVecIndex++)
                        {
                            PVA_FF_TrackAtom* pTrack = (*trefVec)[trefVecIndex];

                            _pCurrentMoofAtom->addTrackFragment(pTrack->getMediaType(),
                                                                pTrack->getCodecType(),
                                                                pTrack->getTrackID(),
                                                                pTrack->getMediaTimeScale());

                            // add random access atom for each track
                            // make sure it hasn't been added yet.
                            if (_pMfraAtom->getTfraAtom(pTrack->getTrackID()) == NULL)
                                _pMfraAtom->addTrackFragmentRandomAccessAtom(pTrack->getTrackID());
                        }
                    }
                }
            }

            // form new MDAT atom
            PVA_FF_MediaDataAtom *pMdatAtom = NULL;
            PVA_FF_UNICODE_STRING_PARAM emptyString = PVA_FF_UNICODE_HEAP_STRING(_STRLIT_WCHAR(""));
            PV_MP4_FF_NEW(fp->auditCB, PVA_FF_MediaDataAtom, (emptyString, _targetFileHandle, _aFs, iCacheSize), pMdatAtom);

            _pCurrentMediaDataAtom = pMdatAtom;

            // current moof offset set at start of mdat, (later updated by size of mdat atom)
            _currentMoofOffset = _baseOffset;

            // base offset set to start of mdat (after fourcc code) as base data offset
            _baseOffset += _pCurrentMediaDataAtom->prepareTargetFileForFragments(_offsetDataRenderedToFile);
        }
    }

    if (false == _oMovieFragmentEnabled || false == _oComposeMoofAtom)
    {
        // interleave buffer have enough space?
        if (true == pInterLeaveBuffer->checkInterLeaveBufferSpace(pSampleParam->_sampleSize))
        {
            // exceeded the interleave duration?
            if (true != checkInterLeaveDuration(trackID, pSampleParam->_timeStamp))
            {
                _oChunkStart = false;
            }
            else
            {
                _oChunkStart = true;

                Oscl_Vector<uint32, OsclMemAllocator>* tsVec   = pInterLeaveBuffer->getTimeStampVec();
                Oscl_Vector<uint32, OsclMemAllocator>* sizeVec = pInterLeaveBuffer->getSampleSizeVec();
                Oscl_Vector<uint8, OsclMemAllocator>* flagsVec = pInterLeaveBuffer->getFlagsVec();

                int32 numBufferedSamples = tsVec->size();

                pSampleParam->_baseOffset = _baseOffset;
                for (int32 ii = 0; ii < numBufferedSamples; ++ii)
                {
                    PVMP4FFComposerSampleParam  sampleParam;
                    sampleParam._timeStamp  = (*tsVec)[ii];
                    sampleParam._sampleSize = (*sizeVec)[ii];
                    sampleParam._flags      = (*flagsVec)[ii];
                    sampleParam._baseOffset = _baseOffset;

                    // Add to moov atom (in turn adds to tracks)
                    _pmovieAtom->addSampleToTrack(trackID, &sampleParam, _oChunkStart);

                    _oChunkStart = false;
                }

                if (0 < numBufferedSamples)
                {
                    // render chunk
                    uint32 chunkSize = 0;
                    uint8* ptr = pInterLeaveBuffer->resetInterLeaveBuffer(chunkSize);

                    if (false == mdatAtom->addRawSample(ptr, chunkSize))
                        return false;

                    _baseOffset += chunkSize;
                    pSampleParam->_baseOffset = _baseOffset;
                }
            }

            pSampleParam->_baseOffset = _baseOffset;
            return pInterLeaveBuffer->addSampleToInterLeaveBuffer(pSampleParam);
        }

        // set chunk end time to the last sample TS in the interleave buffer
        pInterLeaveBuffer->setLastChunkEndTime(pSampleParam->_timeStamp);

        _oChunkStart = true;

        Oscl_Vector<uint32, OsclMemAllocator>* tsVec   = pInterLeaveBuffer->getTimeStampVec();
        Oscl_Vector<uint32, OsclMemAllocator>* sizeVec = pInterLeaveBuffer->getSampleSizeVec();
        Oscl_Vector<uint8, OsclMemAllocator>* flagsVec = pInterLeaveBuffer->getFlagsVec();

        int32 numBufferedSamples = tsVec->size();

        pSampleParam->_baseOffset = _baseOffset;
        for (int32 ii = 0; ii < numBufferedSamples; ++ii)
        {
            PVMP4FFComposerSampleParam  sampleParam;
            sampleParam._timeStamp  = (*tsVec)[ii];
            sampleParam._sampleSize = (*sizeVec)[ii];
            sampleParam._flags      = (*flagsVec)[ii];
            sampleParam._baseOffset = _baseOffset;

            // add to moov atom (in turn adds to tracks)
            _pmovieAtom->addSampleToTrack(trackID, &sampleParam, _oChunkStart);

            _oChunkStart = false;
        }

        if (0 < numBufferedSamples)
        {
            //Render chunk
            uint32 chunkSize = 0;
            uint8* ptr = pInterLeaveBuffer->resetInterLeaveBuffer(chunkSize);

            if (!mdatAtom->addRawSample(ptr, chunkSize))
                return false;

            _baseOffset += chunkSize;
            pSampleParam->_baseOffset = _baseOffset;
        }

        return pInterLeaveBuffer->addSampleToInterLeaveBuffer(pSampleParam);
    }

    // add data in movie fragment
    uint32 trackFragmentDuration = _pCurrentMoofAtom->getTrackFragmentDuration(trackID);

    // check if Movie Fragment duration is reached for current fragment
    if (trackFragmentDuration < _movieFragmentDuration)
    {
        // add fragment to the current track fragment

        //check for interleaving in current track fragment
        PVA_FF_TrackFragmentAtom *pCurrentTrackFragment;
        pCurrentTrackFragment = _pCurrentMoofAtom->getTrackFragment(trackID);

        if (!pInterLeaveBuffer->checkInterLeaveBufferSpace(pSampleParam->_sampleSize))
        {
            // Set trun end time to the last sample TS
            // in the interleave buffer
            pInterLeaveBuffer->setLastChunkEndTime(pSampleParam->_timeStamp);

            _oTrunStart = true;

            Oscl_Vector<uint32, OsclMemAllocator>* tsVec   = pInterLeaveBuffer->getTimeStampVec();
            Oscl_Vector<uint32, OsclMemAllocator>* sizeVec = pInterLeaveBuffer->getSampleSizeVec();
            Oscl_Vector<uint8, OsclMemAllocator>* flagsVec = pInterLeaveBuffer->getFlagsVec();

            int32 numBufferedSamples = tsVec->size();

            for (int32 ii = 0; ii < numBufferedSamples; ii++)
            {
                uint32 sampleTS   = (*tsVec)[ii];
                uint32 sampleSize = (*sizeVec)[ii];
                uint8  sampleFlag = (*flagsVec)[ii];

                // Add to moof atom (in turn adds to tracks)
                _pCurrentMoofAtom->addSampleToFragment(trackID, sampleSize, sampleTS,
                                                       sampleFlag,
                                                       _baseOffset, // update data offset for each new trun
                                                       _oTrunStart); // determine to add new trun or not

                // update movie duration in MVEX atom
                _pmovieAtom->updateMovieFragmentDuration(trackID, sampleTS);

                if (MEDIA_TYPE_VISUAL == mediaType)
                {
                    // make entry for every sync sample
                    uint8 codingType = (uint8)((sampleFlag >> 2) & 0x03);
                    if (CODING_TYPE_I == codingType)
                    {
                        // add video key frame as random sample entry
                        _pMfraAtom->addSampleEntry(trackID, sampleTS, _currentMoofOffset,
                                                   _pCurrentMoofAtom->getTrackFragmentNumber(trackID),
                                                   pCurrentTrackFragment->getTrunNumber(),
                                                   (ii + 1));
                    }
                }
                else if (MEDIA_TYPE_AUDIO == mediaType && true == _oTrunStart)
                {
                    // add first audio sample in each TRUN as random sample entry
                    _pMfraAtom->addSampleEntry(trackID, sampleTS, _currentMoofOffset,
                                               _pCurrentMoofAtom->getTrackFragmentNumber(trackID),
                                               pCurrentTrackFragment->getTrunNumber(),
                                               (ii + 1));
                }

                _oTrunStart = false;
            }

            pCurrentTrackFragment->updateLastTSEntry(pSampleParam->_timeStamp);

            if (0 < numBufferedSamples)
            {
                //Render chunk
                uint32 trunSize = 0;
                uint8* ptr = pInterLeaveBuffer->resetInterLeaveBuffer(trunSize);

                if (!_pCurrentMediaDataAtom->addRawSample(ptr, trunSize))
                    return false;

                _baseOffset += trunSize;
            }

            pSampleParam->_baseOffset = _baseOffset;
            if (!(pInterLeaveBuffer->addSampleToInterLeaveBuffer(pSampleParam)))
                return false;
        }
        else
        {
            if (checkInterLeaveDuration(trackID, pSampleParam->_timeStamp))
            {
                _oTrunStart = true;

                Oscl_Vector<uint32, OsclMemAllocator>* tsVec   = pInterLeaveBuffer->getTimeStampVec();
                Oscl_Vector<uint32, OsclMemAllocator>* sizeVec = pInterLeaveBuffer->getSampleSizeVec();
                Oscl_Vector<uint8, OsclMemAllocator>* flagsVec = pInterLeaveBuffer->getFlagsVec();

                int32 numBufferedSamples = tsVec->size();


                for (int32 ii = 0; ii < numBufferedSamples; ii++)
                {
                    uint32 sampleTS   = (*tsVec)[ii];
                    uint32 sampleSize = (*sizeVec)[ii];
                    uint8  sampleFlag = (*flagsVec)[ii];

                    // update movie duration in MVEX atom
                    _pmovieAtom->updateMovieFragmentDuration(trackID, sampleTS);

                    // Add to moov atom (in turn adds to tracks)
                    _pCurrentMoofAtom->addSampleToFragment(trackID, sampleSize, sampleTS,
                                                           sampleFlag,
                                                           _baseOffset,
                                                           _oTrunStart);

                    if (MEDIA_TYPE_VISUAL == mediaType)
                    {
                        // make entry for every sync sample
                        uint8 codingType = (uint8)((sampleFlag >> 2) & 0x03);
                        if (codingType == CODING_TYPE_I)
                        {
                            // add video key frame as random sample entry
                            _pMfraAtom->addSampleEntry(trackID, sampleTS, _currentMoofOffset,
                                                       _pCurrentMoofAtom->getTrackFragmentNumber(trackID),
                                                       pCurrentTrackFragment->getTrunNumber(),
                                                       (ii + 1));
                        }
                    }
                    else if (MEDIA_TYPE_AUDIO == mediaType  && true == _oTrunStart)
                    {
                        // add first audio sample in each TRUN as random sample entry
                        _pMfraAtom->addSampleEntry(trackID, sampleTS, _currentMoofOffset,
                                                   _pCurrentMoofAtom->getTrackFragmentNumber(trackID),
                                                   pCurrentTrackFragment->getTrunNumber(),
                                                   (ii + 1));
                    }

                    _oTrunStart = false;
                }

                pCurrentTrackFragment->updateLastTSEntry(pSampleParam->_timeStamp);

                if (numBufferedSamples > 0)
                {
                    //Render chunk
                    uint32 trunSize = 0;
                    uint8* ptr = pInterLeaveBuffer->resetInterLeaveBuffer(trunSize);

                    if (!_pCurrentMediaDataAtom->addRawSample(ptr, trunSize))
                        return false;

                    _baseOffset += trunSize;
                }
            }
            else
            {
                _oTrunStart = false;
            }

            pSampleParam->_baseOffset = _baseOffset;
            if (!(pInterLeaveBuffer->addSampleToInterLeaveBuffer(pSampleParam)))
                return false;
        }
    }
    else
    {
        pSampleParam->_baseOffset = _baseOffset; // add sample

        if (!(pInterLeaveBuffer->addSampleToInterLeaveBuffer(pSampleParam)))
            return false;

        uint32 kk = 0;

        // update last sample TS entry
        for (kk = 0; kk < _pmediaDataAtomVec->size(); kk++)
        {
            if (false == _totalTempFileRemoval)
                continue;

            Oscl_Vector<PVA_FF_TrackAtom*, OsclMemAllocator> *trefVec =
                (*_pmediaDataAtomVec)[kk]->getTrackReferencePtrVec();

            if (0 == trefVec)
                continue;

            for (uint32 trefVecIndex = 0; trefVecIndex < trefVec->size(); trefVecIndex++)
            {
                PVA_FF_TrackAtom* pTrack = (*trefVec)[trefVecIndex];
                uint32 trackID = pTrack->getTrackID();

                PVA_FF_TrackFragmentAtom *pCurrentTrackFragment;
                pCurrentTrackFragment = _pCurrentMoofAtom->getTrackFragment(trackID);

                PVA_FF_InterLeaveBuffer *pInterLeaveBuffer = getInterLeaveBuffer(trackID);
                uint32 ts = pInterLeaveBuffer->getFirstTSEntry();
                pCurrentTrackFragment->updateLastTSEntry(ts);
            }
        }

        // close MDAT atom and render current fragment
        if (!renderMovieFragments())
        {
            _fileWriteFailed = true;
            return false;
        }

        // delete current moof atom
        PV_MP4_FF_DELETE(NULL, PVA_FF_MovieFragmentAtom, _pCurrentMoofAtom);

        // allocate new fragment
        PVA_FF_MovieFragmentAtom *pMoofAtom;
        _sequenceNumber++;
        PV_MP4_FF_NEW(fp->auditCB, PVA_FF_MovieFragmentAtom, (_sequenceNumber,
                      _movieFragmentDuration,
                      _interLeaveDuration),
                      pMoofAtom);
        _pCurrentMoofAtom = pMoofAtom;

        // add track fragments
        for (kk = 0; kk < _pmediaDataAtomVec->size(); kk++)
        {
            if (false == _totalTempFileRemoval)
                continue;

            Oscl_Vector<PVA_FF_TrackAtom*, OsclMemAllocator> *trefVec =
                (*_pmediaDataAtomVec)[kk]->getTrackReferencePtrVec();

            if (0 == trefVec)
                continue;

            for (uint32 trefVecIndex = 0; trefVecIndex < trefVec->size(); trefVecIndex++)
            {
                PVA_FF_TrackAtom* pTrack = (*trefVec)[trefVecIndex];

                _pCurrentMoofAtom->addTrackFragment(pTrack->getMediaType(),
                                                    pTrack->getCodecType(),
                                                    pTrack->getTrackID(),
                                                    pTrack->getMediaTimeScale());
            }
        }

        // delete current MDAT atom for movie fragment
        PV_MP4_FF_DELETE(NULL, PVA_FF_MediaDataAtom, _pCurrentMediaDataAtom);

        // form new MDAT atom
        PVA_FF_MediaDataAtom *pMdatAtom = NULL;
        PVA_FF_UNICODE_STRING_PARAM emptyString = PVA_FF_UNICODE_HEAP_STRING(_STRLIT_WCHAR(""));
        PV_MP4_FF_NEW(fp->auditCB, PVA_FF_MediaDataAtom, (emptyString, _targetFileHandle, _aFs, iCacheSize), pMdatAtom);

        _pCurrentMediaDataAtom = pMdatAtom;

        // current moof offset set at start of mdat, (later updated by size of mdat atom)
        _currentMoofOffset = _baseOffset;

        // base offset set to start of mdat (after fourcc code) as base data offset
        _baseOffset += _pCurrentMediaDataAtom->prepareTargetFileForFragments(_offsetDataRenderedToFile);
    }

    return true;
}

bool
PVA_FF_Mpeg4File::addTextMediaSampleInterleave(uint32 trackID, PVMP4FFComposerSampleParam *pSampleParam)
{
    if (pSampleParam == NULL)
        return false;

    PVA_FF_TrackAtom *mediaTrack = _pmovieAtom->getMediaTrack(trackID);
    PVA_FF_MediaDataAtom *mdatAtom = getMediaDataAtomForTrack(trackID);
    PVA_FF_InterLeaveBuffer *pInterLeaveBuffer = getInterLeaveBuffer(trackID);
    PVA_FF_MP4_CODEC_TYPE codecType = _pmovieAtom->getCodecType(trackID);
    uint32 mediaType  = mediaTrack->getMediaType();

    if (_oFirstSampleEditMode)
    {
        _oChunkStart = true;
        /*
         * In this mode very first sample in each track is authored
         * in a separate chunk. It is easier to go back and edit the
         * sample meta data if we authored it in a seperate chunk by
         * itself.
         */
        if (mediaTrack->IsFirstSample())
        {
            // Add to moov atom (in turn adds to tracks)
            pSampleParam->_baseOffset = _baseOffset;

            _pmovieAtom->addTextSampleToTrack(trackID,
                                              pSampleParam,
                                              _oChunkStart);
            _oChunkStart = false;

            if (!mdatAtom->addRawSample(pSampleParam->_fragmentList, pSampleParam->_sampleSize, mediaType, codecType))
            {
                return false;
            }
            _baseOffset += pSampleParam->_sampleSize;
            return true;
        }
    }

    if (!pInterLeaveBuffer->checkInterLeaveBufferSpace(pSampleParam->_sampleSize))
    {
        // Set Chunk end time to the last sample TS
        // in the interleave buffer
        pInterLeaveBuffer->setLastChunkEndTime();

        _oChunkStart = true;

        Oscl_Vector<uint32, OsclMemAllocator> *tsVec =
            pInterLeaveBuffer->getTimeStampVec();

        Oscl_Vector<uint32, OsclMemAllocator> *sizeVec =
            pInterLeaveBuffer->getSampleSizeVec();

        Oscl_Vector<uint8, OsclMemAllocator> *flagsVec =
            pInterLeaveBuffer->getFlagsVec();

        Oscl_Vector<int32, OsclMemAllocator> *indexVec =
            pInterLeaveBuffer->getTextIndexVec();

        int32 numBufferedSamples = tsVec->size();

        pSampleParam->_baseOffset = _baseOffset;
        for (int32 i = 0; i < numBufferedSamples; i++)
        {
            PVMP4FFComposerSampleParam  sampleParam;
            sampleParam._timeStamp   = (*tsVec)[i];
            sampleParam._sampleSize = (*sizeVec)[i];
            sampleParam._flags = (*flagsVec)[i];
            sampleParam._index = (*indexVec)[i];
            sampleParam._baseOffset = _baseOffset;

            // Add to moov atom (in turn adds to tracks)
            _pmovieAtom->addTextSampleToTrack(trackID,
                                              &sampleParam,
                                              _oChunkStart);

            _oChunkStart = false;
        }

        if (numBufferedSamples > 0)
        {
            //Render chunk
            uint32 chunkSize = 0;
            uint8* ptr =
                pInterLeaveBuffer->resetInterLeaveBuffer(chunkSize);

            if (!mdatAtom->addRawSample(ptr, chunkSize))
            {
                return false;
            }
            _baseOffset += chunkSize;
        }
        pSampleParam->_baseOffset = _baseOffset;

        if (!(pInterLeaveBuffer->addSampleToInterLeaveBuffer(pSampleParam)))
        {
            return false;
        }
    }
    else
    {
        if (checkInterLeaveDuration(trackID, pSampleParam->_timeStamp))
        {
            _oChunkStart = true;

            Oscl_Vector<uint32, OsclMemAllocator> *tsVec =
                pInterLeaveBuffer->getTimeStampVec();

            Oscl_Vector<uint32, OsclMemAllocator> *sizeVec =
                pInterLeaveBuffer->getSampleSizeVec();

            Oscl_Vector<uint8, OsclMemAllocator> *flagsVec =
                pInterLeaveBuffer->getFlagsVec();

            Oscl_Vector<int32, OsclMemAllocator> *indexVec =
                pInterLeaveBuffer->getTextIndexVec();

            int32 numBufferedSamples = tsVec->size();

            pSampleParam->_baseOffset = _baseOffset;
            for (int32 i = 0; i < numBufferedSamples; i++)
            {
                PVMP4FFComposerSampleParam  sampleParam;
                sampleParam._timeStamp   = (*tsVec)[i];
                sampleParam._sampleSize = (*sizeVec)[i];
                sampleParam._flags = (*flagsVec)[i];
                sampleParam._index = (*indexVec)[i];
                sampleParam._baseOffset = _baseOffset;

                // Add to moov atom (in turn adds to tracks)
                _pmovieAtom->addTextSampleToTrack(trackID,
                                                  &sampleParam,
                                                  _oChunkStart);



                _oChunkStart = false;
            }

            if (numBufferedSamples > 0)
            {
                //Render chunk
                uint32 chunkSize = 0;
                uint8* ptr =
                    pInterLeaveBuffer->resetInterLeaveBuffer(chunkSize);

                if (!mdatAtom->addRawSample(ptr, chunkSize))
                {
                    return false;
                }
                _baseOffset += chunkSize;
            }
        }
        else
        {
            _oChunkStart = false;
        }

        pSampleParam->_baseOffset = _baseOffset;
        if (!(pInterLeaveBuffer->addSampleToInterLeaveBuffer(pSampleParam)))
        {
            return false;
        }
    }

    return true;
}

bool
PVA_FF_Mpeg4File::reAuthorFirstSampleInTrack(uint32 trackID,
        uint8 *psample,
        uint32 size)
{
    bool retVal = false;
    if (_oInterLeaveEnabled)
    {
        PVA_FF_TrackAtom *mediaTrack = _pmovieAtom->getMediaTrack(trackID);
        PVA_FF_MediaDataAtom *mdatAtom = getMediaDataAtomForTrack(trackID);

        mediaTrack = _pmovieAtom->getMediaTrack(trackID);

        // Add to moov atom (in turn adds to tracks)
        retVal =
            _pmovieAtom->reAuthorFirstSampleInTrack(trackID,
                                                    size,
                                                    _baseOffset);

        if (!mdatAtom->addRawSample(psample, size))
        {
            retVal = false;
        }
        else
        {
            _baseOffset += size;
        }
    }
    return retVal;
}


bool
PVA_FF_Mpeg4File::renderMoovAtom()
{
    MP4_AUTHOR_FF_FILE_IO_WRAP fp;
    fp._filePtr = NULL;
    fp._osclFileServerSession = NULL;

    //make sure to flush the interleave buffers, be it to temp files
    //or to target files
    uint32 kk = 0;

    for (kk = 0; kk < _pmediaDataAtomVec->size(); kk++)
    {
        Oscl_Vector<PVA_FF_TrackAtom*, OsclMemAllocator> *trefVec =
            (*_pmediaDataAtomVec)[kk]->getTrackReferencePtrVec();

        if (trefVec != NULL)
        {
            for (uint32 trefVecIndex = 0; trefVecIndex < trefVec->size(); trefVecIndex++)
            {
                PVA_FF_TrackAtom* pTrack = (*trefVec)[trefVecIndex];
                uint32 trackID = pTrack->getTrackID();
                PVA_FF_InterLeaveBuffer *pInterLeaveBuffer = getInterLeaveBuffer(trackID);
                if (pInterLeaveBuffer != NULL)
                {
                    uint32 ts = pInterLeaveBuffer->getFirstTSEntry();
                    pTrack->updateLastTSEntry(ts);
                }
            }
        }
    }

    bool targetRender = false;
    _offsetDataRenderedToFile = 0;

    if (_totalTempFileRemoval)
    {
        for (uint32 kk = 0; kk < _pmediaDataAtomVec->size(); kk++)
        {
            if (targetRender)
            {
                //Only one track is allowed to be rendered directly onto the target
                //file
                return false;
            }
            else
            {
                targetRender = true;

                if (!((*_pmediaDataAtomVec)[kk]->closeTargetFile()))
                {
                    return false;
                }

                fp._filePtr = ((*_pmediaDataAtomVec)[kk]->getTargetFilePtr());
                fp._osclFileServerSession = OSCL_STATIC_CAST(Oscl_FileServer*, _aFs);

                _offsetDataRenderedToFile =
                    ((*_pmediaDataAtomVec)[kk]->getTotalDataRenderedToTargetFile());
            }
        }
    }

    if (fp._filePtr == NULL)
    {
        return false;
    }

    if (!renderToFileStream(&fp))
    {
        return false;
    }
    _offsetDataRenderedToFile = PVA_FF_AtomUtils::getCurrentFilePosition(&fp); // hereafter movie fragments are written
    _baseOffset = _offsetDataRenderedToFile; // base offset is used to set base data offset of Moof


    // store target file handle used to write further movie fragments
    _targetFileHandle = fp._filePtr;

    return true;
}

bool
PVA_FF_Mpeg4File::renderMovieFragments()
{
    uint32 size;

    uint32 fileWriteOffset;

    MP4_AUTHOR_FF_FILE_IO_WRAP fp;

    fp._filePtr = _pCurrentMediaDataAtom->getTargetFilePtr();
    fp._osclFileServerSession = OSCL_STATIC_CAST(Oscl_FileServer*, _aFs);

    fileWriteOffset = PVA_FF_AtomUtils::getCurrentFilePosition(&fp);

    if (_totalTempFileRemoval)
        _pCurrentMediaDataAtom->closeTargetFile();

    size = _pCurrentMediaDataAtom->getMediaDataSize();

    _pMfraAtom->updateMoofOffset(size);

    PVA_FF_AtomUtils::seekFromStart(&fp, fileWriteOffset);


    if (!(_pCurrentMoofAtom->renderToFileStream(&fp)))
    {
        return false;
    }

    _offsetDataRenderedToFile = PVA_FF_AtomUtils::getCurrentFilePosition(&fp); // hereafter further movie fragments are written
    _baseOffset = _offsetDataRenderedToFile; // base offset is used to set base data offset of Moof

    return true;
}

void PVA_FF_Mpeg4File::addMovieFragmentRandAccessEntry(uint32 trackId, uint64 time, uint64 moofOffset,
        uint32 trafNumber, uint32 trunNumber,  uint32 sampleNumber)
{
    // Add a new track fragment rand. access box for live movie fragment mode
    if (_oLiveMovieFragmentEnabled)
        _pMfraAtom->addSampleEntry(trackId, time, moofOffset, trafNumber, trunNumber,  sampleNumber);
}


void
PVA_FF_Mpeg4File::addInterLeaveBuffer(PVA_FF_InterLeaveBuffer *pInterLeaveBuffer)
{
    if (_oInterLeaveEnabled)
    {
        if (_modifiable)
        {
            _pInterLeaveBufferVec->push_back(pInterLeaveBuffer);
        }
    }
}



PVA_FF_InterLeaveBuffer*
PVA_FF_Mpeg4File::getInterLeaveBuffer(uint32 trackID)
{
    if (_pInterLeaveBufferVec->size() > 0)
    {
        for (uint32 ii = 0; ii < _pInterLeaveBufferVec->size(); ii++)
        {
            if ((*_pInterLeaveBufferVec)[ii]->getTrackID() == trackID)
                return (*_pInterLeaveBufferVec)[ii];
        }
    }
    return NULL;
}

void
PVA_FF_Mpeg4File::setAudioEncodeParams(uint32 trackId,
                                       PVMP4FFComposerAudioEncodeParams &audioParams)
{
    PVA_FF_TrackAtom *trackAtom;
    trackAtom = _pmovieAtom->getMediaTrack(trackId);

    if (trackAtom != NULL)
        trackAtom->setAudioEncodeParams(audioParams);

    return;
}

void
PVA_FF_Mpeg4File::setUserDataInfo(uint32 size, uint8* buff)
{
    if (_pmovieAtom)
        _pmovieAtom->setUserDataInfo(size, buff);
}





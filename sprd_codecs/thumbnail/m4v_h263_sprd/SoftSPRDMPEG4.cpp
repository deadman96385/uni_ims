/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "SoftSPRDMPEG4"
#include <utils/Log.h>

#include "SoftSPRDMPEG4.h"

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/IOMX.h>

#include "m4v_h263_dec_api.h"


namespace android {

typedef enum
{
    H263_MODE = 0,MPEG4_MODE,
    FLV_MODE,
    UNKNOWN_MODE
} MP4DecodingMode;

#define MPEG4_DECODER_INTERNAL_BUFFER_SIZE 10*1024

static const CodecProfileLevel kM4VProfileLevels[] = {
    { OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level0 },
    { OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level0b },
    { OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level1 },
    { OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level2 },
    { OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level3 },

    { OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level0 },
    { OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level0b },
    { OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level1 },
    { OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level2 },
    { OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level3 },
    { OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level4 },
    { OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level4a },
    { OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level5 },
};

static const CodecProfileLevel kH263ProfileLevels[] = {
    { OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level10 },
    { OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level20 },
    { OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level30 },
    { OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level45 },
    { OMX_VIDEO_H263ProfileISWV2,    OMX_VIDEO_H263Level10 },
    { OMX_VIDEO_H263ProfileISWV2,    OMX_VIDEO_H263Level20 },
    { OMX_VIDEO_H263ProfileISWV2,    OMX_VIDEO_H263Level30 },
    { OMX_VIDEO_H263ProfileISWV2,    OMX_VIDEO_H263Level45 },
};

template<class T>
static void InitOMXParams(T *params) {
    params->nSize = sizeof(T);
    params->nVersion.s.nVersionMajor = 1;
    params->nVersion.s.nVersionMinor = 0;
    params->nVersion.s.nRevision = 0;
    params->nVersion.s.nStep = 0;
}

void dump_bs( uint8 * pBuffer,uint32 aInBufSize)
{
    FILE *fp = fopen("/data/video_es.m4v","ab");
    fwrite(pBuffer,1,aInBufSize,fp);
    fclose(fp);
}

void dump_yuv( uint8 * pBuffer,uint32 aInBufSize)
{
    FILE *fp = fopen("/data/video_omx.yuv","ab");
    fwrite(pBuffer,1,aInBufSize,fp);
    fclose(fp);
}

SoftSPRDMPEG4::SoftSPRDMPEG4(
    const char *name,
    const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData,
    OMX_COMPONENTTYPE **component)
    : SprdSimpleOMXComponent(name, callbacks, appData, component),
      mMode(MODE_MPEG4),
      mHandle(new tagMP4Handle),
      mInputBufferCount(0),
      mWidth(352),
      mHeight(288),
      mCropLeft(0),
      mCropTop(0),
      mCropRight(mWidth - 1),
      mCropBottom(mHeight - 1),
      mSignalledError(false),
      mInitialized(false),
      mFramesConfigured(false),
      mNumSamplesOutput(0),
      mOutputPortSettingsChange(NONE),
      mStopDecode(false),
      mCodecExtraBufferMalloced(false) {
    if (!strcmp(name, "OMX.sprd.soft.h263.decoder")) {
        mMode = MODE_H263;
    } else {
        CHECK(!strcmp(name, "OMX.sprd.soft.mpeg4.decoder"));
    }

    initPorts();
    CHECK_EQ(initDecoder(), (status_t)OK);
}

SoftSPRDMPEG4::~SoftSPRDMPEG4() {
    if (mInitialized) {
        MP4DecRelease(mHandle);
    }

    delete mHandle;
    mHandle = NULL;

    free(mCodecInterBuffer);
    mCodecInterBuffer = NULL;

    if (mCodecExtraBufferMalloced)
    {
        free(mCodecExtraBuffer);
        mCodecExtraBuffer = NULL;
    }
}

void SoftSPRDMPEG4::initPorts() {
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParams(&def);

    def.nPortIndex = 0;
    def.eDir = OMX_DirInput;
    def.nBufferCountMin = kNumInputBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.nBufferSize = 8192;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainVideo;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 1;

    def.format.video.cMIMEType =
        (mMode == MODE_MPEG4)
        ? const_cast<char *>(MEDIA_MIMETYPE_VIDEO_MPEG4)
        : const_cast<char *>(MEDIA_MIMETYPE_VIDEO_H263);

    def.format.video.pNativeRender = NULL;
    def.format.video.nFrameWidth = mWidth;
    def.format.video.nFrameHeight = mHeight;
    def.format.video.nStride = def.format.video.nFrameWidth;
    def.format.video.nSliceHeight = def.format.video.nFrameHeight;
    def.format.video.nBitrate = 0;
    def.format.video.xFramerate = 0;
    def.format.video.bFlagErrorConcealment = OMX_FALSE;

    def.format.video.eCompressionFormat =
        mMode == MODE_MPEG4 ? OMX_VIDEO_CodingMPEG4 : OMX_VIDEO_CodingH263;

    def.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    def.format.video.pNativeWindow = NULL;

    addPort(def);

    def.nPortIndex = 1;
    def.eDir = OMX_DirOutput;
    def.nBufferCountMin = kNumOutputBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainVideo;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 2;

    def.format.video.cMIMEType = const_cast<char *>(MEDIA_MIMETYPE_VIDEO_RAW);
    def.format.video.pNativeRender = NULL;
    def.format.video.nFrameWidth = mWidth;
    def.format.video.nFrameHeight = mHeight;
    def.format.video.nStride = def.format.video.nFrameWidth;
    def.format.video.nSliceHeight = def.format.video.nFrameHeight;
    def.format.video.nBitrate = 0;
    def.format.video.xFramerate = 0;
    def.format.video.bFlagErrorConcealment = OMX_FALSE;
    def.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    def.format.video.eColorFormat = OMX_COLOR_FormatYUV420Planar;
    def.format.video.pNativeWindow = NULL;

    def.nBufferSize =
        (def.format.video.nFrameWidth * def.format.video.nFrameHeight * 3) / 2;

    addPort(def);
}

status_t SoftSPRDMPEG4::initDecoder() {
    memset(mHandle, 0, sizeof(tagMP4Handle));

    mHandle->userdata = (void *)this;
    mHandle->VSP_extMemCb = extMemoryAllocWrapper;

    mCodecInterBufferSize = MPEG4_DECODER_INTERNAL_BUFFER_SIZE;
    mCodecInterBuffer = (uint8 *)malloc(mCodecInterBufferSize);

    MMCodecBuffer codec_buf;

    codec_buf.int_buffer_ptr = (uint8 *)( mCodecInterBuffer);
    codec_buf.int_size = mCodecInterBufferSize;

    MMDecRet success = MP4DecInit( mHandle, &codec_buf);

    return OK;
}

OMX_ERRORTYPE SoftSPRDMPEG4::internalGetParameter(
    OMX_INDEXTYPE index, OMX_PTR params) {
    switch (index) {
    case OMX_IndexParamVideoPortFormat:
    {
        OMX_VIDEO_PARAM_PORTFORMATTYPE *formatParams =
            (OMX_VIDEO_PARAM_PORTFORMATTYPE *)params;

        if (formatParams->nPortIndex > 1) {
            return OMX_ErrorUndefined;
        }

        if (formatParams->nIndex != 0) {
            return OMX_ErrorNoMore;
        }

        if (formatParams->nPortIndex == 0) {
            formatParams->eCompressionFormat =
                (mMode == MODE_MPEG4)
                ? OMX_VIDEO_CodingMPEG4 : OMX_VIDEO_CodingH263;

            formatParams->eColorFormat = OMX_COLOR_FormatUnused;
            formatParams->xFramerate = 0;
        } else {
            CHECK_EQ(formatParams->nPortIndex, 1u);

            formatParams->eCompressionFormat = OMX_VIDEO_CodingUnused;
            formatParams->eColorFormat = OMX_COLOR_FormatYUV420Planar;
            formatParams->xFramerate = 0;
        }

        return OMX_ErrorNone;
    }

    case OMX_IndexParamVideoProfileLevelQuerySupported:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *profileLevel =
            (OMX_VIDEO_PARAM_PROFILELEVELTYPE *) params;

        if (profileLevel->nPortIndex != 0) {  // Input port only
            ALOGE("Invalid port index: %ld", profileLevel->nPortIndex);
            return OMX_ErrorUnsupportedIndex;
        }

        size_t index = profileLevel->nProfileIndex;
        if (mMode == MODE_H263) {
            size_t nProfileLevels =
                sizeof(kH263ProfileLevels) / sizeof(kH263ProfileLevels[0]);
            if (index >= nProfileLevels) {
                return OMX_ErrorNoMore;
            }

            profileLevel->eProfile = kH263ProfileLevels[index].mProfile;
            profileLevel->eLevel = kH263ProfileLevels[index].mLevel;
        } else {
            size_t nProfileLevels =
                sizeof(kM4VProfileLevels) / sizeof(kM4VProfileLevels[0]);
            if (index >= nProfileLevels) {
                return OMX_ErrorNoMore;
            }

            profileLevel->eProfile = kM4VProfileLevels[index].mProfile;
            profileLevel->eLevel = kM4VProfileLevels[index].mLevel;
        }
        return OMX_ErrorNone;
    }

    default:
        return SprdSimpleOMXComponent::internalGetParameter(index, params);
    }
}

OMX_ERRORTYPE SoftSPRDMPEG4::internalSetParameter(
    OMX_INDEXTYPE index, const OMX_PTR params) {
    switch (index) {
    case OMX_IndexParamStandardComponentRole:
    {
        const OMX_PARAM_COMPONENTROLETYPE *roleParams =
            (const OMX_PARAM_COMPONENTROLETYPE *)params;

        if (mMode == MODE_MPEG4) {
            if (strncmp((const char *)roleParams->cRole,
                        "video_decoder.mpeg4",
                        OMX_MAX_STRINGNAME_SIZE - 1)) {
                return OMX_ErrorUndefined;
            }
        } else {
            if (strncmp((const char *)roleParams->cRole,
                        "video_decoder.h263",
                        OMX_MAX_STRINGNAME_SIZE - 1)) {
                return OMX_ErrorUndefined;
            }
        }

        return OMX_ErrorNone;
    }

    case OMX_IndexParamVideoPortFormat:
    {
        OMX_VIDEO_PARAM_PORTFORMATTYPE *formatParams =
            (OMX_VIDEO_PARAM_PORTFORMATTYPE *)params;

        if (formatParams->nPortIndex > 1) {
            return OMX_ErrorUndefined;
        }

        if (formatParams->nIndex != 0) {
            return OMX_ErrorNoMore;
        }

        return OMX_ErrorNone;
    }

    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE *defParams =
            (OMX_PARAM_PORTDEFINITIONTYPE *)params;

        if (defParams->nPortIndex > 1
                || defParams->nSize
                != sizeof(OMX_PARAM_PORTDEFINITIONTYPE)) {
            return OMX_ErrorUndefined;
        }

        PortInfo *port = editPortInfo(defParams->nPortIndex);

        if (defParams->nBufferSize != port->mDef.nBufferSize) {
            CHECK_GE(defParams->nBufferSize, port->mDef.nBufferSize);
            port->mDef.nBufferSize = defParams->nBufferSize;
        }

        if (defParams->nBufferCountActual
                != port->mDef.nBufferCountActual) {
            CHECK_GE(defParams->nBufferCountActual,
                     port->mDef.nBufferCountMin);

            port->mDef.nBufferCountActual = defParams->nBufferCountActual;
        }

        memcpy(&port->mDef.format.video, &defParams->format.video, sizeof(OMX_VIDEO_PORTDEFINITIONTYPE));

        if(defParams->nPortIndex == 1) {
            port->mDef.format.video.nStride = port->mDef.format.video.nFrameWidth;
            port->mDef.format.video.nSliceHeight = port->mDef.format.video.nFrameHeight;
            mWidth = port->mDef.format.video.nFrameWidth;
            mHeight = port->mDef.format.video.nFrameHeight;
            mCropRight = mWidth - 1;
            mCropBottom = mHeight -1;
            port->mDef.nBufferSize =(((mWidth + 15) & -16)* ((mHeight + 15) & -16) * 3) / 2;
        }

        return OMX_ErrorNone;
    }

    default:
        return SprdSimpleOMXComponent::internalSetParameter(index, params);
    }
}

OMX_ERRORTYPE SoftSPRDMPEG4::getConfig(
    OMX_INDEXTYPE index, OMX_PTR params) {
    switch (index) {
    case OMX_IndexConfigCommonOutputCrop:
    {
        OMX_CONFIG_RECTTYPE *rectParams = (OMX_CONFIG_RECTTYPE *)params;

        if (rectParams->nPortIndex != 1) {
            return OMX_ErrorUndefined;
        }

        rectParams->nLeft = mCropLeft;
        rectParams->nTop = mCropTop;
        rectParams->nWidth = mCropRight - mCropLeft + 1;
        rectParams->nHeight = mCropBottom - mCropTop + 1;

        return OMX_ErrorNone;
    }

    default:
        return OMX_ErrorUnsupportedIndex;
    }
}

void SoftSPRDMPEG4::onQueueFilled(OMX_U32 portIndex) {
    if (mSignalledError || mOutputPortSettingsChange != NONE) {
        return;
    }

    List<BufferInfo *> &inQueue = getPortQueue(0);
    List<BufferInfo *> &outQueue = getPortQueue(1);

    while (!mStopDecode && (!inQueue.empty()) && (outQueue.size() == kNumOutputBuffers)) {
        BufferInfo *inInfo = *inQueue.begin();
        OMX_BUFFERHEADERTYPE *inHeader = inInfo->mHeader;

        PortInfo *port = editPortInfo(1);

        OMX_BUFFERHEADERTYPE *outHeader =
            port->mBuffers.editItemAt(mNumSamplesOutput & 1).mHeader;

        if (inHeader->nFlags & OMX_BUFFERFLAG_EOS) {
            inQueue.erase(inQueue.begin());
            inInfo->mOwnedByUs = false;
            notifyEmptyBufferDone(inHeader);

            outHeader->nFilledLen = 0;
            outHeader->nFlags = OMX_BUFFERFLAG_EOS;

            List<BufferInfo *>::iterator it = outQueue.begin();
            while ((*it)->mHeader != outHeader) {
                ++it;
            }

            BufferInfo *outInfo = *it;
            outInfo->mOwnedByUs = false;
            outQueue.erase(it);
            outInfo = NULL;

            notifyFillBufferDone(outHeader);
            outHeader = NULL;
            return;
        }

        uint8_t *bitstream = inHeader->pBuffer + inHeader->nOffset;

//        ALOGI("%s, %d, %0x, %0x, %0x, %0x, %0x, %0x, %d", __FUNCTION__, __LINE__, bitstream[0],bitstream[1],bitstream[2],bitstream[3],bitstream[4],bitstream[5],inHeader->nFilledLen);

        if (!mInitialized) {
            uint8_t *vol_data[1];
            int32_t vol_size = 0;

            vol_data[0] = NULL;

            if (inHeader->nFlags & OMX_BUFFERFLAG_CODECCONFIG) {
                vol_data[0] = bitstream;
                vol_size = inHeader->nFilledLen;
            }

            MMDecVideoFormat video_format;

            video_format.i_extra = vol_size;
            if( video_format.i_extra>0)
            {
                video_format.p_extra =(void *)(vol_data[0]);
#if 0
                dump_bs((uint8_t *)video_format.p_extra,  video_format.i_extra);
#endif
            } else {
                video_format.p_extra = NULL;
            }

            if(mMode == MODE_H263)
            {
                video_format.video_std = ITU_H263;
            } else if(mMode == MODE_MPEG4)
            {
                video_format.video_std = MPEG4;
            } else if(mMode == MODE_FLV)
            {
                video_format.video_std = FLV_V1;
            } else
            {
                video_format.video_std = MPEG4;
            }

            video_format.frame_width = 0;
            video_format.frame_height = 0;
            video_format.uv_interleaved = 0;// todo jgdu

            MMDecRet ret = MP4DecVolHeader(mHandle, &video_format);

            ALOGI("%s, %d, ret: %d, video_format.i_extra: %d, width: %d, height: %d", __FUNCTION__, __LINE__,
                  ret, video_format.i_extra, video_format.frame_width, video_format.frame_height);

            if (ret != MMDEC_OK) {
                ALOGW("PVInitVideoDecoder failed. Unsupported content?");

                notify(OMX_EventError, OMX_ErrorUndefined, 0, NULL);
                mSignalledError = true;
                return;
            }

            if (inHeader->nFlags & OMX_BUFFERFLAG_CODECCONFIG) {
                inInfo->mOwnedByUs = false;
                inQueue.erase(inQueue.begin());
                inInfo = NULL;
                notifyEmptyBufferDone(inHeader);
                inHeader = NULL;
            }

            mInitialized = true;

            // Deleted for bug#168365
            //if (mMode == MODE_MPEG4&& portSettingsChanged()) {
            //   return;
            //}
            continue;
        }

        if (!mFramesConfigured) {
            PortInfo *port = editPortInfo(1);
            OMX_BUFFERHEADERTYPE *outHeader = port->mBuffers.editItemAt(1).mHeader;

            MP4DecSetReferenceYUV(mHandle, outHeader->pBuffer);

            mFramesConfigured = true;
        }

        uint32_t useExtTimestamp = (inHeader->nOffset == 0);

        // decoder deals in ms, OMX in us.
        uint32_t timestamp =
            useExtTimestamp ? (inHeader->nTimeStamp + 500) / 1000 : 0xFFFFFFFF;

        int32_t bufferSize = inHeader->nFilledLen;

        // The PV decoder is lying to us, sometimes it'll claim to only have
        // consumed a subset of the buffer when it clearly consumed all of it.
        // ignore whatever it says...
        MMDecInput dec_in;
        MMDecOutput dec_out;
        int ret;

        MP4DecSetCurRecPic( mHandle, outHeader->pBuffer);

        dec_in.pStream = (uint8 *) bitstream;
        dec_in.dataLen = bufferSize;
        dec_in.beLastFrm = 0;
        dec_in.expected_IVOP = 0;
        dec_in.beDisplayed = 1;
        dec_in.err_pkt_num = 0;

        dec_out.VopPredType = -1;
        dec_out.frameEffective = 0;

#if 0
        dump_bs((uint8_t *)dec_in.pStream,  dec_in.dataLen);
#endif

        MMDecRet decRet =	MP4DecDecode( mHandle, &dec_in,&dec_out);
        ALOGI("%s, %d, video_format.i_extra: %d, decRet: %d, dec_out.frameEffective: %d", __FUNCTION__, __LINE__, dec_in.dataLen, decRet, dec_out.frameEffective);

        if (decRet == MMDEC_OK || decRet == MMDEC_MEMORY_ALLOCED )
        {
            if (portSettingsChanged()) {
                return;
            } else if( decRet == MMDEC_MEMORY_ALLOCED)
            {
                mFramesConfigured =  false;
                continue;
            }
        } else if (decRet == MMDEC_FRAME_SEEK_IVOP)
        {
            inInfo->mOwnedByUs = false;
            inQueue.erase(inQueue.begin());
            inInfo = NULL;
            notifyEmptyBufferDone(inHeader);
            inHeader = NULL;

            continue;
        } else
        {
            ALOGE("failed to decode video frame.");

            notify(OMX_EventError, OMX_ErrorUndefined, 0, NULL);
            mSignalledError = true;
            return;
        }

        // decoder deals in ms, OMX in us.
        outHeader->nTimeStamp = timestamp * 1000;

        CHECK_LE(bufferSize, inHeader->nFilledLen);
        inHeader->nOffset += bufferSize;
        inHeader->nFilledLen -= bufferSize;

        if (inHeader->nFilledLen == 0) {
            inInfo->mOwnedByUs = false;
            inQueue.erase(inQueue.begin());
            inInfo = NULL;
            notifyEmptyBufferDone(inHeader);
            inHeader = NULL;

            ALOGI("%s, %d", __FUNCTION__, __LINE__);
        }

        if (dec_out.frameEffective)
        {
            outHeader->nOffset = 0;
            outHeader->nFilledLen = (mWidth * mHeight * 3) / 2;
            outHeader->nFlags = 0;

            ALOGI("%s, %d, outHeader->nFilledLen: %d", __FUNCTION__, __LINE__, outHeader->nFilledLen);
#if 0
            dump_yuv(outHeader->pBuffer, outHeader->nFilledLen);
#endif
        } else {
            ALOGE("%s, %d, frameEffective=0, return error.", __FUNCTION__, __LINE__);
            notify(OMX_EventError, OMX_ErrorUndefined, 0, NULL);
            mSignalledError = true;
            return;
        }

        List<BufferInfo *>::iterator it = outQueue.begin();
        while ((*it)->mHeader != outHeader) {
            ++it;
        }

        BufferInfo *outInfo = *it;
        outInfo->mOwnedByUs = false;
        outQueue.erase(it);
        outInfo = NULL;

        notifyFillBufferDone(outHeader);
        outHeader = NULL;
        mStopDecode = true;

        ++mNumSamplesOutput;
        ALOGI("%s, %d, mNumSamplesOutput: %d", __FUNCTION__, __LINE__, mNumSamplesOutput);
    }
}

bool SoftSPRDMPEG4::portSettingsChanged() {
    int32_t disp_width, disp_height;
    int32_t buf_width, buf_height;

    Mp4GetVideoDimensions(mHandle, &disp_width, &disp_height);
    Mp4GetBufferDimensions(mHandle, &buf_width, &buf_height);

    ALOGI("%s, %d, disp_width = %d, disp_height = %d, buf_width = %d, buf_height = %d", __FUNCTION__, __LINE__,
          disp_width, disp_height, buf_width, buf_height);

    CHECK_LE(disp_width, buf_width);
    CHECK_LE(disp_height, buf_height);

    if (mCropRight != disp_width - 1
            || mCropBottom != disp_height - 1) {
        ALOGI("%s, %d, mCropLeft: %d, mCropTop: %d, mCropRight: %d, mCropBottom: %d", __FUNCTION__, __LINE__, mCropLeft, mCropTop, mCropRight, mCropBottom);
        mCropLeft = 0;
        mCropTop = 0;
        mCropRight = disp_width - 1;
        mCropBottom = disp_height - 1;

        notify(OMX_EventPortSettingsChanged,
               1,
               OMX_IndexConfigCommonOutputCrop,
               NULL);
    }

    if (buf_width != mWidth || buf_height != mHeight) {
        ALOGI("%s, %d, mWidth: %d, mHeight: %d", __FUNCTION__, __LINE__, mWidth, mHeight);
        mWidth = buf_width;
        mHeight = buf_height;

        updatePortDefinitions();

        mFramesConfigured = false;

        notify(OMX_EventPortSettingsChanged, 1, 0, NULL);
        mOutputPortSettingsChange = AWAITING_DISABLED;
        return true;
    }

    return false;
}

void SoftSPRDMPEG4::onPortFlushCompleted(OMX_U32 portIndex) {
    if (portIndex == 0 && mInitialized) {
//        CHECK_EQ((int)PVResetVideoDecoder(mHandle), (int)PV_TRUE);
    }
}

void SoftSPRDMPEG4::onPortEnableCompleted(OMX_U32 portIndex, bool enabled) {
    if (portIndex != 1) {
        return;
    }

    switch (mOutputPortSettingsChange) {
    case NONE:
        break;

    case AWAITING_DISABLED:
    {
        CHECK(!enabled);
        mOutputPortSettingsChange = AWAITING_ENABLED;
        break;
    }

    default:
    {
        CHECK_EQ((int)mOutputPortSettingsChange, (int)AWAITING_ENABLED);
        CHECK(enabled);
        mOutputPortSettingsChange = NONE;
        break;
    }
    }
}

void SoftSPRDMPEG4::updatePortDefinitions() {
    OMX_PARAM_PORTDEFINITIONTYPE *def = &editPortInfo(0)->mDef;
    def->format.video.nFrameWidth = mWidth;
    def->format.video.nFrameHeight = mHeight;
    def->format.video.nStride = def->format.video.nFrameWidth;
    def->format.video.nSliceHeight = def->format.video.nFrameHeight;

    def = &editPortInfo(1)->mDef;
    def->format.video.nFrameWidth = mWidth;
    def->format.video.nFrameHeight = mHeight;
    def->format.video.nStride = def->format.video.nFrameWidth;
    def->format.video.nSliceHeight = def->format.video.nFrameHeight;

    def->nBufferSize =
        (((def->format.video.nFrameWidth + 15) & -16)
         * ((def->format.video.nFrameHeight + 15) & -16) * 3) / 2;
}

// static
int32_t SoftSPRDMPEG4::extMemoryAllocWrapper(
    void *aUserData, unsigned int width,unsigned int height, unsigned int is_dp) {
    return static_cast<SoftSPRDMPEG4 *>(aUserData)->extMemoryAlloc(width, height, is_dp);
}

int SoftSPRDMPEG4::extMemoryAlloc(unsigned int width,unsigned int height, unsigned int is_dp) {

    int32 Frm_width_align = ((width + 15) & (~15));
    int32 Frm_height_align = ((height + 15) & (~15));
    int32 mb_x = Frm_width_align/16;
    int32 mb_y = Frm_height_align/16;
    int32 total_mb_num = mb_x * mb_y;
    int32 ext_size_y = (mb_x * 16 + 16*2) * (mb_y * 16 + 16*2);
    int32 ext_size_c = ext_size_y >> 2;
    int32 i;

    mCodecExtraBufferSize = total_mb_num * 6 * 2* sizeof(int32); 	//mb_info
    mCodecExtraBufferSize += 4 * 8 * sizeof(int16);				//pLeftCoeff
    mCodecExtraBufferSize += 6 * 64 * sizeof(int16);				//coef_block
    mCodecExtraBufferSize += 1024;
    mCodecExtraBufferSize += 4*8*mb_x*sizeof(int16);	//pTopCoeff
    mCodecExtraBufferSize += ((( 64*4*sizeof(int8) + 255) >>8)<<8);	//mb_cache_ptr->pMBBfrY
    mCodecExtraBufferSize += ((( 64*1*sizeof(int8) + 255) >>8)<<8);	//mb_cache_ptr->pMBBfrU
    mCodecExtraBufferSize += ((( 64*1*sizeof(int8) + 255) >>8)<<8);	//mb_cache_ptr->pMBBfrV

    for (i = 0; i < 3; i++)
    {
        mCodecExtraBufferSize += ((( ext_size_y + 255) >>8)<<8);	//imgYUV[0]
        mCodecExtraBufferSize += ((( ext_size_c + 255) >>8)<<8);	//imgYUV[1]
        mCodecExtraBufferSize += ((( ext_size_c + 255) >>8)<<8);	//imgYUV[2]
    }

#ifdef _MP4CODEC_DATA_PARTITION_
    if (is_dp)
    {
        uint32 i;

        mCodecExtraBufferSize += (sizeof (int32 *) * total_mb_num); //g_dec_dc_store

        for (i = 0; i < total_mb_num; i++)
        {
            mCodecExtraBufferSize += (sizeof (int32) * 6);  //g_dec_dc_store[i]
        }
    }
#endif //_MP4CODEC_DATA_PARTITION_	

    if (mCodecExtraBufferMalloced)
    {
        free(mCodecExtraBuffer);
        mCodecExtraBuffer = NULL;
    }
    mCodecExtraBuffer = (uint8 *)malloc(mCodecExtraBufferSize);
    mCodecExtraBufferMalloced = true;

    MMCodecBuffer codec_buf;

    codec_buf.common_buffer_ptr = mCodecExtraBuffer;
    codec_buf.size = mCodecExtraBufferSize;
    ALOGI("%s, %d, Frm_width_align: %d, Frm_height_align: %d, ext_mem: %0x, ext_mem_size: %d",
          __FUNCTION__, __LINE__, Frm_width_align, Frm_height_align, mCodecExtraBuffer, mCodecExtraBufferSize);

    MP4DecMemInit( ((SoftSPRDMPEG4 *)this)->mHandle,&codec_buf);

    return 1;
}

}  // namespace android

android::SprdOMXComponent *createSprdOMXComponent(
    const char *name, const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData, OMX_COMPONENTTYPE **component) {
    return new android::SoftSPRDMPEG4(name, callbacks, appData, component);
}


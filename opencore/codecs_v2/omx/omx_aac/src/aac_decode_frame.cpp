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

#include "aac_dec.h"

#if PROFILING_ON
#include "oscl_tickcount.h"
#endif

OmxAacDecoder::OmxAacDecoder()
{
    iAacInitFlag = 0;
    iInputUsedLength = 0;

#if PROFILING_ON
    iTotalTicks = 0;
    iNumOutputSamples = 0;
#endif
}


OMX_BOOL OmxAacDecoder::AacDecInit(OMX_U32 aDesiredChannels)
{
    Int                         Status;

    iMemReq = PVMP4AudioDecoderGetMemRequirements();
    ipMem = oscl_malloc(iMemReq);

    if (0 == ipMem)
    {
        return OMX_FALSE;
    }

    oscl_memset(&iExt, 0, sizeof(tPVMP4AudioDecoderExternal));

    aDesiredChannels = 2; // aac decoder outputs 2 channels even in mono case. Keep it that way
    iExt.inputBufferCurrentLength = 0;
    iExt.remainderBits        = 0;      // Not needed anymore.
    iExt.inputBufferMaxLength = PVMP4AUDIODECODER_INBUFSIZE;
    iExt.outputFormat         = OUTPUTFORMAT_16PCM_INTERLEAVED;
    iExt.desiredChannels      = aDesiredChannels;
    iExt.aacPlusEnabled       = TRUE;
    iAacInitFlag = 0;
    iInputUsedLength = 0;
    //This var is required to do init again inbetween
    iNumOfChannels           = aDesiredChannels;


    Status = PVMP4AudioDecoderInitLibrary(&iExt, ipMem);
    return OMX_TRUE;
}

void OmxAacDecoder::AacDecDeinit()
{
    oscl_free(ipMem);
    ipMem = NULL;
}

void OmxAacDecoder::ResetDecoder()
{

    if (ipMem && (iAacInitFlag != 0))
    {
        PVMP4AudioDecoderResetBuffer(ipMem);
    }

}

Int OmxAacDecoder::AacDecodeFrames(OMX_S16* aOutputBuffer,
                                   OMX_U32* aOutputLength, OMX_U8** aInBuffer,
                                   OMX_U32* aInBufSize, OMX_S32* aIsFirstBuffer,
                                   OMX_AUDIO_PARAM_PCMMODETYPE* aAudioPcmParam,
                                   OMX_AUDIO_PARAM_AACPROFILETYPE* aAudioAacParam,
                                   OMX_U32* aSamplesPerFrame,
                                   OMX_BOOL* aResizeFlag)
{
    Int   Status;
    Int32 StreamType;


    *aResizeFlag = OMX_FALSE;

    if (0 == iAacInitFlag)
    {
        //Initialization is required again when the client inbetween rewinds the input bitstream
        //Added to pass khronous conformance tests
        if (*aIsFirstBuffer != 0)
        {
            /* When the input file is reset to the begining by the client,
             * this module should again be called.
             */
            oscl_memset(ipMem, 0, iMemReq);
            oscl_memset(&iExt, 0, sizeof(tPVMP4AudioDecoderExternal));

            iExt.inputBufferCurrentLength = 0;
            iExt.remainderBits        = 0;
            iExt.inputBufferMaxLength = PVMP4AUDIODECODER_INBUFSIZE;
            iExt.outputFormat         = OUTPUTFORMAT_16PCM_INTERLEAVED;
            iExt.desiredChannels      = iNumOfChannels;
            iExt.aacPlusEnabled       = TRUE;
            iInputUsedLength = 0;

            PVMP4AudioDecoderInitLibrary(&iExt, ipMem);
        }
    }

    iExt.pInputBuffer         = *aInBuffer + iInputUsedLength;
    iExt.pOutputBuffer        = &aOutputBuffer[0];

#ifdef AAC_PLUS
    iExt.pOutputBuffer_plus   = &aOutputBuffer[2048];
#endif

    iExt.inputBufferCurrentLength = *aInBufSize;

    //Decode the config buffer
    if (0 == iAacInitFlag)
    {

#if PROFILING_ON
        OMX_U32 StartTime = OsclTickCount::TickCount();
#endif
        Status = PVMP4AudioDecoderConfig(&iExt, ipMem);

#if PROFILING_ON
        OMX_U32 EndTime = OsclTickCount::TickCount();
        iTotalTicks += (EndTime - StartTime);
#endif

        if (MP4AUDEC_SUCCESS == Status)
        {
            iAacInitFlag = 1;
        }

        iConfigUpSamplingFactor = iExt.aacPlusUpsamplingFactor;

        if (2 == iExt.aacPlusUpsamplingFactor)
        {
            *aSamplesPerFrame = 2 * AACDEC_PCM_FRAME_SAMPLE_SIZE;
            aAudioAacParam->nFrameLength = *aSamplesPerFrame;
        }
        else
        {
            *aSamplesPerFrame = AACDEC_PCM_FRAME_SAMPLE_SIZE;
            aAudioAacParam->nFrameLength = *aSamplesPerFrame;
        }


        // record the number of channels (original, i.e. encoded) - the PCM side will always use 2 channels
        aAudioAacParam->nChannels = iExt.encodedChannels;
        /*
         *  *aInBufSize -= iExt.inputBufferUsedLength;  should render *aInBufSize == 0,
         *  If the size of the audio config buffer exceeds the size of the audio config data
         *  the excess bits could be taken as part of next raw aac stream. To avoid that
         *  we force to consume all bits of the audio config buffer, by making *aInBufSize == 0
         */
        *aInBufSize = 0;
        iInputUsedLength = 0;

        return Status;
    }

    iExt.inputBufferUsedLength = 0;

#if PROFILING_ON
    OMX_U32 StartTime = OsclTickCount::TickCount();
#endif

    Status = PVMP4AudioDecodeFrame(&iExt, ipMem);

#if PROFILING_ON
    OMX_U32 EndTime = OsclTickCount::TickCount();
    iTotalTicks += (EndTime - StartTime);
#endif

    if (MP4AUDEC_SUCCESS == Status || SUCCESS == Status)
    {
        *aInBufSize -= iExt.inputBufferUsedLength;
        if (0 == *aInBufSize)
        {
            iInputUsedLength = 0;
        }
        else
        {
            iInputUsedLength += iExt.inputBufferUsedLength;
        }

        *aOutputLength = iExt.frameLength * iExt.desiredChannels;

#ifdef AAC_PLUS
        if (2 == iExt.aacPlusUpsamplingFactor)
        {
            if (1 == iExt.desiredChannels)
            {
                oscl_memcpy(&aOutputBuffer[1024], &aOutputBuffer[2048], (*aOutputLength * 2));
            }
            *aOutputLength *= 2;

        }
#endif
        (*aIsFirstBuffer)++;

        //After decoding the first frame, modify all the input & output port settings
        if (1 == *aIsFirstBuffer)
        {
            StreamType = (Int32) RetrieveDecodedStreamType();

            if ((0 == StreamType) && (2 == iConfigUpSamplingFactor))
            {
                PVMP4AudioDecoderDisableAacPlus(&iExt, &ipMem);
                *aSamplesPerFrame = AACDEC_PCM_FRAME_SAMPLE_SIZE;
                aAudioAacParam->eAACProfile = OMX_AUDIO_AACObjectMain;
                aAudioAacParam->nFrameLength = *aSamplesPerFrame;
            }

            //Output Port Parameters
            aAudioPcmParam->nSamplingRate = iExt.samplingRate;
            aAudioPcmParam->nChannels = iExt.desiredChannels;

            //Input Port Parameters
            aAudioAacParam->nSampleRate = iExt.samplingRate;
            // record the number of channels (original, i.e. encoded) - the PCM side will always use 2 channels
            aAudioAacParam->nChannels = iExt.encodedChannels;

            //Set the Resize flag to send the port settings changed callback
            *aResizeFlag = OMX_TRUE;
        }

#if PROFILING_ON
        iNumOutputSamples += *aSamplesPerFrame;
#endif

        return Status;

    }
    else if (MP4AUDEC_INCOMPLETE_FRAME == Status)
    {
        *aInBuffer += iInputUsedLength;
        iInputUsedLength = 0;
    }
    else
    {
        *aInBufSize = 0;
        iInputUsedLength = 0;
    }

    return Status;
}


//Retrieve the Stream Type of AAC input-bitstream
Int OmxAacDecoder::RetrieveDecodedStreamType()
{

    if ((iExt.extendedAudioObjectType == MP4AUDIO_AAC_LC) ||
            (iExt.extendedAudioObjectType == MP4AUDIO_LTP))
    {
        return AAC;   /*  AAC */
    }
    else if (iExt.extendedAudioObjectType == MP4AUDIO_SBR)
    {
        return AACPLUS;   /*  AAC+ */
    }
    else if (iExt.extendedAudioObjectType == MP4AUDIO_PS)
    {
        return ENH_AACPLUS;   /*  AAC++ */
    }

    return -1;   /*  Error evaluating the stream type */
}


//Change the AACEnable flag, according to the AAC profile set by the client in SetParameter API
void OmxAacDecoder::UpdateAACPlusEnabled(OMX_BOOL flag)
{
    //Mark this flag as false if client sets any non HE AAC profile in SetParameter call
    iExt.aacPlusEnabled = (OMX_TRUE == flag) ? true : false;
}

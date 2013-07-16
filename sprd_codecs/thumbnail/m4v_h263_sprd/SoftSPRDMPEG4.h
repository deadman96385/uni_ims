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

#ifndef SOFT_SPRD_MPEG4_H_

#define SOFT_SPRD_MPEG4_H_

#include "SprdSimpleOMXComponent.h"
#include "m4v_h263_dec_api.h"

namespace android {

struct SoftSPRDMPEG4 : public SprdSimpleOMXComponent {
    SoftSPRDMPEG4(const char *name,
                  const OMX_CALLBACKTYPE *callbacks,
                  OMX_PTR appData,
                  OMX_COMPONENTTYPE **component);

protected:
    virtual ~SoftSPRDMPEG4();

    virtual OMX_ERRORTYPE internalGetParameter(
        OMX_INDEXTYPE index, OMX_PTR params);

    virtual OMX_ERRORTYPE internalSetParameter(
        OMX_INDEXTYPE index, const OMX_PTR params);

    virtual OMX_ERRORTYPE getConfig(OMX_INDEXTYPE index, OMX_PTR params);

    virtual void onQueueFilled(OMX_U32 portIndex);
    virtual void onPortFlushCompleted(OMX_U32 portIndex);
    virtual void onPortEnableCompleted(OMX_U32 portIndex, bool enabled);

private:
    enum {
        kNumInputBuffers  = 4,
        kNumOutputBuffers = 2,
    };

    enum {
        MODE_MPEG4,
        MODE_H263,
        MODE_FLV,

    } mMode;

    tagMP4Handle *mHandle;

    size_t mInputBufferCount;

    int32_t mWidth, mHeight;
    int32_t mCropLeft, mCropTop, mCropRight, mCropBottom;

    bool mSignalledError;
    bool mInitialized;
    bool mFramesConfigured;
    bool mStopDecode;

    int32_t mNumSamplesOutput;

    uint8_t *mCodecInterBuffer;
    uint32_t mCodecInterBufferSize;
    uint8_t *mCodecExtraBuffer;
    uint32_t mCodecExtraBufferSize;
    bool mCodecExtraBufferMalloced;

    static int32_t extMemoryAllocWrapper(void *userData, unsigned int width,unsigned int height, unsigned int is_dp);
    int extMemoryAlloc(unsigned int width,unsigned int height, unsigned int is_dp);

    enum {
        NONE,
        AWAITING_DISABLED,
        AWAITING_ENABLED
    } mOutputPortSettingsChange;

    void initPorts();
    status_t initDecoder();

    void updatePortDefinitions();
    bool portSettingsChanged();

    DISALLOW_EVIL_CONSTRUCTORS(SoftSPRDMPEG4);
};

}  // namespace android

#endif  // SOFT_SPRD_MPEG4_H_



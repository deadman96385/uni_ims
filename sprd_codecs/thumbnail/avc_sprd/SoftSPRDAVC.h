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

#ifndef SOFT_AVC_SPRD_H_

#define SOFT_AVC_SPRD_H_

#include "SprdSimpleOMXComponent.h"
#include <utils/KeyedVector.h>

#include "avc_dec_api.h"
//#include "basetype.h"

namespace android {

struct SoftSPRDAVC : public SprdSimpleOMXComponent {
    SoftSPRDAVC(const char *name,
            const OMX_CALLBACKTYPE *callbacks,
            OMX_PTR appData,
            OMX_COMPONENTTYPE **component);

protected:
    virtual ~SoftSPRDAVC();

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
        kInputPortIndex   = 0,
        kOutputPortIndex  = 1,
        kNumInputBuffers  = 8,
        kNumOutputBuffers = 2,
    };

    enum EOSStatus {
        INPUT_DATA_AVAILABLE,
        INPUT_EOS_SEEN,
        OUTPUT_FRAMES_FLUSHED,
    };

    tagvideoDecControls *mHandle;

    uint8_t *iStream_buffer_ptr;


    size_t mInputBufferCount;

    uint8_t *mCodecInterBuffer;
    uint32_t mCodecInterBufferSize;
    uint8_t *mCodecExtraBuffer;
    uint32_t mCodecExtraBufferSize;
    bool mCodecExtraBufferMalloced;

    uint32_t mWidth, mHeight, mPictureSize;
    uint32_t mCropLeft, mCropTop;
    uint32_t mCropWidth, mCropHeight;

#if 0
    uint8_t *mFirstPicture;
    int32_t mFirstPictureId;
#endif

    int32_t mPicId;  // Which output picture is for which input buffer?

    // OMX_BUFFERHEADERTYPE may be overkill, but it is convenient
    // for tracking the following fields: nFlags, nTimeStamp, etc.
    KeyedVector<int32_t, OMX_BUFFERHEADERTYPE *> mPicToHeaderMap;
    bool mHeadersDecoded;

    EOSStatus mEOSStatus;

    enum OutputPortSettingChange {
        NONE,
        AWAITING_DISABLED,
        AWAITING_ENABLED
    };
    OutputPortSettingChange mOutputPortSettingsChange;

    bool mSignalledError;

    void initPorts();
    status_t initDecoder();
    void updatePortDefinitions();
    bool drainAllOutputBuffers();
    void drainOneOutputBuffer(int32_t picId, uint8_t *data);
#if 0    
    void saveFirstOutputBuffer(int32_t pidId, uint8_t *data);
#endif
    bool handleCropRectEvent(const CropParams* crop);
    bool handlePortSettingChangeEvent(const H264SwDecInfo *info);

    static int32_t ActivateSPSWrapper(void *userData, unsigned int width,unsigned int height, unsigned int numBuffers) ;
//    static int32_t BindFrameWrapper(void *userData/*, int32_t index*/, uint8_t **yuv);
//    static void UnbindFrame(void *userData, int32_t index);

    int activateSPS(unsigned int width,unsigned int height, unsigned int numBuffers);

    int32 bindFrame(/*int32_t index,*/ uint8 **yuv); 

    DISALLOW_EVIL_CONSTRUCTORS(SoftSPRDAVC);
};

}  // namespace android

#endif  // SOFT_AVC_H_


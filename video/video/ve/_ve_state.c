/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12522 $ $Date: 2010-07-13 18:52:44 -0400 (Tue, 13 Jul 2010) $
 *
 */

/*
 * This file handles commands sent to the driver which are pending
 * in the down-queue
 */
#include "_ve_private.h"


/*
 * ======== _VE_initEncoder() ========
 *
 * Inits an encoder.
 */
void _VE_initEncoder(
    _VE_Dsp       *dsp_ptr,
    _VE_StreamObj *stream_ptr,
    vint             type)
{
    if(stream_ptr->enc.encInited) {
        _VE_TRACE(__FILE__, __LINE__);
        return;
    }

    switch (type) {
        case VTSP_CODER_VIDEO_H263:
            dsp_ptr->h263EncParams.height = dsp_ptr->pic.height;
            dsp_ptr->h263EncParams.width = dsp_ptr->pic.width;
            dsp_ptr->h263EncParams.stride = dsp_ptr->pic.stride;
            dsp_ptr->h263EncParams.rotation = dsp_ptr->pic.rotation;
            dsp_ptr->h263EncParams.fps = 20;
            dsp_ptr->h263EncParams.maxBitrate = 
                stream_ptr->streamParam.encodeMaxBps[type];

            /*
             * XXX Parse profile and level Id.
             */
            H263_encInit(&stream_ptr->enc.h263EncObj, &dsp_ptr->h263EncParams);
            stream_ptr->enc.encInited = 1;
            break;
        case VTSP_CODER_VIDEO_H264:
            dsp_ptr->h264EncParams.height = dsp_ptr->pic.height;
            dsp_ptr->h264EncParams.width = dsp_ptr->pic.width;
            dsp_ptr->h263EncParams.stride = dsp_ptr->pic.stride;
            dsp_ptr->h263EncParams.rotation = dsp_ptr->pic.rotation;
            dsp_ptr->h264EncParams.fps = 20;
            dsp_ptr->h264EncParams.maxBitrate = 
                stream_ptr->streamParam.encodeMaxBps[type];

            /*
             * XXX Parse profile and level Id.
             */
            H264_encInit(&stream_ptr->enc.h264EncObj, &dsp_ptr->h264EncParams);
            stream_ptr->enc.encInited = 1;
            break;

        default:
            break;

    }
}


/*
 * ======== _VE_shutEncoder() ========
 *
 * Shuts an encoder.
 */
void _VE_shutEncoder(
    _VE_Dsp       *dsp_ptr,
    _VE_StreamObj *stream_ptr,
    vint           type)
{
    if(!stream_ptr->enc.encInited) {
        return;
    }

    switch (type) {
        case VTSP_CODER_VIDEO_H263:
            if (stream_ptr->enc.encInited) {
                H263_encShut(&stream_ptr->enc.h263EncObj);
                stream_ptr->enc.encInited = 0;
            }
            break;
        case VTSP_CODER_VIDEO_H264:
            if (stream_ptr->enc.encInited) {
                H264_encShut(&stream_ptr->enc.h264EncObj);
                stream_ptr->enc.encInited = 0;
            }
            break;

        default:
            break;

    }
}


/*
 * ======== _VE_initDecoder() ========
 *
 * Inits an encoder.
 */
void _VE_initDecoder(
    _VE_Dsp       *dsp_ptr,
    _VE_StreamObj *stream_ptr,
    vint             type)
{
    if(stream_ptr->dec.decInited) {
        _VE_TRACE(__FILE__, __LINE__);
        return;
    }

    switch (type) {
        case VTSP_CODER_VIDEO_H263:
            dsp_ptr->h263DecParams.height = dsp_ptr->pic.height;
            dsp_ptr->h263DecParams.width = dsp_ptr->pic.width;
            H263_decInit(&stream_ptr->dec.h263DecObj, &dsp_ptr->h263DecParams);
            stream_ptr->dec.decInited = 1;
            break;

        case VTSP_CODER_VIDEO_H264:
            dsp_ptr->h264DecParams.height = dsp_ptr->pic.height;
            dsp_ptr->h264DecParams.width = dsp_ptr->pic.width;
            H264_decInit(&stream_ptr->dec.h264DecObj, &dsp_ptr->h264DecParams);
            stream_ptr->dec.decInited = 1;
            break;

        default:
            break;

    }
}

/*
 * ======== _VE_shutDecoder() ========
 *
 * shuts the decoder.
 */
void _VE_shutDecoder(
    _VE_Dsp       *dsp_ptr,
    _VE_StreamObj *stream_ptr,
    vint           type)
{
    if(!stream_ptr->dec.decInited) {
        return;
    }

    switch (type) {
        case VTSP_CODER_VIDEO_H263:
            if (stream_ptr->dec.decInited) {
                H263_decShut(&stream_ptr->dec.h263DecObj);
                stream_ptr->dec.decInited = 0;
            }
            break;

        case VTSP_CODER_VIDEO_H264:
            if (stream_ptr->dec.decInited) {
                H264_decShut(&stream_ptr->dec.h264DecObj);
                stream_ptr->dec.decInited = 0;
            }
            break;

        default:
            break;

    }
}


/*
 * ======== _VE_algStateStream ========
 * Set/Reset algorithm state for stream processing
 * and initialize algorithms as necessary
 */
void _VE_algStateStream(
    _VE_Dsp   *dsp_ptr,
    uvint        infc,
    uvint        streamId,
    uint32       clearMask,
    uint32       setMask)
{
    _VE_StreamObj  *stream_ptr;
    uint32            oldState;
    uint32            newState;

    stream_ptr = _VE_streamIdToStreamPtr(dsp_ptr, infc, streamId);

    oldState = stream_ptr->algStreamState;
    newState = oldState | setMask;
    newState = newState & (~clearMask);

    if (0 != (setMask & _VE_ALG_STREAM_JB)) {
        JBV_init(&stream_ptr->dec.jbObj, &dsp_ptr->jbParams);
    }

    /*
     * Shut down coders related to stream to save power.
     */

    if (0 != (clearMask & _VE_ALG_STREAM_JB)) {
        _VE_TRACE(__FILE__, __LINE__);

        /*
         * Clear the screen as stream has ended.
         */
        _VE_shutEncoder(dsp_ptr, stream_ptr, stream_ptr->enc.lastEncoder);
        _VE_shutDecoder(dsp_ptr, stream_ptr, stream_ptr->dec.decoderType);
        VDD_clear(streamId);
        JBV_init(&stream_ptr->dec.jbObj, &dsp_ptr->jbParams);
    }


    stream_ptr->algStreamState = newState;
}

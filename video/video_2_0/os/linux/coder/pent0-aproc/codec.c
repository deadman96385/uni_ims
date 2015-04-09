/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 25157 $ $Date: 2014-03-16 09:59:26 -0700 (Sun, 16 Mar 2014) $
 */
#include <codec.h>

/*
 * ======== initialCodecs ========
 * Initialize codec context.
 *
 * Return Values:
 *    none
 */
void Codec_init(
    CODEC_Ptr codecContext_ptr)
{
    /* Do nothing...........Due to we do not know CODEC type. */
}

/*
 * ======== codecInit ========
 * Initialize a codec.
 *
 * Return Values:
 *    none
 */
void codecInit(
    CODEC_Ptr           codec_ptr,
    vint                codec,
    vint                codecType)
{
    switch (codec) {
        case CODEC_ENCODER:
            OSAL_logMsg("%s:%d start encode\n", __FUNCTION__, __LINE__);
            /* Set encoder type */
            switch (codecType) {
               case VTSP_CODER_VIDEO_H263:
                   /* H263 could support 176*144, but camera should set 320*240. */
                   codec_ptr->enc.data.width = 176;
                   codec_ptr->enc.data.height = 144;
                   codec_ptr->codecEncoder_ptr = (CodecObject_Ptr)(&_CodecH263Encoder);
                   break;
               case VTSP_CODER_VIDEO_H264:
                   codec_ptr->codecEncoder_ptr = (CodecObject_Ptr)(&_CodecH264Encoder);
                   break;
            }
            codec_ptr->codecEncoder_ptr->codecInit(codec_ptr, codecType);
            break;
        case CODEC_DECODER:
            OSAL_logMsg("%s:%d start decode\n", __FUNCTION__, __LINE__);
            /* Set decoder type */
            codec_ptr->codecDecoder_ptr = (CodecObject_Ptr)(&_CodecDecoder);
            codec_ptr->codecDecoder_ptr->codecInit(codec_ptr, codecType);
            break;
        default:
            break;
    }
}

/*
 * ======== codecRelease ========
 * Shutdown a codec.
 *
 * Return Values:
 *    none
 */
void codecRelease(
    CODEC_Ptr codec_ptr,
    vint      codec)
{
    switch (codec) {
        case CODEC_ENCODER:
            codec_ptr->codecEncoder_ptr->codecRelease(codec_ptr);
            break;
        case CODEC_DECODER:
            codec_ptr->codecDecoder_ptr->codecRelease(codec_ptr);
            break;
        default:
            break;
    }
}

/*
 * ======== codecModify ========
 * Modify a codec.
 *
 * Return Values:
 *    none
 */
void codecModify(
    CODEC_Ptr codec_ptr)
{
    codec_ptr->codecEncoder_ptr->codecModify(codec_ptr);
}

/*
 * ======== requestFIR ========
 * Request for a FIR. Only works on encoder.
 *
 * Return Values:
 *    none
 */
void requestFIR(
    CODEC_Ptr  codec_ptr)
{
    /* Do nothing...........Due to we already set sendFIR flag. */
}


/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12522 $ $Date: 2010-07-13 18:52:44 -0400 (Tue, 13 Jul 2010) $
 *
 */
#include "_ve_private.h"

/*
 * ======== _VE_videoRawToCoded() ========
 *
 * Generate encoded video block
 * by the coders.
 */
void _VE_videoRawToCoded(
    _VE_StreamObj   *stream_ptr,
    Video_Picture   *pic_ptr,
    uvint            encType,
    vint             infc)
{

    switch (encType) {


        case VTSP_CODER_VIDEO_H263:

            if ((NULL != pic_ptr->base_ptr)) {
                if (H263_enc(&stream_ptr->enc.h263EncObj, pic_ptr)) {
                    stream_ptr->enc.countEncode++;
                    stream_ptr->enc.encodePkt++;
                }
            }
            break;
        case VTSP_CODER_VIDEO_H264:

            if ((NULL != pic_ptr->base_ptr)) {
                if (H264_enc(&stream_ptr->enc.h264EncObj, pic_ptr)) {
                    stream_ptr->enc.countEncode++;
                    stream_ptr->enc.encodePkt++;
                }
            }
            break;
        default:
            _VE_TRACE(__FILE__, __LINE__);
            break;
    }
}


/*
 * ======== _VE_videoCodedToRaw() ========
 *
 * Generate raw video data
 * from coded block.
 *
 *
 */
void _VE_videoCodedToRaw(
    _VE_StreamObj   *stream_ptr,
    Video_Packet    *pkt_ptr,
    uvint             decType,
    vint             infc)
{

    /*
     * Decode
     */
    switch (decType) {

        case VTSP_CODER_VIDEO_H263:
            if (H263_dec(&stream_ptr->dec.h263DecObj, pkt_ptr)) {
                stream_ptr->dec.countDecode++;
                stream_ptr->dec.decodePkt++;
            }
            break;
        case VTSP_CODER_VIDEO_H264:
            if (H264_dec(&stream_ptr->dec.h264DecObj, pkt_ptr)) {
                stream_ptr->dec.countDecode++;
                stream_ptr->dec.decodePkt++;
            }
            break;

        case VTSP_CODER_UNAVAIL:
            /*
             * This condition will occur if pkts are received on
             * an unopened or unconfigured stream.
             */
            /* fall through to default */
            _VE_TRACE(__FILE__, __LINE__);
        default:
            _VE_TRACE(__FILE__, __LINE__);
            break;
    }
}

/*
 * ======== _VE_videoGetCodedData() ========
 *
 * Get encoded data.
 * Returns:
 * 1 : Data available
 * 0 : Data not available
 *
 */
vint _VE_videoGetCodedData(
    _VE_StreamObj   *stream_ptr,
    Video_Packet    *pkt_ptr,
    uvint            encType,
    vint             infc)
{

    switch (encType) {


        case VTSP_CODER_VIDEO_H263:
            return(H263_encGetData(&stream_ptr->enc.h263EncObj, pkt_ptr));

        case VTSP_CODER_VIDEO_H264:
            return(H264_encGetData(&stream_ptr->enc.h264EncObj, pkt_ptr));

        default:
            _VE_TRACE(__FILE__, __LINE__);
            break;
    }

    return (0);
}


/*
 * ======== _VE_videoGetRawData() ========
 *
 * Get decoded data.
 * Returns:
 * 1 : Data available
 * 0 : Data not available
 *
 */
vint _VE_videoGetRawData(
    _VE_StreamObj   *stream_ptr,
    Video_Picture   *pic_ptr,
    uvint            decType,
    vint             infc)
{
    switch (decType) {

        case VTSP_CODER_VIDEO_H263:
            return(H263_decGetData(&stream_ptr->dec.h263DecObj, pic_ptr));

        case VTSP_CODER_VIDEO_H264:
            return(H264_decGetData(&stream_ptr->dec.h264DecObj, pic_ptr));

        default:
            _VE_TRACE(__FILE__, __LINE__);
            break;
    }

    return (0);

}

/*
 * ======== _VE_videoRequestKeyFrame() ========
 *
 * Request a key frame generation. (New sequence)
 * Returns:
 * 1 : Request OK
 * 0 : Request failed
 *
 */
vint _VE_videoRequestKeyFrame(
    _VE_StreamObj   *stream_ptr,
    uvint            encType,
    vint             infc)
{
    switch (encType) {

        case VTSP_CODER_VIDEO_H263:
            _VE_TRACE(__FILE__, __LINE__);
            break;

        case VTSP_CODER_VIDEO_H264:
            if (0 == H264_encRequestKeyFrame(
                    &stream_ptr->enc.h264EncObj)) {
                return (1);
            }

        default:
            _VE_TRACE(__FILE__, __LINE__);
            break;
    }

    return (0);
}


/*
 * ======== _VE_videoRequestResolutionChange() ========
 *
 * Request change of resolution.
 * Returns:
 * 1 : Request OK
 * 0 : Request failed
 *
 */
vint _VE_videoRequestResolutionChange(
    _VE_StreamObj   *stream_ptr,
    uvint            encType,
    vint             width,
    vint             height,
    vint             infc)
{
    switch (encType) {

        case VTSP_CODER_VIDEO_H263:
            if (0 == H263_encRequestResolution(
                    &stream_ptr->enc.h263EncObj, width, height)) {
                return (1);
            }
            break;

        case VTSP_CODER_VIDEO_H264:
            if (0 == H264_encRequestResolution(
                    &stream_ptr->enc.h264EncObj, width, height)) {
                return (1);
            }

        default:
            _VE_TRACE(__FILE__, __LINE__);
            break;
    }

    return (0);
}

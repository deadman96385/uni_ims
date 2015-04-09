/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12522 $ $Date: 2010-07-13 18:52:44 -0400 (Tue, 13 Jul 2010) $
 *
 */
#include "_vc_private.h"

/*
 * ======== _VC_videoGetCodedData() ========
 *
 * Get encoded data.
 * Returns:
 * 1 : Data available
 * 0 : Data not available
 *
 */
vint _VC_videoGetCodedData(
    _VC_StreamObj   *stream_ptr,
    Video_Packet    *pkt_ptr,
    uint8           *data_ptr,
    vint            length,
    uint64          tsMs,
    uvint           encType)
{
    switch (encType) {
        case VTSP_CODER_VIDEO_H263:
            return(H263_encGetData(&stream_ptr->enc.VideoEncObj, pkt_ptr, data_ptr, length, tsMs));

        case VTSP_CODER_VIDEO_H264:
            return(H264_encGetData(&stream_ptr->enc.VideoEncObj, pkt_ptr, data_ptr, length, tsMs));

        default:
            _VC_TRACE(__FILE__, __LINE__);
            break;
    }

    return (0);
}

/*
 * ======== _VC_videoRequestKeyFrame() ========
 *
 * Request a key frame generation. (New sequence)
 * Returns:
 * 1 : Request OK
 * 0 : Request failed
 *
 */
vint _VC_videoRequestKeyFrame(
    _VC_StreamObj   *stream_ptr,
    uvint            encType,
    vint             infc)
{
    switch (encType) {

        case VTSP_CODER_VIDEO_H263:
            _VC_TRACE(__FILE__, __LINE__);
            break;

        case VTSP_CODER_VIDEO_H264:
//            if (0 == H264_encRequestKeyFrame(
//                    &stream_ptr->enc.h264EncObj)) {
//                return (1);
//            }

        default:
            _VC_TRACE(__FILE__, __LINE__);
            break;
    }

    return (0);
}

/*
 * ======== _VC_videoRequestResolutionChange() ========
 *
 * Request change of resolution.
 * Returns:
 * 1 : Request OK
 * 0 : Request failed
 *
 */
vint _VC_videoRequestResolutionChange(
    _VC_StreamObj   *stream_ptr,
    uvint            encType,
    vint             width,
    vint             height,
    vint             infc)
{
    switch (encType) {

        case VTSP_CODER_VIDEO_H263:
//            if (0 == H263_encRequestResolution(
//                    &stream_ptr->enc.h263EncObj, width, height)) {
//                return (1);
//            }
            break;

        case VTSP_CODER_VIDEO_H264:
//            if (0 == H264_encRequestResolution(
//                    &stream_ptr->enc.h264EncObj, width, height)) {
//                return (1);
//            }

        default:
            _VC_TRACE(__FILE__, __LINE__);
            break;
    }

    return (0);
}

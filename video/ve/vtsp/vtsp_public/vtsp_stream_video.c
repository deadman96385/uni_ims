/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2008 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29711 $ $Date: 2014-11-06 12:42:22 +0800 (Thu, 06 Nov 2014) $
 *
 */

#include "osal.h"
#include "vtsp.h"
#include "../vtsp_private/_vtsp_private.h"

/*
 * ======== VTSP_streamVideoStart ========
 */
VTSP_Return VTSP_streamVideoStart(
    uvint             infc,
    VTSP_StreamVideo *stream_ptr)
{
    VTSP_Return      e;
    _VTSP_CmdMsg     cmd;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;

    if (VTSP_INFC_VIDEO != infc) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (VTSP_E_INFC);
    }

    if (NULL == stream_ptr) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (VTSP_E_STREAM_DATA);
    }

    /* check if streamId is in valid range
     */
    if (VTSP_OK != (e = _VTSP_isStreamVideoIdValid(infc, stream_ptr->streamId))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    /* check if dir is valid
     */
    if (VTSP_OK != (e = _VTSP_isStreamVideoDirValid(infc, stream_ptr->streamId, 
                    stream_ptr->dir))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    if (VTSP_OK != (e = _VTSP_isStreamVideoPeerValid(infc, stream_ptr))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    if (VTSP_OK != (e = _VTSP_isStreamVideoConfMaskValid(infc, 
                    stream_ptr->streamId, stream_ptr->confMask))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    /* update to ipc */
    cmd.code = _VTSP_CMD_STREAM_VIDEO_START;
    cmd.infc = infc;
    OSAL_memCpy(&cmd.msg.streamVideo, stream_ptr, sizeof(VTSP_StreamVideo));

    _VTSP_putCmd(infc, &cmd, 1);
    return (VTSP_OK);
}

/*
 * ======== VTSP_streamVideoEnd ========
 */
VTSP_Return VTSP_streamVideoEnd(
    uvint           infc,
    uvint           streamId)
{
    VTSP_Return      e;
    _VTSP_CmdMsg     cmd;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;
    if (VTSP_INFC_VIDEO != infc) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (VTSP_E_INFC);
    }

    /* check if streamId is in valid range
     */
    if (VTSP_OK != (e = _VTSP_isStreamVideoIdValid(infc, streamId))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }


    /* Send cmd to deactivate stream
     */
    cmd.code = _VTSP_CMD_STREAM_VIDEO_END;
    cmd.infc = infc;
    cmd.msg.arg.arg0 = streamId;

    _VTSP_putCmd(infc, &cmd, 1);
    return (VTSP_OK);
}

/*
 * ======== VTSP_streamVideoModify ========
 */
VTSP_Return VTSP_streamVideoModify(
    uvint                 infc,
    VTSP_StreamVideo     *stream_ptr)
{

    VTSP_Return      e;
    _VTSP_CmdMsg     cmd;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;
    if (VTSP_INFC_VIDEO != infc) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (VTSP_E_INFC);
    }
    if (NULL == stream_ptr) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (VTSP_E_STREAM_DATA);
    }

    /* check if streamId is in valid range
     */
    if (VTSP_OK != (e = _VTSP_isStreamVideoIdValid(infc, stream_ptr->streamId))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    /* check if dir is valid
     */
    if (VTSP_OK != (e = _VTSP_isStreamVideoDirValid(infc, stream_ptr->streamId, 
                    stream_ptr->dir))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    /* update to ipc */
    cmd.code = _VTSP_CMD_STREAM_VIDEO_MODIFY;
    cmd.infc = infc;
    OSAL_memCpy(&cmd.msg.streamVideo, stream_ptr, sizeof(VTSP_StreamVideo));

    _VTSP_putCmd(infc, &cmd, 1);

    return (VTSP_OK);
}


/*
 * ======== VTSP_streamVideoModifyDir ========
 */
VTSP_Return VTSP_streamVideoModifyDir(
    uvint           infc,
    uvint           streamId,
    VTSP_StreamDir  dir)
{

    VTSP_Return      e;
    _VTSP_CmdMsg     cmd;


    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;
    if (VTSP_INFC_VIDEO != infc) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (VTSP_E_INFC);
    }

    /* check if streamId is in valid range
     */
    if (VTSP_OK != (e = _VTSP_isStreamVideoIdValid(infc, streamId))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    /* check if dir is valid
     */
    if (VTSP_OK != (e = _VTSP_isStreamVideoDirValid(infc, streamId, dir))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }


    /* Send cmd to modify stream dir 
     */
    cmd.code = _VTSP_CMD_STREAM_VIDEO_MODIFY_DIR;
    cmd.infc = infc;
    cmd.msg.arg.arg0 = streamId;
    cmd.msg.arg.arg1 = dir;

    _VTSP_putCmd(infc, &cmd, 1);
    return (VTSP_OK);
}

/*
 * ======== VTSP_streamVideoRequestResolution ========
 */
VTSP_Return VTSP_streamVideoRequestResolution(
    uvint           infc,
    uvint           streamId,
    vint            width,
    vint            height)
{

    VTSP_Return      e;
    _VTSP_CmdMsg     cmd;


    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;
    if (VTSP_INFC_VIDEO != infc) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (VTSP_E_INFC);
    }

    /* check if streamId is in valid range
     */
    if (VTSP_OK != (e = _VTSP_isStreamVideoIdValid(infc, streamId))) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }


    /* Send cmd to modify stream
     */
    cmd.code = _VTSP_CMD_STREAM_VIDEO_REQUEST_RESOLUTION;
    cmd.infc = infc;
    cmd.msg.arg.arg0 = streamId;
    cmd.msg.arg.arg1 = width;
    cmd.msg.arg.arg2 = height;

    _VTSP_putCmd(infc, &cmd, 1);
    return (VTSP_OK);
}

/*
 * ======== VTSP_streamVideoRequestKeyFrame ========
 */
VTSP_Return VTSP_streamVideoRequestKeyFrame(
    uvint           infc,
    uvint           streamId)
{

    VTSP_Return      e;
    _VTSP_CmdMsg     cmd;


    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;
    if (VTSP_INFC_VIDEO != infc) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (VTSP_E_INFC);
    }

    /* check if streamId is in valid range
     */
    if (VTSP_OK != (e = _VTSP_isStreamVideoIdValid(infc, streamId))) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    /* Send cmd to modify stream dir
     */
    cmd.code = _VTSP_CMD_STREAM_VIDEO_REQUEST_KEY;
    cmd.infc = infc;
    cmd.msg.streamVideo.streamId = streamId;

    _VTSP_putCmd(infc, &cmd, 1);
    return (VTSP_OK);
}

/*
 * ======== VTSP_streamVideoModifyEncoder ========
 *
 */
VTSP_Return VTSP_streamVideoModifyEncoder(
    uvint            infc,
    uvint            streamId,
    uvint            encoder)
{

    VTSP_Return      e;
    _VTSP_CmdMsg     cmd;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;

    if (VTSP_INFC_VIDEO != infc) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (VTSP_E_INFC);
    }

    /* check if streamId is in valid range
     */
    if (VTSP_OK != (e = _VTSP_isStreamVideoIdValid(infc, streamId))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    /* Send cmd to modify stream coder
     */
    cmd.code = _VTSP_CMD_STREAM_VIDEO_MODIFY_ENCODER;
    cmd.infc = infc;
    cmd.msg.arg.arg0 = streamId;
    cmd.msg.arg.arg1 = encoder;

    _VTSP_putCmd(infc, &cmd, 1);

    return (VTSP_OK);
}

/*
 * ======== VTSP_streamModifyConf ========
 */

VTSP_Return VTSP_streamVideoModifyConf(
    uvint           infc,
    uvint           streamId,
    uint32          confMask)
{

    VTSP_Return      e;
    _VTSP_CmdMsg     cmd;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;

    if (VTSP_INFC_VIDEO != infc) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (VTSP_E_INFC);
    }

    /* check if streamId is in valid range
     */
    if (VTSP_OK != (e = _VTSP_isStreamVideoIdValid(infc, streamId))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }


    /* check if mask is valid
     */
    if (VTSP_OK != (e = _VTSP_isStreamVideoConfMaskValid(infc, 
                    streamId, confMask))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    /* Send cmd to modify stream confMask
     */
    cmd.code = _VTSP_CMD_STREAM_VIDEO_MODIFY_CONFMASK;
    cmd.infc = infc;
    cmd.msg.arg.arg0 = streamId;
    cmd.msg.arg.arg1 = confMask;

    _VTSP_putCmd(infc, &cmd, 1);

    return (VTSP_OK);
}

/*
 * ======== VTSP_streamVideoQuery ========
 */
VTSP_Return VTSP_streamVideoQuery(
    uvint           infc,
    uvint           streamId)
{

    VTSP_Return      e;
    _VTSP_CmdMsg     cmd;


    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;
    if (VTSP_INFC_VIDEO != infc) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (VTSP_E_INFC);
    }

    /* check if streamId is in valid range
     */
    if (VTSP_OK != (e = _VTSP_isStreamVideoIdValid(infc, streamId))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    /* Send cmd as stream query
     */
    cmd.code = _VTSP_CMD_STREAM_VIDEO_QUERY;
    cmd.infc = infc;
    cmd.msg.arg.arg0 = streamId;

    _VTSP_putCmd(infc, &cmd, 1);
    return (VTSP_OK);
}

/*
 * ======== VTSP_streamVideoLipSync ========
 *
 * audioVideoSkew : A signed integer that indicates Audio video skew.
 *     audioVideoSkew = aLatency - vLatency (in milliseconds)
 * the value is +ve if audio is lagging,
 *         this means we have to slow down video play back.
 * the value is -ve if audio is leading,
 *         this means we have to increase video play back.
 */
VTSP_Return VTSP_streamVideoLipSync(
    uvint           infc,
    uvint           streamId,
    vint            audioVideoSkew)
{
    VTSP_Return      e;
    _VTSP_CmdMsg     cmd;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;
    if (VTSP_INFC_VIDEO != infc) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (VTSP_E_INFC);
    }

    /* check if streamId is in valid range
    */
    if (VTSP_OK != (e = _VTSP_isStreamVideoIdValid(infc, streamId))) {
       _VTSP_TRACE(__FILE__, __LINE__);
       return (e);
    }

    /* 
     * Send cmd as stream query
     */
    cmd.code = _VTSP_CMD_STREAM_VIDEO_SYNC;
    cmd.infc = infc;
    cmd.msg.arg.arg0 = streamId;
    cmd.msg.arg.arg1 = audioVideoSkew;

    _VTSP_putCmd(infc, &cmd, 1);
    return (VTSP_OK);
}

/*
 * ======== VTSP_rtcpCnameVideo() ========
 */
VTSP_Return VTSP_rtcpCnameVideo(
    uvint       infc,
    const char *name_ptr)
{
    _VTSP_CmdMsg cmd;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;
    if (VTSP_INFC_VIDEO != infc) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (VTSP_E_INFC);
    }

    /*
     * Send the CNAME to VTSPR.
     */
    cmd.code = _VTSP_CMD_RTCP_CNAME;
    cmd.infc = infc;
    OSAL_strncpy((char *)(cmd.msg.cname.cname), name_ptr, _VTSP_RTCP_CNAME_CHARS);

    _VTSP_putCmd(infc, &cmd, 1);

    return (VTSP_OK);
}

/*
 * ======== VTSP_rtcpFeedbackMaskVideo() ========
 */
VTSP_Return VTSP_rtcpFeedbackMaskVideo(
    uvint                infc,
    VTSP_RtcpTemplate   *rtcpConfig_ptr)
{
    _VTSP_CmdMsg cmd;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;
    if (VTSP_INFC_VIDEO != infc) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (VTSP_E_INFC);
    }

    cmd.code = _VTSP_CMD_CONFIG;
    cmd.msg.config.templCode = VTSP_TEMPL_CODE_RTCP;
    cmd.infc = infc;

    cmd.msg.config.u.data[0] = rtcpConfig_ptr->control;
    cmd.msg.config.u.data[1] = rtcpConfig_ptr->streamId;
    cmd.msg.config.u.data[2] = rtcpConfig_ptr->mask;

    _VTSP_putCmd(infc, &cmd, 1);

    return (VTSP_OK);
}

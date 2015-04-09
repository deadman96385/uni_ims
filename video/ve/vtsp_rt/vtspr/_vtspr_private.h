/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006-2008 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30131 $ $Date: 2014-12-01 14:45:03 +0800 (Mon, 01 Dec 2014) $
 */

#ifndef __VTSPR_PRIVATE_H_
#define __VTSPR_PRIVATE_H_

/*
 * Private Externs
 * --------
 */

extern vint           _VTSPR_toneDtmfFreq[16][2];
#ifndef VTSP_ENABLE_MP_LITE
extern TONE_Params    _VTSPR_toneDtmf;
extern TONE_Params    _VTSPR_toneVoipDialtone;
extern TONE_Params    _VTSPR_toneVoipBusy;
extern TONE_Params    _VTSPR_toneVoipRingback;
extern TONE_Params    _VTSPR_toneVoipReorder;
extern TONE_Params    _VTSPR_toneVoipCWAlert;
extern TONE_Params    _VTSPR_toneTest0;
extern TONE_Params    _VTSPR_toneDtmf;
#endif
extern GLOBAL_Params  _VTSPR_globals;

/*
 * Private functions
 * --------
 */
void _VTSPR_defaults(
    VTSPR_DSP *dsp_ptr);

void _VTSPR_cidSetCountryCode(
    VTSPR_ChanObj *chan_ptr,
    vint           infc,
    uvint          code);

vint _VTSPR_printInfo(void);

/* 
 * _vtspr_netlog.c
 */
void _VTSPR_netlogSend(
    VTSPR_Obj  *vtspr_ptr,
    vint        bufId,
    void       *buf_ptr,
    vint        bufLen,
    vint        infc);

void _VTSPR_netlogInit(
    VTSPR_Obj *vtspr_ptr);

/* _vtspr_map.c
 */
_VTSPR_RtcpObject *_VTSPR_streamIdToRtcpPtr(
    VTSPR_NetObj *net_ptr,
    uvint         infc,
    uvint         streamId);
_VTSPR_RtpObject *_VTSPR_streamIdToRtpPtr(
    VTSPR_NetObj *net_ptr,
    uvint         infc,
    uvint         streamId);
uvint _VTSPR_jbCoderToLocalCoder(
    JB_Payload coder,
    vint       payloadSize);
JB_Payload _VTSPR_localCoderToJbCoder(
    vint coder);
uvint _VTSPR_localToDynamicEncoder(
    VTSP_Stream *streamParam_ptr,
    vint         local);
uvint _VTSPR_dynamicToLocalDecoder(
    VTSP_Stream *streamParam_ptr,
    vint         dynamic);
uvint _VTSPR_infcToChan(
    uvint infc);
void *_VTSPR_infcToChanPtr(
    VTSPR_DSP  *dsp_ptr,
    uvint       infc);
void *_VTSPR_infcToInfcPtr(
    VTSPR_DSP  *dsp_ptr,
    uvint       infc);
void *_VTSPR_infcToTicPtr(
    VTSPR_DSP  *dsp_ptr,
    uvint       infc);
vint _VTSPR_infcToType(
    uvint infc);
vint _VTSPR_charToDtmf(
    unsigned char c);
unsigned char _VTSPR_dtmfToChar(
    uvint dig);
VTSPR_StreamObj *_VTSPR_streamIdToStreamPtr(
    VTSPR_DSP *dsp_ptr,
    vint       infc,
    vint       streamId);
_VTSPR_FlowObj *_VTSPR_streamIdToFlowPtr(
    VTSPR_DSP *dsp_ptr,
    vint       infc,
    vint       streamId);
_VTSPR_MultiEncObj *_VTSPR_streamIdToMultiEncPtr(
    VTSPR_DSP *dsp_ptr,
    vint       infc,
    vint       streamId);
OSAL_MsgQId _VTSPR_streamIdToMultiEncQId(
    VTSPR_Queues *q_ptr,
    vint          infc,
    vint          streamId,
    vint          pktRate);
_VTSPR_MultiDecObj *_VTSPR_streamIdToMultiDecPtr(
    VTSPR_DSP    *dsp_ptr,
    vint          infc,
    vint          streamId);
OSAL_MsgQId _VTSPR_streamIdToMultiDecQId(
    VTSPR_Queues *q_ptr,
    vint          infc,
    vint          streamId,
    vint          pktRate);
_VTSPR_RtcpObject *_VTSPR_streamIdToRtcpPtr(
    VTSPR_NetObj *net_ptr,
    uvint         infc,
    uvint         streamId);
_VTSPR_RtpObject *_VTSPR_streamIdToRtpPtr(
    VTSPR_NetObj *net_ptr,
    uvint         infc,
    uvint         streamId);
vint _VTSPR_infcToVadStatus(
    VTSPR_ChanObj *chan_ptr,
    vint           infc);
OSAL_Boolean _VTSPR_isCoderWb(
    uvint      encType);
OSAL_Boolean _VTSPR_isCoderPassthrough(
    uvint      encType);
OSAL_Boolean _VTSPR_isPacketCn(
    vint coder,
    vint bytes);
vint _VTSPR_get10msSize(
    vint coder);
TIC_ControlCode _VTSPR_infcLineToTicCode(
    VTSP_ControlInfcLine code);
TIC_ControlCode _VTSPR_infcIOToTicCode(
    VTSP_ControlInfcIO code);
vint _VTSPR_infcToTicInfcType(
    uvint infc);

/* _vtspr_default.c
 */
void _VTSPR_setObj(
    VTSPR_DSP *dsp_ptr);

/* _vtspr_cmd.c
 */
void _VTSPR_runDnCmd(
    VTSPR_Obj     *vtspr_ptr,
    VTSPR_Queues  *q_ptr,
    VTSPR_DSP     *dsp_ptr,
    _VTSP_CmdMsg  *cmd_ptr);

/* 
 * _vtspr_cmd_tone.c
 */
VTSP_Return _VTSPR_updateToneSeq(
    VTSPR_DSP           *dsp_ptr,
    VTSPR_ToneSeq       *toneSeq_ptr,
    uvint               *toneList_ptr,
    vint                 tId,
    uint32               maxTime,
    vint                 numToneIds,
    uint32               control,
    uint32               repeat,
    uvint                toneType);

/* vtspr_send.c
 */
void VTSPR_sendEvent(
    VTSPR_Queues   *q_ptr,
    VTSP_EventMsg  *msg_ptr,
    uvint           infc);

void _VTSPR_LipSync_rtcpRecv(
    uvint       infc,
    VTSPR_Queues      *q_ptr,
    _VTSPR_RtpObject   *rtp_ptr);

void _VTSPR_LipSync_rtpTs(
    uvint           infc,
    VTSPR_Queues    *q_ptr,
    JB_Pkt          *pkt_ptr);

/* vtspr_recv.c
*/
void _VTSPR_recvAllCmd(
    VTSPR_Obj     *vtspr_ptr,
    VTSPR_Queues  *q_ptr,
    VTSPR_DSP     *dsp_ptr);

/* vtspr_state.c
*/
void _VTSPR_utdTranslate(
    VTSPR_DSP        *dsp_ptr);

uint32 _VTSPR_getAlgStateChan(
    VTSPR_ChanObj *chan_ptr,
    uvint          infc);

void _VTSPR_algStateChan(
    VTSPR_DSP        *dsp_ptr,
    vint              infc,
    uint32            clearMask,
    uint32            setMask);

void _VTSPR_algStateStream(
    VTSPR_DSP   *dsp_ptr,
    uvint        infc,
    uvint        streamId,
    uint32       clearMask,
    uint32       setMask);

void _VTSPR_initCoder(
    VTSPR_DSP       *dsp_ptr,
    VTSPR_StreamObj *stream_ptr,
    vint             encDec,
    vint             type);

/* vtspr_event.c
 */
void _VTSPR_genEventHs(
    VTSPR_Obj     *vtspr_ptr,
    VTSPR_Queues  *q_ptr,
    VTSPR_DSP     *dsp_ptr);
void _VTSPR_genEventFx(
    VTSPR_Obj     *vtspr_ptr,
    VTSPR_Queues  *q_ptr,
    VTSPR_DSP     *dsp_ptr);
void _VTSPR_genEventFxs(
    VTSPR_Obj     *vtspr_ptr,
    VTSPR_Queues  *q_ptr,
    VTSPR_DSP     *dsp_ptr);
void _VTSPR_genEventFxo(
    VTSPR_Obj     *vtspr_ptr,
    VTSPR_Queues  *q_ptr,
    VTSPR_DSP     *dsp_ptr);
void _VTSPR_genEventStreamQuery(
    VTSPR_Obj    *vtspr_ptr,
    VTSPR_Queues *q_ptr,
    VTSPR_DSP    *dsp_ptr,
    vint          infc,
    vint          streamId);
void _VTSPR_genEventTone(
    VTSPR_DSP       *dsp_ptr,
    VTSPR_Queues    *q_ptr,
    vint             infc,
    VTSPR_StreamObj *stream_ptr,
    VTSPR_ToneSeq   *tone_ptr);
void _VTSPR_genEventStream(
    VTSPR_Obj    *vtspr_ptr,
    VTSPR_Queues *q_ptr,
    VTSPR_DSP    *dsp_ptr);

/*
 * _vtspr_audio.c
 */
void _VTSPR_audioGain(
    vint       *audio_ptr,
    int32       gain,
    vint        nsamp);
void _VTSPR_audioRxFormatHs(
    VTSPR_Obj   *vtspr_ptr,
    VTSPR_DSP   *dsp_ptr);
void _VTSPR_audioRxFormat(
    VTSPR_Obj   *vtspr_ptr,
    VTSPR_DSP   *dsp_ptr);
void _VTSPR_audioRemoveEchoFx(
    VTSPR_Obj   *vtspr_ptr,
    VTSPR_DSP   *dsp_ptr);
void _VTSPR_audioRemoveEchoHs(
    VTSPR_Obj   *vtspr_ptr,
    VTSPR_DSP   *dsp_ptr);
void _VTSPR_audioPreFilter(
    VTSPR_DSP   *dsp_ptr);
void _VTSPR_audioNear(
    VTSPR_Obj   *vtspr_ptr,
    VTSPR_DSP   *dsp_ptr);
void _VTSPR_audioTxDcrm(
    VTSPR_Obj   *vtspr_ptr,
    VTSPR_DSP   *dsp_ptr);
void _VTSPR_audioTxFormat(
    VTSPR_Obj   *vtspr_ptr,
    VTSPR_DSP   *dsp_ptr);
void _VTSPR_audioTxFormatHs(
    VTSPR_Obj   *vtspr_ptr,
    VTSPR_DSP   *dsp_ptr);
void _VTSPR_audioRoutFx(
    VTSPR_DSP   *dsp_ptr);
void _VTSPR_audioRoutHs(
    VTSPR_DSP   *dsp_ptr);

/* _vtspr_audio_tone.c
 */
void _VTSPR_audioToneSeq(
    VTSPR_DSP        *dsp_ptr,
    vint              infc,
    VTSPR_StreamObj  *stream_ptr,
    VTSPR_ToneSeq    *tone_ptr,
    uvint             control);
void _VTSPR_audioNearTone(
    VTSPR_DSP        *dsp_ptr,
    vint              infc,
    VTSPR_ChanObj  *chan_ptr);
void _VTSPR_audioPeerTone(
    VTSPR_DSP        *dsp_ptr,
    vint              infc,
    VTSPR_StreamObj  *stream_ptr);

/*
 * _vtspr_t38.c
 */
#ifdef VTSP_ENABLE_T38
void _VTSPR_processT38EventLog(
        vint infc,
        VTSPR_Obj    *vtspr_ptr,
        VTSPR_Queues *q_ptr,
        VTSPR_DSP    *dsp_ptr);
vint _VTSPR_processT38(
    VTSPR_Obj    *vtspr_ptr,
    VTSPR_DSP    *dsp_ptr);
#endif

/*
 * _vtspr_conf.c
 */
void _VTSPR_audioConfToLocal(
    VTSPR_Obj    *vtspr_ptr,
    VTSPR_DSP    *dsp_ptr);
void _VTSPR_audioConfToPeer(
    VTSPR_Obj    *vtspr_ptr,
    VTSPR_Queues *q_ptr,
    VTSPR_DSP    *dsp_ptr);

/* 
 * _vtspr_coder.c
 */
uvint _VTSPR_audioLinearToCoded(
    VTSPR_StreamObj   *stream_ptr,
    vint              *src_ptr,
    vint              *dst_ptr,
    uvint              encType,
    uvint              voiceActive,
#ifdef VTSP_ENABLE_DTMFR
    DR_EventObj       *drEvent_ptr,
#endif
#if !defined(VTSP_ENABLE_MP_LITE) || defined(VTSP_ENABLE_NFE) || defined(VTSP_ENABLE_AEC)
    vint               noiseFloor,
#endif
    vint               infc);

void _VTSPR_audioCodedToLinear(
    VTSPR_Obj         *vtspr_ptr,
    VTSPR_StreamObj   *stream_ptr,
    vint              *decIn_ptr,
    vint              *decOut_ptr,
    vint               pSize,
    vint               decType,
#ifndef VTSP_ENABLE_MP_LITE
    PLC_Obj           *plc_ptr,
#endif
    vint               infc);

/*
 * _vtspr_multi_decode.c
 */
vint _VTSPR_multiDecode(
    vint               *src_ptr,
    vint               *dst_ptr,
    vint                payloadSize,
    VTSPR_StreamObj    *stream_ptr,
    vint                decType,
    vint                infc);

void _VTSPR_multiDecodeBuffer10(
    vint               *dst_ptr,
    vint               *decodedSpeech_ptr,
    _VTSPR_MultiDecObj *multiDecObj_ptr);

void _VTSPR_multiDecodeBuffer20(
    vint               *dst_ptr,
    vint               *decodedSpeech_ptr,
    _VTSPR_MultiDecObj *multiDecObj_ptr);

void _VTSPR_multiDecodeBuffer30(
    vint               *dst_ptr,
    vint               *decodedSpeech_ptr,
    _VTSPR_MultiDecObj *multiDecObj_ptr);

void _VTSPR_multiDecodeMsgToTask(
    vint               *src_ptr,
    _VTSPR_MultiDecObj *MultiDecObj_ptr,
    VTSP_Stream        *streamParam,
    vint                payloadSize,
    vint                decType,
    vint                infc);

vint _VTSPR_multiDecodeMsgFromTask(
    _VTSPR_MultiDecObj *MultiDecObj_ptr,
    vint               *dst_ptr);

/*
 * _vtspr_multi_decode_task.c
 */
vint _VTSPR_multiDecode10Task(
        OSAL_TaskArg taskArg);
vint _VTSPR_multiDecode20Task(
        OSAL_TaskArg taskArg);
vint _VTSPR_multiDecode30Task(
        OSAL_TaskArg taskArg);

/*
 * _vtspr_multi_encode.c
 */
vint _VTSPR_multiEncode(
    vint            *src_ptr,
    vint            *dst_ptr,
    VTSPR_StreamObj *stream_ptr,
    vint             encType,
    vint             infc);

vint _VTSPR_multiEncodeBuffer10(
    vint               *src_ptr,
    vint               *dst_ptr,
    _VTSPR_MultiEncObj *MultiEncObj_ptr,
    VTSPR_StreamObj    *stream_ptr);  

vint _VTSPR_multiEncodeBuffer20(
    vint               *src_ptr,
    vint               *dst_ptr,
    _VTSPR_MultiEncObj *MultiEncObj_ptr,
    VTSPR_StreamObj    *stream_ptr);  

vint _VTSPR_multiEncodeBuffer30(
    vint               *src_ptr,
    vint               *dst_ptr,
    _VTSPR_MultiEncObj *MultiEncObj_ptr,
    VTSPR_StreamObj    *stream_ptr,  
    vint                encType);

void _VTSPR_multiEncodeMsgToTask(
    _VTSPR_MultiEncObj *MultiEncObj_ptr,
    VTSP_Stream        *streamParam,
    vint                encType,
    vint                infc);

vint _VTSPR_multiEncodeMsgFromTask(
    _VTSPR_MultiEncObj *MultiEncObj_ptr,
    vint               *dst_ptr);

/*
 * _vtspr_multi_coder_init.c
 */
vint _VTSPR_multiEncodeTaskInit(
        VTSPR_Obj *vtspr_ptr);

vint _VTSPR_multiDecodeTaskInit(
        VTSPR_Obj *vtspr_ptr);

vint _VTSPR_multiEncodeInit(
    VTSPR_Obj      *vtspr_ptr,
    VTSPR_Queues   *q_ptr,
    VTSPR_DSP      *dsp_ptr);  

vint _VTSPR_multiDecodeInit(
    VTSPR_Obj      *vtspr_ptr,
    VTSPR_Queues   *q_ptr,
    VTSPR_DSP      *dsp_ptr);  

void _VTSPR_multiCoderInit(
    VTSPR_StreamObj *stream_ptr,
    vint             encDec,
    vint             coderType);
/*
 * _vtspr_multi_encode_task.c
 */
vint _VTSPR_multiEncode10Task(
        OSAL_TaskArg taskArg);
vint _VTSPR_multiEncode20Task(
        OSAL_TaskArg taskArg);
vint _VTSPR_multiEncode30Task(
        OSAL_TaskArg taskArg);

/* _vtspr_stream.c
 */
void _VTSPR_setBlockHdr(
    VTSPR_StreamObj *stream_ptr,
    VTSP_BlockHeader *hdr_ptr,
    uvint             encType);
void _VTSPR_audioStreamDecode(
    VTSPR_Obj    *vtspr_ptr,
    VTSPR_Queues *q_ptr,
    VTSPR_DSP    *dsp_ptr);
void _VTSPR_audioStreamEncode(
    VTSPR_Obj          *vtspr_ptr,
    VTSPR_DSP          *dsp_ptr,
    vint                infc,
    VTSPR_StreamObj    *stream_ptr);

/*
 * vtspr_cid.c
 */
void _VTSPR_callerIdSend(
    VTSPR_Obj    *vtspr_ptr,
    VTSPR_Queues *q_ptr,
    VTSPR_DSP    *dsp_ptr);

/* _vtspr_time.c
 */
void _VTSPR_time(
    VTSPR_Obj    *vtspr_ptr,
    VTSPR_Queues *q_ptr,
    VTSPR_DSP    *dsp_ptr);

void _VTSPR_timeMeasure(
    VTSPR_Obj    *vtspr_ptr,
    VTSPR_Queues *q_ptr,
    VTSPR_DSP    *dsp_ptr);
uint32 VTSPR_getTime(
    VTSPR_DSP   *dsp_ptr);
/*
 * _vtsp_flow.c
 */
void _VTSPR_flowAbortFlush(
    VTSPR_DSP       *dsp_ptr,
    _VTSPR_FlowObj  *flow_ptr);

void _VTSPR_flowAbortRec(
    VTSPR_Obj       *vtspr_ptr,
    VTSPR_Queues    *q_ptr,
    _VTSPR_FlowObj  *flow_ptr,
    VTSPR_StreamObj *stream_ptr);

void _VTSPR_flowOpenFlush(
    _VTSPR_FlowObj  *flow_ptr,
    VTSPR_StreamObj *stream_ptr);

vint _VTSPR_flowPlayNext(
    VTSPR_DSP        *dsp_ptr,
    VTSPR_Queues     *q_ptr,
    _VTSPR_FlowObj   *flow_ptr,
    VTSPR_StreamObj  *stream_ptr,
    void            **data_ptr);

vint _VTSPR_flowRecord(
    VTSPR_DSP        *dsp_ptr,
    VTSPR_Queues     *q_ptr,
    _VTSPR_FlowObj  *flow_ptr,
    VTSPR_StreamObj *stream_ptr);

#ifdef    VTSP_ENABLE_BENCHMARK
/*
 * _vtsp_benchmark.c
 */
void _VTSPR_benchmarkCompute(
    VTSPR_Obj *vtspr_ptr);

void _VTSPR_benchmarkStart(
    VTSPR_Obj *vtspr_ptr,
    vint       component,
    vint       option);

void _VTSPR_benchmarkStop(
    VTSPR_Obj *vtspr_ptr,
    vint       component,
    vint       option);
#endif

#if defined(VTSP_ENABLE_STREAM_16K) || defined(VTSP_ENABLE_AUDIO_SRC)
#ifndef VTSP_ENABLE_MP_LITE
/*
 * _vtsp_rate.c
 */
void _VTSPR_upSample(
        UDS_Obj *udsObj_ptr,
        vint    *dst_ptr,
        vint    *src_ptr);

void _VTSPR_downSample(
        UDS_Obj *udsObj_ptr,
        vint    *dst_ptr,
        vint    *src_ptr);
#endif
#endif

#endif


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
#ifndef PVMF_JITTER_BUFFER_H_INCLUDED
#include "pvmf_rtp_jitter_buffer_impl.h"
#endif

#ifndef OSCL_BYTE_ORDER_H_INCLUDED
#include "oscl_byte_order.h"
#endif

#ifndef PVMF_SM_NODE_EVENTS_H_INCLUDED
#include "pvmf_sm_node_events.h"
#endif


#ifndef PVMF_MEDIA_CMD_H_INCLUDED
#include "pvmf_media_cmd.h"
#endif

#ifndef PVMF_BASIC_ERRORINFOMESSAGE_H_INCLUDED
#include "pvmf_basic_errorinfomessage.h"
#endif

#ifndef PVMF_MEDIA_MSG_HEADER_H_INCLUDED
#include "pvmf_media_msg_header.h"
#endif

// Define entry point for this DLL
OSCL_DLL_ENTRY_POINT_DEFAULT()

#define PVMF_JITTER_BUFFER_ROLL_OVER_THRESHOLD_16BIT 2000
const uint16 RTPSEQNUM_ROLLOVER_WRAP_THRESHHOLD_16BIT = 0x8000;

const int32 iEstimatedServerKeepAheadInMilliSeconds = 2000;
const int32 OOO_DATAPATH_DELAY_FACTOR = 500;

/* RTP HEADER CONSTANTS */
#define SUPPORTED_RTP_HEADER_VERSION    2
#define RTP_FIXED_HEADER_SIZE           12
#define RTP_HEADER_V_BIT_MASK           0xC0
#define RTP_HEADER_V_BIT_OFFSET         6
#define RTP_HEADER_P_BIT_MASK           0x20
#define RTP_HEADER_P_BIT_OFFSET         5
#define RTP_HEADER_X_BIT_MASK           0x10
#define RTP_HEADER_X_BIT_OFFSET         4
#define RTP_HEADER_CC_BIT_MASK          0x0F
#define RTP_HEADER_M_BIT_MASK           0x80
#define RTP_HEADER_M_BIT_OFFSET         7
#define RTP_HEADER_PT_MASK              0x7F

OSCL_EXPORT_REF PVMFJitterBuffer* PVMFRTPJitterBufferImpl::New(const PVMFJitterBufferConstructParams& aCreationData)
{
    PVMFRTPJitterBufferImpl* ptr = OSCL_NEW(PVMFRTPJitterBufferImpl, (aCreationData));
    if (ptr)
    {
        ptr->Construct();
    }
    return ptr;
}

OSCL_EXPORT_REF PVMFRTPJitterBufferImpl::~PVMFRTPJitterBufferImpl()
{
    if (iPacketArrivalClock)
    {
        iPacketArrivalClock->Stop();
        OSCL_DELETE(iPacketArrivalClock);
    }

    if (iBurstClock)
    {
        iBurstClock->Stop();
        OSCL_DELETE(iBurstClock);
    }
}

OSCL_EXPORT_REF void PVMFRTPJitterBufferImpl::StreamingSessionStarted()
{
    if (!iEOSSignalled)
    {
        irEstimatedServerClock.Start();
        iBurstClock->Start();
        CancelEventCallBack(JB_BUFFERING_DURATION_COMPLETE);
        RequestEventCallBack(JB_BUFFERING_DURATION_COMPLETE);
    }
    PVMFJitterBufferImpl::StreamingSessionStarted();
}

PVMFRTPJitterBufferImpl::PVMFRTPJitterBufferImpl(const PVMFJitterBufferConstructParams& aCreationData): PVMFJitterBufferImpl(aCreationData)
{
    if (iInPlaceProcessing)
    {
        iOnePacketPerMediaMsg = true;
    }
    else
    {
        iOnePacketPerMediaMsg = false;
    }

    iOnePacketPerFragment = true;
    iHeaderPreParsed = false;

    seqNumLock = false;
    iPrevPacketTS = 0;
    isPrevNptTimeSet = false;
    iPrevNptTimeInRTPTimeScale = 0;
    iInterArrivalJitterD = 0;
    iPrevPacketRecvTime = 0;
    isPrevRtpTimeSet = false;
    iPrevRtpTimeBase = 0;
    iRTPTimeScale = 0;
    iOverflowFlag = false;

    iPlayListRTPTimeBase = 0;
    iPlayListRTPTimeBaseSet = false;

    iBurstDetect = false;
    iBurstStartTimestamp = 0;
    iEstServerClockBurstStartTimestamp = 0;
    iRTPDataArrived = false;
    iEarlyDecodingTime = 0;
    iServerBurst = false;
    iBurstThreshold = 1.5;
    iBurstDetectDurationInMilliSec = 0;
    iInitialBuffering = true;
    iRTCPBasedAVSyncRequired = true;
    iRTPAVSyncOffset = 0;
    iBurstClock = NULL;
    iAccessUnits.clear();
    ipRTPInfoTrackerLogger = PVLogger::GetLoggerObject("RTPInfoTracker");
    ipParserInLogger = PVLogger::GetLoggerObject(irMimeType.get_cstr());
    ipParserOutLogger = PVLogger::GetLoggerObject(irMimeType.get_cstr());

}

void PVMFRTPJitterBufferImpl::Construct()
{
    iPacketArrivalClock = OSCL_NEW(PVMFMediaClock, ());
    iPacketArrivalClock->SetClockTimebase(iPacketArrivalTimeBase);
    iPacketArrivalClock->Start();
    PVMFJitterBufferImpl::Construct();

    iBurstClock = OSCL_NEW(PVMFMediaClock, ());
    iBurstClock->SetClockTimebase(iBurstClockTimeBase);

    bool overflowFlag = false;
    uint32 start = 0;
    iBurstClock->Stop();
    iBurstClock->SetStartTime32(start, PVMF_MEDIA_CLOCK_MSEC, overflowFlag);
}

OSCL_EXPORT_REF void PVMFRTPJitterBufferImpl::ResetJitterBuffer()
{
    if (iInPlaceProcessing)
    {
        iOnePacketPerMediaMsg = true;
    }
    else
    {
        iOnePacketPerMediaMsg = false;
    }

    iOnePacketPerFragment = true;
    iHeaderPreParsed = false;

    seqNumLock = false;
    iPrevPacketTS = 0;
    isPrevNptTimeSet = false;
    iPrevNptTimeInRTPTimeScale = 0;
    iInterArrivalJitterD = 0;
    iPrevPacketRecvTime = 0;
    isPrevRtpTimeSet = false;
    iPrevRtpTimeBase = 0;
    iRTPTimeScale = 0;

    iPlayListRTPTimeBaseSet = false;
    iPlayListRTPTimeBase = 0;

    iOverflowFlag = false;

    iBurstDetect = false;
    iBurstStartTimestamp = 0;
    iEstServerClockBurstStartTimestamp = 0;
    iRTPDataArrived = false;
    iEarlyDecodingTime = 0;
    iServerBurst = false;
    iBurstThreshold = 1.5;
    iBurstDetectDurationInMilliSec = 0;
    iInitialBuffering = true;
    iRTCPBasedAVSyncRequired = true;
    iRTPAVSyncOffset = 0;
    iCurrFormatId = 0;

    if (iPacketArrivalClock)
    {
        iPacketArrivalClock->Reset();
    }

    if (iBurstClock)
    {
        iBurstClock->Reset();
    }

    PVMFJitterBufferImpl::ResetJitterBuffer();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PVMFRTPJitterBufferImpl::AdjustRTPTimeStamp()
{
    // By the time this function is called, iMonotonicTimeStamp should be already normalized
    // with the corresponding values from other jitterbuffers
    iMaxAdjustedRTPTS = Oscl_Int64_Utils::get_uint64_lower32(iMonotonicTimeStamp);
    UpdateEstimatedServerClock(true);
}

OSCL_EXPORT_REF void PVMFRTPJitterBufferImpl::setRTPInfoParams(PVMFRTPInfoParams rtpInfo, bool oPlayAfterASeek)
{
    //TODO: ??May be we should push in the vect only if seqbase is set
    if (oPlayAfterASeek)
    {
        //Clean everything in the vector before pushing new RTPInfo
        iRTPInfoParamsVec.clear();
    }

    if (rtpInfo.seqNumBaseSet)
    {
        iJitterBuffer->setSeqNumBase(rtpInfo.seqNum);
    }

    PVMFRTPInfoParams iRTPInfoParams;
    iRTPInfoParams.seqNumBaseSet = rtpInfo.seqNumBaseSet;
    if (rtpInfo.seqNumBaseSet == true)
    {
        iRTPInfoParams.seqNum = rtpInfo.seqNum;
    }
    iRTPInfoParams.rtpTimeBaseSet = rtpInfo.rtpTimeBaseSet;
    if (rtpInfo.rtpTimeBaseSet == true)
    {
        iRTPInfoParams.rtpTime = rtpInfo.rtpTime;
    }
    iRTPInfoParams.rtpTimeScale = rtpInfo.rtpTimeScale;
    iRTPTimeScale = rtpInfo.rtpTimeScale;
    iEstServClockMediaClockConvertor.set_timescale(iRTPTimeScale);
    iMediaClockConvertor.set_timescale(1000);
    iMediaClockConvertor.set_clock_other_timescale(0, iRTPInfoParams.rtpTimeScale);
    iRTPInfoParams.nptTimeBaseSet = rtpInfo.nptTimeBaseSet;
    if (iRTPInfoParams.nptTimeBaseSet == true)
    {
        iRTPInfoParams.nptTimeInMS = rtpInfo.nptTimeInMS;
        iMediaClockConvertor.update_clock(iRTPInfoParams.nptTimeInMS);
        iRTPInfoParams.nptTimeInRTPTimeScale =
            iMediaClockConvertor.get_converted_ts(iRTPInfoParams.rtpTimeScale);
    }
    /* In case this is the first rtp info set TS calc variables */
    if (iRTPInfoParamsVec.size() == 0)
    {
        if (iRTPInfoParams.rtpTimeBaseSet)
        {
            iRTCPBasedAVSyncRequired = false;
            ResetRTCPAVSyncAdjustments();
            iPrevTSOut = iRTPInfoParams.rtpTime;
            iPrevTSIn = iRTPInfoParams.rtpTime;
            iPrevAdjustedRTPTS = iRTPInfoParams.rtpTime;
            if (iPlayListRTPTimeBaseSet == false)
            {
                iPlayListRTPTimeBaseSet = true;
                iPlayListRTPTimeBase = iRTPInfoParams.rtpTime;
            }
        }
        else
        {
            /* Use the value from the first packet */
            if (seqNumLock)
            {
                iPrevTSOut = seqLockTimeStamp;
                iPrevTSIn = seqLockTimeStamp;
                iPrevAdjustedRTPTS = seqLockTimeStamp;
            }
        }

        if (iRTPInfoParams.seqNumBaseSet)
        {
            iPrevSeqNumBaseOut = iRTPInfoParams.seqNum;
            iPrevSeqNumBaseIn = iRTPInfoParams.seqNum;
        }
        else
        {
            /* Use the value from the first packet */
            if (seqNumLock)
            {
                iPrevSeqNumBaseOut = iFirstSeqNum;
                iPrevSeqNumBaseIn = iFirstSeqNum;
            }
        }
        /* Initialize the variables used for ts calculation between pause and resume */
        if (iRTPInfoParams.nptTimeBaseSet)
        {
            iPrevNptTimeInRTPTimeScale = iRTPInfoParams.nptTimeInRTPTimeScale;
            isPrevNptTimeSet = true;
        }
        if (iRTPInfoParams.rtpTimeBaseSet)
        {
            iPrevRtpTimeBase = iRTPInfoParams.rtpTime;
            isPrevRtpTimeSet = true;
        }
    }
    else
    {
        iRTPInfoParams.isPlayAfterPause = !oPlayAfterASeek;
    }
    if (iRTPInfoParams.rtpTimeBaseSet)
    {
        iPrevAdjustedRTPTS = iRTPInfoParams.rtpTime;
    }

    iRTPInfoParamsVec.push_back(iRTPInfoParams);
}

OSCL_EXPORT_REF uint32 PVMFRTPJitterBufferImpl::getInterArrivalJitter()
{
    return iInterArrivalJitter;
}

OSCL_EXPORT_REF void
PVMFRTPJitterBufferImpl::PurgeElementsWithSeqNumsLessThan(uint32 aSeqNum,
        uint32 aPlayerClockMS)
{
    iJitterBuffer->PurgeElementsWithSeqNumsLessThan(aSeqNum, iPrevSeqNumBaseOut);

    {
        iMaxAdjustedRTPTS =
            Oscl_Int64_Utils::get_uint64_lower32(iMonotonicTimeStamp);
        /*
         * In case of 3GPP streaming this clock adjustment is performed
         * at a later point, via the "AdjustRTPTimeStamp" API call from
         * jitter buffer node.
         */
        OSCL_UNUSED_ARG(aPlayerClockMS);
    }

    PVMF_JB_LOGDATATRAFFIC_IN((0, "PVMFJitterBufferImpl::PurgeElementsWithSeqNumsLessThan - SeqNum=%d",
                               aSeqNum));
}

OSCL_EXPORT_REF void
PVMFRTPJitterBufferImpl::PurgeElementsWithTimestampLessThan(PVMFTimestamp aTS)
{
    PVMFTimestamp rtpTS;
    if (iPlayListRTPTimeBaseSet)
    {
        rtpTS = iPlayListRTPTimeBase + aTS;
    }
    else
    {
        rtpTS = aTS;
    }

    iJitterBuffer->PurgeElementsWithTimestampLessThan(rtpTS);
    iMaxAdjustedRTPTS = aTS;
    //iPrevAdjustedRTPTS = rtpTS;
    UpdateEstimatedServerClock(true);
    iMonotonicTimeStamp = aTS;
    iPrevTSOut = rtpTS;
    PVMF_JB_LOGDATATRAFFIC_IN((0, "PVMFJitterBufferImpl::PurgeElementsWithTimestampLessThan - ntpTS=%u, rtpTS=%u",
                               aTS, rtpTS));
}

/**
*/
OSCL_EXPORT_REF bool PVMFRTPJitterBufferImpl::GetRTPTimeStampOffset(uint32& aTimeStampOffset) const
{
    if (seqNumLock)
        aTimeStampOffset = seqLockTimeStamp;

    return seqNumLock;
}

/**
*/
OSCL_EXPORT_REF void  PVMFRTPJitterBufferImpl::SetRTPTimeStampOffset(uint32 newTSBase)
{
    // This function must be used only to offset the RTP TB
    // for broadcast streaming.
    // Based on that, the following is assumed:
    // 1) seqLockTimeStamp is valid
    // 2) newTSBase <= seqLockTimeStamp
    if (iRTCPBasedAVSyncRequired)
    {
        iRTPAVSyncOffset = seqLockTimeStamp - newTSBase;
        iMonotonicTimeStamp += iRTPAVSyncOffset;
        iMaxAdjustedRTPTS += iRTPAVSyncOffset;
        iRTCPBasedAVSyncRequired = false;
    }
}

void  PVMFRTPJitterBufferImpl::ResetRTCPAVSyncAdjustments()
{
    iMonotonicTimeStamp -= iRTPAVSyncOffset;
    iMaxAdjustedRTPTS -= iRTPAVSyncOffset;
    iRTPAVSyncOffset = 0;
}

/**
*/
OSCL_EXPORT_REF bool PVMFRTPJitterBufferImpl::NotifyFreeSpaceAvailable()
{

    if (iMediaDataGroupAlloc)
    {
        iMediaDataGroupAlloc->notifyfreechunkavailable(*this);
        return true;
    }
    return false;
}

void PVMFRTPJitterBufferImpl::SetEarlyDecodingTimeInMilliSeconds(uint32 aDuration)
{
    if (aDuration > 0)
    {
        iBurstDetect = true;
        iEarlyDecodingTime = aDuration;
        iBurstDetectDurationInMilliSec = (aDuration / 2);
    }
    else
    {
        iBurstDetect = false;
        iEarlyDecodingTime = 0;
        iBurstDetectDurationInMilliSec = 0;
    }
}

void PVMFRTPJitterBufferImpl::SetBurstThreshold(float aBurstThreshold)
{
    iBurstThreshold = aBurstThreshold;
}


bool PVMFRTPJitterBufferImpl::IsDelayEstablished(uint32& aClockDiff)
{
    aClockDiff = iDurationInMilliSeconds;
    if (GetState() == PVMF_JITTER_BUFFER_IN_TRANSITION)
    {
        PVMF_JB_LOGCLOCK((0, "PVMFJitterBufferNode::IsDelayEstablished - Jitter Buffer In Transition - Preparing for Seek"));
        irDelayEstablished = false;
        irJitterDelayPercent = 0;
        return irDelayEstablished;
    }

    uint32 timebase32 = 0;
    uint32 estServerClock = 0;
    uint32 clientClock = 0;
    bool overflowFlag = false;

    irEstimatedServerClock.GetCurrentTime32(estServerClock, overflowFlag, PVMF_MEDIA_CLOCK_MSEC, timebase32);
    irClientPlayBackClock.GetCurrentTime32(clientClock, overflowFlag, PVMF_MEDIA_CLOCK_MSEC, timebase32);

    PVMF_JB_LOGCLOCK((0, "PVMFJitterBufferNode::IsDelayEstablished - Time Delay Check - EstServClock=%d", estServerClock));
    PVMF_JB_LOGCLOCK((0, "PVMFJitterBufferNode::IsDelayEstablished - Time Delay Check - ClientClock=%d", clientClock));

    if (iEOSSignalled)
    {
        /*
        * No check needed - We are past the clip time, just play out the last
        * bit in the jitter buffer
        */
        PVMF_JB_LOGCLOCK((0, "PVMFJitterBufferNode::IsDelayEstablished - Session Duration Expired"));
        if (irDelayEstablished == false)
        {
            /*
            * Coming out of rebuffering in case we had gone into
            * rebuffering just before
            */
            irJitterDelayPercent = 100;
            PVMFAsyncEvent jbEvent(PVMFInfoEvent, PVMFInfoDataReady, NULL, NULL);
            ReportJBInfoEvent(jbEvent);
        }
        irDelayEstablished = true;
    }
    else
    {
        uint32 diff32ms = 0;
        bool isEarly = PVTimeComparisonUtils::IsEarlier(estServerClock, clientClock, diff32ms);
        if (isEarly && diff32ms > 0)
        {
            /* Could happen during repositioning */
            if (irDelayEstablished == true)
            {
                aClockDiff = 0;
                irDelayEstablished = false;
                irJitterDelayPercent = 0;
                /* Start timer */
                PVMFAsyncEvent jbEvent(PVMFInfoEvent, PVMFInfoUnderflow, NULL, NULL);
                ReportJBInfoEvent(jbEvent);
                PVMF_JB_LOGCLOCK((0, "PVMFJitterBufferNode::IsDelayEstablished - Time Delay Check - EstServClock=%d",
                                  Oscl_Int64_Utils::get_uint64_lower32(estServerClock)));
                PVMF_JB_LOGCLOCK((0, "PVMFJitterBufferNode::IsDelayEstablished - Time Delay Check - ClientClock=%d",
                                  Oscl_Int64_Utils::get_uint64_lower32(clientClock)));
                PVMF_JB_LOGCLOCK((0, "PVMFJitterBufferNode::IsDelayEstablished - Estimated Serv Clock Less Than ClientClock!!!!"));
                PVMF_JB_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::IsDelayEstablished - Time Delay Check - EstServClock=%d",
                                         Oscl_Int64_Utils::get_uint64_lower32(estServerClock)));
                PVMF_JB_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::IsDelayEstablished - Time Delay Check - ClientClock=%d",
                                         Oscl_Int64_Utils::get_uint64_lower32(clientClock)));
                PVMF_JB_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::IsDelayEstablished - Estimated Serv Clock Less Than ClientClock!!!!"));
            }
            return irDelayEstablished;
        }
        aClockDiff = diff32ms;

        // Make sure RTP packets have arrived before starting burst measurement
        if (iRTPDataArrived && iBurstDetect)
        {
            uint32 timebase32 = 0;
            uint32 aCurrentRealTime = 0;
            uint32 aRealTimeDelta = 0;
            uint32 aServerClientClockDelta = 0;
            float  aBurstRateSrvClnt = 0;
            bool overFlowFlagBurst;

            // Save burst start time only if its current value is 0
            if (iBurstStartTimestamp == 0)
            {
                iBurstClock->GetCurrentTime32(iBurstStartTimestamp, overFlowFlagBurst, PVMF_MEDIA_CLOCK_MSEC, timebase32);
                iEstServerClockBurstStartTimestamp = estServerClock;
            }

            // Find current real time
            iBurstClock->GetCurrentTime32(aCurrentRealTime, overFlowFlagBurst, PVMF_MEDIA_CLOCK_MSEC, timebase32);

            // Compute real time clk delta and est srv clk delta
            aRealTimeDelta = aCurrentRealTime - iBurstStartTimestamp;

            if (aRealTimeDelta >= iBurstDetectDurationInMilliSec)
            {
                // Est srv clk - client clk is used to calculate burst rate
                if (!iInitialBuffering)
                {
                    aServerClientClockDelta = estServerClock - iEstServerClockBurstStartTimestamp;
                }
                else
                {
                    aServerClientClockDelta = diff32ms;
                }

                // Compute burst rate
                if (aRealTimeDelta)
                {
                    aBurstRateSrvClnt = (float)(aServerClientClockDelta) /
                                        (float)(aRealTimeDelta);
                }

                iBurstDetect = false;
                if (aBurstRateSrvClnt > iBurstThreshold)
                {
                    iServerBurst = true;
                }
                PVMF_JB_LOGCLOCK((0, "PVMFJitterBufferNode::IsDelayEstablished - Real time clk:: ref val: %2d,", iBurstStartTimestamp));
                if (iInitialBuffering)
                {
                    PVMF_JB_LOGCLOCK((0, "PVMFJitterBufferNode::IsDelayEstablished - client clk:: ref val: %d", clientClock));
                }
                else
                {
                    PVMF_JB_LOGCLOCK((0, "PVMFJitterBufferNode::IsDelayEstablished - est srv:: ref val: %d", iEstServerClockBurstStartTimestamp));
                }
                PVMF_JB_LOGCLOCK((0, "PVMFJitterBufferNode::IsDelayEstablished - real time clk:: cur val: %2d", aCurrentRealTime));
                PVMF_JB_LOGCLOCK((0, "PVMFJitterBufferNode::IsDelayEstablished - est srv clk:: cur val: %2d", estServerClock));
                PVMF_JB_LOGCLOCK((0, "PVMFJitterBufferNode::IsDelayEstablished - real time delta: %2d", aRealTimeDelta));
                PVMF_JB_LOGCLOCK((0, "PVMFJitterBufferNode::IsDelayEstablished - est srv client clk delta: %2d", aServerClientClockDelta));
                PVMF_JB_LOGCLOCK((0, "PVMFJitterBufferNode::IsDelayEstablished - burst rate * 100: %d", (uint32)(100.0 * aBurstRateSrvClnt)));
                PVMF_JB_LOGCLOCK((0, "PVMFJitterBufferNode::IsDelayEstablished - burst threshold * 100: %d", (uint32)(100.0 * iBurstThreshold)));
            }
        }
        // Check if SM has set the early decoding flag. If true, and if burst rate exceeds threshold,
        // signal buffering completion.
        if (iServerBurst && iEarlyDecodingTime && (diff32ms >= iEarlyDecodingTime))
        {
            iServerBurst = false;
            PVMF_JB_LOGCLOCK((0, "PVMFJitterBufferNode::IsDelayEstablished - Cancelling Jitter Buffer Duration Timer"));
            irDelayEstablished = true;
            irJitterDelayPercent = 100;
            PVMFAsyncEvent jbEvent(PVMFInfoEvent, PVMFInfoDataReady, NULL, NULL);
            ReportJBInfoEvent(jbEvent);
        }
        else
        {
            if (diff32ms >= iDurationInMilliSeconds)
            {
                if (iBufferAlloc)
                {
                    uint32 jbSize = iBufferAlloc->getBufferSize();
                    uint32 largestContiguousFreeBlockSize = iBufferAlloc->getLargestContiguousFreeBlockSize();
                    uint32 minPercentOccupancy = 100;
                    if ((largestContiguousFreeBlockSize*100 / jbSize) < minPercentOccupancy)
                    {
                        minPercentOccupancy = (uint32)(largestContiguousFreeBlockSize * 100 / jbSize);
                    }

                    if ((prevMinPercentOccupancy < MIN_PERCENT_OCCUPANCY_THRESHOLD) && (minPercentOccupancy < MIN_PERCENT_OCCUPANCY_THRESHOLD))
                    {
                        consecutiveLowBufferCount++;
                    }
                    else
                    {
                        consecutiveLowBufferCount = 0;
                    }

                    prevMinPercentOccupancy = minPercentOccupancy;
                    PVMF_JB_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::IsDelayEstablished - minPercentOccupancy=%d, consecutiveLowBufferCount=%d jbSize%d largestContiguousFreeBlockSize %d",
                                             minPercentOccupancy,
                                             consecutiveLowBufferCount, jbSize, largestContiguousFreeBlockSize));


                    if ((diff32ms > JITTER_BUFFER_DURATION_MULTIPLIER_THRESHOLD*iDurationInMilliSeconds) && !iOverflowFlag && (consecutiveLowBufferCount > CONSECUTIVE_LOW_BUFFER_COUNT_THRESHOLD))
                    {
                        iOverflowFlag = true;
                        PVMFAsyncEvent jbEvent(PVMFInfoEvent, PVMFInfoOverflow, NULL, NULL);
                        ReportJBInfoEvent(jbEvent);
                        PVMF_JB_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::IsDelayEstablished - minPercentOccupancy=%d, consecutiveLowBufferCount=%d, diff32ms = %d, largestContiguousFreeBlockSize = %d",
                                                 minPercentOccupancy,
                                                 consecutiveLowBufferCount, diff32ms, largestContiguousFreeBlockSize));
                        PVMF_JB_LOGCLOCK((0, "PVMFJitterBufferNode::IsDelayEstablished - Time Delay Established - EstServClock=%d", estServerClock));
                        PVMF_JB_LOGCLOCK((0, "PVMFJitterBufferNode::IsDelayEstablished - Time Delay Established - ClientClock=%d",  clientClock));
                        PVMF_JB_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::IsDelayEstablished - Time Delay Established - EstServClock=%d",
                                                 estServerClock));
                        PVMF_JB_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::IsDelayEstablished - Time Delay Established - ClientClock=%d",
                                                 clientClock));
                    }
                }
                if (irDelayEstablished == false)
                {
                    if (CheckNumElements())
                    {
                        PVMF_JB_LOGCLOCK((0, "PVMFJitterBufferNode::IsDelayEstablished - Cancelling Jitter Buffer Duration Timer"));
                        irDelayEstablished = true;
                        irJitterDelayPercent = 100;
                        PVMFAsyncEvent jbEvent(PVMFInfoEvent, PVMFInfoDataReady, NULL, NULL);
                        ReportJBInfoEvent(jbEvent);
                        PVMF_JB_LOGCLOCK((0, "PVMFJitterBufferNode::IsDelayEstablished - Time Delay Established - EstServClock=%d", estServerClock));
                        PVMF_JB_LOGCLOCK((0, "PVMFJitterBufferNode::IsDelayEstablished - Time Delay Established - ClientClock=%d",  clientClock));
                        PVMF_JB_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::IsDelayEstablished - Time Delay Established - EstServClock=%d",
                                                 estServerClock));
                        PVMF_JB_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::IsDelayEstablished - Time Delay Established - ClientClock=%d",
                                                 clientClock));
                    }
                    else
                    {
                        irJitterDelayPercent = 0;
                    }
                }
                else
                {
                    irJitterDelayPercent = 100;
                }
            }
            else
            {
                /*
                * Update the buffering percent - to be used while sending buffering
                * status events, in case we go into rebuffering or if we are in buffering
                * state.
                */
                irJitterDelayPercent = ((diff32ms * 100) / iDurationInMilliSeconds);
                if (irDelayEstablished == true)
                {
                    if (diff32ms <= iRebufferingThresholdInMilliSeconds)
                    {
                        /* Implies that we are going into rebuffering */
                        if (!iEOSSignalled)
                        {
                            irDelayEstablished = false;
                            PVMFAsyncEvent jbEvent(PVMFInfoEvent, PVMFInfoUnderflow, NULL, NULL);
                            ReportJBInfoEvent(jbEvent);
                            LOGCLIENTANDESTIMATEDSERVCLK_REBUFF;
                        }
                        /* we are past the end of the clip, no more rebuffering */
                        irClientPlayBackClock.Pause();
                    }
                }
                if (irDelayEstablished == false && CheckNumElements() == false)
                {
                    irJitterDelayPercent = 0;
                }
                PVMF_JB_LOGCLOCK((0, "PVMFJitterBufferNode::IsDelayEstablished: Delay Percent = %d", irJitterDelayPercent));
            }
        }
        /* if we are not rebuffering check for flow control */
        PerformFlowControl(false);
    }
    if (irDelayEstablished)
    {
        iInitialBuffering = false;
    }
    return (irDelayEstablished);
}

void PVMFRTPJitterBufferImpl::EOSCmdReceived()
{
    /*
     * Check for upstream EOS. Set the boolean if it is. EOS from jitter buffer
     * is sent when jitter buffer is all empty. We do not really expect EOS from
     * upstream nodes in case of RTP processing. So ignore it.
     */
    PVMF_JB_LOGERROR((0, "PVMFRTPJitterBufferImpl::ProcessIncomingMsgRTP - Unexpected EOS"));
    PVMF_JB_LOGDATATRAFFIC_IN((0, "PVMFRTPJitterBufferImpl::ProcessIncomingMsgRTP - Unexpected EOS"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//_______________________________________________________________________________________________
//00 01|02|03|04 05 06 07|08|09 10 11 12 13 14 15|16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31|
//_______________________________________________________________________________________________
// Ver |P |X |    CC     |M |         PT         |                 Sequence Number               |
//_______________________________________________________________________________________________
//                                     Timestamp                                                 |
//_______________________________________________________________________________________________
//                                      SSRC                                                     |
//_______________________________________________________________________________________________
//                                  CSRC [0..15] :::                                             |
//_______________________________________________________________________________________________
//////////////////////////////////////////////////////////////////////////////////////////////////
PVMFJBPacketParsingAndStatUpdationStatus PVMFRTPJitterBufferImpl::ParsePacketHeaderAndUpdateJBStats(PVMFSharedMediaDataPtr& rtpPacketContainer,
        PVMFSharedMediaDataPtr& aOutDataPacket,
        uint32 aFragIndex)
{
    //Initial validations
    OsclSharedPtr<PVMFMediaDataImpl> mediaDataIn;
    if (!rtpPacketContainer->getMediaDataImpl(mediaDataIn))
    {
        return PVMF_JB_ERR_NO_PACKET;
    }

    uint32 SSRC = 0;
    uint32 markerInfo = 0;
    uint32 seqNum = 0;
    PVMFTimestamp rtpTimeStamp  = 0;
    OsclRefCounterMemFrag rtpPacket;
    rtpPacketContainer->getMediaFragment(aFragIndex, rtpPacket);
    uint8* rtpHeaderOffset = (uint8*)(rtpPacket.getMemFrag().ptr);
    uint32 rtpPacketLenExcludingHeader = rtpPacket.getMemFrag().len;


    if (!iHeaderPreParsed)     //RTP packet
    {
        //Validate the packet as per streaming requirements
        //Update jitter buffer stats like, packet arrival latency
        /* Get start of RTP packet */
        uint8* rtpHeader    = (uint8*)(rtpPacket.getMemFrag().ptr);
        uint32 rtpPacketLen = rtpPacket.getMemFrag().len;

        /* is this a legal data packet? */
        if (rtpPacketLen <= RTP_FIXED_HEADER_SIZE)
        {
            return PVMF_JB_ERR_TRUNCATED_HDR;
        }


        if (!iRTPDataArrived)
        {
            iRTPDataArrived = true;
        }


        /* Parse RTP version */
        uint8  rtpVersion = (((*rtpHeader) & RTP_HEADER_V_BIT_MASK) >> RTP_HEADER_V_BIT_OFFSET);
        if (rtpVersion != SUPPORTED_RTP_HEADER_VERSION)
        {
            PVMF_JB_LOGERROR((0, "0x%x RTPPacketHeaderParser::ParseRTPHeader: illegal rtp version", this));
            return PVMF_JB_ERR_CORRUPT_HDR;
        }

        /* Check for padding */
        uint8 pbit = (((*rtpHeader) & RTP_HEADER_P_BIT_MASK) >> RTP_HEADER_P_BIT_OFFSET);
        uint8 numPaddingOctets = 0;
        if (pbit)
        {
            numPaddingOctets = *(rtpHeader + (rtpPacketLen - 1));
        }

        /* Check for header extension */
        uint8 xbit = (((*rtpHeader) & RTP_HEADER_X_BIT_MASK) >> RTP_HEADER_X_BIT_OFFSET);

        /* Check for CSRC */
        uint8 csrcCount = ((*rtpHeader) & RTP_HEADER_CC_BIT_MASK);

        rtpHeader++;

        /* Parse M bit */
        uint8 mbit = (((*rtpHeader) & RTP_HEADER_M_BIT_MASK) >> RTP_HEADER_M_BIT_OFFSET);

        rtpHeader++;

        /* Parse sequence number */
        uint16 seqNum16 = 0;
        oscl_memcpy((char *)&seqNum16, rtpHeader, sizeof(seqNum16));
        big_endian_to_host((char *)&seqNum16, sizeof(seqNum16));
        seqNum = (uint32)seqNum16;
        rtpHeader += 2;

        /* Parse rtp time stamp */
        uint32 ts32 = 0;
        oscl_memcpy((char *)&ts32, rtpHeader, sizeof(ts32));
        big_endian_to_host((char *)&ts32, sizeof(ts32));
        rtpTimeStamp = (PVMFTimestamp)ts32;
        rtpHeader += 4;

        /* Parse SSRC */
        uint32 ssrc32 = 0;
        oscl_memcpy((char *)&ssrc32, rtpHeader, sizeof(ssrc32));
        big_endian_to_host((char *)&ssrc32, sizeof(ssrc32));
        SSRC = ssrc32;
        rtpHeader += 4;

        rtpPacketLen -= RTP_FIXED_HEADER_SIZE;

        /* Check for CSRC list - If present skip over */
        if (csrcCount)
        {
            if ((uint32)(csrcCount*4) > rtpPacketLen)
            {
                PVMF_JB_LOGERROR((0, "0x%x RTPPacketHeaderParser::ParseRTPHeader: Corrupt CSRC", this));
                return PVMF_JB_ERR_CORRUPT_HDR;
            }
            rtpHeader += (csrcCount * 4);
            rtpPacketLen -= (csrcCount * 4);
        }

        /* Check for extended RTP header - If present skip over */
        if (xbit)
        {
            rtpHeader += 2;
            uint16 len16 = 0;
            oscl_memcpy((char *)&len16, rtpHeader, sizeof(len16));
            big_endian_to_host((char *)&len16, sizeof(len16));
            uint32 extensionHeaderLen = (uint32)len16;
            rtpPacketLen -= 4;
            if ((extensionHeaderLen*4) > rtpPacketLen)
            {
                PVMF_JB_LOGERROR((0, "0x%x RTPPacketHeaderParser::ParseRTPHeader: Corrupt Extended Header Length", this));
                return PVMF_JB_ERR_CORRUPT_HDR;
            }
            rtpHeader += (extensionHeaderLen * 4);
            rtpPacketLen -= (extensionHeaderLen * 4);
        }

        /* Ignore padding octets */
        if (numPaddingOctets)
        {
            if ((uint32)numPaddingOctets > rtpPacketLen)
            {
                PVMF_JB_LOGERROR((0, "0x%x RTPPacketHeaderParser::ParseRTPHeader: Corrupt Padding Length", this));
                return PVMF_JB_ERR_CORRUPT_HDR;
            }
            rtpPacketLen -= numPaddingOctets;
        }

        if (mbit == 1) markerInfo |= PVMF_MEDIA_DATA_MARKER_INFO_M_BIT;
        rtpHeaderOffset = rtpHeader;
        rtpPacketLenExcludingHeader = rtpPacketLen;
    }
    else
    {
        if (aFragIndex != 0)
        {
            return PVMF_JB_ERR_INVALID_CONFIGURATION;
        }
        rtpTimeStamp = rtpPacketContainer->getTimestamp();
        seqNum = rtpPacketContainer->getSeqNum();
        rtpTimeStamp = rtpPacketContainer->getTimestamp();
        SSRC = rtpPacketContainer->getStreamID();
        markerInfo = rtpPacketContainer->getMarkerInfo();
    }

    if (iInPlaceProcessing == false)
    {
        OsclSharedPtr<PVMFMediaDataImpl> mediaDataOut;
        bool status = Allocate(mediaDataOut);
        if (status == false)
        {
            PVMF_JB_LOGERROR((0, "0x%x RTPPacketHeaderParser::ParseRTPHeader: Jitter Buffer Full", this));
            return PVMF_JB_ERR_INSUFFICIENT_MEM_TO_PACKETIZE;
        }

        OsclRefCounterMemFrag memFragOut(rtpPacket);
        memFragOut.getMemFrag().ptr = rtpHeaderOffset;
        memFragOut.getMemFrag().len = rtpPacketLenExcludingHeader;

        mediaDataOut->appendMediaFragment(memFragOut);
        mediaDataOut->setMarkerInfo(markerInfo);

        bool retVal = CreateMediaData(aOutDataPacket, mediaDataOut);

        if (retVal == false)
        {
            PVMF_JB_LOGERROR((0, "0x%x RTPPacketHeaderParser::ParseRTPHeader: Jitter Buffer Full", this));
            return PVMF_JB_ERR_INSUFFICIENT_MEM_TO_PACKETIZE;
        }

        aOutDataPacket->setTimestamp(rtpTimeStamp);
        aOutDataPacket->setStreamID(SSRC);
        aOutDataPacket->setSeqNum(seqNum);
    }
    else
    {
        if (aFragIndex != 0)
        {
            return PVMF_JB_ERR_INVALID_CONFIGURATION;
        }

        rtpPacket.getMemFrag().ptr = rtpHeaderOffset;
        rtpPacket.getMemFrag().len = rtpPacketLenExcludingHeader;
        mediaDataIn->clearMediaFragments();
        mediaDataIn->appendMediaFragment(rtpPacket);
        rtpPacketContainer->setMarkerInfo(markerInfo);
        rtpPacketContainer->setTimestamp(rtpTimeStamp);
        rtpPacketContainer->setStreamID(SSRC);
        rtpPacketContainer->setSeqNum(seqNum);
    }

    PVMF_JB_LOGDATATRAFFIC_IN((0, "RTPPacketHeaderParser::ParseRTPHeader: SSRC=%u, rtpSeqNum=%d, rtpTs=%u, rtpPacketLen=%d",
                               SSRC, seqNum, rtpTimeStamp, rtpPacketLenExcludingHeader));


    //Validate the packet for Ts, Seq num etc... <only if seq num is locked>
    if (seqNumLock)
    {
        //First packet is always considered to be valid
        if (iFirstSeqNum != seqNum)
        {
            PVMFJitterBufferStats& jbStats = getJitterBufferStats();

            if (iBroadCastSession == true)
            {
                /*
                 * This can happen when using prerecorded transport streams that loop
                 * If this happens, just signal an unexpected data event
                 */
                if (rtpTimeStamp < jbStats.maxTimeStampRegistered)
                {
                    return PVMF_JB_ERR_UNEXPECTED_PKT;
                }
            }
            if (!IsSeqTsValidForPkt(seqNum, rtpTimeStamp, jbStats))
                return PVMF_JB_ERR_LATE_PACKET;
        }
    }

    if (iInPlaceProcessing)
    {
        UpdatePacketArrivalStats(rtpPacketContainer);
    }
    else
    {
        UpdatePacketArrivalStats(aOutDataPacket);
    }

    return PVMF_JB_PACKET_PARSING_SUCCESS;
}

OSCL_EXPORT_REF bool PVMFRTPJitterBufferImpl::IsSeqTsValidForPkt(uint32 aSeqNum, uint32 rtpTimeStamp, PVMFJitterBufferStats& jbStats)
{

    //Validate the packet for
    //- Late Packet <based on seqnum of the last packet sent out of JB>
    //- Roll Over <based on max seqnum of the packet registered with the JB>
    uint16 seqNum = (uint16)aSeqNum;

    uint16 delta = 0;
    uint16 referenceSeqNum = 0;
    if (jbStats.totalNumPacketsRetrieved > 0)
    {
        referenceSeqNum = OSCL_STATIC_CAST(uint16, (jbStats.lastRetrievedSeqNum + 1));
        if (IsSequenceNumEarlier(seqNum, referenceSeqNum, delta))
        {
            PVMF_JB_LOGDATATRAFFIC_IN_E((0, "PVMFRTPJitterBufferImpl::IsSeqTsValidForPkt - Late packet seqNum%u, Ts%u, LastRetrievedSeqNum%u, MaxSeqReg%u", seqNum, rtpTimeStamp, jbStats.lastRetrievedSeqNum, jbStats.maxSeqNumRegistered));
            return false;
        }
    }

    //Check for packet rollover <based on the max seq num registered with the jitter buffer>
    delta = 0;
    if ((seqNum < jbStats.maxSeqNumRegistered) && IsSequenceNumEarlier(OSCL_STATIC_CAST(uint16, jbStats.maxSeqNumRegistered), seqNum, delta))
    {
        PVMF_JB_LOGDATATRAFFIC_IN_E((0, "PVMFRTPJitterBufferImpl::IsSeqTsValidForPkt - Rollover seqNum%u, Ts%u, LastRetrievedSeqNum%u, MaxSeqReg%u", seqNum, rtpTimeStamp, jbStats.lastRetrievedSeqNum, jbStats.maxSeqNumRegistered));
        /* Reset these variables to acct for seqnum rollover */
        jbStats.maxSeqNumRegistered = seqNum;
        jbStats.maxSeqNumReceived = seqNum;
        jbStats.maxTimeStampRegistered = rtpTimeStamp;
    }
    return true;
}

void PVMFRTPJitterBufferImpl::UpdatePacketArrivalStats(PVMFSharedMediaDataPtr& aArrivedPacket)
{
    //Update interarrival jitter
    /* D(i-1,i) = (RecvT(i) - RTP_TS(i)) -
                  (RecvT(i-1) - RTP_TS(i-1)) */

    uint32 currPacketRecvTime32;
    bool overflowFlag = false;
    iPacketArrivalClock->GetCurrentTime32(currPacketRecvTime32, overflowFlag,
                                          PVMF_MEDIA_CLOCK_MSEC);

    const PVMFTimestamp rtpTimeStamp = aArrivedPacket->getTimestamp();
    int32 ts_diff = rtpTimeStamp - iPrevPacketTS;
    int32 arrival_diff = currPacketRecvTime32 - iPrevPacketRecvTime;

    int32 arrivalJitter = ts_diff - arrival_diff;
    if (ts_diff < arrival_diff)
        arrivalJitter = arrival_diff - ts_diff;

    PVMF_JB_LOGDATATRAFFIC_IN((0, "PVMFRTPJitterBufferImpl::UpdatePacketArrivalStats [%s] - new packet Ts %d prev packet ts %d , curr time %d prev time %d iInterArrivalJitterD %d", irMimeType.get_cstr(), rtpTimeStamp, iPrevPacketTS, currPacketRecvTime32, iPrevPacketRecvTime, iInterArrivalJitterD));

    /* J(i) = J(i-1) + (ABS(D(i-1,i)) - J(i-1))/16.0 */
    iInterArrivalJitterD += OSCL_STATIC_CAST(double, ((arrivalJitter - iInterArrivalJitterD) / 16.0));
    /* Round up */
    iInterArrivalJitter = (uint32)(iInterArrivalJitterD + 0.5);

    /* Update variables */
    iPrevPacketTS = rtpTimeStamp;
    iPrevPacketRecvTime = currPacketRecvTime32;
}

bool PVMFRTPJitterBufferImpl::CanRetrievePacket()
{
    LOGCLIENTANDESTIMATEDSERVCLK_DATAPATH_OUT;

    uint32 clockDiff;
    bool delayEstablished = IsDelayEstablished(clockDiff);
    if (!delayEstablished)
    {
        return false;
    }
    else
    {
        if (HasPendingCommand() == true)
        {
            return true;
        }

        PVMFJitterBufferStats stats = getJitterBufferStats();
        if (stats.currentOccupancy > 0)
        {
            if (iJitterBuffer->CheckCurrentReadPosition() == false)
            {
                uint32 in_wrap_count = 0;
                /*
                 * Peek Next timestamp
                 */
                PVMFTimestamp ts = peekNextElementTimeStamp();

                /*
                 * Convert Time stamp to milliseconds
                 */

                ipMediaClockConverter->set_clock(ts, in_wrap_count);
                PVMFTimestamp converted_ts =
                    ipMediaClockConverter->get_converted_ts(1000);
                /*
                 * Get current client playback clock in milliseconds
                 */
                uint32 clientClock;
                bool overflowFlag = false;

                irClientPlayBackClock.GetCurrentTime32(clientClock, overflowFlag, PVMF_MEDIA_CLOCK_MSEC);


                uint32 estimatedServClock = 0;
                overflowFlag = false;
                irEstimatedServerClock.GetCurrentTime32(estimatedServClock, overflowFlag, PVMF_MEDIA_CLOCK_MSEC);
                uint32 delta = 0;
                if (!iEOSSignalled && (PVTimeComparisonUtils::IsEarlier(clientClock + OOO_DATAPATH_DELAY_FACTOR, converted_ts, delta) && (delta > 0)))
                {
                    //hold the available data packet, and wait for hole in the JB due to OOO packet to be filled
                    if (!IsCallbackPending(JB_NOTIFY_WAIT_FOR_OOO_PACKET_COMPLETE, NULL))
                    {
                        PVMF_JB_LOGDATATRAFFIC_OUT((0, "PVMFJitterBufferNode::SendData Detected Hole in JB PeekTs[%d] clientClock[%d] , ClientClockState [%d], estimatedServClock[%d], EstimatedServClockState[%d], oSessionDurationExpired[%d] MimeStr[%s], delta[%d]", converted_ts, clientClock, irClientPlayBackClock.GetState(), estimatedServClock, irEstimatedServerClock.GetState(), iEOSSignalled, irMimeType.get_cstr(), delta));
                        RequestEventCallBack(JB_NOTIFY_WAIT_FOR_OOO_PACKET_COMPLETE , delta, NULL);
                    }
                    return false;
                }
            }
            return true;
        }
        else
        {
            if (iEOSSignalled)
            {
                if (!iEOSSent)
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                CancelEventCallBack(JB_MONITOR_REBUFFERING);
                RequestEventCallBack(JB_MONITOR_REBUFFERING, (clockDiff - iRebufferingThresholdInMilliSeconds));
                return false;
            }
        }
    }
}

bool PVMFRTPJitterBufferImpl::CanRetrievePacket(PVMFSharedMediaMsgPtr& aMediaOutMsg, bool& aCmdPacket)
{
    aCmdPacket = false;
    LOGCLIENTANDESTIMATEDSERVCLK_DATAPATH_OUT;

    uint32 clockDiff = 0;
    //There's data in the Jb and delay is established with no hole in the next read position ||
    //JB is empty, EOS is already signalled, and EOS is not yet pushed out

    bool delayEstablished = IsDelayEstablished(clockDiff);
    if (!delayEstablished)
    {
        return false;
    }
    else
    {
        if (GetPendingCommand(aMediaOutMsg) == true)
        {
            aCmdPacket = true;
            return true;
        }

        PVMFJitterBufferStats stats = getJitterBufferStats();
        if (stats.currentOccupancy > 0)
        {
            if (iJitterBuffer->CheckCurrentReadPosition() == false)
            {
                uint32 in_wrap_count = 0;
                /*
                 * Peek Next timestamp
                 */
                PVMFTimestamp ts = peekNextElementTimeStamp();

                /*
                 * Convert Time stamp to milliseconds
                 */
                //todo mediaclockconverter

                ipMediaClockConverter->set_clock(ts, in_wrap_count);
                PVMFTimestamp converted_ts =
                    ipMediaClockConverter->get_converted_ts(1000);
                /*
                 * Get current client playback clock in milliseconds
                 */
                uint32 clientClock;
                bool overflowFlag = false;

                irClientPlayBackClock.GetCurrentTime32(clientClock, overflowFlag, PVMF_MEDIA_CLOCK_MSEC);


                uint32 estimatedServClock = 0;
                overflowFlag = false;
                irEstimatedServerClock.GetCurrentTime32(estimatedServClock, overflowFlag, PVMF_MEDIA_CLOCK_MSEC);
                uint32 delta = 0;
                if (!iEOSSignalled && (PVTimeComparisonUtils::IsEarlier(clientClock + OOO_DATAPATH_DELAY_FACTOR, converted_ts, delta) && (delta > 0)))
                {
                    //hold the available data packet, and wait for hole in the JB due to OOO packet to be filled
                    if (!IsCallbackPending(JB_NOTIFY_WAIT_FOR_OOO_PACKET_COMPLETE, NULL))
                    {
                        PVMF_JB_LOGDATATRAFFIC_OUT_E((0, "PVMFJitterBufferNode::SendData Detected Hole in JB PeekTs[%d] clientClock[%d] , ClientClockState [%d], estimatedServClock[%d], EstimatedServClockState[%d], oSessionDurationExpired[%d] MimeStr[%s], delta[%d]", converted_ts, clientClock, irClientPlayBackClock.GetState(), estimatedServClock, irEstimatedServerClock.GetState(), iEOSSignalled, irMimeType.get_cstr(), delta));
                        PVMF_JB_LOGDATATRAFFIC_OUT((0, "PVMFJitterBufferNode::SendData Detected Hole in JB PeekTs[%d] clientClock[%d] , ClientClockState [%d], estimatedServClock[%d], EstimatedServClockState[%d], oSessionDurationExpired[%d] MimeStr[%s], delta[%d]", converted_ts, clientClock, irClientPlayBackClock.GetState(), estimatedServClock, irEstimatedServerClock.GetState(), iEOSSignalled, irMimeType.get_cstr(), delta));
                        RequestEventCallBack(JB_NOTIFY_WAIT_FOR_OOO_PACKET_COMPLETE , delta, NULL);
                    }
                    return false;
                }
            }
            //Retrieve element from JB
            //Cancel pending sequencing OOO packet callback (if any)
            if (IsCallbackPending(JB_NOTIFY_WAIT_FOR_OOO_PACKET_COMPLETE, NULL))
            {
                CancelEventCallBack(JB_NOTIFY_WAIT_FOR_OOO_PACKET_COMPLETE, NULL);
            }
            PVMFTimestamp ts;
            uint32 converted_ts;
            uint32 in_wrap_count = 0;

            PVMFSharedMediaDataPtr mediaOut = RetrievePacketPayload();

            if (mediaOut.GetRep() != NULL)
            {
                ts = mediaOut->getTimestamp();

                ipMediaClockConverter->set_clock(ts, in_wrap_count);
                converted_ts =
                    ipMediaClockConverter->get_converted_ts(1000);
                mediaOut->setTimestamp(converted_ts);
                iLastPacketOutTs = converted_ts;
                mediaOut->setFormatSpecificInfo(iTrackConfig);
                convertToPVMFMediaMsg(aMediaOutMsg, mediaOut);
            }

            LOGCLIENTANDESTIMATEDSERVCLK_DATAPATH_OUT;
            PVMF_JB_LOGDATATRAFFIC_OUT((0, "PVMFJitterBufferNode::SendData: Mime=%s, SSRC=%d, TS=%d, SEQNUM= %d",
                                        irMimeType.get_cstr(),
                                        aMediaOutMsg->getStreamID(),
                                        aMediaOutMsg->getTimestamp(),
                                        aMediaOutMsg->getSeqNum()));

            PVMF_JB_LOGDATATRAFFIC_FLOWCTRL((0, "PVMFJBNode::SendData:"
                                             "MaxSNReg=%d, MaxTSReg=%u, LastSNRet=%d, LastTSRet=%u, NumMsgsInJB=%d",
                                             stats.maxSeqNumRegistered, stats.maxTimeStampRegistered,
                                             stats.lastRetrievedSeqNum, stats.maxTimeStampRetrieved,
                                             (stats.maxSeqNumRegistered - stats.lastRetrievedSeqNum)));
            return true;



        }
        else
        {
            if (iEOSSignalled)
            {
                if (!iEOSSent)
                {
                    GenerateAndSendEOSCommand(aMediaOutMsg, aCmdPacket);
                    iEOSSent = true;
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                if (!IsCallbackPending(JB_MONITOR_REBUFFERING, NULL) && !iEOSSignalled)
                {
                    RequestEventCallBack(JB_MONITOR_REBUFFERING, (clockDiff - iRebufferingThresholdInMilliSeconds));
                }
                return false;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//Utility functions
//////////////////////////////////////////////////////////////////////////////////////////////////

void PVMFRTPJitterBufferImpl::UpdateEstimatedServerClock(bool oFreshStart)
{
    uint32 rtpTSInMS;
    uint32 currentTime32 = 0;
    uint32 currentTimeBase32 = 0;
    uint32 adjustTime32 = 0;
    bool overflowFlag = false;

    if (oFreshStart)
    {
        uint32 in_wrap_count = 0;
        iEstServClockMediaClockConvertor.set_clock(iMaxAdjustedRTPTS, in_wrap_count);
        rtpTSInMS = iEstServClockMediaClockConvertor.get_converted_ts(1000);
        irEstimatedServerClock.Stop();

        irEstimatedServerClock.SetStartTime32(rtpTSInMS,
                                              PVMF_MEDIA_CLOCK_MSEC, overflowFlag);
        irEstimatedServerClock.Start();
        PVMF_JB_LOGCLOCK((0, "PVMFJitterBufferImpl::UpdateEstimatedServerClock - Setting start time - MaxAdjustedRTPTS=%u, StartTime=%d",
                          iMaxAdjustedRTPTS, rtpTSInMS));
    }
    else
    {
        iEstServClockMediaClockConvertor.update_clock(iMaxAdjustedRTPTS);
        rtpTSInMS = iEstServClockMediaClockConvertor.get_converted_ts(1000);
        adjustTime32 = rtpTSInMS;
        bool overflowFlag = false;
        irEstimatedServerClock.GetCurrentTime32(currentTime32, overflowFlag,
                                                PVMF_MEDIA_CLOCK_MSEC,
                                                currentTimeBase32);

        irEstimatedServerClock.AdjustClockTime32(currentTime32,
                currentTimeBase32,
                adjustTime32,
                PVMF_MEDIA_CLOCK_MSEC,
                overflowFlag);

        irEstimatedServerClock.GetCurrentTime32(currentTime32, overflowFlag,
                                                PVMF_MEDIA_CLOCK_MSEC,
                                                currentTimeBase32);

        PVMF_JB_LOGINFO((0, "PVMFJitterBufferImpl::UpdateEstimatedServerClock - Mime=%s",
                         irMimeType.get_cstr()));
        PVMF_JB_LOGINFO((0, "PVMFJitterBufferImpl::UpdateEstimatedServerClock - EstServClock=%d",
                         currentTime32));
        PVMF_JB_LOGINFO((0, "PVMFJitterBufferImpl::UpdateEstimatedServerClock - RTPTime32=%u",
                         adjustTime32));
        PVMF_JB_LOGINFO((0, "PVMFJitterBufferImpl::UpdateEstimatedServerClock - Adjusting Clock - iMaxAdjustedRTPTS=%u, currentTimeBase32=%d",
                         iMaxAdjustedRTPTS, currentTimeBase32));
    }

}

void PVMFRTPJitterBufferImpl::ComputeMaxAdjustedRTPTS()
{
    PVMFJitterBufferStats jbStats = getJitterBufferStats();

    uint32 aSeqNum = jbStats.maxSeqNumReceived;
    PVMFTimestamp aTS = jbStats.maxTimeStampRegistered;

    PVMFRTPInfoParams* rtpInfoParams = FindRTPInfoParams(aSeqNum);

#if (PVLOGGER_INST_LEVEL > PVLOGMSG_INST_LLDBG)
    uint32 logSeqNum = rtpInfoParams ? (rtpInfoParams->seqNum) : 0;
    PVMF_JB_LOGINFO((0, "PVMFJitterBufferImpl::ComputeMaxAdjustedRTPTS - maxSeqNumReceived=%d, rtpInfoParams = %x, rtpInfoParams->seqNum =%d, iPrevSeqNumBaseIn=%d, Mime=%s",
                     aSeqNum, rtpInfoParams ? rtpInfoParams : 0, logSeqNum, iPrevSeqNumBaseIn, irMimeType.get_cstr()));
#endif
    uint16 diff = 0;
    if (rtpInfoParams && rtpInfoParams->seqNumBaseSet && IsSequenceNumEarlier(OSCL_STATIC_CAST(uint16, iPrevSeqNumBaseIn), OSCL_STATIC_CAST(uint16, rtpInfoParams->seqNum), diff))
    {
        iPrevSeqNumBaseIn = rtpInfoParams->seqNum;
        iPrevTSIn = rtpInfoParams->rtpTime;
    }
    iMaxAdjustedRTPTS += (aTS - iPrevAdjustedRTPTS);
    iPrevAdjustedRTPTS = aTS;

    PVMF_JB_LOGINFO((0, "PVMFJitterBufferImpl::ComputeMaxAdjustedRTPTS - maxTimeStampRegistered=%u, iPrevAdjustedRTPTS=%u, iMaxAdjustedRTPTS=%u, Mime=%s",
                     aTS, iPrevAdjustedRTPTS, iMaxAdjustedRTPTS, irMimeType.get_cstr()));

    UpdateEstimatedServerClock();
}

void PVMFRTPJitterBufferImpl::DeterminePrevTimeStampPeek(uint32 aSeqNum,
        PVMFTimestamp& aPrevTS)
{
    PVMFRTPInfoParams* rtpInfoParams = FindRTPInfoParams(aSeqNum);
    if (rtpInfoParams && (iPrevSeqNumBaseOut != rtpInfoParams->seqNum))
    {
        aPrevTS = rtpInfoParams->rtpTime;
    }
    else
    {
        //iPrevTsOut would have been updated correctly by now in the function
        //(i)RetrievePacketPayload (if iPrevSeqNumBaseOut != rtpInfoParams->seqNum)
        //(ii)[CheckForRTPTimeAndRTPSeqNumberBase or setRTPInfoParams](if server didn't provided the RTPInfo in play response and hence rtpInfoParams is NULL)
        //So, let the iPrevTsOut be unchanged.
        aPrevTS = iPrevTSOut;
    }
}

bool PVMFRTPJitterBufferImpl::IsSequenceNumEarlier(uint16 aSeqNumToComp, uint16 aBaseSeqNum, uint16& aDiff)
{
    PVMF_JB_LOGINFO((0, "PVMFRTPJitterBufferImpl::IsSequenceNumEarlier aSeqNumToComp[%d] , aBaseSeqNum[%d], aDiff[%d]", aSeqNumToComp , aBaseSeqNum, aDiff));
    aDiff = aBaseSeqNum - aSeqNumToComp;
    if ((aDiff != 0) && (aDiff < RTPSEQNUM_ROLLOVER_WRAP_THRESHHOLD_16BIT))
    {
        return true;
    }
    aDiff = 0 - aDiff;
    return false;
}

PVMFStatus PVMFRTPJitterBufferImpl::PerformFlowControl(bool aIncomingPacket)
{
    OSCL_UNUSED_ARG(aIncomingPacket);
    return PVMFSuccess;
}

void PVMFRTPJitterBufferImpl::DeterminePrevTimeStamp(uint32 aSeqNum)
{

    PVMFRTPInfoParams* rtpInfoParams = FindRTPInfoParams(aSeqNum);
    //No need to check for seqnum set here, FindRTPInfoParams is expected to return RTPInfoParams with seqbase set only
    if (rtpInfoParams && (iPrevSeqNumBaseOut != rtpInfoParams->seqNum))
    {
        /* We need to adjust iMonotonicTimeStamp as well for resume */
        if (rtpInfoParams->isPlayAfterPause && rtpInfoParams->nptTimeBaseSet && isPrevNptTimeSet && isPrevRtpTimeSet)
        {
            uint64 CurrNptTime, PrevNptTime;
            Oscl_Int64_Utils::set_uint64(CurrNptTime, 0, rtpInfoParams->nptTimeInRTPTimeScale);
            Oscl_Int64_Utils::set_uint64(PrevNptTime, 0, iPrevNptTimeInRTPTimeScale + iPrevTSOut - iPrevRtpTimeBase);
            iMonotonicTimeStamp = iMonotonicTimeStamp + CurrNptTime - PrevNptTime;
            PVMF_JB_LOGINFO((0, "PVMFJitterBufferImpl::DeterminePrevTimeStamp: TS adjustment for resume"
                             "nptTimeInRTPTimeScale=%d, iPrevNptTimeInRTPTimeScale=%d, iPrevRtpTimeBase=%d, iPrevTSOut=%d", rtpInfoParams->nptTimeInRTPTimeScale, iPrevNptTimeInRTPTimeScale, iPrevRtpTimeBase, iPrevTSOut));
        }
        if (rtpInfoParams->nptTimeBaseSet)
        {
            iPrevNptTimeInRTPTimeScale = rtpInfoParams->nptTimeInRTPTimeScale;
            isPrevNptTimeSet = true;
        }
        else
        {
            isPrevNptTimeSet = false;
        }
        if (rtpInfoParams->rtpTimeBaseSet)
        {
            iPrevRtpTimeBase = rtpInfoParams->rtpTime;
            isPrevRtpTimeSet = true;
        }
        else
        {
            isPrevRtpTimeSet = false;
        }
        PVMF_JB_LOGINFO((0, "PVMFJitterBufferImpl::DeterminePrevTimeStamp: TS update for "
                         "iPrevNptTimeInRTPTimeScale=%d, iPrevRtpTimeBase=%d", rtpInfoParams->nptTimeInRTPTimeScale, rtpInfoParams->rtpTime));
        iPrevSeqNumBaseOut = rtpInfoParams->seqNum;
        iPrevTSOut = rtpInfoParams->rtpTime;
    }
    else
    {
        //iPrevTsOut would have been updated correctly by now in the function
        //(i)RetrievePacketPayload (if iPrevSeqNumBaseOut != rtpInfoParams->seqNum)
        //(ii)[CheckForRTPTimeAndRTPSeqNumberBase or setRTPInfoParams](if server didn't provided the RTPInfo in play response and hence rtpInfoParams is NULL)
        //So, let the iPrevTsOut be unchanged.
    }
    PVMF_JB_LOGINFO((0, "PVMFJitterBufferImpl::DeterminePrevTimeStamp: MimeStr %s, RTPInfoSeqNum=%d, iPrevSeqNumBaseOut=%d, iPrevTSOut=%u",
                     irMimeType.get_cstr(), rtpInfoParams ? rtpInfoParams->seqNum : 0, iPrevSeqNumBaseOut, iPrevTSOut));
}

void PVMFRTPJitterBufferImpl::ReportJBInfoEvent(PVMFAsyncEvent& aEvent)
{
    if (PVMFInfoUnderflow == aEvent.GetEventType())
    {
        CancelEventCallBack(JB_BUFFERING_DURATION_COMPLETE);
        RequestEventCallBack(JB_BUFFERING_DURATION_COMPLETE);
    }
    else if (PVMFInfoDataReady == aEvent.GetEventType())
    {
        CancelEventCallBack(JB_BUFFERING_DURATION_COMPLETE);
    }
    PVMFJitterBufferImpl::ReportJBInfoEvent(aEvent);
}

PVMFRTPInfoParams* PVMFRTPJitterBufferImpl::FindRTPInfoParams(uint32 aSeqNum)
{
    //Repos use case OR no pause in streaming session use case
    if (iRTPInfoParamsVec.size() == 1)
    {
        PVMFRTPInfoParams* pRTPInfoParam = iRTPInfoParamsVec.begin();
        if (pRTPInfoParam && (pRTPInfoParam->seqNumBaseSet))
            return pRTPInfoParam;
        else
            return NULL;
    }

    //Pause-Resume use case.
    //1. Purge stail entries in RTPInfo vector
    //   If the number of entries in the vect are more than one
    //   Traverse the vector from bottom to top (earliest RTPInfo to latest RTPInfo)
    //   Purge the RTPInfo if
    //   (aSeqNum - RTPInfo's seqnum) >= RTPSEQNUM_ROLLOVER_WRAP_THRESHHOLD_16BIT in unsigned arithmatic
    //2. Determine correct RTPInfo
    //   Traverse RTP entries from latest to earliest and get the one that satisfies
    //   RTPInfo's seqnum >= aSeqNum in unsigned arithmatic


    /////////////////////Purge stail entries///////////////////////
    bool rtpInfoParamsMustBeAvl = false;
    bool continuePurging = (iRTPInfoParamsVec.size() > 1);
    while (continuePurging)
    {
        Oscl_Vector<PVMFRTPInfoParams, OsclMemAllocator>::iterator it;
        for (it = iRTPInfoParamsVec.begin();
                it < iRTPInfoParamsVec.end();
                it++)
        {
            if ((it->seqNumBaseSet) && ((OSCL_STATIC_CAST(uint16, aSeqNum) - OSCL_STATIC_CAST(uint16, it->seqNum)) >= RTPSEQNUM_ROLLOVER_WRAP_THRESHHOLD_16BIT))
            {
                rtpInfoParamsMustBeAvl = true;
                PVMF_JB_LOGRTPINFO_I((0, "PVMFRTPJitterBufferImpl::FindRTPInfoParams Purging RTPInfo MimeStr[%s] SeqNum[%u] RTPSeqNum[%u]", irMimeType.get_cstr(), aSeqNum, it->seqNum));
                iLatestPurgedRTPInfoParams.isPlayAfterPause = it->isPlayAfterPause;
                iLatestPurgedRTPInfoParams.nptTimeBaseSet = it->nptTimeBaseSet;
                iLatestPurgedRTPInfoParams.nptTimeInMS = it->nptTimeInMS;
                iLatestPurgedRTPInfoParams.nptTimeInRTPTimeScale = it->nptTimeInRTPTimeScale;
                iLatestPurgedRTPInfoParams.rtpTime = it->rtpTime;
                iLatestPurgedRTPInfoParams.rtpTimeBaseSet = it->rtpTimeBaseSet;
                iLatestPurgedRTPInfoParams.rtpTimeScale = it->rtpTimeScale;
                iLatestPurgedRTPInfoParams.seqNum = it->seqNum;
                iLatestPurgedRTPInfoParams.seqNumBaseSet = it->seqNumBaseSet;
                iRTPInfoParamsVec.erase(it);
            }
            else
            {
                continuePurging = false;
            }
            break;
        }
        continuePurging = ((iRTPInfoParamsVec.size() > 1) && continuePurging);
    }

    /////////////////////Look for correct RTPInfo///////////////////
    PVMFRTPInfoParams* retVal = NULL;
    Oscl_Vector<PVMFRTPInfoParams, OsclMemAllocator>::iterator it;

    if (iRTPInfoParamsVec.size() > 0)
    {
        for (it = iRTPInfoParamsVec.end() - 1;
                it >= iRTPInfoParamsVec.begin();
                it--)
        {
            uint16 diff = 0;
            if (it->seqNumBaseSet)
            {
                rtpInfoParamsMustBeAvl = true;
                bool seqEarlier = IsSequenceNumEarlier(it->seqNum, aSeqNum, diff);
                if (seqEarlier || (0 == diff))
                {
                    retVal = it;
                    break;
                }
            }
        }
    }

    PVMF_JB_LOGRTPINFO_I((0, "PVMFRTPJitterBufferImpl::FindRTPInfoParams Mime [%s] SeqNum[%u] RTPSeqNum[%u]", irMimeType.get_cstr(), aSeqNum, retVal ? (retVal->seqNum) : 0xFFFFFFFF));
    if ((NULL == retVal) && (iLatestPurgedRTPInfoParams.seqNumBaseSet))
    {
        //It is possible not to get the RTPInfo under the following use-case.
        //RTP-Info's received for the session 0, 42000
        //On receving the packet with seqnum >=42000. RTPInfo with seqnum 0 will be purged.
        //Now, suppose after purging we get the packet with the sequence num <42000 say 39999
        //Now, even tho packet is not that late as per the JB logic, and is just OOO
        //packet. We should still could be able to get the correct RTP_Info for it.
        //So, we better persist the latest purged RTPInfo.
        retVal = &iLatestPurgedRTPInfoParams;
    }

    if (!retVal && rtpInfoParamsMustBeAvl)
    {
        //If RTPInfo is provided by the server and still we cannot locate the RTPInof. There's something wrong with the input seqnum or the purging logic
        PVMF_JB_LOGRTPINFO_I((0, "PVMFRTPJitterBufferImpl::FindRTPInfoParams Critical ERROR -- Mime [%s] SeqNum[%u] RTPInfoVectSz[%d] PurgedRTPInfoSet[%d]", irMimeType.get_cstr(), aSeqNum, iRTPInfoParamsVec.size(), (iLatestPurgedRTPInfoParams.seqNumBaseSet ? true : false)));
        OSCL_LEAVE(OsclErrArgument);
    }

    return retVal;
}

PayloadParserStatus PVMFRTPJitterBufferImpl::ParsePayload(PVMFSharedMediaMsgPtr& aMediaMsgPtr)
{
    PVMFSharedMediaDataPtr mediaData;
    convertToPVMFMediaData(mediaData, aMediaMsgPtr);

    IPayloadParser::Payload payload; //Input of Payload Parser

    payload.vfragments.clear();
    payload.stream = mediaData->getStreamID();
    payload.marker = (mediaData->getMarkerInfo() & PVMF_MEDIA_DATA_MARKER_INFO_M_BIT);
    if (mediaData->getMarkerInfo() & PVMF_MEDIA_DATA_MARKER_INFO_RANDOM_ACCESS_POINT_BIT)
    {
        payload.randAccessPt = true;
    }

    payload.sequence = mediaData->getSeqNum();
    payload.timestamp = mediaData->getTimestamp();

    //Persist the format specific info and the FormatId <may be required in the use case where Parser may push out multiple Access unit(s) say for H264>
    iCurrFormatId = mediaData->getFormatID();

    for (uint f = 0; f < mediaData->getNumFragments(); f++) //ideally there'll b only one frag in RTP use case
    {
        OsclRefCounterMemFrag memFrag;
        mediaData->getMediaFragment(f, memFrag);
        payload.vfragments.push_back(memFrag);
    }

#if (PVLOGGER_INST_LEVEL > PVLOGMSG_INST_LLDBG)
    uint32 inputDataSize = 0;
    uint32 inputNumFrags = mediaData->getNumFragments();
    for (uint32 i = 0; i < inputNumFrags; i++)
    {
        OsclRefCounterMemFrag memFrag;
        mediaData->getMediaFragment(i, memFrag);
        inputDataSize += memFrag.getMemFragSize();
    }
    uint32 timebase32 = 0;
    uint32 clientClock32 = 0;
    bool overflowFlag = false;
    irClientPlayBackClock.GetCurrentTime32(clientClock32, overflowFlag, PVMF_MEDIA_CLOCK_MSEC, timebase32);
    PVMF_JB_LOG_PP_INPUT((0,
                          "PVMFRTPJitterBufferImpl::ParsePayload - Input: MimeType=%s, TS=%d, SEQNUM=%d, Stream ID = %d, Format id=%d, SIZE=%d, MarkerInfo=0x%x, RandomAccessbit=%d, Clock=%d",
                          irMimeType.get_cstr(), mediaData->getTimestamp() , mediaData->getSeqNum(), mediaData->getStreamID(), mediaData->getFormatID(), inputDataSize, mediaData->getMarkerInfo(), payload.randAccessPt, clientClock32));


#endif

    //Assumption: Each RTP payload will have one access unit in it(to be parsed by Payload Parser)
    iAccessUnits.clear();

    PayloadParserStatus retval = PayloadParserStatus_InputNotExhausted;
    while (retval == PayloadParserStatus_InputNotExhausted)
    {
        retval = iPayLoadParser->Parse(payload, iAccessUnits);
        if ((retval == PayloadParserStatus_MemorAllocFail) ||
                (retval == PayloadParserStatus_Failure))
        {
            PVUuid eventuuid = PVMFJitterBufferNodeEventTypeUUID;
            int32 infocode = PVMFJitterBufferNodePayloadParserError;

            PVMFBasicErrorInfoMessage* eventmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (infocode, eventuuid, NULL));

            PVMFAsyncEvent event(PVMFInfoEvent, PVMFErrProcessing, OSCL_STATIC_CAST(PVInterface*, eventmsg), (OsclAny*)irMimeType.get_cstr(), NULL, 0);
            PVMFJitterBufferImpl::ReportJBInfoEvent(event);
        }
    }

    if (retval != PayloadParserStatus_Success)
    {
        return retval;
    }

    if ((iAccessUnits.size() > 1) && (!ipMediaDataGroupAllocOutputMediaMsg))
    {
        OSCL_LEAVE(PVMFErrNotSupported);
    }

    GetParsedMediaMsg(aMediaMsgPtr);

    return retval;
}

PVMFStatus PVMFRTPJitterBufferImpl::GetParsedMediaMsg(PVMFSharedMediaMsgPtr& aMediaMsgPtr)
{
    //Packetize the parsed payload to media msg.
    PVMFStatus status = PVMFErrNotReady;
    if (iAccessUnits.size() > 0)
    {
        IPayloadParser::Payload& parsedAccessUnit = iAccessUnits.front();
        // fill the data implementation with the contents of the payload structure
        uint32 markerInfo = 0;
        if (parsedAccessUnit.marker == 1)
        {
            markerInfo |= PVMF_MEDIA_DATA_MARKER_INFO_M_BIT;
        }
        if (parsedAccessUnit.randAccessPt)
        {
            markerInfo |= PVMF_MEDIA_DATA_MARKER_INFO_RANDOM_ACCESS_POINT_BIT;
        }
        if (parsedAccessUnit.endOfNAL)
        {
            markerInfo |= PVMF_MEDIA_DATA_MARKER_INFO_END_OF_NAL_BIT;
        }

        PVMFSharedMediaDataPtr mediaData;

        OsclSharedPtr<PVMFMediaDataImpl> media_data_impl_ptr;
        PVMFSharedMediaDataPtr mediaDataOut;

        if (ipMediaDataGroupAllocOutputMediaMsg)
        {
            AllocateMediaDataGroupMediaMsg(media_data_impl_ptr);
        }
        else
        {
            convertToPVMFMediaData(mediaData, aMediaMsgPtr);
            mediaData->getMediaDataImpl(media_data_impl_ptr);
            media_data_impl_ptr->clearMediaFragments();
        }

        media_data_impl_ptr->setMarkerInfo(markerInfo);
        media_data_impl_ptr->setRandomAccessPoint(parsedAccessUnit.randAccessPt);

        for (uint ii = 0; ii < parsedAccessUnit.vfragments.size(); ii++)
        {
            media_data_impl_ptr->appendMediaFragment(parsedAccessUnit.vfragments[ii]);
        }

        if (ipMediaDataGroupAllocOutputMediaMsg)
        {
            // create a data message
            PVMFMediaMsgHeader newMsgHeader;
            newMsgHeader.format_id = iCurrFormatId;
            mediaDataOut = PVMFMediaData::createMediaData(media_data_impl_ptr, &newMsgHeader);
            mediaDataOut->setFormatSpecificInfo(iTrackConfig);
        }
        else
        {
            mediaDataOut = mediaData;
        }

        mediaDataOut->setSeqNum(parsedAccessUnit.sequence);
        mediaDataOut->setStreamID(parsedAccessUnit.stream);
        mediaDataOut->setTimestamp(parsedAccessUnit.timestamp);

#if (PVLOGGER_INST_LEVEL > PVLOGMSG_INST_LLDBG)
        {
            uint32 timebase32 = 0;
            uint32 clientClock32 = 0;
            bool overflowFlag = false;
            irClientPlayBackClock.GetCurrentTime32(clientClock32, overflowFlag, PVMF_MEDIA_CLOCK_MSEC, timebase32);

            /* Get size for log purposes */
            uint32 numFrags = mediaDataOut->getNumFragments();
            uint32 size = 0;
            OsclRefCounterMemFrag memFrag;
            for (uint32 sizecount = 0; sizecount < numFrags; sizecount++)
            {
                mediaDataOut->getMediaFragment(sizecount, memFrag);
                size += memFrag.getMemFragSize();
            }

            PVMF_JB_LOG_PP_OUTPUT((0,
                                   "PVMFRTPJitterBufferImpl::ParsePayload - Output: "
                                   "MimeType=%s, TS=%d, SEQNUM=%d, Stream ID=%d, FormatID=%d, SIZE=%d, MBIT=0x%x, RandAccessBit=%d, EndOfNALBit=%d, Clock=%d Delta=%d",
                                   irMimeType.get_cstr(), mediaDataOut->getTimestamp(), mediaDataOut->getSeqNum(),
                                   mediaDataOut->getStreamID(), mediaDataOut->getFormatID(), size,
                                   (mediaDataOut->getMarkerInfo() & PVMF_MEDIA_DATA_MARKER_INFO_M_BIT),
                                   (mediaDataOut->getMarkerInfo() & PVMF_MEDIA_DATA_MARKER_INFO_RANDOM_ACCESS_POINT_BIT),
                                   (mediaDataOut->getMarkerInfo() & PVMF_MEDIA_DATA_MARKER_INFO_END_OF_NAL_BIT),
                                   clientClock32, (mediaDataOut->getTimestamp() - clientClock32)));
        }
#endif

        convertToPVMFMediaMsg(aMediaMsgPtr, mediaDataOut);
        iAccessUnits.erase(iAccessUnits.begin());
        status = PVMFSuccess;
    }
    return status;
}

OSCL_EXPORT_REF uint32 PVMFRTPJitterBufferImpl::GetPrevSampleDuration(uint32 aCurrentSampleRTPTime, uint32 aPrevSampleRTPTime, uint32 aCurrPacketSequenceNum)
{
    uint32 duration = 0;
    bool isEarly = PVTimeComparisonUtils::IsEarlier(aCurrentSampleRTPTime, aPrevSampleRTPTime, duration);
    PVMF_JB_LOGDATATRAFFIC_OUT((0, "PVMFRTPJitterBufferImpl::GetPrevSampleDuration aCurrentSampleRTPTime [%u] aPrevSampleRTPTime [%u] Duration[%u] SequenceNo[%u] Early[%d]", aCurrentSampleRTPTime, aPrevSampleRTPTime, duration, aCurrPacketSequenceNum, isEarly));
    if (isEarly && (duration != 0))
    {
        duration = 0;
        PVMFRTPInfoParams* rtpInfoToRefer = FindRTPInfoParams(aCurrPacketSequenceNum);
        if (!rtpInfoToRefer || (rtpInfoToRefer->rtpTime != aPrevSampleRTPTime))
        {
            //Curr ts is allowed to be earlier only in the use case where:
            //Client received the rtp info from the server.
            //the rtptime of the first valid packet in the streaming session is less than the rtpinof's rtptime
            //Log the error here and send duration to be zero.
            //For debug build assert here
            PVMF_JB_LOGDATATRAFFIC_OUT_E((0, "PVMFRTPJitterBufferImpl::GetPrevSampleDuration aCurrentSampleRTPTime [%u] aPrevSampleRTPTime [%u] Duration[%u] SequenceNo[%u] Early[%d] RTPInfoTs[%u]", aCurrentSampleRTPTime, aPrevSampleRTPTime, duration, aCurrPacketSequenceNum, isEarly, rtpInfoToRefer->rtpTime));
            OSCL_ASSERT(false);
            PVMFAsyncEvent jbEvent(PVMFInfoEvent, PVMFErrInvalidRTPTimeInPkt, NULL, NULL);
            ReportJBInfoEvent(jbEvent);
            return duration;
        }
    }
    return duration;
}

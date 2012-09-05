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
#include "omxenctest.h"

/*
 * Active Object class's Run () function
 * Control all the states of AO & sends API's to the component
 */

void OmxEncTestEosMissing::Run()
{
    switch (iState)
    {
        case StateUnLoaded:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "OmxEncTestEosMissing::Run() - StateUnLoaded IN"));

            OMX_ERRORTYPE Err;
            OMX_U32 PortIndex, ii;

            if (!iCallbacks->initCallbacks())
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "OmxEncTestEosMissing::RunLL() - ERROR initCallbacks failed, OUT"));
                StopOnError();
                break;
            }

            ipAppPriv = (AppPrivateType*) oscl_malloc(sizeof(AppPrivateType));
            CHECK_MEM(ipAppPriv, "Component_Handle");

            //This should be the first call to the component to load it.
            Err = OMX_MasterInit();
            CHECK_ERROR(Err, "OMX_MasterInit");
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0, "OmxEncTestEosMissing::Run() - OMX_MasterInit done"));



            if (NULL != iRole)
            {
                //Determine the component first & then get the handle
                OMX_U32 NumComps = 0;
                OMX_STRING* pCompOfRole;

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0, "OmxEncTestEosMissing::Run() - Finding out the role for the component %s", iRole));

                // call once to find out the number of components that can fit the role
                Err = OMX_MasterGetComponentsOfRole(iRole, &NumComps, NULL);

                if (OMX_ErrorNone != Err || NumComps < 1)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "OmxEncTestEosMissing::Run() - ERROR, No component can handle the specified role %s", iRole));
                    StopOnError();
                    ipAppPriv->Handle = NULL;
                    break;
                }

                pCompOfRole = (OMX_STRING*) oscl_malloc(NumComps * sizeof(OMX_STRING));
                CHECK_MEM(pCompOfRole, "ComponentRoleArray");

                for (ii = 0; ii < NumComps; ii++)
                {
                    pCompOfRole[ii] = (OMX_STRING) oscl_malloc(PV_OMX_MAX_COMPONENT_NAME_LENGTH * sizeof(OMX_U8));
                    CHECK_MEM(pCompOfRole[ii], "ComponentRoleArray");
                }

                if (StateError == iState)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "OmxEncTestEosMissing::Run() - Error occured in this state, StateUnLoaded OUT"));
                    RunIfNotReady();
                    break;
                }

                // call 2nd time to get the component names
                Err = OMX_MasterGetComponentsOfRole(iRole, &NumComps, (OMX_U8**) pCompOfRole);
                CHECK_ERROR(Err, "GetComponentsOfRole");

                for (ii = 0; ii < NumComps; ii++)
                {
                    // try to create component
                    Err = OMX_MasterGetHandle(&ipAppPriv->Handle, (OMX_STRING) pCompOfRole[ii], (OMX_PTR) this, iCallbacks->getCallbackStruct());
                    // if successful, no need to continue
                    if ((OMX_ErrorNone == Err) && (NULL != ipAppPriv->Handle))
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0, "OmxEncTestEosMissing::Run() - Got Handle for the component %s", pCompOfRole[ii]));
                        break;
                    }
                    else
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "OmxEncTestEosMissing::Run() - ERROR, Cannot get component %s handle, try another if possible", pCompOfRole[ii]));
                    }

                }
                // whether successful or not, need to free CompOfRoles
                for (ii = 0; ii < NumComps; ii++)
                {
                    oscl_free(pCompOfRole[ii]);
                    pCompOfRole[ii] = NULL;
                }
                oscl_free(pCompOfRole);
                pCompOfRole = NULL;

                // check if there was a problem
                CHECK_ERROR(Err, "GetHandle");
                CHECK_MEM(ipAppPriv->Handle, "ComponentHandle");

            }

            ipInputFile = fopen(iInputFileName, "rb");
            CHECK_MEM(ipInputFile, "Input_FileName");

            ipOutputFile = fopen(iOutputFileName, "wb");
            CHECK_MEM(ipOutputFile, "Output_FileName")


            //This will initialize the size and version of the iPortInit structure
            INIT_GETPARAMETER_STRUCT(OMX_PORT_PARAM_TYPE, iPortInit);

            if ((0 == oscl_strcmp(iFormat, "AMRNB")) || (0 == oscl_strcmp(iFormat, "AAC")))
            {
                if (ipInputFile)
                {
                    fseek(ipInputFile, 0, SEEK_END);
                    iInputFileSize = ftell(ipInputFile);
                    fseek(ipInputFile, 0, SEEK_SET);
                }

                Err = OMX_GetParameter(ipAppPriv->Handle, OMX_IndexParamAudioInit, &iPortInit);
            }
            else
            {
                Err = OMX_GetParameter(ipAppPriv->Handle, OMX_IndexParamVideoInit, &iPortInit);
            }

            CHECK_ERROR(Err, "GetParameter_Audio/Video_Init");

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0, "OmxEncTestEosMissing::Run() - GetParameter called for OMX_IndexParamAudioInit/OMX_IndexParamVideoInit"));

            for (ii = 0; ii < iPortInit.nPorts; ii++)
            {
                PortIndex = iPortInit.nStartPortNumber + ii;

                //This will initialize the size and version of the iParamPort structure
                INIT_GETPARAMETER_STRUCT(OMX_PARAM_PORTDEFINITIONTYPE, iParamPort);
                iParamPort.nPortIndex = PortIndex;

                Err = OMX_GetParameter(ipAppPriv->Handle, OMX_IndexParamPortDefinition, &iParamPort);
                CHECK_ERROR(Err, "GetParameter_IndexParamPortDefinition");
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0, "OmxEncTestEosMissing::Run() - GetParameter called for OMX_IndexParamPortDefinition on port %d", PortIndex));

                if (0 == iParamPort.nBufferCountMin)
                {
                    /* a buffer count of 0 is not allowed */
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "OmxEncTestEosMissing::Run() - Error, GetParameter for OMX_IndexParamPortDefinition returned 0 min buffer count"));
                    StopOnError();
                    break;
                }

                if (iParamPort.nBufferCountMin > iParamPort.nBufferCountActual)
                {
                    /* Min buff count can't be more than actual buff count */
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "OmxEncTestEosMissing::Run() - ERROR, GetParameter for OMX_IndexParamPortDefinition returned actual buffer count %d less than min buffer count %d", iParamPort.nBufferCountActual, iParamPort.nBufferCountMin));
                    StopOnError();
                    break;
                }

                if (OMX_DirInput == iParamPort.eDir)
                {
                    iInputPortIndex = PortIndex;

                    iInBufferSize = iParamPort.nBufferSize;
                    iInBufferCount = iParamPort.nBufferCountActual;
                    iParamPort.nBufferCountActual = iInBufferCount;

                    if ((0 == oscl_strcmp(iFormat, "M4V")) | (0 == oscl_strcmp(iFormat, "H264")))
                    {
                        //Assign a default frame size, will change later based on color format
                        OMX_U32 InputFrameSize = (iFrameWidth * iFrameHeight * 3);

                        if (OMX_COLOR_Format24bitRGB888 == iColorFormat)
                        {
                            InputFrameSize = (iFrameWidth * iFrameHeight * 3);
                        }
                        else if (OMX_COLOR_Format12bitRGB444 == iColorFormat)
                        {
                            InputFrameSize = (iFrameWidth * iFrameHeight * 2);
                        }
                        else if (OMX_COLOR_FormatYUV420Planar == iColorFormat)
                        {
                            InputFrameSize = (iFrameWidth * iFrameHeight * 3) >> 1;
                        }
                        else if (OMX_COLOR_FormatYUV420SemiPlanar == iColorFormat)
                        {
                            InputFrameSize = (iFrameWidth * iFrameHeight * 3) >> 1;
                        }
                        else if ((OMX_COLOR_FormatYCbYCr == iColorFormat) ||
                                 (OMX_COLOR_FormatYCrYCb == iColorFormat) ||
                                 (OMX_COLOR_FormatCbYCrY == iColorFormat) ||
                                 (OMX_COLOR_FormatCrYCbY == iColorFormat))
                        {
                            InputFrameSize = (iFrameWidth * iFrameHeight * 2);

                        }
                        else
                        {
                            //We do not handle more color formats, return an error
                            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                            (0, "OmxEncTestEosMissing::Run() - Unsupported Color Format %d, Returning Error", iColorFormat));
                            StopOnError();
                            break;
                        }

                        iInBufferSize = InputFrameSize;

                        //Set the input parameters to the encoder component
                        iParamPort.nPortIndex = PortIndex;
                        iParamPort.nBufferSize = InputFrameSize;

                        iParamPort.format.video.nFrameWidth = iFrameWidth;
                        iParamPort.format.video.nFrameHeight = iFrameHeight;
                        iParamPort.format.video.xFramerate = iFrameRate << 16;
                        iParamPort.format.video.eColorFormat = iColorFormat;

                    }

                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                    (0, "OmxEncTestEosMissing::Run() - GetParameter returned Num of input buffers %d with Size %d", iInBufferCount, iInBufferSize));

                }
                else if (OMX_DirOutput == iParamPort.eDir)
                {
                    iOutputPortIndex = PortIndex;

                    iOutBufferSize = iParamPort.nBufferSize;
                    iOutBufferCount = iParamPort.nBufferCountActual;

                    iParamPort.nBufferCountActual = iOutBufferCount;

                    if ((0 == oscl_strcmp(iFormat, "M4V")) | (0 == oscl_strcmp(iFormat, "H264")))
                    {
                        iParamPort.nPortIndex = PortIndex;
                        iParamPort.format.video.nFrameWidth = iTgtFrameWidth;
                        iParamPort.format.video.nFrameHeight = iTgtFrameHeight;
                        iParamPort.format.video.xFramerate = iTgtFrameRate << 16;
                        iParamPort.format.video.nBitrate = iTgtBitRate;

                    }

                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                    (0, "OmxEncTestEosMissing::Run() - GetParameter returned Num of output buffers %d with Size %d", iOutBufferCount, iOutBufferSize));
                }

                //Take the buffer parameters of what component has specified
                iParamPort.nPortIndex = PortIndex;
                Err = OMX_SetParameter(ipAppPriv->Handle, OMX_IndexParamPortDefinition, &iParamPort);
                CHECK_ERROR(Err, "SetParameter_OMX_IndexParamPortDefinition");

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::Run() - SetParameter called for OMX_IndexParamPortDefinition on port %d", PortIndex));
            }

            if (StateError == iState)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "OmxEncTestEosMissing::Run() - Error Exiting the test case, OUT"));
                RunIfNotReady();
                break;
            }

#if PROXY_INTERFACE
            ipThreadSafeHandlerEventHandler = OSCL_NEW(OmxEncEventHandlerThreadSafeCallbackAO, (this, EVENT_HANDLER_QUEUE_DEPTH, "EventHandlerAO"));
            ipThreadSafeHandlerEmptyBufferDone = OSCL_NEW(OmxEncEmptyBufferDoneThreadSafeCallbackAO, (this, iInBufferCount, "EmptyBufferDoneAO"));
            ipThreadSafeHandlerFillBufferDone = OSCL_NEW(OmxEncFillBufferDoneThreadSafeCallbackAO, (this, iOutBufferCount, "FillBufferDoneAO"));

            if ((NULL == ipThreadSafeHandlerEventHandler) ||
                    (NULL == ipThreadSafeHandlerEmptyBufferDone) ||
                    (NULL == ipThreadSafeHandlerFillBufferDone))
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "OmxEncTestEosMissing::Run() - Error ThreadSafe Callback Handler initialization failed, OUT"));

                iState = StateUnLoaded;
                OsclExecScheduler* sched = OsclExecScheduler::Current();
                sched->StopScheduler();
            }
#endif

            if (0 == oscl_strcmp(iFormat, "AMRNB"))
            {
                INIT_GETPARAMETER_STRUCT(OMX_AUDIO_PARAM_PCMMODETYPE, iPcmMode);
                iPcmMode.nPortIndex = iInputPortIndex;

                Err = OMX_GetParameter(ipAppPriv->Handle, OMX_IndexParamAudioPcm, &iPcmMode);
                CHECK_ERROR(Err, "GetParameter_ENC_IndexParamAudioPcm");

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::Run() - GetParameter called for OMX_IndexParamAudioPcm on port %d", iOutputPortIndex));

                //Set the input port parameters
                iPcmMode.nPortIndex = iInputPortIndex;

                iPcmMode.nChannels = iInputNumberOfChannels;
                iPcmMode.nBitPerSample = iInputBitsPerSample;
                iPcmMode.nSamplingRate = iInputSamplingRate;

                Err = OMX_SetParameter(ipAppPriv->Handle, OMX_IndexParamAudioPcm, &iPcmMode);
                CHECK_ERROR(Err, "SetParameter_ENC_IndexParamAudioPcm");

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::Run() - SetParameter called for OMX_IndexParamAudioPcm on port %d", iOutputPortIndex));

                /* Pass the output format type information to the  component*/

                INIT_GETPARAMETER_STRUCT(OMX_AUDIO_PARAM_AMRTYPE, iAmrParam);
                iAmrParam.nPortIndex = iOutputPortIndex;

                Err = OMX_GetParameter(ipAppPriv->Handle, OMX_IndexParamAudioAmr, &iAmrParam);
                CHECK_ERROR(Err, "GetParameter_ENC_IndexParamAudio");

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::Run() - GetParameter called for OMX_IndexParamAudio on port %d", iInputPortIndex));

                iAmrParam.nPortIndex = iOutputPortIndex;
                iAmrParam.eAMRFrameFormat = iOutputFormat;
                iAmrParam.eAMRBandMode = iOutputBandMode;

                Err = OMX_SetParameter(ipAppPriv->Handle, OMX_IndexParamAudioAmr, &iAmrParam);
                CHECK_ERROR(Err, "SetParameter_ENC_IndexParamAudio");

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::Run() - SetParameter called for OMX_IndexParamAudio on port %d", iInputPortIndex));
            }
            else if (0 == oscl_strcmp(iFormat, "M4V"))
            {
                INIT_GETPARAMETER_STRUCT(OMX_VIDEO_PARAM_MPEG4TYPE, iMpeg4Type);
                iMpeg4Type.nPortIndex = iOutputPortIndex;

                Err = OMX_GetParameter(ipAppPriv->Handle, OMX_IndexParamVideoMpeg4, &iMpeg4Type);
                CHECK_ERROR(Err, "GetParameter_MPEG4ENC_IndexParamVideoMpeg4");
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::Run() - GetParameter called for OMX_IndexParamVideoMpeg4 for Mpeg4/h263 on port %d", iOutputPortIndex));

                //Set the OMX_VIDEO_PARAM_MPEG4TYPE parameters
                iMpeg4Type.nPortIndex = iOutputPortIndex;
                iMpeg4Type.eProfile = iMpeg4Profile;
                iMpeg4Type.eLevel = iMpeg4Level;
                iMpeg4Type.nPFrames = iNumPFrames;
                iMpeg4Type.nMaxPacketSize = iMaxPacketSize;
                iMpeg4Type.bSVH = iShortHeaderFlag;
                iMpeg4Type.bReversibleVLC = iReversibleVLCFlag;
                iMpeg4Type.nTimeIncRes = iTimeIncRes;

                Err = OMX_SetParameter(ipAppPriv->Handle, OMX_IndexParamVideoMpeg4, &iMpeg4Type);
                CHECK_ERROR(Err, "SetParameter_MPEG4ENC_IndexParamVideoMpeg4");

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::Run() - SetParameter called for OMX_IndexParamVideoMpeg4 for Mpeg4/h263 on port %d", iOutputPortIndex));

                //OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE Settings
                INIT_GETPARAMETER_STRUCT(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE, iErrCorrType);
                iErrCorrType.nPortIndex = iOutputPortIndex;

                Err = OMX_GetParameter(ipAppPriv->Handle, OMX_IndexParamVideoErrorCorrection, &iErrCorrType);
                CHECK_ERROR(Err, "GetParameter_MPEG4ENC_IndexParamVideoErrorCorrection");
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::Run() - GetParameter called for OMX_IndexParamVideoErrorCorrection for Mpeg4/h263 on port %d", iOutputPortIndex));

                //Set the parameters now
                iErrCorrType.nPortIndex = iOutputPortIndex;
                iErrCorrType.bEnableDataPartitioning = iDataPartitioningFlag;
                iErrCorrType.bEnableResync = iResyncFlag;
                iErrCorrType.nResynchMarkerSpacing = iResynchMarkerSpacing;

                Err = OMX_SetParameter(ipAppPriv->Handle, OMX_IndexParamVideoErrorCorrection, &iErrCorrType);
                CHECK_ERROR(Err, "SetParameter_MPEG4ENC_IndexParamVideoErrorCorrection");

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::Run() - SetParameter called for OMX_IndexParamVideoErrorCorrection for Mpeg4/h263 on port %d", iOutputPortIndex));



                //OMX_VIDEO_PARAM_BITRATETYPE Settings
                INIT_GETPARAMETER_STRUCT(OMX_VIDEO_PARAM_BITRATETYPE, iBitRateType);
                iBitRateType.nPortIndex = iOutputPortIndex;

                Err = OMX_GetParameter(ipAppPriv->Handle, OMX_IndexParamVideoBitrate, &iBitRateType);
                CHECK_ERROR(Err, "GetParameter_MPEG4ENC_IndexParamVideoBitrate");

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::Run() - GetParameter called for OMX_IndexParamVideoBitrate for Mpeg4/h263 on port %d", iOutputPortIndex));
                //Set the parameters now
                iBitRateType.nPortIndex = iOutputPortIndex;
                iBitRateType.eControlRate = iRateControlType;

                Err = OMX_SetParameter(ipAppPriv->Handle, OMX_IndexParamVideoBitrate, &iBitRateType);
                CHECK_ERROR(Err, "SetParameter_MPEG4ENC_IndexParamVideoBitrate");
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::Run() - SetParameter called for OMX_IndexParamVideoBitrate for Mpeg4/h263 on port %d", iOutputPortIndex));


                //OMX_VIDEO_PARAM_QUANTIZATIONTYPE Settings
                INIT_GETPARAMETER_STRUCT(OMX_VIDEO_PARAM_QUANTIZATIONTYPE, iQuantParam);
                iQuantParam.nPortIndex = iOutputPortIndex;

                Err = OMX_GetParameter(ipAppPriv->Handle, OMX_IndexParamVideoQuantization, &iQuantParam);
                CHECK_ERROR(Err, "GetParameter_MPEG4ENC_IndexParamVideoQuant");

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::Run() - GetParameter called for OMX_IndexParamVideoQuantization for Mpeg4/h263 on port %d", iOutputPortIndex));
                //Set the parameters now
                iQuantParam.nPortIndex = iOutputPortIndex;
                iQuantParam.nQpI = iIQuant;
                iQuantParam.nQpP = iPQuant;

                Err = OMX_SetParameter(ipAppPriv->Handle, OMX_IndexParamVideoQuantization, &iQuantParam);
                CHECK_ERROR(Err, "SetParameter_MPEG4ENC_IndexParamVideoQuant");

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::Run() - SetParameter called for OMX_IndexParamVideoQuantization for Mpeg4/h263 on port %d", iOutputPortIndex));

                //OMX_VIDEO_PARAM_MOTIONVECTORTYPE Settings
                INIT_GETPARAMETER_STRUCT(OMX_VIDEO_PARAM_MOTIONVECTORTYPE, iMotionVector);
                iMotionVector.nPortIndex = iOutputPortIndex;

                Err = OMX_GetParameter(ipAppPriv->Handle, OMX_IndexParamVideoMotionVector, &iMotionVector);
                CHECK_ERROR(Err, "GetParameter_MPEG4ENC_IndexParamVideoMotionVector");
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::Run() - GetParameter called for OMX_IndexParamVideoMotionVector for Mpeg4/h263 on port %d", iOutputPortIndex));

                //Set the parameters now
                iMotionVector.nPortIndex = iOutputPortIndex;
                iMotionVector.sXSearchRange = iSearchRange;
                iMotionVector.sYSearchRange = iSearchRange;
                iMotionVector.bFourMV = iFourMV;

                Err = OMX_SetParameter(ipAppPriv->Handle, OMX_IndexParamVideoMotionVector, &iMotionVector);
                CHECK_ERROR(Err, "SetParameter_MPEG4ENC_IndexParamVideoMotionVector");

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::Run() - SetParameter called for OMX_IndexParamVideoMotionVector for Mpeg4/h263 on port %d", iOutputPortIndex));

                //OMX_VIDEO_PARAM_INTRAREFRESHTYPE Settings
                INIT_GETPARAMETER_STRUCT(OMX_VIDEO_PARAM_INTRAREFRESHTYPE, iRefreshParam);
                iRefreshParam.nPortIndex = iOutputPortIndex;

                Err = OMX_GetParameter(ipAppPriv->Handle, OMX_IndexParamVideoIntraRefresh, &iRefreshParam);
                CHECK_ERROR(Err, "GetParameter_MPEG4ENC_IndexParamVideoIntraRefresh");

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::Run() - GetParameter called for OMX_IndexParamVideoIntraRefresh for Mpeg4/h263 on port %d", iOutputPortIndex));
                //Set the parameters now
                iRefreshParam.nPortIndex = iOutputPortIndex;
                iRefreshParam.eRefreshMode = iRefreshMode;
                iRefreshParam.nCirMBs = iNumIntraMB;

                Err = OMX_SetParameter(ipAppPriv->Handle, OMX_IndexParamVideoIntraRefresh, &iRefreshParam);
                CHECK_ERROR(Err, "SetParameter_MPEG4ENC_IndexParamVideoIntraRefresh");
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::Run() - SetParameter called for OMX_IndexParamVideoIntraRefresh for Mpeg4/h263 on port %d", iOutputPortIndex));


                //OMX_VIDEO_PARAM_H263TYPE type in case of h263 and short header mode
                if ((1 == iCodecMode) || (1 == iShortHeaderFlag))
                {
                    INIT_GETPARAMETER_STRUCT(OMX_VIDEO_PARAM_H263TYPE, iH263Type);
                    iH263Type.nPortIndex = iOutputPortIndex;

                    Err = OMX_GetParameter(ipAppPriv->Handle, OMX_IndexParamVideoH263, &iH263Type);
                    CHECK_ERROR(Err, "GetParameter_MPEG4ENC_IndexParamVideoH263");

                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                    (0, "OmxEncTestEosMissing::Run() - GetParameter called for OMX_IndexParamVideoH263 for h263 on port %d", iOutputPortIndex));
                    //Set the parameters now
                    iH263Type.nPortIndex = iOutputPortIndex;
                    iH263Type.nGOBHeaderInterval = iGobHeaderInterval;

                    Err = OMX_SetParameter(ipAppPriv->Handle, OMX_IndexParamVideoH263, &iH263Type);
                    CHECK_ERROR(Err, "SetParameter_MPEG4ENC_IndexParamVideoH263");
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                    (0, "OmxEncTestEosMissing::Run() - SetParameter called for OMX_IndexParamVideoH263 for h263 on port %d", iOutputPortIndex));
                }

            }
            else if (0 == oscl_strcmp(iFormat, "H264"))
            {
                INIT_GETPARAMETER_STRUCT(OMX_VIDEO_PARAM_AVCTYPE, iAvcType);
                iAvcType.nPortIndex = iOutputPortIndex;

                Err = OMX_GetParameter(ipAppPriv->Handle, OMX_IndexParamVideoAvc, &iAvcType);
                CHECK_ERROR(Err, "GetParameter_AVCENC_IndexParamVideoAvc");

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::RunL() - GetParameter called for OMX_IndexParamVideoAvc for AVC on port %d", iOutputPortIndex));

                //Set the OMX_VIDEO_PARAM_AVCTYPE parameters
                iAvcType.nPortIndex = iOutputPortIndex;
                iAvcType.eProfile = iAvcProfile;
                iAvcType.eLevel = iAvcLevel;
                iAvcType.nPFrames = iNumPFrames;
                iAvcType.eLoopFilterMode = iLoopFilterType;
                iAvcType.bconstIpred = iIPredictionFlag;

                Err = OMX_SetParameter(ipAppPriv->Handle, OMX_IndexParamVideoAvc, &iAvcType);
                CHECK_ERROR(Err, "SetParameter_AVCENC_IndexParamVideoAvc");

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::RunL() - SetParameter called for OMX_IndexParamVideoAvc for AVC on port %d", iOutputPortIndex));

                //OMX_VIDEO_PARAM_BITRATETYPE Settings
                INIT_GETPARAMETER_STRUCT(OMX_VIDEO_PARAM_BITRATETYPE, iBitRateType);
                iBitRateType.nPortIndex = iOutputPortIndex;

                Err = OMX_GetParameter(ipAppPriv->Handle, OMX_IndexParamVideoBitrate, &iBitRateType);
                CHECK_ERROR(Err, "GetParameter_AVCENC_IndexParamVideoBitrate");

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::RunL() - GetParameter called for OMX_IndexParamVideoBitrate for AVC on port %d", iOutputPortIndex));

                //Set the parameters now
                iBitRateType.nPortIndex = iOutputPortIndex;
                iBitRateType.eControlRate = iRateControlType;
                iBitRateType.nTargetBitrate = iTgtBitRate;

                Err = OMX_SetParameter(ipAppPriv->Handle, OMX_IndexParamVideoBitrate, &iBitRateType);
                CHECK_ERROR(Err, "SetParameter_AVCENC_IndexParamVideoBitrate");

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::RunL() - SetParameter called for OMX_IndexParamVideoBitrate for AVC on port %d", iOutputPortIndex));


                //OMX_VIDEO_PARAM_QUANTIZATIONTYPE Settings
                INIT_GETPARAMETER_STRUCT(OMX_VIDEO_PARAM_QUANTIZATIONTYPE, iQuantParam);
                iQuantParam.nPortIndex = iOutputPortIndex;

                Err = OMX_GetParameter(ipAppPriv->Handle, OMX_IndexParamVideoQuantization, &iQuantParam);
                CHECK_ERROR(Err, "GetParameter_AVCENC_IndexParamVideoQuant");

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::RunL() - GetParameter called for OMX_IndexParamVideoQuantization for AVC on port %d", iOutputPortIndex));

                //Set the parameters now
                iQuantParam.nPortIndex = iOutputPortIndex;
                iQuantParam.nQpP = iPQuant;

                Err = OMX_SetParameter(ipAppPriv->Handle, OMX_IndexParamVideoQuantization, &iQuantParam);
                CHECK_ERROR(Err, "SetParameter_AVCENC_IndexParamVideoQuant");

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::RunL() - SetParameter called for OMX_IndexParamVideoQuantization for AVC on port %d", iOutputPortIndex));


                //OMX_VIDEO_PARAM_MOTIONVECTORTYPE Settings
                INIT_GETPARAMETER_STRUCT(OMX_VIDEO_PARAM_MOTIONVECTORTYPE, iMotionVector);
                iMotionVector.nPortIndex = iOutputPortIndex;

                Err = OMX_GetParameter(ipAppPriv->Handle, OMX_IndexParamVideoMotionVector, &iMotionVector);
                CHECK_ERROR(Err, "GetParameter_AVCENC_IndexParamVideoMotionVector");

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::RunL() - GetParameter called for OMX_IndexParamVideoMotionVector for AVC on port %d", iOutputPortIndex));

                //Set the parameters now
                iMotionVector.nPortIndex = iOutputPortIndex;
                iMotionVector.sXSearchRange = iSearchRange;
                iMotionVector.sYSearchRange = iSearchRange;
                iMotionVector.eAccuracy = iAccuracy;

                Err = OMX_SetParameter(ipAppPriv->Handle, OMX_IndexParamVideoMotionVector, &iMotionVector);
                CHECK_ERROR(Err, "SetParameter_AVCENC_IndexParamVideoMotionVector");

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::RunL() - SetParameter called for OMX_IndexParamVideoMotionVector for AVC on port %d", iOutputPortIndex));


                //OMX_VIDEO_PARAM_INTRAREFRESHTYPE Settings
                INIT_GETPARAMETER_STRUCT(OMX_VIDEO_PARAM_INTRAREFRESHTYPE, iRefreshParam);
                iRefreshParam.nPortIndex = iOutputPortIndex;

                Err = OMX_GetParameter(ipAppPriv->Handle, OMX_IndexParamVideoIntraRefresh, &iRefreshParam);
                CHECK_ERROR(Err, "GetParameter_AVCENC_IndexParamVideoIntraRefresh");

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::RunL() - GetParameter called for OMX_IndexParamVideoIntraRefresh for AVC on port %d", iOutputPortIndex));

                //Set the parameters now
                iRefreshParam.nPortIndex = iOutputPortIndex;
                iRefreshParam.eRefreshMode = iRefreshMode;
                iRefreshParam.nCirMBs = iNumIntraMB;

                Err = OMX_SetParameter(ipAppPriv->Handle, OMX_IndexParamVideoIntraRefresh, &iRefreshParam);
                CHECK_ERROR(Err, "SetParameter_AVCENC_IndexParamVideoIntraRefresh");

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::RunL() - SetParameter called for OMX_IndexParamVideoIntraRefresh for AVC on port %d", iOutputPortIndex));


            }
            else if (0 == oscl_strcmp(iFormat, "AAC"))
            {
                INIT_GETPARAMETER_STRUCT(OMX_AUDIO_PARAM_PCMMODETYPE, iPcmMode);
                iPcmMode.nPortIndex = iInputPortIndex;
                Err = OMX_GetParameter(ipAppPriv->Handle, OMX_IndexParamAudioPcm, &iPcmMode);
                CHECK_ERROR(Err, "GetParameter_ENC_IndexParamAudioPcm");

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxComponentEncTest::RunL() - GetParameter called for OMX_IndexParamAudioPcm for AAC on port %d", iInputPortIndex));


                //Set the input port parameters
                iPcmMode.nPortIndex = iInputPortIndex;

                iPcmMode.nChannels = iInputNumberOfChannels;
                iPcmMode.nBitPerSample = iInputBitsPerSample;
                iPcmMode.nSamplingRate = iInputSamplingRate;

                Err = OMX_SetParameter(ipAppPriv->Handle, OMX_IndexParamAudioPcm, &iPcmMode);
                CHECK_ERROR(Err, "SetParameter_ENC_IndexParamAudioPcm");

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxComponentEncTest::RunL() - SetParameter called for OMX_IndexParamAudioPcm for AAC on port %d", iInputPortIndex));



                /* Pass the output format type information to the component*/

                INIT_GETPARAMETER_STRUCT(OMX_AUDIO_PARAM_AACPROFILETYPE, iAacParam);
                iAacParam.nPortIndex = iOutputPortIndex;

                Err = OMX_GetParameter(ipAppPriv->Handle, OMX_IndexParamAudioAac, &iAacParam);
                CHECK_ERROR(Err, "GetParameter_ENC_IndexParamAudioAac");
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxComponentEncTest::RunL() - GetParameter called for IndexParamAudioAac for AAC on port %d", iOutputPortIndex));

                iAacParam.nPortIndex = iOutputPortIndex;

                iAacParam.nChannels = iOutputNumberOfChannels;
                iAacParam.nSampleRate = iInputSamplingRate;
                iAacParam.nBitRate = iTgtBitRate;
                iAacParam.nAudioBandWidth = iAacBandWidth;
                iAacParam.nAACtools = iAacTools;
                iAacParam.eAACProfile = iAacProfile;
                iAacParam.eAACStreamFormat = iAacStreamFormat;
                iAacParam.eChannelMode = iChannelMode;


                Err = OMX_SetParameter(ipAppPriv->Handle, OMX_IndexParamAudioAac, &iAacParam);
                CHECK_ERROR(Err, "SetParameter_ENC_IndexParamAudioAac");
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxComponentEncTest::RunL() - SetParameter called for OMX_IndexParamAudioAac for AAC on port %d", iOutputPortIndex));
            }

            if (StateError != iState)
            {
                iState = StateLoaded;
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "OmxEncTestEosMissing::Run() - StateUnLoaded OUT, moving to next state"));
            }

            RunIfNotReady();
        }
        break;

        case StateLoaded:
        {
            OMX_ERRORTYPE Err;
            OMX_S32 ii;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "OmxEncTestEosMissing::Run() - StateLoaded IN"));

            // allocate memory for ipInBuffer
            ipInBuffer = (OMX_BUFFERHEADERTYPE**) oscl_malloc(sizeof(OMX_BUFFERHEADERTYPE*) * iInBufferCount);
            CHECK_MEM(ipInBuffer, "InputBufferHeader");

            ipInputAvail = (OMX_BOOL*) oscl_malloc(sizeof(OMX_BOOL) * iInBufferCount);
            CHECK_MEM(ipInputAvail, "InputBufferFlag");

            /* Initialize all the buffers to NULL */
            for (ii = 0; ii < iInBufferCount; ii++)
            {
                ipInBuffer[ii] = NULL;
            }

            //allocate memory for output buffer
            ipOutBuffer = (OMX_BUFFERHEADERTYPE**) oscl_malloc(sizeof(OMX_BUFFERHEADERTYPE*) * iOutBufferCount);
            CHECK_MEM(ipOutBuffer, "OutputBuffer");

            ipOutReleased = (OMX_BOOL*) oscl_malloc(sizeof(OMX_BOOL) * iOutBufferCount);
            CHECK_MEM(ipOutReleased, "OutputBufferFlag");

            /* Initialize all the buffers to NULL */
            for (ii = 0; ii < iOutBufferCount; ii++)
            {
                ipOutBuffer[ii] = NULL;
            }

            Err = OMX_SendCommand(ipAppPriv->Handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
            CHECK_ERROR(Err, "SendCommand Loaded->Idle");

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                            (0, "OmxEncTestEosMissing::Run() - Sent State Transition Command from Loaded->Idle"));

            iPendingCommands = 1;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                            (0, "OmxEncTestEosMissing::Run() - Allocating %d input and %d output buffers", iInBufferCount, iOutBufferCount));

            //These calls are required because the control of in & out buffer should be with the testapp.
            for (ii = 0; ii < iInBufferCount; ii++)
            {

                Err = OMX_AllocateBuffer(ipAppPriv->Handle, &ipInBuffer[ii], iInputPortIndex, NULL, iInBufferSize);

                CHECK_ERROR(Err, "AllocateBuffer_Input");

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::Run() - Called AllocateBuffer for buffer index %d on port %d", ii, iInputPortIndex));

                ipInputAvail[ii] = OMX_TRUE;
                ipInBuffer[ii]->nInputPortIndex = iInputPortIndex;
            }

            if (StateError == iState)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "OmxEncTestEosMissing::Run() - AllocateBuffer Error, StateLoaded OUT"));
                RunIfNotReady();
                break;
            }


            for (ii = 0; ii < iOutBufferCount; ii++)
            {

                Err = OMX_AllocateBuffer(ipAppPriv->Handle, &ipOutBuffer[ii], iOutputPortIndex, NULL, iOutBufferSize);

                CHECK_ERROR(Err, "AllocateBuffer_Output");

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::Run() - Called AllocateBuffer for buffer index %d on port %d", ii, iOutputPortIndex));

                ipOutReleased[ii] = OMX_TRUE;

                ipOutBuffer[ii]->nOutputPortIndex = iOutputPortIndex;
                ipOutBuffer[ii]->nInputPortIndex = 0;
            }

            if (StateError == iState)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "OmxEncTestEosMissing::Run() - AllocateBuffer Error, StateLoaded OUT"));
                RunIfNotReady();
                break;
            }

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "OmxEncTestEosMissing::Run() - StateLoaded OUT, Moving to next state"));

        }
        break;

        case StateIdle:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "OmxEncTestEosMissing::Run() - StateIdle IN"));

            OMX_ERRORTYPE Err = OMX_ErrorNone;

            /*Send an output buffer before dynamic reconfig */
            Err = OMX_FillThisBuffer(ipAppPriv->Handle, ipOutBuffer[0]);
            CHECK_ERROR(Err, "FillThisBuffer");

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                            (0, "OmxComponentEncTest::Run() - FillThisBuffer command called for initiating dynamic port reconfiguration"));

            ipOutReleased[0] = OMX_FALSE;

            Err = OMX_SendCommand(ipAppPriv->Handle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
            CHECK_ERROR(Err, "SendCommand Idle->Executing");

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                            (0, "OmxEncTestEosMissing::Run() - Sent State Transition Command from Idle->Executing"));

            iPendingCommands = 1;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "OmxEncTestEosMissing::Run() - StateIdle OUT"));
        }
        break;

        case StateDisablePort:
        {
            OMX_BOOL Status = OMX_TRUE;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "OmxComponentEncTest::Run() - StateDisablePort IN"));

            if (!iDisableRun)
            {
                Status = HandlePortDisable();
                if (OMX_FALSE == Status)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "OmxComponentEncTest::Run() - Error occured in this state, StateDisablePort OUT"));
                    iState = StateError;
                    RunIfNotReady();
                    break;
                }

                RunIfNotReady();
            }

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "OmxComponentEncTest::Run() - StateDisablePort OUT"));
        }
        break;

        case StateDynamicReconfig:
        {
            OMX_BOOL Status = OMX_TRUE;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "OmxComponentEncTest::Run() - StateDynamicReconfig IN"));

            Status = HandlePortReEnable();
            if (OMX_FALSE == Status)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "OmxComponentEncTest::Run() - Error occured in this state, StateDynamicReconfig OUT"));
                iState = StateError;
                RunIfNotReady();
                break;
            }

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "OmxComponentEncTest::Run() - StateDynamicReconfig OUT"));
        }
        break;

        case StateExecuting:
        {
            static OMX_ERRORTYPE Status;
            OMX_S32 Index;
            OMX_BOOL MoreOutput;

            OMX_ERRORTYPE Err = OMX_ErrorNone;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "OmxEncTestEosMissing::Run() - StateExecuting IN"));


            MoreOutput = OMX_TRUE;
            while (MoreOutput)
            {
                Index = 0;
                while (OMX_FALSE == ipOutReleased[Index] && Index < iOutBufferCount)
                {
                    Index++;
                }

                if (Index != iOutBufferCount)
                {
                    //This call is being made only once per frame
                    ipOutBuffer[Index]->nOffset = 0;
                    Err = OMX_FillThisBuffer(ipAppPriv->Handle, ipOutBuffer[Index]);
                    CHECK_ERROR(Err, "FillThisBuffer");
                    //Make this flag OMX_TRUE till u receive the callback for output buffer free
                    ipOutReleased[Index] = OMX_FALSE;
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                    (0, "OmxEncTestEosMissing::Run() - FillThisBuffer command called for output buffer index %d", Index));
                }
                else
                {
                    MoreOutput = OMX_FALSE;
                }
            }


            if (!iStopProcessingInput || (OMX_ErrorInsufficientResources == Status))
            {
                // find available input buffer
                Index = 0;
                while (OMX_FALSE == ipInputAvail[Index] && Index < iInBufferCount)
                {
                    Index++;
                }

                if (Index != iInBufferCount)
                {
                    if ((0 == oscl_strcmp(iFormat, "AMRNB")) || (0 == oscl_strcmp(iFormat, "AAC")))
                    {
                        Status = GetInputAudioFrame();

                    }
                    else
                    {
                        Status = GetInputVideoFrame();
                    }
                }
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::Run() - Finished sending all the input buffers, Do not send the EOS buffer to the component"));

                iState = StateStopping;

            }

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "OmxEncTestEosMissing::Run() - StateExecuting OUT"));

            RunIfNotReady();
        }
        break;

        /********** STOP THE COMPONENT **********/
        case StateStopping:
        {
            static OMX_BOOL FlagTemp = OMX_FALSE;
            OMX_ERRORTYPE Err = OMX_ErrorNone;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "OmxEncTestEosMissing::Run() - StateStopping IN"));

            //stop execution by state transition to Idle state.
            if (!FlagTemp)
            {
                Err = OMX_SendCommand(ipAppPriv->Handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
                CHECK_ERROR(Err, "SendCommand Executing->Idle");

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::Run() - Sent State Transition Command from Executing->Idle"));

                iPendingCommands = 1;
                FlagTemp = OMX_TRUE;
            }

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "OmxEncTestEosMissing::Run() - StateStopping OUT"));
        }
        break;

        case StateCleanUp:
        {
            OMX_S32 ii;
            OMX_ERRORTYPE Err = OMX_ErrorNone;
            static OMX_BOOL FlagTemp = OMX_FALSE;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "OmxEncTestEosMissing::Run() - StateCleanUp IN"));

            if (!FlagTemp)
            {
                if (OMX_FALSE == VerifyAllBuffersReturned())
                {
                    // not all buffers have been returned yet, reschedule
                    RunIfNotReady();
                    break;
                }

                //Destroy the component by state transition to Loaded state
                Err = OMX_SendCommand(ipAppPriv->Handle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
                CHECK_ERROR(Err, "SendCommand Idle->Loaded");

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::Run() - Sent State Transition Command from Idle->Loaded"));

                iPendingCommands = 1;

                if (ipInBuffer)
                {
                    for (ii = 0; ii < iInBufferCount; ii++)
                    {
                        if (ipInBuffer[ii])
                        {
                            Err = OMX_FreeBuffer(ipAppPriv->Handle, iInputPortIndex, ipInBuffer[ii]);
                            CHECK_ERROR(Err, "FreeBuffer_Input");
                            ipInBuffer[ii] = NULL;

                            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                            (0, "OmxEncTestEosMissing::Run() - Called FreeBuffer for buffer index %d on port %d", ii, iInputPortIndex));
                        }
                    }

                    oscl_free(ipInBuffer);
                    ipInBuffer = NULL;
                }

                if (ipInputAvail)
                {
                    oscl_free(ipInputAvail);
                    ipInputAvail = NULL;
                }


                if (ipOutBuffer)
                {
                    for (ii = 0; ii < iOutBufferCount; ii++)
                    {
                        if (ipOutBuffer[ii])
                        {
                            Err = OMX_FreeBuffer(ipAppPriv->Handle, iOutputPortIndex, ipOutBuffer[ii]);
                            CHECK_ERROR(Err, "FreeBuffer_Output");
                            ipOutBuffer[ii] = NULL;

                            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                            (0, "OmxEncTestEosMissing::Run() - Called FreeBuffer for buffer index %d on port %d", ii, iOutputPortIndex));
                        }
                    }
                    oscl_free(ipOutBuffer);
                    ipOutBuffer = NULL;

                }

                if (ipOutReleased)
                {
                    oscl_free(ipOutReleased);
                    ipOutReleased = NULL;
                }

                FlagTemp = OMX_TRUE;
            }

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "OmxEncTestEosMissing::Run() - StateCleanUp OUT"));

        }
        break;


        /********* FREE THE HANDLE & CLOSE FILES FOR THE COMPONENT ********/
        case StateStop:
        {
            OMX_U8 TestName[] = "ENDOFSTREAM_MISSING_TEST";
            OMX_ERRORTYPE Err = OMX_ErrorNone;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "OmxEncTestEosMissing::Run() - StateStop IN"));

            if (ipAppPriv)
            {
                if (ipAppPriv->Handle)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                    (0, "OmxEncTestEosMissing::Run() - Free the Component Handle"));

                    Err = OMX_MasterFreeHandle(ipAppPriv->Handle);
                    if (OMX_ErrorNone != Err)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "OmxEncTestEosMissing::Run() - FreeHandle Error"));
                        iTestStatus = OMX_FALSE;
                    }
                }
            }

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                            (0, "OmxEncTestEosMissing::Run() - De-initialize the omx component"));

            Err = OMX_MasterDeinit();
            if (OMX_ErrorNone != Err)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "OmxEncTestEosMissing::Run() - OMX_MasterDeinit Error"));
                iTestStatus = OMX_FALSE;
            }

            if (ipAppPriv)
            {
                oscl_free(ipAppPriv);
                ipAppPriv = NULL;
            }

            if (ipInputFile)
            {
                fclose(ipInputFile);
                ipInputFile = NULL;
            }

            if (ipOutputFile)
            {
                fclose(ipOutputFile);
                ipOutputFile = NULL;
            }

#if PROXY_INTERFACE
            OSCL_DELETE(ipThreadSafeHandlerEventHandler);
            ipThreadSafeHandlerEventHandler = NULL;

            OSCL_DELETE(ipThreadSafeHandlerEmptyBufferDone);
            ipThreadSafeHandlerEmptyBufferDone = NULL;

            OSCL_DELETE(ipThreadSafeHandlerFillBufferDone);
            ipThreadSafeHandlerFillBufferDone = NULL;
#endif

            if (OMX_FALSE == iTestStatus)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::Run() - %s: Fail", TestName));
#ifdef PRINT_RESULT
                printf("%s: Fail \n", TestName);
                OMX_ENC_TEST(false);
                iTestCase->TestCompleted();
#endif

            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "OmxEncTestEosMissing::Run() - %s: Success", TestName));
#ifdef PRINT_RESULT
                printf("%s: Success \n", TestName);
                OMX_ENC_TEST(true);
                iTestCase->TestCompleted();
#endif
            }


            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "OmxEncTestEosMissing::Run() - StateStop OUT"));

            iState = StateUnLoaded;
            OsclExecScheduler* sched = OsclExecScheduler::Current();
            sched->StopScheduler();
        }
        break;

        case StateError:
        {
            //Do all the cleanup's and exit from here
            OMX_S32 ii;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "OmxEncTestEosMissing::Run() - StateError IN"));

            iTestStatus = OMX_FALSE;

            if (ipInBuffer)
            {
                for (ii = 0; ii < iInBufferCount; ii++)
                {
                    if (ipInBuffer[ii])
                    {
                        OMX_FreeBuffer(ipAppPriv->Handle, iInputPortIndex, ipInBuffer[ii]);
                        ipInBuffer[ii] = NULL;

                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                        (0, "OmxEncTestEosMissing::Run() - Called FreeBuffer for buffer index %d on port %d", ii, iInputPortIndex));
                    }
                }
                oscl_free(ipInBuffer);
                ipInBuffer = NULL;
            }

            if (ipInputAvail)
            {
                oscl_free(ipInputAvail);
                ipInputAvail = NULL;
            }

            if (ipOutBuffer)
            {
                for (ii = 0; ii < iOutBufferCount; ii++)
                {
                    if (ipOutBuffer[ii])
                    {
                        OMX_FreeBuffer(ipAppPriv->Handle, iOutputPortIndex, ipOutBuffer[ii]);
                        ipOutBuffer[ii] = NULL;

                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                        (0, "OmxEncTestEosMissing::Run() - Called FreeBuffer for buffer index %d on port %d", ii, iOutputPortIndex));
                    }
                }
                oscl_free(ipOutBuffer);
                ipOutBuffer = NULL;
            }

            if (ipOutReleased)
            {
                oscl_free(ipOutReleased);
                ipOutReleased = NULL;
            }

            iState = StateStop;
            RunIfNotReady();

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "OmxEncTestEosMissing::Run() - StateError OUT"));

        }
        break;


        default:
        {
            break;
        }
    }
    return;
}


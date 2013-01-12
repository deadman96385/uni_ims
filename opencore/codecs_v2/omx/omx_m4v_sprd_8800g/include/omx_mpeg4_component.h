/* ------------------------------------------------------------------
 * Copyright (C) 1998-2009 PacketVideo
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
/**
    @file omx_mpeg4_component.h
    OpenMax decoder_component component.

*/

#ifndef OMX_MPEG4_COMPONENT_H_INCLUDED
#define OMX_MPEG4_COMPONENT_H_INCLUDED

#ifndef PV_OMXCOMPONENT_H_INCLUDED
#include "pv_omxcomponent.h"
#endif

#ifndef MPEG4_DEC_H_INCLUDED
#include "mpeg4_dec.h"
#endif


#define INPUT_BUFFER_SIZE_MP4 16000
// qcif size - 176*144*3/2
#define OUTPUT_BUFFER_SIZE_MP4 38016

#define NUMBER_INPUT_BUFFER_MP4  8
#define NUMBER_OUTPUT_BUFFER_MP4  5

#define MINIMUM_H263_SHORT_HEADER_SIZE 12
/**
 * The structure for port Type.
 */
enum
{
    MODE_H263 = 0,
    MODE_MPEG4
};

// data structures for tunneling buffers
typedef struct PLATFORM_PRIVATE_PMEM_INFO
{
    /* pmem file descriptor */
    uint32 pmem_fd;
    uint32 offset;
} PLATFORM_PRIVATE_PMEM_INFO;

typedef struct PLATFORM_PRIVATE_ENTRY
{
    /* Entry type */
    uint32 type;

    /* Pointer to platform specific entry */
    OsclAny* entry;
} PLATFORM_PRIVATE_ENTRY;

typedef struct PLATFORM_PRIVATE_LIST
{
    /* Number of entries */
    uint32 nEntries;

    /* Pointer to array of platform specific entries *
     * Contiguous block of PLATFORM_PRIVATE_ENTRY elements */
    PLATFORM_PRIVATE_ENTRY* entryList;
} PLATFORM_PRIVATE_LIST;

#define PLATFORM_PRIVATE_PMEM 1

class OpenmaxMpeg4AO : public OmxComponentVideo
{
    public:
	//modified by jgdu
	void pop_output_buffer_from_queue(OMX_BUFFERHEADERTYPE *pTargetBuffer);
	static OSCL_EXPORT_REF OMX_ERRORTYPE OpenmaxMpeg4AOAllocateBuffer(
    	OMX_IN OMX_HANDLETYPE hComponent,
        OMX_INOUT OMX_BUFFERHEADERTYPE** pBuffer,
        OMX_IN OMX_U32 nPortIndex,
        OMX_IN OMX_PTR pAppPrivate,
        OMX_IN OMX_U32 nSizeBytes);	
	static OSCL_EXPORT_REF OMX_ERRORTYPE OpenmaxMpeg4AOFreeBuffer(
    	OMX_IN  OMX_HANDLETYPE hComponent,
    	OMX_IN  OMX_U32 nPortIndex,
    	OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

	OMX_ERRORTYPE AllocateBuffer(
    	OMX_IN OMX_HANDLETYPE hComponent,
    	OMX_INOUT OMX_BUFFERHEADERTYPE** pBuffer,
    	OMX_IN OMX_U32 nPortIndex,
    	OMX_IN OMX_PTR pAppPrivate,
    	OMX_IN OMX_U32 nSizeBytes);	
	OMX_ERRORTYPE FreeBuffer(
    	OMX_IN  OMX_HANDLETYPE hComponent,
    	OMX_IN  OMX_U32 nPortIndex,
    	OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

//for graphic buffer
static OSCL_EXPORT_REF OMX_ERRORTYPE OpenmaxMpeg4AOGetExtensionIndex(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_STRING cParameterName,
    OMX_OUT OMX_INDEXTYPE* pIndexType);
OSCL_IMPORT_REF OMX_ERRORTYPE GetParameter(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_INDEXTYPE nParamIndex,
            OMX_INOUT OMX_PTR ComponentParameterStructure);
OSCL_IMPORT_REF OMX_ERRORTYPE SetParameter(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_INDEXTYPE nParamIndex,
            OMX_IN  OMX_PTR ComponentParameterStructure);
static OSCL_EXPORT_REF OMX_ERRORTYPE OpenmaxMpeg4AOUseBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes,
    OMX_IN OMX_U8* pBuffer);
     OMX_ERRORTYPE UseBuffer(
     OMX_IN OMX_HANDLETYPE hComponent,
     OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
     OMX_IN OMX_U32 nPortIndex,
     OMX_IN OMX_PTR pAppPrivate,
     OMX_IN OMX_U32 nSizeBytes,
     OMX_IN OMX_U8* pBuffer);
     OMX_BOOL iUseAndroidNativeBuffer[2];	 
//	
	void ResetComponent();
        void ReleaseReferenceBuffers();

	OMX_BUFFERHEADERTYPE *ipOutputBufferForRendering;
	OMX_BOOL iFlushOutputStatus;
	
         uint32 FindPhyAddr(uint32 Vaddr); 
	 int  iNumberOfPmemBuffers;
	 MemoryHeapIon * iMemHeapBae;
 	 sp<MemoryHeapIon> iIonHeap;
	 uint32 iPmemBase;
	 uint32 iPmemBasePhy;
        //
	OpenmaxMpeg4AO();
        ~OpenmaxMpeg4AO();

        OMX_ERRORTYPE ConstructComponent(OMX_PTR pAppData, OMX_PTR pProxy);
        OMX_ERRORTYPE DestroyComponent();

        OMX_ERRORTYPE ComponentInit();
        OMX_ERRORTYPE ComponentDeInit();



        void SetDecoderMode(int);
        void ComponentBufferMgmtWithoutMarker();
        void ProcessData();
        void DecodeWithoutMarker();
        void DecodeWithMarker();

        OMX_ERRORTYPE GetConfig(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_INDEXTYPE nIndex,
            OMX_INOUT OMX_PTR pComponentConfigStructure);

        OMX_ERRORTYPE ReAllocatePartialAssemblyBuffers(OMX_BUFFERHEADERTYPE* aInputBufferHdr);

    private:

        OMX_BOOL DecodeH263Header(OMX_U8* aInputBuffer, OMX_U32* aBufferSize);

        void ReadBits(OMX_U8* aStream, uint8 aNumBits, uint32* aOutData);

        OMX_BOOL                iUseExtTimestamp;
        Mpeg4Decoder_OMX* ipMpegDecoderObject;
        OMX_S32 iDecMode;

        //Parameters required for H.263 source format parsing
        OMX_U32 iH263DataBitPos;
        OMX_U32 iH263BitPos;
        OMX_U32 iH263BitBuf;

 	 OMX_U32 iLogCount;

	 OMX_BOOL need_to_send_empty_done;  
};




#endif // OMX_MPEG4_COMPONENT_H_INCLUDED

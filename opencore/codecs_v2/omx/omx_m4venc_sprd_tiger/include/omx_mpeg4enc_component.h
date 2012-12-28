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
/**
    @file omx_mpeg4enc_component.h
    OpenMax encoder_component component.

*/

#ifndef OMX_MPEG4ENC_COMPONENT_H_INCLUDED
#define OMX_MPEG4ENC_COMPONENT_H_INCLUDED

#ifndef PV_OMXCOMPONENT_H_INCLUDED
#include "pv_omxcomponent.h"
#endif

#ifndef MPEG4_ENC_H_INCLUDED
#include "mpeg4_enc.h"
#endif

#if defined(CHIP_8800G)
#include "vsp_drv_sc8800g.h"
#elif defined(CHIP_8810)
#include "vsp_drv_sc8810.h"
#elif defined(CHIP_TIGER)
#include "vsp_drv_tiger.h"
#endif


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


#define INPUT_BUFFER_SIZE_MP4ENC 38016          //(176 * 144 * 1.5) for YUV 420 format.
#define OUTPUT_BUFFER_SIZE_MP4ENC (1500*1024)  //this size should be equal or larger than ONEFRAME_BITSTREAM_BFR_SIZE defined in  mp4enc_mode.h


#define NUMBER_INPUT_BUFFER_MP4ENC  5
#define NUMBER_OUTPUT_BUFFER_MP4ENC  2



class OmxComponentMpeg4EncAO : public OmxComponentVideo
{
    public:

        OmxComponentMpeg4EncAO();
        ~OmxComponentMpeg4EncAO();

	static OSCL_EXPORT_REF OMX_ERRORTYPE OpenmaxMpeg4EncAOAllocateBuffer(
    	OMX_IN OMX_HANDLETYPE hComponent,
        OMX_INOUT OMX_BUFFERHEADERTYPE** pBuffer,
        OMX_IN OMX_U32 nPortIndex,
        OMX_IN OMX_PTR pAppPrivate,
        OMX_IN OMX_U32 nSizeBytes);	
	static OSCL_EXPORT_REF OMX_ERRORTYPE OpenmaxMpeg4EncAOFreeBuffer(
    	OMX_IN  OMX_HANDLETYPE hComponent,
    	OMX_IN  OMX_U32 nPortIndex,
    	OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

//for meta buffer
static OSCL_EXPORT_REF OMX_ERRORTYPE OpenmaxMpeg4EncAOGetExtensionIndex(
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

	 OMX_BOOL          iStoreMetaDataBuffer;	
	 
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


         uint32 FindPhyAddr(uint32 Vaddr); 
	 OMX_COLOR_FORMATTYPE Get_eColorFormat_of_input();
	 int  iNumberOfPmemBuffers;
	 MemoryHeapIon * iMemHeapBae;
	 //sp<MemoryHeapBase> iMasterHeap; 
 	 sp<MemoryHeapIon> iPmemHeap;
	 uint32 iPmemBase;
	 uint32 iPmemBasePhy;
	 
        OMX_ERRORTYPE ConstructComponent(OMX_PTR pAppData, OMX_PTR pProxy);
        OMX_ERRORTYPE DestroyComponent();

        OMX_ERRORTYPE ComponentInit();
        OMX_ERRORTYPE ComponentDeInit();



        void SetEncoderMode(OMX_S32 aMode);
        void ProcessInBufferFlag();

        void ProcessData();

        OMX_ERRORTYPE SetConfig(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_INDEXTYPE nIndex,
            OMX_IN  OMX_PTR pComponentConfigStructure);

    private:

        OMX_BOOL CopyDataToOutputBuffer();

        Mpeg4Encoder_OMX* ipMpegEncoderObject;
        OMX_S32           iEncMode;

        OMX_BOOL          iBufferOverRun;
        OMX_U8*           ipInternalOutBuffer;
        OMX_U32           iInternalOutBufFilledLen;
        OMX_TICKS         iOutputTimeStamp;
        OMX_BOOL          iSyncFlag;
        OMX_U32           iOutputBufferSizeAfterInit;
        OMX_BOOL          iPortReconfigurationNeeded;
        OMX_BOOL          iSendVolHeaderFlag;
};

#endif // OMX_MPEG4ENC_COMPONENT_H_INCLUDED

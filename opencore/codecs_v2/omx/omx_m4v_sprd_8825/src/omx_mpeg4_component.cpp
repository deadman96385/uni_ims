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

#define LOG_NDEBUG 0
#define LOG_TAG "omx"
#include <utils/Log.h>

#include "oscl_base.h"
#include "pv_omxdefs.h"
#include "omx_mpeg4_component.h"

#if PROXY_INTERFACE
#include "omx_proxy_interface.h"
#endif

// Use default DLL entry point
#ifndef OSCL_DLL_H_INCLUDED
#include "oscl_dll.h"
#endif

#include <media/hardware/HardwareAPI.h>
//#include <ui/android_native_buffer.h>
#include "gralloc_priv.h"


OSCL_DLL_ENTRY_POINT_DEFAULT()

static const uint32 mask[33] =
{
    0x00000000, 0x00000001, 0x00000003, 0x00000007,
    0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
    0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
    0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
    0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
    0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
    0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
    0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
    0xffffffff
};

// This function is called by OMX_GetHandle and it creates an instance of the mpeg4 component AO
OSCL_EXPORT_REF OMX_ERRORTYPE Mpeg4OmxComponentFactory(OMX_OUT OMX_HANDLETYPE* pHandle, OMX_IN  OMX_PTR pAppData, OMX_IN OMX_PTR pProxy , OMX_STRING aOmxLibName, OMX_PTR &aOmxLib, OMX_PTR aOsclUuid, OMX_U32 &aRefCount)
{
    OSCL_UNUSED_ARG(aOmxLibName);
    OSCL_UNUSED_ARG(aOmxLib);
    OSCL_UNUSED_ARG(aOsclUuid);
    OSCL_UNUSED_ARG(aRefCount);


    OpenmaxMpeg4AO* pOpenmaxAOType;
    OMX_ERRORTYPE Status;

    // move InitMpeg4OmxComponentFields content to actual constructor

    pOpenmaxAOType = (OpenmaxMpeg4AO*) OSCL_NEW(OpenmaxMpeg4AO, ());

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorInsufficientResources;
    }

    // set decoding mode to H263
    pOpenmaxAOType->SetDecoderMode(MODE_MPEG4);

    //Call the construct component to initialize OMX types
    Status = pOpenmaxAOType->ConstructComponent(pAppData, pProxy);

    *pHandle = pOpenmaxAOType->GetOmxHandle();

    return Status;
    ///////////////////////////////////////////////////////////////////////////////////////
}

// This function is called by OMX_FreeHandle when component AO needs to be destroyed
OSCL_EXPORT_REF OMX_ERRORTYPE Mpeg4OmxComponentDestructor(OMX_IN OMX_HANDLETYPE pHandle, OMX_PTR &aOmxLib, OMX_PTR aOsclUuid, OMX_U32 &aRefCount)
{
    OSCL_UNUSED_ARG(aOmxLib);
    OSCL_UNUSED_ARG(aOsclUuid);
    OSCL_UNUSED_ARG(aRefCount);

    // get pointer to component AO
    OpenmaxMpeg4AO* pOpenmaxAOType = (OpenmaxMpeg4AO*)((OMX_COMPONENTTYPE*)pHandle)->pComponentPrivate;

    // clean up decoder, OMX component stuff
    pOpenmaxAOType->DestroyComponent();

    // destroy the AO class
    OSCL_DELETE(pOpenmaxAOType);

    return OMX_ErrorNone;
}

// This function is called by OMX_GetHandle and it creates an instance of the h263 component AO
OSCL_EXPORT_REF OMX_ERRORTYPE H263OmxComponentFactory(OMX_OUT OMX_HANDLETYPE* pHandle, OMX_IN  OMX_PTR pAppData, OMX_IN OMX_PTR pProxy, OMX_STRING aOmxLibName, OMX_PTR &aOmxLib, OMX_PTR aOsclUuid, OMX_U32 &aRefCount)
{
    OSCL_UNUSED_ARG(aOmxLibName);
    OSCL_UNUSED_ARG(aOmxLib);
    OSCL_UNUSED_ARG(aOsclUuid);
    OSCL_UNUSED_ARG(aRefCount);

    OpenmaxMpeg4AO* pOpenmaxAOType;
    OMX_ERRORTYPE Status;

    // move InitMpeg4OmxComponentFields content to actual constructor

    pOpenmaxAOType = (OpenmaxMpeg4AO*) OSCL_NEW(OpenmaxMpeg4AO, ());

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorInsufficientResources;
    }

    // set decoding mode to H263
    pOpenmaxAOType->SetDecoderMode(MODE_H263);

    //Call the construct component to initialize OMX types
    Status = pOpenmaxAOType->ConstructComponent(pAppData, pProxy);

    *pHandle = pOpenmaxAOType->GetOmxHandle();

    return Status;
    ///////////////////////////////////////////////////////////////////////////////////////
}

// This function is called by OMX_FreeHandle when component AO needs to be destroyed
OSCL_EXPORT_REF OMX_ERRORTYPE H263OmxComponentDestructor(OMX_IN OMX_HANDLETYPE pHandle, OMX_PTR &aOmxLib, OMX_PTR aOsclUuid, OMX_U32 &aRefCount)
{
    OSCL_UNUSED_ARG(aOmxLib);
    OSCL_UNUSED_ARG(aOsclUuid);
    OSCL_UNUSED_ARG(aRefCount);

    // get pointer to component AO
    OpenmaxMpeg4AO* pOpenmaxAOType = (OpenmaxMpeg4AO*)((OMX_COMPONENTTYPE*)pHandle)->pComponentPrivate;

    // clean up decoder, OMX component stuff
    pOpenmaxAOType->DestroyComponent();

    // destroy the AO class
    OSCL_DELETE(pOpenmaxAOType);

    return OMX_ErrorNone;
}

#if (DYNAMIC_LOAD_OMX_M4V_COMPONENT || DYNAMIC_LOAD_OMX_H263_COMPONENT)
class Mpeg4H263OmxSharedLibraryInterface: public OsclSharedLibraryInterface,
        public OmxSharedLibraryInterface

{
    public:
        OsclAny *QueryOmxComponentInterface(const OsclUuid& aOmxTypeId, const OsclUuid& aInterfaceId)
        {
            if (PV_OMX_M4VDEC_UUID == aOmxTypeId)
            {
                if (PV_OMX_CREATE_INTERFACE == aInterfaceId)
                {
                    return ((OsclAny*)(&Mpeg4OmxComponentFactory));
                }
                else if (PV_OMX_DESTROY_INTERFACE == aInterfaceId)
                {
                    return ((OsclAny*)(&Mpeg4OmxComponentDestructor));
                }
            } else if (PV_OMX_H263DEC_UUID == aOmxTypeId)
            {
                if (PV_OMX_CREATE_INTERFACE == aInterfaceId)
                {
                    return ((OsclAny*)(&H263OmxComponentFactory));
                }else if (PV_OMX_DESTROY_INTERFACE == aInterfaceId)
                {
                    return ((OsclAny*)(&H263OmxComponentDestructor));
                }
            }
            return NULL;
        };
        OsclAny *SharedLibraryLookup(const OsclUuid& aInterfaceId)
        {
            if (aInterfaceId == PV_OMX_SHARED_INTERFACE)
            {
                return OSCL_STATIC_CAST(OmxSharedLibraryInterface*, this);
            }
            return NULL;
        };

        Mpeg4H263OmxSharedLibraryInterface() {};
};

// function to obtain the interface object from the shared library
extern "C"
{
    OSCL_EXPORT_REF OsclAny* PVGetInterface()
    {
        return (OsclAny*) OSCL_NEW(Mpeg4H263OmxSharedLibraryInterface, ());
    }

    OSCL_EXPORT_REF void PVReleaseInterface(OsclSharedLibraryInterface* aInstance)
    {
        Mpeg4H263OmxSharedLibraryInterface* module = (Mpeg4H263OmxSharedLibraryInterface*)aInstance;
        OSCL_DELETE(module);
    }

}

#endif

void OpenmaxMpeg4AO::SetDecoderMode(int mode)
{
    iDecMode = mode;
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//modified by jgdu
void OpenmaxMpeg4AO::ResetComponent()
{
	if(ipMpegDecoderObject)
	{
		ipMpegDecoderObject->ResetDecoder();
	}
}
 
OSCL_EXPORT_REF OMX_ERRORTYPE OpenmaxMpeg4AO::OpenmaxMpeg4AOAllocateBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE** pBuffer,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes)
{
    OpenmaxMpeg4AO* pOpenmaxAOType = (OpenmaxMpeg4AO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    OMX_ERRORTYPE Status;

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorBadParameter;
    }

   Status = pOpenmaxAOType->AllocateBuffer(hComponent, pBuffer, nPortIndex, pAppPrivate, nSizeBytes);

    return Status;
}

OMX_ERRORTYPE OpenmaxMpeg4AO::AllocateBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE** pBuffer,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes)
{
    OSCL_UNUSED_ARG(hComponent);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentBase : AllocateBuffer IN"));

    ComponentPortType* pBaseComponentPort;
    OMX_U32 ii;

    if (nPortIndex >= iNumPorts)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentBase : AllocateBuffer error bad port index"));
        return OMX_ErrorBadPortIndex;
    }

    pBaseComponentPort = ipPorts[nPortIndex];

    if (pBaseComponentPort->TransientState != OMX_StateIdle)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentBase : AllocateBuffer error incorrect state"));
        return OMX_ErrorIncorrectStateTransition;
    }

    if (NULL == pBaseComponentPort->pBuffer)
    {
        pBaseComponentPort->pBuffer = (OMX_BUFFERHEADERTYPE**) oscl_calloc(pBaseComponentPort->PortParam.nBufferCountActual, sizeof(OMX_BUFFERHEADERTYPE*));
        if (NULL == pBaseComponentPort->pBuffer)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentBase : AllocateBuffer error insufficient resources"));
            return OMX_ErrorInsufficientResources;
        }

        pBaseComponentPort->BufferState = (OMX_U32*) oscl_calloc(pBaseComponentPort->PortParam.nBufferCountActual, sizeof(OMX_U32));
        if (NULL == pBaseComponentPort->BufferState)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentBase : AllocateBuffer error insufficient resources"));
            return OMX_ErrorInsufficientResources;
        }
    }

    //If the actual buffer size sent by the client is less than nBufferSize, return an error
    if (nSizeBytes < pBaseComponentPort->PortParam.nBufferSize)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentBase : AllocateBuffer error nSizeBytes is less than minimum buffer size"));
        return OMX_ErrorBadParameter;
    }

    for (ii = 0; ii < pBaseComponentPort->PortParam.nBufferCountActual; ii++)
    {
        if (!(pBaseComponentPort->BufferState[ii] & BUFFER_ALLOCATED) &&
                !(pBaseComponentPort->BufferState[ii] & BUFFER_ASSIGNED))
        {
            pBaseComponentPort->pBuffer[ii] = (OMX_BUFFERHEADERTYPE*) oscl_malloc(sizeof(OMX_BUFFERHEADERTYPE));
            if (NULL == pBaseComponentPort->pBuffer[ii])
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentBase : AllocateBuffer error insufficient resources"));
                return OMX_ErrorInsufficientResources;
            }
            SetHeader(pBaseComponentPort->pBuffer[ii], sizeof(OMX_BUFFERHEADERTYPE));
            /* allocate the buffer */
	   //modified by jgdu    
	   //pBaseComponentPort->pBuffer[ii]->pBuffer = (OMX_BYTE) oscl_malloc(nSizeBytes);
	    if (OMX_DirInput == pBaseComponentPort->PortParam.eDir)
	    {
	        pBaseComponentPort->pBuffer[ii]->pBuffer = (OMX_BYTE) oscl_malloc(nSizeBytes);
	    }else
	    {
               // pBaseComponentPort->pBuffer[ii]->pBuffer = (OMX_BYTE) oscl_malloc(nSizeBytes);	
                int  nSizeBytes_64word = (nSizeBytes+ 1024*4 - 1) & ~(1024*4 - 1);
	    	if(iNumberOfPmemBuffers==0)
	    	{
	    		if(nSizeBytes_64word<=0)
				return OMX_ErrorInsufficientResources;
			//	nSizeBytes_64word = 176*144*3/2;//in stagefight it may be 0
			OMX_MP4DEC_INFO ("%s: dec_sw %d", __FUNCTION__, ipMpegDecoderObject->iDecoder_sw_flag);

                    if(!ipMpegDecoderObject->iDecoder_sw_flag){
			   iMemHeapBae =  new MemoryHeapIon(SPRD_ION_DEV, nSizeBytes_64word*pBaseComponentPort->PortParam.nBufferCountActual,MemoryHeapBase::NO_CACHING, ION_HEAP_CARVEOUT_MASK);
			   iIonHeap = iMemHeapBae;
                    }else{
			   iMemHeapBae =  new MemoryHeapIon(SPRD_ION_DEV, nSizeBytes_64word*pBaseComponentPort->PortParam.nBufferCountActual,0, ION_HEAP_CARVEOUT_MASK);
			   iIonHeap = iMemHeapBae;
                    }
			int fd =iIonHeap->getHeapID();
			if(fd>=0){
				int ret,phy_addr, buffer_size;
				ret = iIonHeap->get_phy_addr_from_ion(&phy_addr, &buffer_size);
				if(ret) 
                                {
                                    OMX_MP4DEC_ERR ("get_phy_addr_from_ion fail %d",ret);
                                }
				iPmemBasePhy = phy_addr;
				iPmemBase  =(uint32) iIonHeap->getBase();
				OMX_MP4DEC_INFO ("OpenmaxMpeg4AO::AllocateBuffer pmemmpool num = %d,size = %d:%x,%x,%x,%x\n",pBaseComponentPort->PortParam.nBufferCountActual,nSizeBytes_64word,fd,iPmemBase,phy_addr,buffer_size);				

			}else
			{
				OMX_MP4DEC_ERR ("%s: pmempool error %d %d\n", __FUNCTION__, nSizeBytes_64word,pBaseComponentPort->PortParam.nBufferCountActual);
				return OMX_ErrorInsufficientResources;
			}
	    	}
			
	        if(iNumberOfPmemBuffers>=pBaseComponentPort->PortParam.nBufferCountActual)
	        {       
	        	OMX_MP4DEC_ERR ("%s: iNumberOfPmemHeaps>=nBufferCountActual %d %d\n", __FUNCTION__, iNumberOfPmemBuffers,pBaseComponentPort->PortParam.nBufferCountActual);
			return OMX_ErrorInsufficientResources;	
	        }
		pBaseComponentPort->pBuffer[ii]->pBuffer = (OMX_BYTE)(iPmemBase + iNumberOfPmemBuffers*nSizeBytes_64word);	
		iNumberOfPmemBuffers++;
            }
            
            if (NULL == pBaseComponentPort->pBuffer[ii]->pBuffer)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentBase : AllocateBuffer error insufficient resources"));
                return OMX_ErrorInsufficientResources;
            }
            pBaseComponentPort->pBuffer[ii]->nAllocLen = nSizeBytes;
            pBaseComponentPort->pBuffer[ii]->nFlags = 0;
	     if (OMX_DirInput == pBaseComponentPort->PortParam.eDir)
	    {
	    	pBaseComponentPort->pBuffer[ii]->pPlatformPrivate = pBaseComponentPort;
	     }else
	     {
                 //pBaseComponentPort->pBuffer[ii]->pPlatformPrivate = pBaseComponentPort;//modified by jgdu
                 pBaseComponentPort->pBuffer[ii]->pPlatformPrivate = NULL;
				 
		 PLATFORM_PRIVATE_PMEM_INFO *pInfo = new PLATFORM_PRIVATE_PMEM_INFO;
		 PLATFORM_PRIVATE_ENTRY *pEntry = new PLATFORM_PRIVATE_ENTRY;
                 PLATFORM_PRIVATE_LIST *pList =  new PLATFORM_PRIVATE_LIST;
		 if(!pInfo||!pEntry||!pList)
		 {
		 	if(pInfo)
				delete  pInfo;
			if(pEntry)
				delete pEntry;
			if(pList)
				delete pList;
		 	return OMX_ErrorInsufficientResources;
		 }
		 pInfo->pmem_fd = (uint32)(iMemHeapBae);
		 pInfo->offset =  (uint32)pBaseComponentPort->pBuffer[ii]->pBuffer - iPmemBase + iPmemBasePhy;
		 OMX_MP4DEC_INFO ("pInfo->offset %x",pInfo->offset);
		 pList->nEntries = 1;
		 pList->entryList = pEntry;
		 pList->entryList->type = PLATFORM_PRIVATE_PMEM;
		 pList->entryList->entry = pInfo;
		 
                 pBaseComponentPort->pBuffer[ii]->pPlatformPrivate = pList;		 		 
	     }
            pBaseComponentPort->pBuffer[ii]->pAppPrivate = pAppPrivate;
            *pBuffer = pBaseComponentPort->pBuffer[ii];
            pBaseComponentPort->BufferState[ii] |= BUFFER_ALLOCATED;
            pBaseComponentPort->BufferState[ii] |= HEADER_ALLOCATED;

            if (OMX_DirInput == pBaseComponentPort->PortParam.eDir)
            {
                pBaseComponentPort->pBuffer[ii]->nInputPortIndex = nPortIndex;
                // here is assigned a non-valid port index
                pBaseComponentPort->pBuffer[ii]->nOutputPortIndex = iNumPorts;


            }
            else
            {
                // here is assigned a non-valid port index
                pBaseComponentPort->pBuffer[ii]->nInputPortIndex = iNumPorts;
                pBaseComponentPort->pBuffer[ii]->nOutputPortIndex = nPortIndex;

                // create BufferCtrlStruct and assign pointer to it through OutputPortPrivateData
                pBaseComponentPort->pBuffer[ii]->pOutputPortPrivate = NULL;
                pBaseComponentPort->pBuffer[ii]->pOutputPortPrivate = oscl_malloc(sizeof(BufferCtrlStruct));

                if (NULL == pBaseComponentPort->pBuffer[ii]->pOutputPortPrivate)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentBase : UseBuffer error insufficient resources"));
                    return OMX_ErrorInsufficientResources;
                }
                BufferCtrlStruct *pBCTRL = (BufferCtrlStruct *) pBaseComponentPort->pBuffer[ii]->pOutputPortPrivate;
                pBCTRL->iRefCount = 1; // initialize ref counter to 1 (since buffers are initially with the IL client)
                pBCTRL->iIsBufferInComponentQueue = OMX_FALSE;

            }

            pBaseComponentPort->NumAssignedBuffers++;

            if (pBaseComponentPort->PortParam.nBufferCountActual == pBaseComponentPort->NumAssignedBuffers)
            {
                pBaseComponentPort->PortParam.bPopulated = OMX_TRUE;

                if (OMX_TRUE == iStateTransitionFlag)
                {
                    //Reschedule the AO for a state change (Loaded->Idle) if its pending on buffer allocation
                    RunIfNotReady();
                    iStateTransitionFlag = OMX_FALSE;
                }
            }

            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentBase : AllocateBuffer OUT"));
            return OMX_ErrorNone;
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentBase : AllocateBuffer OUT"));
    return OMX_ErrorInsufficientResources;
}
OSCL_EXPORT_REF OMX_ERRORTYPE OpenmaxMpeg4AO::OpenmaxMpeg4AOFreeBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_U32 nPortIndex,
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    OpenmaxMpeg4AO* pOpenmaxAOType = (OpenmaxMpeg4AO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    OMX_ERRORTYPE Status;

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorBadParameter;
    }

    Status = pOpenmaxAOType->FreeBuffer(hComponent, nPortIndex, pBuffer);

    return Status;
}
OMX_ERRORTYPE OpenmaxMpeg4AO::FreeBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_U32 nPortIndex,
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentBase : FreeBuffer IN"));

    ComponentPortType* pBaseComponentPort;

    OMX_U32 ii;
    OMX_BOOL FoundBuffer;

    if (nPortIndex >= iNumPorts)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentBase : FreeBuffer error bad port index"));
        return OMX_ErrorBadPortIndex;
    }

    pBaseComponentPort = ipPorts[nPortIndex];

    if (pBaseComponentPort->TransientState != OMX_StateLoaded
            && pBaseComponentPort->TransientState != OMX_StateInvalid)
    {

        (*(ipCallbacks->EventHandler))
        (hComponent,
         iCallbackData,
         OMX_EventError, /* The command was completed */
         OMX_ErrorPortUnpopulated, /* The commands was a OMX_CommandStateSet */
         nPortIndex, /* The State has been changed in message->MessageParam2 */
         NULL);
    }

    for (ii = 0; ii < pBaseComponentPort->PortParam.nBufferCountActual; ii++)
    {
        if ((pBaseComponentPort->BufferState[ii] & BUFFER_ALLOCATED) &&
                (pBaseComponentPort->pBuffer[ii]->pBuffer == pBuffer->pBuffer))
        {

            pBaseComponentPort->NumAssignedBuffers--;
	    //modified by jgdu		
            //oscl_free(pBuffer->pBuffer);
             if (OMX_DirInput == pBaseComponentPort->PortParam.eDir)
	     {
	    	 oscl_free(pBuffer->pBuffer);
             }else
             {
                if( iNumberOfPmemBuffers)
                {
               	    iNumberOfPmemBuffers = 0;
                    if(iIonHeap!=NULL)
                    {
			iIonHeap.clear();				
		        OMX_MP4DEC_INFO ("%s: pmempool clear", __FUNCTION__);
                    }
                }
		PLATFORM_PRIVATE_LIST *pList = (PLATFORM_PRIVATE_LIST *)pBaseComponentPort->pBuffer[ii]->pPlatformPrivate;
		if(pList)
		{
		 	PLATFORM_PRIVATE_ENTRY *pEntry = pList->entryList ;
			PLATFORM_PRIVATE_PMEM_INFO *pInfo =(PLATFORM_PRIVATE_PMEM_INFO *) pEntry->entry;
			delete pList;
			delete pEntry;
			delete pInfo;		
		}
             }
            pBuffer->pBuffer = NULL;

            if (pBaseComponentPort->BufferState[ii] & HEADER_ALLOCATED)
            {
                // deallocate the BufferCtrlStruct from output port
                if (OMX_DirOutput == pBaseComponentPort->PortParam.eDir)
                {

                    if (pBuffer->pOutputPortPrivate)
                    {
                        oscl_free(pBuffer->pOutputPortPrivate);
                        pBuffer->pOutputPortPrivate = NULL;
                    }
                }

                oscl_free(pBuffer);
                pBuffer = NULL;
            }
            pBaseComponentPort->BufferState[ii] = BUFFER_FREE;
            break;
        }
        else if ((pBaseComponentPort->BufferState[ii] & BUFFER_ASSIGNED) &&
                 (pBaseComponentPort->pBuffer[ii] == pBuffer))
        {
            pBaseComponentPort->NumAssignedBuffers--;

            if (pBaseComponentPort->BufferState[ii] & HEADER_ALLOCATED)
            {
                // deallocate the BufferCtrlStruct
                if (OMX_DirOutput == pBaseComponentPort->PortParam.eDir)
                {
                    PLATFORM_PRIVATE_LIST *pList = (PLATFORM_PRIVATE_LIST *)pBaseComponentPort->pBuffer[ii]->pPlatformPrivate;
                    if(pList)
                    {
                        PLATFORM_PRIVATE_ENTRY *pEntry = pList->entryList ;
                        PLATFORM_PRIVATE_PMEM_INFO *pInfo =(PLATFORM_PRIVATE_PMEM_INFO *) pEntry->entry;
                        delete pList;
                        delete pEntry;
                        delete pInfo;
                    }
                    
                    if (pBuffer->pOutputPortPrivate)
                    {
                        oscl_free(pBuffer->pOutputPortPrivate);
                        pBuffer->pOutputPortPrivate = NULL;
                    }
                }

                oscl_free(pBuffer);
                pBuffer = NULL;
            }

            pBaseComponentPort->BufferState[ii] = BUFFER_FREE;
            break;
        }
    }

    FoundBuffer = OMX_FALSE;

    for (ii = 0; ii < pBaseComponentPort->PortParam.nBufferCountActual; ii++)
    {
        if (pBaseComponentPort->BufferState[ii] != BUFFER_FREE)
        {
            FoundBuffer = OMX_TRUE;
            break;
        }
    }
    if (!FoundBuffer)
    {
        pBaseComponentPort->PortParam.bPopulated = OMX_FALSE;

        if (OMX_TRUE == iStateTransitionFlag)
        {
            //Reschedule the AO for a state change (Idle->Loaded) if its pending on buffer de-allocation
            RunIfNotReady();
            iStateTransitionFlag = OMX_FALSE;

            //Reset the decoding flags while freeing buffers
            if (OMX_PORT_INPUTPORT_INDEX == nPortIndex)
            {
                iIsInputBufferEnded = OMX_TRUE;
                iTempInputBufferLength = 0;
                iTempConsumedLength = 0;
                iNewInBufferRequired = OMX_TRUE;
            }else if (OMX_PORT_OUTPUTPORT_INDEX == nPortIndex)
            {
                iNewOutBufRequired = OMX_TRUE;
            }
        }

        if (NULL != pBaseComponentPort->pBuffer)
        {
            oscl_free(pBaseComponentPort->pBuffer);
            pBaseComponentPort->pBuffer = NULL;
            oscl_free(pBaseComponentPort->BufferState);
            pBaseComponentPort->BufferState = NULL;
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentBase : FreeBuffer OUT"));
    return OMX_ErrorNone;
}

OSCL_EXPORT_REF OMX_ERRORTYPE OpenmaxMpeg4AO::OpenmaxMpeg4AOUseBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes,
    OMX_IN OMX_U8* pBuffer)
{
    OpenmaxMpeg4AO* pOpenmaxAOType = (OpenmaxMpeg4AO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    OMX_ERRORTYPE Status;

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorBadParameter;
    }

    Status = pOpenmaxAOType->UseBuffer(hComponent, ppBufferHdr, nPortIndex, pAppPrivate, nSizeBytes, pBuffer);

    return Status;
}

OMX_ERRORTYPE OpenmaxMpeg4AO::UseBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes,
    OMX_IN OMX_U8* pBuffer)
{
    OSCL_UNUSED_ARG(hComponent);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentBase : UseBuffer IN"));
    ComponentPortType* pBaseComponentPort;
    OMX_U32 ii;

    if (nPortIndex >= iNumPorts)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentBase : UseBuffer error bad port index"));
        return OMX_ErrorBadPortIndex;
    }

    pBaseComponentPort = ipPorts[nPortIndex];

    if (pBaseComponentPort->TransientState != OMX_StateIdle)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentBase : UseBuffer error incorrect state"));
        return OMX_ErrorIncorrectStateTransition;
    }

    if (NULL == pBaseComponentPort->pBuffer)
    {
        pBaseComponentPort->pBuffer = (OMX_BUFFERHEADERTYPE**) oscl_calloc(pBaseComponentPort->PortParam.nBufferCountActual, sizeof(OMX_BUFFERHEADERTYPE*));
        if (NULL == pBaseComponentPort->pBuffer)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentBase : UseBuffer error insufficient resources"));
            return OMX_ErrorInsufficientResources;
        }

        pBaseComponentPort->BufferState = (OMX_U32*) oscl_calloc(pBaseComponentPort->PortParam.nBufferCountActual, sizeof(OMX_U32));
        if (NULL == pBaseComponentPort->BufferState)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentBase : UseBuffer error insufficient resources"));
            return OMX_ErrorInsufficientResources;
        }
    }

    //If the actual buffer size sent by the client is less than nBufferSize, return an error
    if (nSizeBytes < pBaseComponentPort->PortParam.nBufferSize)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentBase : UseBuffer error nSizeBytes is less than minimum buffer size"));
        return OMX_ErrorBadParameter;
    }
	
    for (ii = 0; ii < pBaseComponentPort->PortParam.nBufferCountActual; ii++)
    {
        if (!(pBaseComponentPort->BufferState[ii] & BUFFER_ALLOCATED) &&
                !(pBaseComponentPort->BufferState[ii] & BUFFER_ASSIGNED))
        {
            pBaseComponentPort->pBuffer[ii] = (OMX_BUFFERHEADERTYPE*) oscl_malloc(sizeof(OMX_BUFFERHEADERTYPE));
            if (NULL == pBaseComponentPort->pBuffer[ii])
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentBase : UseBuffer error insufficient resources"));
                return OMX_ErrorInsufficientResources;
            }
            SetHeader(pBaseComponentPort->pBuffer[ii], sizeof(OMX_BUFFERHEADERTYPE));
            pBaseComponentPort->pBuffer[ii]->pBuffer = pBuffer;
            pBaseComponentPort->pBuffer[ii]->nAllocLen = nSizeBytes;
            pBaseComponentPort->pBuffer[ii]->nFilledLen = 0;
            pBaseComponentPort->pBuffer[ii]->nOffset = 0;
            pBaseComponentPort->pBuffer[ii]->nFlags = 0;
            pBaseComponentPort->pBuffer[ii]->pPlatformPrivate = pBaseComponentPort;
            pBaseComponentPort->pBuffer[ii]->pAppPrivate = pAppPrivate;
            pBaseComponentPort->pBuffer[ii]->nTickCount = 0;
            pBaseComponentPort->pBuffer[ii]->nTimeStamp = 0;
            pBaseComponentPort->pBuffer[ii]->hMarkTargetComponent = NULL;
            *ppBufferHdr = pBaseComponentPort->pBuffer[ii];
            if (OMX_DirInput == pBaseComponentPort->PortParam.eDir)
            {
                pBaseComponentPort->pBuffer[ii]->nInputPortIndex = nPortIndex;
                pBaseComponentPort->pBuffer[ii]->nOutputPortIndex = iNumPorts; // here is assigned a non-valid port index
            }
            else
            {
                if(!iUseAndroidNativeBuffer[OMX_PORT_OUTPUTPORT_INDEX])
                {
            	    OMX_MP4DEC_INFO ("OpenmaxMpeg4AO call use buffer on output port when iUseAndroidNativeBuffer disenabled");
                    return OMX_ErrorInsufficientResources;            	  	
                }else
                {
                    pBaseComponentPort->pBuffer[ii]->pPlatformPrivate = NULL;
				 
                    PLATFORM_PRIVATE_PMEM_INFO *pInfo = new PLATFORM_PRIVATE_PMEM_INFO;
                    PLATFORM_PRIVATE_ENTRY *pEntry = new PLATFORM_PRIVATE_ENTRY;
                    PLATFORM_PRIVATE_LIST *pList =  new PLATFORM_PRIVATE_LIST;
                    if(!pInfo||!pEntry||!pList)
                    {
                        if(pInfo)
                        {
			    delete  pInfo;
                        }
                        if(pEntry)
                        {
			    delete pEntry;
                        }
                        if(pList)
                        {
			    delete pList;
                        }
                        return OMX_ErrorInsufficientResources;
                    }

                    native_handle_t *pNativeHandle = (native_handle_t *)pBaseComponentPort->pBuffer[ii]->pBuffer;
		    struct private_handle_t *private_h = (struct private_handle_t *)pNativeHandle;
                    pInfo->offset =  (uint32)(private_h->phyaddr);
                    OMX_MP4DEC_INFO("pInfo->offset %x",pInfo->offset);
                    pList->nEntries = 1;
                    pList->entryList = pEntry;
                    pList->entryList->type = PLATFORM_PRIVATE_PMEM;
                    pList->entryList->entry = pInfo;
		 
                    pBaseComponentPort->pBuffer[ii]->pPlatformPrivate = pList;		 		 
                }
		  		
                pBaseComponentPort->pBuffer[ii]->nOutputPortIndex = nPortIndex;
                pBaseComponentPort->pBuffer[ii]->nInputPortIndex = iNumPorts; // here is assigned a non-valid port index

                // create BufferCtrlStruct and assign pointer to it through OutputPortPrivateData
                pBaseComponentPort->pBuffer[ii]->pOutputPortPrivate = NULL;
                pBaseComponentPort->pBuffer[ii]->pOutputPortPrivate = oscl_malloc(sizeof(BufferCtrlStruct));

                if (NULL == pBaseComponentPort->pBuffer[ii]->pOutputPortPrivate)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentBase : UseBuffer error insufficient resources"));
                    return OMX_ErrorInsufficientResources;
                }
                BufferCtrlStruct *pBCTRL = (BufferCtrlStruct *) pBaseComponentPort->pBuffer[ii]->pOutputPortPrivate;
                pBCTRL->iRefCount = 1; // initialize ref counter to 1 (since buffers are initially with the IL client)
                pBCTRL->iIsBufferInComponentQueue = OMX_FALSE;
            }
            
            pBaseComponentPort->BufferState[ii] |= BUFFER_ASSIGNED;
            pBaseComponentPort->BufferState[ii] |= HEADER_ALLOCATED;
            pBaseComponentPort->NumAssignedBuffers++;
            if (pBaseComponentPort->PortParam.nBufferCountActual == pBaseComponentPort->NumAssignedBuffers)
            {
                pBaseComponentPort->PortParam.bPopulated = OMX_TRUE;

                if (OMX_TRUE == iStateTransitionFlag)
                {
                    //Reschedule the AO for a state change (Loaded->Idle) if its pending on buffer allocation
                    RunIfNotReady();
                    iStateTransitionFlag = OMX_FALSE;
                }
            }
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentBase : UseBuffer OUT"));
            return OMX_ErrorNone;
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentBase : UseBuffer OUT"));
    return OMX_ErrorNone;
}

OSCL_EXPORT_REF OMX_ERRORTYPE OpenmaxMpeg4AO::OpenmaxMpeg4AOGetExtensionIndex(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_STRING cParameterName,
    OMX_OUT OMX_INDEXTYPE* pIndexType)
{
    OpenmaxMpeg4AO* pOpenmaxAOType = (OpenmaxMpeg4AO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorBadParameter;
    }
	
    if(strcmp(cParameterName, SPRD_INDEX_PARAM_ENABLE_ANB) == 0)
    {
        OMX_MP4DEC_INFO("OpenmaxMpeg4AO::%s",SPRD_INDEX_PARAM_ENABLE_ANB);
	*pIndexType = (OMX_INDEXTYPE) OMX_IndexParamEnableAndroidBuffers;
	return OMX_ErrorNone;
    }else if (strcmp(cParameterName, SPRD_INDEX_PARAM_GET_ANB) == 0)
    {
     	OMX_MP4DEC_INFO("OpenmaxMpeg4AO::%s",SPRD_INDEX_PARAM_GET_ANB);   
	*pIndexType = (OMX_INDEXTYPE) OMX_IndexParamGetAndroidNativeBuffer;
	return OMX_ErrorNone;
    }	else if (strcmp(cParameterName, SPRD_INDEX_PARAM_USE_ANB) == 0)
    {
     	OMX_MP4DEC_INFO("OpenmaxMpeg4AO::%s",SPRD_INDEX_PARAM_USE_ANB);     
	*pIndexType = OMX_IndexParamUseAndroidNativeBuffer2;
	return OMX_ErrorNone;
    }
	
    return OMX_ErrorNotImplemented;
}

OSCL_EXPORT_REF OMX_ERRORTYPE OpenmaxMpeg4AO::GetParameter(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nParamIndex,
    OMX_INOUT OMX_PTR ComponentParameterStructure)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
       OpenmaxMpeg4AO* pOpenmaxAOType = (OpenmaxMpeg4AO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
	OMX_MP4DEC_DEBUG ("OpenmaxMpeg4AO::GetParameter");	   
    	if (NULL == pOpenmaxAOType)
    	{
            return OMX_ErrorBadParameter;
    	}
       switch (nParamIndex) {
	    case OMX_IndexParamEnableAndroidBuffers:
	    {
                EnableAndroidNativeBuffersParams *peanbp = (EnableAndroidNativeBuffersParams *)ComponentParameterStructure;			
		peanbp->enable = iUseAndroidNativeBuffer[OMX_PORT_OUTPUTPORT_INDEX];
		OMX_MP4DEC_INFO("OpenmaxMpeg4AO::GetParameter OMX_IndexParamEnableAndroidBuffers %d",peanbp->enable);			
	    }
		break;
	    case OMX_IndexParamGetAndroidNativeBuffer:
	    {
                GetAndroidNativeBufferUsageParams *pganbp;

    		pganbp = (GetAndroidNativeBufferUsageParams *)ComponentParameterStructure;
		if(ipMpegDecoderObject->iDecoder_sw_flag)
    		    pganbp->nUsage = GRALLOC_USAGE_PRIVATE_0|GRALLOC_USAGE_SW_READ_OFTEN|GRALLOC_USAGE_SW_WRITE_OFTEN;
                else
		    pganbp->nUsage = GRALLOC_USAGE_PRIVATE_0;
                OMX_MP4DEC_INFO("OpenmaxMpeg4AO::GetParameter OMX_IndexParamGetAndroidNativeBuffer %x",pganbp->nUsage);
	    }		
		break;
	    default:
		ret = OmxComponentVideo::GetParameter(hComponent,nParamIndex,ComponentParameterStructure);
		break;
       }
	
	return ret;
}

OSCL_EXPORT_REF OMX_ERRORTYPE OpenmaxMpeg4AO::SetParameter(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nParamIndex,
    OMX_IN  OMX_PTR ComponentParameterStructure)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
       OpenmaxMpeg4AO* pOpenmaxAOType = (OpenmaxMpeg4AO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
	OMX_MP4DEC_DEBUG("OpenmaxMpeg4AO::SetParameter");	
    	if (NULL == pOpenmaxAOType)
    	{
            return OMX_ErrorBadParameter;
    	}	
	switch (nParamIndex) 
        {
	    case OMX_IndexParamEnableAndroidBuffers:
	    {
                EnableAndroidNativeBuffersParams *peanbp = (EnableAndroidNativeBuffersParams *)ComponentParameterStructure;
		ComponentPortType *pOutPort = (ComponentPortType*) ipPorts[OMX_PORT_OUTPUTPORT_INDEX];
		if (peanbp->enable == OMX_FALSE) 
                {
        	    OMX_MP4DEC_INFO("OpenmaxMpeg4AO::disable AndroidNativeBuffer");
        	    iUseAndroidNativeBuffer[OMX_PORT_OUTPUTPORT_INDEX] = OMX_FALSE;
		    if(ipMpegDecoderObject->iDecoder_sw_flag)
                    {
#ifdef YUV_THREE_PLANE
		        pOutPort->VideoParam[0].eColorFormat = OMX_COLOR_FormatYUV420Planar;
    		        pOutPort->PortParam.format.video.eColorFormat = OMX_COLOR_FormatYUV420Planar;
#else
		        pOutPort->VideoParam[0].eColorFormat = (OMX_COLOR_FORMATTYPE)0x7FA30C00; //modified by jgdu	
    		        pOutPort->PortParam.format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)0x7FA30C00;	
#endif
		    }else
		    {
		        pOutPort->VideoParam[0].eColorFormat = (OMX_COLOR_FORMATTYPE)0x7FA30C00; //modified by jgdu	
    		        pOutPort->PortParam.format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)0x7FA30C00;
		    }
    	        } else
    		{
        	    OMX_MP4DEC_INFO ("OpenmaxMpeg4AO::enable AndroidNativeBuffer");
                    iUseAndroidNativeBuffer[OMX_PORT_OUTPUTPORT_INDEX] = OMX_TRUE;
		    if(ipMpegDecoderObject->iDecoder_sw_flag)
                    {	
#ifdef YUV_THREE_PLANE
                        pOutPort->VideoParam[0].eColorFormat = (OMX_COLOR_FORMATTYPE)HAL_PIXEL_FORMAT_YV12;
    		        pOutPort->PortParam.format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)HAL_PIXEL_FORMAT_YV12;
#else
                        pOutPort->VideoParam[0].eColorFormat = (OMX_COLOR_FORMATTYPE)HAL_PIXEL_FORMAT_YCbCr_420_SP;
    		        pOutPort->PortParam.format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)HAL_PIXEL_FORMAT_YCbCr_420_SP;
#endif
                    }else
                    {
		        pOutPort->VideoParam[0].eColorFormat = (OMX_COLOR_FORMATTYPE)HAL_PIXEL_FORMAT_YCbCr_420_SP;
    		        pOutPort->PortParam.format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)HAL_PIXEL_FORMAT_YCbCr_420_SP;
		    }
     	        }
	    }	
		break;
	    default:
                ret = OmxComponentVideo::SetParameter(hComponent,nParamIndex,ComponentParameterStructure);
		break;
	}
	return ret;	
}

static bool getPlatformPhyAddr(void *platformPrivate, int *phy) {

    OMX_BUFFERHEADERTYPE *header = (OMX_BUFFERHEADERTYPE *)platformPrivate;

    *phy = 0;

    PLATFORM_PRIVATE_LIST *list = (PLATFORM_PRIVATE_LIST *)header->pPlatformPrivate;
    for (uint32_t i = 0; i < list->nEntries; ++i) {
        if (list->entryList[i].type != PLATFORM_PRIVATE_PMEM) {
            continue;
        }

        PLATFORM_PRIVATE_PMEM_INFO *info =
            (PLATFORM_PRIVATE_PMEM_INFO *)list->entryList[i].entry;

        if (info != NULL) {
            *phy = info->offset;
            return true;
        }
    }
    return false;
}

uint32 OpenmaxMpeg4AO:: FindPhyAddr(uint32 Vaddr)
{
    ComponentPortType* pBaseComponentPort;
    OMX_U32 ii;
    int phy = 0;
    pBaseComponentPort = ipPorts[OMX_PORT_OUTPUTPORT_INDEX];
    for (ii = 0; ii < pBaseComponentPort->PortParam.nBufferCountActual; ii++)
    {
        if(pBaseComponentPort->pBuffer[ii]->pBuffer == (OMX_U8*)Vaddr)
        {
            getPlatformPhyAddr(pBaseComponentPort->pBuffer[ii],&phy);
            break;
        }
    }

    OMX_MP4DEC_DEBUG ("find phy addr 0x%x",phy);
    if((0 == phy)&&ipMpegDecoderObject->iDecoder_sw_flag)
    {
        return Vaddr;
    }

    return phy;	
 }

 void OpenmaxMpeg4AO::ReleaseReferenceBuffers()
{
    if (ipMpegDecoderObject)
    {
        ipMpegDecoderObject->ReleaseReferenceBuffers();
    }
}
 
OMX_ERRORTYPE OpenmaxMpeg4AO::ConstructComponent(OMX_PTR pAppData, OMX_PTR pProxy)
{
    ComponentPortType *pInPort, *pOutPort;
    OMX_ERRORTYPE Status;

    iNumPorts = 2;
    iOmxComponent.nSize = sizeof(OMX_COMPONENTTYPE);
    iOmxComponent.pComponentPrivate = (OMX_PTR) this;  // pComponentPrivate points to THIS component AO class
    ipComponentProxy = pProxy;
    iOmxComponent.pApplicationPrivate = pAppData; // init the App data

    //modified by jgdu
    ipOutputBufferForRendering = NULL;
    iNumberOfPmemBuffers = 0;
    iFlushOutputStatus = OMX_TRUE;
    iLogCount = 0;	
    iUseAndroidNativeBuffer[OMX_PORT_INPUTPORT_INDEX] = OMX_FALSE;
    iUseAndroidNativeBuffer[OMX_PORT_OUTPUTPORT_INDEX] = OMX_FALSE;
    need_to_send_empty_done = OMX_FALSE;
	
#if PROXY_INTERFACE
    iPVCapabilityFlags.iIsOMXComponentMultiThreaded = OMX_TRUE;

    iOmxComponent.SendCommand = OpenmaxMpeg4AO::BaseComponentProxySendCommand;
    iOmxComponent.GetParameter = OpenmaxMpeg4AO::BaseComponentProxyGetParameter;
    iOmxComponent.SetParameter = OpenmaxMpeg4AO::BaseComponentProxySetParameter;
    iOmxComponent.GetConfig = OpenmaxMpeg4AO::BaseComponentProxyGetConfig;
    iOmxComponent.SetConfig = OpenmaxMpeg4AO::BaseComponentProxySetConfig;
    iOmxComponent.GetExtensionIndex = OpenmaxMpeg4AO::BaseComponentProxyGetExtensionIndex;
    iOmxComponent.GetState = OpenmaxMpeg4AO::BaseComponentProxyGetState;
    iOmxComponent.UseBuffer = OpenmaxMpeg4AO::BaseComponentProxyUseBuffer;
    iOmxComponent.AllocateBuffer = OpenmaxMpeg4AO::BaseComponentProxyAllocateBuffer;
    iOmxComponent.FreeBuffer = OpenmaxMpeg4AO::BaseComponentProxyFreeBuffer;
    iOmxComponent.EmptyThisBuffer = OpenmaxMpeg4AO::BaseComponentProxyEmptyThisBuffer;
    iOmxComponent.FillThisBuffer = OpenmaxMpeg4AO::BaseComponentProxyFillThisBuffer;
#else
    iPVCapabilityFlags.iIsOMXComponentMultiThreaded = OMX_FALSE;

    iOmxComponent.SendCommand = OpenmaxMpeg4AO::BaseComponentSendCommand;
    iOmxComponent.GetParameter = OpenmaxMpeg4AO::BaseComponentGetParameter;
    iOmxComponent.SetParameter = OpenmaxMpeg4AO::BaseComponentSetParameter;
    iOmxComponent.GetConfig = OpenmaxMpeg4AO::BaseComponentGetConfig;
    iOmxComponent.SetConfig = OpenmaxMpeg4AO::BaseComponentSetConfig;
    iOmxComponent.GetExtensionIndex = OpenmaxMpeg4AO::BaseComponentGetExtensionIndex;
    iOmxComponent.GetState = OpenmaxMpeg4AO::BaseComponentGetState;
    iOmxComponent.UseBuffer = OpenmaxMpeg4AO::BaseComponentUseBuffer;
    iOmxComponent.AllocateBuffer = OpenmaxMpeg4AO::BaseComponentAllocateBuffer;
    iOmxComponent.FreeBuffer = OpenmaxMpeg4AO::BaseComponentFreeBuffer;
    iOmxComponent.EmptyThisBuffer = OpenmaxMpeg4AO::BaseComponentEmptyThisBuffer;
    iOmxComponent.FillThisBuffer = OpenmaxMpeg4AO::BaseComponentFillThisBuffer;
#endif

    iOmxComponent.SetCallbacks = OpenmaxMpeg4AO::BaseComponentSetCallbacks;
    iOmxComponent.nVersion.s.nVersionMajor = SPECVERSIONMAJOR;
    iOmxComponent.nVersion.s.nVersionMinor = SPECVERSIONMINOR;
    iOmxComponent.nVersion.s.nRevision = SPECREVISION;
    iOmxComponent.nVersion.s.nStep = SPECSTEP;

    // PV capability
    iPVCapabilityFlags.iOMXComponentSupportsExternalInputBufferAlloc = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentSupportsExternalOutputBufferAlloc  = OMX_FALSE;//modified by jgdu
    iPVCapabilityFlags.iOMXComponentSupportsMovableInputBuffers = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentSupportsPartialFrames = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentUsesNALStartCodes = OMX_FALSE;
    iPVCapabilityFlags.iOMXComponentCanHandleIncompleteFrames = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentUsesFullAVCFrames = OMX_FALSE;
    iPVCapabilityFlags.iOMXComponentUsesInterleaved2BNALSizes = OMX_FALSE;
    iPVCapabilityFlags.iOMXComponentUsesInterleaved4BNALSizes = OMX_FALSE;

    if (ipAppPriv)
    {
        oscl_free(ipAppPriv);
        ipAppPriv = NULL;
    }

    ipAppPriv = (ComponentPrivateType*) oscl_malloc(sizeof(ComponentPrivateType));

    if (NULL == ipAppPriv)
    {
        return OMX_ErrorInsufficientResources;
    }

    //Construct base class now
    Status = ConstructBaseComponent(pAppData);

    if (OMX_ErrorNone != Status)
    {
        return Status;
    }

    /** Domain specific section for the ports. */
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.nPortIndex = OMX_PORT_INPUTPORT_INDEX;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.eDomain = OMX_PortDomainVideo;
    if (iDecMode == MODE_MPEG4)
    {
        ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.cMIMEType = (OMX_STRING)"video/mpeg4";
        ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG4;

    }
    else if (iDecMode == MODE_H263)
    {
        ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.cMIMEType = (OMX_STRING)"video/h263";
        ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.eCompressionFormat = OMX_VIDEO_CodingH263;

    }

    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.pNativeRender = 0;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.bFlagErrorConcealment = OMX_FALSE;

    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.nFrameWidth = 176;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.nFrameHeight = 144;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.nBitrate = 64000;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.xFramerate = (15 << 16);
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.eDir = OMX_DirInput;
    //Set to a default value, will change later during setparameter call
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.nBufferCountActual = NUMBER_INPUT_BUFFER_MP4;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.nBufferCountMin = 1;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.nBufferSize = INPUT_BUFFER_SIZE_MP4;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.bEnabled = OMX_TRUE;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.bPopulated = OMX_FALSE;


    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.nPortIndex = OMX_PORT_OUTPUTPORT_INDEX;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.eDomain = OMX_PortDomainVideo;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.cMIMEType = (OMX_STRING)"raw";
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.pNativeRender = 0;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.bFlagErrorConcealment = OMX_FALSE;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    // ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.eColorFormat = OMX_COLOR_FormatYUV420Planar;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar; //modified by jgdu
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.nFrameWidth = 176; //320; //176;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.nStride = 176;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.nFrameHeight = 144; //240; //144;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.nSliceHeight = 144;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.nBitrate = 64000;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.xFramerate = (15 << 16);
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.eDir = OMX_DirOutput;
    //Set to a default value, will change later during setparameter call
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.nBufferCountActual = NUMBER_OUTPUT_BUFFER_MP4;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.nBufferCountMin = 2;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.nBufferSize = OUTPUT_BUFFER_SIZE_MP4;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.bEnabled = OMX_TRUE;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.bPopulated = OMX_FALSE;

    if (iDecMode == MODE_MPEG4)
    {
        //Default values for mpeg4 video param port
        ipPorts[OMX_PORT_INPUTPORT_INDEX]->VideoMpeg4.nPortIndex = OMX_PORT_INPUTPORT_INDEX;
        ipPorts[OMX_PORT_INPUTPORT_INDEX]->VideoMpeg4.eProfile = OMX_VIDEO_MPEG4ProfileSimple;
        ipPorts[OMX_PORT_INPUTPORT_INDEX]->VideoMpeg4.eLevel = OMX_VIDEO_MPEG4Level3;

        ipPorts[OMX_PORT_INPUTPORT_INDEX]->ProfileLevel.nPortIndex = OMX_PORT_INPUTPORT_INDEX;
        ipPorts[OMX_PORT_INPUTPORT_INDEX]->ProfileLevel.nProfileIndex = 0;
        ipPorts[OMX_PORT_INPUTPORT_INDEX]->ProfileLevel.eProfile = OMX_VIDEO_MPEG4ProfileSimple;
        ipPorts[OMX_PORT_INPUTPORT_INDEX]->ProfileLevel.eLevel = OMX_VIDEO_MPEG4Level3;
        oscl_strncpy((OMX_STRING)iComponentRole, (OMX_STRING)"video_decoder.mpeg4", OMX_MAX_STRINGNAME_SIZE);
    }
    else if (iDecMode == MODE_H263)
    {
        //Default values for h263 video param port
        ipPorts[OMX_PORT_INPUTPORT_INDEX]->VideoH263.nPortIndex = OMX_PORT_INPUTPORT_INDEX;
        ipPorts[OMX_PORT_INPUTPORT_INDEX]->VideoH263.eProfile = (OMX_VIDEO_H263PROFILETYPE)(OMX_VIDEO_H263ProfileBaseline | OMX_VIDEO_H263ProfileISWV2);
        ipPorts[OMX_PORT_INPUTPORT_INDEX]->VideoH263.eLevel = OMX_VIDEO_H263Level45;

        ipPorts[OMX_PORT_INPUTPORT_INDEX]->ProfileLevel.nPortIndex = OMX_PORT_INPUTPORT_INDEX;
        ipPorts[OMX_PORT_INPUTPORT_INDEX]->ProfileLevel.nProfileIndex = 0;
        ipPorts[OMX_PORT_INPUTPORT_INDEX]->ProfileLevel.eProfile = OMX_VIDEO_H263ProfileBaseline | OMX_VIDEO_H263ProfileISWV2;
        ipPorts[OMX_PORT_INPUTPORT_INDEX]->ProfileLevel.eLevel = OMX_VIDEO_H263Level45;
        oscl_strncpy((OMX_STRING)iComponentRole, (OMX_STRING)"video_decoder.h263", OMX_MAX_STRINGNAME_SIZE);
    }

    iPortTypesParam.nPorts = 2;
    iPortTypesParam.nStartPortNumber = 0;

    pInPort = (ComponentPortType*) ipPorts[OMX_PORT_INPUTPORT_INDEX];
    pOutPort = (ComponentPortType*) ipPorts[OMX_PORT_OUTPUTPORT_INDEX];

    pInPort->ActualNumPortFormatsSupported = 1;

    //OMX_VIDEO_PARAM_PORTFORMATTYPE INPUT PORT SETTINGS
    //On input port for index 0
    SetHeader(&pInPort->VideoParam[0], sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
    pInPort->VideoParam[0].nPortIndex = 0;
    pInPort->VideoParam[0].nIndex = 0;

    if (iDecMode == MODE_MPEG4)
    {
        pInPort->VideoParam[0].eCompressionFormat = OMX_VIDEO_CodingMPEG4;

    }
    else if (iDecMode == MODE_H263)
    {
        pInPort->VideoParam[0].eCompressionFormat = OMX_VIDEO_CodingH263;
    }

    pInPort->VideoParam[0].eColorFormat = OMX_COLOR_FormatUnused;

    pOutPort->ActualNumPortFormatsSupported = 1;

    //OMX_VIDEO_PARAM_PORTFORMATTYPE OUTPUT PORT SETTINGS
    //On output port for index 0
    SetHeader(&pOutPort->VideoParam[0], sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
    pOutPort->VideoParam[0].nPortIndex = 1;
    pOutPort->VideoParam[0].nIndex = 0;
    pOutPort->VideoParam[0].eCompressionFormat = OMX_VIDEO_CodingUnused;
    // pOutPort->VideoParam[0].eColorFormat = OMX_COLOR_FormatYUV420Planar;
    pOutPort->VideoParam[0].eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar; //modified by jgdu


    //OMX_PARAM_DEBLOCKINGTYPE settings of the mpeg4/h263 output port
    SetHeader(&pOutPort->VideoDeBlocking, sizeof(OMX_PARAM_DEBLOCKINGTYPE));
    pOutPort->VideoDeBlocking.nPortIndex = OMX_PORT_OUTPUTPORT_INDEX;
    pOutPort->VideoDeBlocking.bDeblocking = OMX_FALSE;    //Keep deblocking off by default

    iUseExtTimestamp = OMX_TRUE;


    if (ipMpegDecoderObject)
    {
        OSCL_DELETE(ipMpegDecoderObject);
        ipMpegDecoderObject = NULL;
    }
    ipMpegDecoderObject = OSCL_NEW(Mpeg4Decoder_OMX, ((OmxComponentBase*)this));
    if(Mpeg4Decoder_OMX::g_mpeg4_dec_inst_num>=1)
    {
 	OMX_MP4DEC_INFO ("Mpeg4Decoder_OMX more than 1 inst\n");   	
    	return OMX_ErrorInsufficientResources;
    }	
    Mpeg4Decoder_OMX::g_mpeg4_dec_inst_num++;	
    //modified by jgdu
    //oscl_memset(&(ipMpegDecoderObject->VideoCtrl), 0, sizeof(VideoDecControls));

#if PROXY_INTERFACE
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentSendCommand = BaseComponentSendCommand;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentGetParameter = BaseComponentGetParameter;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentSetParameter = BaseComponentSetParameter;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentGetConfig = BaseComponentGetConfig;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentSetConfig = BaseComponentSetConfig;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentGetExtensionIndex = OpenmaxMpeg4AOGetExtensionIndex;//BaseComponentGetExtensionIndex;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentGetState = BaseComponentGetState;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentUseBuffer = OpenmaxMpeg4AOUseBuffer;//BaseComponentUseBuffer;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentAllocateBuffer = OpenmaxMpeg4AOAllocateBuffer;//BaseComponentAllocateBuffer;//modified by jgdu
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentFreeBuffer = OpenmaxMpeg4AOFreeBuffer;//BaseComponentFreeBuffer;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentEmptyThisBuffer = BaseComponentEmptyThisBuffer;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentFillThisBuffer = BaseComponentFillThisBuffer;
#endif

    return OMX_ErrorNone;
}


/** This function is called by the omx core when the component
    * is disposed by the IL client with a call to FreeHandle().
    */

OMX_ERRORTYPE OpenmaxMpeg4AO::DestroyComponent()
{
    if (OMX_FALSE != iIsInit)
    {
        ComponentDeInit();
    }

    //Destroy the base class now
    DestroyBaseComponent();

    if (ipMpegDecoderObject)
    {
        OSCL_DELETE(ipMpegDecoderObject);
        ipMpegDecoderObject = NULL;
    }

    if (ipAppPriv)
    {
        ipAppPriv->CompHandle = NULL;

        oscl_free(ipAppPriv);
        ipAppPriv = NULL;
    }
	
    if(Mpeg4Decoder_OMX::g_mpeg4_dec_inst_num>0)
	Mpeg4Decoder_OMX::g_mpeg4_dec_inst_num--;
	
    return OMX_ErrorNone;
}



/* This function will be called in case of buffer management without marker bit present
 * The purpose is to copy the current input buffer into a big temporary buffer, so that
 * an incomplete/partial frame is never passed to the decoder library for decode
 */
void OpenmaxMpeg4AO::ComponentBufferMgmtWithoutMarker()
{
    //This common routine has been written in the base class
    TempInputBufferMgmtWithoutMarker();
}

void OpenmaxMpeg4AO::ProcessData()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMpeg4AO : ProcessData IN"));
    if (!iEndOfFrameFlag)
    {
        DecodeWithoutMarker();
    }
    else
    {
        DecodeWithMarker();
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMpeg4AO : ProcessData OUT"));
}

void OpenmaxMpeg4AO::pop_output_buffer_from_queue(OMX_BUFFERHEADERTYPE *pTargetBuffer)
{
    BufferCtrlStruct *pBCTRL = (BufferCtrlStruct *)pTargetBuffer->pOutputPortPrivate;
    if(pBCTRL->iIsBufferInComponentQueue == OMX_TRUE)
    {
	OMX_MP4DEC_INFO ("pop_output_buffer_from_queue %x",pTargetBuffer);
    	QueueType* pOutputQueue = ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->pBufferQueue;
    	OMX_BUFFERHEADERTYPE *pOutputBuffer;
    	int QueueNumElem = GetQueueNumElem(pOutputQueue);
    	for(int i=0;i< QueueNumElem;i++)
    	{
            pOutputBuffer = (OMX_BUFFERHEADERTYPE*) DeQueue(pOutputQueue);
	    if(pTargetBuffer==pOutputBuffer)	
	   	break;
	    else
	    	Queue(pOutputQueue, (void*)pOutputBuffer);	
    	}	
    }else if(pBCTRL->iRefCount == 0)
    {
    	iNumAvailableOutputBuffers++;
    }
}

void OpenmaxMpeg4AO::DecodeWithoutMarker()
{
    OMX_MP4DEC_INFO ("OpenmaxMpeg4AO : DecodeWithoutMarker IN\n");
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMpeg4AO : DecodeWithoutMarker IN"));

    QueueType* pInputQueue = ipPorts[OMX_PORT_INPUTPORT_INDEX]->pBufferQueue;
    QueueType* pOutputQueue = ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->pBufferQueue;
    ComponentPortType*  pOutPort = ipPorts[OMX_PORT_OUTPUTPORT_INDEX];
    OMX_COMPONENTTYPE  *pHandle = &iOmxComponent;

    OMX_U32                 OutputLength;
    OMX_U8*                 pTempInBuffer;
    OMX_U32                 TempInLength;
    OMX_BOOL                DecodeReturn;
    OMX_BOOL                MarkerFlag = OMX_FALSE;
    OMX_BOOL                ResizeNeeded = OMX_FALSE;

    OMX_U32 TempInputBufferSize = (2 * sizeof(uint8) * (ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.nBufferSize));

    OMX_U32 CurrWidth =  ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.nFrameWidth;
    OMX_U32 CurrHeight = ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.nFrameHeight;

    if ((!iIsInputBufferEnded) || iEndofStream)
    {
         //modified by jgdu Check whether prev output bufer has been released or not
        if (OMX_TRUE == iNewOutBufRequired)
        {    
        //Check whether a new output buffer is available or not

            if (iNumAvailableOutputBuffers == 0)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMpeg4AO : DecodeWithoutMarker OUT output buffer unavailable"));
                //Store the mark data for output buffer, as it will be overwritten next time
                if (NULL != ipTargetComponent)
                {
                    ipTempTargetComponent = ipTargetComponent;
                    iTempTargetMarkData = iTargetMarkData;
                    iMarkPropagate = OMX_TRUE;
                }
                return;
            }

            // Dequeue until getting a valid (available output buffer)
            BufferCtrlStruct *pBCTRL = NULL;
            do
            {
                ipOutputBuffer = (OMX_BUFFERHEADERTYPE*) DeQueue(pOutputQueue);
                if (NULL == ipOutputBuffer)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMpeg4AO : DecodeWithoutMarker Error, output buffer dequeue returned NULL, OUT"));
                    return;
                }

                pBCTRL = (BufferCtrlStruct *)ipOutputBuffer->pOutputPortPrivate;
                if (pBCTRL->iRefCount == 0)
                {
                    //buffer is available
                    iNumAvailableOutputBuffers--;
    		 //modified by jgdu
    		 pBCTRL->iIsBufferInComponentQueue = OMX_FALSE;					
                }
                else
                {
                    Queue(pOutputQueue, (void*)ipOutputBuffer);
                }
            }
            while (pBCTRL->iRefCount > 0);

            //Do not proceed if the output buffer can't fit the YUV data
            if ((ipOutputBuffer->nAllocLen < (OMX_U32)((((CurrWidth + 15) >> 4) << 4) *(((CurrHeight + 15) >> 4) << 4) * 3 / 2)) && (OMX_TRUE == ipMpegDecoderObject->Mpeg4InitCompleteFlag))
            {
                ipOutputBuffer->nFilledLen = 0;
                ReturnOutputBuffer(ipOutputBuffer, pOutPort);
                ipOutputBuffer = NULL;
                return;
            }

            ipOutputBuffer->nFilledLen = 0;
    	//modified by jgdu
    	iNewOutBufRequired = OMX_FALSE;

            /* Code for the marking buffer. Takes care of the OMX_CommandMarkBuffer
             * command and hMarkTargetComponent as given by the specifications
             */
            if (ipMark != NULL)
            {
                ipOutputBuffer->hMarkTargetComponent = ipMark->hMarkTargetComponent;
                ipOutputBuffer->pMarkData = ipMark->pMarkData;
                ipMark = NULL;
            }

            if ((OMX_TRUE == iMarkPropagate) && (ipTempTargetComponent != ipTargetComponent))
            {
                ipOutputBuffer->hMarkTargetComponent = ipTempTargetComponent;
                ipOutputBuffer->pMarkData = iTempTargetMarkData;
                ipTempTargetComponent = NULL;
                iMarkPropagate = OMX_FALSE;
            } else if (ipTargetComponent != NULL)
            {
                ipOutputBuffer->hMarkTargetComponent = ipTargetComponent;
                ipOutputBuffer->pMarkData = iTargetMarkData;
                ipTargetComponent = NULL;
                iMarkPropagate = OMX_FALSE;
            }
        //Mark buffer code ends here
        }
        OutputLength = 0;

        pTempInBuffer = ipTempInputBuffer + iTempConsumedLength;
        TempInLength = iTempInputBufferLength;

	//modified by jgdu
        ipOutputBufferForRendering = NULL;
   	OMX_BOOL  need_new_pic = OMX_TRUE;
	
        OMX_BOOL notSupport = OMX_FALSE;
	//Output buffer is passed as a short pointer
        DecodeReturn = ipMpegDecoderObject->Mp4DecodeVideo(&need_new_pic,ipOutputBuffer, (OMX_U32*) & OutputLength,
                       &(pTempInBuffer),
                       &TempInLength,
                       &(ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam),
                       ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->VideoDeBlocking.bDeblocking,
                       &iFrameCount,
                       MarkerFlag,
                       &ResizeNeeded,&ipOutputBufferForRendering,NULL,&notSupport);
	 
        ipOutputBuffer->nFilledLen = OutputLength;

        //offset not required in our case, set it to zero
        ipOutputBuffer->nOffset = 0;

	 if(notSupport)
	 {
            	(*(ipCallbacks->EventHandler))
            	(pHandle,
             	iCallbackData,
             	OMX_EventError,
             	OMX_ErrorBadParameter,
             	0,
             	NULL);	 
	 }
		 
        //If decoder returned error, report it to the client via a callback
        if (!DecodeReturn && OMX_FALSE == ipMpegDecoderObject->Mpeg4InitCompleteFlag)
        {
            // initialization error, stop playback
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMpeg4AO : DecodeWithoutMarker ErrorBadParameter callback send"));
/*jgdu
            (*(ipCallbacks->EventHandler))
            (pHandle,
             iCallbackData,
             OMX_EventError,
             OMX_ErrorBadParameter,
             0,
             NULL);
*/            
        } else if (!DecodeReturn && OMX_FALSE == iEndofStream)
        {
            // decoding error
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMpeg4AO : DecodeWithoutMarker ErrorStreamCorrupt callback send"));

            (*(ipCallbacks->EventHandler))
            (pHandle,
             iCallbackData,
             OMX_EventError,
             OMX_ErrorStreamCorrupt,
             0,
             NULL);
        }

        if (ResizeNeeded == OMX_TRUE)
        {
            // send port settings changed event
            OMX_COMPONENTTYPE* pHandle = (OMX_COMPONENTTYPE*) ipAppPriv->CompHandle;
			
            // set the flag to disable further processing until Client reacts to this
            //  by doing dynamic port reconfiguration
            iResizePending = OMX_TRUE;
			
            // make sure to release buffers held by decoder
            ReleaseReferenceBuffers(); //modified by jgdu
				
            (*(ipCallbacks->EventHandler))
            (pHandle,
             iCallbackData,
             OMX_EventPortSettingsChanged, //The command was completed
             OMX_PORT_OUTPUTPORT_INDEX,
             0,
             NULL);

        }
		
        //Set the timestamp equal to the input buffer timestamp
        ipOutputBuffer->nTimeStamp = iFrameTimestamp;

	 //modified by jgdu
	 if(need_new_pic)
	 {
		iNewOutBufRequired = OMX_TRUE;
		ipOutputBuffer = NULL;
	 }
	 
        iTempConsumedLength += (iTempInputBufferLength - TempInLength);
        iTempInputBufferLength = TempInLength;

        //Do not decode if big buffer is less than half the size
        if (TempInLength < (TempInputBufferSize >> 1))
        {
            iIsInputBufferEnded = OMX_TRUE;
            iNewInBufferRequired = OMX_TRUE;
        }


        /* If EOS flag has come from the client & there are no more
         * input buffers to decode, send the callback to the client
         */
        if (OMX_TRUE == iEndofStream)
        {
       	   //modified by jgdu
	   if(!iFlushOutputStatus)
	   {
                if ((0 == iTempInputBufferLength) || (!DecodeReturn))
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMpeg4AO : DecodeWithoutMarker EOS callback send"));

                    (*(ipCallbacks->EventHandler))
                        (pHandle,
                        iCallbackData,
                        OMX_EventBufferFlag,
                        1,
                        OMX_BUFFERFLAG_EOS,
                        NULL);

                    iNewInBufferRequired = OMX_TRUE;
                    iEndofStream = OMX_FALSE;

                    if (NULL != ipOutputBuffer)
                    {
                        ipOutputBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
                        ReturnOutputBuffer(ipOutputBuffer, pOutPort);
                        ipOutputBuffer = NULL;
                    }

                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMpeg4AO : DecodeWithoutMarker OUT"));

                    return;
                }
	   }else
	   {
	   	ipOutputBufferForRendering = NULL;
		iFlushOutputStatus = ipMpegDecoderObject->FlushOutput_OMX(&ipOutputBufferForRendering);
		if(OMX_FALSE !=iFlushOutputStatus )
		{
		    if(ipOutputBufferForRendering)
                    {
	 		pop_output_buffer_from_queue(ipOutputBufferForRendering);		
			ReturnOutputBuffer(ipOutputBufferForRendering, pOutPort);
			ipOutputBufferForRendering = NULL;
                    }
                    RunIfNotReady();
                    if(OMX_TRUE == iNewOutBufRequired && ipOutputBuffer != NULL)
		    {
			iNewOutBufRequired = OMX_FALSE;
		    }
		    return;
		}else
		{
		     RunIfNotReady();
		}
            }
        }

	//modified by jgdu
	/*
        //Send the output buffer back after decode
        ReturnOutputBuffer(ipOutputBuffer, pOutPort);
        ipOutputBuffer = NULL;
	*/
	if(ipOutputBufferForRendering)
	{
            pop_output_buffer_from_queue(ipOutputBufferForRendering);	
	    ReturnOutputBuffer(ipOutputBufferForRendering, pOutPort);
	    ipOutputBufferForRendering = NULL;
	}
	if(OMX_TRUE == iNewOutBufRequired && ipOutputBuffer != NULL)
	{
	    iNewOutBufRequired = OMX_FALSE;
	}
        /* If there is some more processing left with current buffers, re-schedule the AO
         * Do not go for more than one round of processing at a time.
         * This may block the AO longer than required.
         */
        //modified by jgdu
        //if ((TempInLength != 0 || GetQueueNumElem(pInputQueue) > 0) && (iNumAvailableOutputBuffers > 0) && (ResizeNeeded == OMX_FALSE))
	if ((TempInLength != 0 || GetQueueNumElem(pInputQueue) > 0) && ((iNumAvailableOutputBuffers > 0)||(OMX_FALSE== iNewOutBufRequired)) && (ResizeNeeded == OMX_FALSE))
	{
            RunIfNotReady();
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMpeg4AO : DecodeWithoutMarker OUT"));
    OMX_MP4DEC_INFO("OpenmaxMpeg4AO : DecodeWithoutMarker OUT\n");
    return;
}

void OpenmaxMpeg4AO::DecodeWithMarker()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMpeg4AO : DecodeWithMarker IN"));
    OMX_MP4DEC_DEBUG("OpenmaxMpeg4AO : DecodeWithMarker IN\n");
    if(iLogCount%4==0)
    {
        OMX_MP4DEC_INFO("OMXMpeg4::Dec %d,%d,%d,%d,%x:%d\n",iOutBufferCount,iNewOutBufRequired,iNumAvailableOutputBuffers,GetQueueNumElem( ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->pBufferQueue),ipOutputBuffer,iFrameCount);		
    }
    iLogCount++;
/*
{
	BufferCtrlStruct *pBCTRL = NULL;
       OMX_BUFFERHEADERTYPE*   ipOutputBuffer;	
	QueueType* pOutputQueue = ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->pBufferQueue;	
	int eleInQueue = GetQueueNumElem( ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->pBufferQueue);
	for(int i=0;i<eleInQueue;i++)
	{
		ipOutputBuffer = (OMX_BUFFERHEADERTYPE*) DeQueue(pOutputQueue);
		pBCTRL = (BufferCtrlStruct *)ipOutputBuffer->pOutputPortPrivate;
		OMX_MP4DEC_DEBUG("buffer status %x ,ref %d,is %d\n",ipOutputBuffer->pBuffer,pBCTRL->iRefCount,pBCTRL->iIsBufferInComponentQueue);
		Queue(pOutputQueue, (void*)ipOutputBuffer);
	}
 }
 */
    QueueType* pInputQueue = ipPorts[OMX_PORT_INPUTPORT_INDEX]->pBufferQueue;
    QueueType* pOutputQueue = ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->pBufferQueue;

    ComponentPortType*  pInPort = ipPorts[OMX_PORT_INPUTPORT_INDEX];
    ComponentPortType*  pOutPort = ipPorts[OMX_PORT_OUTPUTPORT_INDEX];

    OMX_U32                 OutputLength;
    OMX_BOOL                DecodeReturn = OMX_FALSE;
    OMX_BOOL                MarkerFlag = OMX_TRUE;
    OMX_COMPONENTTYPE *     pHandle = &iOmxComponent;
    OMX_BOOL                ResizeNeeded = OMX_FALSE;
		
    OMX_U32 CurrWidth =  ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.nFrameWidth;
    OMX_U32 CurrHeight = ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.nFrameHeight;

    if ((!iIsInputBufferEnded) || (iEndofStream))
    {
         //modified by jgdu  Check whether prev output bufer has been released or not
        if (OMX_TRUE == iNewOutBufRequired)
        {
            //Check whether a new output buffer is available or not
            if (iNumAvailableOutputBuffers > 0)
            {
                // Dequeue until getting a valid (available output buffer)
                BufferCtrlStruct *pBCTRL = NULL;
                do
                {
                    ipOutputBuffer = (OMX_BUFFERHEADERTYPE*) DeQueue(pOutputQueue);
                    if (NULL == ipOutputBuffer)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMpeg4AO : DecodeWithoutMarker Error, output buffer dequeue returned NULL, OUT"));
                        return;
                    }

                    pBCTRL = (BufferCtrlStruct *)ipOutputBuffer->pOutputPortPrivate;
                    if (pBCTRL->iRefCount == 0)
                    {
                        //buffer is available
                        iNumAvailableOutputBuffers--;
    		    //modified by jgdu
    		    pBCTRL->iIsBufferInComponentQueue = OMX_FALSE;		
                    } else
                    {
                        Queue(pOutputQueue, (void*)ipOutputBuffer);
                    }
                }
                while (pBCTRL->iRefCount > 0);

                if (NULL == ipOutputBuffer)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMpeg4AO : DecodeWithMarker Error, output buffer dequeue returned NULL, OUT"));
                    iNewInBufferRequired = OMX_FALSE;
                    return;
                }

                //Do not proceed if the output buffer can't fit the YUV data
                if ((ipOutputBuffer->nAllocLen < (OMX_U32)((((CurrWidth + 15) >> 4) << 4) *(((CurrHeight + 15) >> 4) << 4) * 3 / 2)) && (OMX_TRUE == ipMpegDecoderObject->Mpeg4InitCompleteFlag))
                {
                    ipOutputBuffer->nFilledLen = 0;
                    ReturnOutputBuffer(ipOutputBuffer, pOutPort);
                    ipOutputBuffer = NULL;
                    return;
                }
                ipOutputBuffer->nFilledLen = 0;
    	   //modified by jgdu
    	    iNewOutBufRequired = OMX_FALSE;
                /* Code for the marking buffer. Takes care of the OMX_CommandMarkBuffer
                 * command and hMarkTargetComponent as given by the specifications
                 */
                if (ipMark != NULL)
                {
                    ipOutputBuffer->hMarkTargetComponent = ipMark->hMarkTargetComponent;
                    ipOutputBuffer->pMarkData = ipMark->pMarkData;
                    ipMark = NULL;
                }

                if (ipTargetComponent != NULL)
                {
                    ipOutputBuffer->hMarkTargetComponent = ipTargetComponent;
                    ipOutputBuffer->pMarkData = iTargetMarkData;
                    ipTargetComponent = NULL;

                }
                //Mark buffer code ends here
            }else// if (OMX_TRUE == ipMpegDecoderObject->Mpeg4InitCompleteFlag)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMpeg4AO : DecodeWithMarker OUT output buffer unavailable"));
                iNewInBufferRequired = OMX_FALSE;
                return;
            }
        }

        if (iInputCurrLength > 0)
        {
            OutputLength = 0;

	    //modified by jgdu
            ipOutputBufferForRendering = NULL;

            OMX_BOOL  need_new_pic = OMX_TRUE;		
            OMX_BOOL notSupport = OMX_FALSE;			
            OMX_MP4DEC_DEBUG("VSP DPB::ipOutputBufferForRendering begin %x,%x,%x\n ",ipOutputBuffer->pBuffer,ipOutputBuffer,(int)iFrameTimestamp);//.nHighPart,ipOutputBufferForRendering->nTimeStamp.nLowPart);	
            //Output buffer is passed as a short pointer
            DecodeReturn = ipMpegDecoderObject->Mp4DecodeVideo(&need_new_pic,ipOutputBuffer, (OMX_U32*) & OutputLength,
                           &(ipFrameDecodeBuffer),
                           &(iInputCurrLength),
                           &(ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam),
                           ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->VideoDeBlocking.bDeblocking,
                           &iFrameCount,
                           MarkerFlag,
                           &ResizeNeeded,&ipOutputBufferForRendering,NULL,&notSupport);

	     if(notSupport)
	     {
            	(*(ipCallbacks->EventHandler))
            	    (pHandle,
             	    iCallbackData,
             	    OMX_EventError,
             	    OMX_ErrorBadParameter,
             	    0,
             	    NULL);	 
	     }
		
            //If decoder returned error, report it to the client via a callback
            if (!DecodeReturn && OMX_FALSE == ipMpegDecoderObject->Mpeg4InitCompleteFlag)
            {
                // initialization error, stop playback
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMpeg4AO : DecodeWithoutMarker ErrorBadParameter callback send"));
/*jgdu
                (*(ipCallbacks->EventHandler))
                (pHandle,
                 iCallbackData,
                 OMX_EventError,
                 OMX_ErrorBadParameter,
                 0,
                 NULL);
*/                 
            } else if (!DecodeReturn && OMX_FALSE == iEndofStream)
            {
                // decode error
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMpeg4AO : DecodeWithMarker ErrorStreamCorrupt callback send"));

                (*(ipCallbacks->EventHandler))
                    (pHandle,
                    iCallbackData,
                    OMX_EventError,
                    OMX_ErrorStreamCorrupt,
                    0,
                    NULL);
            }

            if (ResizeNeeded == OMX_TRUE)
            {
                // send port settings changed event
                OMX_COMPONENTTYPE* pHandle = (OMX_COMPONENTTYPE*) ipAppPriv->CompHandle;
                
                if(ipMpegDecoderObject->iDecoder_sw_flag)
                {
#ifdef YUV_THREE_PLANE
                    OMX_MP4DEC_INFO ("OpenmaxMpeg4AO reset eColorFormat");
                    ComponentPortType *pOutPort = (ComponentPortType*) ipPorts[OMX_PORT_OUTPUTPORT_INDEX];
                    if(!iUseAndroidNativeBuffer[OMX_PORT_OUTPUTPORT_INDEX])
                    {
                        pOutPort->VideoParam[0].eColorFormat = OMX_COLOR_FormatYUV420Planar;
		      	pOutPort->PortParam.format.video.eColorFormat = OMX_COLOR_FormatYUV420Planar;
                    }else
                    {
                        pOutPort->VideoParam[0].eColorFormat = (OMX_COLOR_FORMATTYPE)HAL_PIXEL_FORMAT_YV12;
    			pOutPort->PortParam.format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)HAL_PIXEL_FORMAT_YV12;
                    }
#endif
                }

                iResizePending = OMX_TRUE;
                // make sure to release buffers held by decoder
                ReleaseReferenceBuffers(); //modified by jgdu	
                
                (*(ipCallbacks->EventHandler))
                    (pHandle,
                    iCallbackData,
                    OMX_EventPortSettingsChanged, //The command was completed
                    OMX_PORT_OUTPUTPORT_INDEX,
                    0,
                    NULL);
            }

            /* Discard the input frame if it is with the marker bit & decoder fails*/
            if (iInputCurrLength == 0 || !DecodeReturn)
            {
                ipInputBuffer->nFilledLen = 0;
                if(ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->VideoDeBlocking.bDeblocking)
                {		
                    //ReturnInputBuffer(ipInputBuffer, pInPort); to speed up vt
                    //ipInputBuffer = NULL;                
                    need_to_send_empty_done = OMX_TRUE;
                }else
                {
                    ReturnInputBuffer(ipInputBuffer, pInPort);
                    ipInputBuffer = NULL; 		  
                }			
                iNewInBufferRequired = OMX_TRUE;
                iIsInputBufferEnded = OMX_TRUE;
                iUseExtTimestamp = OMX_TRUE;
                iInputCurrLength = 0;
            } else
            {
                iNewInBufferRequired = OMX_FALSE;
                iIsInputBufferEnded = OMX_FALSE;
                iUseExtTimestamp = OMX_FALSE;
            }

            if (NULL != ipOutputBuffer)
            {
                ipOutputBuffer->nFilledLen = OutputLength;
                //offset not required in our case, set it to zero
                ipOutputBuffer->nOffset = 0;
                //Set the timestamp equal to the input buffer timestamp
                if (OMX_TRUE == iUseExtTimestamp)
                {
                    ipOutputBuffer->nTimeStamp = iFrameTimestamp;
                }
            }
	    //modified by jgdu
	    if(need_new_pic)
	    {
		 iNewOutBufRequired = OMX_TRUE;
		 ipOutputBuffer = NULL;
	    }		
        } else if (iEndofStream == OMX_FALSE)
        {
            // it's possible that after partial frame assembly, the input buffer still remains empty (due to
            // client erroneously sending such buffers). This code adds robustness in the sense that it returns such buffer to the client

            ipInputBuffer->nFilledLen = 0;
            ReturnInputBuffer(ipInputBuffer, pInPort);
            ipInputBuffer = NULL;
            iNewInBufferRequired = OMX_TRUE;
            iIsInputBufferEnded = OMX_TRUE;
            iUseExtTimestamp = OMX_TRUE;
        }

        /* If EOS flag has come from the client & there are no more
         * input buffers to decode, send the callback to the client
         */
        
        if (OMX_TRUE == iEndofStream)
        { 
            if(need_to_send_empty_done)
            {
	        ReturnInputBuffer(ipInputBuffer, pInPort);	
                ipInputBuffer = NULL;	 
		need_to_send_empty_done = OMX_FALSE;	 
            }        
            OMX_MP4DEC_INFO ("OpenmaxMpeg4AO::DecodeWithMarker iEndofStream\n");	
	     //modified by jgdu
	    if(!iFlushOutputStatus)
	    {
                if (!DecodeReturn)
                {		   
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMpeg4AO : DecodeWithMarker EOS callback send"));

                    (*(ipCallbacks->EventHandler))
                        (pHandle,
                        iCallbackData,
                        OMX_EventBufferFlag,
                        1,
                        OMX_BUFFERFLAG_EOS,
                        NULL);

                    iNewInBufferRequired = OMX_TRUE;
                    //Mark this flag false once the callback has been send back
                    iEndofStream = OMX_FALSE;

                    if (NULL != ipOutputBuffer)
                    {
                        ipOutputBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
                        ReturnOutputBuffer(ipOutputBuffer, pOutPort);
                        ipOutputBuffer = NULL;
                    }

                    if ((iNumInputBuffer != 0) && (NULL != ipInputBuffer))
                    {
                        ReturnInputBuffer(ipInputBuffer, pInPort);
                        ipInputBuffer = NULL;
                        iIsInputBufferEnded = OMX_TRUE;
                        iInputCurrLength = 0;
                    }
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMpeg4AO : DecodeWithMarker OUT"));
                    return;
                }
	    }else
	    {
		ipOutputBufferForRendering = NULL;
		iFlushOutputStatus = ipMpegDecoderObject->FlushOutput_OMX(&ipOutputBufferForRendering);
		if(OMX_FALSE !=iFlushOutputStatus )
		{
                    if(ipOutputBufferForRendering)
                    {
			OMX_MP4DEC_DEBUG("VSP DPB::ipOutputBufferForRendering  end  %x,%x,%x\n ",ipOutputBufferForRendering->pBuffer,ipOutputBufferForRendering,(int)ipOutputBufferForRendering->nTimeStamp);//.nHighPart,ipOutputBufferForRendering->nTimeStamp.nLowPart);
                        pop_output_buffer_from_queue(ipOutputBufferForRendering);
			ReturnOutputBuffer(ipOutputBufferForRendering, pOutPort);
			ipOutputBufferForRendering = NULL;
                    }
                    RunIfNotReady();
                    if(OMX_TRUE == iNewOutBufRequired && ipOutputBuffer != NULL)
                    {
			iNewOutBufRequired = OMX_FALSE;
                    }
                    return;
		}else
		{
                    RunIfNotReady();
		}
	    }
        }

       //modified by jgdu
       /*
        //Send the output buffer back after decode
        if (NULL != ipOutputBuffer)
        {
            ReturnOutputBuffer(ipOutputBuffer, pOutPort);
            ipOutputBuffer = NULL;
        }
	*/
	if(ipOutputBufferForRendering)
	{
            OMX_MP4DEC_DEBUG("VSP DPB::ipOutputBufferForRendering end %x,%x,%x\n ",ipOutputBufferForRendering->pBuffer,ipOutputBufferForRendering,(int)ipOutputBufferForRendering->nTimeStamp);//.nHighPart,ipOutputBufferForRendering->nTimeStamp.nLowPart);
	    pop_output_buffer_from_queue(ipOutputBufferForRendering);
	    ReturnOutputBuffer(ipOutputBufferForRendering, pOutPort);
	    ipOutputBufferForRendering = NULL;
	}
	if(OMX_TRUE == iNewOutBufRequired && ipOutputBuffer != NULL)
	{
	    iNewOutBufRequired = OMX_FALSE;
	}

       if(need_to_send_empty_done)
        {
            ReturnInputBuffer(ipInputBuffer, pInPort);
            ipInputBuffer = NULL;
            need_to_send_empty_done = OMX_FALSE;	 
       } 	
        /* If there is some more processing left with current buffers, re-schedule the AO
         * Do not go for more than one round of processing at a time.
         * This may block the AO longer than required.
         */
        if ((iInputCurrLength != 0 || GetQueueNumElem(pInputQueue) > 0)
 //               && (iNumAvailableOutputBuffers > 0) && (ResizeNeeded == OMX_FALSE))
            && ((iNumAvailableOutputBuffers > 0)||(OMX_FALSE== iNewOutBufRequired)) && (ResizeNeeded == OMX_FALSE))//modified by jgdu
        {
            RunIfNotReady();
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMpeg4AO : DecodeWithMarker OUT"));
    OMX_MP4DEC_DEBUG ( "OpenmaxMpeg4AO : DecodeWithMarker OUT\n");
    return;
}

//Component constructor
OpenmaxMpeg4AO::OpenmaxMpeg4AO()
{
    iUseExtTimestamp = OMX_TRUE;
    ipMpegDecoderObject = NULL;

    if (!IsAdded())
    {
        AddToScheduler();
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMpeg4AO : constructed"));
}


//Active object destructor
OpenmaxMpeg4AO::~OpenmaxMpeg4AO()
{
    if (IsAdded())
    {
        RemoveFromScheduler();
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMpeg4AO : destructed"));
}


/** The Initialization function
 */
OMX_ERRORTYPE OpenmaxMpeg4AO::ComponentInit()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMpeg4AO : ComponentInit IN"));

    OMX_ERRORTYPE Status = OMX_ErrorNone;

    if (OMX_TRUE == iIsInit)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMpeg4AO : ComponentInit error incorrect operation"));
        return OMX_ErrorIncorrectStateOperation;
    }
    iIsInit = OMX_TRUE;


    if (!iCodecReady)
    {
        //Call the init routine here in case of H263 mode, without waiting for buffers

        if (iDecMode == MODE_H263)
        {
            OMX_S32 Width, Height, Size = 0;
            OMX_U8* Buff = NULL;
            //OMX_BOOL DeBlocking = OMX_FALSE;//jgdu
	    OMX_BOOL DeBlocking = ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->VideoDeBlocking.bDeblocking;
	    OMX_MP4DEC_INFO("DeBlocking %d",DeBlocking);
		
            //Pass dummy pointers during initializations
             OMX_BOOL notSupport = OMX_FALSE;
            if (OMX_TRUE != ipMpegDecoderObject->InitializeVideoDecode(&Width, &Height, &Buff, &Size, iDecMode, DeBlocking, &notSupport))
            {
                Status = OMX_ErrorInsufficientResources;
            }
        } else
        {
            //mp4 lib init
            Status = ipMpegDecoderObject->Mp4DecInit();
        }

        iCodecReady = OMX_TRUE;
    }

    iUseExtTimestamp = OMX_TRUE;
    iInputCurrLength = 0;

    //Used in dynamic port reconfiguration
    iFrameCount = 0;
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMpeg4AO : ComponentInit OUT"));

    return Status;

}

/** This function is called upon a transition to the idle or invalid state.
 *  Also it is called by the ComponentDestructor() function
 */
OMX_ERRORTYPE OpenmaxMpeg4AO::ComponentDeInit()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMpeg4AO : ComponentDeInit IN"));

    OMX_ERRORTYPE Status = OMX_ErrorNone;

    iIsInit = OMX_FALSE;

    if (iCodecReady)
    {
        Status = ipMpegDecoderObject->Mp4DecDeinit();
        iCodecReady = OMX_FALSE;
    }

#if PROFILING_ON
    if (0 != iFrameCount)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iDiagnosticsLogger, PVLOGMSG_INFO, (0, "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"));

        PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iDiagnosticsLogger, PVLOGMSG_INFO, (0, "OpenmaxMpeg4AO - Total Decoding Time (ms) = %d", OsclTickCount::TicksToMsec(ipMpegDecoderObject->iTotalTicks)));
        PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iDiagnosticsLogger, PVLOGMSG_INFO, (0, "OpenmaxMpeg4AO - Total Number of Decoded Frames = %d", iFrameCount));
    }
#endif

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMpeg4AO : ComponentDeInit OUT"));

    return Status;

}

OMX_ERRORTYPE OpenmaxMpeg4AO::GetConfig(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nIndex,
    OMX_INOUT OMX_PTR pComponentConfigStructure)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
       OpenmaxMpeg4AO* pOpenmaxAOType = (OpenmaxMpeg4AO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;   
    	if (NULL == pOpenmaxAOType)
    	{
            return OMX_ErrorBadParameter;
    	}
       switch (nIndex) {
	    case OMX_IndexConfigCommonOutputCrop:
	    {
                int32_t disp_left,disp_top,disp_width, disp_height; 
                OMX_CONFIG_RECTTYPE *rectParams = (OMX_CONFIG_RECTTYPE *)pComponentConfigStructure;

                if (rectParams->nPortIndex != OMX_PORT_OUTPUTPORT_INDEX) {
                    return OMX_ErrorUndefined;
                }

                if( !ipMpegDecoderObject->getDisplayRect(&disp_left, &disp_top,&disp_width, &disp_height)){
                    return OMX_ErrorUndefined;
                }
                rectParams->nLeft = disp_left;
                rectParams->nTop = disp_top;
                rectParams->nWidth = disp_width;
                rectParams->nHeight = disp_height;

                OMX_MP4DEC_INFO ("OpenmaxMpeg4AO OMX_IndexConfigCommonOutputCrop %d,%d,%d,%d",
                                rectParams->nLeft,rectParams->nTop,rectParams->nWidth,rectParams->nHeight); 
	    }	
			break;
	    default:
                ret = OmxComponentVideo::GetConfig(hComponent,nIndex,pComponentConfigStructure);
		break;
       }
	
	return ret;
}

OMX_ERRORTYPE OpenmaxMpeg4AO::ReAllocatePartialAssemblyBuffers(OMX_BUFFERHEADERTYPE* aInputBufferHdr)
{
    // check if there is enough data in the buffer to read the information that we need
    if (aInputBufferHdr->nFilledLen >= MINIMUM_H263_SHORT_HEADER_SIZE)
    {
        OMX_U8 *pInputBuffer = (aInputBufferHdr->pBuffer + aInputBufferHdr->nOffset);

        if (MODE_H263 == iDecMode)
        {
            OMX_BOOL Status = OMX_TRUE;

            Status = DecodeH263Header(pInputBuffer, &iInputCurrBufferSize);

            // Re-allocate the partial frame buffer in case the stream is not corrupted,
            // otherwise leave the buffer size as it is
            if (OMX_TRUE == Status)
            {
                if (NULL != ipInputCurrBuffer)
                {
                    ipInputCurrBuffer = (OMX_U8*) oscl_realloc(ipInputCurrBuffer, iInputCurrBufferSize);
                    if (NULL == ipInputCurrBuffer)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentBase : DoStateSet error insufficient resources"));
                        return OMX_ErrorInsufficientResources;
                    }
                }

                //Used when the buffers are not marked with EndOfFrame flag
                if (NULL != ipTempInputBuffer)
                {
                    ipTempInputBuffer = (OMX_U8*) oscl_realloc(ipTempInputBuffer, iInputCurrBufferSize);
                    if (NULL == ipTempInputBuffer)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentBase : DoStateSet error insufficient resources"));
                        return OMX_ErrorInsufficientResources;
                    }
                }
            }
        }

        return OMX_ErrorNone;
    } else
    {
        return OMX_ErrorInsufficientResources;
    }
}

OMX_BOOL OpenmaxMpeg4AO::DecodeH263Header(OMX_U8* aInputBuffer,
        OMX_U32* aBufferSize)
{
    uint32 codeword;
    int32   extended_PTYPE = 0;
    int32 UFEP = 0;
    int32 custom_PFMT = 0;

    //Reset the data bit position to the start of the stream
    iH263DataBitPos = 0;
    iH263BitPos = 0;
    //BitBuf contains the first 4 bytes of the aInputBuffer
    iH263BitBuf = (aInputBuffer[0] << 24) | (aInputBuffer[1] << 16) | (aInputBuffer[2] << 8) | aInputBuffer[3];

    ReadBits(aInputBuffer, 22, &codeword);
    if (codeword !=  0x20)
    {
        return OMX_FALSE;
    }

    ReadBits(aInputBuffer, 8, &codeword);

    ReadBits(aInputBuffer, 1, &codeword);
    if (codeword == 0) return OMX_FALSE;

    ReadBits(aInputBuffer, 1, &codeword);
    if (codeword == 1) return OMX_FALSE;

    ReadBits(aInputBuffer, 1, &codeword);
    if (codeword == 1) return OMX_FALSE;

    ReadBits(aInputBuffer, 1, &codeword);
    if (codeword == 1) return OMX_FALSE;

    ReadBits(aInputBuffer, 1, &codeword);
    if (codeword == 1) return OMX_FALSE;

    /* source format */
    ReadBits(aInputBuffer, 3, &codeword);
    switch (codeword)
    {
        case 1:
            *aBufferSize = 32000;
            break;

        case 2:
            *aBufferSize = 32000;
            break;

        case 3:
            *aBufferSize = 128000;
            break;

        case 4:
            *aBufferSize = 256000;
            break;

        case 5:
            *aBufferSize = 512000;
            break;

        case 7:
            extended_PTYPE = 1;
            break;
        default:
            /* Msg("H.263 source format not legal\n"); */
            return OMX_FALSE;
    }

    if (extended_PTYPE == 0)
    {
        return OMX_TRUE;
    }

    /* source format */
    ReadBits(aInputBuffer, 3, &codeword);
    UFEP = codeword;
    if (UFEP == 1)
    {
        ReadBits(aInputBuffer, 3, &codeword);
        switch (codeword)
        {
            case 1:
                *aBufferSize = 32000;
                break;

            case 2:
                *aBufferSize = 32000;
                break;

            case 3:
                *aBufferSize = 128000;
                break;

            case 4:
                *aBufferSize = 256000;
                break;

            case 5:
                *aBufferSize = 512000;
                break;

            case 6:
                custom_PFMT = 1;
                break;
            default:
                /* Msg("H.263 source format not legal\n"); */
                return OMX_FALSE;
        }
        if (custom_PFMT == 0)
        {
            return OMX_TRUE;
        }


        ReadBits(aInputBuffer, 1, &codeword);
        ReadBits(aInputBuffer, 1, &codeword);
        if (codeword) return OMX_FALSE;
        ReadBits(aInputBuffer, 1, &codeword);
        if (codeword) return OMX_FALSE;
        ReadBits(aInputBuffer, 1, &codeword);
        if (codeword) return OMX_FALSE;
        ReadBits(aInputBuffer, 3, &codeword);
        ReadBits(aInputBuffer, 3, &codeword);
        if (codeword) return OMX_FALSE;             /* RPS, ISD, AIV */
        ReadBits(aInputBuffer, 1, &codeword);
        ReadBits(aInputBuffer, 4, &codeword);
        if (codeword != 8) return OMX_FALSE;
    }

    if (UFEP == 0 || UFEP == 1)
    {
        ReadBits(aInputBuffer, 3, &codeword);
        if (codeword > 1) return OMX_FALSE;
        ReadBits(aInputBuffer, 1, &codeword);
        if (codeword) return OMX_FALSE;
        ReadBits(aInputBuffer, 1, &codeword);
        if (codeword) return OMX_FALSE;
        ReadBits(aInputBuffer, 1, &codeword);
        ReadBits(aInputBuffer, 3, &codeword);
        if (codeword != 1) return OMX_FALSE;
    }
    else
    {
        return OMX_FALSE;
    }

    ReadBits(aInputBuffer, 1, &codeword);
    if (codeword) return OMX_FALSE; /* CPM */
    if (custom_PFMT == 1 && UFEP == 1)
    {
        OMX_U32 DisplayWidth, Width, DisplayHeight, Height, Resolution;

        ReadBits(aInputBuffer, 4, &codeword);
        if (codeword == 0) return OMX_FALSE;
        if (codeword == 0xf)
        {
            ReadBits(aInputBuffer, 8, &codeword);
            ReadBits(aInputBuffer, 8, &codeword);
        }
        ReadBits(aInputBuffer, 9, &codeword);
        DisplayWidth = (codeword + 1) << 2;
        Width = (DisplayWidth + 15) & -16;

        ReadBits(aInputBuffer, 1, &codeword);
        if (codeword != 1) return OMX_FALSE;
        ReadBits(aInputBuffer, 9, &codeword);
        if (codeword == 0) return OMX_FALSE;
        DisplayHeight = codeword << 2;
        Height = (DisplayHeight + 15) & -16;

        Resolution = Width * Height;

        if (Resolution <= 25344)        //25344 = 176x144 (QCIF)
        {
            *aBufferSize = 32000;
        }
        else if (Resolution <= 101376)  //101376 = 352x288 (CIF)
        {
            *aBufferSize = 128000;
        }
        else if (Resolution <= 405504)  //405504 = 704*576 (4CIF)
        {
            *aBufferSize = 256000;
        }
        else                            //1408x1152 (16CIF)
        {
            //This is the max buffer size that we want to allocate
            *aBufferSize = 512000;
        }
    }

    return OMX_TRUE;
}

void OpenmaxMpeg4AO::ReadBits(OMX_U8* aStream,           /* Input Stream */
                              uint8 aNumBits,                     /* nr of bits to read */
                              uint32* aOutData                 /* output target */
                             )
{
    uint8 *bits;
    uint32 dataBitPos = iH263DataBitPos;
    uint32 bitPos = iH263BitPos;
    uint32 dataBytePos;

    if (aNumBits > (32 - bitPos))    /* not enough bits */
    {
        dataBytePos = dataBitPos >> 3;    /* Byte Aligned Position */
        bitPos = dataBitPos & 7; /* update bit position */
        bits = &aStream[dataBytePos];
        iH263BitBuf = (bits[0] << 24) | (bits[1] << 16) | (bits[2] << 8) | bits[3];
    }

    iH263DataBitPos += aNumBits;
    iH263BitPos      = (unsigned char)(bitPos + aNumBits);

    *aOutData = (iH263BitBuf >> (32 - iH263BitPos)) & mask[(uint16)aNumBits];

    return;
}


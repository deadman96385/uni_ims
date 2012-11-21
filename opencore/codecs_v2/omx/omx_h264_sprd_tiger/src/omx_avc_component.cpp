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
#include "oscl_base.h"
#include "OMX_Types.h"
#include "pv_omxdefs.h"
#include "omx_avc_component.h"

#if PROXY_INTERFACE
#include "omx_proxy_interface.h"
#endif

#include <media/hardware/HardwareAPI.h>
//#include <ui/android_native_buffer.h>
#include "gralloc_priv.h"

// Use default DLL entry point
#ifndef OSCL_DLL_H_INCLUDED
#include "oscl_dll.h"
#endif


OSCL_DLL_ENTRY_POINT_DEFAULT()

// This function is called by OMX_GetHandle and it creates an instance of the avc component AO
OSCL_EXPORT_REF OMX_ERRORTYPE AvcOmxComponentFactory(OMX_OUT OMX_HANDLETYPE* pHandle, OMX_IN  OMX_PTR pAppData, OMX_IN OMX_PTR pProxy , OMX_STRING aOmxLibName, OMX_PTR &aOmxLib, OMX_PTR aOsclUuid, OMX_U32 &aRefCount)
{
    OSCL_UNUSED_ARG(aOmxLibName);
    OSCL_UNUSED_ARG(aOmxLib);
    OSCL_UNUSED_ARG(aOsclUuid);
    OSCL_UNUSED_ARG(aRefCount);

    OpenmaxAvcAO* pOpenmaxAOType;
    OMX_ERRORTYPE Status;

    // move InitAvcOmxComponentFields content to actual constructor

    pOpenmaxAOType = (OpenmaxAvcAO*) OSCL_NEW(OpenmaxAvcAO, ());

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorInsufficientResources;
    }

    //Call the construct component to initialize OMX types
    Status = pOpenmaxAOType->ConstructComponent(pAppData, pProxy);

    *pHandle = pOpenmaxAOType->GetOmxHandle();

    return Status;
    ///////////////////////////////////////////////////////////////////////////////////////
}

// This function is called by OMX_FreeHandle when component AO needs to be destroyed
OSCL_EXPORT_REF OMX_ERRORTYPE AvcOmxComponentDestructor(OMX_IN OMX_HANDLETYPE pHandle, OMX_PTR &aOmxLib, OMX_PTR aOsclUuid, OMX_U32 &aRefCount)
{
    OSCL_UNUSED_ARG(aOmxLib);
    OSCL_UNUSED_ARG(aOsclUuid);
    OSCL_UNUSED_ARG(aRefCount);

    // get pointer to component AO
    OpenmaxAvcAO* pOpenmaxAOType = (OpenmaxAvcAO*)((OMX_COMPONENTTYPE*)pHandle)->pComponentPrivate;

    // clean up decoder, OMX component stuff
    pOpenmaxAOType->DestroyComponent();

    // destroy the AO class
    OSCL_DELETE(pOpenmaxAOType);

    return OMX_ErrorNone;
}

#if DYNAMIC_LOAD_OMX_AVC_COMPONENT
class OsclSharedLibraryInterface;
class AvcOmxSharedLibraryInterface: public OsclSharedLibraryInterface,
        public OmxSharedLibraryInterface

{
    public:
        OsclAny *QueryOmxComponentInterface(const OsclUuid& aOmxTypeId, const OsclUuid& aInterfaceId)
        {
            if (PV_OMX_AVCDEC_UUID == aOmxTypeId)
            {
                if (PV_OMX_CREATE_INTERFACE == aInterfaceId)
                {
                    return ((OsclAny*)(&AvcOmxComponentFactory));
                }
                else if (PV_OMX_DESTROY_INTERFACE == aInterfaceId)
                {
                    return ((OsclAny*)(&AvcOmxComponentDestructor));
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

        AvcOmxSharedLibraryInterface() {};
};

// function to obtain the interface object from the shared library
extern "C"
{
    OSCL_EXPORT_REF OsclAny* PVGetInterface()
    {
        return (OsclAny*) OSCL_NEW(AvcOmxSharedLibraryInterface, ());
    }

    OSCL_EXPORT_REF void PVReleaseInterface(OsclSharedLibraryInterface* aInstance)
    {
        AvcOmxSharedLibraryInterface* module = (AvcOmxSharedLibraryInterface*)aInstance;
        OSCL_DELETE(module);
    }
}

#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//modified by jgdu
OSCL_EXPORT_REF OMX_ERRORTYPE OpenmaxAvcAO::OpenmaxAvcAOAllocateBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE** pBuffer,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes)
{
    OpenmaxAvcAO* pOpenmaxAOType = (OpenmaxAvcAO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    OMX_ERRORTYPE Status;

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorBadParameter;
    }

   Status = pOpenmaxAOType->AllocateBuffer(hComponent, pBuffer, nPortIndex, pAppPrivate, nSizeBytes);

    return Status;
}

OMX_ERRORTYPE OpenmaxAvcAO::AllocateBuffer(
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
#if 0 
			iMemHeapBae =  new MemoryHeapBase(PMEM_DRIVER,nSizeBytes_64word*pBaseComponentPort->PortParam.nBufferCountActual,MemoryHeapBase::NO_CACHING);
	 		iMasterHeap = iMemHeapBae;
			iPmemHeap = new MemoryHeapPmem(iMasterHeap,MemoryHeapBase::NO_CACHING);
			int fd =iPmemHeap->getHeapID();
			if(fd>=0){
				iPmemHeap->slap();
	                	struct pmem_region region;
	                	::ioctl(fd,PMEM_GET_PHYS,&region);	
				iPmemBasePhy = region.offset;
				iPmemBase  =(uint32) iPmemHeap->base();
				SCI_TRACE_LOW("OpenmaxAvcAO::AllocateBuffer pmemmpool num = %d,size = %d:%x,%x,%x,%x\n",pBaseComponentPort->PortParam.nBufferCountActual,nSizeBytes_64word,iPmemHeap->getHeapID(),iPmemBase,region.offset,region.len);				
			}
#else
	iPmemHeap = new MemoryHeapIon(SPRD_ION_DEV, nSizeBytes_64word*pBaseComponentPort->PortParam.nBufferCountActual, MemoryHeapBase::NO_CACHING, ION_HEAP_CARVEOUT_MASK);

 	 int fd = iPmemHeap->getHeapID();
	 if(fd>=0){
	 	int ret,phy_addr, buffer_size;
	 	ret = iPmemHeap->get_phy_addr_from_ion(&phy_addr, &buffer_size);
	 	if(ret) SCI_TRACE_LOW("iDecExtPmemHeap get_phy_addr_from_ion fail %d",ret);

		iPmemBasePhy =(OMX_U32)phy_addr;
		SCI_TRACE_LOW("AvcDecoder_OMX ext mem pmempool %x,%x,%x,%x\n",iPmemHeap->getHeapID(),iPmemHeap->base(),phy_addr,buffer_size);
	 	iPmemBase = (OMX_U32)iPmemHeap->base();
		SCI_TRACE_LOW("OpenmaxAvcAO::AllocateBuffer pmemmpool num = %d,size = %d:%x,%x,%x,%x\n",pBaseComponentPort->PortParam.nBufferCountActual,nSizeBytes_64word,iPmemHeap->getHeapID(),iPmemBase,phy_addr,buffer_size);	
	 	}
#endif
			else
			{
				SCI_TRACE_LOW("OpenmaxAvcAO::AllocateBuffer pmempool error %d %d\n",nSizeBytes_64word,pBaseComponentPort->PortParam.nBufferCountActual);
				return OMX_ErrorInsufficientResources;
			}
	    	}
			
	        if(iNumberOfPmemBuffers>=pBaseComponentPort->PortParam.nBufferCountActual)
	        {       
	        	SCI_TRACE_LOW("OpenmaxAvcAO::AllocateBuffer iNumberOfPmemHeaps>=nBufferCountActual %d %d\n",iNumberOfPmemBuffers,pBaseComponentPort->PortParam.nBufferCountActual);
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
		 SCI_TRACE_LOW("pInfo->offset %x",pInfo->offset);
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
OSCL_EXPORT_REF OMX_ERRORTYPE OpenmaxAvcAO::OpenmaxAvcAOFreeBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_U32 nPortIndex,
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    OpenmaxAvcAO* pOpenmaxAOType = (OpenmaxAvcAO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    OMX_ERRORTYPE Status;

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorBadParameter;
    }

    Status = pOpenmaxAOType->FreeBuffer(hComponent, nPortIndex, pBuffer);

    return Status;
}
OMX_ERRORTYPE OpenmaxAvcAO::FreeBuffer(
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
			//if(iMasterHeap!=NULL)
			//	iMasterHeap.clear();
			//if(iPmemHeap!=NULL)
			//	iPmemHeap.clear();	
			
			if(iPmemHeap!=NULL)
				iPmemHeap.clear();
			SCI_TRACE_LOW("OpenmaxAvcAO::FreeBuffer pmempool clear\n");
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
            }
            else if (OMX_PORT_OUTPUTPORT_INDEX == nPortIndex)
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

OSCL_EXPORT_REF OMX_ERRORTYPE OpenmaxAvcAO::OpenmaxAvcAOUseBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes,
    OMX_IN OMX_U8* pBuffer)
{
    OpenmaxAvcAO* pOpenmaxAOType = (OpenmaxAvcAO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    OMX_ERRORTYPE Status;

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorBadParameter;
    }

    Status = pOpenmaxAOType->UseBuffer(hComponent, ppBufferHdr, nPortIndex, pAppPrivate, nSizeBytes, pBuffer);

    return Status;
}

OMX_ERRORTYPE OpenmaxAvcAO::UseBuffer(
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
SCI_TRACE_LOW("UseBuffer *ppBufferHdr %x,pBuffer %x",*ppBufferHdr,pBuffer);
            if (OMX_DirInput == pBaseComponentPort->PortParam.eDir)
            {
                pBaseComponentPort->pBuffer[ii]->nInputPortIndex = nPortIndex;
                pBaseComponentPort->pBuffer[ii]->nOutputPortIndex = iNumPorts; // here is assigned a non-valid port index
            }
            else
            {
                if(!iUseAndroidNativeBuffer[OMX_PORT_OUTPUTPORT_INDEX])
            	  {
            	      SCI_TRACE_LOW("OpenmaxAvcAO call use buffer on output port when iUseAndroidNativeBuffer disenabled");
                    return OMX_ErrorInsufficientResources;            	  	
            	  }else
            	  {
            	  SCI_TRACE_LOW("OMX_DirOutput iUseAndroidNativeBuffer");
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
			native_handle_t *pNativeHandle = (native_handle_t *)pBaseComponentPort->pBuffer[ii]->pBuffer;

#if 0
			int *pInt = (int *)pNativeHandle;
		 	pInfo->pmem_fd = (uint32)pInt[3];

			int32_t phy_base_addr;
	 		struct pmem_region region;
	 		if(::ioctl(pInfo->pmem_fd,PMEM_GET_PHYS,&region)<0){
				SCI_TRACE_LOW("get phy_base_addr failed!");
                    		return OMX_ErrorInsufficientResources;				
	 		}else{
	 			phy_base_addr = region.offset;	
                     	SCI_TRACE_LOW("phy_base_addr %x",phy_base_addr);
	 		}	  
			
		 	pInfo->offset =  (uint32)(pInt[13]+phy_base_addr);
#else
			struct private_handle_t *private_h = (struct private_handle_t *)pNativeHandle;
                   pInfo->offset =  (uint32)(private_h->phyaddr);
#endif
SCI_TRACE_LOW("pInfo->offset(phy_addr) %x ,private_h->base %x",pInfo->offset,private_h->base );
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

OSCL_EXPORT_REF OMX_ERRORTYPE OpenmaxAvcAO::OpenmaxAvcAOGetExtensionIndex(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_STRING cParameterName,
    OMX_OUT OMX_INDEXTYPE* pIndexType)
{
    OpenmaxAvcAO* pOpenmaxAOType = (OpenmaxAvcAO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorBadParameter;
    }
	
    if(strcmp(cParameterName, SPRD_INDEX_PARAM_ENABLE_ANB) == 0)
    {
    		SCI_TRACE_LOW("OpenmaxMpeg4AO::%s",SPRD_INDEX_PARAM_ENABLE_ANB);
		*pIndexType = (OMX_INDEXTYPE) OMX_IndexParamEnableAndroidBuffers;
		return OMX_ErrorNone;
    }else if (strcmp(cParameterName, SPRD_INDEX_PARAM_GET_ANB) == 0)
    {
     		SCI_TRACE_LOW("OpenmaxMpeg4AO::%s",SPRD_INDEX_PARAM_GET_ANB);   
		*pIndexType = (OMX_INDEXTYPE) OMX_IndexParamGetAndroidNativeBuffer;
		return OMX_ErrorNone;
    }	else if (strcmp(cParameterName, SPRD_INDEX_PARAM_USE_ANB) == 0)
    {
     		SCI_TRACE_LOW("OpenmaxMpeg4AO::%s",SPRD_INDEX_PARAM_USE_ANB);     
		*pIndexType = OMX_IndexParamUseAndroidNativeBuffer2;
		return OMX_ErrorNone;
    }
	
    return OMX_ErrorNotImplemented;
}

OSCL_EXPORT_REF OMX_ERRORTYPE OpenmaxAvcAO::GetParameter(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nParamIndex,
    OMX_INOUT OMX_PTR ComponentParameterStructure)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
       OpenmaxAvcAO* pOpenmaxAOType = (OpenmaxAvcAO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
	//SCI_TRACE_LOW("OpenmaxMpeg4AO::GetParameter");	   
    	if (NULL == pOpenmaxAOType)
    	{
            return OMX_ErrorBadParameter;
    	}
       switch (nParamIndex) {
	    case OMX_IndexParamEnableAndroidBuffers:
	    {
			EnableAndroidNativeBuffersParams *peanbp = (EnableAndroidNativeBuffersParams *)ComponentParameterStructure;			
			peanbp->enable = iUseAndroidNativeBuffer[OMX_PORT_OUTPUTPORT_INDEX];
			SCI_TRACE_LOW("OpenmaxMpeg4AO::GetParameter OMX_IndexParamEnableAndroidBuffers %d",peanbp->enable);			
	    }
			break;
	    case OMX_IndexParamGetAndroidNativeBuffer:
	    {
			GetAndroidNativeBufferUsageParams *pganbp;

    			pganbp = (GetAndroidNativeBufferUsageParams *)ComponentParameterStructure;

    			pganbp->nUsage = GRALLOC_USAGE_PRIVATE_0;//GRALLOC_USAGE_HW_VIDEO_ENCODER;
		       SCI_TRACE_LOW("OpenmaxMpeg4AO::GetParameter OMX_IndexParamGetAndroidNativeBuffer %x",pganbp->nUsage);	
	    }		
			break;
	    default:
			ret = OmxComponentVideo::GetParameter(hComponent,nParamIndex,ComponentParameterStructure);
			break;
       }
	
	return ret;
}

OSCL_EXPORT_REF OMX_ERRORTYPE OpenmaxAvcAO::SetParameter(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nParamIndex,
    OMX_IN  OMX_PTR ComponentParameterStructure)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
       OpenmaxAvcAO* pOpenmaxAOType = (OpenmaxAvcAO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
	//SCI_TRACE_LOW("OpenmaxMpeg4AO::SetParameter");	
    	if (NULL == pOpenmaxAOType)
    	{
            return OMX_ErrorBadParameter;
    	}	
	switch (nParamIndex) {
	    case OMX_IndexParamEnableAndroidBuffers:
	    {
			EnableAndroidNativeBuffersParams *peanbp = (EnableAndroidNativeBuffersParams *)ComponentParameterStructure;
			ComponentPortType *pOutPort = (ComponentPortType*) ipPorts[OMX_PORT_OUTPUTPORT_INDEX];
			if (peanbp->enable == OMX_FALSE) {
        			SCI_TRACE_LOW("OpenmaxMpeg4AO::disable AndroidNativeBuffer");
        			iUseAndroidNativeBuffer[OMX_PORT_OUTPUTPORT_INDEX] = OMX_FALSE;
				pOutPort->VideoParam[0].eColorFormat = (OMX_COLOR_FORMATTYPE)0x7FA30C00; //modified by jgdu	
    				pOutPort->PortParam.format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)0x7FA30C00;				
    			} else {
        			SCI_TRACE_LOW("OpenmaxMpeg4AO::enable AndroidNativeBuffer");
        			iUseAndroidNativeBuffer[OMX_PORT_OUTPUTPORT_INDEX] = OMX_TRUE;
				pOutPort->VideoParam[0].eColorFormat = (OMX_COLOR_FORMATTYPE)HAL_PIXEL_FORMAT_YCbCr_420_SP;
    				pOutPort->PortParam.format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)HAL_PIXEL_FORMAT_YCbCr_420_SP;
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

 uint32 OpenmaxAvcAO:: FindPhyAddr(uint32 Vaddr)
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
	//SCI_TRACE_LOW("find phy addr 0x%x",phy);
	return phy;
	//return Vaddr - iPmemBase + iPmemBasePhy;
 }
 
OMX_ERRORTYPE OpenmaxAvcAO::ConstructComponent(OMX_PTR pAppData, OMX_PTR pProxy)
{
    ComponentPortType* pInPort, *pOutPort;
    OMX_ERRORTYPE Status;

    iNumPorts = 2;
    iOmxComponent.nSize = sizeof(OMX_COMPONENTTYPE);
    iOmxComponent.pComponentPrivate = (OMX_PTR) this;  // pComponentPrivate points to THIS component AO class
    ipComponentProxy = pProxy;
    iOmxComponent.pApplicationPrivate = pAppData; // init the App data

    //modified by jgdu
    iNumberOfPmemBuffers = 0;
    iLogCount = 0;	
    iUseAndroidNativeBuffer[OMX_PORT_INPUTPORT_INDEX] = OMX_FALSE;
    iUseAndroidNativeBuffer[OMX_PORT_OUTPUTPORT_INDEX] = OMX_FALSE;
	
    ipOutputBufferForRendering = NULL;
    iNumNALs = 0;
    iCurrNAL = 0;
    iNALOffset = 0;
    oscl_memset(iNALSizeArray, 0, MAX_NAL_PER_FRAME * sizeof(uint32));
    oscl_memset(iNALStartCodeSizeArray, 0, MAX_NAL_PER_FRAME * sizeof(uint32));


#if PROXY_INTERFACE
    iPVCapabilityFlags.iIsOMXComponentMultiThreaded = OMX_TRUE;

    iOmxComponent.SendCommand = OpenmaxAvcAO::BaseComponentProxySendCommand;
    iOmxComponent.GetParameter = OpenmaxAvcAO::BaseComponentProxyGetParameter;
    iOmxComponent.SetParameter = OpenmaxAvcAO::BaseComponentProxySetParameter;
    iOmxComponent.GetConfig = OpenmaxAvcAO::BaseComponentProxyGetConfig;
    iOmxComponent.SetConfig = OpenmaxAvcAO::BaseComponentProxySetConfig;
    iOmxComponent.GetExtensionIndex = OpenmaxAvcAO::BaseComponentProxyGetExtensionIndex;
    iOmxComponent.GetState = OpenmaxAvcAO::BaseComponentProxyGetState;
    iOmxComponent.UseBuffer = OpenmaxAvcAO::BaseComponentProxyUseBuffer;
    iOmxComponent.AllocateBuffer = OpenmaxAvcAO::BaseComponentProxyAllocateBuffer;
    iOmxComponent.FreeBuffer = OpenmaxAvcAO::BaseComponentProxyFreeBuffer;
    iOmxComponent.EmptyThisBuffer = OpenmaxAvcAO::BaseComponentProxyEmptyThisBuffer;
    iOmxComponent.FillThisBuffer = OpenmaxAvcAO::BaseComponentProxyFillThisBuffer;

#else
    iPVCapabilityFlags.iIsOMXComponentMultiThreaded = OMX_FALSE;

    iOmxComponent.SendCommand = OpenmaxAvcAO::BaseComponentSendCommand;
    iOmxComponent.GetParameter = OpenmaxAvcAO::BaseComponentGetParameter;
    iOmxComponent.SetParameter = OpenmaxAvcAO::BaseComponentSetParameter;
    iOmxComponent.GetConfig = OpenmaxAvcAO::BaseComponentGetConfig;
    iOmxComponent.SetConfig = OpenmaxAvcAO::BaseComponentSetConfig;
    iOmxComponent.GetExtensionIndex = OpenmaxAvcAO::BaseComponentGetExtensionIndex;
    iOmxComponent.GetState = OpenmaxAvcAO::BaseComponentGetState;
    iOmxComponent.UseBuffer = OpenmaxAvcAO::BaseComponentUseBuffer;
    iOmxComponent.AllocateBuffer = OpenmaxAvcAO::BaseComponentAllocateBuffer;
    iOmxComponent.FreeBuffer = OpenmaxAvcAO::BaseComponentFreeBuffer;
    iOmxComponent.EmptyThisBuffer = OpenmaxAvcAO::BaseComponentEmptyThisBuffer;
    iOmxComponent.FillThisBuffer = OpenmaxAvcAO::BaseComponentFillThisBuffer;
#endif

    iOmxComponent.SetCallbacks = OpenmaxAvcAO::BaseComponentSetCallbacks;
    iOmxComponent.nVersion.s.nVersionMajor = SPECVERSIONMAJOR;
    iOmxComponent.nVersion.s.nVersionMinor = SPECVERSIONMINOR;
    iOmxComponent.nVersion.s.nRevision = SPECREVISION;
    iOmxComponent.nVersion.s.nStep = SPECSTEP;

    // PV capability
#if defined(TEST_FULL_AVC_FRAME_MODE)
    iPVCapabilityFlags.iOMXComponentSupportsExternalInputBufferAlloc = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentSupportsExternalOutputBufferAlloc = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentSupportsMovableInputBuffers = OMX_FALSE; // since we need copying for frame assembly in streaming case
    iPVCapabilityFlags.iOMXComponentSupportsPartialFrames = OMX_FALSE;
    iPVCapabilityFlags.iOMXComponentUsesNALStartCodes = OMX_FALSE;
    iPVCapabilityFlags.iOMXComponentCanHandleIncompleteFrames = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentUsesFullAVCFrames = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentUsesInterleaved2BNALSizes = OMX_FALSE;
    iPVCapabilityFlags.iOMXComponentUsesInterleaved4BNALSizes = OMX_FALSE;
#elif defined(TEST_FULL_AVC_FRAME_MODE_SC)
    iPVCapabilityFlags.iOMXComponentSupportsExternalInputBufferAlloc = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentSupportsExternalOutputBufferAlloc = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentSupportsMovableInputBuffers = OMX_FALSE; // since we need copying for frame assembly in streaming case
    iPVCapabilityFlags.iOMXComponentSupportsPartialFrames = OMX_FALSE;
    iPVCapabilityFlags.iOMXComponentUsesNALStartCodes = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentCanHandleIncompleteFrames = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentUsesFullAVCFrames = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentUsesInterleaved2BNALSizes = OMX_FALSE;
    iPVCapabilityFlags.iOMXComponentUsesInterleaved4BNALSizes = OMX_FALSE;
#elif defined(TEST_FULL_AVC_FRAME_MODE_I2BNS)
    iPVCapabilityFlags.iOMXComponentSupportsExternalInputBufferAlloc = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentSupportsExternalOutputBufferAlloc = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentSupportsMovableInputBuffers = OMX_FALSE; // since we need copying for frame assembly in streaming case
    iPVCapabilityFlags.iOMXComponentSupportsPartialFrames = OMX_FALSE;
    iPVCapabilityFlags.iOMXComponentUsesNALStartCodes = OMX_FALSE;
    iPVCapabilityFlags.iOMXComponentCanHandleIncompleteFrames = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentUsesFullAVCFrames = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentUsesInterleaved2BNALSizes = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentUsesInterleaved4BNALSizes = OMX_FALSE;
#elif defined(TEST_FULL_AVC_FRAME_MODE_I4BNS)
    iPVCapabilityFlags.iOMXComponentSupportsExternalInputBufferAlloc = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentSupportsExternalOutputBufferAlloc = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentSupportsMovableInputBuffers = OMX_FALSE; // since we need copying for frame assembly in streaming case
    iPVCapabilityFlags.iOMXComponentSupportsPartialFrames = OMX_FALSE;
    iPVCapabilityFlags.iOMXComponentUsesNALStartCodes = OMX_FALSE;
    iPVCapabilityFlags.iOMXComponentCanHandleIncompleteFrames = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentUsesFullAVCFrames = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentUsesInterleaved2BNALSizes = OMX_FALSE;
    iPVCapabilityFlags.iOMXComponentUsesInterleaved4BNALSizes = OMX_TRUE;
#elif defined(TEST_AVC_NAL_MODE_SC)
    iPVCapabilityFlags.iOMXComponentSupportsExternalInputBufferAlloc = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentSupportsExternalOutputBufferAlloc = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentSupportsMovableInputBuffers = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentSupportsPartialFrames = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentUsesNALStartCodes = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentCanHandleIncompleteFrames = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentUsesFullAVCFrames = OMX_FALSE;
    iPVCapabilityFlags.iOMXComponentUsesInterleaved2BNALSizes = OMX_FALSE;
    iPVCapabilityFlags.iOMXComponentUsesInterleaved4BNALSizes = OMX_FALSE;
#else
    iPVCapabilityFlags.iOMXComponentSupportsExternalInputBufferAlloc = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentSupportsExternalOutputBufferAlloc  = OMX_FALSE;//modified by jgdu
    iPVCapabilityFlags.iOMXComponentSupportsMovableInputBuffers = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentSupportsPartialFrames = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentUsesNALStartCodes = OMX_FALSE;
    iPVCapabilityFlags.iOMXComponentCanHandleIncompleteFrames = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentUsesFullAVCFrames = OMX_FALSE;
    iPVCapabilityFlags.iOMXComponentUsesInterleaved2BNALSizes = OMX_FALSE;
    iPVCapabilityFlags.iOMXComponentUsesInterleaved4BNALSizes = OMX_FALSE;
#endif

    if (iPVCapabilityFlags.iOMXComponentUsesInterleaved2BNALSizes)
    {
        iSizeOfNALSize = sizeof(OMX_U16);
    }
    else if (iPVCapabilityFlags.iOMXComponentUsesInterleaved4BNALSizes)
    {
        iSizeOfNALSize = sizeof(OMX_U32);
    }
    else
    {
        iSizeOfNALSize = 0;
    }


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
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.cMIMEType = (OMX_STRING)"video/Avc";
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.pNativeRender = 0;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.bFlagErrorConcealment = OMX_FALSE;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.nFrameWidth = 176;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.nFrameHeight = 144;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.nBitrate = 64000;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.xFramerate = (15 << 16);
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.eDir = OMX_DirInput;
    //Set to a default value, will change later during setparameter call
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.nBufferCountActual = NUMBER_INPUT_BUFFER_AVC;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.nBufferCountMin = 1;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.nBufferSize = INPUT_BUFFER_SIZE_AVC;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.bEnabled = OMX_TRUE;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.bPopulated = OMX_FALSE;

    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.nPortIndex = OMX_PORT_OUTPUTPORT_INDEX;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.eDomain = OMX_PortDomainVideo;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.cMIMEType = (OMX_STRING)"raw";
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.pNativeRender = 0;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.bFlagErrorConcealment = OMX_FALSE;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
   // ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.eColorFormat = OMX_COLOR_FormatYUV420Planar;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)0x7FA30C00; //modified by jgdu
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.nFrameWidth = 176;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.nStride = 176;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.nFrameHeight = 144;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.nSliceHeight = 144;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.nBitrate = 64000;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.xFramerate = (15 << 16);
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.eDir = OMX_DirOutput;
    //Set to a default value, will change later during setparameter call
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.nBufferCountActual = NUMBER_OUTPUT_BUFFER_AVC;

    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.nBufferCountMin = NUMBER_OUTPUT_BUFFER_AVC;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.nBufferSize = OUTPUT_BUFFER_SIZE_AVC; //just use QCIF (as default)
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.bEnabled = OMX_TRUE;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.bPopulated = OMX_FALSE;

    //Default values for Avc video param port
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->VideoAvc.nPortIndex = OMX_PORT_INPUTPORT_INDEX;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->VideoAvc.eProfile = OMX_VIDEO_AVCProfileBaseline;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->VideoAvc.eLevel = OMX_VIDEO_AVCLevel1;

    ipPorts[OMX_PORT_INPUTPORT_INDEX]->ProfileLevel.nPortIndex = OMX_PORT_INPUTPORT_INDEX;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->ProfileLevel.nProfileIndex = 0;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->ProfileLevel.eProfile = OMX_VIDEO_AVCProfileBaseline;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->ProfileLevel.eLevel = OMX_VIDEO_AVCLevel1;

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
    pInPort->VideoParam[0].eCompressionFormat = OMX_VIDEO_CodingAVC;
    pInPort->VideoParam[0].eColorFormat = OMX_COLOR_FormatUnused;

    pOutPort->ActualNumPortFormatsSupported = 1;

    //OMX_VIDEO_PARAM_PORTFORMATTYPE OUTPUT PORT SETTINGS
    //On output port for index 0
    SetHeader(&pOutPort->VideoParam[0], sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
    pOutPort->VideoParam[0].nPortIndex = 1;
    pOutPort->VideoParam[0].nIndex = 0;
    pOutPort->VideoParam[0].eCompressionFormat = OMX_VIDEO_CodingUnused;
    //pOutPort->VideoParam[0].eColorFormat = OMX_COLOR_FormatYUV420Planar;
    pOutPort->VideoParam[0].eColorFormat = (OMX_COLOR_FORMATTYPE)0x7FA30C00; //modified by jgdu 

    oscl_strncpy((OMX_STRING)iComponentRole, (OMX_STRING)"video_decoder.avc", OMX_MAX_STRINGNAME_SIZE);

    iDecodeReturn = OMX_FALSE;
    iFlushOutputStatus = OMX_TRUE;

    if (ipAvcDec)
    {
        OSCL_DELETE(ipAvcDec);
        ipAvcDec = NULL;
    }

    if(AvcDecoder_OMX::g_h264_dec_inst_num>=1)
    {
 	SCI_TRACE_LOW("AvcDecoder_OMX more than 1 inst\n");   	
    	//return OMX_ErrorInsufficientResources;
    }		
    AvcDecoder_OMX::g_h264_dec_inst_num++;
	
    ipAvcDec = OSCL_NEW(AvcDecoder_OMX, (this));

    if (ipAvcDec == NULL)
    {
        return OMX_ErrorInsufficientResources;
    }


#if PROXY_INTERFACE

    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentSendCommand = BaseComponentSendCommand;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentGetParameter = BaseComponentGetParameter;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentSetParameter = BaseComponentSetParameter;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentGetConfig = BaseComponentGetConfig;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentSetConfig = BaseComponentSetConfig;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentGetExtensionIndex = OpenmaxAvcAOGetExtensionIndex;;//BaseComponentGetExtensionIndex;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentGetState = BaseComponentGetState;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentUseBuffer = OpenmaxAvcAOUseBuffer;//BaseComponentUseBuffer;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentAllocateBuffer = OpenmaxAvcAOAllocateBuffer;//BaseComponentAllocateBuffer;//modified by jgdu
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentFreeBuffer = OpenmaxAvcAOFreeBuffer;//BaseComponentFreeBuffer;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentEmptyThisBuffer = BaseComponentEmptyThisBuffer;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentFillThisBuffer = BaseComponentFillThisBuffer;

#endif

    return OMX_ErrorNone;
}


/** This function is called by the omx core when the component
    * is disposed by the IL client with a call to FreeHandle().
    * \param Component, the component to be disposed
    */

OMX_ERRORTYPE OpenmaxAvcAO::DestroyComponent()
{
    if (iIsInit != OMX_FALSE)
    {
        ComponentDeInit();
    }

    //Destroy the base class now
    DestroyBaseComponent();

    if (ipAvcDec)
    {
        OSCL_DELETE(ipAvcDec);
        ipAvcDec = NULL;
    }

    if (ipAppPriv)
    {
        ipAppPriv->CompHandle = NULL;

        oscl_free(ipAppPriv);
        ipAppPriv = NULL;
    }

    if(AvcDecoder_OMX::g_h264_dec_inst_num>0)
        AvcDecoder_OMX::g_h264_dec_inst_num--;
	
    return OMX_ErrorNone;
}


/* This function will be called in case of buffer management without marker bit present
 * The purpose is to copy the current input buffer into a big temporary buffer, so that
 * an incomplete/partial frame is never passed to the decoder library for decode
 */
void OpenmaxAvcAO::ComponentBufferMgmtWithoutMarker()
{
    //This common routine has been written in the base class
    TempInputBufferMgmtWithoutMarker();
}

OMX_BOOL OpenmaxAvcAO::ParseFullAVCFramesIntoNALs(OMX_BUFFERHEADERTYPE* aInputBuffer)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : ParseFullAVCFramesIntoNALs IN"));

    ipInputBuffer = aInputBuffer;

    if (iNumInputBuffer == 0)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentBase : ParseFullAVCFramesIntoNALs ERROR"));
        return OMX_FALSE;
    }

    if (iPVCapabilityFlags.iOMXComponentUsesNALStartCodes && !(ipInputBuffer->nFlags & OMX_BUFFERFLAG_EXTRADATA))
    {
        OMX_U32 offset = ipInputBuffer->nOffset;
        OMX_U32 length = ipInputBuffer->nFilledLen;
        OMX_U8* pBuffer = ipInputBuffer->pBuffer + offset;
        OMX_U8* pNAL;
        int nalSize;

        iNumNALs = 0;

        while (length > 0)
        {
            if (AVCDEC_SUCCESS != ipAvcDec->GetNextFullNAL_OMX(&pNAL, &nalSize, pBuffer, &length))
            {
                break;
            }

            iNALSizeArray[iNumNALs] = nalSize;
            iNALStartCodeSizeArray[iNumNALs] = (uint32)(pNAL - pBuffer);

            // increment buffer to end of current NAL
            pBuffer = pNAL + nalSize;

            iNumNALs++;
        }

        if (iNumNALs > 0)
        {
            iCurrNAL = 0;
            iNALOffset = ipInputBuffer->nOffset + iNALStartCodeSizeArray[iCurrNAL];
            ipFrameDecodeBuffer = ipInputBuffer->pBuffer + iNALOffset;
            iInputCurrLength = iNALSizeArray[iCurrNAL];
            if (iNumNALs > 1)
            {
                iNALOffset += (iInputCurrLength + iNALStartCodeSizeArray[iCurrNAL + 1]); // offset for next NAL
            }

            //capture the timestamp to be send to the corresponding output buffer
            iFrameTimestamp = ipInputBuffer->nTimeStamp;
        }
        else
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentBase : ParseFullAVCFramesIntoNALs ERROR"));
            return OMX_FALSE;
        }
    }
    else if ((iPVCapabilityFlags.iOMXComponentUsesInterleaved2BNALSizes || iPVCapabilityFlags.iOMXComponentUsesInterleaved4BNALSizes)
             && !(ipInputBuffer->nFlags & OMX_BUFFERFLAG_EXTRADATA))
    {
        OMX_U32 offset = ipInputBuffer->nOffset;
        OMX_S32 length = ipInputBuffer->nFilledLen;
        OMX_U8* pBuffer = ipInputBuffer->pBuffer + offset;
        int32 nalSize = 0;

        iNumNALs = 0;

        while (length > 0)
        {
            if (iSizeOfNALSize == sizeof(OMX_U16))
            {
                nalSize = (OMX_U32)(*((OMX_U16*)pBuffer));
            }
            else if (iSizeOfNALSize == sizeof(OMX_U32))
            {
                nalSize = *((OMX_U32*)pBuffer);
            }

            uint32 jmp = nalSize + iSizeOfNALSize;
            length -= jmp;

            // make sure that nalSize fits in this buffer (since buffer should be full frames and therefore no partial NALs)
            if (length >= 0)
            {
                pBuffer += jmp;
                iNALSizeArray[iNumNALs] = nalSize;
                iNumNALs++;
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentBase : ParseFullAVCFramesIntoNALs ERROR"));
                return OMX_FALSE;
            }
        }

        if (iNumNALs > 0)
        {
            iCurrNAL = 0;
            iNALOffset = ipInputBuffer->nOffset + iSizeOfNALSize;
            ipFrameDecodeBuffer = ipInputBuffer->pBuffer + iNALOffset;
            iInputCurrLength = iNALSizeArray[iCurrNAL];
            iNALOffset += (iInputCurrLength + iSizeOfNALSize); // offset for next NAL
            //capture the timestamp to be send to the corresponding output buffer
            iFrameTimestamp = ipInputBuffer->nTimeStamp;
        }
        else
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentBase : ParseFullAVCFramesIntoNALs ERROR"));
            return OMX_FALSE;
        }
    }
    // may be a full frame, or may incomplete, therefore don't check for OMX_BUFFERFLAG_ENDOFFRAME
    else if (ipInputBuffer->nFlags & OMX_BUFFERFLAG_EXTRADATA)
    {
        // get extra data from end of buffer
        OMX_OTHER_EXTRADATATYPE *pExtra;
        OMX_U32 offset = ipInputBuffer->nOffset + ipInputBuffer->nFilledLen;
        OMX_U32 allocLen = ipInputBuffer->nAllocLen;
        OMX_U8* pTemp = ipInputBuffer->pBuffer + offset;

        // align
        pExtra = (OMX_OTHER_EXTRADATATYPE *)(((OMX_U32) pTemp + 3) & ~3);
        offset += (OMX_U32) pExtra - (OMX_U32) pTemp;

        while (pExtra->eType != OMX_ExtraDataNone)
        {
            if (pExtra->eType == OMX_ExtraDataNALSizeArray)
            {
                oscl_memcpy(iNALSizeArray, ((OMX_U8*)pExtra + 20), pExtra->nDataSize);
                iNumNALs = pExtra->nDataSize >> 2;
                iCurrNAL = 0;
                iNALOffset = ipInputBuffer->nOffset;
                break;
            }

            offset += pExtra->nSize;
            if (offset > (allocLen - 20))
            {
                // corrupt data
                break;
            }
            else
            {
                pExtra = (OMX_OTHER_EXTRADATATYPE *)((OMX_U8*)pExtra + pExtra->nSize);
            }
        }

        if (pExtra->eType != OMX_ExtraDataNALSizeArray)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentBase : ParseFullAVCFramesIntoNALs ERROR"));
            return OMX_FALSE;
        }

        ipFrameDecodeBuffer = ipInputBuffer->pBuffer + iNALOffset;
        iInputCurrLength = iNALSizeArray[iCurrNAL];
        iNALOffset += iInputCurrLength; // offset for next NAL
        //capture the timestamp to be send to the corresponding output buffer
        iFrameTimestamp = ipInputBuffer->nTimeStamp;
    }
    else if (ipInputBuffer->nFlags & OMX_BUFFERFLAG_CODECCONFIG)
    {
        iInputCurrLength = ipInputBuffer->nFilledLen;
        ipFrameDecodeBuffer = ipInputBuffer->pBuffer + ipInputBuffer->nOffset;
        //capture the timestamp to be send to the corresponding output buffer
        iFrameTimestamp = ipInputBuffer->nTimeStamp;
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentBase : ParseFullAVCFramesIntoNALs ERROR"));
        return OMX_FALSE;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : ParseFullAVCFramesIntoNALs OUT"));
    return OMX_TRUE;
}


void OpenmaxAvcAO::ProcessData()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : ProcessData IN"));
    if (!iEndOfFrameFlag)
    {
        DecodeWithoutMarker();
    }
    else
    {
        DecodeWithMarker();
    }
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : ProcessData OUT"));
}

void OpenmaxAvcAO::pop_output_buffer_from_queue(OMX_BUFFERHEADERTYPE *pTargetBuffer)
{
    BufferCtrlStruct *pBCTRL = (BufferCtrlStruct *)pTargetBuffer->pOutputPortPrivate;
    if(pBCTRL->iIsBufferInComponentQueue == OMX_TRUE){
	SCI_TRACE_LOW("pop_output_buffer_from_queue %x",pTargetBuffer);
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
    }else if(pBCTRL->iRefCount == 0){
    	iNumAvailableOutputBuffers++;
    }
}

/* Decoding function for input buffers without end of frame flag marked */
void OpenmaxAvcAO::DecodeWithoutMarker()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : DecodeWithoutMarker IN"));

    QueueType*              pInputQueue  = ipPorts[OMX_PORT_INPUTPORT_INDEX]->pBufferQueue;
    QueueType*              pOutputQueue = ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->pBufferQueue;
    ComponentPortType*  pOutPort =     ipPorts[OMX_PORT_OUTPUTPORT_INDEX];
    OMX_COMPONENTTYPE *     pHandle =      &iOmxComponent;

    OMX_U8*                 pTempInBuffer;
    OMX_U32                 TempInLength;
    OMX_BOOL                MarkerFlag = OMX_FALSE;
    OMX_BOOL                ResizeNeeded = OMX_FALSE;

    OMX_U32 TempInputBufferSize = (2 * sizeof(uint8) * (ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.nBufferSize));
    OMX_U32 CurrWidth =  ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.nFrameWidth;
    OMX_U32 CurrHeight = ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.nFrameHeight;


    if ((!iIsInputBufferEnded) || ((iEndofStream) || (0 != iTempInputBufferLength)))
    {
        //Check whether prev output bufer has been released or not
        if (OMX_TRUE == iNewOutBufRequired)
        {
            //Check whether a new output buffer is available or not

            if (0 == iNumAvailableOutputBuffers)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : DecodeWithoutMarker OUT output buffer unavailable"));
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
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : DecodeWithoutMarker Error, output buffer dequeue returned NULL, OUT"));
                    return;
                }

                pBCTRL = (BufferCtrlStruct *)ipOutputBuffer->pOutputPortPrivate;
                if (pBCTRL->iRefCount == 0)
                {
                    //buffer is available
                    iNumAvailableOutputBuffers--;
                    pBCTRL->iIsBufferInComponentQueue = OMX_FALSE;
                    break; // found buffer, EXIT
                }
                else
                {
                    Queue(pOutputQueue, (void*)ipOutputBuffer);
                }
            }
            while (1);



            //Do not proceed if the output buffer can't fit the YUV data
            if ((ipOutputBuffer->nAllocLen < (OMX_U32)(((CurrWidth + 15)&(~15)) *((CurrHeight + 15)&(~15)) * 3 / 2)) && (OMX_TRUE == ipAvcDec->iAvcActiveFlag))
            {
                ipOutputBuffer->nFilledLen = 0;
                ReturnOutputBuffer(ipOutputBuffer, pOutPort);
                ipOutputBuffer = NULL;
                return;
            }

            ipOutputBuffer->nFilledLen = 0;
            iNewOutBufRequired = OMX_FALSE;
        }


        /* Code for the marking buffer. Takes care of the OMX_CommandMarkBuffer
         * command and hMarkTargetComponent as given by the specifications
         */
        if (NULL != ipMark)
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
        }
        else if (ipTargetComponent != NULL)
        {
            ipOutputBuffer->hMarkTargetComponent = ipTargetComponent;
            ipOutputBuffer->pMarkData = iTargetMarkData;
            ipTargetComponent = NULL;
            iMarkPropagate = OMX_FALSE;
        }
        //Mark buffer code ends here


        pTempInBuffer = ipTempInputBuffer + iTempConsumedLength;
        TempInLength = iTempInputBufferLength;

        //Output buffer ipOutputBuffer is passed in - decoder may or may not use it as a reference
        // if the decoder decides to use ipOutputBuffer it will set ipOutputBuffer to NULL
        // Decoder may or may not produce a different output buffer in ipOutputBufferForRendering
        // if decoder produces output buffer for rendering - ipOutputBufferForRendering will != NULL
        ipOutputBufferForRendering = NULL;

    	OMX_BOOL  need_new_pic = OMX_TRUE;
        OMX_U32  OutputLength = 0;	
        OMX_BOOL notSupport = OMX_FALSE;		
        iDecodeReturn = ipAvcDec->AvcDecodeVideo_OMX(&need_new_pic,ipOutputBuffer,&OutputLength, &ipOutputBufferForRendering,
                        &(pTempInBuffer),
                        &TempInLength,
                        &(ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam),
                        &iFrameCount,
                        MarkerFlag, &ResizeNeeded,&notSupport);

/*
        if (ipOutputBufferForRendering)
        {
            //Set the timestamp equal to the input buffer timestamp
            ipOutputBufferForRendering->nTimeStamp = iFrameTimestamp;
        }
*/
	    //modified by jgdu			
        if (NULL != ipOutputBuffer)
        {
                ipOutputBuffer->nFilledLen = OutputLength;
                //offset not required in our case, set it to zero
                ipOutputBuffer->nOffset = 0;
                //Set the timestamp equal to the input buffer timestamp
                ipOutputBuffer->nTimeStamp = iFrameTimestamp;
         }
	 if(need_new_pic)
	 {
		 iNewOutBufRequired = OMX_TRUE;
		 ipOutputBuffer = NULL;
	}		
		
        iTempConsumedLength += (iTempInputBufferLength - TempInLength);
        iTempInputBufferLength = TempInLength;

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
        if (!iDecodeReturn && OMX_FALSE == ipAvcDec->iAvcActiveFlag)
        {
            // initialization error
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : DecodeWithoutMarker ErrorBadParameter callback send"));
/*jgdu
            (*(ipCallbacks->EventHandler))
            (pHandle,
             iCallbackData,
             OMX_EventError,
             OMX_ErrorBadParameter,
             0,
             NULL);
*/             
        }
        else if (!iDecodeReturn && OMX_FALSE == iEndofStream)
        {
            // decoding error
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : DecodeWithoutMarker ErrorStreamCorrupt callback send"));

            (*(ipCallbacks->EventHandler))
            (pHandle,
             iCallbackData,
             OMX_EventError,
             OMX_ErrorStreamCorrupt,
             0,
             NULL);
        }


        //Do not decode if big buffer is less than half the size
        if (TempInLength < (TempInputBufferSize >> 1))
        {
            iIsInputBufferEnded = OMX_TRUE;
            iNewInBufferRequired = OMX_TRUE;
        }

        if (ResizeNeeded == OMX_TRUE)
        {
            OMX_COMPONENTTYPE* pHandle = (OMX_COMPONENTTYPE*) ipAppPriv->CompHandle;

            // make sure to release buffers held by decoder
            ReleaseReferenceBuffers();

            iResizePending = OMX_TRUE;
            (*(ipCallbacks->EventHandler))
            (pHandle,
             iCallbackData,
             OMX_EventPortSettingsChanged, //The command was completed
             OMX_PORT_OUTPUTPORT_INDEX,
             0,
             NULL);
        }

        /* If EOS flag has come from the client & there are no more
         * input buffers to decode, send the callback to the client
         */
        if (OMX_TRUE == iEndofStream)
        {
            if ((0 == iTempInputBufferLength) && !iDecodeReturn)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : DecodeWithoutMarker EOS callback send"));

                (*(ipCallbacks->EventHandler))
                (pHandle,
                 iCallbackData,
                 OMX_EventBufferFlag,
                 1,
                 OMX_BUFFERFLAG_EOS,
                 NULL);

                iNewInBufferRequired = OMX_TRUE;
                iEndofStream = OMX_FALSE;

                if (ipOutputBuffer)
                {
                    ipOutputBuffer->nFlags |= OMX_BUFFERFLAG_EOS;

                    ReturnOutputBuffer(ipOutputBuffer, pOutPort);
                    ipOutputBuffer = NULL;
                }

                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : DecodeWithoutMarker OUT"));

                // method ReturnOutputBuffer automatically sets flag iNewOutBufRequired to true
                // it is possible that the decoder never used ipOutputBuffer for reference.
                // in this case - keep the ipOutputBuffer and reset the flag back


                if (OMX_TRUE == iNewOutBufRequired && ipOutputBuffer != NULL)
                {
                    iNewOutBufRequired = OMX_FALSE;
                }

                return;
            }
        }


        //Send the output buffer back when it has some data in it
        if (ipOutputBufferForRendering)
        {
            pop_output_buffer_from_queue(ipOutputBufferForRendering);	 
            ReturnOutputBuffer(ipOutputBufferForRendering, pOutPort);
            ipOutputBufferForRendering = NULL;
        }

        // method ReturnOutputBuffer automatically sets flag iNewOutBufRequired to true
        // it is possible that the decoder never used ipOutputBuffer for reference.
        // in this case - keep the ipOutputBuffer and reset the flag back

        if (OMX_TRUE == iNewOutBufRequired && ipOutputBuffer != NULL)
        {
            iNewOutBufRequired = OMX_FALSE;
        }

        /* If there is some more processing left with current buffers, re-schedule the AO
         * Do not go for more than one round of processing at a time.
         * This may block the AO longer than required.
         */
        if ((ResizeNeeded == OMX_FALSE) && ((TempInLength != 0) || (GetQueueNumElem(pInputQueue) > 0))
                && ((iNumAvailableOutputBuffers > 0) || (OMX_FALSE == iNewOutBufRequired)))
        {
            RunIfNotReady();
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : DecodeWithoutMarker OUT"));
    return;
}


/* Decoding function for input buffers with end of frame flag marked */
void OpenmaxAvcAO::DecodeWithMarker()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : DecodeWithMarker IN"));
    if(iLogCount%8==0){	
    	SCI_TRACE_LOW("OMXAvc::Dec %d,%d,%d,%d,%x:%d\n",iOutBufferCount,iNewOutBufRequired,iNumAvailableOutputBuffers,GetQueueNumElem( ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->pBufferQueue),ipOutputBuffer,iFrameCount);		
    }
    iLogCount++;
	
    OMX_COMPONENTTYPE  *pHandle = &iOmxComponent;
    QueueType* pInputQueue = ipPorts[OMX_PORT_INPUTPORT_INDEX]->pBufferQueue;
    QueueType* pOutputQueue = ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->pBufferQueue;

    ComponentPortType*  pInPort = ipPorts[OMX_PORT_INPUTPORT_INDEX];
    ComponentPortType*  pOutPort = ipPorts[OMX_PORT_OUTPUTPORT_INDEX];

    OMX_BOOL                MarkerFlag = OMX_TRUE;
    OMX_BOOL                ResizeNeeded = OMX_FALSE;

    OMX_U32 CurrWidth =  ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.nFrameWidth;
    OMX_U32 CurrHeight = ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.nFrameHeight;

    if ((!iIsInputBufferEnded) || (iEndofStream))
    {
        //Check whether prev output bufer has been released or not
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
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : DecodeWithMarker Error, output buffer dequeue returned NULL, OUT"));
                        return;
                    }

                    pBCTRL = (BufferCtrlStruct *)ipOutputBuffer->pOutputPortPrivate;
                    if (pBCTRL->iRefCount == 0)
                    {
                        //buffer is available
                        iNumAvailableOutputBuffers--;
                        pBCTRL->iIsBufferInComponentQueue = OMX_FALSE;
                        break; // found buffer, EXIT
                    }
                    else
                    {
                        // queue the buffer back
                        Queue(pOutputQueue, (void*)ipOutputBuffer);
                    }
                }
                while (1);



                //Do not proceed if the output buffer can't fit the YUV data
                if ((ipOutputBuffer->nAllocLen < (OMX_U32)(((CurrWidth + 15)&(~15)) *((CurrHeight + 15)&(~15)) * 3 / 2)) && (OMX_TRUE == ipAvcDec->iAvcActiveFlag))
                {
                    ipOutputBuffer->nFilledLen = 0;
                    ReturnOutputBuffer(ipOutputBuffer, pOutPort);
                    ipOutputBuffer = NULL;
                    return;
                }
                ipOutputBuffer->nFilledLen = 0;
                iNewOutBufRequired = OMX_FALSE;
            }
            else// if (OMX_TRUE == ipAvcDec->iAvcActiveFlag)
            {
                iNewInBufferRequired = OMX_FALSE;
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : DecodeWithMarker OUT output buffer unavailable"));
                return;
            }
        }

        if (NULL != ipOutputBuffer)
        {
            /* Code for the marking buffer. Takes care of the OMX_CommandMarkBuffer
             * command and hMarkTargetComponent as given by the specifications
             */
            if (NULL != ipMark)
            {
                ipOutputBuffer->hMarkTargetComponent = ipMark->hMarkTargetComponent;
                ipOutputBuffer->pMarkData = ipMark->pMarkData;
                ipMark = NULL;
            }

            if (NULL != ipTargetComponent)
            {
                ipOutputBuffer->hMarkTargetComponent = ipTargetComponent;
                ipOutputBuffer->pMarkData = iTargetMarkData;
                ipTargetComponent = NULL;

            }
            //Mark buffer code ends here
        }

        if (iInputCurrLength > 0)
        {
            //Store the input timestamp into a temp variable
            ipAvcDec->CurrInputTimestamp = iFrameTimestamp;

            //Output buffer ipOutputBuffer is passed in - decoder may or may not use it as a reference
            // if the decoder decides to use ipOutputBuffer it will set ipOutputBuffer to NULL
            // Decoder may or may not produce a different output buffer in ipOutputBufferForRendering
            // if there is output to be rendered - then ipOutputBufferForRendering will be != NULL
            ipOutputBufferForRendering = NULL;
			
    	    OMX_BOOL  need_new_pic = OMX_TRUE;
	    OMX_U32  OutputLength = 0;	
	    OMX_BOOL notSupport = OMX_FALSE;	
	   // SCI_TRACE_LOW("VSP DPB::ipOutputBufferForRendering begin %x,%x,%x\n ",ipOutputBuffer->pBuffer,ipOutputBuffer,(int)iFrameTimestamp);//.nHighPart,ipOutputBufferForRendering->nTimeStamp.nLowPart);	
		
            iDecodeReturn = ipAvcDec->AvcDecodeVideo_OMX(&need_new_pic,ipOutputBuffer,&OutputLength, &ipOutputBufferForRendering,
                            &(ipFrameDecodeBuffer),
                            &(iInputCurrLength),
                            &(ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam),
                            &iFrameCount,
                            MarkerFlag,
                            &ResizeNeeded,&notSupport);
			
	    //modified by jgdu			
            if (NULL != ipOutputBuffer)
            {
                ipOutputBuffer->nFilledLen = OutputLength;
                //offset not required in our case, set it to zero
                ipOutputBuffer->nOffset = 0;
                //Set the timestamp equal to the input buffer timestamp
                ipOutputBuffer->nTimeStamp = iFrameTimestamp;
            }
	    if(need_new_pic)
	    {
		 iNewOutBufRequired = OMX_TRUE;
		 ipOutputBuffer = NULL;
	    }		

            if (ResizeNeeded == OMX_TRUE)
            {
                OMX_COMPONENTTYPE* pHandle = (OMX_COMPONENTTYPE*) ipAppPriv->CompHandle;

                // set the flag. Do not process any more frames until
                // IL Client sends PortDisable event (thus starting the procedure for
                // dynamic port reconfiguration)
                iResizePending = OMX_TRUE;

                // make sure to release buffers held by decoder
                ReleaseReferenceBuffers();
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : Sending the PortSettings Changed Callback"));

                (*(ipCallbacks->EventHandler))
                (pHandle,
                 iCallbackData,
                 OMX_EventPortSettingsChanged, //The command was completed
                 OMX_PORT_OUTPUTPORT_INDEX,
                 0,
                 NULL);
            }


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
            if (!iDecodeReturn && OMX_FALSE == ipAvcDec->iAvcActiveFlag)
            {
                // initialization error
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : DecodeWithoutMarker ErrorBadParameter callback send"));

/*jgdu
                (*(ipCallbacks->EventHandler))
                (pHandle,
                 iCallbackData,
                 OMX_EventError,
                 OMX_ErrorBadParameter,
                 0,
                 NULL);
*/                 
            }
            else if (!iDecodeReturn && OMX_FALSE == iEndofStream)
            {
                // decode error
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : DecodeWithMarker ErrorStreamCorrupt callback send"));

                (*(ipCallbacks->EventHandler))
                (pHandle,
                 iCallbackData,
                 OMX_EventError,
                 OMX_ErrorStreamCorrupt,
                 0,
                 NULL);
            }


            if (0 == iInputCurrLength)
            {
                if (iPVCapabilityFlags.iOMXComponentUsesFullAVCFrames)
                {
                    iCurrNAL++;

                    if (iCurrNAL < iNumNALs)
                    {
                        ipFrameDecodeBuffer = ipInputBuffer->pBuffer + iNALOffset;
                        iInputCurrLength = iNALSizeArray[iCurrNAL];

                        if (iPVCapabilityFlags.iOMXComponentUsesNALStartCodes && ((iNumNALs - iCurrNAL) > 1)) // how many NALs are left? make sure at least 1
                        {
                            iNALOffset += (iInputCurrLength + iNALStartCodeSizeArray[iCurrNAL + 1]); // offset for next NAL
                        }
                        else if (iPVCapabilityFlags.iOMXComponentUsesInterleaved2BNALSizes || iPVCapabilityFlags.iOMXComponentUsesInterleaved4BNALSizes)
                        {
                            iNALOffset += (iInputCurrLength + iSizeOfNALSize); // offset for next NAL
                        }
                        else
                        {
                            iNALOffset += iInputCurrLength; // offset for next NAL
                        }

                        iNewInBufferRequired = OMX_FALSE;
                    }
                    else
                    {
                        ipInputBuffer->nFilledLen = 0;
                        ReturnInputBuffer(ipInputBuffer, pInPort);
                        iNewInBufferRequired = OMX_TRUE;
                        iIsInputBufferEnded = OMX_TRUE;
                        ipInputBuffer = NULL;
                    }
                }
                else
                {
                    ipInputBuffer->nFilledLen = 0;
                    ReturnInputBuffer(ipInputBuffer, pInPort);
                    iNewInBufferRequired = OMX_TRUE;
                    iIsInputBufferEnded = OMX_TRUE;
                    ipInputBuffer = NULL;
                }
            }
            else
            {
                iNewInBufferRequired = OMX_FALSE;
            }
        }
        else if (iEndofStream == OMX_FALSE)
        {
            // it's possible that after partial frame assembly, the input buffer still remains empty (due to
            // client erroneously sending such buffers). This may cause error in processing/returning buffers
            // This code adds robustness in the sense that it returns such buffer to the client

            ipInputBuffer->nFilledLen = 0;
            ReturnInputBuffer(ipInputBuffer, pInPort);
            ipInputBuffer = NULL;
            iNewInBufferRequired = OMX_TRUE;
            iIsInputBufferEnded = OMX_TRUE;
        }


        /* If EOS flag has come from the client & there are no more
         * input buffers to decode, send the callback to the client*/
        if (OMX_TRUE == iEndofStream)
        {
            if (!iFlushOutputStatus)
            {
                // this is the very last buffer to be sent out.
                // it is empty - and has EOS flag attached to it
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : DecodeWithMarker EOS callback send"));

                (*(ipCallbacks->EventHandler))
                (pHandle,
                 iCallbackData,
                 OMX_EventBufferFlag,
                 1,
                 OMX_BUFFERFLAG_EOS,
                 NULL);

                iNewInBufferRequired = OMX_TRUE;
                iEndofStream = OMX_FALSE;
                iFlushOutputStatus = OMX_TRUE;

                // in this case - simply return the available ipOutputBuffer
                // there is no need for involving decoder and ipOutputBufferForRendering - since this
                // is an empty buffer
                if (ipOutputBuffer)
                {
                    ipOutputBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
                    ReturnOutputBuffer(ipOutputBuffer, pOutPort);
                    ipOutputBuffer = NULL;
                }

                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : DecodeWithMarker OUT"));

                return;
            }
            else if (iFlushOutputStatus)
            {
                // flush output buffer
                ipOutputBufferForRendering = NULL;

                iFlushOutputStatus = ipAvcDec->FlushOutput_OMX(&ipOutputBufferForRendering);

                if (OMX_FALSE != iFlushOutputStatus)
                {
                    // this is the case where Flush succeeded (i.e. there is one output
                    // buffer left)
                    if (ipOutputBufferForRendering)
                    {
                        pop_output_buffer_from_queue(ipOutputBufferForRendering);	
                        ReturnOutputBuffer(ipOutputBufferForRendering, pOutPort);
                        ipOutputBufferForRendering = NULL;
                    }

                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : DecodeWithMarker OUT"));
                    RunIfNotReady();

                    // method ReturnOutputBuffer automatically sets flag iNewOutBufRequired to true
                    // it is possible that the decoder never used ipOutputBuffer for reference.
                    // in this case - keep the ipOutputBuffer and reset the flag back

                    if (OMX_TRUE == iNewOutBufRequired && ipOutputBuffer != NULL)
                    {
                        iNewOutBufRequired = OMX_FALSE;
                    }

                    return;
                }
                else
                {
                    // this is the case where no output buffers can be flushed.
                    // spin the AO once again to send the empty output buffer with EOS attached to it
                    RunIfNotReady();
                }
            }
        }

        //Send the output buffer back when it has some data in it
        if (ipOutputBufferForRendering)
        {
            //SCI_TRACE_LOW("VSP DPB::ipOutputBufferForRendering end %x,%x,%x,%d,%d\n ",ipOutputBufferForRendering->pBuffer,ipOutputBufferForRendering,(int)ipOutputBufferForRendering->nTimeStamp,ipOutputBufferForRendering->nOffset,ipOutputBufferForRendering->nFilledLen);//.nHighPart,ipOutputBufferForRendering->nTimeStamp.nLowPart);        
            pop_output_buffer_from_queue(ipOutputBufferForRendering);	
	     ReturnOutputBuffer(ipOutputBufferForRendering, pOutPort);
            ipOutputBufferForRendering = NULL;
        }

        // method ReturnOutputBuffer automatically sets flag iNewOutBufRequired to true
        // it is possible that the decoder never used ipOutputBuffer for reference.
        // in this case - keep the ipOutputBuffer and reset the flag back

        if (OMX_TRUE == iNewOutBufRequired && ipOutputBuffer != NULL)
        {
            iNewOutBufRequired = OMX_FALSE;
        }


        /* If there is some more processing left with current buffers, re-schedule the AO
         * Do not go for more than one round of processing at a time.
         * This may block the AO longer than required.
         */
        if ((ResizeNeeded == OMX_FALSE) && ((iInputCurrLength != 0) || (GetQueueNumElem(pInputQueue) > 0))
                && ((iNumAvailableOutputBuffers > 0) || (OMX_FALSE == iNewOutBufRequired)))
        {
            RunIfNotReady();
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : DecodeWithMarker OUT"));
    return;
}





//Component object constructor
OpenmaxAvcAO::OpenmaxAvcAO()
{
    ipAvcDec = NULL;

    if (!IsAdded())
    {
        AddToScheduler();
    }
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : constructed"));
}


//Component object destructor
OpenmaxAvcAO::~OpenmaxAvcAO()
{
    if (IsAdded())
    {
        RemoveFromScheduler();
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : destructed"));
}


/** The Initialization function
 */
OMX_ERRORTYPE OpenmaxAvcAO::ComponentInit()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : ComponentInit IN"));

    OMX_ERRORTYPE Status = OMX_ErrorNone;

    if (OMX_TRUE == iIsInit)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : ComponentInit error incorrect operation"));
        return OMX_ErrorIncorrectStateOperation;
    }
    iIsInit = OMX_TRUE;


    //avc lib init
    if (!iCodecReady)
    {
        Status = ipAvcDec->AvcDecInit_OMX();
        iCodecReady = OMX_TRUE;
    }

    iInputCurrLength = 0;
    iNumNALs = 0;
    iCurrNAL = 0;
    iNALOffset = 0;
    oscl_memset(iNALSizeArray, 0, MAX_NAL_PER_FRAME * sizeof(uint32));
    //Used in dynamic port reconfiguration
    iFrameCount = 0;
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : ComponentInit OUT"));

    return Status;

}

/** This function is called upon a transition to the idle or invalid state.
 *  Also it is called by the AvcComponentDestructor() function
 */
OMX_ERRORTYPE OpenmaxAvcAO::ComponentDeInit()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : ComponentDeInit IN"));

    OMX_ERRORTYPE Status = OMX_ErrorNone;

    iIsInit = OMX_FALSE;

    if (iCodecReady)
    {
        Status = ipAvcDec->AvcDecDeinit_OMX();
        iCodecReady = OMX_FALSE;
    }

#if PROFILING_ON
    if (0 != iFrameCount)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iDiagnosticsLogger, PVLOGMSG_INFO, (0, "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"));

        PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iDiagnosticsLogger, PVLOGMSG_INFO, (0, "OpenmaxAvcAO - Total Decoding Time (ms) = %d", OsclTickCount::TicksToMsec(ipAvcDec->iTotalTicks)));
        PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iDiagnosticsLogger, PVLOGMSG_INFO, (0, "OpenmaxAvcAO - Total Number of Decoded Frames = %d", iFrameCount));
    }
#endif

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : ComponentDeInit OUT"));

    return Status;

}


OMX_ERRORTYPE OpenmaxAvcAO::GetConfig(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nIndex,
    OMX_INOUT OMX_PTR pComponentConfigStructure)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
       OpenmaxAvcAO* pOpenmaxAOType = (OpenmaxAvcAO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;   
    	if (NULL == pOpenmaxAOType)
    	{
            return OMX_ErrorBadParameter;
    	}
       switch (nIndex) {
	    case OMX_IndexConfigCommonOutputCrop:
	    {
	    		OMX_CONFIG_RECTTYPE *rectParams = (OMX_CONFIG_RECTTYPE *)pComponentConfigStructure;

            		if (rectParams->nPortIndex != OMX_PORT_OUTPUTPORT_INDEX) {
                		return OMX_ErrorUndefined;
            		}
			ComponentPortType *pOutPort = (ComponentPortType*) ipPorts[OMX_PORT_OUTPUTPORT_INDEX];
            		rectParams->nLeft = 0;
            		rectParams->nTop = 0;
            		rectParams->nWidth = pOutPort->PortParam.format.video.nFrameWidth;
            		rectParams->nHeight = pOutPort->PortParam.format.video.nFrameHeight ;
			SCI_TRACE_LOW("OpenmaxAvcAO OMX_IndexConfigCommonOutputCrop %d,%d,%d,%d",rectParams->nLeft,rectParams->nTop,rectParams->nWidth,rectParams->nHeight);	
	    }	
			break;
	    default:
			ret = OmxComponentVideo::GetConfig(hComponent,nIndex,pComponentConfigStructure);
			break;
       }
	
	return ret;
}

/* This routine will reset the decoder library and some of the associated flags*/
void OpenmaxAvcAO::ResetComponent()
{
    // reset decoder
    if (ipAvcDec)
    {
        ipAvcDec->ResetDecoder();
    }

}

void OpenmaxAvcAO::ReleaseReferenceBuffers()
{
    if (ipAvcDec)
    {
    #if 1
		OMX_BOOL status;
	
    	// flush output buffer
		do{
			ComponentPortType*  pOutPort = ipPorts[OMX_PORT_OUTPUTPORT_INDEX];
			ipOutputBufferForRendering = NULL;
			SCI_TRACE_LOW("Seeklog :::iFlushOutputStatus% d\n",(int)status);
			status = ipAvcDec->FlushOutput_OMX(&ipOutputBufferForRendering);
			SCI_TRACE_LOW("Seeklog :::iFlushOutputStatus% d\n",(int)status);
			SCI_TRACE_LOW("ipOutputBufferForRendering :addr:: %x\n",ipOutputBufferForRendering);
			if (OMX_FALSE != status)
			{
				// this is the case where Flush succeeded (i.e. there is one output buffer left)
				if (ipOutputBufferForRendering)
				{
					pop_output_buffer_from_queue(ipOutputBufferForRendering);	
					ReturnOutputBuffer(ipOutputBufferForRendering, pOutPort);
					//  ipOutputBufferForRendering = NULL;
				}

				PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : DecodeWithMarker OUT"));
				RunIfNotReady();
			SCI_TRACE_LOW("ipOutputBufferForRendering :addr:: %x\n",ipOutputBufferForRendering);
			}
		}while(ipOutputBufferForRendering);
#endif	
	
        ipAvcDec->ReleaseReferenceBuffers();
    }
}

/* This routine finds the size of the start code and points to the beginning of the NAL.
 * The start code should be at the beginning of the buffer for this routine to function properly.
 * If the buffer does not have a valid start code at the beginning of the buffer, the function
 * will return false.
 */
OMX_BOOL OpenmaxAvcAO::DetectStartCodeLength(OMX_U8* aBitstream, OMX_U8** aNalUnit, OMX_U32 aBufSize, OMX_U32* aSCSize)
{
    uint32 i = 0;

    while (aBitstream[i] == 0 && i < aBufSize)
    {
        i++;
    }
    if (i >= aBufSize)
    {
        // buffer is entirely 0s - cannot find any start_code_prefix.
        *aNalUnit = aBitstream;
        *aSCSize = 0;
        return OMX_FALSE;
    }
    else if (aBitstream[i] != 0x1)
    {
        // start_code_prefix is not at the beginning
        *aNalUnit = aBitstream;
        *aSCSize = 0;
        return OMX_FALSE;
    }
    else
    {
        // start_code found
        i++;
        *aNalUnit = aBitstream + i; /* point to the beginning of the NAL unit */
        *aSCSize = i;

        if (i != 3 && i != 4)
        {
            // AVC start codes are either 3 or 4 bytes.  if it finds otherwise, then this frame is corrupt
            return OMX_FALSE;
        }
        else
        {
            return OMX_TRUE;
        }
    }
}

void OpenmaxAvcAO::CalculateBufferParameters(OMX_U32 PortIndex)
{
    ipAvcDec->CalculateBufferParameters(&ipPorts[PortIndex]->PortParam);

    return ;
}

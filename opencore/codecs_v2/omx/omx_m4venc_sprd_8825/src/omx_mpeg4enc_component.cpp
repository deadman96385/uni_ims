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


#include "omx_mpeg4enc_component.h"

#if PROXY_INTERFACE
#include "omx_proxy_interface.h"
#endif

#include <media/hardware/HardwareAPI.h>


// This function is called by OMX_GetHandle and it creates an instance of the mpeg4 component AO
OSCL_EXPORT_REF OMX_ERRORTYPE Mpeg4EncOmxComponentFactory(OMX_OUT OMX_HANDLETYPE* pHandle, OMX_IN OMX_PTR pAppData, OMX_PTR pProxy, OMX_STRING aOmxLibName, OMX_PTR &aOmxLib, OMX_PTR aOsclUuid, OMX_U32 &aRefCount)
{
    OSCL_UNUSED_ARG(aOmxLibName);
    OSCL_UNUSED_ARG(aOmxLib);
    OSCL_UNUSED_ARG(aOsclUuid);
    OSCL_UNUSED_ARG(aRefCount);


    OmxComponentMpeg4EncAO* pOpenmaxAOType;
    OMX_ERRORTYPE Status;

    // move InitMpeg4OmxComponentFields content to actual constructor

    pOpenmaxAOType = (OmxComponentMpeg4EncAO*) OSCL_NEW(OmxComponentMpeg4EncAO, ());

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorInsufficientResources;
    }

    // set encoding mode to H263
    pOpenmaxAOType->SetEncoderMode(MODE_MPEG4);

    //Call the construct component to initialize OMX types
    Status = pOpenmaxAOType->ConstructComponent(pAppData, pProxy);

    *pHandle = pOpenmaxAOType->GetOmxHandle();

    return Status;
}


// This function is called by OMX_FreeHandle when component AO needs to be destroyed
OSCL_EXPORT_REF OMX_ERRORTYPE Mpeg4EncOmxComponentDestructor(OMX_IN OMX_HANDLETYPE pHandle, OMX_PTR &aOmxLib, OMX_PTR aOsclUuid, OMX_U32 &aRefCount)
{
    OSCL_UNUSED_ARG(aOmxLib);
    OSCL_UNUSED_ARG(aOsclUuid);
    OSCL_UNUSED_ARG(aRefCount);

    // get pointer to component AO
    OmxComponentMpeg4EncAO* pOpenmaxAOType = (OmxComponentMpeg4EncAO*)((OMX_COMPONENTTYPE*)pHandle)->pComponentPrivate;

    // clean up decoder, OMX component stuff
    pOpenmaxAOType->DestroyComponent();

    // destroy the AO class
    OSCL_DELETE(pOpenmaxAOType);

    return OMX_ErrorNone;
}


// This function is called by OMX_GetHandle and it creates an instance of the h263 component AO
OSCL_EXPORT_REF OMX_ERRORTYPE H263EncOmxComponentFactory(OMX_OUT OMX_HANDLETYPE* pHandle, OMX_IN  OMX_PTR pAppData, OMX_PTR pProxy, OMX_STRING aOmxLibName, OMX_PTR &aOmxLib, OMX_PTR aOsclUuid, OMX_U32 &aRefCount)
{
    OSCL_UNUSED_ARG(aOmxLibName);
    OSCL_UNUSED_ARG(aOmxLib);
    OSCL_UNUSED_ARG(aOsclUuid);
    OSCL_UNUSED_ARG(aRefCount);


    OmxComponentMpeg4EncAO* pOpenmaxAOType;
    OMX_ERRORTYPE Status;

    // move InitMpeg4OmxComponentFields content to actual constructor

    pOpenmaxAOType = (OmxComponentMpeg4EncAO*) OSCL_NEW(OmxComponentMpeg4EncAO, ());

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorInsufficientResources;
    }

    // set encoding mode to H263
    pOpenmaxAOType->SetEncoderMode(MODE_H263);

    //Call the construct component to initialize OMX types
    Status = pOpenmaxAOType->ConstructComponent(pAppData, pProxy);

    *pHandle = pOpenmaxAOType->GetOmxHandle();

    return Status;
    ///////////////////////////////////////////////////////////////////////////////////////
}


// This function is called by OMX_FreeHandle when component AO needs to be destroyed
OSCL_EXPORT_REF OMX_ERRORTYPE H263EncOmxComponentDestructor(OMX_IN OMX_HANDLETYPE pHandle, OMX_PTR &aOmxLib, OMX_PTR aOsclUuid, OMX_U32 &aRefCount)
{
    OSCL_UNUSED_ARG(aOmxLib);
    OSCL_UNUSED_ARG(aOsclUuid);
    OSCL_UNUSED_ARG(aRefCount);

    // get pointer to component AO
    OmxComponentMpeg4EncAO* pOpenmaxAOType = (OmxComponentMpeg4EncAO*)((OMX_COMPONENTTYPE*)pHandle)->pComponentPrivate;

    // clean up decoder, OMX component stuff
    pOpenmaxAOType->DestroyComponent();

    // destroy the AO class
    OSCL_DELETE(pOpenmaxAOType);

    return OMX_ErrorNone;
}

#if (DYNAMIC_LOAD_OMX_M4VENC_COMPONENT || DYNAMIC_LOAD_OMX_H263ENC_COMPONENT)
class Mpeg4H263EncOmxSharedLibraryInterface:  public OsclSharedLibraryInterface,
        public OmxSharedLibraryInterface

{
    public:
        OsclAny *QueryOmxComponentInterface(const OsclUuid& aOmxTypeId, const OsclUuid& aInterfaceId)
        {
            if (PV_OMX_M4VENC_UUID == aOmxTypeId)
            {
                if (PV_OMX_CREATE_INTERFACE == aInterfaceId)
                {
                    return ((OsclAny*)(&Mpeg4EncOmxComponentFactory));
                }
                else if (PV_OMX_DESTROY_INTERFACE == aInterfaceId)
                {
                    return ((OsclAny*)(&Mpeg4EncOmxComponentDestructor));
                }
            }
            else if (PV_OMX_H263ENC_UUID == aOmxTypeId)
            {
                if (PV_OMX_CREATE_INTERFACE == aInterfaceId)
                {
                    return ((OsclAny*)(&H263EncOmxComponentFactory));
                }
                else if (PV_OMX_DESTROY_INTERFACE == aInterfaceId)
                {
                    return ((OsclAny*)(&H263EncOmxComponentDestructor));
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

        Mpeg4H263EncOmxSharedLibraryInterface() {};
};

// function to obtain the interface object from the shared library
extern "C"
{
    OSCL_EXPORT_REF OsclAny* PVGetInterface()
    {
        return (OsclAny*) OSCL_NEW(Mpeg4H263EncOmxSharedLibraryInterface, ());
    }

    OSCL_EXPORT_REF void PVReleaseInterface(OsclSharedLibraryInterface* aInstance)
    {
        Mpeg4H263EncOmxSharedLibraryInterface* module = (Mpeg4H263EncOmxSharedLibraryInterface*)aInstance;
        OSCL_DELETE(module);
    }
}

#endif

void OmxComponentMpeg4EncAO::SetEncoderMode(OMX_S32 aMode)
{
    iEncMode = aMode;
}


OSCL_EXPORT_REF OMX_ERRORTYPE OmxComponentMpeg4EncAO::OpenmaxMpeg4EncAOAllocateBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE** pBuffer,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes)
{
    OmxComponentMpeg4EncAO* pOpenmaxAOType = (OmxComponentMpeg4EncAO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    OMX_ERRORTYPE Status;

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorBadParameter;
    }

   Status = pOpenmaxAOType->AllocateBuffer(hComponent, pBuffer, nPortIndex, pAppPrivate, nSizeBytes);

    return Status;
}

OMX_ERRORTYPE OmxComponentMpeg4EncAO::AllocateBuffer(
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
	    if (OMX_DirOutput == pBaseComponentPort->PortParam.eDir)
	    {
	        pBaseComponentPort->pBuffer[ii]->pBuffer = (OMX_BYTE) oscl_malloc(nSizeBytes);
	    }else
	    {
               // pBaseComponentPort->pBuffer[ii]->pBuffer = (OMX_BYTE) oscl_malloc(nSizeBytes);	
                int  nSizeBytes_64word = (nSizeBytes+ 1024*4 - 1) & ~(1024*4 - 1);
	       if(iStoreMetaDataBuffer)
	       {
	       		nSizeBytes_64word = 1024*4;
			OMX_MP4ENC_ERR("OmxComponentMpeg4EncAO::AllocateBuffer StoreMetaDataBuffer");
	       }
		   
	    	if(iNumberOfPmemBuffers==0)
	    	{
	    		if(nSizeBytes_64word<=0)
				return OMX_ErrorInsufficientResources;
			//	nSizeBytes_64word = 176*144*3/2;//in stagefight it may be 0
	       		iMemHeapBae =  new MemoryHeapIon(SPRD_ION_DEV, nSizeBytes_64word*pBaseComponentPort->PortParam.nBufferCountActual,MemoryHeapBase::NO_CACHING, ION_HEAP_CARVEOUT_MASK);
			iIonHeap = iMemHeapBae;
			int fd =iIonHeap->getHeapID();
			if(fd>=0){
				int ret,phy_addr, buffer_size;
				ret = iIonHeap->get_phy_addr_from_ion(&phy_addr, &buffer_size);
				if(ret) OMX_MP4ENC_ERR("get_phy_addr_from_ion fail %d",ret);	
				iPmemBasePhy = phy_addr;
				iPmemBase  =(uint32) iIonHeap->base();
				OMX_MP4ENC_ERR("OmxComponentMpeg4EncAO::AllocateBuffer pmemmpool num = %d,size = %d:%x,%x,%x,%x\n",pBaseComponentPort->PortParam.nBufferCountActual,nSizeBytes_64word,iIonHeap->getHeapID(),iPmemBase,phy_addr,buffer_size);				
			}
			else
			{
				OMX_MP4ENC_ERR("OmxComponentMpeg4EncAO::AllocateBuffer pmempool error %d %d\n",nSizeBytes_64word,pBaseComponentPort->PortParam.nBufferCountActual);
				return OMX_ErrorInsufficientResources;
			}
	    	}
			
	        if(iNumberOfPmemBuffers>=pBaseComponentPort->PortParam.nBufferCountActual)
	        {       
	        	OMX_MP4ENC_ERR("OmxComponentMpeg4EncAO::AllocateBuffer iNumberOfPmemHeaps>=nBufferCountActual %d %d\n",iNumberOfPmemBuffers,pBaseComponentPort->PortParam.nBufferCountActual);
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
	     if (OMX_DirOutput == pBaseComponentPort->PortParam.eDir)
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
		 OMX_MP4ENC_ERR("pInfo->offset %x",pInfo->offset);
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
OSCL_EXPORT_REF OMX_ERRORTYPE OmxComponentMpeg4EncAO::OpenmaxMpeg4EncAOFreeBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_U32 nPortIndex,
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    OmxComponentMpeg4EncAO* pOpenmaxAOType = (OmxComponentMpeg4EncAO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    OMX_ERRORTYPE Status;

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorBadParameter;
    }

    Status = pOpenmaxAOType->FreeBuffer(hComponent, nPortIndex, pBuffer);

    return Status;
}
OMX_ERRORTYPE OmxComponentMpeg4EncAO::FreeBuffer(
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
             if (OMX_DirOutput == pBaseComponentPort->PortParam.eDir)
	     {
	    	 oscl_free(pBuffer->pBuffer);
             }else
             {
                if( iNumberOfPmemBuffers)
                {
               	        iNumberOfPmemBuffers = 0;
			if(iIonHeap!=NULL)
				iIonHeap.clear();
			OMX_MP4ENC_ERR("OpenmaxMpeg4AO::FreeBuffer pmempool clear\n");
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

 uint32 OmxComponentMpeg4EncAO:: FindPhyAddr(uint32 Vaddr)
 {
	return Vaddr - iPmemBase + iPmemBasePhy;
 }

OMX_COLOR_FORMATTYPE OmxComponentMpeg4EncAO:: Get_eColorFormat_of_input()
{
    ComponentPortType *pOutPort = (ComponentPortType*) ipPorts[OMX_PORT_INPUTPORT_INDEX];
        return pOutPort->PortParam.format.video.eColorFormat;
}

OMX_ERRORTYPE OmxComponentMpeg4EncAO::ConstructComponent(OMX_PTR pAppData, OMX_PTR pProxy)
{
    ComponentPortType *pInPort, *pOutPort;
    OMX_ERRORTYPE Status;

    iNumberOfPmemBuffers = 0;
    iNumPorts = 2;
    iCompressedFormatPortNum = OMX_PORT_OUTPUTPORT_INDEX;
    iOmxComponent.nSize = sizeof(OMX_COMPONENTTYPE);
    iOmxComponent.pComponentPrivate = (OMX_PTR) this;  // pComponentPrivate points to THIS component AO class
    ipComponentProxy = pProxy;
    iOmxComponent.pApplicationPrivate = pAppData; // init the App data

    iStoreMetaDataBuffer = OMX_FALSE;

#if PROXY_INTERFACE
    iPVCapabilityFlags.iIsOMXComponentMultiThreaded = OMX_TRUE;

    iOmxComponent.SendCommand = OmxComponentMpeg4EncAO::BaseComponentProxySendCommand;
    iOmxComponent.GetParameter = OmxComponentMpeg4EncAO::BaseComponentProxyGetParameter;
    iOmxComponent.SetParameter = OmxComponentMpeg4EncAO::BaseComponentProxySetParameter;
    iOmxComponent.GetConfig = OmxComponentMpeg4EncAO::BaseComponentProxyGetConfig;
    iOmxComponent.SetConfig = OmxComponentMpeg4EncAO::BaseComponentProxySetConfig;
    iOmxComponent.GetExtensionIndex = OmxComponentMpeg4EncAO::BaseComponentProxyGetExtensionIndex;
    iOmxComponent.GetState = OmxComponentMpeg4EncAO::BaseComponentProxyGetState;
    iOmxComponent.UseBuffer = OmxComponentMpeg4EncAO::BaseComponentProxyUseBuffer;
    iOmxComponent.AllocateBuffer = OmxComponentMpeg4EncAO::BaseComponentProxyAllocateBuffer;
    iOmxComponent.FreeBuffer = OmxComponentMpeg4EncAO::BaseComponentProxyFreeBuffer;
    iOmxComponent.EmptyThisBuffer = OmxComponentMpeg4EncAO::BaseComponentProxyEmptyThisBuffer;
    iOmxComponent.FillThisBuffer = OmxComponentMpeg4EncAO::BaseComponentProxyFillThisBuffer;

#else
    iPVCapabilityFlags.iIsOMXComponentMultiThreaded = OMX_FALSE;

    iOmxComponent.SendCommand = OmxComponentMpeg4EncAO::BaseComponentSendCommand;
    iOmxComponent.GetParameter = OmxComponentMpeg4EncAO::BaseComponentGetParameter;
    iOmxComponent.SetParameter = OmxComponentMpeg4EncAO::BaseComponentSetParameter;
    iOmxComponent.GetConfig = OmxComponentMpeg4EncAO::BaseComponentGetConfig;
    iOmxComponent.SetConfig = OmxComponentMpeg4EncAO::BaseComponentSetConfig;
    iOmxComponent.GetExtensionIndex = OmxComponentMpeg4EncAO::BaseComponentGetExtensionIndex;
    iOmxComponent.GetState = OmxComponentMpeg4EncAO::BaseComponentGetState;
    iOmxComponent.UseBuffer = OmxComponentMpeg4EncAO::BaseComponentUseBuffer;
    iOmxComponent.AllocateBuffer = OmxComponentMpeg4EncAO::BaseComponentAllocateBuffer;
    iOmxComponent.FreeBuffer = OmxComponentMpeg4EncAO::BaseComponentFreeBuffer;
    iOmxComponent.EmptyThisBuffer = OmxComponentMpeg4EncAO::BaseComponentEmptyThisBuffer;
    iOmxComponent.FillThisBuffer = OmxComponentMpeg4EncAO::BaseComponentFillThisBuffer;
#endif

    iOmxComponent.SetCallbacks = OmxComponentMpeg4EncAO::BaseComponentSetCallbacks;
    iOmxComponent.nVersion.s.nVersionMajor = SPECVERSIONMAJOR;
    iOmxComponent.nVersion.s.nVersionMinor = SPECVERSIONMINOR;
    iOmxComponent.nVersion.s.nRevision = SPECREVISION;
    iOmxComponent.nVersion.s.nStep = SPECSTEP;

    // PV capability
    iPVCapabilityFlags.iOMXComponentSupportsExternalInputBufferAlloc = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentSupportsExternalOutputBufferAlloc = OMX_TRUE;
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

    /** Domain specific section for input raw port */ //OMX_PARAM_PORTDEFINITIONTYPE
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.nPortIndex = OMX_PORT_INPUTPORT_INDEX;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.eDomain = OMX_PortDomainVideo;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.cMIMEType = (OMX_STRING)"raw";
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.pNativeRender = 0;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.bFlagErrorConcealment = OMX_FALSE;

    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.nFrameWidth = 176;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.nFrameHeight = 144;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.nBitrate = 64000;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.xFramerate = (15 << 16);
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.eDir = OMX_DirInput;
    //Set to a default value, will change later during setparameter call
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.nBufferCountActual = 2;//NUMBER_INPUT_BUFFER_MP4ENC;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.nBufferCountMin = 1;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.nBufferSize = INPUT_BUFFER_SIZE_MP4ENC;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.bEnabled = OMX_TRUE;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.bPopulated = OMX_FALSE;


    /** Domain specific section for output mpeg4/h263 port */
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.nPortIndex = OMX_PORT_OUTPUTPORT_INDEX;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.eDomain = OMX_PortDomainVideo;
    if (MODE_MPEG4 == iEncMode)
    {
        ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.cMIMEType = (OMX_STRING)"video/mpeg4";
        ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG4;

    }
    else if (MODE_H263 == iEncMode)
    {
        ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.cMIMEType = (OMX_STRING)"video/h263";
        ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.eCompressionFormat = OMX_VIDEO_CodingH263;
    }

    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.pNativeRender = 0;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.bFlagErrorConcealment = OMX_FALSE;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.nFrameWidth = 176;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.nFrameHeight = 144;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.nBitrate = 64000;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.xFramerate = (15 << 16);
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.eDir = OMX_DirOutput;
    //Set to a default value, will change later during setparameter call
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.nBufferCountActual = NUMBER_OUTPUT_BUFFER_MP4ENC;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.nBufferCountMin = 1;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.nBufferSize = OUTPUT_BUFFER_SIZE_MP4ENC;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.bEnabled = OMX_TRUE;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.bPopulated = OMX_FALSE;


    //OMX_VIDEO_PARAM_MPEG4TYPE
    //Default values for mpeg4 video output param port
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->VideoMpeg4.nPortIndex = OMX_PORT_OUTPUTPORT_INDEX;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->VideoMpeg4.eProfile = OMX_VIDEO_MPEG4ProfileCore;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->VideoMpeg4.eLevel = OMX_VIDEO_MPEG4Level2;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->VideoMpeg4.nPFrames = 10;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->VideoMpeg4.nBFrames = 0;        //No support for B frames
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->VideoMpeg4.nMaxPacketSize = 256;    //Default value
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->VideoMpeg4.nAllowedPictureTypes = OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->VideoMpeg4.bGov = OMX_FALSE;

    //This will be an additional structure maintained on output port if the encoder is H.263
    if (MODE_H263 == iEncMode)
    {
        ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->VideoH263.nPortIndex = OMX_PORT_OUTPUTPORT_INDEX;
        ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->VideoH263.eProfile = OMX_VIDEO_H263ProfileBaseline;
        ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->VideoH263.eLevel = OMX_VIDEO_H263Level45;
        ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->VideoH263.bPLUSPTYPEAllowed = OMX_FALSE;
        ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->VideoH263.nAllowedPictureTypes = OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP;
        ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->VideoH263.bForceRoundingTypeToZero = OMX_TRUE;
        ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->VideoH263.nPictureHeaderRepetition = 0;
        ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->VideoH263.nGOBHeaderInterval = 2; //one can change it by SetGOBHdrInterval in pvomxencnode
    }


    if (MODE_MPEG4 == iEncMode)
    {
        //OMX_VIDEO_PARAM_PROFILELEVELTYPE structure
        ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->ProfileLevel.nPortIndex = OMX_PORT_OUTPUTPORT_INDEX;
        ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->ProfileLevel.nProfileIndex = 0;
        ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->ProfileLevel.eProfile = OMX_VIDEO_MPEG4ProfileCore;
        ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->ProfileLevel.eLevel = OMX_VIDEO_MPEG4Level2;
    }
    else if (MODE_H263 == iEncMode)
    {
        ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->ProfileLevel.nPortIndex = OMX_PORT_OUTPUTPORT_INDEX;
        ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->ProfileLevel.nProfileIndex = 0;
        ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->ProfileLevel.eProfile = OMX_VIDEO_H263ProfileBaseline;
        ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->ProfileLevel.eLevel = OMX_VIDEO_H263Level45;
    }


    iPortTypesParam.nPorts = 2;
    iPortTypesParam.nStartPortNumber = 0;

    pInPort = (ComponentPortType*) ipPorts[OMX_PORT_INPUTPORT_INDEX];
    pOutPort = (ComponentPortType*) ipPorts[OMX_PORT_OUTPUTPORT_INDEX];

    pInPort->ActualNumPortFormatsSupported = 2;

    //OMX_VIDEO_PARAM_PORTFORMATTYPE INPUT PORT SETTINGS
    //On input port for index 0
    SetHeader(&pInPort->VideoParam[0], sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
    pInPort->VideoParam[0].nPortIndex = OMX_PORT_INPUTPORT_INDEX;
    pInPort->VideoParam[0].nIndex = 0;
    pInPort->VideoParam[0].eCompressionFormat = OMX_VIDEO_CodingUnused;
    pInPort->VideoParam[0].eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
    pInPort->VideoParam[0].xFramerate = (15 << 16);

    //On input port for index 1
    SetHeader(&pInPort->VideoParam[1], sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
    pInPort->VideoParam[1].nPortIndex = OMX_PORT_INPUTPORT_INDEX;
    pInPort->VideoParam[1].nIndex = 1;
    pInPort->VideoParam[1].eCompressionFormat = OMX_VIDEO_CodingUnused;
    pInPort->VideoParam[1].eColorFormat = OMX_COLOR_FormatAndroidOpaque;
    pInPort->VideoParam[1].xFramerate = (15 << 16);

    pOutPort->ActualNumPortFormatsSupported = 1;

    //OMX_VIDEO_PARAM_PORTFORMATTYPE OUTPUT PORT SETTINGS
    //On output port for index 0
    SetHeader(&pOutPort->VideoParam[0], sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
    pOutPort->VideoParam[0].nPortIndex = OMX_PORT_OUTPUTPORT_INDEX;
    pOutPort->VideoParam[0].nIndex = 0;
    pOutPort->VideoParam[0].eColorFormat = OMX_COLOR_FormatUnused;
    pOutPort->VideoParam[0].xFramerate = (15 << 16);
    if (iEncMode == MODE_MPEG4)
    {
        pOutPort->VideoParam[0].eCompressionFormat = OMX_VIDEO_CodingMPEG4;
        oscl_strncpy((OMX_STRING)iComponentRole, (OMX_STRING)"video_encoder.mpeg4", OMX_MAX_STRINGNAME_SIZE);

    }
    else if (iEncMode == MODE_H263)
    {
        pOutPort->VideoParam[0].eCompressionFormat = OMX_VIDEO_CodingH263;
        oscl_strncpy((OMX_STRING)iComponentRole, (OMX_STRING)"video_encoder.h263", OMX_MAX_STRINGNAME_SIZE);
    }




    //OMX_CONFIG_ROTATIONTYPE SETTINGS ON INPUT PORT
    SetHeader(&pInPort->VideoOrientationType, sizeof(OMX_CONFIG_ROTATIONTYPE));
    pInPort->VideoOrientationType.nPortIndex = OMX_PORT_INPUTPORT_INDEX;
    pInPort->VideoOrientationType.nRotation = -1;  //For all the YUV formats that are other than RGB


    //OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE settings of output port
    oscl_memset(&pOutPort->VideoErrorCorrection, 0, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
    SetHeader(&pOutPort->VideoErrorCorrection, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
    pOutPort->VideoErrorCorrection.nPortIndex = OMX_PORT_OUTPUTPORT_INDEX;
    pOutPort->VideoErrorCorrection.bEnableDataPartitioning = OMX_FALSE; //As in node default is h263


    //OMX_VIDEO_PARAM_BITRATETYPE settings of output port
    SetHeader(&pOutPort->VideoRateType, sizeof(OMX_VIDEO_PARAM_BITRATETYPE));
    pOutPort->VideoRateType.nPortIndex = OMX_PORT_OUTPUTPORT_INDEX;
    pOutPort->VideoRateType.eControlRate = OMX_Video_ControlRateConstant;
    pOutPort->VideoRateType.nTargetBitrate = 64000;


    //OMX_VIDEO_PARAM_BITRATETYPE settings of max bit rate,
    // NOTE, we are not using this in encoder setting.
    SetHeader(&pOutPort->VideoMaxRate, sizeof(OMX_VIDEO_PARAM_BITRATETYPE));
    pOutPort->VideoMaxRate.nPortIndex = OMX_PORT_OUTPUTPORT_INDEX;
    pOutPort->VideoMaxRate.eControlRate = OMX_Video_ControlRateMax;
    pOutPort->VideoMaxRate.nTargetBitrate = 64000;


    //OMX_CONFIG_FRAMERATETYPE default seetings (specified in khronos conformance test)
    SetHeader(&pOutPort->VideoConfigFrameRateType, sizeof(OMX_CONFIG_FRAMERATETYPE));
    pOutPort->VideoConfigFrameRateType.nPortIndex = OMX_PORT_OUTPUTPORT_INDEX;
    pOutPort->VideoConfigFrameRateType.xEncodeFramerate = (15 << 16);

    //OMX_VIDEO_CONFIG_BITRATETYPE default settings (specified in khronos conformance test)
    SetHeader(&pOutPort->VideoConfigBitRateType, sizeof(OMX_VIDEO_CONFIG_BITRATETYPE));
    pOutPort->VideoConfigBitRateType.nPortIndex = OMX_PORT_OUTPUTPORT_INDEX;
    pOutPort->VideoConfigBitRateType.nEncodeBitrate = 64000;


    //OMX_VIDEO_PARAM_QUANTIZATIONTYPE settings of output port
    SetHeader(&pOutPort->VideoQuantType, sizeof(OMX_VIDEO_PARAM_QUANTIZATIONTYPE));
    pOutPort->VideoQuantType.nPortIndex = OMX_PORT_OUTPUTPORT_INDEX;
    pOutPort->VideoQuantType.nQpI = 6;
    pOutPort->VideoQuantType.nQpP = 6;
    pOutPort->VideoQuantType.nQpB = 6;


    //OMX_VIDEO_PARAM_VBSMCTYPE settings of output port
    oscl_memset(&pOutPort->VideoBlockMotionSize, 0, sizeof(OMX_VIDEO_PARAM_VBSMCTYPE));
    SetHeader(&pOutPort->VideoBlockMotionSize, sizeof(OMX_VIDEO_PARAM_VBSMCTYPE));
    pOutPort->VideoBlockMotionSize.nPortIndex = OMX_PORT_OUTPUTPORT_INDEX;
    pOutPort->VideoBlockMotionSize.b16x16 = OMX_TRUE;


    //OMX_VIDEO_PARAM_MOTIONVECTORTYPE settings of output port
    oscl_memset(&pOutPort->VideoMotionVector, 0, sizeof(OMX_VIDEO_PARAM_MOTIONVECTORTYPE));
    SetHeader(&pOutPort->VideoMotionVector, sizeof(OMX_VIDEO_PARAM_MOTIONVECTORTYPE));
    pOutPort->VideoMotionVector.nPortIndex = OMX_PORT_OUTPUTPORT_INDEX;
    pOutPort->VideoMotionVector.eAccuracy = OMX_Video_MotionVectorHalfPel;
    pOutPort->VideoMotionVector.bUnrestrictedMVs = OMX_TRUE;
    pOutPort->VideoMotionVector.sXSearchRange = 16;
    pOutPort->VideoMotionVector.sYSearchRange = 16;


    //OMX_VIDEO_PARAM_INTRAREFRESHTYPE settings of output port
    oscl_memset(&pOutPort->VideoIntraRefresh, 0, sizeof(OMX_VIDEO_PARAM_INTRAREFRESHTYPE));
    SetHeader(&pOutPort->VideoIntraRefresh, sizeof(OMX_VIDEO_PARAM_INTRAREFRESHTYPE));
    pOutPort->VideoIntraRefresh.nPortIndex = OMX_PORT_OUTPUTPORT_INDEX;
    pOutPort->VideoIntraRefresh.eRefreshMode = OMX_VIDEO_IntraRefreshCyclic;
    pOutPort->VideoIntraRefresh.nCirMBs = 0;


    //OMX_CONFIG_INTRAREFRESHVOPTYPE settings of output port
    oscl_memset(&pOutPort->VideoIFrame, 0, sizeof(OMX_CONFIG_INTRAREFRESHVOPTYPE));
    SetHeader(&pOutPort->VideoIFrame, sizeof(OMX_CONFIG_INTRAREFRESHVOPTYPE));
    pOutPort->VideoIFrame.nPortIndex = OMX_PORT_OUTPUTPORT_INDEX;
    pOutPort->VideoIFrame.IntraRefreshVOP = OMX_FALSE;

    iOutputBufferSizeAfterInit = OUTPUT_BUFFER_SIZE_MP4ENC;

    //Construct the encoder object
    if (ipMpegEncoderObject)
    {
        OSCL_DELETE(ipMpegEncoderObject);
        ipMpegEncoderObject = NULL;
    }

    ipMpegEncoderObject = OSCL_NEW(Mpeg4Encoder_OMX, ((OmxComponentBase*)this));

#if PROXY_INTERFACE

    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentSendCommand = BaseComponentSendCommand;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentGetParameter = BaseComponentGetParameter;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentSetParameter = BaseComponentSetParameter;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentGetConfig = BaseComponentGetConfig;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentSetConfig = BaseComponentSetConfig;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentGetExtensionIndex = OpenmaxMpeg4EncAOGetExtensionIndex;//BaseComponentGetExtensionIndex;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentGetState = BaseComponentGetState;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentUseBuffer = BaseComponentUseBuffer;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentAllocateBuffer = OpenmaxMpeg4EncAOAllocateBuffer;//BaseComponentAllocateBuffer;////modified by jgdu
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentFreeBuffer = OpenmaxMpeg4EncAOFreeBuffer; //BaseComponentFreeBuffer;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentEmptyThisBuffer = BaseComponentEmptyThisBuffer;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentFillThisBuffer = BaseComponentFillThisBuffer;

#endif

    return OMX_ErrorNone;
}


OSCL_EXPORT_REF OMX_ERRORTYPE OmxComponentMpeg4EncAO::GetParameter(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nParamIndex,
    OMX_INOUT OMX_PTR ComponentParameterStructure)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
       OmxComponentMpeg4EncAO* pOpenmaxAOType = (OmxComponentMpeg4EncAO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
	//OMX_MP4ENC_ERR("OmxComponentMpeg4EncAO::GetParameter");	   
    	if (NULL == pOpenmaxAOType)
    	{
            return OMX_ErrorBadParameter;
    	}
       switch (nParamIndex) {
	    case OMX_IndexParamStoreMetaDataBuffer:
	    {	
			StoreMetaDataInBuffersParams *pStoreMetaData = (StoreMetaDataInBuffersParams *)ComponentParameterStructure;			
			pStoreMetaData->bStoreMetaData = iStoreMetaDataBuffer;
			OMX_MP4ENC_ERR("OmxComponentMpeg4EncAO::GetParameter OMX_IndexParamStoreMetaDataBuffer %d",pStoreMetaData->bStoreMetaData);
	    }
			break;
	    default:
			ret = OmxComponentVideo::GetParameter(hComponent,nParamIndex,ComponentParameterStructure);
			break;
       }
	
	return ret;
}

OSCL_EXPORT_REF OMX_ERRORTYPE OmxComponentMpeg4EncAO::SetParameter(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nParamIndex,
    OMX_IN  OMX_PTR ComponentParameterStructure)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
       OmxComponentMpeg4EncAO* pOpenmaxAOType = (OmxComponentMpeg4EncAO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
	//OMX_MP4ENC_ERR("OmxComponentMpeg4EncAO::SetParameter");	
    	if (NULL == pOpenmaxAOType)
    	{
            return OMX_ErrorBadParameter;
    	}	
	switch (nParamIndex) {
	    case OMX_IndexParamStoreMetaDataBuffer:
	    {
			StoreMetaDataInBuffersParams *pStoreMetaData = (StoreMetaDataInBuffersParams *)ComponentParameterStructure;
			ComponentPortType *pOutPort = (ComponentPortType*) ipPorts[OMX_PORT_INPUTPORT_INDEX];			
			if (pStoreMetaData->bStoreMetaData == OMX_FALSE) {
        			OMX_MP4ENC_ERR("OmxComponentMpeg4EncAO::disable StoreMetaData");
        			iStoreMetaDataBuffer = OMX_FALSE;
    				pOutPort->PortParam.format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)OMX_COLOR_FormatYUV420SemiPlanar;				
    			} else {
        			OMX_MP4ENC_ERR("OmxComponentMpeg4EncAO::enable StoreMetaData");
        			iStoreMetaDataBuffer = OMX_TRUE;
    				pOutPort->PortParam.format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)OMX_COLOR_FormatAndroidOpaque;
    			}  			
	    }	
			break;
	    default:
			ret = OmxComponentVideo::SetParameter(hComponent,nParamIndex,ComponentParameterStructure);
			break;
	}
	return ret;	
}

OSCL_EXPORT_REF OMX_ERRORTYPE OmxComponentMpeg4EncAO::OpenmaxMpeg4EncAOGetExtensionIndex(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_STRING cParameterName,
    OMX_OUT OMX_INDEXTYPE* pIndexType)
{
    OmxComponentMpeg4EncAO* pOpenmaxAOType = (OmxComponentMpeg4EncAO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorBadParameter;
    }
	
    if(strcmp(cParameterName, SPRD_INDEX_PARAM_STORE_METADATA_BUFFER) == 0)
    {
    		OMX_MP4ENC_ERR("OmxComponentMpeg4EncAO::%s",SPRD_INDEX_PARAM_STORE_METADATA_BUFFER);
		*pIndexType = (OMX_INDEXTYPE) OMX_IndexParamStoreMetaDataBuffer;
		return OMX_ErrorNone;
    }
	
    return OMX_ErrorNotImplemented;
}

/** This function is called by the omx core when the component
    * is disposed by the IL client with a call to FreeHandle().
    */

OMX_ERRORTYPE OmxComponentMpeg4EncAO::DestroyComponent()
{
    if (OMX_FALSE != iIsInit)
    {
        ComponentDeInit();
    }

    //Destroy the base class now
    DestroyBaseComponent();

    if (ipMpegEncoderObject)
    {
        OSCL_DELETE(ipMpegEncoderObject);
        ipMpegEncoderObject = NULL;
    }

    if (ipAppPriv)
    {
        ipAppPriv->CompHandle = NULL;

        oscl_free(ipAppPriv);
        ipAppPriv = NULL;
    }

    return OMX_ErrorNone;
}


void OmxComponentMpeg4EncAO::ProcessData()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentMpeg4EncAO : ProcessData IN"));

    QueueType* pInputQueue = ipPorts[OMX_PORT_INPUTPORT_INDEX]->pBufferQueue;
    QueueType* pOutputQueue = ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->pBufferQueue;

    ComponentPortType*  pInPort = ipPorts[OMX_PORT_INPUTPORT_INDEX];
    ComponentPortType*  pOutPort = ipPorts[OMX_PORT_OUTPUTPORT_INDEX];

    OMX_U8*                 pOutBuffer;
    OMX_U32                 OutputLength;
    OMX_BOOL                EncodeReturn = OMX_FALSE;
    OMX_COMPONENTTYPE*      pHandle = &iOmxComponent;


    if ((!iIsInputBufferEnded) || (iEndofStream) ||
            ((OMX_TRUE == iSendVolHeaderFlag) && (MODE_MPEG4 == iEncMode)))
    {
        //Send port reconfiguration callback for encoder components if required
        if (OMX_TRUE == iPortReconfigurationNeeded)
        {
            ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.nBufferSize = iOutputBufferSizeAfterInit;

            iResizePending = OMX_TRUE;

            (*(ipCallbacks->EventHandler))
            (pHandle,
             iCallbackData,
             OMX_EventPortSettingsChanged, //The command was completed
             OMX_PORT_OUTPUTPORT_INDEX,
             0,
             NULL);

            iPortReconfigurationNeeded = OMX_FALSE;

            return;
        }

        //Check whether prev output bufer has been released or not
        if (OMX_TRUE == iNewOutBufRequired)
        {
            //Check whether a new output buffer is available or not

            if (0 == (GetQueueNumElem(pOutputQueue)))
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentMpeg4EncAO : ProcessData OUT output buffer unavailable"));
                return;
            }

            ipOutputBuffer = (OMX_BUFFERHEADERTYPE*) DeQueue(pOutputQueue);

            OSCL_ASSERT(NULL != ipOutputBuffer);
            if (ipOutputBuffer == NULL)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "OmxComponentMpeg4EncAO : ProcessData OUT ERR output buffer cannot be dequeued"));
                return;
            }

            ipOutputBuffer->nFilledLen = 0;
            iNewOutBufRequired = OMX_FALSE;


            /* If some output data was left to be send from the last processing due to
             * unavailability of required number of output buffers,
             * copy it now and send back before processing new input frame */
            if (iInternalOutBufFilledLen > 0)
            {
                if (OMX_FALSE == CopyDataToOutputBuffer())
                {
                    //We fell short of output buffers, exit now and wait for some more buffers to get queued
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentMpeg4EncAO : ProcessData OUT output buffer unavailable"));
                    return;
                }
                else
                {
                    //Attach the end of frame flag while sending out the last piece of output buffer
                    ipOutputBuffer->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
                    if (OMX_TRUE == iSyncFlag)
                    {
                        ipOutputBuffer->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;
                        iSyncFlag = OMX_FALSE;
                    }
                    ReturnOutputBuffer(ipOutputBuffer, pOutPort);

                    //Dequeue new output buffer to continue encoding the next frame
                    if (0 == (GetQueueNumElem(pOutputQueue)))
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentMpeg4EncAO : ProcessData OUT output buffer unavailable"));
                        return;
                    }

                    ipOutputBuffer = (OMX_BUFFERHEADERTYPE*) DeQueue(pOutputQueue);

                    OSCL_ASSERT(NULL != ipOutputBuffer);
                    if (ipOutputBuffer == NULL)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "OmxComponentMpeg4EncAO : ProcessData OUT ERR output buffer cannot be dequeued"));
                        return;
                    }

                    ipOutputBuffer->nFilledLen = 0;
                    iNewOutBufRequired = OMX_FALSE;
                }
            }
        }

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

        //Send the VOL Header in the beginning before processing any input frames
        if ((OMX_TRUE == iSendVolHeaderFlag) && (MODE_MPEG4 == iEncMode))
        {
            EncodeReturn = ipMpegEncoderObject->Mp4GetVolHeader(ipOutputBuffer->pBuffer, &(ipOutputBuffer->nFilledLen));

            if (OMX_TRUE == EncodeReturn)
            {
                iSendVolHeaderFlag = OMX_FALSE;
                ipOutputBuffer->nOffset = 0;
                ipOutputBuffer->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME|OMX_BUFFERFLAG_CODECCONFIG;//jgdu
                ReturnOutputBuffer(ipOutputBuffer, pOutPort);
            }

            return;
        }

        //Call the encoder only if there is some data to encode
        if (iInputCurrLength > 0)
        {
            pOutBuffer = ipOutputBuffer->pBuffer;
            OutputLength = ipOutputBuffer->nAllocLen;

            //Output buffer is passed as a short pointer
            EncodeReturn = ipMpegEncoderObject->Mp4EncodeVideo(pOutBuffer,
                           &OutputLength,
                           &iBufferOverRun,
                           &ipInternalOutBuffer,
                           ipFrameDecodeBuffer,
                           (OMX_U8*)ipInputBuffer->pPlatformPrivate,
                           iInputCurrLength,
                           iFrameTimestamp,
                           &iOutputTimeStamp,
                           &iSyncFlag);


            //Chk whether output data has been generated or not
            if (OutputLength > 0)
            {
                //offset not required in our case, set it to zero
                ipOutputBuffer->nOffset = 0;
                ipOutputBuffer->nTimeStamp = iOutputTimeStamp;

                if (OMX_FALSE == iBufferOverRun)
                {
                    //No internal buffer is maintained
                    ipOutputBuffer->nFilledLen = OutputLength;
                }
                else
                {
                    iInternalOutBufFilledLen = OutputLength;
                    iBufferOverRun = OMX_FALSE;
                    CopyDataToOutputBuffer();

                }   //else loop of if (OMX_FALSE == iMantainOutInternalBuffer)
            }   //if (OutputLength > 0)  loop


            //If encoder returned error in case of frame skip/corrupt frame, report it to the client via a callback
            if ((OMX_FALSE == EncodeReturn) && (OMX_FALSE == iEndofStream))
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentMpeg4EncAO : Frame skipped, ProcessData ErrorStreamCorrupt callback send"));

                (*(ipCallbacks->EventHandler))
                (pHandle,
                 iCallbackData,
                 OMX_EventError,
                 OMX_ErrorStreamCorrupt,
                 0,
                 NULL);
            }

            //Input bytes consumed now, return the buffer
            ipInputBuffer->nFilledLen = 0;
            ReturnInputBuffer(ipInputBuffer, pInPort);
            ipInputBuffer = NULL;

            iIsInputBufferEnded = OMX_TRUE;
            iInputCurrLength = 0;

            iFrameCount++;
        }


        /* If EOS flag has come from the client & there are no more
         * input buffers to decode, send the callback to the client
         */
        if (OMX_TRUE == iEndofStream)
        {
            if (((0 == iInputCurrLength) || (OMX_FALSE == EncodeReturn)) &&
                    (0 == iInternalOutBufFilledLen))
            {

                (*(ipCallbacks->EventHandler))
                (pHandle,
                 iCallbackData,
                 OMX_EventBufferFlag,
                 1,
                 OMX_BUFFERFLAG_EOS,
                 NULL);
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentMpeg4EncAO : ProcessData EOS callback sent"));


                //Mark this flag false once the callback has been send back
                iEndofStream = OMX_FALSE;

                ipOutputBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
                if (OMX_TRUE == iSyncFlag)
                {
                    ipOutputBuffer->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;
                    iSyncFlag = OMX_FALSE;
                }
                ReturnOutputBuffer(ipOutputBuffer, pOutPort);

                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentMpeg4EncAO : ProcessData OUT"));
                return;
            }

        }

        //Send the output buffer back after decode
//        if ((ipOutputBuffer->nFilledLen > 0) && (OMX_FALSE == iNewOutBufRequired))
        if( OMX_FALSE == iNewOutBufRequired )
        {
            //Attach the end of frame flag while sending out the last piece of output buffer
            ipOutputBuffer->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
            if (OMX_TRUE == iSyncFlag)
            {
                ipOutputBuffer->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;
                iSyncFlag = OMX_FALSE;
            }
            ReturnOutputBuffer(ipOutputBuffer, pOutPort);
        }


        /* If there is some more processing left with current buffers, re-schedule the AO
         * Do not go for more than one round of processing at a time.
         * This may block the AO longer than required.
         */
        if ((iInputCurrLength != 0 || GetQueueNumElem(pInputQueue) > 0)
                && (GetQueueNumElem(pOutputQueue) > 0))
        {
            RunIfNotReady();
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentMpeg4EncAO : ProcessData OUT"));
}


OMX_BOOL OmxComponentMpeg4EncAO::CopyDataToOutputBuffer()
{
    ComponentPortType*  pOutPort = ipPorts[OMX_PORT_OUTPUTPORT_INDEX];
    QueueType* pOutputQueue = ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->pBufferQueue;

    while (iInternalOutBufFilledLen > 0)
    {
        if (ipOutputBuffer->nAllocLen >= iInternalOutBufFilledLen)
        {
            //Pack the whole  data into the output buffer Alloc length data in one buffer and return it
            oscl_memcpy(ipOutputBuffer->pBuffer, ipInternalOutBuffer, iInternalOutBufFilledLen);
            ipOutputBuffer->nFilledLen = iInternalOutBufFilledLen;

        }
        else
        {
            oscl_memcpy(ipOutputBuffer->pBuffer, ipInternalOutBuffer, ipOutputBuffer->nAllocLen);
            ipOutputBuffer->nFilledLen = ipOutputBuffer->nAllocLen;
        }

        iInternalOutBufFilledLen -= ipOutputBuffer->nFilledLen;
        ipInternalOutBuffer += ipOutputBuffer->nFilledLen;


        if (0 != iInternalOutBufFilledLen)
        {
            //Mark sync flag in each piece of partial output buffer
            if (OMX_TRUE == iSyncFlag)
            {
                ipOutputBuffer->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;
            }
            //Return the partial output buffer and try to fetch a new output buffer for filling the remaining data
            ReturnOutputBuffer(ipOutputBuffer, pOutPort);

            //Check whether a new output buffer is available or not
            if (0 == (GetQueueNumElem(pOutputQueue)))
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentMpeg4EncAO : CopyDataToOutputBuffer OUT output buffer unavailable"));
                return OMX_FALSE;
            }

            ipOutputBuffer = (OMX_BUFFERHEADERTYPE*) DeQueue(pOutputQueue);

            OSCL_ASSERT(NULL != ipOutputBuffer);
            if (ipOutputBuffer == NULL)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "OmxComponentMpeg4EncAO : CopyDataToOutputBuffer  OUT ERR output buffer cannot be dequeued"));
                return OMX_FALSE;
            }

            ipOutputBuffer->nFilledLen = 0;
            ipOutputBuffer->nTimeStamp = iOutputTimeStamp;
            ipOutputBuffer->nOffset = 0;
            iNewOutBufRequired = OMX_FALSE;
        }
    }   //while (iInternalOutBufFilledLen > 0)

    return OMX_TRUE;

}




//Component constructor
OmxComponentMpeg4EncAO::OmxComponentMpeg4EncAO()
{
    ipMpegEncoderObject = NULL;
    iEncMode = MODE_H263;
    //iMantainOutInternalBuffer = OMX_FALSE;
    ipInternalOutBuffer = NULL;
    iInternalOutBufFilledLen = 0;
    iSyncFlag = OMX_FALSE;
    iBufferOverRun = OMX_FALSE;
    iPortReconfigurationNeeded = OMX_FALSE;
    iSendVolHeaderFlag = OMX_TRUE;

    if (!IsAdded())
    {
        AddToScheduler();
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentMpeg4EncAO : constructed"));
}


//Active object destructor
OmxComponentMpeg4EncAO::~OmxComponentMpeg4EncAO()
{
    if (IsAdded())
    {
        RemoveFromScheduler();
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentMpeg4EncAO : destructed"));
}


OMX_ERRORTYPE OmxComponentMpeg4EncAO::SetConfig(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nIndex,
    OMX_IN  OMX_PTR pComponentConfigStructure)
{
    OSCL_UNUSED_ARG(hComponent);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentMpeg4EncAO : SetConfig IN"));

    OMX_U32 PortIndex;

    OMX_ERRORTYPE ErrorType = OMX_ErrorNone;
    OMX_CONFIG_INTRAREFRESHVOPTYPE* pVideoIFrame;
    OMX_VIDEO_CONFIG_BITRATETYPE* pBitRateType;
    OMX_CONFIG_FRAMERATETYPE* pFrameRateType;
    OMX_VIDEO_CONFIG_AVCINTRAPERIOD* pM4vIntraPeriod;


    if (NULL == pComponentConfigStructure)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentMpeg4EncAO : SetConfig error bad parameter"));
        return OMX_ErrorBadParameter;
    }

    switch (nIndex)
    {
        case OMX_IndexConfigVideoIntraVOPRefresh:
        {
            pVideoIFrame = (OMX_CONFIG_INTRAREFRESHVOPTYPE*) pComponentConfigStructure;
            PortIndex = pVideoIFrame->nPortIndex;

            if (PortIndex != iCompressedFormatPortNum)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentMpeg4EncAO : SetConfig error invalid port index"));
                return OMX_ErrorBadPortIndex;
            }

            /*Check Structure Header*/
            ErrorType = CheckHeader(pVideoIFrame, sizeof(OMX_CONFIG_INTRAREFRESHVOPTYPE));
            if (ErrorType != OMX_ErrorNone)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentMpeg4EncAO : SetConfig error param check failed"));
                return ErrorType;
            }

            //Call the RequestI frame routine of the encoder in case of setconfig call
            if (OMX_TRUE == pVideoIFrame->IntraRefreshVOP)
            {
                ipMpegEncoderObject->Mp4RequestIFrame();

            }
        }
        break;

        case OMX_IndexConfigVideoBitrate:
        {
            pBitRateType = (OMX_VIDEO_CONFIG_BITRATETYPE*) pComponentConfigStructure;
            PortIndex = pBitRateType->nPortIndex;

            if (PortIndex != iCompressedFormatPortNum)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentMpeg4EncAO : SetConfig error invalid port index"));
                return OMX_ErrorBadPortIndex;
            }

            /*Check Structure Header*/
            ErrorType = CheckHeader(pBitRateType, sizeof(OMX_VIDEO_CONFIG_BITRATETYPE));
            if (ErrorType != OMX_ErrorNone)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentMpeg4EncAO : SetConfig error param check failed"));
                return ErrorType;
            }

            //Call the corresponding routine of the encoder in case of setconfig call
            if (OMX_FALSE == (ipMpegEncoderObject->Mp4UpdateBitRate(pBitRateType->nEncodeBitrate)))
            {
                return OMX_ErrorUnsupportedSetting;
            }
            ipPorts[PortIndex]->VideoConfigBitRateType.nEncodeBitrate = pBitRateType->nEncodeBitrate;
        }
        break;

        case OMX_IndexConfigVideoFramerate:
        {
            pFrameRateType = (OMX_CONFIG_FRAMERATETYPE*) pComponentConfigStructure;
            PortIndex = pFrameRateType->nPortIndex;

            if (PortIndex != iCompressedFormatPortNum)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentMpeg4EncAO : SetConfig error invalid port index"));
                return OMX_ErrorBadPortIndex;
            }

            /*Check Structure Header*/
            ErrorType = CheckHeader(pFrameRateType, sizeof(OMX_CONFIG_FRAMERATETYPE));
            if (ErrorType != OMX_ErrorNone)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentMpeg4EncAO : SetConfig error param check failed"));
                return ErrorType;
            }

            //Call the corresponding routine of the encoder in case of setconfig call
            if (OMX_FALSE == (ipMpegEncoderObject->Mp4UpdateFrameRate(pFrameRateType->xEncodeFramerate)))
            {
                return OMX_ErrorUnsupportedSetting;
            }

            ipPorts[PortIndex]->VideoConfigFrameRateType.xEncodeFramerate = pFrameRateType->xEncodeFramerate;
        }
        break;

        /* Using OMX_IndexConfigVideoAVCIntraPeriod for M4V until standard defines some struct for it*/
        case OMX_IndexConfigVideoAVCIntraPeriod:
        {
            pM4vIntraPeriod = (OMX_VIDEO_CONFIG_AVCINTRAPERIOD*) pComponentConfigStructure;
            PortIndex = pM4vIntraPeriod->nPortIndex;

            if (PortIndex != iCompressedFormatPortNum)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentAvcEncAO : SetConfig error invalid port index"));
                return OMX_ErrorBadPortIndex;
            }

            /*Check Structure Header*/
            ErrorType = CheckHeader(pM4vIntraPeriod, sizeof(OMX_VIDEO_CONFIG_AVCINTRAPERIOD));
            if (ErrorType != OMX_ErrorNone)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentAvcEncAO : SetConfig error param check failed"));
                return ErrorType;
            }

            //Call the corresponding routine of the encoder in case of setconfig call
            if (OMX_FALSE == (ipMpegEncoderObject->Mp4UpdateIFrameInterval(pM4vIntraPeriod->nPFrames + 1)))
            {
                return OMX_ErrorBadParameter;
            }

            ipPorts[PortIndex]->AvcIntraPeriod.nPFrames = pM4vIntraPeriod->nPFrames;
        }
        break;

        default:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentMpeg4EncAO : SetConfig error Unsupported Index"));
            return OMX_ErrorUnsupportedIndex;
        }
        break;

    }


    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentMpeg4EncAO : SetConfig OUT"));

    return OMX_ErrorNone;

}



/** The Initialization function
 */
OMX_ERRORTYPE OmxComponentMpeg4EncAO::ComponentInit()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentMpeg4EncAO : ComponentInit IN"));

    OMX_ERRORTYPE Status = OMX_ErrorNone;

    if (OMX_TRUE == iIsInit)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentMpeg4EncAO : ComponentInit error incorrect operation"));
        return OMX_ErrorIncorrectStateOperation;
    }
    iIsInit = OMX_TRUE;


    if (!iCodecReady)
    {
        iCodecReady = OMX_TRUE;
    }

    //Check the H.263 parameters before initializing the encoder if there was any change in the SetParameter call
    if (MODE_H263 == iEncMode)
    {
        OMX_VIDEO_PARAM_H263TYPE* H263Param = (OMX_VIDEO_PARAM_H263TYPE*) & ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->VideoH263;

        if ((OMX_VIDEO_H263ProfileBaseline != H263Param->eProfile) ||
                (H263Param->eLevel > OMX_VIDEO_H263Level45) ||
                (OMX_TRUE == H263Param->bPLUSPTYPEAllowed) ||
                (0 == (H263Param->nAllowedPictureTypes & (OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP))) ||
                (0 != H263Param->nPictureHeaderRepetition))
        {

            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentMpeg4EncAO : ComponentInit Error, unsupported H.263 settings, OUT"));
            return OMX_ErrorUnsupportedSetting;
        }
    }


    //Library init routine
    Status = ipMpegEncoderObject->Mp4EncInit(iEncMode,
             ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video,
             ipPorts[OMX_PORT_INPUTPORT_INDEX]->VideoOrientationType,
             ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video,
             ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->VideoMpeg4,
             ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->VideoErrorCorrection,
             ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->VideoRateType,
             ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->VideoQuantType,
             ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->VideoMotionVector,
             ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->VideoIntraRefresh,
             ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->VideoH263,
             &(ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->ProfileLevel));

    iInputCurrLength = 0;

    //Compute the new output buffer requirement after encoder initialization
    if (OMX_ErrorNone == Status)
    {
        Status = ipMpegEncoderObject->Mp4OutBufferSize(&iOutputBufferSizeAfterInit);
        if (iOutputBufferSizeAfterInit > ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.nBufferSize)
        {
            iPortReconfigurationNeeded = OMX_TRUE;
        }
    }

    //Used in dynamic port reconfiguration
    iFrameCount = 0;
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentMpeg4EncAO : ComponentInit OUT"));

    return Status;

}

/** This function is called upon a transition to the idle or invalid state.
 *  Also it is called by the ComponentDestructor() function
 */
OMX_ERRORTYPE OmxComponentMpeg4EncAO::ComponentDeInit()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentMpeg4EncAO : ComponentDeInit IN"));

    OMX_ERRORTYPE Status = OMX_ErrorNone;

    iIsInit = OMX_FALSE;

    if (iCodecReady)
    {
        Status = ipMpegEncoderObject->Mp4EncDeinit();
        iCodecReady = OMX_FALSE;
    }

#if PROFILING_ON

    OMX_U32 FrameRate =  0;
    if (ipMpegEncoderObject->iProfileStats.iDuration > 0)
    {
        FrameRate = (OMX_U32)(((OsclFloat) ipMpegEncoderObject->iProfileStats.iNumFramesEncoded * 1000) / ipMpegEncoderObject->iProfileStats.iDuration) * 100;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iDiagnosticsLogger, PVLOGMSG_INFO, (0, "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iDiagnosticsLogger, PVLOGMSG_INFO, (0, "OmxComponentMpeg4EncAO - Encoding Time (ms), excluding color conversion = %d", OsclTickCount::TicksToMsec(ipMpegEncoderObject->iProfileStats.iTotalEncTime)));
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iDiagnosticsLogger, PVLOGMSG_INFO, (0, "OmxComponentMpeg4EncAO - Time Spent in Color Conversion (ms) = %d", OsclTickCount::TicksToMsec(ipMpegEncoderObject->iProfileStats.iColorConversionTime)));
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iDiagnosticsLogger, PVLOGMSG_INFO, (0, "OmxComponentMpeg4EncAO - Total Frames Sent for Encoding = %d", ipMpegEncoderObject->iProfileStats.iTotalNumFrames));
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iDiagnosticsLogger, PVLOGMSG_INFO, (0, "OmxComponentMpeg4EncAO - Number of Encoded Frames = %d", ipMpegEncoderObject->iProfileStats.iNumFramesEncoded));
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iDiagnosticsLogger, PVLOGMSG_INFO, (0, "OmxComponentMpeg4EncAO - Frame Rate (Multiplied with 100 for keeping 2 decimal points accuracy) = %d", FrameRate));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iDiagnosticsLogger, PVLOGMSG_INFO, (0, "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"));
#endif

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentMpeg4EncAO : ComponentDeInit OUT"));

    return Status;

}

/* A component specific routine called from BufferMgmtWithoutMarker */
void OmxComponentMpeg4EncAO::ProcessInBufferFlag()
{
    iIsInputBufferEnded = OMX_FALSE;
}

/******************************************************************************
 ** File Name:    mp4dec_bfrctrl.c                                          *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         12/14/2006                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "sc8810_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
/*lint -save -e553 */
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

PUBLIC MMDecRet Mp4Dec_InitYUVBfr(MP4DecObject *vd, DEC_VOP_MODE_T *vop_mode_ptr)
{
	int32 id;
	int32 size_y, size_c;
	int32 ext_size_y, ext_size_c;
	DEC_FRM_BFR *pDecFrame;
	int32 need_malloc_decY = 0, need_malloc_decYUV = 0, need_malloc_dispY = 0,need_malloc_dispYUV=0;

	size_y = vop_mode_ptr->FrameWidth * vop_mode_ptr->FrameHeight;	
	size_c = size_y >> 2;

	ext_size_y = vop_mode_ptr->FrameExtendWidth * vop_mode_ptr->FrameExtendHeight;
	ext_size_c = ext_size_y >> 2;

	if(vop_mode_ptr->post_filter_en)
	{
		if(vop_mode_ptr->VSP_used)
		{
			need_malloc_decY = 1;
			need_malloc_decYUV = 0;
		}
		else if(vop_mode_ptr->VT_used)
		{
			need_malloc_decY = 0;
			need_malloc_dispYUV = 1;
			need_malloc_decYUV = 1;
		}

#ifdef _VSP_LINUX_
		//only alloc reconstruction frame, the display frame has been alloced in omx layer	
#else
		need_malloc_dispY = 1;		//only work in VSP simulation
#endif
	}else
	{
		if (vop_mode_ptr->VSP_used)
		{
	#ifdef _VSP_LINUX_
	#else
			need_malloc_decY = 1;
			need_malloc_decYUV = 0;
	#endif
		}
	}
	//SCI_TRACE_LOW("Mp4Dec_InitYUVBfr : VT_used %d, post_filter_en %d  ", vop_mode_ptr->VT_used , vop_mode_ptr->post_filter_en);

	memset(vd->g_FrmYUVBfr,0,sizeof(vd->g_FrmYUVBfr));
	pDecFrame = vd->g_FrmYUVBfr;
    for(id = 0; id < DEC_YUV_BUFFER_NUM; id++)
	{
		pDecFrame->id = id;    
		pDecFrame->bRef = FALSE;
		pDecFrame->bDisp = FALSE;
		
		if (need_malloc_decY)
		{			
	        	pDecFrame->imgY = (uint8 *)Mp4Dec_MemAlloc(vd, (uint32)size_y * sizeof(uint8), 256, HW_NO_CACHABLE);  //y
			if (vop_mode_ptr->uv_interleaved) //two plane
			{
				pDecFrame->imgU = (uint8 *)Mp4Dec_MemAlloc(vd, 2*(uint32)size_c * sizeof(uint8), 256, HW_NO_CACHABLE);  //uv
				pDecFrame->imgV = PNULL;
			}else //three plane
			{
		        	pDecFrame->imgU = (uint8 *)Mp4Dec_MemAlloc(vd, (uint32)size_c * sizeof(uint8), 256, HW_NO_CACHABLE);  //u
		        	pDecFrame->imgV = (uint8 *)Mp4Dec_MemAlloc(vd, (uint32)size_c * sizeof(uint8), 256, HW_NO_CACHABLE);  //v
		        	if( NULL == pDecFrame->imgV )
		        	{
		            		return MMDEC_MEMORY_ERROR;
		        	}
			}
		        if( (NULL == pDecFrame->imgY) || (NULL == pDecFrame->imgU) )
		        {
		            return MMDEC_MEMORY_ERROR;
		        }
			
		#ifdef _VSP_LINUX_
			pDecFrame->imgYAddr = (uint32)Mp4Dec_MemV2P(vd, pDecFrame->imgY, HW_NO_CACHABLE);  //y
			pDecFrame->imgUAddr = (uint32)Mp4Dec_MemV2P(vd, pDecFrame->imgU, HW_NO_CACHABLE);  //u
			pDecFrame->imgVAddr = (uint32)Mp4Dec_MemV2P(vd, pDecFrame->imgV, HW_NO_CACHABLE);  //v
		#else
			pDecFrame->imgYAddr = (uint32)pDecFrame->imgY >> 8;  //y
			pDecFrame->imgUAddr = (uint32)pDecFrame->imgU >> 8;  //u
			pDecFrame->imgVAddr = (uint32)pDecFrame->imgV >> 8;  //v
		#endif	
		}

		if (need_malloc_decYUV)
		{
			pDecFrame->imgYUV[0] = (uint8 *)Mp4Dec_MemAlloc(vd, (uint32)ext_size_y * sizeof(uint8), 256, SW_CACHABLE);  //y
			pDecFrame->imgYUV[1] = (uint8 *)Mp4Dec_MemAlloc(vd, (uint32)ext_size_c * sizeof(uint8), 256, SW_CACHABLE);  //u
       	 		pDecFrame->imgYUV[2] = (uint8 *)Mp4Dec_MemAlloc(vd, (uint32)(ext_size_c + 8) * sizeof(uint8), 256, SW_CACHABLE);  //v // 8 extra byte for mc loading of V.  

			if( NULL == pDecFrame->imgYUV[0] ||(NULL == pDecFrame->imgYUV[1]) || (NULL == pDecFrame->imgYUV[2]))
       			{
				return MMDEC_MEMORY_ERROR;
			}
		}

		pDecFrame++;
	}

    if (vop_mode_ptr->post_filter_en)
    {
        memset(vd->g_DispFrmYUVBfr,0,sizeof(vd->g_DispFrmYUVBfr));
        pDecFrame = vd->g_DispFrmYUVBfr;
        for(id = 0; id < DISP_YUV_BUFFER_NUM; id++)
        {
            pDecFrame->id = id;    
            pDecFrame->bRef = FALSE;
            pDecFrame->bDisp = FALSE;
			
			if (need_malloc_dispY)
			{
				pDecFrame->imgY = (uint8 *)Mp4Dec_MemAlloc(vd, (uint32)size_y * sizeof(uint8), 256, HW_NO_CACHABLE);  //y
				if (vop_mode_ptr->uv_interleaved) //two plane
				{
					pDecFrame->imgU = (uint8 *)Mp4Dec_MemAlloc(vd, 2*(uint32)size_c * sizeof(uint8), 256, HW_NO_CACHABLE);  //uv
					pDecFrame->imgV = PNULL;
				}else //three plane
				{
	        		pDecFrame->imgU = (uint8 *)Mp4Dec_MemAlloc(vd, (uint32)size_c * sizeof(uint8), 256, HW_NO_CACHABLE);  //u
	            	pDecFrame->imgV = (uint8 *)Mp4Dec_MemAlloc(vd, (uint32)size_c * sizeof(uint8), 256, HW_NO_CACHABLE);  //v

	        		if( NULL == pDecFrame->imgV )
	        		{
	            		return MMDEC_MEMORY_ERROR;
	        		}
				}
				if( (NULL == pDecFrame->imgY) || (NULL == pDecFrame->imgU))
				{
					return MMDEC_MEMORY_ERROR;
				}
			
				pDecFrame->imgYAddr = (uint32)pDecFrame->imgY >> 8;  //y
				pDecFrame->imgUAddr = (uint32)pDecFrame->imgU >> 8;  //u
				pDecFrame->imgVAddr = (uint32)pDecFrame->imgV >> 8;  //v
			}

			if(need_malloc_dispYUV)
			{
				pDecFrame->imgYUV[0] = (uint8 *)Mp4Dec_MemAlloc(vd, (uint32)ext_size_y * sizeof(uint8), 256, SW_CACHABLE);  //y
				pDecFrame->imgYUV[1] = (uint8 *)Mp4Dec_MemAlloc(vd, (uint32)ext_size_c * sizeof(uint8), 256, SW_CACHABLE);  //u
	       	 		pDecFrame->imgYUV[2] = (uint8 *)Mp4Dec_MemAlloc(vd, (uint32)ext_size_c * sizeof(uint8), 256, SW_CACHABLE);  //v

				SCI_TRACE_LOW("Mp4Dec_InitYUVBfr : imgYUV is %x, %x, %x", pDecFrame->imgYUV[0],pDecFrame->imgYUV[1],pDecFrame->imgYUV[2]);	

				if( NULL == pDecFrame->imgYUV[0] ||(NULL == pDecFrame->imgYUV[1]) || (NULL == pDecFrame->imgYUV[2]))
	       			{
					return MMDEC_MEMORY_ERROR;
				}		
			}

            pDecFrame++;
        }
    }

#if _CMODEL_ //for RTL simulation.
	vd->g_FrmYUVBfr[0].imgYAddr = FRAME0_Y_ADDR>>8;
	vd->g_FrmYUVBfr[1].imgYAddr = FRAME1_Y_ADDR>>8;
	vd->g_FrmYUVBfr[2].imgYAddr = FRAME2_Y_ADDR>>8;

	//must modify later!!
	vd->g_DispFrmYUVBfr[0].imgYAddr = DISPLAY_FRAME0_Y_ADDR>>8;
	vd->g_DispFrmYUVBfr[1].imgYAddr = DISPLAY_FRAME1_Y_ADDR>>8;
#endif

    return MMDEC_OK;
}

/**********************************************************
check is there free frame buffer, if has, return the frame buffer ID
************************************************************/
/*find a buffer which has been displayed and will no longer be a reference frame from g_dec_frame,
and set the buffer to be reference frame, if current frame picture type is not B frame */
static BOOLEAN Mp4Dec_GetOneFreeDecBfr(MP4DecObject *vd, int32 *bfrId)
{
	int id;
	
	for(id = 0; id < DEC_YUV_BUFFER_NUM; id++)
	{
		if((!vd->g_FrmYUVBfr[id].bRef) && (!vd->g_FrmYUVBfr[id].bDisp))
		{
			*bfrId = id;
			break;
		}
	}

	if(id == DEC_YUV_BUFFER_NUM)
	{
		return FALSE;
	}else
	{
		return TRUE;
	}
}

static BOOLEAN Mp4Dec_GetOneFreeDispBfr(MP4DecObject *vd, int32 *bfrId)
{
	int id;
	
	for(id = 0; id < DISP_YUV_BUFFER_NUM; id++)
	{
		if((!vd->g_DispFrmYUVBfr[id].bRef) && (!vd->g_DispFrmYUVBfr[id].bDisp))
		{
			*bfrId = id;
			break;
		}
	}

	if(id == DISP_YUV_BUFFER_NUM)
	{
		return FALSE;
	}else
	{
		return TRUE;
	}
}

/*allocate frame buffer for current frame before current frame decoding*/
BOOLEAN Mp4Dec_GetCurRecFrameBfr(MP4DecObject *vd, DEC_VOP_MODE_T *vop_mode_ptr)
{
	int32 bfrId;
	DEC_FRM_BFR *pDecFrame;

	while(!Mp4Dec_GetOneFreeDecBfr(vd, &bfrId))
	{
		//wait for display free buffer
		SCI_TRACE_LOW("Mp4Dec_GetCurRecFrameBfr: no buffer is available!\n");
		return FALSE;
	}

	pDecFrame = vop_mode_ptr->pCurRecFrame->pDecFrame = &vd->g_FrmYUVBfr[bfrId];
	vop_mode_ptr->pCurRecFrame->bfrId = bfrId;
	
#ifdef _VSP_LINUX_
	if(!vop_mode_ptr->post_filter_en)
	{
		int32 size_y;

		if (vop_mode_ptr->VSP_used || vop_mode_ptr->VT_used)
		{
			size_y = vop_mode_ptr->FrameWidth * vop_mode_ptr->FrameHeight;
			
			pDecFrame->imgY = vd->g_rec_buf.imgY;
			pDecFrame->imgU = pDecFrame->imgY + size_y;
			pDecFrame->imgV = pDecFrame->imgU + (size_y>>2);

			pDecFrame->imgYAddr = vd->g_rec_buf.imgYAddr;
			pDecFrame->imgUAddr = pDecFrame->imgYAddr + (size_y>>8);
			pDecFrame->imgVAddr = pDecFrame->imgUAddr + (size_y>>10);
		}else
		{
	#ifdef YUV_THREE_PLANE
		int32 size_y = vop_mode_ptr->FrameExtendWidth * vop_mode_ptr->FrameExtendHeigth;
		int32 uv_format = 1; 	//0: uv, 1: vu
	#else
		int32 size_y = vop_mode_ptr->FrameWidth * vop_mode_ptr->FrameHeight;
		int32 uv_format = 0;	//0: uv, 1: vu
	#endif	

		if (uv_format == 0)	//uv
		{
			pDecFrame->imgY = vd->g_rec_buf.imgY;
			pDecFrame->imgU = pDecFrame->imgY + size_y; //g_rec_buf.imgU;
			pDecFrame->imgV = pDecFrame->imgU + (size_y>>2);//g_rec_buf.imgV;
		}else	//vu
		{
			pDecFrame->imgY =  vd->g_rec_buf.imgY;
			pDecFrame->imgV = pDecFrame->imgY + size_y; //g_rec_buf.imgU;
			pDecFrame->imgU = pDecFrame->imgV + (size_y>>2);//g_rec_buf.imgV;
		}

	#ifdef YUV_THREE_PLANE
		pDecFrame->imgYUV[0] = pDecFrame->imgY;
		pDecFrame->imgYUV[1] = pDecFrame->imgU;
		pDecFrame->imgYUV[2] = pDecFrame->imgV;
//		SCI_TRACE_LOW("software decoding, three plane");
	#else
//		SCI_TRACE_LOW("software decoding, two plane");
	#endif
		}

		vop_mode_ptr->pCurRecFrame->pDecFrame->pBufferHeader= vd->g_rec_buf.pBufferHeader;
	}
#endif

	return TRUE;
}

/*allocate frame buffer for current frame before current frame displaying*/
BOOLEAN Mp4Dec_GetCurDispFrameBfr(MP4DecObject *vd, DEC_VOP_MODE_T *vop_mode_ptr)
{
	int32 bfrId;
	int32 size_y;
	DEC_FRM_BFR *pDecFrame;

	while(!Mp4Dec_GetOneFreeDispBfr(vd, &bfrId))
	{
		//wait for display free buffer
		PRINTF("no buffer is available!\n");
		return FALSE;
	}

	pDecFrame = vop_mode_ptr->pCurDispFrame->pDecFrame = &(vd->g_DispFrmYUVBfr[bfrId]);
	vop_mode_ptr->pCurDispFrame->bfrId = bfrId;
#ifndef _VSP_LINUX_	
	vop_mode_ptr->pCurDispFrame->pDecFrame->bDisp = TRUE;
#endif
//#ifdef _VSP_LINUX_
//	vop_mode_ptr->pCurDispFrame->pDecFrame->bRef= TRUE;
//#endif

	//recode it's corresponding decoding frame.
	if(vop_mode_ptr->VSP_used)
	{
		pDecFrame->rec_imgY = vop_mode_ptr->pCurRecFrame->pDecFrame->imgY;
	}
	else
	{
		pDecFrame->rec_imgY = vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[0];
	}
#ifdef _VSP_LINUX_
	size_y = vop_mode_ptr->FrameWidth * vop_mode_ptr->FrameHeight;

	pDecFrame->imgY = vd->g_rec_buf.imgY;
	pDecFrame->imgU = pDecFrame->imgY + size_y;
	pDecFrame->imgV = pDecFrame->imgU + (size_y>>2);

	pDecFrame->imgYAddr = vd->g_rec_buf.imgYAddr;
	pDecFrame->imgUAddr = pDecFrame->imgYAddr + (size_y>>8);
	pDecFrame->imgVAddr = pDecFrame->imgUAddr + (size_y>>10);

	vop_mode_ptr->pCurDispFrame->pDecFrame->pBufferHeader= vd->g_rec_buf.pBufferHeader;
#endif

	return TRUE;
}

/*get the display frame which is corresponding to reconstruction frame*/
DEC_FRM_BFR* Mp4Dec_GetDispFrameBfr(MP4DecObject *vd, Mp4DecStorablePic *rec_pic)
{
	int32 i;
	uint8 *rec_imgY = PNULL;
	DEC_FRM_BFR *disp_frm = PNULL;
        MP4Handle *mp4Handle = vd->mp4Handle;
	
#ifdef _VSP_LINUX_	
	DEC_VOP_MODE_T *vop_mode_ptr = vd->vop_mode_ptr;
	vop_mode_ptr->pCurDispFrame->pDecFrame->bDisp = 1;
	vop_mode_ptr->pCurDispFrame->pDecFrame->bRef = 1;	
	if((*mp4Handle->VSP_bindCb)&&(vop_mode_ptr->VopPredType != BVOP)&&(vop_mode_ptr->pCurDispFrame->pDecFrame->pBufferHeader!=NULL))
        {
            (*mp4Handle->VSP_bindCb)(mp4Handle->userdata,vop_mode_ptr->pCurDispFrame->pDecFrame->pBufferHeader,0);
        }
#endif

	if( (PNULL == rec_pic) || (PNULL == rec_pic->pDecFrame) )
	{
		return PNULL;
	}
	if(vop_mode_ptr->VSP_used)
	{
		rec_imgY =rec_pic->pDecFrame->imgY;
	}
	else
	{
		rec_imgY =rec_pic->pDecFrame->imgYUV[0];
	}

	for(i = 0; i < DISP_YUV_BUFFER_NUM; i++)
	{
		if(vd->g_DispFrmYUVBfr[i].bDisp && (rec_imgY == vd->g_DispFrmYUVBfr[i].rec_imgY))
		{
			disp_frm = &vd->g_DispFrmYUVBfr[i];
			break;
		}
	}

	if(DISP_YUV_BUFFER_NUM == i)
	{
		return PNULL;
	}
	
#ifdef _VSP_LINUX_	
	disp_frm->bDisp = FALSE;
       disp_frm->bRef = FALSE;
	if((*mp4Handle->VSP_unbindCb) && (vop_mode_ptr->VopPredType != BVOP)&&(disp_frm->pBufferHeader!=NULL))
	{
		(*mp4Handle->VSP_unbindCb)(mp4Handle->userdata, disp_frm->pBufferHeader, 0);
		//disp_frm->pBufferHeader = NULL;
	}
#endif

	return disp_frm;
}

/****************************************************
input: the address of Y frame to be released which has been displayed
output: set the corresponding frame's display type to 0
****************************************************/
MMDecRet MPEG4_DecReleaseDispBfr(MP4DecObject *vd, uint8 *pBfrAddr)
{
 	int i;
	DEC_VOP_MODE_T *vop_mode_ptr = vd->vop_mode_ptr;
	
	SCI_ASSERT(pBfrAddr != NULL);

	if (vop_mode_ptr->post_filter_en)
	{
		for(i = 0; i < DISP_YUV_BUFFER_NUM; i++)
		{
			if(pBfrAddr == vd->g_DispFrmYUVBfr[i].imgY)
			{
				vd->g_DispFrmYUVBfr[i].bDisp = FALSE;
#ifdef _VSP_LINUX_
				vd->g_DispFrmYUVBfr[i].bRef= FALSE;
	//                     (*VSP_unbindCb)(g_user_data,g_DispFrmYUVBfr[i].pBufferHeader,0);
#endif					

				break;
			}
		}

		if(DISP_YUV_BUFFER_NUM == i)
		{
			return MMDEC_ERROR;
		}
	}else
	{
		for(i = 0; i < DEC_YUV_BUFFER_NUM; i++)
		{
			if(pBfrAddr == vd->g_FrmYUVBfr[i].imgY)
			{
				vd->g_FrmYUVBfr[i].bDisp = 0;
				break;
			}
		}

		if(DEC_YUV_BUFFER_NUM == i)
		{
			return MMDEC_ERROR;
		}
	}

	return MMDEC_OK;
}

void Mp4Dec_SetRefFlag ()
{

}

void Mp4Dec_SetDspFlag ()
{

}

/****************************************************
input: the address of Y frame to be displayed
output: the corresponding reconstruction frame. for debug.
****************************************************/
DEC_FRM_BFR* Mp4Dec_GetRecFrm(MP4DecObject *vd, uint8 *pBfrAddr)
{
 	int dispId, decId;
	DEC_FRM_BFR *rec_frm = PNULL;

	SCI_ASSERT(pBfrAddr != NULL);

	//first, found display buffer.
	for(dispId = 0; dispId < DISP_YUV_BUFFER_NUM; dispId++)
	{
		if(pBfrAddr == vd->g_DispFrmYUVBfr[dispId].imgY)
		{
			break;
		}
	}

	if(DISP_YUV_BUFFER_NUM == dispId)
	{
		return PNULL;
	}

	//then, found rec buffer.
	for(decId = 0; decId < DEC_YUV_BUFFER_NUM; decId++)
	{
		if(vd->g_DispFrmYUVBfr[dispId].rec_imgY == vd->g_FrmYUVBfr[decId].imgY)
		{
			rec_frm = &vd->g_FrmYUVBfr[decId];
			break;
		}
	}

	if(DEC_YUV_BUFFER_NUM == decId)
	{
		return PNULL;
	}

	return rec_frm;
	
}


/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 

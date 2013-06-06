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
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

PUBLIC MMDecRet Mp4Dec_InitYUVBfr(DEC_VOP_MODE_T *vop_mode_ptr)
{
	int32 id;
	int32 size_y, size_c;
	DEC_FRM_BFR *pDecFrame;
	int32 need_malloc_decY = 0, need_malloc_dispY = 0;

	size_y = vop_mode_ptr->FrameWidth * vop_mode_ptr->FrameHeight;	
	size_c = size_y >> 2;


	if(vop_mode_ptr->post_filter_en)
	{
		need_malloc_decY = 1;
#ifdef _VSP_LINUX_
		//only alloc reconstruction frame, the display frame has been alloced in omx layer	
#else
		need_malloc_dispY = 1;		//only work in VSP simulation
#endif
	}else
	{
	#ifdef _VSP_LINUX_
	#else
			need_malloc_decY = 1;
	#endif
	}

	memset(g_FrmYUVBfr,0,sizeof(g_FrmYUVBfr));
	pDecFrame = g_FrmYUVBfr;
    for(id = 0; id < DEC_YUV_BUFFER_NUM; id++)
	{
		pDecFrame->id = id;    
		pDecFrame->bRef = FALSE;
		pDecFrame->bDisp = FALSE;
		
		if (need_malloc_decY)
		{			
	        	pDecFrame->imgY = (uint8 *)Mp4Dec_ExtraMemAlloc_64WordAlign((uint32)size_y * sizeof(uint8));  //y
			if (vop_mode_ptr->uv_interleaved) //two plane
			{
				pDecFrame->imgU = (uint8 *)Mp4Dec_ExtraMemAlloc_64WordAlign(2*(uint32)size_c * sizeof(uint8));  //uv
				pDecFrame->imgV = PNULL;
			}else //three plane
			{
		        	pDecFrame->imgU = (uint8 *)Mp4Dec_ExtraMemAlloc_64WordAlign((uint32)size_c * sizeof(uint8));  //u
		        	pDecFrame->imgV = (uint8 *)Mp4Dec_ExtraMemAlloc_64WordAlign((uint32)size_c * sizeof(uint8));  //v
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
			pDecFrame->imgYAddr = (uint32)pDecFrame->imgY;  //y
			pDecFrame->imgUAddr = (uint32)pDecFrame->imgU;  //u
			pDecFrame->imgVAddr = (uint32)pDecFrame->imgV;  //v

		#else
			pDecFrame->imgYAddr = (uint32)pDecFrame->imgY;  //y
			pDecFrame->imgUAddr = (uint32)pDecFrame->imgU;  //u
			pDecFrame->imgVAddr = (uint32)pDecFrame->imgV;  //v
		#endif	
		}

		// Malloc frame info buffer. 80 Bytes for each MB.
		pDecFrame->rec_info = (uint8 *)Mp4Dec_ExtraMemAlloc_64WordAlign((uint32)((size_y >>8)*80));
		pDecFrame->rec_infoAddr = (uint32)pDecFrame->rec_info;		

		pDecFrame++;
	}

    if (vop_mode_ptr->post_filter_en)
    {
        memset(g_DispFrmYUVBfr,0,sizeof(g_DispFrmYUVBfr));
        pDecFrame = g_DispFrmYUVBfr;
        for(id = 0; id < DISP_YUV_BUFFER_NUM; id++)
        {
            pDecFrame->id = id;    
            pDecFrame->bRef = FALSE;
            pDecFrame->bDisp = FALSE;
			
			if (need_malloc_dispY)
			{
				pDecFrame->imgY = (uint8 *)Mp4Dec_ExtraMemAlloc_64WordAlign((uint32)size_y * sizeof(uint8));  //y
				if (vop_mode_ptr->uv_interleaved) //two plane
				{
					pDecFrame->imgU = (uint8 *)Mp4Dec_ExtraMemAlloc_64WordAlign(2*(uint32)size_c * sizeof(uint8));  //uv
					pDecFrame->imgV = PNULL;
				}else //three plane
				{
	        		pDecFrame->imgU = (uint8 *)Mp4Dec_ExtraMemAlloc_64WordAlign((uint32)size_c * sizeof(uint8));  //u
	            	pDecFrame->imgV = (uint8 *)Mp4Dec_ExtraMemAlloc_64WordAlign((uint32)size_c * sizeof(uint8));  //v

	        		if( NULL == pDecFrame->imgV )
	        		{
	            		return MMDEC_MEMORY_ERROR;
	        		}
				}
				if( (NULL == pDecFrame->imgY) || (NULL == pDecFrame->imgU))
				{
					return MMDEC_MEMORY_ERROR;
				}
			
				pDecFrame->imgYAddr = (uint32)pDecFrame->imgY ;  //y
				pDecFrame->imgUAddr = (uint32)pDecFrame->imgU ;  //u
				pDecFrame->imgVAddr = (uint32)pDecFrame->imgV ;  //v
			}

            pDecFrame++;
        }
    }



#if _CMODEL_ //for RTL simulation.
	g_FrmYUVBfr[0].imgYAddr = FRAME0_Y_ADDR>>8;
	g_FrmYUVBfr[1].imgYAddr = FRAME1_Y_ADDR>>8;
	g_FrmYUVBfr[2].imgYAddr = FRAME2_Y_ADDR>>8;

	//must modify later!!
	g_DispFrmYUVBfr[0].imgYAddr = DISPLAY_FRAME0_Y_ADDR>>8;
	g_DispFrmYUVBfr[1].imgYAddr = DISPLAY_FRAME1_Y_ADDR>>8;
#endif

    return MMDEC_OK;
}

/**********************************************************
check is there free frame buffer, if has, return the frame buffer ID
************************************************************/
/*find a buffer which has been displayed and will no longer be a reference frame from g_dec_frame,
and set the buffer to be reference frame, if current frame picture type is not B frame */
BOOLEAN Mp4Dec_GetOneFreeDecBfr(int32 *bfrId)
{
	int id;
	
	for(id = 0; id < DEC_YUV_BUFFER_NUM; id++)
	{
		if((!g_FrmYUVBfr[id].bRef) && (!g_FrmYUVBfr[id].bDisp))
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

BOOLEAN Mp4Dec_GetOneFreeDispBfr(int32 *bfrId)
{
	int id;
	
	for(id = 0; id < DISP_YUV_BUFFER_NUM; id++)
	{
		if((!g_DispFrmYUVBfr[id].bRef) && (!g_DispFrmYUVBfr[id].bDisp))
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
BOOLEAN Mp4Dec_GetCurRecFrameBfr(DEC_VOP_MODE_T *vop_mode_ptr)
{
	int32 bfrId;
	DEC_FRM_BFR *pDecFrame;

	while(!Mp4Dec_GetOneFreeDecBfr(&bfrId))
	{
		//wait for display free buffer
//		PRINTF("no buffer is available!\n");
		return FALSE;
	}

	pDecFrame = vop_mode_ptr->pCurRecFrame->pDecFrame = &g_FrmYUVBfr[bfrId];
	vop_mode_ptr->pCurRecFrame->bfrId = bfrId;

#ifdef _VSP_LINUX_
	if(!vop_mode_ptr->post_filter_en)
	{
		int32 size_y;

			size_y = vop_mode_ptr->FrameWidth * vop_mode_ptr->FrameHeight;
			
			pDecFrame->imgY = g_rec_buf.imgY;
			pDecFrame->imgU = pDecFrame->imgY + size_y;
			pDecFrame->imgV = pDecFrame->imgU + (size_y>>2);

			pDecFrame->imgYAddr = g_rec_buf.imgYAddr;
			pDecFrame->imgUAddr = pDecFrame->imgYAddr + size_y;
			pDecFrame->imgVAddr = pDecFrame->imgUAddr + (size_y>>2);

		vop_mode_ptr->pCurRecFrame->pDecFrame->pBufferHeader= g_rec_buf.pBufferHeader;
	}
#endif

	return TRUE;
}

/*allocate frame buffer for current frame before current frame displaying*/
BOOLEAN Mp4Dec_GetCurDispFrameBfr(DEC_VOP_MODE_T *vop_mode_ptr)
{
	int32 bfrId;
	int32 size_y;
	DEC_FRM_BFR *pDecFrame;

	while(!Mp4Dec_GetOneFreeDispBfr(&bfrId))
	{
		//wait for display free buffer
//		PRINTF("no buffer is available!\n");
		return FALSE;
	}

	pDecFrame = vop_mode_ptr->pCurDispFrame->pDecFrame = &g_DispFrmYUVBfr[bfrId];
	vop_mode_ptr->pCurDispFrame->bfrId = bfrId;
#ifndef _VSP_LINUX_	
	vop_mode_ptr->pCurDispFrame->pDecFrame->bDisp = TRUE;
#endif
//#ifdef _VSP_LINUX_
//	vop_mode_ptr->pCurDispFrame->pDecFrame->bRef= TRUE;
//#endif

	//recode it's corresponding decoding frame.
	pDecFrame->rec_imgY = vop_mode_ptr->pCurRecFrame->pDecFrame->imgY;
#ifdef _VSP_LINUX_
	size_y = vop_mode_ptr->FrameWidth * vop_mode_ptr->FrameHeight;

	pDecFrame->imgY = g_rec_buf.imgY;
	pDecFrame->imgU = pDecFrame->imgY + size_y;
	pDecFrame->imgV = pDecFrame->imgU + (size_y>>2);

	pDecFrame->imgYAddr = g_rec_buf.imgYAddr;
	pDecFrame->imgUAddr = pDecFrame->imgYAddr + size_y;
	pDecFrame->imgVAddr = pDecFrame->imgUAddr + (size_y>>2);

	vop_mode_ptr->pCurDispFrame->pDecFrame->pBufferHeader= g_rec_buf.pBufferHeader;
#endif

	return TRUE;
}

/*get the display frame which is corresponding to reconstruction frame*/
DEC_FRM_BFR* Mp4Dec_GetDispFrameBfr(Mp4DecStorablePic *rec_pic)
{
	int32 i;
	uint8 *rec_imgY = PNULL;
	DEC_FRM_BFR *disp_frm = PNULL;
	
#ifdef _VSP_LINUX_	
	DEC_VOP_MODE_T *vop_mode_ptr = Mp4Dec_GetVopmode();
	vop_mode_ptr->pCurDispFrame->pDecFrame->bDisp = 1;
	vop_mode_ptr->pCurDispFrame->pDecFrame->bRef = 1;	
	if((vop_mode_ptr->VopPredType != BVOP)&&(vop_mode_ptr->pCurDispFrame->pDecFrame->pBufferHeader!=NULL))
		OR_VSP_BIND(vop_mode_ptr->pCurDispFrame->pDecFrame->pBufferHeader);
	#endif

	if( (PNULL == rec_pic) || (PNULL == rec_pic->pDecFrame) )
	{
		return PNULL;
	}

	rec_imgY = rec_pic->pDecFrame->imgY;

	for(i = 0; i < DISP_YUV_BUFFER_NUM; i++)
	{
		if(g_DispFrmYUVBfr[i].bDisp && (rec_imgY == g_DispFrmYUVBfr[i].rec_imgY))
		{
			disp_frm = &g_DispFrmYUVBfr[i];
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
	if((vop_mode_ptr->VopPredType != BVOP)&&(disp_frm->pBufferHeader!=NULL))
	{
		OR_VSP_UNBIND(disp_frm->pBufferHeader);
		//disp_frm->pBufferHeader = NULL;
	}
#endif

	return disp_frm;
}

/****************************************************
input: the address of Y frame to be released which has been displayed
output: set the corresponding frame's display type to 0
****************************************************/
MMDecRet MPEG4_DecReleaseDispBfr(uint8 *pBfrAddr)
{
 	int i;
	DEC_VOP_MODE_T *vop_mode_ptr = Mp4Dec_GetVopmode();
	
	SCI_ASSERT(pBfrAddr != NULL);

	if (vop_mode_ptr->post_filter_en)
	{
		for(i = 0; i < DISP_YUV_BUFFER_NUM; i++)
		{
			if(pBfrAddr == g_DispFrmYUVBfr[i].imgY)
			{
				g_DispFrmYUVBfr[i].bDisp = FALSE;
#ifdef _VSP_LINUX_
				g_DispFrmYUVBfr[i].bRef= FALSE;
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
			if(pBfrAddr == g_FrmYUVBfr[i].imgY)
			{
				g_FrmYUVBfr[i].bDisp = 0;
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
DEC_FRM_BFR* Mp4Dec_GetRecFrm(uint8 *pBfrAddr)
{
 	int dispId, decId;
	DEC_FRM_BFR *rec_frm = PNULL;

	SCI_ASSERT(pBfrAddr != NULL);

	//first, found display buffer.
	for(dispId = 0; dispId < DISP_YUV_BUFFER_NUM; dispId++)
	{
		if(pBfrAddr == g_DispFrmYUVBfr[dispId].imgY)
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
		if(g_DispFrmYUVBfr[dispId].rec_imgY == g_FrmYUVBfr[decId].imgY)
		{
			rec_frm = &g_FrmYUVBfr[decId];
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


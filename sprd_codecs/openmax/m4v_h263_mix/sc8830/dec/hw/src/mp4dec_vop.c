/******************************************************************************
 ** File Name:    mp4dec_vop.c                                             *
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

PUBLIC void Mp4Dec_exit_picture(DEC_VOP_MODE_T *vop_mode_ptr)
{	
	/*reorder reference frame list*/
	if(vop_mode_ptr->VopPredType != BVOP)
	{
		Mp4DecStorablePic * pframetmp;

		vop_mode_ptr->pCurRecFrame->pDecFrame->bRef = TRUE;
#ifdef _VSP_LINUX_
		if((!vop_mode_ptr->post_filter_en)&&(vop_mode_ptr->pCurRecFrame->pDecFrame->pBufferHeader!=NULL))
		{
			//(*VSP_bindCb)(g_user_data,vop_mode_ptr->pCurRecFrame->pDecFrame->pBufferHeader,0);
			OR_VSP_BIND(vop_mode_ptr->pCurRecFrame->pDecFrame->pBufferHeader);
		}
#endif

		/*the buffer for forward reference frame will no longer be a reference frame*/		
		if(vop_mode_ptr->pFrdRefFrame->pDecFrame != NULL)
		{
#ifdef _VSP_LINUX_
			if((!vop_mode_ptr->post_filter_en)&&(vop_mode_ptr->pFrdRefFrame->pDecFrame->pBufferHeader!=NULL))
			{
				//(*VSP_unbindCb)(g_user_data,vop_mode_ptr->pFrdRefFrame->pDecFrame->pBufferHeader,0);
				OR_VSP_UNBIND(vop_mode_ptr->pFrdRefFrame->pDecFrame->pBufferHeader);
				vop_mode_ptr->pFrdRefFrame->pDecFrame->pBufferHeader = NULL;
			}
#endif		

			vop_mode_ptr->pFrdRefFrame->pDecFrame->bRef = FALSE;
			vop_mode_ptr->pFrdRefFrame->pDecFrame = NULL;
		}
		
		//exchange buffer address. bck->frd, current->bck, frd->current
		pframetmp = vop_mode_ptr->pFrdRefFrame;
		vop_mode_ptr->pFrdRefFrame = vop_mode_ptr->pBckRefFrame;
		vop_mode_ptr->pBckRefFrame = vop_mode_ptr->pCurRecFrame;	
		vop_mode_ptr->pCurRecFrame = pframetmp;	
	}
}

PUBLIC MMDecRet Mp4Dec_InitVop(DEC_VOP_MODE_T *vop_mode_ptr, MMDecInput *dec_input_ptr)
{
	uint32 vld_table_addr;
	MMDecRet ret = MMDEC_ERROR;

	vop_mode_ptr->sliceNumber	= 0;
	vop_mode_ptr->stop_decoding = FALSE;
	vop_mode_ptr->mbnumDec		= 0;
	vop_mode_ptr->error_flag	= FALSE;
//	vop_mode_ptr->return_pos = 0;
//	vop_mode_ptr->return_pos1 = 0;
//	vop_mode_ptr->return_pos2 = 0;
	vop_mode_ptr->frame_len		= dec_input_ptr->dataLen;
	vop_mode_ptr->err_num		= dec_input_ptr->err_pkt_num;
	vop_mode_ptr->err_left		= dec_input_ptr->err_pkt_num;
	vop_mode_ptr->err_pos_ptr	= dec_input_ptr->err_pkt_pos;
	vop_mode_ptr->err_MB_num	= vop_mode_ptr->MBNum;

	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+IMG_SIZE_OFF, ((((g_dec_vop_mode_ptr->OrgFrameHeight%16==0)?(g_dec_vop_mode_ptr->OrgFrameHeight/16):(g_dec_vop_mode_ptr->OrgFrameHeight/16+1))&0xff)<<8)|((((g_dec_vop_mode_ptr->OrgFrameWidth%16==0)?(g_dec_vop_mode_ptr->OrgFrameWidth/16):(g_dec_vop_mode_ptr->OrgFrameWidth/16+1))&0xff)),"IMG_SIZE");

	OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x10,vop_mode_ptr->data_partition_buffer_Addr>>3,"data partition buffer." );

	//load_vld_table_en=1;
	if(vop_mode_ptr->bReversibleVlc&&vop_mode_ptr->VopPredType!=BVOP)		
	{
		vld_table_addr= Mp4Dec_GetPhyAddr(g_rvlc_tbl_ptr);
	}
	else
	{
		vld_table_addr= Mp4Dec_GetPhyAddr(g_huff_tbl_ptr);
	}
	OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR+0xc, (vld_table_addr)/8,"ddr vlc table start addr");//qiangshen@2013_01_11
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_SIZE_SET_OFF, 0x100,"ddr VLC table size");
	
	if (g_is_need_init_vsp_dcttab)
	{
					
		{  
			int32 inter_quant=0;//(vop_mode_ptr->InterQuantizerMatrix);
			int32 intra_quant=0;//(vop_mode_ptr->IntraQuantizerMatrix);
			int i;
			volatile int8 tmp1,tmp2,tmp3,tmp4;
			
						
			for(i=0;i<96;i++)
			{	
				if(i<16)
				{
					tmp1=vop_mode_ptr->InterQuantizerMatrix[4*i];
					tmp2=vop_mode_ptr->InterQuantizerMatrix[4*i+1];
					tmp3=vop_mode_ptr->InterQuantizerMatrix[4*i+2];
					tmp4=vop_mode_ptr->InterQuantizerMatrix[4*i+3];
					
					inter_quant=((tmp4&0xff)<<24)|((tmp3&0xff)<<16)|((tmp2&0xff)<<8)|(tmp1&0xff);
					OR1200_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+4*i,inter_quant,"weightscale inter8x8");
				}else if(i<32)
				{
				     OR1200_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+i*4,0,"zero");

				}else if(i<48)
				{
					tmp1=vop_mode_ptr->IntraQuantizerMatrix[4*(i-32)];
					tmp2=vop_mode_ptr->IntraQuantizerMatrix[4*(i-32)+1];
					tmp3=vop_mode_ptr->IntraQuantizerMatrix[4*(i-32)+2];
					tmp4=vop_mode_ptr->IntraQuantizerMatrix[4*(i-32)+3];
					
					intra_quant=((tmp4&0xff)<<24)|((tmp3&0xff)<<16)|((tmp2&0xff)<<8)|(tmp1&0xff);
					
					OR1200_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+4*i,intra_quant,"weightscale intra8x8");
				}else
				{
					OR1200_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+i*4,0,"zero");
				}
				
			}
		}
	}	
			
  
	/*init  current frame, forward reference frame, backward reference frame*/	

	// NOTE: MPEG4 decoder DO NOT support post_filter in Shark.

	OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR,  vop_mode_ptr->pCurRecFrame->pDecFrame->imgYAddr >> 3,"current Y addr");
	OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR+4,  vop_mode_ptr->pCurRecFrame->pDecFrame->imgUAddr >>3,"current UV addr");
	OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR+8,  vop_mode_ptr->pCurRecFrame->pDecFrame->rec_infoAddr >>3,"current info addr");

//	if (vop_mode_ptr->post_filter_en)
//	{
//		VSP_WRITE_REG(pTableAddr+ 4, vop_mode_ptr->pCurDispFrame->pDecFrame->imgYAddr, "configure display frame Y");
//	}else
//	{
//		VSP_WRITE_REG(pTableAddr+ 4, vop_mode_ptr->pCurRecFrame->pDecFrame->imgYAddr, "configure display frame Y");
//	}
	
	if(IVOP != vop_mode_ptr->VopPredType)
	{
		if (vop_mode_ptr->pBckRefFrame->pDecFrame == NULL)
		{
			return MMDEC_ERROR;
		}else
		{
			OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR+0x80,vop_mode_ptr->pBckRefFrame->pDecFrame->imgYAddr >>3,"ref L0 Y addr");
			OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR+0x100,vop_mode_ptr->pBckRefFrame->pDecFrame->imgUAddr >>3,"ref L0 UV addr");
			OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR+0x180,vop_mode_ptr->pBckRefFrame->pDecFrame->rec_infoAddr>>3,"ref L0 info addr");
		}
		
		if(BVOP == vop_mode_ptr->VopPredType)
		{
			if (vop_mode_ptr->pFrdRefFrame->pDecFrame == NULL)
			{
				return MMDEC_ERROR;
			}else
			{
				OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR+0x80,vop_mode_ptr->pFrdRefFrame->pDecFrame->imgYAddr >>3,"ref L0 Y addr");
				OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR+0x100,vop_mode_ptr->pFrdRefFrame->pDecFrame->imgUAddr >>3,"ref L0 UV addr");
				OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR+0x180,vop_mode_ptr->pFrdRefFrame->pDecFrame->rec_infoAddr>>3,"ref L0 info addr");	

				OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR+0xc0,vop_mode_ptr->pBckRefFrame->pDecFrame->imgYAddr >>3,"ref L1 Y addr");
				OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR+0x140,vop_mode_ptr->pBckRefFrame->pDecFrame->imgUAddr >>3,"ref L1 UV addr");
				OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR+0x1c0,vop_mode_ptr->pBckRefFrame->pDecFrame->rec_infoAddr>>3,"ref L1 info addr");
			}
		}
	}

	return MMDEC_OK;
}


PUBLIC void Mp4Dec_output_one_frame (MMDecOutput *dec_output_ptr, DEC_VOP_MODE_T *vop_mode_ptr)
{
	Mp4DecStorablePic *pic = PNULL;
	VOP_PRED_TYPE_E VopPredType = vop_mode_ptr->VopPredType;//pre_vop_type;
		
	/*output frame for display*/
	if(VopPredType != BVOP)
	{
		if(g_nFrame_dec > 0)
		{
			/*send backward reference (the lastest reference frame) frame's YUV to display*/
			pic = vop_mode_ptr->pBckRefFrame;
		}		
	}else
	{
		/*send current B frame's YUV to display*/
		pic = vop_mode_ptr->pCurRecFrame;
	}
        
	if (vop_mode_ptr->post_filter_en)
	{
		DEC_FRM_BFR *display_frame = PNULL;

		/*get display frame from display queue*/
		display_frame = Mp4Dec_GetDispFrameBfr(pic);
		if(PNULL != display_frame)
		{
			dec_output_ptr->pOutFrameY = display_frame->imgY;
			dec_output_ptr->pOutFrameU = display_frame->imgU;
			dec_output_ptr->pOutFrameV = display_frame->imgV;
			dec_output_ptr->is_transposed = 0;
			dec_output_ptr->frame_width = vop_mode_ptr->FrameWidth;
			dec_output_ptr->frame_height = vop_mode_ptr->FrameHeight;
			dec_output_ptr->frameEffective = 1;
			dec_output_ptr->err_MB_num = vop_mode_ptr->err_MB_num;
#ifdef _VSP_LINUX_
			dec_output_ptr->pBufferHeader = display_frame->pBufferHeader;
#endif	
		}else
		{
			dec_output_ptr->frameEffective = 0;
		}
	}else
	{
		if(pic != PNULL)
		{
			
			dec_output_ptr->pOutFrameY = pic->pDecFrame->imgY;
			dec_output_ptr->pOutFrameU = pic->pDecFrame->imgU;
			dec_output_ptr->pOutFrameV = pic->pDecFrame->imgV;
#ifdef _VSP_LINUX_			
			dec_output_ptr->pBufferHeader = pic->pDecFrame->pBufferHeader;
#endif
			dec_output_ptr->is_transposed = 0;

			dec_output_ptr->frame_width = vop_mode_ptr->FrameWidth;
			dec_output_ptr->frame_height = vop_mode_ptr->FrameHeight;
			dec_output_ptr->frameEffective = 1;
			dec_output_ptr->err_MB_num = vop_mode_ptr->err_MB_num;
#ifndef _VSP_LINUX_				
//			pic->pDecFrame->bDisp = TRUE; 
#endif
		}else
		{
			dec_output_ptr->frame_width = vop_mode_ptr->FrameWidth;
			dec_output_ptr->frame_height = vop_mode_ptr->FrameHeight;
			dec_output_ptr->frameEffective = 0;
		}
	}		
}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 


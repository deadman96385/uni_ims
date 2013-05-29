/******************************************************************************
 ** File Name:    mp4dec_interface.c  		                                  *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         08/25/2008                                                  *
 ** Copyright:    2008 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 01/23/2007    Xiaowei Luo     Create.                                     *
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

PUBLIC void  MP4Dec_SetCurRecPic(uint8	*pFrameY,uint8 *pFrameY_phy,void *pBufferHeader)
{
	 int i; 
	g_rec_buf.imgY =  pFrameY;

	g_rec_buf.imgYAddr = (uint32)pFrameY_phy;

	g_rec_buf.pBufferHeader = pBufferHeader;	

	
}


/*****************************************************************************/
//  Description:   Init mpeg4 decoder	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
PUBLIC MMDecRet MP4DecInit(MMCodecBuffer *pBuffer, MMDecVideoFormat *video_format_ptr)
{
	MMDecRet is_init_success = MMDEC_OK;
	DEC_VOP_MODE_T *vop_mode_ptr = NULL;
	H263_PLUS_HEAD_INFO_T *h263_plus_head_info_ptr = NULL;

	Mp4Dec_InitInterMem(pBuffer);

	vop_mode_ptr = (DEC_VOP_MODE_T *)Mp4Dec_InterMemAlloc(sizeof(DEC_VOP_MODE_T));
	SCI_ASSERT(NULL != vop_mode_ptr);	
    Mp4Dec_SetVopmode(vop_mode_ptr);

	g_nFrame_dec = 0;

	g_dec_is_first_frame = TRUE;
	g_dec_is_stop_decode_vol = FALSE;
	g_dec_is_changed_format = FALSE;	
	g_dec_pre_vop_format = IVOP;

	g_is_need_init_vsp_hufftab = TRUE;
	g_is_need_init_vsp_dcttab= TRUE;
	
	vop_mode_ptr->error_flag = FALSE;	
	


	//for H263 plus header
	h263_plus_head_info_ptr = (H263_PLUS_HEAD_INFO_T *)Mp4Dec_InterMemAlloc(sizeof(H263_PLUS_HEAD_INFO_T));
	SCI_ASSERT(NULL != h263_plus_head_info_ptr);
	Mp4Dec_SetH263PlusHeadInfo(h263_plus_head_info_ptr);
#if SIM_IN_WIN
	vop_mode_ptr->pMbMode_B = (DEC_MB_MODE_T *)Mp4Dec_InterMemAlloc(sizeof(DEC_MB_MODE_T)); 
    SCI_ASSERT(NULL != vop_mode_ptr->pMbMode_B);
    vop_mode_ptr->mb_cache_ptr = (DEC_MB_BFR_T *)Mp4Dec_InterMemAlloc(sizeof(DEC_MB_BFR_T));
	SCI_ASSERT(NULL != vop_mode_ptr->mb_cache_ptr); 	

#else
	Mp4Dec_InitDecoderPara(vop_mode_ptr);	

	
    vop_mode_ptr->uv_interleaved = video_format_ptr->uv_interleaved = 1;
	//vop_mode_ptr->video_std = video_format_ptr->video_std;
	vop_mode_ptr->intra_acdc_pred_disable = TRUE;
	vop_mode_ptr->QuantizerType = Q_H263;
	vop_mode_ptr->bDataPartitioning = FALSE;
	vop_mode_ptr->bReversibleVlc = FALSE;
	vop_mode_ptr->bResyncMarkerDisable = TRUE;
	vop_mode_ptr->bAlternateScan = FALSE;
	vop_mode_ptr->bQuarter_pel = FALSE;
	vop_mode_ptr->bInterlace = FALSE;
	vop_mode_ptr->bInitSuceess = FALSE;	
	memset(vop_mode_ptr->InterQuantizerMatrix,0,64);
    memset(vop_mode_ptr->IntraQuantizerMatrix,0,64);
	vop_mode_ptr->NumGobInVop=0;
    vop_mode_ptr->NumMBInGob=0;
	vop_mode_ptr->num_mbline_gob=0;
    vop_mode_ptr->last_non_b_time=0;
    vop_mode_ptr->last_time_base=0;
	vop_mode_ptr->time_base=0;
	vop_mode_ptr->time=0;
	vop_mode_ptr->time_pp=0;
    vop_mode_ptr->time_bp=0;
	vop_mode_ptr->mvInfoForward.FCode=0;
    vop_mode_ptr->mvInfoBckward.FCode=0;
	vop_mode_ptr->QuantPrecision = 5;
	vop_mode_ptr->bCoded = TRUE;
	vop_mode_ptr->RoundingControl = 0;
	vop_mode_ptr->IntraDcSwitchThr = 0;
	

#endif

#if 0
	/*judge h.263 or mpeg4*/
	if(video_format_ptr->video_std !=VSP_MPEG4)
	{
		PRINTF("\nH263(ITU or Sorenson format) is detected!\n");
	}else 
	{
		#if SIM_IN_WIN
		Mp4Dec_Reset(vop_mode_ptr);
		#endif
		if(video_format_ptr->i_extra > 0)
		{
			#if SIM_IN_WIN
			PRINTF("\nIt is MPEG-4 bitstream!\n");
			Mp4Dec_InitBitstream(video_format_ptr->p_extra, video_format_ptr->i_extra);
		   #endif
			is_init_success = Mp4Dec_DecMp4Header(vop_mode_ptr, video_format_ptr->i_extra);

		 	if(MMDEC_OK == is_init_success)
			{
				video_format_ptr->frame_width = vop_mode_ptr->OrgFrameWidth;
				video_format_ptr->frame_height= vop_mode_ptr->OrgFrameHeight;
			}
		}

	#if 0 //removed to MP4DecDecode() function.
		if(MMDEC_OK == is_init_success)
		{
			Mp4Dec_InitSessionDecode(vop_mode_ptr);
			vop_mode_ptr->bInitSuceess = TRUE;
		}
	#endif	
	}
#endif    

	return is_init_success;
}

PUBLIC MMDecRet MP4DecVolHeader(MMDecVideoFormat *video_format_ptr)
{
	DEC_VOP_MODE_T *vop_mode_ptr = Mp4Dec_GetVopmode();
	
	/*judge h.263 or mpeg4*/
	if(video_format_ptr->video_std !=VSP_MPEG4)
	{
		PRINTF("\nH263(ITU or Sorenson format) is detected!\n");
	}else 
	{

		if(video_format_ptr->i_extra > 0)
		{
			MMDecRet is_init_success;

			is_init_success = Mp4Dec_DecMp4Header(vop_mode_ptr, video_format_ptr->i_extra);

		 	if(MMDEC_OK == is_init_success)
			{
				video_format_ptr->frame_width = vop_mode_ptr->OrgFrameWidth;
				video_format_ptr->frame_height= vop_mode_ptr->OrgFrameHeight;
			}
		}
	}
}
/*****************************************************************************/
//  Description:   firware Decode one vop	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
SLICEINFO SliceInfo;
PUBLIC static mpeg4_decode_vop(DEC_VOP_MODE_T *vop_mode_ptr)	
{
	int tmp;
	int pic_end=0;
	int VopPredType=vop_mode_ptr->VopPredType;
    int resyn_bits=VopPredType?vop_mode_ptr->mvInfoForward.FCode - 1:0;


	while(!pic_end)
	{	
		tmp=OR1200_READ_REG(GLB_REG_BASE_ADDR+VSP_INT_RAW_OFF, "check interrupt type");
		if(tmp&0x30)
		{
			vop_mode_ptr->error_flag=1;

			OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_CLR_OFF, 0x1ff,"clear BSM_frame done int");
			//OR1200_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL");
			pic_end=1;//weihu
		}
		else if((tmp&0x00000004)==0x00000004)
		{
		    
			vop_mode_ptr->error_flag=0;
			OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_CLR_OFF, 0x1ff,"clear BSM_frame done int");
			OR1200_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL");
			pic_end=0;
            
			tmp=OR1200_READ_REG(GLB_REG_BASE_ADDR+VSP_DBG0_OFF, "read mbx mby");
			
			vop_mode_ptr->mb_y=tmp&0xff;
           		vop_mode_ptr->mb_x=(tmp>>8)&0xff;

			if(vop_mode_ptr->mb_y==(vop_mode_ptr->MBNumY-1)&&vop_mode_ptr->mb_x==(vop_mode_ptr->MBNumX-1))
			{
				pic_end=1;
			}else
			{

				if(vop_mode_ptr->video_std!=ITU_H263)
				{	
					if(!vop_mode_ptr->bResyncMarkerDisable)
					{
						if(Mp4Dec_CheckResyncMarker(resyn_bits))
						{	
							Mp4Dec_GetVideoPacketHeader(vop_mode_ptr,&SliceInfo, resyn_bits);
						}
					}
				}else if(VopPredType!=BVOP)
				{
					SliceInfo.GobNum++;
					Mp4Dec_DecGobHeader(vop_mode_ptr,&SliceInfo);
				}
			}

		
		}
	}	
}


/*****************************************************************************/
//  Description:   Decode one vop	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/



#if SIM_IN_WIN
	char *bitptr;
	extern int OR1200_Vaild;
	FILE *LENTH;
	extern FILE *GlobalInfo;
	int32 slice_offset;
#endif

int g_mpeg4_dec_err_flag;
PUBLIC MMDecRet MP4DecDecode(MMDecInput *dec_input_ptr, MMDecOutput *dec_output_ptr)
{
	MMDecRet ret = MMDEC_ERROR;

   
	int16 idx=0;
    //uint32 Slice_lenth[1080]={0};
	DEC_VOP_MODE_T *vop_mode_ptr = Mp4Dec_GetVopmode();
    
	vop_mode_ptr->error_flag = FALSE;

	g_mpeg4_dec_err_flag = 0;


#if SIM_IN_WIN
     PPAParaBUF *PPAParaBuf;//=NULL;
	assert((PPAParaBuf=malloc(sizeof(PPAParaBUF)))!=NULL);
	memset(PPAParaBuf,0,sizeof(PPAParaBUF));
	 if(!Mp4Dec_GetCurRecFrameBfr(vop_mode_ptr))
    {
        memset(dec_output_ptr,0,sizeof(MMDecOutput));
        dec_output_ptr->frameEffective = 0;

        return MMDEC_OUTPUT_BUFFER_OVERFLOW;
    }
    
	if(LENTH==NULL)
	{
		LENTH=fopen("..\\..\\test_vectors\\bs_offset.txt","w+");
		assert(LENTH!=NULL);
	}
	
	if (vop_mode_ptr->error_flag)
	{
		vop_mode_ptr->error_flag = FALSE;
		return MMDEC_STREAM_ERROR;
	}

	Mp4Dec_Reset(vop_mode_ptr);

	Mp4Dec_InitBitstream(dec_input_ptr->pStream, dec_input_ptr->dataLen);//I think this is c model @2012_10_6

#else
    if(!Mp4Dec_GetCurRecFrameBfr(vop_mode_ptr))
    {
        memset(dec_output_ptr,0,sizeof(MMDecOutput));
        dec_output_ptr->frameEffective = 0;
	g_mpeg4_dec_err_flag |= 1;	
        return MMDEC_OUTPUT_BUFFER_OVERFLOW;
    }	
 
#endif



	if(ITU_H263 == vop_mode_ptr->video_std)
	{
		ret = Mp4Dec_DecH263Header(vop_mode_ptr,SliceInfo);
	}else if(VSP_MPEG4 == vop_mode_ptr->video_std)
	{
		vop_mode_ptr->find_vop_header  = 0;
		ret = Mp4Dec_DecMp4Header(vop_mode_ptr, dec_input_ptr->dataLen);
		if(!vop_mode_ptr->find_vop_header)
		{
#ifdef _VSP_LINUX_							
			dec_output_ptr->VopPredType = NVOP;
#endif
			return MMDEC_OK;
		}
	}else
	{
		ret = Mp4Dec_FlvH263PicHeader(vop_mode_ptr);
	}

	
	if(ret != MMDEC_OK)
	{
		//modified by xwluo, 20100511
		g_mpeg4_dec_err_flag |= 1<<1;		

		if (vop_mode_ptr->VopPredType == BVOP)
		{
			//here, error occur may be NO refereance frame for BVOP, so we skip this B frame.
			dec_output_ptr->VopPredType = NVOP;
			return MMDEC_OK;
		}else
		{
			return ret;
		}
	}

	if(dec_input_ptr->expected_IVOP && (vop_mode_ptr->VopPredType != IVOP))
	{
        if (g_nFrame_dec)
	    {
			g_nFrame_dec++;
	    }
		g_mpeg4_dec_err_flag |= 1<<2;			
		return MMDEC_FRAME_SEEK_IVOP;
	}
   


	dec_output_ptr->frameEffective = FALSE;//weihu	
	dec_output_ptr->pOutFrameY = PNULL;
	dec_output_ptr->pOutFrameU = PNULL;
	dec_output_ptr->pOutFrameV = PNULL;
	
# if 1 //SIM_IN_WIN	
    if(!vop_mode_ptr->bInitSuceess)
    {
        if( MMDEC_OK != Mp4Dec_InitSessionDecode(vop_mode_ptr) )
        {
			
            return MMDEC_MEMORY_ERROR;
        }
        vop_mode_ptr->bInitSuceess = TRUE;
    }
	
	memset(&SliceInfo,0,sizeof(SLICEINFO));
#else
	{
		if(!vop_mode_ptr->bInitSuceess)
		{
			vop_mode_ptr->MBNumX = (vop_mode_ptr->OrgFrameWidth  + 15) / MB_SIZE;
			vop_mode_ptr->MBNumY = (vop_mode_ptr->OrgFrameHeight + 15) / MB_SIZE;
			vop_mode_ptr->MBNum  = (int16)(vop_mode_ptr->MBNumX  * vop_mode_ptr->MBNumY);
			vop_mode_ptr->FrameWidth  = (int16)(vop_mode_ptr->MBNumX  * MB_SIZE);
			vop_mode_ptr->FrameHeight = (int16)(vop_mode_ptr->MBNumY  * MB_SIZE);
			
			vop_mode_ptr->MB_in_VOP_length = Mp4Dec_Compute_log2(vop_mode_ptr->MBNum);
			
			if(VSP_MPEG4 == vop_mode_ptr->video_std)
			{
				vop_mode_ptr->time_inc_resolution_in_vol_length = Mp4Dec_Compute_log2(vop_mode_ptr->time_inc_resolution);
			}
			
			vop_mode_ptr->post_filter_en = TRUE;//FALSE;
            vop_mode_ptr->sliceNumber=0;
			
			Mp4Dec_InitHuffmanTable(vop_mode_ptr);

			 g_is_need_init_vsp_hufftab=TRUE;
		}
		vop_mode_ptr->bInitSuceess = TRUE;
		
	}
#endif
     
	

	SliceInfo.video_std=vop_mode_ptr->video_std;
	SliceInfo.DataPartition=vop_mode_ptr->bDataPartitioning;
    SliceInfo.VOPCodingType=vop_mode_ptr->VopPredType;
	SliceInfo.ShortHeader=(ITU_H263 == vop_mode_ptr->video_std)?1:0;
	SliceInfo.Max_MBX=vop_mode_ptr->MBNumX;
	SliceInfo.Max_MBy=vop_mode_ptr->MBNumY;
	SliceInfo.IsRvlc=vop_mode_ptr->bReversibleVlc;;
	SliceInfo.QuantType=vop_mode_ptr->QuantizerType;
	SliceInfo.PicHeight=vop_mode_ptr->FrameHeight;
	SliceInfo.PicWidth=vop_mode_ptr->FrameWidth;
    SliceInfo.NumMbsInGob=vop_mode_ptr->NumMBInGob;
	SliceInfo.NumMbLineInGob=vop_mode_ptr->num_mbline_gob;
	SliceInfo.VopQuant=vop_mode_ptr->StepSize;
    SliceInfo.VOPFcodeFwd=vop_mode_ptr->mvInfoForward.FCode;
    SliceInfo.VOPFcodeBck=vop_mode_ptr->mvInfoBckward.FCode;
	SliceInfo.FirstMBx=0;
    SliceInfo.FirstMBy=0;
	SliceInfo.SliceNum=0;
   #if SIM_IN_WIN
	SliceInfo.pMbMode=vop_mode_ptr->pMbMode;
  #endif
    SliceInfo.VopRoundingType=(SliceInfo.VOPCodingType==PVOP)?vop_mode_ptr->RoundingControl:0;
    SliceInfo.IntraDCThr=vop_mode_ptr->IntraDcSwitchThr;


	
	if(vop_mode_ptr->VopPredType != NVOP)
	{
		//sorenson H263 and itu h.263 dont support B frame.
		if((BVOP == vop_mode_ptr->VopPredType) &&  (VSP_MPEG4 != vop_mode_ptr->video_std) )
		{
#ifdef _VSP_LINUX_		
			dec_output_ptr->VopPredType = NVOP;
#endif		
			return MMDEC_OK;
		}

		if(!Mp4Dec_GetCurRecFrameBfr(vop_mode_ptr))
		{
			g_mpeg4_dec_err_flag |= 1<<4;			
			return MMDEC_OUTPUT_BUFFER_OVERFLOW;
		}
		
		if (vop_mode_ptr->post_filter_en)
		{
			if(dec_input_ptr->beDisplayed)
			{
				if(!Mp4Dec_GetCurDispFrameBfr(vop_mode_ptr))
				{
					g_mpeg4_dec_err_flag |= 1<<5;					
					return MMDEC_OUTPUT_BUFFER_OVERFLOW;
				}
			}
		}else //for fpga verification
		{
			vop_mode_ptr->pCurDispFrame = vop_mode_ptr->pCurRecFrame;
		}

	}else //NVOP
	{
		PRINTF ("frame not coded!\n");
#ifdef _VSP_LINUX_
	{
		uint32 BitConsumed,ByteConsumed;
		OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");
		BitConsumed = OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR + TOTAL_BITS_OFF, "Read out BitConsumed. ") ;
		ByteConsumed = (BitConsumed + 7) >>3;
		
		if(ByteConsumed > dec_input_ptr->dataLen)
			dec_input_ptr->dataLen = 0;
		else
			dec_input_ptr->dataLen = dec_input_ptr->dataLen - ByteConsumed;

		dec_output_ptr->VopPredType = NVOP;
	}
#endif			
		return MMDEC_OK;
	}

////////////////////////////before is display ////////////////////////////////////
	
	Mp4Dec_InitVop(vop_mode_ptr, dec_input_ptr);

	{		
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG0_OFF, ((0x0)<<31)|((SliceInfo.VopQuant&0x3f)<<25)|((0&0x1ff)<<16)|((SliceInfo.Max_MBX*SliceInfo.Max_MBy)<<3)|SliceInfo.VOPCodingType&0x7,"VSP_CFG0");
       		 OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG1_OFF, (SliceInfo.NumMbsInGob&0x1ff)<<20|(SliceInfo.NumMbLineInGob&0x7)<<29,"VSP_CFG1");
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG2_OFF,(BVOP == vop_mode_ptr->VopPredType?0:1)<<31|(1-SliceInfo.VopRoundingType)<<30|0,"VSP_CFG2");
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG3_OFF,(SliceInfo.ShortHeader|SliceInfo.DataPartition<<1|((SliceInfo.IsRvlc&(SliceInfo.VOPCodingType!=BVOP))<<2)|
			SliceInfo.IntraDCThr<<3|SliceInfo.VOPFcodeFwd<<6|SliceInfo.VOPFcodeBck<<9|0<<12|SliceInfo.QuantType<<13),"VSP_CFG3");
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG4_OFF,((vop_mode_ptr->time_pp&0xffff)<<16|(vop_mode_ptr->time_bp&0xffff)),"VSP_CFG4");	
		//OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG3_OFF,vop_mode_ptr->time_pp&0xffff,"VSP_CFG3");
		if(vop_mode_ptr->time_pp==0)
		{
			OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG5_OFF,0,"VSP_CFG5");
		}
		else
		{
			OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG5_OFF,((1<<29)/vop_mode_ptr->time_pp),"VSP_CFG5");
		}

		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_MASK_OFF,0x0,"VSP_INT_MASK");//enable int
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 1,"RAM_ACC_SEL");//change ram access to vsp hw
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_START_OFF,0xa|g_is_need_init_vsp_hufftab,"VSP_START");//start vsp   vld/vld_table//load_vld_table_en
	//	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+MCU_SLEEP_OFF,0x1,"MCU_SLEEP");//MCU_SLEEP
	#if SIM_IN_WIN
		FPRINTF(g_fp_global_tv,"//***********************************frame num=%d slice id=%d\n",g_nFrame_dec,0);
		OR1200_Vaild=0;
	#endif
		g_is_need_init_vsp_hufftab = TRUE;
		g_is_need_init_vsp_dcttab= FALSE;
	}	


#if SIM_IN_WIN	
		if(IVOP == vop_mode_ptr->VopPredType)
		{
			PRINTF ("\t I VOP\t%d\n", dec_input_ptr->dataLen); 
	
			if(!vop_mode_ptr->bDataPartitioning)
			{
				ret = Mp4Dec_DecIVOP(vop_mode_ptr,PPAParaBuf,&SliceInfo); 
			}
 //#ifdef _MP4CODEC_DATA_PARTITION_	
			else
			{
				ret = Mp4Dec_DecIVOPErrResDataPartitioning(vop_mode_ptr,PPAParaBuf,&SliceInfo);
			}
		
			if (dec_input_ptr->err_pkt_num || vop_mode_ptr->err_MB_num)
			{
				
				Mp4Dec_Reset(vop_mode_ptr);
				ret =Mp4Dec_VspFrameInit(vop_mode_ptr);
				VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_CFG_OFF, vop_mode_ptr->RoundingControl << 1, "configure MCA register: MCA CFG");
				Mp4Dec_EC_IVOP(vop_mode_ptr);
			}
//#endif //_MP4CODEC_DATA_PARTITION_


	
	}else if(PVOP == vop_mode_ptr->VopPredType)
	{

		PRINTF ("\t P VOP\t%d\n", dec_input_ptr->dataLen); 

		if (vop_mode_ptr->pBckRefFrame->pDecFrame == PNULL)
		{
			if( (PNULL != vop_mode_ptr->pCurDispFrame) && 
				(PNULL != vop_mode_ptr->pCurDispFrame->pDecFrame) && 
				(PNULL != vop_mode_ptr->pCurDispFrame->pDecFrame->imgY)
			  )
			{
				MPEG4_DecReleaseDispBfr(vop_mode_ptr->pCurDispFrame->pDecFrame->imgY);
			}
			return MMDEC_ERROR;
		}

		VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_CFG_OFF, vop_mode_ptr->RoundingControl << 1, "configure MCA register: MCA CFG");

		if(!vop_mode_ptr->bDataPartitioning)
		{
			ret = Mp4Dec_DecPVOP(vop_mode_ptr,PPAParaBuf,&SliceInfo) ;
		}
//	#ifdef _MP4CODEC_DATA_PARTITION_	
		else
		{
			ret = Mp4Dec_DecPVOPErrResDataPartitioning(vop_mode_ptr,PPAParaBuf,&SliceInfo);
		}
//	#endif //_MP4CODEC_DATA_PARTITION_	

		if (dec_input_ptr->err_pkt_num || vop_mode_ptr->err_MB_num)
		{
			
			Mp4Dec_Reset(vop_mode_ptr);
			ret = Mp4Dec_VspFrameInit(vop_mode_ptr);
			VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_CFG_OFF, vop_mode_ptr->RoundingControl << 1, "configure MCA register: MCA CFG");
			Mp4Dec_EC_PVOP(vop_mode_ptr);
		}
			

	}else if(SVOP == vop_mode_ptr->VopPredType)
	{
		PRINTF ("\t S VOP, Don't SUPPORTED!\n"); 
//		Mp4Dec_DecPVOP(vop_mode_ptr);  //removed by Xiaowei.Luo, because SC6800H don't support GMC
	}else if(NVOP == vop_mode_ptr->VopPredType)
	{
		PRINTF ("frame not coded!\n");
        MPEG4_DecReleaseDispBfr(vop_mode_ptr->pCurDispFrame->pDecFrame->imgY);
		return MMDEC_OK;
	}else
	{
		PRINTF ("\t B VOP\t%d\n", dec_input_ptr->dataLen);
		vop_mode_ptr->RoundingControl = 0; //Notes: roundingctrol is 0 in B-VOP.
		vop_mode_ptr->err_MB_num = 0;

		Mp4Dec_DecBVOP(vop_mode_ptr,PPAParaBuf, &SliceInfo);
	}

	if(READ_REG_POLL(VSP_DBK_REG_BASE+DBK_CTR1_OFF, V_BIT_0, 0, TIME_OUT_CLK, "DBK_CTR1: polling DBK_CFG_FLAG flag = 0"))
	{
		vop_mode_ptr->error_flag = TRUE;
		return MMDEC_HW_ERROR;
	}
		
	if(READ_REG_POLL(VSP_AHBM_REG_BASE+AHBM_STS_OFFSET, V_BIT_0, 0, TIME_OUT_CLK, "AHBM_STS: polling AHB idle status"))
	{
		PRINTF("TIME OUT!\n");
		return MMDEC_HW_ERROR;
	}
	
	VSP_WRITE_REG(VSP_GLB_REG_BASE+VSP_TST_OFF, g_nFrame_dec, "VSP_TST: configure frame_cnt to debug register for end of picture");
/////////////////////////end
#ifdef MD5_CHK
	{
	uint32 yDataLen = vop_mode_ptr->FrameWidth * vop_mode_ptr->FrameHeight;
	uint32 uvDataLen = yDataLen/2;
	Get_MD5_Code(vop_mode_ptr->pCurDispFrame->pDecFrame->imgY,  
				yDataLen, dec_output_ptr->ycode);
	Get_MD5_Code(vop_mode_ptr->pCurDispFrame->pDecFrame->imgU,
				uvDataLen, dec_output_ptr->uvcode);

#if defined(_FPGA_AUTO_VERIFICATION_) 
	waiting_n_ms(60);
	while(!read_pc_finished_dec_one_frame_msg())
	{
		;
	}
		

	send_one_frm_data(dec_output_ptr->ycode, dec_output_ptr->uvcode, 0
					16,8);

#endif //_FPGA_AUTO_VERIFICATION_	
	}
#endif
#else
			mpeg4_decode_vop(vop_mode_ptr);//wait hw return
#endif	



#if SIM_IN_WIN
	{	
	
		Mp4DecStorablePic *pic = PNULL;
		
		/*output frame for display*/
		if(vop_mode_ptr->VopPredType != BVOP)
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


		
		{
			int32 i,j;

			uint32 *img=(uint32*)(vop_mode_ptr->pCurRecFrame->pDecFrame->imgY);

			FPRINTF (g_fp_rec_frm_tv,"//frame_no=%d\n",g_nFrame_dec);
			for(i=0;i<vop_mode_ptr->FrameHeight;i++)
			{
				for(j=0;j<(vop_mode_ptr->FrameWidth/8);j++)
				{
					FPRINTF (g_fp_rec_frm_tv, "%08x%08x\n",*(img+2*j+1+i*vop_mode_ptr->FrameWidth/4),*(img+2*j+i*vop_mode_ptr->FrameWidth/4));
				}
			}
			
			img=(uint32*)(vop_mode_ptr->pCurRecFrame->pDecFrame->imgU);
			for(i=0;i<(vop_mode_ptr->FrameHeight/2);i++)
			{
				for(j=0;j<(vop_mode_ptr->FrameWidth/8);j++)
				{
					FPRINTF (g_fp_rec_frm_tv, "%08x%08x\n",*(img+2*j+1+i*vop_mode_ptr->FrameWidth/4),*(img+2*j+i*vop_mode_ptr->FrameWidth/4));
				}
			}
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
			

//ONLY FOR AUTO FPGA VERIFICATION, BEGIN
		#if _CMODEL_&& defined(_FPGA_AUTO_VERIFICATION_)	
				MPEG4_DecReleaseDispBfr(dec_output_ptr->pOutFrameY); //only for AUTO FPGA VERIFICATION
		#endif
//ONLY FOR AUTO FPGA VERIFICATION, END		

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
				dec_output_ptr->is_transposed = 0;
				dec_output_ptr->frame_width = vop_mode_ptr->FrameWidth;
				dec_output_ptr->frame_height = vop_mode_ptr->FrameHeight;
				dec_output_ptr->frameEffective = 1;
				dec_output_ptr->err_MB_num = vop_mode_ptr->err_MB_num;

		//		pic->pDecFrame->bDisp = TRUE; 
			}else
			{
				dec_output_ptr->frame_width = vop_mode_ptr->FrameWidth;
				dec_output_ptr->frame_height = vop_mode_ptr->FrameHeight;
				dec_output_ptr->frameEffective = 0;
			}
		}

		dec_output_ptr->pOutFrameY = vop_mode_ptr->pCurDispFrame->pDecFrame->imgY;
		dec_output_ptr->pOutFrameU = vop_mode_ptr->pCurDispFrame->pDecFrame->imgU;
		dec_output_ptr->pOutFrameV = vop_mode_ptr->pCurDispFrame->pDecFrame->imgV;
//ONLY FOR AUTO FPGA VERIFICATION, END		
           free(PPAParaBuf);	
           Mp4Dec_exit_picture(vop_mode_ptr);
	}

#else
	Mp4Dec_output_one_frame (dec_output_ptr, vop_mode_ptr);	
	
	
	Mp4Dec_exit_picture(vop_mode_ptr);



#ifdef _VSP_LINUX_
{
	uint32 BitConsumed,ByteConsumed;
	OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");
	BitConsumed = OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR + TOTAL_BITS_OFF, "Read out BitConsumed. ") ;
	ByteConsumed = (BitConsumed + 7) >>3;
	
	if(ByteConsumed > dec_input_ptr->dataLen)
		dec_input_ptr->dataLen = 0;
	else
		dec_input_ptr->dataLen = dec_input_ptr->dataLen - ByteConsumed;

	dec_output_ptr->VopPredType = NVOP;
	}
#endif
#endif
	vop_mode_ptr->pre_vop_type = vop_mode_ptr->VopPredType;
	g_nFrame_dec++;
	
	return ret;
}

/*****************************************************************************/
//  Description: for display, return one frame for display
//	Global resource dependence: 
//  Author:        
//	Note:  the transposed type is passed from MMI "req_transposed"
//         req_transposed£º 1£ºtranposed  0: normal    
/*****************************************************************************/
# if SIM_IN_WIN
PUBLIC void mpeg4dec_GetOneDspFrm(MMDecOutput *pOutput, int req_transposed, int is_last_frame)
{
	DEC_VOP_MODE_T *vop_mode_ptr = Mp4Dec_GetVopmode();
	Mp4DecStorablePic *pic;

	if(is_last_frame)
	{
		//notes: if the last frame is bvop, then the last display frame should be it's latest reference frame(pBckRefFrame).
		//else, the last display frame should be last decoded I or P vop. In MP4DecDecode function, Mp4Dec_exit_picture has 
		//exchanged pCurRecFrame to pBckRefFrame. so here use pBckRefFrame is correct.
		pic = vop_mode_ptr->pBckRefFrame;
	}else
	{
		pic = vop_mode_ptr->pFrdRefFrame;
	}

	if(PNULL != pic)
	{
		pic->pDecFrame->bDisp = TRUE; 
		pOutput->frame_width = vop_mode_ptr->FrameWidth;
		pOutput->frame_height = vop_mode_ptr->FrameHeight;
		pOutput->frameEffective = 1;
		pOutput->is_transposed = 0;
		pOutput->pOutFrameY = pic->pDecFrame->imgY;
		pOutput->pOutFrameU = pic->pDecFrame->imgU;
		pOutput->pOutFrameV = pic->pDecFrame->imgV;
	}else
	{
		pOutput->frameEffective = 0;
	}

	return;
}
#endif

/*****************************************************************************/
//  Description: check whether VSP can used for video decoding or not
//	Global resource dependence: 
//  Author:        
//	Note: return VSP status:
//        1: dcam is idle and can be used for vsp   0: dcam is used by isp           
/*****************************************************************************/
#if SIM_IN_WIN
BOOLEAN MPEG4DEC_VSP_Available(void)
{
	int dcam_cfg;

	dcam_cfg = VSP_READ_REG(VSP_DCAM_BASE+DCAM_CFG_OFF, "DCAM_CFG: read dcam configure register");

	if (((dcam_cfg >> 3) & 1) == 0) //bit3: VSP_EB, xiaowei.luo@20090907
		return TRUE;
	else
		return FALSE;
}
#endif

MMDecRet MP4DecRelease(void)
{
#if _CMODEL_
	VSP_Delete_CModel();
#endif

	return MMDEC_OK;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 

/******************************************************************************
 ** File Name:    h264dec_slice.c                                             *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         01/23/2007                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
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
#include "sc8800g_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif


PUBLIC void H264Dec_SeqLevelConfig (DEC_IMAGE_PARAMS_T *img_ptr)
{
	return;
}

PUBLIC MMDecRet h264Dec_PicLevelSendRefAddressCommmand (DEC_IMAGE_PARAMS_T *img_ptr)
{
	int32 i;
	uint32 cmd;
	uint32 pTableAddr = (uint32)VSP_MEMO10_ADDR;

	if(READ_REG_POLL(VSP_DBK_REG_BASE+DBK_DEBUG_OFF, V_BIT_17, V_BIT_17, TIME_OUT_CLK, "DBK_DEBUG: polling slice idle (WAIT_MBC_DONE or Idle) status"))
	{
		img_ptr->error_flag |= ER_DBK_ID;
        img_ptr->return_pos |= (1<<5);
        H264Dec_get_HW_status(img_ptr);
		return MMDEC_HW_ERROR;
	}

	if(READ_REG_POLL(VSP_AHBM_REG_BASE+AHBM_STS_OFFSET, V_BIT_0, 0, TIME_OUT_CLK, "AHBM_STS: polling AHB idle status"))
	{
		PRINTF("TIME OUT!\n");
        img_ptr->error_flag |= ER_AHB_ID;
        img_ptr->return_pos |= (1<<6);
        H264Dec_get_HW_status(img_ptr);
		return MMDEC_HW_ERROR;
	}

	open_vsp_iram();
    if (g_dec_picture_ptr == NULL)	//added by xw, 20100526, for mb2frm bug.
	{
		img_ptr->error_flag |= ER_PICTURE_NULL_ID;
        img_ptr->return_pos |= (1<<7);
        H264Dec_get_HW_status(img_ptr);
		return MMDEC_ERROR;
	}
	VSP_WRITE_REG(pTableAddr+ 0, g_dec_picture_ptr->imgYAddr, "configure reconstruct frame Y");
#if _CMODEL_ //for RTL simulation
	VSP_WRITE_REG(pTableAddr+ 4, ((uint32)INTRA_PREDICTION_ADDR)>>8, "Top MB bottom data block first line pixel data for IPRED prediction");
#else

#ifdef _VSP_LINUX_
	VSP_WRITE_REG(pTableAddr+ 4, ((uint32)H264Dec_ExtraMem_V2Phy(img_ptr->ipred_top_line_buffer))>>8, "Top MB bottom data block first line pixel data for IPRED prediction");
#else
	VSP_WRITE_REG(pTableAddr+ 4, ((uint32)img_ptr->ipred_top_line_buffer)>>8, "Top MB bottom data block first line pixel data for IPRED prediction");
#endif

#endif
	for (i = 0; i < MAX_REF_FRAME_NUMBER; i++)
	{
		VSP_WRITE_REG(pTableAddr+ (4+(uint32)i)*4, g_list0[i]->imgYAddr, "configure reference picture");
		//SCI_TRACE_LOW("imgYAddr %d:%x",i,g_list0[i]->imgYAddr);
	}
	close_vsp_iram();

	cmd = VSP_READ_REG (VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, "read out Endian Info") & 0xf;
	cmd |= ((img_ptr->width*img_ptr->height/4) << 4);
	VSP_WRITE_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, cmd, "configure uv offset");

	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_SRC_SIZE_OFF, img_ptr->width, "configure frame width");
	
	return MMDEC_OK;
}

PUBLIC void H264Dec_mb_level_sync (DEC_IMAGE_PARAMS_T *img_ptr)
{
#if _CMODEL_
	mbc_module();
#endif

	//check the mbc done flag, or time out flag; if time out, error occur then reset the vsp
	if(READ_REG_POLL(VSP_MBC_REG_BASE+MBC_ST0_OFF, V_BIT_5, V_BIT_5, TIME_OUT_CLK, "MBC: polling mbc done"))
	{
		img_ptr->error_flag |= ER_MBC_ID;
        img_ptr->return_pos |= (1<<8);
        H264Dec_get_HW_status(img_ptr);
		return;
	}

    //check the mbc done flag, or time out flag; if time out, error occur then reset the vsp	
	if(VSP_READ_REG(VSP_MBC_REG_BASE+MBC_ST0_OFF, "MBC: ipred error flag") & 0x10000)
	{
		img_ptr->error_flag |= ER_IPRED_ID;
        img_ptr->return_pos |= (1<<9);
        H264Dec_get_HW_status(img_ptr);    
		return;
	}
	
	VSP_WRITE_REG(VSP_MBC_REG_BASE+MBC_ST0_OFF, V_BIT_5, "clear MBC done flag");

	//hold next mb's mbc by setting mb type to be intra
	VSP_WRITE_REG(VSP_MBC_REG_BASE+MBC_CMD0_OFF, (0<<30), "configure mb information.");

#if _CMODEL_
	dbk_module();
#endif	

	return;
}

PUBLIC MMDecRet H264Dec_Picture_Level_Sync (DEC_IMAGE_PARAMS_T *img_ptr)
{
	if(READ_REG_POLL(VSP_DBK_REG_BASE+DBK_DEBUG_OFF, V_BIT_16, V_BIT_16, TIME_OUT_CLK, "DBK_CTR1: polling Frame idle status"))
	{
		img_ptr->error_flag |= ER_DBK_ID;
        img_ptr->return_pos |= (1<<10);
        H264Dec_get_HW_status(img_ptr);
		return MMDEC_HW_ERROR;
	}
		
	if(READ_REG_POLL(VSP_AHBM_REG_BASE+AHBM_STS_OFFSET, V_BIT_0, 0, TIME_OUT_CLK, "AHBM_STS: polling AHB idle status"))
	{
		PRINTF("TIME OUT!\n");
		img_ptr->error_flag |= ER_AHB_ID;
        img_ptr->return_pos |= (1<<11);
        H264Dec_get_HW_status(img_ptr);
		return MMDEC_HW_ERROR;
	}
	
	if (!img_ptr->fmo_used)
	{
		VSP_WRITE_REG(VSP_GLB_REG_BASE+VSP_TST_OFF, g_nFrame_dec_h264, "VSP_TST: configure frame_cnt to debug register for end of picture");
	}

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

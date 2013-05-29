/******************************************************************************
 ** File Name:    common_top.c                                                *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         11/19/2008                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:   c model of bsmr module in mpeg4 decoder                    *
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

uint32 * vsp_quant_tab;			//64x32
uint32 * vsp_huff_dcac_tab;		//196x32
int32  * vsp_dct_io_0;			//216x32
int32  * vsp_dct_io_1;			//216x32
uint32 * vsp_fw_mca_out_bfr;	//96x32
uint32 * vsp_bw_mca_out_bfr;	//96x32
uint32 * vsp_mbc_out_bfr;		//216x32
uint32 * vsp_dbk_out_bfr;		//216x32
uint32 * vsp_mea_out_bfr;		//96*32
uint32 * vsp_frame_addr_bfr;	//128*32

PUBLIC  void VSP_InitBfrAddr(void)
{
	vsp_quant_tab		= (uint32 *)safe_malloc(sizeof(uint32) * QUANT_TABLE_BFR_SIZE);
#if defined (H264_DEC)
	vsp_huff_dcac_tab	= g_hvld_huff_tab;
#else
	vsp_huff_dcac_tab   = (uint32 *)safe_malloc(sizeof(uint32) * HUFF_DCAC_BFR_SIZE);
#endif
	vsp_dct_io_0		= (int32  *)safe_malloc(sizeof(uint32) * DCT_IO_BFR_SIZE);
	vsp_dct_io_1		= (int32  *)safe_malloc(sizeof(uint32) * DCT_IO_BFR_SIZE);
	vsp_fw_mca_out_bfr	= (int32  *)safe_malloc(sizeof(uint32) * MCA_BFR_SIZE);
	vsp_bw_mca_out_bfr	= (int32  *)safe_malloc(sizeof(uint32) * MCA_BFR_SIZE);
	vsp_mbc_out_bfr		= (int32  *)safe_malloc(sizeof(uint32) * MBC_OUT_BFR_SIZE);
	vsp_dbk_out_bfr		= (int32  *)safe_malloc(sizeof(uint32) * MBC_OUT_BFR_SIZE);
	vsp_mea_out_bfr		= (int32  *)safe_malloc(sizeof(uint32) * MEA_OUT_BFR_SIZE);
	vsp_frame_addr_bfr	= (int32  *)safe_malloc(sizeof(uint32) * FRAME_ADDR_BFR_SIZE);
}

/*****************************************************************************
 **	Name : 			VSP_DelBfrAddr
 ** Description:	release vsp internal buffer.
 ** Author:			Binggo Zhou
 **	Note:			
 *****************************************************************************/
PUBLIC  void VSP_DelBfrAddr(void)
{
	SAFE_FREE(vsp_quant_tab);
#if defined (H264_DEC)
#else
	SAFE_FREE(vsp_huff_dcac_tab);
#endif		
	SAFE_FREE(vsp_dct_io_0); 
	SAFE_FREE(vsp_dct_io_1); 
	SAFE_FREE(vsp_fw_mca_out_bfr);
	SAFE_FREE(vsp_bw_mca_out_bfr);
	SAFE_FREE(vsp_mbc_out_bfr);
	SAFE_FREE(vsp_dbk_out_bfr);
	SAFE_FREE(vsp_mea_out_bfr);
	SAFE_FREE(vsp_frame_addr_bfr);
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
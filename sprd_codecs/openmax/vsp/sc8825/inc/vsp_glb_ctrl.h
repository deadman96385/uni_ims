/******************************************************************************
 ** File Name:      vsp_vld.h	                                              *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           06/11/2009                                                *
 ** Copyright:      2009 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    VSP VLD Module Driver									  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 06/11/2009    Xiaowei Luo     Create.                                     *
 ** 06/26/2012   Leon Li             modify
 *****************************************************************************/
#ifndef _VSP_GLB_CTRL_H_
#define _VSP_GLB_CTRL_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif 
#define VSP_GLB_CTRL_REG_BASE	(VSP_CTRL_BASE + 0x2400)
#define VSP_GLB_CTRL_REG_SIZE	0xc

#define MP4ENC_GLB_CTRL_OFFSET				0x00
#define MP4ENC_MEA_CFG0_OFF			(0x04)
#define MP4ENC_MEA_CFG1_OFF			(0x08)

typedef struct vsp_glb_ctrl_reg_tag
{
	volatile uint32 VSP_MP4ENC_CTRL_CFG;			//[31:19]:	reserved
											//[18:15]:	MB_In_VOP_length
											//[14:12]:	fcode
											//[11:9]: 	vop_pred_type
											//[8:6]:		mbLine_num_gob
											//[5]:		is_short_header
											//[4:0]:		QP

											
	volatile uint32 VSP_MP4ENC_MEA_CFG0;			//[31:25]: reserved
										//[24]: PRE_FLT_ENA, MEA source MB data pre-filtering enable, active high
										//[23:16]: PRE_FLT_TH, MEA pre-filter threshold value.
										//[12:8]: search_rangeY -- size of search window. maxium 63 pixels in vertical component.
										//[7:5]: reserved
										//[4:0]:search_rangeX -- size of search window. maxium 63 pixels in horizontal component.   
	
	volatile uint32 VSP_MP4ENC_MEA_CFG1;			//[31:16]:mea_sad_thres -- the rhreshold of minimum sad value.
										//[15:8]: max_search_step -- max. search steps for ds algorithm
										//[7:6]: reserved
										//[5]: auto_en -- 0: mea controlled by sw; 1: used hardware pipeline
										//[4]: prediction_en -- nss search algorithm enable
										//[3]: mea_test -- 0: motion estimation will be started at(0,0), 1: start at the prediction point
										//[2]: 4mv_en -- 0: disalbe search in 8x8 mode; 1: enable 8x8 mode
										//[1]: intra_en -- 0: disable intra sad calculation; 1: enable intra sad calculation
										//[0]: me_enable -- 0: disable motion estimation; 1: enable motion estimaion


}VSP_GLB_CTRL_REG_T;


/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif 
/**---------------------------------------------------------------------------*/
// End 
#endif  //_VSP_GLB_CTRL_H_

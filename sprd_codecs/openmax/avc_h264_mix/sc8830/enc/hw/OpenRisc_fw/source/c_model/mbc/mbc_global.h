/******************************************************************************
 ** File Name:      mbc_global.h                                              *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           11/20/2007                                                *
 ** Copyright:      2007 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    VSP bsm Driver for video codec.	  						  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 11/20/2007    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _MBC_GLOBAL_H_
#define _MBC_GLOBAL_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "video_common.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

//byte
#define MBC_Y_SIZE	24	
#define MBC_C_SIZE	16	

//yuv componences offset in mbc buffer
#define MBC_Y_OFFSET		0
#define MBC_U_OFFSET		(120*4)	//6*5*4
#define MBC_V_OFFSET		(168*4)	//120+3*4*4

//current mb data offset in mbc buffer
#define MBC_Y_DATA_OFFSET	(MBC_Y_SIZE*4+2*4) //byte
#define MBC_C_DATA_OFFSET (MBC_C_SIZE*4+2*4) //byte
		
typedef void (*DOWN_SAMPLE)(uint8 *pSrc, uint8 *pDst, int32 width, int32 height);

extern DOWN_SAMPLE		g_downsample[3];
extern int32 g_blk_rec_ord_tbl [16];

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
void mp4_add_ref_and_residual (void);
void h264_add_ref_and_residual (void);
void printf_mbc_mb (FILE * pfMBC);
void Init_downsample_fun();

//////////////////////////////////////////////////////////////////////////
/////						FORMAT CONVERTION						//////
//////////////////////////////////////////////////////////////////////////
void format_convertion_444to422();
void format_convertion_411to422();
void format_convertion_411Rto420();
void format_convertion_422Rto420();
void format_convertion_422to422();
void format_convertion_420to420();
void format_convertion_400to400();

//////////////////////////////////////////////////////////////////////////
/////						DOWN SAMPLE								//////
//////////////////////////////////////////////////////////////////////////
void downsample_org444(int32 scale_down_factor);
void downsample_org411(int32 scale_down_factor);
void downsample_org411R(int32 scale_down_factor);
void downsample_org422R(int32 scale_down_factor);
void downsample_org422(int32 scale_down_factor);
void downsample_org420(int32 scale_down_factor);
void downsample_org400(int32 scale_down_factor);

void init_mbc_module(void);
void mbc_module(void);
PUBLIC int32 READ_REG_MBC_ST0_REG(uint32 addr, uint32 mask,uint32 exp_value, char *pstring);
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_MBC_GLOBAL_H_
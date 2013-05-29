/******************************************************************************
 ** File Name:      buffer_global.h	                                          *
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
#ifndef _BUFFER_GLOBAL_H_
#define _BUFFER_GLOBAL_H_

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

#define	QUANT_TABLE_BFR_SIZE		(96)
#define	HUFF_DCAC_BFR_SIZE			(400)//james//(196)
#define DCT_IO_BFR_SIZE				(256) //(216)
#define MCA_BFR_SIZE				(96)
#define MBC_OUT_BFR_SIZE			(216)//(172)
#define MEA_OUT_BFR_SIZE			(96)
#define FRAME_ADDR_BFR_SIZE			(128)

/************************************************************************************************************************
0~151: vld table
one word contains one intra and one inter symbol   31~16 for intra, 15:0 for inter
16bits:		| last  |  run    | level   |  length|
			   15     14~9       8~4        3~0

#define OFFSET_PRX0	0x0         //offset of prefix of 0 consecutive 0
#define OFFSET_PRX1	0x8			//offset of prefix of 1 consecutive 0
#define OFFSET_PRX2	0x18		//offset of prefix of 2 consecutive 0
#define OFFSET_PRX3	0x28		//offset of prefix of 3 consecutive 0
#define OFFSET_PRX4	0x48		//offset of prefix of 4 consecutive 0
#define OFFSET_PRX5	0x68		//offset of prefix of 5 consecutive 0
#define OFFSET_PRX6	0x88		//offset of prefix of 6 consecutive 0
#define OFFSET_PRX7	0x90		//offset of prefix of 7 consecutive 0
#define OFFSET_PRX8	0x94		//offset of prefix of 8 consecutive 0

152~167: left DC/AC coefficient(first column), 152~155: y block1, 156~159: y block3, 160~163: u block, 164~167: v block

168~183: top DC/AC coefficient(first row), 168~171: y block2, 172~175: y block3, 176~179: u block, 180~183: v block
************************************************************************************************************************/
extern uint32 * vsp_quant_tab;			
extern uint32 * vsp_huff_dcac_tab;	//huffman table, and dc/ac pred coefficient buffer	
extern int32  * vsp_dct_io_0;
extern int32  * vsp_dct_io_1;			
extern uint32 * vsp_fw_mca_out_bfr;	
extern uint32 * vsp_bw_mca_out_bfr;	
extern uint32 * vsp_mbc_out_bfr;		
extern uint32 * vsp_dbk_out_bfr;
extern uint32 * vsp_mea_out_bfr;
extern uint32 * vsp_frame_addr_bfr;

PUBLIC  void VSP_InitBfrAddr(void);
PUBLIC  void VSP_DelBfrAddr(void);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_BUFFER_GLOBAL_H_

/******************************************************************************
 ** File Name:      vsp_global_define.h                                       *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           06/11/2009                                                *
 ** Copyright:      2009 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    VSP MEA Module Driver									  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 06/11/2009    Xiaowei Luo     Create.                                     *
 ** 06/26/2012   Leon Li             modify
 *****************************************************************************/
#ifndef _VSP_GLOBAL_DEFINE_H_
#define _VSP_GLOBAL_DEFINE_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "vsp_dcam.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif 

//#define CHIP_ENDIAN_LITTLE

/*JPEG MCU FORMAT*/
typedef enum {
		JPEG_FW_YUV420 = 0, 
		JPEG_FW_YUV411, 
		JPEG_FW_YUV444, 
		JPEG_FW_YUV422, 
		JPEG_FW_YUV400, 
		JPEG_FW_YUV422_R, 
		JPEG_FW_YUV411_R
		}JPEG_MCU_FORMAT_E;

#define	VSP_MEMO0_ADDR				(VSP_DCAM_REG_BASE + 0x2000)
#define	VSP_MEMO1_ADDR				(VSP_DCAM_REG_BASE + 0x3000)
#define	VSP_MEMO2_ADDR				(VSP_DCAM_REG_BASE + 0x4000)
#define	VSP_MEMO3_ADDR				(VSP_DCAM_REG_BASE + 0x5000)
#define	VSP_MEMO4_ADDR				(VSP_DCAM_REG_BASE + 0x6000)
#define	VSP_MEMO5_ADDR				(VSP_DCAM_REG_BASE + 0x7000)
#define	VSP_MEMO6_ADDR				(VSP_DCAM_REG_BASE + 0x8000)
#define	VSP_MEMO7_ADDR				(VSP_DCAM_REG_BASE + 0x9000)
#define	VSP_MEMO8_ADDR				(VSP_DCAM_REG_BASE + 0xa000)
#define	VSP_MEMO9_ADDR				(VSP_DCAM_REG_BASE + 0xe000)
//#define	VSP_MEMO10_ADDR				(VSP_DCAM_REG_BASE + 0x1000)
#define VSP_MEMO10_ADDR				(VSP_CTRL_BASE + 0x2800)
#define	VSP_MEMO11_ADDR				(VSP_DCAM_REG_BASE + 0x1400)
#define	VSP_MEMO12_ADDR				(VSP_DCAM_REG_BASE + 0x1800)
#define	VSP_MEMO13_ADDR				(VSP_DCAM_REG_BASE + 0xb000)
#define	VSP_MEMO14_ADDR				(VSP_DCAM_REG_BASE + 0xc000)
#define	VSP_MEMO15_ADDR				(VSP_DCAM_REG_BASE + 0x1c00)
#define	VSP_MEMO16_ADDR				(VSP_DCAM_REG_BASE + 0x0400)
#define	VSP_MEMO17_ADDR				(VSP_DCAM_REG_BASE + 0x0800)

#define MEMO0_ADDR_SIZE				(128*4)
#define MEMO1_ADDR_SIZE				(128*4)
#define MEMO2_ADDR_SIZE				(128*4)
#define MEMO3_ADDR_SIZE				(128*4)
#define MEMO4_ADDR_SIZE				(128*4)
#define MEMO5_ADDR_SIZE				(64*4)
#define MEMO6_ADDR_SIZE				(240*4)
#define MEMO7_ADDR_SIZE				(216*4)
#define MEMO8_ADDR_SIZE				(216*4)
#define MEMO9_ADDR_SIZE				(1536*8)
#define MEMO10_ADDR_SIZE			(64*4)
#define MEMO11_ADDR_SIZE			(64*4)
#define MEMO12_ADDR_SIZE			(128*4)
#define MEMO13_ADDR_SIZE			(1024*4)
#define MEMO14_ADDR_SIZE			(1024*4)
#define MEMO15_ADDR_SIZE			(64*4)
#define MEMO16_ADDR_SIZE			(64*4)
#define MEMO17_ADDR_SIZE			(256*4)

//COMMON
#define HUFFMAN_TBL_ADDR			VSP_MEMO14_ADDR	
#define QUANT_TBL_ADDR				VSP_MEMO0_ADDR

//JPEG ENC
#define JENC_QUANT_TBL_ADDR			QUANT_TBL_ADDR
#define JENC_SRC_MCU_BFR0_ADDR		VSP_MEMO1_ADDR
#define JENC_SRC_MCU_BFR1_ADDR		VSP_MEMO2_ADDR
#define JENC_VLC_TBL_ADDR			HUFFMAN_TBL_ADDR
#define JENC_DCTIO_BFR_ADDR			VSP_MEMO6_ADDR

//JPEG DEC
#define JDEC_IQUANT_TBL_ADDR		QUANT_TBL_ADDR
#define JDEC_VLD_TBL_ADDR			HUFFMAN_TBL_ADDR
#define JDEC_IDCTIO_BFR_ADDR		VSP_MEMO6_ADDR
#define JDEC_MBC_OUT_BFR0_ADDR		VSP_MEMO7_ADDR
#define JDEC_MBC_OUT_BFR1_ADDR		VSP_MEMO8_ADDR

//MPEG4 DEC
#define MP4DEC_IQUANT_TBL_ADDR		QUANT_TBL_ADDR
#define MP4DEC_MCA_FW_BFR_ADDR		VSP_MEMO3_ADDR
#define MP4DEC_MCA_BW_BFR_ADDR		VSP_MEMO4_ADDR
#define MP4DEC_VLD_BFR_ADDR			HUFFMAN_TBL_ADDR
#define MP4DEC_IDCTIO_BFR_ADDR		VSP_MEMO6_ADDR
#define MP4DEC_MBC_OUT_BFR0_ADDR	VSP_MEMO7_ADDR
#define MP4DEC_MBC_OUT_BFR1_ADDR	VSP_MEMO8_ADDR

//MPEG4 ENC
#define MP4ENC_MEA_REF_UV_BFR_ADDR	VSP_MEMO0_ADDR
#define MP4ENC_MEA_SRC_BFR0_ADDR	VSP_MEMO1_ADDR
#define MP4ENC_MEA_SRC_BFR1_ADDR	VSP_MEMO2_ADDR
#define MP4ENC_MEA_OUT_BFR0_ADDR	VSP_MEMO3_ADDR
#define MP4ENC_MEA_OUT_BFR1_ADDR	VSP_MEMO4_ADDR
#define MP4ENC_VLC_BFR_ADDR			HUFFMAN_TBL_ADDR
#define MP4ENC_DCTIO_BFR_ADDR		VSP_MEMO6_ADDR
#define MP4ENC_MBC_OUT_BFR0_ADDR	VSP_MEMO7_ADDR
#define MP4ENC_MBC_OUT_BFR1_ADDR	VSP_MEMO8_ADDR
#define MP4ENC_MEA_REF_Y_BFR_ADDR	VSP_MEMO9_ADDR
#define MP4ENC_MV_PREDICTION_BFR_ADDR	VSP_MEMO13_ADDR
#define MP4ENC_MEA_FETCH_BFR0_ADDR	VSP_MEMO16_ADDR
#define MP4ENC_MEA_FETCH_BFR1_ADDR	VSP_MEMO17_ADDR

//H.264 DEC
#define H264DEC_MCA_FLT0_PROC_BFR0		VSP_MEMO0_ADDR
#define H264DEC_MCA_FLT1_PROC_BFR0		VSP_MEMO1_ADDR
#define H264DEC_MCA_FLT1_PROC_BFR1		VSP_MEMO2_ADDR
#define H264DEC_MCA_MB_OUT_BFR0	VSP_MEMO3_ADDR
#define H264DEC_MCA_MB_OUT_BFR1	VSP_MEMO4_ADDR
#define H264DEC_MBC_WP_PARA_BFR	VSP_MEMO5_ADDR
#define H264DEC_IDCT_IO_BFR	VSP_MEMO6_ADDR
#define H264DEC_MBC_OUT_BFR0	VSP_MEMO7_ADDR
#define H264DEC_MBC_OUT_BFR1	VSP_MEMO8_ADDR
#define H264DEC_DBK_TOP_MB_BOTTOM_FOUR_DATA_LINE_BFR	VSP_MEMO9_ADDR
#define H264DEC_MBC_IPRED_TOPLEFT_POINT_DATA_BFR	VSP_MEMO10_ADDR
#define H264DEC_EXE_CMD_DATA_BFR0	VSP_MEMO11_ADDR
#define H264DEC_MCA_FLT0_PROC_BFR1	VSP_MEMO12_ADDR
#define H264DEC_VLD_TBL	VSP_MEMO14_ADDR
#define H264DEC_EXE_CMD_DATA	VSP_MEMO15_ADDR
#define H264DEC_MCA_FETCH_BFR0	VSP_MEMO16_ADDR
#define H264DEC_MCA_FETCH_BFR1	VSP_MEMO17_ADDR

//REAL DEC, same as H.264 dec
#define RVDEC_MCA_FLT0_PROC_BFR0		VSP_MEMO0_ADDR
#define RVDEC_MCA_FLT1_PROC_BFR0		VSP_MEMO1_ADDR
#define RVDEC_MCA_FLT1_PROC_BFR1		VSP_MEMO2_ADDR
#define RVDEC_MCA_MB_OUT_BFR0	VSP_MEMO3_ADDR
#define RVDEC_MCA_MB_OUT_BFR1	VSP_MEMO4_ADDR
#define RVDEC_MBC_WP_PARA_BFR	VSP_MEMO5_ADDR
#define RVDEC_IDCT_IO_BFR	VSP_MEMO6_ADDR
#define RVDEC_MBC_OUT_BFR0	VSP_MEMO7_ADDR
#define RVDEC_MBC_OUT_BFR1	VSP_MEMO8_ADDR
#define RVDEC_DBK_TOP_MB_BOTTOM_FOUR_DATA_LINE_BFR	VSP_MEMO9_ADDR
#define RVDEC_MBC_IPRED_TOPLEFT_POINT_DATA_BFR	VSP_MEMO10_ADDR
#define RVDEC_EXE_CMD_DATA_BFR0	VSP_MEMO11_ADDR
#define RVDEC_MCA_FLT0_PROC_BFR1	VSP_MEMO12_ADDR
#define RVDEC_VLD_TBL	VSP_MEMO14_ADDR
#define RVDEC_EXE_CMD_DATA	VSP_MEMO15_ADDR
#define RVDEC_MCA_FETCH_BFR0	VSP_MEMO16_ADDR
#define RVDEC_MCA_FETCH_BFR1	VSP_MEMO17_ADDR

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif 
/**---------------------------------------------------------------------------*/
// End 
#endif //_VSP_GLOBAL_DEFINE_H_

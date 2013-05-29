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
 *****************************************************************************/
#ifndef _VSP_GLOBAL_DEFINE_H_
#define _VSP_GLOBAL_DEFINE_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#if SIM_IN_WIN
 #include "vsp_dcam.h"
#endif
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif 

#define CHIP_ENDIAN_LITTLE

#if SIM_IN_WIN
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
#define	VSP_MEMO9_ADDR				(VSP_DCAM_REG_BASE + 0xd000)
#define	VSP_MEMO10_ADDR				(VSP_DCAM_REG_BASE + 0x1000)
#define	VSP_MEMO11_ADDR				(VSP_DCAM_REG_BASE + 0x1100)
#define	VSP_MEMO12_ADDR				(VSP_DCAM_REG_BASE + 0x1200)
#define	VSP_MEMO13_ADDR				(VSP_DCAM_REG_BASE + 0xb000)
#define	VSP_MEMO14_ADDR				(VSP_DCAM_REG_BASE + 0xc000)
#define	VSP_MEMO15_ADDR				(VSP_DCAM_REG_BASE + 0x1400)

#define MEMO0_ADDR_SIZE				(96*4)
#define MEMO1_ADDR_SIZE				(96*4)
#define MEMO2_ADDR_SIZE				(96*4)
#define MEMO3_ADDR_SIZE				(96*4)
#define MEMO4_ADDR_SIZE				(96*4)
#define MEMO5_ADDR_SIZE				(196*4)
#define MEMO6_ADDR_SIZE				(216*4)
#define MEMO7_ADDR_SIZE				(172*4)
#define MEMO8_ADDR_SIZE				(172*4)
#define MEMO9_ADDR_SIZE				(1024*8)
#define MEMO10_ADDR_SIZE			(64*4)
#define MEMO11_ADDR_SIZE			(64*4)
#define MEMO12_ADDR_SIZE			(128*4)
#define MEMO13_ADDR_SIZE			(1024*4)
#define MEMO14_ADDR_SIZE			(1024*4)
#define MEMO15_ADDR_SIZE			(64*4)

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
#endif
/*****************openrisc mem map***********/
#define	VSP_IMEM_BASE_ADDR 0x00000000 
#define	VSP_REG_BASE_ADDR 0x80000000 //VSP REG//weihu//
//#define	DDR_MEM_BASE_ADDR EXTRA_MALLOC_MEM_START_ADDR// YUV address in DDR

#define	SHARE_RAM_BASE_ADDR (VSP_REG_BASE_ADDR+0x200)
#define	GLB_REG_BASE_ADDR (VSP_REG_BASE_ADDR+0x1000)
#define	PPA_SLICE_INFO_BASE_ADDR (VSP_REG_BASE_ADDR+0x1200)
#define	DCT_IQW_TABLE_BASE_ADDR (VSP_REG_BASE_ADDR+0x1400)
#define	FRAME_ADDR_TABLE_BASE_ADDR (VSP_REG_BASE_ADDR+0x1800)
#define	CABAC_CONTEXT_BASE_ADDR (VSP_REG_BASE_ADDR+0x1a00)
#define	VLC_TABLE0_BASE_ADDR (VSP_REG_BASE_ADDR+0x2000)
#define	VLC_TABLE1_BASE_ADDR (VSP_REG_BASE_ADDR+0x3000)
#define	BSM_CTRL_REG_BASE_ADDR (VSP_REG_BASE_ADDR+0x8000)

#define	SHARE_RAM_SIZE 0x100
#define	GLB_REG_SIZE 0x200
#define	PPA_SLICE_INFO_SIZE 0x200
#define	DCT_IQW_TABLE_SIZE 0x400
#define	FRAME_ADDR_TABLE_SIZE 0x200
#define	VLC_TABLE0_SIZE 2624
#define	VLC_TABLE1_SIZE 1728
#define	BSM_CTRL_REG_SIZE 0x1000

#define	FRAME_BUF_SIZE 0x20000//dword//1MB//0x40000

//glb reg
#define VSP_INT_SYS_OFF 0x0
#define VSP_INT_MASK_OFF 0x04
#define VSP_INT_CLR_OFF 0x08
#define VSP_INT_RAW_OFF 0x0c
#define AXIM_ENDIAN_SET_OFF 0x10
#define VSP_MODE_OFF 0x20
#define IMG_SIZE_OFF 0x24
#define RAM_ACC_SEL_OFF 0x28
#define VSP_INT_GEN_OFF 0x2c
#define VSP_START_OFF 0x30
#define VSP_SIZE_SET_OFF 0x34
#define VSP_CFG0_OFF 0x3c
#define VSP_CFG1_OFF 0x40
#define VSP_CFG2_OFF 0x44
#define VSP_CFG3_OFF 0x48
#define VSP_CFG4_OFF 0x4c
#define VSP_CFG5_OFF 0x50
#define VSP_CFG6_OFF 0x54
#define VSP_CFG7_OFF 0x58
#define BSM0_FRM_ADDR_OFF 0x60
//#define BSM1_FRM_ADDR_OFF 0x64
#define VSP_ADDRIDX0_OFF 0x68
#define VSP_ADDRIDX1_OFF 0x6c
#define VSP_ADDRIDX2_OFF 0x70
#define VSP_ADDRIDX3_OFF 0x74
#define VSP_ADDRIDX4_OFF 0x78
#define VSP_ADDRIDX5_OFF 0x7c
#define MCU_START_OFF 0x80//ro
#define CLR_START_OFF 0x84//wo
#define MCU_SLEEP_OFF 0x88//wo
#define VSP_DBG_STS0_OFF 0x100//mbx mby

//bsm reg

#define BSM_CFG0_OFF 0x0
#define BSM_CFG1_OFF 0x4
#define BSM_OP_OFF 0x8
#define BSM_WDATA_OFF 0xc
#define BSM_RDATA_OFF 0x10
#define TOTAL_BITS_OFF 0x14
#define BSM_DBG0_OFF 0x18
#define BSM_DBG1_OFF 0x1c
#define BSM_RDY_OFF 0x20
#define USEV_RD_OFF 0x24
#define USEV_RDATA_OFF 0x28
#define DSTUF_NUM_OFF 0x2c
#define BSM_NAL_LEN 0x34
#define BSM_NAL_DATA_LEN 0x38

//IQW_TABLE
#define INTER4x4Y_OFF 0x00
#define INTER4x4U_OFF 0x10
#define INTER4x4V_OFF 0x20
#define INTRA4x4Y_OFF 0x40
#define INTRA4x4U_OFF 0x50
#define INTRA4x4V_OFF 0x60
#define INTER8x8_OFF 0x80
#define INTRA8x8_OFF 0x100
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif 
/**---------------------------------------------------------------------------*/
// End 
#endif //_VSP_GLOBAL_DEFINE_H_































































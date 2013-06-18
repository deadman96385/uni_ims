/******************************************************************************
** File Name:      video_commmon.h                                           *
** Author:         Xiaowei Luo                                               *
** DATE:           12/06/2007                                                *
** Copyright:      2007 Spreatrum, Incoporated. All Rights Reserved.         *
** Description:    common define for video codec.	     			          *
*****************************************************************************/
/******************************************************************************
**                   Edit    History                                         *
**---------------------------------------------------------------------------* 
** DATE          NAME            DESCRIPTION                                 * 
** 11/20/2007    Xiaowei Luo     Create.                                     *
*****************************************************************************/
#ifndef _VIDEO_COMMON_H_
#define _VIDEO_COMMON_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "sci_types.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C" 
{
#endif

/*#if defined(_SIMULATION_)
	#if defined(_VSP_)
		#define SIM_WITH_VSP  //may run in dvb board
	#else
		#define SIM_NO_VSP
		#if defined(_ARM_)	
			#define SIM_IN_ADS	
		#else 
			#define SIM_IN_WIN	
		#endif
	#endif
	#define _DEBUG_		1
#else
	#define RUN_IN_PLATFORM
	#define _DEBUG_     0
#endif

#if defined(SIM_NO_VSP)
#define _CMODEL_   1
#else
#define _CMODEL_   0
#endif //defined(SIM_NO_VSP)*/

#if 0//defined(_SIMULATION_)
	#define SIM_IN_WIN
	#define _CMODEL_   1
#else
	#undef SIM_IN_WIN
	#define _CMODEL_   0
#endif

#if defined(SIM_IN_WIN)&&!defined(_LIB)
	#define _FW_TEST_VECTOR_ 0
#else
	#define _FW_TEST_VECTOR_ 0
#endif //defined(SIM_IN_WIN)&&!defined(_LIB)

/*standard*/
	/*
typedef enum {
	ITU_H263 = 0, 
		VSP_MPEG4,  
		VSP_JPEG,
		FLV_H263,
		VSP_H264,
		VSP_RV8,
		VSP_RV9,
		VSP_VP8
		}VIDEO_FORMAT_E;
*/
/*
	Bit define, for video
*/
#define V_BIT_0               0x00000001
#define V_BIT_1               0x00000002
#define V_BIT_2               0x00000004
#define V_BIT_3               0x00000008
#define V_BIT_4               0x00000010
#define V_BIT_5               0x00000020
#define V_BIT_6               0x00000040
#define V_BIT_7               0x00000080
#define V_BIT_8               0x00000100
#define V_BIT_9               0x00000200
#define V_BIT_10              0x00000400
#define V_BIT_11              0x00000800
#define V_BIT_12              0x00001000
#define V_BIT_13              0x00002000
#define V_BIT_14              0x00004000
#define V_BIT_15              0x00008000
#define V_BIT_16              0x00010000
#define V_BIT_17              0x00020000
#define V_BIT_18              0x00040000
#define V_BIT_19              0x00080000
#define V_BIT_20              0x00100000
#define V_BIT_21              0x00200000
#define V_BIT_22              0x00400000
#define V_BIT_23              0x00800000
#define V_BIT_24              0x01000000
#define V_BIT_25              0x02000000
#define V_BIT_26              0x04000000
#define V_BIT_27              0x08000000
#define V_BIT_28              0x10000000
#define V_BIT_29              0x20000000
#define V_BIT_30              0x40000000 
#define V_BIT_31              0x80000000

//trace enable flag define
#define TRACE_ENABLE_COUNT			11
#define TRACE_ENABLE_VLD			0x1
#define TRACE_ENABLE_ISQT			0x2
#define TRACE_ENABLE_IPRED			0x4
#define TRACE_ENABLE_DBK			0x8
#define TRACE_ENABLE_MBC			0x10
#define TRACE_ENABLE_MCA			0x20
#define TRACE_ENABLE_DCT			0x40
#define TRACE_ENABLE_MEA			0x80
#define TRACE_ENABLE_VLC			0x100
#define TRACE_ENABLE_BSM			0x200
#define TRACE_ENABLE_FW				0x400
	
//test vector enable flag define
#define VECTOR_ENABLE_COUNT			13
#define VECTOR_ENABLE_VLD			0x1		
#define VECTOR_ENABLE_ISQT			0x2
#define VECTOR_ENABLE_IPRED			0x4
#define VECTOR_ENABLE_DBK			0x8
#define VECTOR_ENABLE_MBC			0x10
#define VECTOR_ENABLE_MCA			0x20
#define VECTOR_ENABLE_DCT			0x40
#define VECTOR_ENABLE_MEA			0x80
#define VECTOR_ENABLE_VLC			0x100
#define VECTOR_ENABLE_BSM			0x200
#define VECTOR_ENABLE_FW			0x400
#define VECTOR_ENABLE_IEA			0x800
#define VECTOR_ENABLE_PPA			0x1000

extern int32 g_trace_enable_flag;
extern int32 g_vector_enable_flag;

#if defined(SIM_IN_WIN)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

//#define VLC_TV_SPLIT
#define FRAME_X 20

#define FPRINTF_ORSC fprintf
#define PRINTF printf
#define FPRINTF //fprintf
#define FFLUSH fflush
#define _COMPUTE_PSNR_
#define SCI_ASSERT assert
#define SCI_PASSERT 
#ifdef SIM_IN_WIN
	#define EXIT  exit
#else
	#define EXIT //exit
#endif
#define IPRED_PRINTF //printf  //for ipred debug
#ifndef SIM_IN_WIN
	#deinfe exit
#endif
#define _TRACE_	0
	
#else //if defined(SIM_IN_ADS)||defined(RUN_IN_PLATFORM)
#include <stdlib.h>
#include <string.h>
#define PRINTF 
#define FPRINTF
#define FFLUSH
#define SCI_ASSERT 
#define EXIT
#define IPRED_PRINTF 
#define _TRACE_	0
#endif

#if defined(_SIMULATION_)
#define SCI_MEMSET  memset
#define SCI_MEMCPY	memcpy
#endif

//for motion vector
typedef struct motion_vector_tag
{	
	int16 x;
	int16 y;
}MOTION_VECTOR_T;

typedef struct mv_info_tag		/* for motion vector coding*/
{	
	uint16 Range;			/* search range (32f (-32f, 32f-1))*/
	uint8  FCode;			/* f-code  (vop_fcode)*/
	uint8  ScaleFactor;	/* scale factor (f)*/
}MV_INFO_T;

typedef struct mcbpc_table_code_len_tag
{
	uint8 code;	/* right justified *///the value of the variable length code
	uint8 len;	// the length of the variable length code
}MCBPC_TABLE_CODE_LEN_T;

typedef struct mv_table_code_len_tag
{
	int8 code;	/* right justified *///the value of the variable length code
	uint8 len;	// the length of the variable length code
}MV_TABLE_CODE_LEN_T;

typedef enum {INTRA,INTRAQ,INTER,INTERQ,INTER4V,MODE_STUFFING=7}MB_MODE_E;

//for macroblock
#define MB_SIZE					16
#define MB_CHROMA_SIZE			8
#define MB_SQUARE_SIZE			256
	
//for block
#define BLOCK_SIZE				8
#define BLOCK_SQUARE_SIZE		64
#define BLOCK_CNT				6
	
#define ABS(x) ((x) > 0 ? (x) : -(x))
#define Clip3(min,max,val) (((val)<(min))?(min):(((val)>(max))?(max):(val)))
#define YUV420					0
#define YUV411					1
#define YUV444					2
#define YUV422					3
#define YUV400					4
	
#define mmax(aa,bb)		(((aa) > (bb)) ? (aa) : (bb))
#define mmin(aa,bb)		(((aa) < (bb)) ? (aa) : (bb))
#define ssign(x)		((x) > 0 ? 1 : (-1))		
#define ssignof(x)		(((x) > 0) ? 1 : 0)
#define rounded(x)		(((x) > 0) ? (x + 0.5) :(x - 0.5))
#define IClip(Min, Max, Val) (((Val)<(Min))? (Min):(((Val)>(Max))? (Max):(Val)))
#define MEDIAN(aa, bb, cc) ((aa-bb)*(aa-cc)<=0)?aa :(((bb-cc)*(bb-aa)<=0)?bb:cc)

#define STREAM_ID_H263				0
#define STREAM_ID_MPEG4				1
#define STREAM_ID_JPEG				2
#define STREAM_ID_FLVH263			3
#define STREAM_ID_H264				4
#define STREAM_ID_REAL8				5
#define STREAM_ID_REAL9				6
#define STREAM_ID_VP8				7
#define STREAM_ID_WEBP				8
#define STREAM_ID_VC1				9
#define STREAM_ID_AVS				10

#define I_16X16	1
#define I_4X4	0
#define	I_8X8	2//weihu

#if 1 //for mp4 encoder
// input parameters from configuration file
typedef struct inp_par_tag
{
	char	infile[200];			//!< inputfile bitstream file
	char	outfile[200];			//!< Decoded YUV 4:2:0 output
	char	recfile[200];			//!< Optional YUV 4:2:0 reference file for SNR measurement
	int		is_short_header;		//1: h.263  0: mpeg4
	int		pic_width;				//only for encoder
	int		pic_height;				//only for encoder
	int		frame_num_enc;			//!< Frame number to be decoded
	int		rc_ena;					//rate control enable or not
	int		bit_rate;				//bit rate (kbps)
	int		src_frm_rate;			//source frame rate
	int		samp_rate;				//distance between two encoded frame
	int		step_I;					//first I frame's QP
	int		step_P;					//first P frame's QP
}INPUT_PARA_T;
#else //for mp4 decoder
// input parameters from configuration file
typedef struct inp_par_tag
{
	char infile[200];				//!< inputfile bitstream file
	char outfile[200];				//!< Decoded YUV 4:2:0 output
	char reffile[200];				//!< Optional YUV 4:2:0 reference file for SNR measurement
	char streamType[20];			//H.264, AVS, MPEG-4, H.263, VC-1
	char AVS_Version[20];			//avs bs version
	char trace_enable_flag[20];
	char vector_enable_flag[20];
	uint32 pic_width;				//only for encoder
	uint32 pic_height;				//only for encoder
	int32 frame_num_dec;			//!< Frame number to be decoded
	uint32 quality_level;			//only JPEG encoder
	uint8  scaling_down_factor;     //only JPEG decoder
	uint32 mcu_info;
	BOOLEAN is_output_yuv;
}INPUT_PARA_T;
#endif

#if defined(SIM_IN_WIN)
extern FILE *s_pEnc_input_src_file;
extern FILE *s_pEnc_output_bs_file;
extern FILE *s_pEnc_output_recon_file;
extern FILE *s_pDec_recon_yuv_file; //file pointer of reconstructed YUV sequence
extern FILE *s_pDec_disp_yuv_file; //file pointer of display YUV sequence
extern FILE *s_fp_src_enc;	
extern FILE *g_fp_trace_fw;
#endif

extern int g_samp_rate;

/*#if defined(SIM_WITH_VSP)
#if 0
	#define IRAM_START_ADDR				0xa00000 //start from 10Mbyte offset of SDRAM
	#define IRAM_SIZE					200*1024  //256 kByte
	#define CODE_SIZE					1*1024*1024
	#define SDRAM_SIZE					(31*1024*1024-CODE_SIZE)  ///3 MByte, (total 32Mbyte, 1Mbyte for code)
	#define TOTAL_INTER_MALLOC_SIZE		30*1024	//30 kByte for malloc
	#define TOTAL_EXTRA_MALLOC_SIZE		2*1024*1024	///1MByte for malloc
	#define SDRAM_START_ADDR			(0x00000000+CODE_SIZE) //start from 1Mbyte offset
	#define INTER_MALLOC_MEM_START_ADDR	(IRAM_START_ADDR + (IRAM_SIZE - TOTAL_INTER_MALLOC_SIZE))
	#define EXTRA_MALLOC_MEM_START_ADDR (SDRAM_START_ADDR + (SDRAM_SIZE - TOTAL_EXTRA_MALLOC_SIZE))
	#define BS_START_ADDR_OFFSET		0x1000000
#else
	//#define IRAM_START_ADDR				0xa00000 //start from 10Mbyte offset of SDRAM //start addr??
	#define IRAM_SIZE					200*1024  //256 kByte
	#define CODE_SIZE					1*1024*1024
	//#define SDRAM_SIZE					(31*1024*1024-CODE_SIZE)  ///3 MByte, (total 32Mbyte, 1Mbyte for code)
	#define TOTAL_INTER_MALLOC_SIZE		30*1024	//30 kByte for malloc
	#define TOTAL_EXTRA_MALLOC_SIZE		2*1024*1024	///1MByte for malloc
	//#define SDRAM_START_ADDR			(0x00000000+CODE_SIZE) //start from 1Mbyte offset
	#define INTER_MALLOC_MEM_START_ADDR	0x00080000
	#define EXTRA_MALLOC_MEM_START_ADDR (0x00200000)
	#define BS_START_ADDR				0x4600000//for rtl simulation//weihu
	#define BS_BUF_SIZE		0x800000//for rtl simulation//weihu
#endif
#elif defined(SIM_NO_VSP)
#if 0
	#define SDRAM_START_ADDR			0x00000000 //RTL simulation
	#define TOTAL_INTER_MALLOC_SIZE		3000*1024	//3000 kByte for malloc
	#define TOTAL_EXTRA_MALLOC_SIZE		30*1024*1024	//30MByte for malloc
#else
	#define SDRAM_START_ADDR			0x00000000 //RTL simulation
	#define TOTAL_INTER_MALLOC_SIZE		3000*1024	//3000 kByte for malloc
	#define TOTAL_EXTRA_MALLOC_SIZE		30*1024*1024	//30MByte for malloc
	#define INTER_MALLOC_MEM_START_ADDR	0x00080000
	#define EXTRA_MALLOC_MEM_START_ADDR (0x00200000)
	#define BS_START_ADDR				0x4600000//for rtl simulation//weihu
	#define BS_BUF_SIZE		0x800000//for rtl simulation//weihu
#endif
#endif*/
#ifdef SIM_IN_WIN
	#define TOTAL_INTER_MALLOC_SIZE		0x400000	//4MByte for malloc
	#define TOTAL_EXTRA_MALLOC_SIZE		256*1024*1024	///256MByte for malloc
#else
	#define TOTAL_INTER_MALLOC_SIZE		0x190000	//1.56MByte for malloc
	#define TOTAL_EXTRA_MALLOC_SIZE		0x800000	//8MByte for malloc
#endif

#ifdef ORSC_SIM
	#define INTER_MALLOC_MEM_START_ADDR	0x00800000
	#define EXTRA_MALLOC_MEM_START_ADDR	0x01000000
	#define BS_START_ADDR				0x01800004
	#define BS_BUF_SIZE					0x001F0000
#else
	#define INTER_MALLOC_MEM_START_ADDR	0x00080000	//IRAM_START_ADDR //(IRAM_START_ADDR + (IRAM_SIZE - TOTAL_INTER_MALLOC_SIZE))
	#define EXTRA_MALLOC_MEM_START_ADDR	0x00200000	//DDRAM_START_ADDR //(DDRAM_START_ADDR + (DDRAM_SIZE - TOTAL_EXTRA_MALLOC_SIZE))
	#define BS_START_ADDR				0x4600000	//for rtl simulation//weihu
	#define BS_BUF_SIZE					0x800000	//for rtl simulation//weihu
#endif
#define IRAM_SIZE				0x100000	//64*1024  //64 kByte
#define CODE_SIZE				1024*1024/2
#define VLD_TABLE_ADDR				0+CODE_SIZE+IRAM_SIZE
#define	FRAME_BUF_SIZE				0x40000//dword 2MB

PUBLIC void *MallocInterMem(uint32 mem_size);
PUBLIC void *MallocExtraMem(uint32 mem_size);
PUBLIC void *ExtraMemAlloc_64WordAlign(uint32 mem_size);
PUBLIC extern uint32 g_inter_malloced_size;
PUBLIC extern uint32 g_extra_malloced_size;
PUBLIC extern INPUT_PARA_T *g_input;
PUBLIC extern int32	g_stream_type;
PUBLIC extern int32 g_uv_interleaved;

void Get_MD5_Code(uint8 *img,   uint32 datalen, uint8 *code);
extern void PrintfVLCOut(uint32 val, int nbits, uint8* comment);



/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_VIDEO_COMMON_H_

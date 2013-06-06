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

#define _SIMULATION_
#define _MP4CODEC_DATA_PARTITION_
#define MPEG4_DEC
#define ASIC_DCT
//#define MD5_CHK
	
#define CHIP_ENDIAN_LITTLE
#define SIM_IN_WIN	0


#if defined(SIM_IN_WIN)&&!defined(_LIB)
	#define _FW_TEST_VECTOR_ 1
#else
	#define _FW_TEST_VECTOR_ 0
#endif //defined(SIM_IN_WIN)&&!defined(_LIB)

#if SIM_IN_WIN
#define _CMODEL_   1
#else
#define _CMODEL_   0
#endif //defined(SIM_NO_VSP)


/*standard*/

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
#define VECTOR_ENABLE_COUNT			11
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

extern int32 g_trace_enable_flag;
extern int32 g_vector_enable_flag;

#if SIM_IN_WIN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define SCI_MEMSET  memset
#define SCI_MEMCPY	memcpy	
#define PRINTF //PRINTF
#define FPRINTF //FPRINTF
#define FFLUSH //fflush
#define _COMPUTE_PSNR_
#define SCI_ASSERT assert
#define SCI_PASSERT 
#define EXIT  exit
#define IPRED_PRINTF //PRINTF  //for ipred debug

#define _TRACE_	0
	
#else //if defined(SIM_IN_ADS)||defined(RUN_IN_PLATFORM)
#define SCI_MEMSET  memset
#define SCI_MEMCPY	memcpy	
#define PRINTF 
#define FPRINTF
#define FFLUSH
#define SCI_ASSERT 
#define EXIT
#define IPRED_PRINTF 

#define _TRACE_	0
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

//for vlc table structure
typedef struct vlc_table_code_len_tag
{
	int16 code;	/* right justified *///the value of the variable length code
	uint16 len;	// the length of the variable length code
}VLC_TABLE_CODE_LEN_T;

typedef enum {INTRA,INTRAQ,INTER,INTERQ,INTER4V,MODE_STUFFING}MB_MODE_E;

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

#if 0 //for mp4 encoder
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
	BOOLEAN is_output_rgb;
}INPUT_PARA_T;
#endif

#if SIM_IN_WIN
	#if defined(JPEG_ENC)||defined(MPEG4_ENC)
		extern FILE *s_pEnc_input_src_file;
		extern FILE *s_pEnc_output_bs_file;
		extern FILE *s_pEnc_output_recon_file;
	#else
		extern FILE *s_pDec_input_bs_file; //file pointer of H.264 bit stream
		extern FILE *s_pDec_recon_yuv_file; //file pointer of reconstructed YUV sequence
		extern FILE *s_pDec_disp_yuv_file; //file pointer of display YUV sequence
		extern FILE *s_pEnc_output_recon_file;
	#endif //#if defined(JPEG_ENC)
#else 
	extern uint8 *ptrInbfr;
	extern uint8 videoStream[];
	extern int g_streamLen;
	extern uint8 *ptrOutBfr;
	extern uint8 *g_pOutBfr;
#endif 	//#if defined(SIM_IN_WIN)
extern int32 g_uv_interleaved;
extern int32 bs_start_addr;
extern int32 extra_malloc_mem_start_addr;
extern int32 inter_malloc_mem_start_addr;
extern int32 frame_buf_size;
extern int32 total_inter_malloc_size;
extern int32 total_extra_malloc_size;
extern uint32 or_addr_offset;//qiangshen@2013_01_11
#if SIM_IN_WIN
  #define TOTAL_INTER_MALLOC_SIZE		0x400000	//1MByte for malloc
  #define TOTAL_EXTRA_MALLOC_SIZE		256*1024*1024	///256MByte for malloc
#else
  #define TOTAL_INTER_MALLOC_SIZE		0x100000	//1MByte for malloc
  #define TOTAL_EXTRA_MALLOC_SIZE		64*1024*1024	///64MByte for malloc
#endif

//#define IRAM_START_ADDR				0x00080000 //start from ?Mbyte offset of DDRAM
#define ARMRAM_SIZE            0x200000 //qiangshen@2013_01_11
#define IRAM_SIZE					0x100000+ARMRAM_SIZE //64*1024  //64 kByte
#define CODE_SIZE					1024*1024/2
//#define DDRAM_START_ADDR			(0x00200000)//+CODE_SIZE) //start from 2Mbyte offset		
#define DDRAM_SIZE					(64*1024*1024)//-CODE_SIZE)  //? MByte, (total 64Mbyte, 2Mbyte for code)		
#define INTER_MALLOC_MEM_START_ADDR		(0x00080000+ARMRAM_SIZE)//IRAM_START_ADDR //(IRAM_START_ADDR + (IRAM_SIZE - TOTAL_INTER_MALLOC_SIZE))
#define EXTRA_MALLOC_MEM_START_ADDR (0x00200000+ARMRAM_SIZE)//DDRAM_START_ADDR //(DDRAM_START_ADDR + (DDRAM_SIZE - TOTAL_EXTRA_MALLOC_SIZE))
#define BS_START_ADDR		(0x4600000++ARMRAM_SIZE)//for rtl simulation//weihu
#define BS_BUF_SIZE		0x800000//for rtl simulation//weihu
#define VLD_TABLE_ADDR      0+CODE_SIZE+IRAM_SIZE

PUBLIC void *MallocInterMem(uint32 mem_size);
PUBLIC void *MallocExtraMem(uint32 mem_size);
PUBLIC void *ExtraMemAlloc_64WordAlign(uint32 mem_size);
PUBLIC void memcpy1( void* s1, void* s2, int n);
PUBLIC void memset1( void* s1, int d, int n);

PUBLIC extern uint32 g_inter_malloced_size;
PUBLIC extern uint32 g_extra_malloced_size;
PUBLIC extern INPUT_PARA_T *g_input;
PUBLIC extern int32	g_stream_type;

extern uint8	g_err_inf[10000];

#if SIM_IN_WIN
extern PUBLIC FILE *g_fp_trace_fw;
#endif //SIM_IN_WIN

void Get_MD5_Code(uint8 *img,   uint32 datalen, uint8 *code);
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_VIDEO_COMMON_H_

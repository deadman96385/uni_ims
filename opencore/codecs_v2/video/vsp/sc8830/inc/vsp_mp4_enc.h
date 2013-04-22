/******************************************************************************
 ** File Name:      hdvsp.h                                         *
 ** Author:         william wei                                               *
 ** DATE:           09/29/2012                                                *
 ** Copyright:      2012 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    														  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 09/29/2012    william wei     Create.                                     *
 *****************************************************************************/
#ifndef _VSP_MP4_ENC_H_
#define _VSP_MP4_ENC_H_


//#define _FPGA_TEST_ 

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/

#include "mmcodec.h"

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif 


/*----------------------------------------------------------------------------*
**                            Macro Definitions                               *
**---------------------------------------------------------------------------*/
#define MP4ENC_OR_RUN_SIZE		                (0x100000)      //byte unit
#define MP4ENC_OR_INTER_START_ADDR		MP4ENC_OR_RUN_SIZE
#define MP4ENC_OR_INTER_MALLOC_SIZE		(0x100000)      //byte unit

#define MP4ENC_ENCODE 		0
#define MP4ENC_INIT			1
#define MP4ENC_HEADER		2
#define MP4ENC_SETCONF		3
#define MP4ENC_GETCONF		4
/* ------------------------------------------------------------------------
** Constants
** ------------------------------------------------------------------------ */

/* -----------------------------------------------------------------------
** Structs
** ----------------------------------------------------------------------- */

typedef struct mp4enc_share_ram_info
{
	// Offset 0x0 ;	
	uint32 standard;			// [3:0] video standard
	uint32 workmode;			// [4]	1: enc; 0 :dec 
	uint32 input_buffer_update;	// [8]
	uint32 video_buffer_malloced;	// [16]
	uint32 b_anti_shake;			// [17]
	uint32 expected_IVOP;		// [28]
	uint32 cpu_will_be_reset;		// [29]
	uint32 g_not_first_reset;		// [30]
	uint32 run;			// [31]
	
	// Offset 0x4
	uint32 pic_width;			// [11:0]
	uint32 pic_height;			// [12:23]
    	uint32 video_size_get;		// [31]
	
	// Offset 0x8
	uint32 malloc_mem0_start_addr;	
	
	// Offset 0xc
	uint32 total_mem0_size;

	// Offset 0x10
	uint32 malloc_mem1_start_addr;	//Number of direct mb info buffer when parsing sps. W by or.

	// Offset 0x14
	uint32 total_mem1_size;			// buffer size of each direct mb info buffer when parsing sps. W by or.

	// Offset 0x18
    	uint32 bs_start_addr;

	// Offset 0x1c
	uint32 bs_buffer_size;

	// Offset 0x20
	uint32 bs_used_len;

	// Offset 0x24
	uint32 table_addr;

	// Offset 0x28
	uint32 table_size;

	// Offset 0x2c
	uint32 frameY_addr;		//output
	
	// Offset 0x30
	uint32 frameUV_addr;

	// Offset 0x34
	uint32 UVframe_size;

	// Offset 0x38	
	uint32 rate_ctr_en;		// [30:0]
	uint32 target_bitrate;		// [31]

	// Offset 0x3c
	uint32 time_stamp;

	// Offset 0x40
	uint32 time_scale;

	// Offset 0x44
	uint32 x_size;
	uint32 y_size;		
	uint32 x_offset;
	uint32 y_offset;

	// Offset 0x48
	uint32 size_scale; 			// TBD. Scaling is NOT supported in VSP.
	
	// Offset 0x4c
	uint32 frame_output_en;

	// Offset 0x50
	uint32 frame_buf_size_x;		// [11:0] 
	uint32 frame_buf_size_y;		// [23:12]

	// Offset 0x54
	uint32 or_addr_offset;

/* -----------------------------------------------------------------------*/
	// Offset 0x58
	uint32 function_type;

	// Offset 0x5c
	uint32 OR_output;


	// Offset 0x60~0x6c. Regs for MPEG4 ENC config.
	// Offset 0x60
	uint8 mp4enc_short_video_header;		// [0]
	uint8 mp4enc_RateCtrlEnable;				// [8]
	uint8 mp4enc_StepI;						// [23:16]
	uint8 mp4enc_StepP;						// [31:24]

	// Offset 0x64
	uint32 mp4enc_FrameRate;

	//Offset 0x68 
	uint32 mp4enc_targetBitRate;

	// Offset 0x6c
	uint32 mp4enc_ProfileAndLevel;

	// Offset 0x70
	uint32 mp4enc_vopType;

	


	// Offset 0x58 ~ 0xf8
	// Reserved.
	
/* -----------------------------------------------------------------------*/
	// Offset 0xfc
	uint32 version; 
	
}MP4ENC_SHARE_RAM;

/* -----------------------------------------------------------------------
** Global
** ----------------------------------------------------------------------- */

/**---------------------------------------------------------------------------
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif 
/**---------------------------------------------------------------------------*/
// End 
#endif  //_VSP_MP4_ENC_H_


#include "video_common.h"
#include "vpx_codec.h"
#include "vp8_init.h"
#include "vp8_swap_yv12buffer.h"
#include "vp8_yv12extend.h"
#include "vp8_loopfilter.h"
#include "vp8_segmentation_common.h"
#include "vp8dec_basic.h"
#include "vp8dec_mode.h"
#include "vp8dec_malloc.h"
#include "vp8dec_global.h"

#include "vp8dbk_trace.h"
#include "hdbk_mode.h"

void Vp8Dec_BS_and_Para(VP8_COMMON *cm,MACROBLOCKD *mbd, int *baseline_filter_level,int mb_row, int mb_col);


void vp8_dbk_frame
(
    VP8_COMMON *cm,
    MACROBLOCKD *mbd,
    int default_filt_lvl
);

void GetFourPixWrite	(int line_id, uint8 pix_write[4]);

void Get8PixFiltering	(int line_id, uint8 pix_p[4], uint8 pix_q[4]);

void PixBlkExchange		();

void PixBlkUptFilter	(
						int		line_id,
						uint8		p0,
						uint8		p1,
						uint8		p2,
						int		p0_upt,
						int		p1_upt,
						int		p2_upt,
						uint8		q0,
						uint8		q1,
						uint8		q2,
						int		q0_upt,
						int		q1_upt,
						int		q2_upt
						);

void PixBlkUptWrite		(
						int	line_id, 
						uint8	pix0,
						uint8	pix1,
						uint8	pix2,
						uint8	pix3
						);

void GetFilterPara_VP8 (
					/*input*/
					int	is_hor_filter, 
					int	filt_blk_comp, 
					int	filt_blk_y, 
					int	filt_blk_x,
	
					/*output*/
					int * edge_limit_ptr,
					int * interior_diff_limit_ptr,
					int * hevthr_ptr,
					int * filter_type_ptr
					);

void DbkFilter_VP8 (
				int		line_id,
				uint8	pix_p[4], 
				uint8	pix_q[4],

				int		edge_limit, 
				int		interior_diff_limit, 
				int		hevthr, 
				int		filter_type);

void dbk_module(unsigned char *y, unsigned char *u, unsigned char *v,
             int ystride, int uv_stride);


//#define VP8DBK_CFG0_OFF		0x58
//#define VP8DBK_CFG0_WOFF	0x16
/*
typedef struct 
{
	volatile uint32 DBK_CFG;			//[4]:		RV8_INTRA_PIC, Intra picture flag for real 8
										//[3]:		H264_FMO_MODE, H.264 FMO mode enable, active high. 
										//[2]:		POST_FLT_ENA, Post filtering enable, active high. 
										//[1:0]:	DBK_RUN_MODE, DBK start mode	2'b10"  -- Free run mode; '2'b01'  -- Auto mode		'2'b00'  - Manual mode		

	volatile uint32 RESV0;

	volatile uint32 DBK_SID_CFG;		//[25:16]:	DBK_START_Y_ID
										//[9:0]:	DBK_START_X_ID

	volatile uint32 DBK_MCU_NUM;		//[15:0]:	DBK_MCU_NUM, MBC total MCU number

	volatile uint32 DBK_VDB_BUF_ST;		//[2]:		DBK_JPEG_END, JPEG picture decoding is completed.
										//[1]:		DBK_VDB_BUF1_FULL, VDB buffer1 is full, active high		
										//[0]:		DBK_VDB_BUF0_FULL, VDB buffer0 is full, active high	
	
	volatile uint32 DBK_CTRL0;			//[1]:		DBK_DONE, DBK done state only when under manual mode. Active high, "DBK_start" will clear this bit if it's '1'.
										//[0]:		DBK_START, DBK start only hen under manual mode.	Virtula register, write '1' to this bit, self clear.		

	volatile uint32 DBK_CTRL1;			//[0]:		DBK SW configuration is completed. Note: only used for free run mode to indicates HW to work. Once one picture codec is completed, this signal will be cleared by HW

	volatile uint32 DBK_DEBUG;			//[17]:		SLICE_IDLE, Slice level idle, active high
										//[16]:		FRM_IDLE, Frame level idle, active high
										//[14]:		DBUF_FULL, DBK data buffer full flag, active high
										//[13]:		DBUF_EMP, DBK data buffer empty flag, Active high
										//[12]:		DBUF_PTR, DBK data buffer current pointer
										//[10:8]:	VDBM_FSM_ST
										//[6:4]:	CTRL_FSM_ST
										//			DBK_CTRL FSM state
										//			3'h0: IDLE
										//			3'h1: BYPASS OUT
										//			3'h2: DBK_FLT
										//			3'h3: DBK_OUT
										//			3'h4: WAIT_MBC_DONE
										//			3'h5: LEFT_TRANS
										//			3'h6: WAIT_END
										//[2]:		DBK_VDB_REQ
										//[1]:		WFIFO_DBK_FULL
										//[0]:		RFIFO_DBK_EMP
									
	volatile uint32	HDBK_MB_INFO;		//[29:24]:	mb_x
										//[21:16]:	qp_top;
										//[13:8] :	qp_left;
										//[5:0]  :	qp_cur;

	volatile uint32	HDBK_BS_H0;			//[31:28]:	bs_h7
										//....
										//[15:12]:	bs_h3
										//[11:8]:	bs_h2
										//[7:4]:	bs_h1
										//[3:0]:	bs_h0;
								
	volatile uint32	HDBK_BS_H1;			//[31:28]:	bs_h15
										//....
										//[15:12]:	bs_h11
										//[11:8]:	bs_h10
										//[7:4]:	bs_h9
										//[3:0]:	bs_h8;

	volatile uint32	HDBK_BS_V0;			//[31:28]:	bs_v7
										//....
										//[15:12]:	bs_v3
										//[11:8]:	bs_v2
										//[7:4]:	bs_v1
										//[3:0]:	bs_v0;

	volatile uint32	HDBK_BS_V1;			//[31:28]:	bs_v15
										//....
										//[15:12]:	bs_v11
										//[11:8]:	bs_v10
										//[7:4]:	bs_v9
										//[3:0]:	bs_v8;
	
	volatile uint32	HDBK_PARS;			//[20:16]:	chroma_qp_index_offset [-12, 12]
										//12~8:		alpha_offset	[-12, 12]
										//4~0:		beta_offset	[-12, 12]

	volatile uint32 HDBK_CFG_FINISH;	//0: write 1 to indicate configure finished

	uint32	rdbk_cfg0;						//[27:24]	mb_type
											//[23:0]	cbp

	uint32	rdbk_cfg1;						//[27:24]	mb_type_left
											//[23:0]	cbp_left

	uint32	rdbk_cfg2;						//[27:24]   mb_type_above
											//[23:0]	cbp_above

	uint32	rdbk_cfg3;						//[27:24]	mb_type_topleft
											//[23:0]	cbp_topleft

	uint32	rdbk_cfg4;						//[31:16]	mvd
											//[15:0]	mvd_left

	uint32	rdbk_cfg5;						//[31:16]	mvd_above
											//[15:0]	mvd_topleft

	uint32	rdbk_cfg6;						//[26]      beSmallPic
											//[25]		beTureBPic
											//[24]		bOnRightEdge
											//[23]		bOnLeftEdge
											//[22]		bOnBottomEdge
											//[21]		bOnTopEdge
											//[20:16]	uQP_left
											//[12:8]	uQP_above
											//[4:0]		uQP_current

	uint32 VP8DBK_CFG0;						//[31:24]	mbflim
											//[21:16]	block_inside_limint. lim
											//[13:8]	Sub block edge limit. loop filter level. flim
											//[5:4]		HEVThreshold. mbthr
											//[3]		b_1st_row. 
											//[2]		b_1st_col
											//[1]		Dc diff. 1, nonzero. 0, zero
											//[0]		filter type. NORMAL_LOOPFILTER = 0,SIMPLE_LOOPFILTER = 1.
	
}VSP_DBK_REG_T;


typedef struct vsp_global_reg_tag
{
	volatile uint32 VSP_CFG0;		//[31:17]: reserved, moved to DCAM:VSP_TIME_OUT
									//[19]: TIME_OUT_ENA, Time out enable, only enable this mode under command queque execution
									//[18]: H264_MP_MODE, H264 Main profile flag, active high 
									//[17]: STREAM_LE, Bit stream is little endian mode, active high
									//[16]: rotation_ena   1: rotation   0: no rotation
									//[15]: DATA_LE, Data is little endian format, 0 - big endian, 1 - little endian
									//[14]: ENC_DEC, codec flag, 1-enc, 0-dec
									//[13]: MEA_EB, active high
									//[12]: DBK_EB, active high
									//[11]: RESERVED
									//[10:8]: STANDARD, VSP standard, 
									//		3'b000 - H.263, 3'b001 -  MPEG4, 3'b010 - JPEG, 3'b011 - FLV_V1, 3'b100-H.264	
									//		3'b101 - RV8, 3'b110 - RV9, 3'b111 - VP8
									//[7]: ARB_EB, Arbitor enable. Active high
									//[6]: VLD_EB, active high
									//[5]: VLC_EB, vlc enable, active high
									//[4]: BSMW_EB, BSMW enable, active high
									//[3]: BSMR_EB, BSMR enable, active high
									//[2]: MCA_EB, MCA enable, active high
									//[1]: DCT_EB, DCT enable, active high
									//[0]: MBC_EB, MBC enable, active high
	
	volatile uint32 VSP_CFG1;		//[26£º24] MB_FORMAT,3'b000 - 4:2:0 format
									//					 3'b001 - 4:1:1 format
									//					 3'b010 - 4:4:4 format
									//					 3'b011 - 4:2:2 format
									//					 3'b100 - 4:0:0 format
									//[20:12]: MB_Y_MAX, Current MB ID in Y direction, Note: maximum is 256 MCU
									//[8:0]: MB_X_MAX, Current MB ID in X direction, Note: maximum is 256 MCU  
*/
	/*Noted by Xiaowei, for supporting VGA size, the bitwidth of mb_x_id and mb_y_id should be modified from 5 to 6, xiaowei,20081229*/
/*	volatile uint32 VSP_CTRL0;		//[14:8]: MB_Y_ID, Current MB ID in Y direction, Only for MPEG4/H.263/ASP decoding, Note: maximum is 512 pixel
									//[6:0]: MB_X_ID, Current MB ID in X direction,	Only for MPEG4/H.263/ASP decoding. Note: maximum is 512 pixel

	volatile uint32 VSP_TST;		//[24]: VSP_BUSY
									//[21]: VLD_BUSY
									//[20]: VLC_BUSY
									//[19]: BSM_BUSY
									//[18]: MCA_BUSY
									//[17]: DCT_BUSY
									//[16]: MBIO_BUSY
									//[15:0]: VSP_DEBUG

	volatile uint32 VSP_DBG;		//[5]: VDB_RFIFO_EMP, AHBM RFIFO is empty, active high
									//[4]: VDB_WFIFO_FULL, AHBM WFIFO is full, active high
									//[3]: reserved
									//[2:0]: ARB_STATE, VSP VDB arbiter state.  
	
	volatile uint32 BSM_RST;		//[0]: BSM_RST, Write 1 to rest bsm module

	volatile uint32 TIME_OUT_VAL;		//[31: 0]: Time out value, only valid under command queue mode
	
}VSP_GLB_REG_T;*/
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
		}VIDEO_STD_E;
*/
extern FILE *  g_dbk_trace_fp;
extern VSP_DBK_REG_T  * g_dbk_reg_ptr;
extern VSP_GLB_REG_T  * g_glb_reg_ptr;
extern uint32 * vsp_dbk_out_bfr;		//172x32
extern uint32			g_dbk_line_buf[4096];
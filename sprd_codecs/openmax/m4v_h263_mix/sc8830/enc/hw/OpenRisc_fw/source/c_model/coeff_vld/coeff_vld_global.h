#ifndef _COEFF_VLD_GLOBAL_H_
#define _COEFF_VLD_GLOBAL_H_
//vld line buffer

//vld mb buffer
typedef struct vld_mb_buf_tag
{
	uint32		mb_x;				// for fetching the data in line buffer
	uint32		mb_type;
	uint32		mb_mode;
	uint32		sub_mb_mode[4];
	uint32		slice_type;
	uint32		mb_skip_flag;
	uint32		c_ipred_mode;
	int32		delta_qp;
	uint32		cbp;
	uint32		lmb_avail;
	uint32		tmb_avail;
	uint32		transform_size_8x8_flag;
	uint32		nnz_y0;			//[28:24]:block0, [20:16]: block1, [12:8]: block2, [4:0]: block3 for storing the total coefficient number of the block
	uint32		nnz_y1;
	uint32		nnz_y2;
	uint32		nnz_y3;
	uint32		nnz_cb;			//cb
	uint32		nnz_cr;			//cr
	uint32		coded_dc_flag;	//3 bits for dc
	int32		mvd[16][2];	//13 bits for x and y respectively
	int32		ref[2];		//7:0 block 0
}MB_BUF_T;

//vld line buffer
typedef struct vld_line_buf_tag
{
	uint32		nnz_y;				//Nnz_y0[28:24],Nnz_y1[20:16],Nnz_y2[12:8],Nnz_y3[4:0], for storing the total coefficient number of the block
	uint32		nnz_c;				//Nnz_cb0[28:24],Nnz_cb1[20:16],Nnz_cr0[12:8],Nnz_cr1[4:0]
	uint32		mb_type;			//[0]mb_skip_flag
	uint32		cbp;
	uint32		c_ipred_mode;
	uint32		mb_skip_flag;
	uint32		transform_size_8x8_flag;
	uint32		coded_dc_flag;	//3 bits for dc [0] Y [1] U [2] V
	int32		mvd[4][2];		//13 bits for x and y respectively
	int32		ref[2];		//7:0 block 0
}LINE_BUF_T;

LINE_BUF_T *vldLineBuf;

MB_BUF_T *currMBBuf;
MB_BUF_T *leftMBBuf;

void coeff_vld_module(void);
void (*residual_block)   (int	blk_type, int blk_id, int startIdx, int maxNumCoeff);
void residual_block_cavlc(int	blk_type, int blk_id, int startIdx, int maxNumCoeff);
void residual_block_cabac(int	blk_type, int blk_id, int startIdx, int maxNumCoeff);
void GetnC(int blk_type, int blk_id, int *nc_ptr);
int CoeffTokenDecHigh (
					int			nc, 
					int		*	trailing_one_ptr, 
					int		*	total_coeff_ptr
					);
int H264VldLevDecHigh (
					int		trailing_one, 
					int		total_coeff
					);
void ReadLevVlcHigh (
					 int	suffix_len, 
					 int *	lev_ptr, 
					 int *	sign_ptr,
					 int *	lev_err_ptr
				 );
int SuffixLenIncHigh (int suffix_len, int level);
int H264VldRunDecHigh (
				   int	total_coeff,
				   int	blk_type,
				   int	max_coeff_num
				   );
int GetTotalZerosHigh (
				   int		total_coeff, 
				   int		blk_type, 
				   int *	total_zeros_ptr
				   );
int ReadRunBeforeHigh (int zeros_left, int * run_before_ptr);
void WriteBackTotalCoeffHigh (
						  int	blk_type, 
						  int	blk_id, 
						  int	total_coeff
						  );

//CABAC related
uint32 readMB_tran8x8_flagInfo_CABAC_High(void);
uint32 readMB_skip_flagInfo_CABAC_High(void);
uint32 readMB_typeInfo_CABAC_High(void);
uint32 readB8_typeInfo_CABAC_High (void);
int32 readCBP_CABAC_High (void);
int32 readDquant_CABAC_High (void);
uint32 readPrev_intra_pred_mode_flag_CABAC_High(void);
uint32 readRem_intra_pred_mode_CABAC_High(void);
int32 readCIPredMode_CABAC_High (void);
int32 readBlkCodedFlag_CABAC_High (int blk_cat, int blk_id);
int32 readMVD_CABAC_High (int32 block4x4Idx, int32 list, int32 compIdx);
int32 readRefFrame_CABAC_High (int32 blk_id, int32 list);
void GetnCodedBlockFlag(int blk_type, int blk_id, int *coded_flag_a_ptr, int *coded_flag_b_ptr);
void ArithSigMapDecoderHigh (int blk_type, int blk_id, int	maxNumCoeff);
void ArithLevInforDecHigh (int blk_type, int blk_id);
void TwoBinArithDecHigh (
					 int		syn_type,			//syntax type for first bin to be decoded
					 uint8	*	ctx_bin0_ptr,		//first bin context to be decoded
					 uint8	*	ctx_bin1_ptr,		//second bin context to be decoded
					 int	*	bin0_ptr,			//returned first binary value
					 int	*	bin1_ptr,			//returned second binary value
					 int		glm_sfx_num
					 );
int ArithOneLevDecHigh (
					int blk_cat, 
					int num_t1, 
					int num_lgt1
					);//这个函数应该不需要改了，只需要将那个里头的ctx_bin方面的东西改了就好了。
#endif


#ifndef _IICT_GLOBAL_H_
#define _IICT_GLOBAL_H_

extern int32 dc_value ;
extern int32 nz_flag ;
extern int32 rd_cnt_max ;
extern int32 fetch_max ;
extern int32 rd_u_dc_nzf ;
extern int32 rd_y_dc_nzf ;
extern int32 rd_y_ac_nzf ;
extern int32 rd_nxt_nzf_ena;
extern int32 rd_nxt_dc_ena ; 

extern int32 g_vsp_dct_trans0[8] ;
extern int32 g_vsp_dct_trans1[8] ;

void Iict(int32 is_DC_itrans, int32 blk4x4Idx, int32 is_luma, int32 second_phase, int32 is_h264, int32 cbp26,  int32 need_y_hadama); 
void itrans4x1(
               int *op, //24bit?
               int *ip,//4 pixel in 
			   char decoder_format,
			   char isyahdamard,
			   char step//raw & col
			   );
void itrans8x1(
               int *op, 
               int *ip,//8 pixel in
			   char decoder_format,
			   char step//raw & col
			   
			   );
void itrans4x4blk(short *input, short *output, int pitch, char decoder_format, char isyhadamard);//weihu
void itrans8x8blk(short *input, short *output, int pitch, char decoder_format,char tran_size);
void iquantblk(short *dqcoeff, short *qcoeff, short qp_ac, short qp_dc, char decoder_format, char slice_info, char MB_info, char ycbcr, char isyhadamard, char inter);//weihu
void iqt_module_rv (
					short outbuf[432], //108*64
					short inbuf[432], 
					short i1, //12b//qp_y_per for H.264
					short i2, //12b//qp_y_rem for H.264
					short i3, //12b//qp_v for H.264
					short i4, //12b
					short i5, //12b//qp_u for H.264
					short i6, //12b
					int  i7,  //26b
					char skip_flag, //1b
					//char intra_flag, //1b/4b?
					char need_y_hadama,//1b
					char decoder_format,//4b/3b?
					char slice_info,
					char MB_info
					
					);
void iqt_module ();
void iqt_module_ppa (
					short outbuf[432], //108*64
					short inbuf[432], 
					//short i1, //12b//qp_y_per for H.264
					//short i2, //12b//qp_y_rem for H.264
					//short i3, //12b//qp_v for H.264
					//short i4, //12b
					//short i5, //12b//qp_u for H.264
					//short i6, //12b
					//int  i7,  //26b
					//char skip_flag, //1b
					//char intra_flag, //1b/4b?
					//char need_y_hadama,//1b
					char decoder_format,//4b/3b?
					int slice_info_list[30],
					int MB_info_buf[10]
					
				   );

void Rv_PrintfResidual (int is_intra16orinter16, int cbp, FILE *g_fp_idct_tv);
#endif //#ifndef _IICT_GLOBAL_H_


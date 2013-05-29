#ifndef _PARSER_GLOBAL_H_
#define _PARSER_GLOBAL_H_

#define IS_I_NxN	((slice_type==I_SLICE)&&(mb_type==0)				|| (slice_type==P_SLICE)&&(mb_type==5)					|| (slice_type==B_SLICE)&&(mb_type==23))
#define IS_I_PCM	((slice_type==I_SLICE)&&(mb_type==25)				|| (slice_type==P_SLICE)&&(mb_type==30)					|| (slice_type==B_SLICE)&&(mb_type==48))
#define IS_I_16x16	((slice_type==I_SLICE)&&((mb_type>=1)&&(mb_type<=24)) || (slice_type==P_SLICE)&&((mb_type>=6)&&(mb_type<=29))	|| (slice_type==B_SLICE)&&((mb_type>=24)&&(mb_type<=47)))
#define IS_P_8x8	((slice_type==P_SLICE)&&((mb_type==3)||(mb_type==4))|| (slice_type==B_SLICE)&&(mb_type==22))	

#define Pred_L0	0
#define Pred_L1 1
#define BiPred	2

#define	skip_direct_h264	0
#define	PB16x16_h264	1
#define	PB16x8_h264		2
#define PB8x16_h264		3
#define	PB8X8_h264		4
#define INxN_h264		5
#define	I16_h264		6
#define	IPCM_h264		7

#define SUB_8X8	0
#define SUB_8X4	1
#define	SUB_4X8	2
#define	SUB_4X4	3
#define	direct_8X8	4

#define BLK_LUMA	0
#define BLK_CB		1
#define BLK_CR		2

int slice_info[40];
int inbuf[36]; // parser out put buffer
int dct_para_buf[10]; //for dct para
int mbc_para_buf[4]; //for mbc para
int dbk_para_buf[8]; //for dbk para
int mca_para_buf[38]; //for mca para
int ref_list_buf[25];//refpic list buf

short dct_out_buf[432];//108*64
char mca_out_buf[384];//96*32
char mbc_out_buf[384];//96*32
char dbk_out_buf[864];//108*64

void H264Dec_parse_slice_data (void);
void H264Dec_parse_MB_layer (void);
void H264Dec_parse_MB_pred (int8 mb_type, BOOLEAN transform_size_8x8_flag);
void H264Dec_parse_subMB_pred (int8 mb_type, int8 *sub_mb_type);
void Interpret_mb_mode_I (int8 mb_type);
void Interpret_mb_mode_P (int8 mb_type);
void Interpret_mb_mode_B (int8 mb_type);
void H264_Ipcm_parser ();
#endif
#include <memory.h>
#include "sci_types.h"
#include "video_common.h"
#include "iict_global.h"
#include "common_global.h"
#include "buffer_global.h"
#include "hvld_mode.h"

//#define debugoutput
#ifdef debugoutput

#include <stdio.h>
FILE *fout1;

#endif
#define CLIPZ(x,y,z)   (z<x)? x : ((z>y)? y : z )
#define ISQT_FPRINT //fprintf

static const int cospi8sqrt2minus1 = 20091;
static const int sinpi8sqrt2      = 35468;

const char normadjust4x4[6][4][4]={
	{{10, 13, 10, 13},{ 13, 16, 13, 16},{10, 13, 10, 13},{ 13, 16, 13, 16}},
	{{11, 14, 11, 14},{ 14, 18, 14, 18},{11, 14, 11, 14},{ 14, 18, 14, 18}},
	{{13, 16, 13, 16},{ 16, 20, 16, 20},{13, 16, 13, 16},{ 16, 20, 16, 20}},
	{{14, 18, 14, 18},{ 18, 23, 18, 23},{14, 18, 14, 18},{ 18, 23, 18, 23}},
	{{16, 20, 16, 20},{ 20, 25, 20, 25},{16, 20, 16, 20},{ 20, 25, 20, 25}},
	{{18, 23, 18, 23},{ 23, 29, 23, 29},{18, 23, 18, 23},{ 23, 29, 23, 29}}
};
const char normadjust8x8[6][8][8]={
	{
		{20,  19, 25, 19, 20, 19, 25, 19},
		{19,  18, 24, 18, 19, 18, 24, 18},
		{25,  24, 32, 24, 25, 24, 32, 24},
		{19,  18, 24, 18, 19, 18, 24, 18},
		{20,  19, 25, 19, 20, 19, 25, 19},
		{19,  18, 24, 18, 19, 18, 24, 18},
		{25,  24, 32, 24, 25, 24, 32, 24},
		{19,  18, 24, 18, 19, 18, 24, 18}
	},
	{
		{22,  21, 28, 21, 22, 21, 28, 21},
		{21,  19, 26, 19, 21, 19, 26, 19},
		{28,  26, 35, 26, 28, 26, 35, 26},
		{21,  19, 26, 19, 21, 19, 26, 19},
		{22,  21, 28, 21, 22, 21, 28, 21},
		{21,  19, 26, 19, 21, 19, 26, 19},
		{28,  26, 35, 26, 28, 26, 35, 26},
		{21,  19, 26, 19, 21, 19, 26, 19}
	},
	{
		{26,  24, 33, 24, 26, 24, 33, 24},
		{24,  23, 31, 23, 24, 23, 31, 23},
		{33,  31, 42, 31, 33, 31, 42, 31},
		{24,  23, 31, 23, 24, 23, 31, 23},
		{26,  24, 33, 24, 26, 24, 33, 24},
		{24,  23, 31, 23, 24, 23, 31, 23},
		{33,  31, 42, 31, 33, 31, 42, 31},
		{24,  23, 31, 23, 24, 23, 31, 23}
	},
	{
		{28,  26, 35, 26, 28, 26, 35, 26},
		{26,  25, 33, 25, 26, 25, 33, 25},
		{35,  33, 45, 33, 35, 33, 45, 33},
		{26,  25, 33, 25, 26, 25, 33, 25},
		{28,  26, 35, 26, 28, 26, 35, 26},
		{26,  25, 33, 25, 26, 25, 33, 25},
		{35,  33, 45, 33, 35, 33, 45, 33},
		{26,  25, 33, 25, 26, 25, 33, 25}
	},
	{
		{32,  30, 40, 30, 32, 30, 40, 30},
		{30,  28, 38, 28, 30, 28, 38, 28},
		{40,  38, 51, 38, 40, 38, 51, 38},
		{30,  28, 38, 28, 30, 28, 38, 28},
		{32,  30, 40, 30, 32, 30, 40, 30},
		{30,  28, 38, 28, 30, 28, 38, 28},
		{40,  38, 51, 38, 40, 38, 51, 38},
		{30,  28, 38, 28, 30, 28, 38, 28}
	},
	{
		{36,  34, 46, 34, 36, 34, 46, 34},
		{34,  32, 43, 32, 34, 32, 43, 32},
		{46,  43, 58, 43, 46, 43, 58, 43},
		{34,  32, 43, 32, 34, 32, 43, 32},
		{36,  34, 46, 34, 36, 34, 46, 34},
		{34,  32, 43, 32, 34, 32, 43, 32},
		{46,  43, 58, 43, 46, 43, 58, 43},
		{34,  32, 43, 32, 34, 32, 43, 32}
	}
	
};

extern char weightscale4x4[6][4][4];//6 6*2+2=14*16=224B
extern char weightscale8x8[2][8][8];//2 2*2+2=6*64=384B
/*
char weightscale4x4_intra_default[16] = {
	6,13,20,28,
		13,20,28,32,
		20,28,32,37,
		28,32,37,42
};

char weightscale4x4_inter_default[16] = {
	10,14,20,24,
		14,20,24,27,
		20,24,27,30,
		24,27,30,34
};

char weightscale8x8_intra_default[64] = {
	6,10,13,16,18,23,25,27,
		10,11,16,18,23,25,27,29,
		13,16,18,23,25,27,29,31,
		16,18,23,25,27,29,31,33,
		18,23,25,27,29,31,33,36,
		23,25,27,29,31,33,36,38,
		25,27,29,31,33,36,38,40,
		27,29,31,33,36,38,40,42
};

char weightscale8x8_inter_default[64] = {
	9,13,15,17,19,21,22,24,
		13,13,17,19,21,22,24,25,
		15,17,19,21,22,24,25,27,
		17,19,21,22,24,25,27,28,
		19,21,22,24,25,27,28,30,
		21,22,24,25,27,28,30,32,
		22,24,25,27,28,30,32,33,
		24,25,27,28,30,32,33,35
};
*/
void trace_after_iqt_for_asic_compare ()
{
if(g_vector_enable_flag&VECTOR_ENABLE_DCT)
{
	int32 i;
	int16 *iqt_result_ptr = (int16 *)(vsp_dct_io_0 + COEFF_LUMA_AC_BASE);

	for (i = 0; i < (16*24); i++)
	{
		FormatPrintHexNum(iqt_result_ptr[i], g_fp_idct_tv);
	}
}
	return;
}

void PrintfResidual (int is_intra16, int cbp)
{
if(g_vector_enable_flag&VECTOR_ENABLE_DCT)
{
	int		i;
	int		val;
	int		blk_id;
	int		blk_cbp;
	int		base_addr;

	//printf inversed transform result of luma dc
	if (is_intra16)
	{
		base_addr = 192;

		for (i = 0; i < 8; i++)
		{
			val = vsp_dct_io_0[base_addr + i];
			FPRINTF (g_fp_idct_tv, "%08x\n", val);
		}
	}

	//printf inversed transform result of chroma dc
	if ((cbp >> 24) & 1)
	{
		base_addr = 200;

		for (i = 0; i < 4; i++)
		{
			val = vsp_dct_io_0[base_addr + i];
			FPRINTF (g_fp_idct_tv, "%08x\n", val);
		}
	}

/**********************************************************************************
	printf 24 4x4 blk with non-zero coefficient
	note: if is_intra16, all luma coeff should be printed
	      if original cbp > 15, all chroma coeff should be printed
************************************************************************************/
	for (blk_id = 0; blk_id < 24; blk_id++)
	{
		//blk_cbp;
		if (blk_id < 16)
		{
			if (is_intra16)
				blk_cbp = 1;
			else
				blk_cbp = (cbp >> blk_id) & 1;
		}
		else
		{
			if ((cbp >> 24) & 1)
				blk_cbp = 1;
			else
				blk_cbp = 0;
		}
		
		if (blk_cbp)
		{	
			base_addr = blk_id * 8;	
			
			for (i = 0; i < 8; i++)
			{
				val = vsp_dct_io_0[base_addr + i];
				FPRINTF (g_fp_idct_tv, "%08x\n", val);
			}			
		}
	}
}
}


void Rv_PrintfResidual (int is_intra16orinter16, int cbp, FILE *g_fp_idct_tv)
{

	int		i;
	int		val;
	int		blk_id;
	int		blk_cbp;
	int		base_addr;


	//printf inversed transform result of luma dc
	if (is_intra16orinter16)
	{
		base_addr = 192;

		for (i = 0; i < 8; i++)
		{
			val = vsp_dct_io_0[base_addr + i];
			if (i == 0)
			{
				//IQT_FPRINTF(g_fp_idct_tv,"mb %d luma dc: \n",mb_num);
			}
			FPRINTF (g_fp_idct_tv, "%08x\n", val);
		}
	}


/**********************************************************************************
	printf 24 4x4 blk with non-zero coefficient
	note: if is_intra16, all luma coeff should be printed
	      if original cbp > 15, all chroma coeff should be printed
************************************************************************************/
	for (blk_id = 0; blk_id < 24; blk_id++)
	{
		//blk_cbp;

		if (is_intra16orinter16 && (blk_id<16))
			blk_cbp = 1;
		else
			blk_cbp = (cbp >> blk_id) & 1;

		
		if (blk_cbp)
		{	
			base_addr = blk_id * 8;	
			
			for (i = 0; i < 8; i++)
			{
				val = vsp_dct_io_0[base_addr + i];
				if (i == 0)
				{
					//IQT_FPRINTF(g_fp_idct_tv,"mb %d blk %d ac: \n",mb_num, blk_id);
				}				
				FPRINTF (g_fp_idct_tv, "%08x\n", val);
			}			
		}
	}
}



void iqt_module ()
{
 	int32 mb_type;
 	int32 cbp26 = (g_dct_reg_ptr->iict_cfg0 >> 0) & 0x3ffffff;
	int32 is_h264 = (((g_glb_reg_ptr->VSP_CFG0 >> 8)&0xf) == VSP_H264);
	int32 rv_blk16_mode = (g_dct_reg_ptr->ridct_cfg0 >>29) & 0x1;
	int32 blk4x4Idx;
	int32 need_y_hadama;
	if (is_h264)
	{
		mb_type = (g_dct_reg_ptr->iict_cfg0 >> 28) & 0x3;
	} 

	need_y_hadama = (is_h264 && mb_type == 0) ||(!is_h264 && rv_blk16_mode); //H264 intra 16x16||Real intra or inter 16

	// IQT Bypass
	if (is_h264 && ((mb_type == 2) || (mb_type == 1)))
	{
		//if h264 ipcm or skip mb. iqt bypass
		return;
	}

	//Read first nzf 
	if (is_h264 && (cbp26 & (1<<24)))
	{	// if UV_HADAMA needed, read nz flag for u luma
		rd_u_dc_nzf = 1;
	}
	else if (need_y_hadama)
	{	// if Y_HADAMA needed, read nz flag for y luma
		rd_y_dc_nzf = 1;
	}else
	{	// read nz flag for first y chroma
		rd_y_ac_nzf = 1;
	}
	rd_cnt_max = 0;
	fetch_max = 0;
	Iict( 0, 0, 0, 0, is_h264, cbp26, need_y_hadama);
	rd_u_dc_nzf = 0;
	rd_y_dc_nzf = 0;
	rd_y_ac_nzf = 0;
	
	// Chroma DC 
	if (is_h264 && (cbp26 & (1<<24)))
	{
		// if cbp[24], H264 UV HADAMA
		rd_cnt_max = 4;
		fetch_max = 1;
		Iict( 1, 0, 0, 0, is_h264, cbp26, need_y_hadama);
		Iict( 1, 0, 0, 1, is_h264, cbp26, need_y_hadama);
	}

	// Luma Dc
	if (need_y_hadama)
	{
		rd_cnt_max = 9;
		fetch_max = 7;
		Iict(1, 0, 1, 0, is_h264, cbp26,  need_y_hadama);
		rd_cnt_max = 12;
		Iict(1, 0, 1, 1, is_h264, cbp26,  need_y_hadama);
	}


	// Luma AC
	for (blk4x4Idx = 0; blk4x4Idx < 16; blk4x4Idx++)
	{
	 
		if (need_y_hadama || (!need_y_hadama && (cbp26 & (1<<blk4x4Idx))))
		{
			rd_cnt_max = 9;
			fetch_max = 7;
			Iict(0, blk4x4Idx, 1, 0, is_h264, cbp26,  need_y_hadama);
			rd_cnt_max = 12;
			Iict(0, blk4x4Idx, 1, 1, is_h264, cbp26,  need_y_hadama);
		} 
		else
		{
			rd_cnt_max = 0;
			fetch_max = 0;
			rd_nxt_nzf_ena = 1;
			Iict( 0, blk4x4Idx, 0, 0, is_h264, cbp26, need_y_hadama);	
			rd_nxt_nzf_ena = 0;
			rd_nxt_dc_ena = 1;
			Iict( 0, blk4x4Idx, 0, 0, is_h264, cbp26, need_y_hadama);
			rd_nxt_dc_ena = 0;
		}

	}

	// Chroma AC
	for (blk4x4Idx = 16; blk4x4Idx < 24; blk4x4Idx++)//4*2(u and v)
	{
		if ((is_h264 && (cbp26 & (1<<24))) || (cbp26 & (1<<blk4x4Idx)))
		{
			rd_cnt_max = 9;
			fetch_max = 7;
			Iict(0, blk4x4Idx, 0, 0, is_h264, cbp26,  need_y_hadama);
			rd_cnt_max = 12;
			Iict(0, blk4x4Idx, 0, 1, is_h264, cbp26,  need_y_hadama);
		}
		else
		{
			//Read next nfz, dc
			rd_cnt_max = 0;
			fetch_max = 0;
			rd_nxt_nzf_ena = 1;
			Iict( 0, blk4x4Idx, 0, 0, is_h264, cbp26, need_y_hadama);	
			rd_nxt_nzf_ena = 0;
			rd_nxt_dc_ena = 1;
			Iict( 0, blk4x4Idx, 0, 0, is_h264, cbp26, need_y_hadama);
			rd_nxt_dc_ena = 0;
		}
	}
	if (!is_h264)
	{
	Rv_PrintfResidual(rv_blk16_mode ,(cbp26  & 0xffffff),g_fp_idct_tv);
	}
	else
	{
 	PrintfResidual ((mb_type == 0), cbp26);
//	trace_after_iqt_for_asic_compare();
	}
}//weihu



void itrans4x1(
               int *op, //24bit?
               int *ip,//4 pixel in 
			   char decoder_format,
			   char isyahdamard,
			   char step//raw & col
			   )
{
	int A1, A2, A3, A4, A5,A6;
	int tmp0;
    int B1, B2, B3, B4;
	int C1, C2, C3, C4;
	
	short a, c, f, r,s,t;//,,,round,shift,
	int z;//clip
	
	if(isyahdamard==1)//if hadamard transform
	{
		a=1; c=1; f=1; 
		if((decoder_format==STREAM_ID_REAL8)||(decoder_format==STREAM_ID_REAL9))//real8/9
		{
			 
			if(step==0)
			{   
				r=0; s=0; z=524288; t=1;//20bit
			}
			else
			{
				r=0; s=15; z=2048; t=48;
			}
		}
		else if(decoder_format==STREAM_ID_H264)
		{

			r=0; s=0; t=1; z=32768;
			
		
		}
		else if((decoder_format==STREAM_ID_VP8)||(decoder_format==STREAM_ID_WEBP))//if VP8/webp 
		{
			//a=1; c=1; f=1; 
			if(step==0)
			{   
				r=0; s=0; z=65536; 
			}
			else
			{
				r=3; s=3; z=65536;
			}
		}
		


		A1 = ip[0]+r;
		A2 = ip[1];
        A3 = ip[3];
		A4 = ip[2];
		A5 = ip[3];
		A6 = ip[2];     //for hadmard architecture
	}
	else//if  IDCT transform
	{
		if((decoder_format==STREAM_ID_REAL8)||(decoder_format==STREAM_ID_REAL9)||(decoder_format==STREAM_ID_VC1))//real8/9 VC1
		{
			if(decoder_format==STREAM_ID_VC1)//vc1
			{
				a=17; c=22; f=10; 
				if(step==0)
				{   
					r=4; s=3; z=4096; 
				}
				else
				{
					r=64; s=7; z=512;
				}
			}
			else//real8/9
			{
			  a=13; c=17; f=7;  
			  if(step==0)
			  {   
				r=0; s=0; z=524288; t=1;
			  }
			  else
			  {
				r=512; s=10; z=512;t=1;
			  }
			}

		  A1 = ip[0]*a+r;
		  A2 = ip[1]*a;
		  A3 = ip[3]*(c+f);        
		  tmp0 = (ip[2]+ip[3])*f;
		  A4 = tmp0;               
		  A5 = tmp0;               
		  A6 = ip[2]*(c-f); //nomal IDCT architecture       
		}
		else if(decoder_format==STREAM_ID_H264)
		{
			a=1; c=1; //f=1/2;  
			if(step==0)
			{   
				r=0; s=0; z=32768; t=1;
			}
			else
			{
				r=32; s=6; z=32768;t=1;
			}

			A1 = ip[0]*a+r;
			A2 = ip[1]*a;
			A3 = ip[3]*c;       
			A4 = ip[2]>>1;    //ip[2]*f               
			A5 = ip[3]>>1;    //ip[3]*f    
			A6 = ip[2]*c; //nomal IDCT architecture       
		}
		else if((decoder_format==STREAM_ID_VP8)||(decoder_format==STREAM_ID_WEBP))//if VP8/webp 
		{
			a=1;
			if(step==0)
			{   
				r=0; s=0; z=65536; 
			}
			else
			{
				r=4; s=3; z=65536;
			}
			A1 = ip[0]+r;
			A2 = ip[1];
			A3 = ip[3] + ((ip[3] * cospi8sqrt2minus1) >> 16);
			A4 = (ip[2] * sinpi8sqrt2) >> 16;
			A5 = (ip[3] * sinpi8sqrt2) >> 16;  //ip[2] + ((ip[2] * cospi8sqrt2minus1) >> 16);
			A6 = ip[2] + ((ip[2] * cospi8sqrt2minus1) >> 16);//(ip[3] * sinpi8sqrt2 ) >> 16;                   
		}
		
       

	
	}
    
	
	
    B1 = A1 + A2;
	B2 = A1 - A2;
	B3 = A4 - A3;
	B4 = A5 + A6;
	
	C1 = B1 + B4;
	C2 = B2 + B3;
	C3 = B2 - B3;
	C4 = B1 - B4;
	
	op[0] = CLIPZ(-z, z-1, (C1*t)>>s);
	op[1] = CLIPZ(-z, z-1, (C2*t)>>s);
	op[2] = CLIPZ(-z, z-1, (C3*t)>>s);
	op[3] = CLIPZ(-z, z-1, (C4*t)>>s);
	
	return;
	
}

void itrans4x4blk(short *input, short *output, int pitch, char decoder_format,char isyhadamard)
{
    int i,j;
   
    int shortpitch = pitch >> 1;
	int transpose_buf_tmp[16];//weihu //for transpose//20bit?
	int transpose_buf[16];//weihu//20bit?
    int *ip;
    int *op;//output;
	int input1[16];
	int output1[384];

	for (i = 0; i < 16; i=i+1)//short to int
	{
         input1[i]=input[i];
	}

	//row dct
	ip = input1;
    op = transpose_buf;//output;
	for (i = 0; i < 16; i=i+4)
    {
       itrans4x1(&op[i], &ip[i], decoder_format, isyhadamard, 0);
		
    }

	//transpose
	ip=transpose_buf_tmp;
	op = transpose_buf;
	for (i = 0; i < 4; i++)
    {
		ip[0]=op[0];
        ip[1]=op[4];
		ip[2]=op[8];
		ip[3]=op[12];
		op++;
		ip +=4;
	}
	ip=transpose_buf_tmp;
	op = transpose_buf;
	for (i = 0; i < 16; i++)
    {
		op[i]=ip[i];
    }
	/*
    fprintf(fout1,"**********trans*******************%d\n",isyhadamard);
	for (i = 0; i < 4; i++)
	{
		fprintf(fout1,"%d %d %d %d\n",op[i*4+0],op[i*4+1],op[i*4+2],op[i*4+3]);
	}*/


    //col dct
	ip=transpose_buf;
    if((decoder_format==STREAM_ID_H264)&&(!isyhadamard))
		op=transpose_buf;
	else
        op=output1;
	//for (i = 0,j=0; i < 16; i+=4,j+=shortpitch)
	for (i = 0; i < 16; i+=4)
    {
         itrans4x1(&op[i], &ip[i], decoder_format, isyhadamard, 1);
        		
       
    }

	//second transpose
    if((decoder_format==STREAM_ID_H264)&&(!isyhadamard))
	{
		
		ip=transpose_buf_tmp;
		op = transpose_buf;
		for (i = 0; i < 4; i++)
		{
			ip[0]=op[0];
			ip[1]=op[4];
			ip[2]=op[8];
			ip[3]=op[12];
			op++;
			ip +=4;
		}
		ip=transpose_buf_tmp;
		op = output1;
		for (i = 0; i < 16; i++)
		{
			op[i]=ip[i];
		}
	}

	for (i = 0,j=0; i < 16; i+=4,j+=shortpitch)//int to short &&  add pitch
	{
		output[j]=output1[i];
		output[j+1]=output1[i+1];
		output[j+2]=output1[i+2];
		output[j+3]=output1[i+3];
	}

}

void itrans8x1(
               int *op, 
               int *ip,//8 pixel in
			   char decoder_format,
			   char step//raw & col
			   
			   )
{
	int A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14;
	int tmp0;
    int B1, B2, B3, B4, B5, B6, B7, B8;
    int C1, C2, C3, C4, C5, C6, C7, C8;
	int D1, D2, D3, D4, D5, D6, D7, D8;
	
	//short a, b, c, d, e, f, g, r0, r1, s, z;//,,,,,,,round,shift,clip;
	short a, c, f, r0, r1, s;//,,,,,,,round,shift,clip;
	int z;
	short x,y,z0,z1,m;
	
	if(decoder_format==STREAM_ID_VC1)//if VC1 decoding
	{
		a=12; c=16; f=6; x=0; y=2; z0=4; z1=0; m=3; 
		if(step==0)
		{   
			r0=4; r1=0; s=3; z=4096; 
		}
		else
		{
			r0=64; r1=1; s=7; z=512;
		}

		A1 = ip[0]*a+r0;
		A2 = ip[1]*a;
		A3 = ip[3]*(c+f);
		tmp0 = (ip[2]+ip[3])*f;
		A4 = tmp0;
		A5 = tmp0;
		A6 = ip[2]*(c-f);
	}
	else if(decoder_format==STREAM_ID_H264)
	{
		a=1; c=1;  x=2; y=0; z0=3; z1=2; m=1; //f=1/2;
		if(step==0)
		{   
			r0=0; r1=0; s=0; z=32768; 
		}
		else
		{
			r0=32; r1=0; s=6; z=32768;
		}

		A1 = ip[0]*a+r0;
		A2 = ip[1]*a;
		A3 = ip[3]*c;       
		A4 = ip[2]>>1;    //ip[2]*f               
		A5 = ip[3]>>1;    //ip[3]*f    
		A6 = ip[2]*c; //nomal IDCT architecture       
	}
	
	
	
	A7 = ip[4]*z0>>z1;
	A8 = ip[5]*z0>>z1;
	A9 = ip[6]*(-z0)>>z1;
	A10 = ip[7]*(-z0)>>z1;
	A11 = (ip[5]+ip[6])*m;
	A12 = (ip[7]-ip[4])*m;
	A13 = (ip[7]+ip[4])*m;
	A14 = (ip[5]-ip[6])*m;
	
	
    B1 = A1 + A2;
	B2 = A1 - A2;
	B3 = A4 - A3;
	B4 = A5 + A6;
	
	B5 = A7 + A11;//5+6
	B6 = A8 + A12;//7-4
	B7 = A9 + A13;//7+4
	B8 = A10 + A14;//5-6
	
	
	C1 = B1 + B4;
	C2 = B2 + B3;
	C3 = B2 - B3;
	C4 = B1 - B4;
	
	C5 = (B5>>x) + (B8<<y);
	C6 = (B6>>x) + (B7<<y);
	C7 = -(B6<<y) + (B7>>x);
	C8 = (B5<<y) - (B8>>x);
	
   	D1 = C1 + C8;
	D2 = C2 + C7;
	D3 = C3 + C6;
	D4 = C4 + C5;
	D5 = C4 - C5;
	D6 = C3 - C6;
	D7 = C2 - C7;
	D8 = C1 - C8;
	
	
	op[0] =  CLIPZ(-z, z-1,D1>>s);
	op[1] =  CLIPZ(-z, z-1,D2>>s);
	op[2] =  CLIPZ(-z, z-1,D3>>s);
	op[3] =  CLIPZ(-z, z-1,D4>>s);
	op[4] =  CLIPZ(-z, z-1,(D5+r1)>>s);
	op[5] =  CLIPZ(-z, z-1,(D6+r1)>>s);
	op[6] =  CLIPZ(-z, z-1,(D7+r1)>>s);
	op[7] =  CLIPZ(-z, z-1,(D8+r1)>>s);
	
	return;
}

void itrans8x8blk(short *input, short *output, int pitch, char decoder_format,char tran_size)//tran_size 0:8x8 1:8*4 2:4x8 3:4x4
{
    int i,j;
   
    int shortpitch = pitch >> 1;
	int transpose_buf_tmp[64];//weihu //for transpose//20bit?
	int transpose_buf[64];//weihu//20bit?
    int *ip;
    int *op;//output;
	int input1[64];
	int output1[384];

	for (i = 0; i < 64; i=i+1)//short to int
	{
         input1[i]=input[i];
	}

	//row dct
	ip = input1;
    op = transpose_buf;//output;
	for (i = 0; i < 64; i=i+8)
    {
		if((tran_size==1)||(tran_size==3))//1:8x4 3:4x4
		{
			 itrans4x1(&op[i], &ip[i], decoder_format, 0, 0);
			 itrans4x1(&op[i+4], &ip[i+4], decoder_format, 0, 0);
		}
		else
             itrans8x1(&op[i], &ip[i], decoder_format,  0);
		
    }

	//transpose
	ip=transpose_buf_tmp;
	op = transpose_buf;
	for (i = 0; i < 8; i++)
    {
		ip[0]=op[0];
        ip[1]=op[8];
		ip[2]=op[16];
		ip[3]=op[24];
		ip[4]=op[32];
        ip[5]=op[40];
		ip[6]=op[48];
		ip[7]=op[56];
		op++;
		ip +=8;
	}
	ip=transpose_buf_tmp;
	op = transpose_buf;
	for (i = 0; i < 64; i++)
    {
		op[i]=ip[i];
    }
	
    /*fprintf(fout1,"**********trans*******************%d\n",isyhadamard);
	for (i = 0; i < 4; i++)
	{
		fprintf(fout1,"%d %d %d %d\n",op[i*4+0],op[i*4+1],op[i*4+2],op[i*4+3]);
	}*/


    //col dct
	ip=transpose_buf;
	if((decoder_format==STREAM_ID_H264)||(decoder_format==STREAM_ID_VC1))//h264||vc1
	    op=transpose_buf;
	else
        op=output1;
	//for (i = 0,j=0; i < 64; i+=8,j+=shortpitch)
    for (i = 0; i < 64; i+=8)
    {
		if((tran_size==2)||(tran_size==3))//2:4x8 3:4x4
		{
			itrans4x1(&op[i], &ip[i], decoder_format, 0, 1);
			itrans4x1(&op[i+4], &ip[i+4], decoder_format, 0, 1);
		}
		else
            itrans8x1(&op[i], &ip[i], decoder_format,  1);
        		
       
    }

	//second transpose
    if((decoder_format==STREAM_ID_H264)||(decoder_format==STREAM_ID_VC1))//h264||vc1
	{
		
		ip=transpose_buf_tmp;
		op = transpose_buf;
		for (i = 0; i < 8; i++)
		{
			ip[0]=op[0];
			ip[1]=op[8];
			ip[2]=op[16];
			ip[3]=op[24];
			ip[4]=op[32];
			ip[5]=op[40];
			ip[6]=op[48];
			ip[7]=op[56];
			op++;
			ip +=8;
		}
		ip=transpose_buf_tmp;
		op = output1;
		for (i = 0; i < 64; i++)
		{
			op[i]=ip[i];
		}
	}

	for (i = 0,j=0; i < 64; i+=8,j+=shortpitch)//int to short && add pitch
	{
		output[j]=output1[i];
		output[j+1]=output1[i+1];
		output[j+2]=output1[i+2];
		output[j+3]=output1[i+3];
		output[j+4]=output1[i+4];
		output[j+5]=output1[i+5];
		output[j+6]=output1[i+6];
		output[j+7]=output1[i+7];
	}

}


char map4[4]={0,2,1,3};
char map8[8]={0,4,2,6,1,5,3,7};


void iquantblk(short *dqcoeff, short *qcoeff, short qp_ac, short qp_dc, char decoder_format, char slice_info, char MB_info, char ycbcr, char isyhadamard, char inter)
{
    int i,j;
	short QP;
	short qp_const;
	short shift_value;
	short lshift_value;
	short per=qp_dc;
	short rem=qp_ac;
	short weight;
	short normadjust;
	char blk_size;//4X4
	char scale_en;//4X4
	
	char NUniform;//for vc1
   	
	if(decoder_format==STREAM_ID_H264)//h.264
	{
      blk_size=((MB_info&0x1)&&(ycbcr==0))&&(decoder_format==STREAM_ID_H264)?8:4;//4X4
	  //inter=(MB_info&0x2)>>1;
	  scale_en=slice_info&0x1;//4X4
	}
	else if((decoder_format==STREAM_ID_REAL8) ||(decoder_format==STREAM_ID_REAL9)||(decoder_format==STREAM_ID_WEBP) ||(decoder_format==STREAM_ID_VP8))
	{
		blk_size= 4;//4X4
		//inter=(MB_info&0x2)>>1;
		//scale_en=0;//4X4
	}
	else if(decoder_format==STREAM_ID_VC1)//vc1
	{
        blk_size=8;
		NUniform=slice_info&0x1;
	}
	
    for (i = 0; i < blk_size; i++)
    {
        for (j = 0; j < blk_size; j++)
		{
			//QP
			if(decoder_format==STREAM_ID_H264)
			{
							
				if(scale_en)
					weight=(blk_size==4) ? weightscale4x4[ycbcr+inter*3][map4[i]][map4[j]] : weightscale8x8[inter][map8[i]][map8[j]];//need map4 map8
				else
					weight=16;
            
                normadjust=(blk_size==4) ? normadjust4x4[rem][map4[i]][map4[j]] : normadjust8x8[rem][map8[i]][map8[j]];
				QP=weight*normadjust;//8*8mul

			}
			else if((i==0)&&(j==0)||isyhadamard&&(decoder_format==STREAM_ID_REAL9)&&((j==2)&&(i==0)||(i==2)&&(j==0)))//isrv9
				QP=qp_dc;
			else
				QP=qp_ac;

			//qp_const shift_value lshift_value
			if(decoder_format==STREAM_ID_H264)
			{
			     if((i==0)&&(j==0)&&(isyhadamard||(ycbcr!=0)))
				 {
					 if(isyhadamard)
					 {
						 if(per>=6)
						 {
							 qp_const=0;
							 shift_value=0;
							 lshift_value=per-6;//0-2 per:6-8
						 }
						 else
						 {
							 qp_const=1<<(5-per);//0-5
							 shift_value=6-per;//1-6 per:0-5
							 lshift_value=0;
						 }

					 }					
					 else//chroma
					 {
						 
							 qp_const=0;
							 shift_value=5;
							 lshift_value=per;//0-6
						 

					 }

				 }
				 else
				 {
					 if(per>=4)
					 {
						 qp_const=0;
						 shift_value=0;
						 lshift_value=per-4;//0-4 per:4-8
					 }
					 else
					 {
						 qp_const=1<<(3-per);//0-3
						 shift_value=4-per;//1-4 per:0-3
						 lshift_value=0;
					 }
					 
				 }
			}
			else if((decoder_format==STREAM_ID_REAL9) ||(decoder_format==STREAM_ID_REAL8))//real8/9/10
			{
				qp_const=8;
				shift_value=4;
				lshift_value=0;
			}
			else if(decoder_format==STREAM_ID_VC1)//vc1
			{
				shift_value=0;
				lshift_value=0;
				
				if(!inter&&(i==0))
					qp_const= 0;
				else
					qp_const= (qcoeff[blk_size*i+j]<0)&&NUniform ? (qp_ac>>1) : 0;
			}
			else if((decoder_format==STREAM_ID_WEBP) ||(decoder_format==STREAM_ID_VP8))//webp/vp8
			{
				shift_value=0;
				lshift_value=0;
				qp_const= 0;
			}
			
			dqcoeff[blk_size*i+j] = ((qcoeff[blk_size*i+j] * QP+qp_const)<<lshift_value)>>shift_value;//16*16mul
			//fprintf(dct_out,"%d %d %x\n",i,j,(dct_out_buf[i*16+j])&0xffff);
		}//j 4*pixel parallel processing
    }
}



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
					
				   )
{
   
	int i,j,k,b4x4inblk;
   

	
	short qp_dc,qp_ac;

	//int diff_offset;
	int buf1[8];//chroma DC short to int
	short qp_ac_y; 
	short qp_dc_y;
	short qp_ac_y2; 
	short qp_dc_y2; 
	short qp_ac_uv; 
	short qp_dc_uv; 
	int  cbp26;
    char qp_y_per;
    char qp_y_rem;
	char qp_u_per;
    char qp_u_rem;
	char qp_v_per;
    char qp_v_rem;
	char tran_size;//4X4
	//char scale_en;//4X4
	char ycbcr;
	char intra_flag;
	char inter;
	char isyhadamard;

	switch(decoder_format) {
    case STREAM_ID_JPEG://Jpeg
	case STREAM_ID_MPEG4://mpeg4/h.263
    case STREAM_ID_H263://mpeg4/h.263
    case STREAM_ID_FLVH263://mpeg4/h.263
		break;
    case STREAM_ID_WEBP://webp
	case STREAM_ID_REAL8:     //real8
    case STREAM_ID_REAL9:     //real9/10
    case STREAM_ID_VP8://vp8
		{
			qp_ac_y=i1; //12b
			qp_dc_y=i2; //12b
			qp_ac_y2=i3; //12b
			qp_dc_y2=i4; //12b
			qp_ac_uv=i5; //12b
			qp_dc_uv=i6; //12b
			cbp26=i7;//26b
		    inter=(MB_info&0x2)>>1;
		
	
		}
		break;
	case STREAM_ID_VC1:     //VC1
		{
			qp_ac_y=i1; //12b
			qp_dc_y=i2; //12b
			//qp_ac_uv=i1; 
			//qp_dc_uv=i2; 
			//intra_flag=MB_info&x3f;//6b
			//trans_type=(MB_info>>6)&0xfff;//12b
			cbp26=i7&0x111111;//26b
            //NUniform=slice_info&0x1;//1b
			
			
		}
		break;
   	default:    //STREAM_ID_H264:h.264
		{
			qp_y_per=i1&0xf;//4b
			qp_y_rem=i2&0x7;//3b
			qp_u_per=i3&0xf;//4b
			qp_u_rem=i4&0x7;//3b
			qp_v_per=i5&0xf;//4b
			qp_v_rem=i6&0x7;//3b
			cbp26=i7;//26b
			//tran_size=MB_info&0x1;//4X4
			inter=(MB_info&0x2)>>1;
			//scale_en=slice_info&0x1;//4X4
			
		}
	}
    
	

 //   fout1=fopen("E:/project/sc8810/Firmware_FPGA_verification/h264_decoder/simulation/vc/WinPrj/quant_in.txt","ab+");


 if(!skip_flag)    
 {
	     
		 if ((decoder_format==STREAM_ID_H264)&& (cbp26 & (1<<24)))//h.264 chroma dc
		 {
			   /* fprintf(fout1,"25 **********chroma DC in*******************\n");
				for (i = 0; i < 2; i++)
				{
					fprintf(fout1,"%d %d %d %d\n",inbuf[400+i*4+0],inbuf[400+i*4+1],inbuf[400+i*4+2],inbuf[400+i*4+3]);
				}*/
				for (i = 0; i < 8; i++)
				{
					buf1[i] =inbuf[400+i];			
					
				}//weihu
				itrans4x1(buf1, buf1, decoder_format, 1, 0);
				itrans4x1(buf1+4, buf1+4, decoder_format, 1, 0);
				for (i = 0; i < 8; i++)
				{
					inbuf[256+i*16] =buf1[i];			
					
				}//weihu
				/*fprintf(fout1,"25 **********chroma DC out*******************\n");
				for (i = 0; i < 2; i++)
				{
					fprintf(fout1,"%d %d %d %d\n",buf1[i*4+0],buf1[i*4+1],buf1[i*4+2],buf1[i*4+3]);
				}*/
				
		 }
		 
		 if (need_y_hadama)
		{

				/*fprintf(fout1,"24 **********h quant in*******************%d %d\n",qp_ac_y2, qp_dc_y2);
				for (i = 0; i < 4; i++)
				{
					fprintf(fout1,"%d %d %d %d\n",inbuf[384+i*4+0],inbuf[384+i*4+1],inbuf[384+i*4+2],inbuf[384+i*4+3]);
				}*/
       
			if(decoder_format!=STREAM_ID_H264)//not h.264
			 iquantblk(inbuf+384, inbuf+384, qp_ac_y2, qp_dc_y2, decoder_format, slice_info, MB_info,0,1,0);//iquant for hadamard
			

       
			{
         
				/*fprintf(fout1,"24 **********h dct in*******************\n");
				for (i = 0; i < 4; i++)
				{
					fprintf(fout1,"%d %d %d %d\n",inbuf[384+i*4+0],inbuf[384+i*4+1],inbuf[384+i*4+2],inbuf[384+i*4+3]);
				}*/

				
				itrans4x4blk(inbuf+384, inbuf+384, 8, decoder_format, 1);
                
				if(decoder_format==STREAM_ID_H264)
				{

					for (i = 0; i < 16; i++)
					{
						j=(i/4)+(i%4)*4;               //transpose
						k=(j/8)*8+((j%8)/4)*2+((j%4)/2)*4+(j%2); //reoder blkindex
						inbuf[k*16] =inbuf[384+i];			
						
					}//weihu
				}
				else
				{
				
					for (i = 0; i < 16; i++)
					{
						inbuf[i*16] =inbuf[384+i];			
						
					}//weihu
				}

			
			

				/*fprintf(fout1,"24 ----------h dct out-------------\n");
				for (i = 0; i < 4; i++)
				{
					fprintf(fout1,"%d %d %d %d\n",inbuf[384+i*4+0],inbuf[384+i*4+1],inbuf[384+i*4+2],inbuf[384+i*4+3]);
				}*/
			}

   
		}//need_y_hadama

		for (i = 0; i < 24; i+=b4x4inblk)//for (i = 0; i < 16; i++)
		{
			
				
			if(i<16)//luma
			{
				if((decoder_format==STREAM_ID_REAL8) ||(decoder_format==STREAM_ID_REAL9)||(decoder_format==STREAM_ID_WEBP) ||(decoder_format==STREAM_ID_VP8))//real8/9/10 webp/vp8
				{
            		qp_dc=qp_dc_y;
					qp_ac=qp_ac_y; 
					isyhadamard=0;
					b4x4inblk=1;
				}
				else if(decoder_format==STREAM_ID_H264)//h.264
				{
					qp_dc=qp_y_per;
					qp_ac=qp_y_rem;
					isyhadamard=need_y_hadama;
					if(MB_info&0x1)//8x8trans
						b4x4inblk=4;
					else
						b4x4inblk=1;
					intra_flag=(MB_info&0x2)>>1;

				}
				else if(decoder_format==STREAM_ID_VC1)//vc1
				{
                    isyhadamard=0;
					b4x4inblk=4;
					intra_flag=(MB_info&0x3f)&(1<<(i/4));
					inter=!intra_flag;
					if(intra_flag)
					     qp_dc=qp_dc_y;
					else
						 qp_dc=qp_ac_y;
					qp_ac=qp_ac_y; 
				}
				ycbcr=0;
			}
			else//chroma
			{
				
				if((decoder_format==STREAM_ID_REAL8) ||(decoder_format==STREAM_ID_REAL9)||(decoder_format==STREAM_ID_WEBP) ||(decoder_format==STREAM_ID_VP8))//real8/9/10 webp/vp8
				{
					qp_ac=qp_ac_uv; 
					qp_dc=qp_dc_uv;
					ycbcr=1;
					b4x4inblk=1;
				}
				else if(decoder_format==STREAM_ID_H264)
				{
					b4x4inblk=1;
					if(i<20)
					{				
					   qp_dc=qp_u_per;
					   qp_ac=qp_u_rem;
					   ycbcr=1;
					}
					else
					{				
						qp_dc=qp_v_per;
						qp_ac=qp_v_rem;
						ycbcr=2;
					}
				}
				else if(decoder_format==STREAM_ID_VC1)//vc1
				{
					ycbcr=1;
                    isyhadamard=0;
					b4x4inblk=4;
					intra_flag=(MB_info&0x3f)&(1<<(i/4));
					inter=!intra_flag;
					if(intra_flag)
						qp_dc=qp_dc_y;
					else
						qp_dc=qp_ac_y;
					qp_ac=qp_ac_y; 
				}
				isyhadamard=0;
			}

            if(((cbp26>>i)&0x1)||(need_y_hadama&&(i<16))||(i>=16)&&((cbp26>>24)&0x1)&&(decoder_format==STREAM_ID_H264));
			{
			
				//dequant
						/*fprintf(fout1,"%d --------- quant in--------------%d %d\n",i, qp_ac, qp_dc);
						for (j = 0; j < 4; j++)
						{
						   fprintf(fout1,"%d %d %d %d\n",inbuf[16*i+j*4+0],inbuf[16*i+j*4+1],inbuf[16*i+j*4+2],inbuf[16*i+j*4+3]);
						}*/
				iquantblk(inbuf+16*i, inbuf+16*i, qp_ac, qp_dc, decoder_format,slice_info, MB_info,ycbcr,isyhadamard, inter);//iquant for DCT
						/*fprintf(fout1,"%d --------- quant out--------------%d\n",i,isyhadamard*10);
						for (j = 0; j < 4; j++)
						{
						   fprintf(fout1,"%d %d %d %d\n",inbuf[16*i+j*4+0],inbuf[16*i+j*4+1],inbuf[16*i+j*4+2],inbuf[16*i+j*4+3]);
						}*/


				if(b4x4inblk==4)//8x8
				{
					if(decoder_format==STREAM_ID_H264)//h.264
					    tran_size=0;
					else //if(decoder_format==3)
						tran_size=(((MB_info>>6)&0xfff)>>(i/2))&0x3; //(transize>>2)&0x3
					itrans8x8blk(inbuf+16*i, outbuf+16*i, 16,decoder_format,tran_size);//0:8x8 1:8*4 2:4x8 3:4x4
				}
				else//4x4
				{
					
        
					/*if (i<16) 
							diff_offset=4*((i/4)*16+i%4);
					else
							diff_offset=4*(64+((i-16)/2)*8+(i-16)%2);
					itrans4x4blk(outbuf+diff_offset, inbuf+16*i, (32 - (i & 16)),decoder_format,0);*/


					itrans4x4blk(inbuf+16*i, outbuf+16*i, 8, decoder_format,0);

						/*fprintf(fout1,"%d --------- dct out--------------\n",i);
						for (j = 0; j < 4; j++)
						{
						   fprintf(fout1,"%d %d %d %d\n",outbuf[16*i+j*4+0],outbuf[16*i+j*4+1],outbuf[16*i+j*4+2],outbuf[16*i+j*4+3]);
						}*/
				}//4x4
			}//((cbp26>>i)&&isyhadamard)
		
		}
	
	//fclose(fout1);
 }//if(!skip_flag)


}//weihu



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
					
				   )
{
   
	int i,j,k,b4x4inblk;
   
	//slice para in
	char seq_scaling_matrix_present_flag;//1b
	char pic_scaling_matrix_present_flag;//1b

    short i1=0; //12b//qp_y_per for H.264
	short i2=0; //12b//qp_y_rem for H.264
	short i3=0; //12b//qp_v for H.264
	short i4=0; //12b
	short i5=0; //12b//qp_u for H.264
	short i6=0; //12b
	int  i7=0;  //26b

	char skip_flag; //1b
	//char intra_flag; //1b/4b?
	char need_y_hadama;//1b
	
	
	char slice_info;
	char MB_info=0;
	
	short qp_dc,qp_ac;

	//int diff_offset;
	int buf1[8];//chroma DC short to int
	short qp_ac_y; 
	short qp_dc_y;
	short qp_ac_y2; 
	short qp_dc_y2; 
	short qp_ac_uv; 
	short qp_dc_uv; 
	int  cbp26;
    char qp_y_per;
    char qp_y_rem;
	char qp_u_per;
    char qp_u_rem;
	char qp_v_per;
    char qp_v_rem;
	char tran_size;//4X4
	char scale_en;//4X4
	char ycbcr;
	char intra_flag;
	char inter;
	char isyhadamard;
	char is_ipcm;

	switch(decoder_format) {
    case STREAM_ID_JPEG://Jpeg
	case STREAM_ID_MPEG4://mpeg4/h.263
    case STREAM_ID_H263://mpeg4/h.263
    case STREAM_ID_FLVH263://mpeg4/h.263
		break;
    case STREAM_ID_WEBP://webp
	case STREAM_ID_REAL8:     //real8
    case STREAM_ID_REAL9:     //real9/10
    case STREAM_ID_VP8://vp8
		{
			qp_ac_y=i1; //12b
			qp_dc_y=i2; //12b
			qp_ac_y2=i3; //12b
			qp_dc_y2=i4; //12b
			qp_ac_uv=i5; //12b
			qp_dc_uv=i6; //12b
			cbp26=i7;//26b
		    inter=(MB_info&0x2)>>1;
		
	
		}
		break;
	case STREAM_ID_VC1:     //VC1
		{
			qp_ac_y=i1; //12b
			qp_dc_y=i2; //12b
			//qp_ac_uv=i1; 
			//qp_dc_uv=i2; 
			//intra_flag=MB_info&x3f;//6b
			//trans_type=(MB_info>>6)&0xfff;//12b
			cbp26=i7&0x111111;//26b
            //NUniform=slice_info&0x1;//1b
			
			
		}
		break;
   	default:    //STREAM_ID_H264:h.264
		{
			need_y_hadama=(MB_info_buf[0]>>14)&0x1;
			//tran_size=(MB_info_buf[0]>>15)&0x1;
			intra_flag=(MB_info_buf[0]>>16)&0x1; //1b
			skip_flag=(MB_info_buf[0]>>17)&0x1; //1b
			is_ipcm=(MB_info_buf[0]>>18)&0x1; //1b
			qp_y_per=(MB_info_buf[1]>>3)&0xf;//4b
			qp_y_rem=MB_info_buf[1]&0x7;//3b
			qp_u_per=(MB_info_buf[1]>>10)&0xf;//4b
			qp_u_rem=(MB_info_buf[1]>>7)&0x7;//3b
			qp_v_per=(MB_info_buf[1]>>17)&0xf;//4b
			qp_v_rem=(MB_info_buf[1]>>14)&0x7;//3b
			cbp26=MB_info_buf[2]&0x3ffffff;//26b
			
			inter=!intra_flag;//
			MB_info=(inter<<1)|((MB_info_buf[0]>>15)&0x1);//inter|tran_size

			seq_scaling_matrix_present_flag=slice_info_list[0];//1b
			pic_scaling_matrix_present_flag=slice_info_list[1];//1b
            scale_en=seq_scaling_matrix_present_flag||pic_scaling_matrix_present_flag;//4X4
            slice_info=scale_en;

			//tran_size=MB_info&0x1;//4X4
			//inter=(MB_info&0x2)>>1;
			//scale_en=slice_info&0x1;//4X4
			
		}
	}
    
	

 //   fout1=fopen("E:/project/sc8810/Firmware_FPGA_verification/h264_decoder/simulation/vc/WinPrj/quant_in.txt","ab+");


 if(!skip_flag)    
 {
	     
		 if ((decoder_format==STREAM_ID_H264)&& (cbp26 & (1<<24)))//h.264 chroma dc
		 {
			   /* fprintf(fout1,"25 **********chroma DC in*******************\n");
				for (i = 0; i < 2; i++)
				{
					fprintf(fout1,"%d %d %d %d\n",inbuf[400+i*4+0],inbuf[400+i*4+1],inbuf[400+i*4+2],inbuf[400+i*4+3]);
				}*/
				for (i = 0; i < 8; i++)
				{
					buf1[i] =inbuf[400+i];			
					
				}//weihu
				itrans4x1(buf1, buf1, decoder_format, 1, 0);
				itrans4x1(buf1+4, buf1+4, decoder_format, 1, 0);
				for (i = 0; i < 8; i++)
				{
					inbuf[256+i*16] =buf1[i];			
					
				}//weihu
				/*fprintf(fout1,"25 **********chroma DC out*******************\n");
				for (i = 0; i < 2; i++)
				{
					fprintf(fout1,"%d %d %d %d\n",buf1[i*4+0],buf1[i*4+1],buf1[i*4+2],buf1[i*4+3]);
				}*/
				
		 }
		 
		 if (need_y_hadama)
		{

				/*fprintf(fout1,"24 **********h quant in*******************%d %d\n",qp_ac_y2, qp_dc_y2);
				for (i = 0; i < 4; i++)
				{
					fprintf(fout1,"%d %d %d %d\n",inbuf[384+i*4+0],inbuf[384+i*4+1],inbuf[384+i*4+2],inbuf[384+i*4+3]);
				}*/
       
			if(decoder_format!=STREAM_ID_H264)//not h.264
			 iquantblk(inbuf+384, inbuf+384, qp_ac_y2, qp_dc_y2, decoder_format, slice_info, MB_info,0,1,0);//iquant for hadamard
			

       
			{
         
				/*fprintf(fout1,"24 **********h dct in*******************\n");
				for (i = 0; i < 4; i++)
				{
					fprintf(fout1,"%d %d %d %d\n",inbuf[384+i*4+0],inbuf[384+i*4+1],inbuf[384+i*4+2],inbuf[384+i*4+3]);
				}*/

				
				itrans4x4blk(inbuf+384, inbuf+384, 8, decoder_format, 1);
                
				if(decoder_format==STREAM_ID_H264)
				{

					for (i = 0; i < 16; i++)
					{
						j=(i/4)+(i%4)*4;               //transpose
						k=(j/8)*8+((j%8)/4)*2+((j%4)/2)*4+(j%2); //reoder blkindex
						inbuf[k*16] =inbuf[384+i];			
						
					}//weihu
				}
				else
				{
				
					for (i = 0; i < 16; i++)
					{
						inbuf[i*16] =inbuf[384+i];			
						
					}//weihu
				}

			
			

				/*fprintf(fout1,"24 ----------h dct out-------------\n");
				for (i = 0; i < 4; i++)
				{
					fprintf(fout1,"%d %d %d %d\n",inbuf[384+i*4+0],inbuf[384+i*4+1],inbuf[384+i*4+2],inbuf[384+i*4+3]);
				}*/
			}

   
		}//need_y_hadama

		for (i = 0; i < 24; i+=b4x4inblk)//for (i = 0; i < 16; i++)
		{
			
				
			if(i<16)//luma
			{
				if((decoder_format==STREAM_ID_REAL8) ||(decoder_format==STREAM_ID_REAL9)||(decoder_format==STREAM_ID_WEBP) ||(decoder_format==STREAM_ID_VP8))//real8/9/10 webp/vp8
				{
            		qp_dc=qp_dc_y;
					qp_ac=qp_ac_y; 
					isyhadamard=0;
					b4x4inblk=1;
				}
				else if(decoder_format==STREAM_ID_H264)//h.264
				{
					qp_dc=qp_y_per;
					qp_ac=qp_y_rem;
					isyhadamard=need_y_hadama;
					if(MB_info&0x1)//8x8trans
						b4x4inblk=4;
					else
						b4x4inblk=1;
					intra_flag=(MB_info&0x2)>>1;

				}
				else if(decoder_format==STREAM_ID_VC1)//vc1
				{
                    isyhadamard=0;
					b4x4inblk=4;
					intra_flag=(MB_info&0x3f)&(1<<(i/4));
					inter=!intra_flag;
					if(intra_flag)
					     qp_dc=qp_dc_y;
					else
						 qp_dc=qp_ac_y;
					qp_ac=qp_ac_y; 
				}
				ycbcr=0;
			}
			else//chroma
			{
				
				if((decoder_format==STREAM_ID_REAL8) ||(decoder_format==STREAM_ID_REAL9)||(decoder_format==STREAM_ID_WEBP) ||(decoder_format==STREAM_ID_VP8))//real8/9/10 webp/vp8
				{
					qp_ac=qp_ac_uv; 
					qp_dc=qp_dc_uv;
					ycbcr=1;
					b4x4inblk=1;
				}
				else if(decoder_format==STREAM_ID_H264)
				{
					b4x4inblk=1;
					if(i<20)
					{				
					   qp_dc=qp_u_per;
					   qp_ac=qp_u_rem;
					   ycbcr=1;
					}
					else
					{				
						qp_dc=qp_v_per;
						qp_ac=qp_v_rem;
						ycbcr=2;
					}
				}
				else if(decoder_format==STREAM_ID_VC1)//vc1
				{
					ycbcr=1;
                    isyhadamard=0;
					b4x4inblk=4;
					intra_flag=(MB_info&0x3f)&(1<<(i/4));
					inter=!intra_flag;
					if(intra_flag)
						qp_dc=qp_dc_y;
					else
						qp_dc=qp_ac_y;
					qp_ac=qp_ac_y; 
				}
				isyhadamard=0;
			}

            if(((cbp26>>i)&0x1)||(need_y_hadama&&(i<16))||(i>=16)&&((cbp26>>24)&0x1)&&(decoder_format==STREAM_ID_H264));
			{
			
				//dequant
						/*fprintf(fout1,"%d --------- quant in--------------%d %d\n",i, qp_ac, qp_dc);
						for (j = 0; j < 4; j++)
						{
						   fprintf(fout1,"%d %d %d %d\n",inbuf[16*i+j*4+0],inbuf[16*i+j*4+1],inbuf[16*i+j*4+2],inbuf[16*i+j*4+3]);
						}*/
				iquantblk(inbuf+16*i, inbuf+16*i, qp_ac, qp_dc, decoder_format,slice_info, MB_info,ycbcr,isyhadamard, inter);//iquant for DCT
						/*fprintf(fout1,"%d --------- quant out--------------%d\n",i,isyhadamard*10);
						for (j = 0; j < 4; j++)
						{
						   fprintf(fout1,"%d %d %d %d\n",inbuf[16*i+j*4+0],inbuf[16*i+j*4+1],inbuf[16*i+j*4+2],inbuf[16*i+j*4+3]);
						}*/


				if(b4x4inblk==4)//8x8
				{
					if(decoder_format==STREAM_ID_H264)//h.264
					    tran_size=0;
					else //if(decoder_format==3)
						tran_size=(((MB_info>>6)&0xfff)>>(i/2))&0x3; //(transize>>2)&0x3
					itrans8x8blk(inbuf+16*i, outbuf+16*i, 16,decoder_format,tran_size);//0:8x8 1:8*4 2:4x8 3:4x4
				}
				else//4x4
				{
					
        
					/*if (i<16) 
							diff_offset=4*((i/4)*16+i%4);
					else
							diff_offset=4*(64+((i-16)/2)*8+(i-16)%2);
					itrans4x4blk(outbuf+diff_offset, inbuf+16*i, (32 - (i & 16)),decoder_format,0);*/


					itrans4x4blk(inbuf+16*i, outbuf+16*i, 8, decoder_format,0);

						/*fprintf(fout1,"%d --------- dct out--------------\n",i);
						for (j = 0; j < 4; j++)
						{
						   fprintf(fout1,"%d %d %d %d\n",outbuf[16*i+j*4+0],outbuf[16*i+j*4+1],outbuf[16*i+j*4+2],outbuf[16*i+j*4+3]);
						}*/
				}//4x4
			}//((cbp26>>i)&&isyhadamard)
		
		}
	
	//fclose(fout1);
 }//if(!skip_flag)
 else
 {
	 for (i = 0; i < 24; i++)
         for (j = 0; j < 16; j++)
			 if(is_ipcm)
				   outbuf[i*16+j]= inbuf[i*16+j];//pcm Ë³Ðò£¿
			 else
		           outbuf[i*16+j] =0;		 
				
 }


}//weihu


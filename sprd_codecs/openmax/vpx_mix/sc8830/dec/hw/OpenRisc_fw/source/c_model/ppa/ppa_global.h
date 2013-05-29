
#ifndef _PPA_GLOBAL_H_
#define _PPA_GLOBAL_H_

//extern int32 dc_value ;

//void Iict(int32 is_DC_itrans, int32 blk4x4Idx, int32 is_luma, int32 second_phase, int32 is_h264, int32 cbp26,  int32 need_y_hadama); 
#define IClip(Min, Max, Val) (((Val)<(Min))? (Min):(((Val)>(Max))? (Max):(Val)))

#define CONTEXT_CACHE_WIDTH 6

#define	skip_direct_h264	0
#define	PB16x16_h264	1
#define	PB16x8_h264		2
#define PB8x16_h264		3
#define	PB8x8_h264		4
#define INxN_h264	5
#define	I16_h264		6
#define	IPCM_h264		7



#define SUB_8X8	0
#define SUB_8X4	1
#define	SUB_4X8	2
#define	SUB_4X4	3
#define	direct_8X8	4

#define I_16X16	0
#define I_4X4	1
#define	I_8X8	2

#define P_slice	0
#define B_slice 1
#define	I_slice	2


typedef struct left_mb_info
{	
	union
	{
			struct 
			{
				uint slice_nr :9;				
				uint is_intra :1;
				uint is_skip :1;
				uint transform_size_8x8_flag :1;
				uint cbp_blk :4;
				uint qp :6;
				uint mb_mode :3;
				uint submb_0_mode :3;
				uint submb_1_mode :3;

			}h264;//_reg0;//31b
    }reg0;
	union
	{
		struct 
		{
			int intra4x4_5_mode :5;//4;//int intra8x8_1_mode :4;
			int intra4x4_7_mode :5;//4;//int intra8x8_1_mode :4;
			int intra4x4_13_mode :5;//4;//int intra8x8_1_mode :4;
			int intra4x4_14_mode :5;//4;//int intra8x8_3_mode :4;
			int intra4x4_15_mode :5;//4;//int intra8x8_3_mode :4;
			//int intra8x8_1_mode :4;
			//int intra8x8_3_mode :4;
			//int intra16x16_mode :2;
		}h264_intra;//_reg1;//25b

		struct 
		{
			int ref_idx_1_l0 :5;			
			int ref_idx_1_l1 :5;
			int tmp :5;
			int ref_idx_3_l0 :5;
			int ref_idx_3_l1 :5;	
		}h264_inter;//_reg1;//25b

    }reg1;
	union
	{
		struct 
		{
		
			int mv_l0_5_y :12;
			int mv_l0_5_x :14;
			uint addr_idx_1_l0 :5;
		}h264;//_reg2;//26b
		
    }reg2;
	union
	{
		struct 
		{
			
			int mv_l0_7_y :12;
			int mv_l0_7_x :14;
			
		}h264;//_reg3;//26b
		
    }reg3;
	union
	{
		struct 
		{
			
			int mv_l0_13_y :12;
			int mv_l0_13_x :14;
			uint addr_idx_3_l0 :5;
		}h264;//_reg4;//26b
		
    }reg4;
	union
	{
		struct 
		{
			
			int mv_l0_15_y :12;
			int mv_l0_15_x :14;
		}h264;//_reg5;//26b
		
    }reg5;
	union
	{
		struct 
		{
			int mv_l1_5_y :12;
			int mv_l1_5_x :14;
			
			uint addr_idx_1_l1 :5;
		}h264;//_reg6;//26b
		
    }reg6;
	union
	{
		struct 
		{
			int mv_l1_7_y :12;
			int mv_l1_7_x :14;
			
		}h264;//_reg7;//26b
		
    }reg7;
	union
	{
		struct 
		{
			int mv_l1_13_y :12;
			int mv_l1_13_x :14;
			
			uint addr_idx_3_l1 :5;
		}h264;//_reg8;//26b
		
    }reg8;
	union
	{
		struct 
		{
			int mv_l1_15_y :12;
			int mv_l1_15_x :14;
			
		}h264;//_reg9;//26b
		
    }reg9;

	
}LEFT_MB;//struct left_mb_info

typedef struct PPA_line_unit
{	
	union
	{
			struct 
			{
				uint slice_nr :9;				
				uint is_intra :1;
				uint is_skip :1;
				uint transform_size_8x8_flag :1;
				uint cbp_blk :4;
				uint qp :6;
				uint mb_mode :3;
				uint submb_0_mode :3;
				uint submb_1_mode :3;

			}h264;//_reg0;//31b
    }reg0;
	union
	{
		struct 
		{
			int intra4x4_10_mode :5;//4;//int intra8x8_2_mode :4;
			int intra4x4_11_mode :5;//4//int intra8x8_2_mode :4;
			int intra4x4_14_mode :5;//4;//int intra8x8_3_mode :4;
			int intra4x4_15_mode :5;//4;//int intra8x8_3_mode :4;
			//int intra8x8_2_mode :4;
			//int intra8x8_3_mode :4;
			//int intra16x16_mode :2;
		}h264_intra;//_reg1;//20b

		struct 
		{
			int ref_idx_2_l0 :5;			
			int ref_idx_2_l1 :5;
			int ref_idx_3_l0 :5;
			int ref_idx_3_l1 :5;	
		}h264_inter;//_reg1;//20b

    }reg1;
	union
	{
		struct 
		{
			int mv_l0_10_y :12;
			int mv_l0_10_x :14;
			
			uint addr_idx_2_l0 :5;
		}h264;//_reg2;//26b
		
    }reg2;
	union
	{
		struct 
		{
			
			int mv_l0_11_y :12;
			int mv_l0_11_x :14;
		}h264;//_reg3;//26b
		
    }reg3;
	union
	{
		struct 
		{
			
			int mv_l0_14_y :12;
			int mv_l0_14_x :14;
			uint addr_idx_3_l0 :5;
		}h264;//_reg4;//26b
		
    }reg4;
	union
	{
		struct 
		{
			
			int mv_l0_15_y :12;
			int mv_l0_15_x :14;
		}h264;//_reg5;//26b
		
    }reg5;
	union
	{
		struct 
		{
			
			int mv_l1_10_y :12;
			int mv_l1_10_x :14;
			uint addr_idx_2_l1 :5;
		}h264;//_reg6;//26b
		
    }reg6;
	union
	{
		struct 
		{
			
			int mv_l1_11_y :12;
			int mv_l1_11_x :14;
		}h264;//_reg7;//26b
		
    }reg7;
	union
	{
		struct 
		{
			
			int mv_l1_14_y :12;
			int mv_l1_14_x :14;
			uint addr_idx_3_l1 :5;
		}h264;//_reg8;//26b
		
    }reg8;
	union
	{
		struct 
		{
			
			int mv_l1_15_y :12;
			int mv_l1_15_x :14;
		}h264;//_reg9;//26b
		
    }reg9;

	
}PPA_LINE_BUF;//struct PPA_line_unit

typedef struct DIRECT_MV_REF_STORAGE
{	
	union{
		struct{
			uint blk0_ref_poc :32;
		}h264;//_reg0;//32b		
    }reg0;
	union{
		struct{
			uint blk1_ref_poc :32;
		}h264;//_reg1;//32b		
    }reg1;
	union{
		struct{
			uint blk2_ref_poc :32;
		}h264;//_reg2;//32b		
    }reg2;
	union{
		struct{
			uint blk3_ref_poc :32;
		}h264;//_reg3;//32b		
    }reg3;
	union{
		struct{
			int mv_0_y :12;
			int mv_0_x :14;
			
			int blk0_ref_idx :5;
		}h264;//_reg4;//31b		
    }reg4;
	union{
		struct{
			int mv_5_y :12;
			int mv_5_x :14;
			
			int blk1_ref_idx :5;
		}h264;//_reg4;//31b		
    }reg5;
	union{
		struct{
			int mv_10_y :12;
			int mv_10_x :14;
			
			int blk2_ref_idx :5;
		}h264;//_reg4;//31b		
    }reg6;
	union{
		struct{
			
			int mv_15_y :12;
			int mv_15_x :14;
			int blk3_ref_idx :5;
		}h264;//_reg4;//31b		
    }reg7;
	union{
		struct{
			
			int mv_4_y :12;
			int mv_4_x :14;
		}h264;//_reg4;//31b		
    }reg8;
	union{
		struct{
			
			int mv_1_y :12;
			int mv_1_x :14;
		}h264;//_reg4;//31b		
    }reg9;
	union{
		struct{
			
			int mv_6_y :12;
			int mv_6_x :14;
		}h264;//_reg4;//31b		
    }reg10;
	union{
		struct{
			
			int mv_7_y :12;
			int mv_7_x :14;
		}h264;//_reg4;//31b		
    }reg11;
	union{
		struct{
			
			int mv_8_y :12;
			int mv_8_x :14;
		}h264;//_reg4;//31b		
    }reg12;
	union{
		struct{
			
			int mv_9_y :12;
			int mv_9_x :14;
		}h264;//_reg4;//31b		
    }reg13;
	union{
		struct{
			
			int mv_2_y :12;
			int mv_2_x :14;
		}h264;//_reg4;//31b		
    }reg14;
    union{
		struct{
			
			int mv_11_y :12;
			int mv_11_x :14;
		}h264;//_reg4;//31b		
    }reg15;
	union{
		struct{
			
			int mv_12_y :12;
			int mv_12_x :14;
		}h264;//_reg4;//31b		
    }reg16;
	union{
		struct{
			
			int mv_13_y :12;
			int mv_13_x :14;
		}h264;//_reg4;//31b		
    }reg17;
	union{
		struct{
			
			int mv_14_y :12;
			int mv_14_x :14;
		}h264;//_reg4;//31b		
    }reg18;
    union{
		struct{
			
			int mv_3_y :12;
			int mv_3_x :14;
		}h264;//_reg4;//31b		
    }reg19;
	

}DIRECT_MV_REF_BUF;

void ppa_module (
				 int dct_buf[10], //for dct para
				 int mbc_buf[4], //for mbc para
				 int dbk_buf[8], //for dct para
				 int mca_buf[38], //for dct para
				 int inbuf[36], //parser output 36x32bxn
                 //int *col_mb_buf,//col_located MB para
                 int ref_list_buf[25],//refpic list buf
				 char decoder_format,//3b
				 char picwidthinMB,//7b
				 char picheightinMB,//7b
				 int slice_info[40],
				 int error_flag				 
				 );

extern int8  g_list0_map_addr[16];//weihu
#endif //#ifndef _PPA_GLOBAL_H_























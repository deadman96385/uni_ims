/*rvld_init.c*/
#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include "rvld_mode.h"
#include "rvld_global.h"

#if defined (REAL_DEC)
void RvldInit ()
{
	g_rvld_trace_fp = fopen ("..\\seq\\vld_trace.txt", "w");

	TestVecInit ();
		
	g_rvld_mode_ptr = (RVLD_MODE_T *)malloc(sizeof(RVLD_MODE_T));	
	

 // 	InitHuffTab ();
	if(0)
	{
		FILE *rvld_intra_code_fp = fopen("..\\..\\test_vectors\\sw_intra_code.dat", "wb");
		int32 i,j;

		for (i = 0; i < MAX_INTRA_QP_REGIONS; i++)
		{
			for (j = 0; j < 4*1024; j++)
			{
				fprintf(rvld_intra_code_fp, "%08x\n", g_rvld_intra_code[i][j]);
			}
		}

		for (i = 0; i < MAX_INTER_QP_REGIONS; i++)
		{
			for (j = 0; j < 4*1024; j++)
			{
				fprintf(rvld_intra_code_fp, "%08x\n", g_rvld_inter_code[i][j]);
			}
		}
		fclose(rvld_intra_code_fp);
	}
	
}

void SetupHuffTab (
				   int			dsc_type,
				   int			ele_num, 
				   int			start_pos,
				   uint32	  *	packed_len_tbl,
				   HUFF_DEC_S * huff_dec_ptr
				   )
{
	int			i;
//	int			max_len;
	uint8		len_tbl[MAX_CBP];
//	uint32		cnt_len[MAX_DEPTH+1];
	uint32		start[MAX_DEPTH+2];
	uint32		offset_tmp [MAX_DEPTH+1];
	int	   *	max_cnt_ptr;
	
	memset (len_tbl, 0, sizeof(uint8)*MAX_CBP);
	memset (huff_dec_ptr->cnt_len, 0, sizeof(uint32)*(MAX_DEPTH+1));
	memset (start, 0, sizeof(uint32)*(MAX_DEPTH+2));
	memset (offset_tmp, 0, sizeof(uint32)*(MAX_DEPTH+1));

	memset (huff_dec_ptr->code_tab_ptr, 0, sizeof(uint32)*ele_num);

	for (i = 0; i < ele_num; i += 8)
	{
		len_tbl [i+0] = (uint8)((packed_len_tbl [i>>3] >> 28) & 0xf) + 1;
		len_tbl [i+1] = (uint8)((packed_len_tbl [i>>3] >> 24) & 0xf) + 1;
		len_tbl [i+2] = (uint8)((packed_len_tbl [i>>3] >> 20) & 0xf) + 1;
		len_tbl [i+3] = (uint8)((packed_len_tbl [i>>3] >> 16) & 0xf) + 1;	
		
		len_tbl [i+4] = (uint8)((packed_len_tbl [i>>3] >> 12) & 0xf) + 1;
		len_tbl [i+5] = (uint8)((packed_len_tbl [i>>3] >>  8) & 0xf) + 1;
		len_tbl [i+6] = (uint8)((packed_len_tbl [i>>3] >>  4) & 0xf) + 1;
		len_tbl [i+7] = (uint8)((packed_len_tbl [i>>3] >>  0) & 0xf) + 1;	
	}

	if (start_pos == 1)
	{
		len_tbl [0] = 0;
	}


	/*count number for each code length*/
	for (i = 0; i < ele_num; i++)
	{
		huff_dec_ptr->cnt_len[len_tbl[i]]++;
	}

	if (dsc_type == DSC_LEV)
	{
		for (i = 1; i <= 16; i++)
		{
			//printf ("len: %d coeff_num: %d\n", i, cnt_len[i]);
			if (huff_dec_ptr->cnt_len[i] > g_max_num_len[i])
				g_max_num_len[i] = huff_dec_ptr->cnt_len[i];
		}

// 		printf ("\n");
	}

	huff_dec_ptr->cnt_len[0] = 0;

	start [1] = 0;
	for (i = 1; i <= MAX_DEPTH; i++)
	{
		start [i+1] = (start [i] + huff_dec_ptr->cnt_len [i]) << 1;
	}
	
	/*generate min_reg*/
	for (i = 1; i <= MAX_DEPTH; i++)
	{
		if (huff_dec_ptr->cnt_len[i] != 0)
		{
			huff_dec_ptr->min_reg[i] = (1 << 31) | start[i];
		}
		else
		{
			huff_dec_ptr->min_reg[i] = 0;
		}
	}
	
	/*generate max_reg*/
	for (i = 1; i < 16; i++)
	{
		int last_effect;

		if (huff_dec_ptr->cnt_len[i] == 0)
		{
			if (i == 1)
				huff_dec_ptr->max_reg[i] = 0;
			else
			{
				last_effect = (huff_dec_ptr->cnt_len[i-1] != 0) ? 1 : 0;

				huff_dec_ptr->max_reg[i] = (0 << 31) | ((huff_dec_ptr->max_reg[i-1]+last_effect)  << 1);
			}
		}
		else
		{
			huff_dec_ptr->max_reg[i] = huff_dec_ptr->min_reg[i] + huff_dec_ptr->cnt_len[i] - 1;
		}
	}


	max_cnt_ptr = (dsc_type == DSC_8X8) ? g_max_cnt_len_dsc8x8 : g_max_cnt_len_dsclev;
	if ((dsc_type == DSC_8X8) || (dsc_type == DSC_LEV))
	{
		for (i = 0; i <=16; i++)
		{
			huff_dec_ptr->cnt_len[i] = max_cnt_ptr[i];
		}
	}

	/*count linear offset of base codeword*/
	huff_dec_ptr->base_addr [0] = 0;
	huff_dec_ptr->base_addr [1] = 0;
	for (i = 1; i < MAX_DEPTH; i++)
	{
		huff_dec_ptr->base_addr [i+1] = huff_dec_ptr->base_addr [i] + huff_dec_ptr->cnt_len [i]; 
	}

	/*generate symbol map*/
	for (i = 0; i <= MAX_DEPTH; i++)
	{
		offset_tmp [i] = huff_dec_ptr->base_addr [i];
	}

	for (i = 0; i < ele_num; i++)
	{
		huff_dec_ptr->code_tab_ptr[ offset_tmp[len_tbl[i]]++ ] = i;
	}
}

void SetupVldContext (
					  int			dsc_type, 
					  int			ele_num, 
					  HUFF_DEC_S *	huff_dec_ptr,
					  int			qp,
					  int			is_intra
					  )
{
	int			i;
	int			byte_num;
	int			word_num;
	uint32		val;
	uint32	*	code_ptr;	
	uint32	*	max_base_ptr;
	int			dsc_start;
	int			cnt0;
	int			cnt1;
	
	dsc_start = (dsc_type == DSC_4X4) ? 0 : g_code_pos;			//word offset

	if (is_intra)
	{
		code_ptr		= g_rvld_intra_code[qp] + g_code_pos;
		max_base_ptr	= g_rvld_intra_max_base[qp] + g_max_base_pos;
	}
	else
	{
		code_ptr		= g_rvld_inter_code[qp] + g_code_pos;
		max_base_ptr	= g_rvld_inter_max_base[qp] + g_max_base_pos;
	}

	/*fill the code_tab*/
	for (i = 0; i < ele_num; i++)
	{
		if (ele_num < 256)
		{	
			((uint8 *)code_ptr)[i] = huff_dec_ptr->code_tab_ptr[i];
		}
		else
		{
			((uint16 *)code_ptr)[i] = huff_dec_ptr->code_tab_ptr[i];
		}
	}

	byte_num = (ele_num < 256) ? ele_num : ele_num*2;

	word_num = (byte_num + 3) / 4;

	g_code_pos += word_num;


	/*assemble max register*/
	/*************************************************************
	len (1, 2, 3, 4, 5, 6) is stored in one word, note: 
	length of max_reg_len1: 1
	length of max_reg_len2: 2
	length of max_reg_len3: 3
	length of max_reg_len4: 4
	length of max_reg_len5: 5
	length of max_reg_len6: 6
	************************************************************/
	val =	((((huff_dec_ptr->max_reg[1] >> 30) & 0x2)	| (huff_dec_ptr->max_reg[1] & 0x1)) << 0)		|		//2
			((((huff_dec_ptr->max_reg[2] >> 29) & 0x4)	| (huff_dec_ptr->max_reg[2] & 0x3)) << 2)		|		//3
			((((huff_dec_ptr->max_reg[3] >> 28) & 0x8)	| (huff_dec_ptr->max_reg[3] & 0x7)) << 5)		|		//4
			((((huff_dec_ptr->max_reg[4] >> 27) & 0x10) | (huff_dec_ptr->max_reg[4] & 0xf)) << 9)		|		//5
			((((huff_dec_ptr->max_reg[5] >> 26) & 0x20) | (huff_dec_ptr->max_reg[5] & 0x1f)) << 14)		|		//6	
			((((huff_dec_ptr->max_reg[6] >> 25) & 0x40) | (huff_dec_ptr->max_reg[6] & 0x3f)) << 20);			//7
	
	*max_base_ptr++ = val;	

	/*************************************************************
	len (7, 8, 9), note:
	length of max_reg_len7: 7
	length of max_reg_len8: 8
	length of max_reg_len9: 9
	*************************************************************/
	val =	((((huff_dec_ptr->max_reg[7] >> 24) & 0x80) | (huff_dec_ptr->max_reg[7] & 0x7f)) << 0)		|		//8
			((((huff_dec_ptr->max_reg[8] >> 23) & 0x100) | (huff_dec_ptr->max_reg[8] & 0xff)) << 8)		|		//9
			((((huff_dec_ptr->max_reg[9] >> 22) & 0x200) | (huff_dec_ptr->max_reg[9] & 0x1ff)) << 17);			//10
	
	*max_base_ptr++ = val;

	g_max_base_pos += 2;
	
	if (dsc_type != DSC_8X8)
	{
		/*len (10, 11), max length is 10, 11 respectively*/
		
		val =	((((huff_dec_ptr->max_reg[10] >> 21) & 0x400) | (huff_dec_ptr->max_reg[10] & 0x3ff))) << 0		|	//11
				((((huff_dec_ptr->max_reg[11] >> 20) & 0x800) | (huff_dec_ptr->max_reg[11] & 0x7ff)) << 11);		//12

		*max_base_ptr++ = val;

		/*len (12, 13), max length is 12, 13 respectively*/
		val =	((((huff_dec_ptr->max_reg[12] >> 19) & 0x1000) | (huff_dec_ptr->max_reg[12] & 0xfff)) << 0)	|		//13
				((((huff_dec_ptr->max_reg[13] >> 18) & 0x2000) | (huff_dec_ptr->max_reg[13] & 0x1fff)) << 13);		//14

		*max_base_ptr++ = val;

		/*len (14, 15), max length is 14, 15 respectively*/	
		val =	((((huff_dec_ptr->max_reg[14] >> 17) & 0x4000) | (huff_dec_ptr->max_reg[14] & 0x3fff)) << 0)	|	//15
				((((huff_dec_ptr->max_reg[15] >> 16) & 0x8000) | (huff_dec_ptr->max_reg[15] & 0x7fff)) << 15);		//16

		*max_base_ptr++ = val;

		g_max_base_pos += 3;
	}

	/*store base address for each length*/
	if ((dsc_type != DSC_8X8) && (dsc_type != DSC_LEV))
	{
		int	byte_ele;
		int	code_pos = dsc_start;
		
		byte_ele = (dsc_type == DSC_2X2) ? 1 : 2;

		if ((dsc_type != DSC_CBP) && (dsc_type != DSC_4X4))
		{
			code_pos = code_pos - (is_intra ? INTRA_DSC8X8_OFFSET : INTER_DSC8X8_OFFSET);

			code_pos = is_intra ? code_pos : (code_pos + INTER_CODE_BASE);
		}

		code_pos = code_pos * 4;		//convert to byte address

		for (i = 1; i <= 16; i+=2)
		{
			cnt0 = code_pos;	
			code_pos += huff_dec_ptr->cnt_len[i] * byte_ele;

			cnt1 = code_pos;
			code_pos += huff_dec_ptr->cnt_len[i+1] * byte_ele;

			*max_base_ptr++ = (cnt1 << 16) | (cnt0);
		}	
		
		g_max_base_pos += 8;
	}
}


void InitHuffTab ()
{
	int		qp;
	int		i;
	int		j;
	int		start_pos;

	/*set intra huffman table*/
	for (qp = 0; qp < MAX_INTRA_QP_REGIONS; qp++)
	{
		if (qp == 3)
			printf ("");

		/********************************************************************
		the cbp table is stored in external memory
		intra cbp table, two types of intra MB: intraMB16x16 and intraMB4x4
		*********************************************************************/	
		for (i = 0; i < 2; i++)
		{
			g_intra_cbp_dsc[qp][i].code_tab_ptr = (uint32 *)malloc (sizeof(uint32)*MAX_CBP);

			/*setup cbp_dsc huffman table*/
			start_pos = 0;
			SetupHuffTab (DSC_CBP, MAX_CBP, start_pos, intra_cbp_len[qp][i], &g_intra_cbp_dsc[qp][i]);

			SetupVldContext (DSC_CBP, MAX_CBP, &g_intra_cbp_dsc[qp][i], qp, 1);
		}

		/*********************************************************************
		cache is used for 6 dsc4x4 table
		setup l_4x4_dsc huffman table
		*********************************************************************/
		for (i = 0; i < 3; i++)
		{
			if (i == 2)
				printf ("");

			g_intra_l4x4_dsc[qp][i].code_tab_ptr = (uint32 *)malloc(sizeof(uint32) * MAX_4x4_DSC);

			start_pos = (i < 2) ? 1 : 0;
			SetupHuffTab (DSC_4X4, MAX_4x4_DSC, start_pos, intra_luma_4x4_dsc_len[qp][i], &g_intra_l4x4_dsc[qp][i]);
			
			SetupVldContext (DSC_4X4, MAX_4x4_DSC, &g_intra_l4x4_dsc[qp][i], qp, 1);
		}

		/*setup c_4x4_dsc huffman table*/
		g_intra_c4x4_dsc[qp].code_tab_ptr = (uint32 *)malloc(sizeof(uint32) * MAX_4x4_DSC);

		start_pos = 1;
		SetupHuffTab (DSC_4X4, MAX_4x4_DSC, start_pos, intra_chroma_4x4_dsc_len[qp], &g_intra_c4x4_dsc[qp]);

		SetupVldContext (DSC_4X4, MAX_4x4_DSC, &g_intra_c4x4_dsc[qp], qp, 1);

		/*********************************************************************
		dsc8x8 dsc2x2_luma, dsc2x2_chroma, dsc_lev is stored in on_chip memory
		setup 8x8_dsc huffman table
		*********************************************************************/
		for (i = 0; i < 2; i++)
		{
			for (j = 0; j < 4; j++)
			{
				g_intra_8x8_dsc[qp][i][j].code_tab_ptr = (uint32 *)malloc (sizeof(uint32) * MAX_DSC8X8_NUM);
				memset (g_intra_8x8_dsc[qp][i][j].code_tab_ptr, 0, sizeof(uint32)*MAX_DSC8X8_NUM);

				start_pos = 1;
				SetupHuffTab (DSC_8X8, MAX_8x8_DSC, start_pos, intra_8x8_dsc_len[qp][i][j], &g_intra_8x8_dsc[qp][i][j]);	
				
				SetupVldContext (DSC_8X8, MAX_DSC8X8_NUM, &g_intra_8x8_dsc[qp][i][j], qp, 1);
			}
		}

		/*setup l_2x2_dsc huffman table*/
		for (i = 0; i < 2; i++)
		{
			g_intra_l2x2_dsc[qp][i].code_tab_ptr = (uint32 *)malloc(sizeof(uint32) * MAX_2x2_DSC);

			start_pos = 1;
			SetupHuffTab (DSC_2X2, MAX_2x2_DSC, start_pos, intra_luma_2x2_dsc_len[qp][i], &g_intra_l2x2_dsc[qp][i]);
			
			SetupVldContext (DSC_2X2, MAX_2x2_DSC, &g_intra_l2x2_dsc[qp][i], qp, 1);
		}

		/*setup c_2x2_dsc huffman table*/
		for (i = 0; i < 2; i++)
		{
			g_intra_c2x2_dsc[qp][i].code_tab_ptr = (uint32 *)malloc(sizeof(uint32) * MAX_2x2_DSC);

			start_pos = 1;
			SetupHuffTab (DSC_2X2, MAX_2x2_DSC, start_pos, intra_chroma_2x2_dsc_len[qp][i], &g_intra_c2x2_dsc[qp][i]);
			
			SetupVldContext (DSC_2X2, MAX_2x2_DSC, &g_intra_c2x2_dsc[qp][i], qp, 1);
		}

		/*setup lev_dsc huffman table*/
		g_intra_lev_dsc[qp].code_tab_ptr = (uint32 *)malloc(sizeof(uint32) * MAX_DSCLEV_NUM);
		memset (g_intra_lev_dsc[qp].code_tab_ptr, 0, sizeof(uint32)*MAX_DSCLEV_NUM);

		start_pos = 0;
		SetupHuffTab (DSC_LEV, MAX_LEVEL_DSC, start_pos, intra_level_dsc_len[qp], &g_intra_lev_dsc[qp]);
		
		SetupVldContext (DSC_LEV, MAX_DSCLEV_NUM, &g_intra_lev_dsc[qp], qp, 1);

		g_code_pos = 0;
		g_max_base_pos = 0;
	}

	/*set inter huffman table*/
	for (qp = 0; qp < MAX_INTER_QP_REGIONS; qp++)
	{
		if (qp == 4)
			printf ("");

		/*inter cbp table*/			
		g_inter_cbp_dsc[qp].code_tab_ptr = (uint32 *)malloc (sizeof(uint32)*1296);
		
		/*setup cbp_dsc huffman table*/
		start_pos = 0;
		SetupHuffTab (DSC_CBP, MAX_CBP, start_pos, inter_cbp_len[qp], &g_inter_cbp_dsc[qp]);
		SetupVldContext (DSC_CBP, MAX_CBP, &g_inter_cbp_dsc[qp], qp, 0);	
		
		/*setup l_4x4_dsc huffman table*/
		g_inter_l4x4_dsc[qp].code_tab_ptr = (uint32 *)malloc(sizeof(uint32) * MAX_4x4_DSC);
			
		start_pos = 1;
		SetupHuffTab (DSC_4X4, MAX_4x4_DSC, start_pos, inter_luma_4x4_dsc_len[qp], &g_inter_l4x4_dsc[qp]);	
		
		SetupVldContext (DSC_4X4, MAX_4x4_DSC, &g_inter_l4x4_dsc[qp], qp, 0);	
	
		/*setup c_4x4_dsc huffman table*/
		g_inter_c4x4_dsc[qp].code_tab_ptr = (uint32 *)malloc(sizeof(uint32) * MAX_4x4_DSC);
		
		start_pos = 1;
		SetupHuffTab (DSC_4X4, MAX_4x4_DSC, start_pos, inter_chroma_4x4_dsc_len[qp], &g_inter_c4x4_dsc[qp]);
		
		SetupVldContext (DSC_4X4, MAX_4x4_DSC, &g_inter_c4x4_dsc[qp], qp, 0);
		
		/*setup 8x8_dsc huffman table*/
		for (i = 0; i < 4; i++)
		{
			g_inter_8x8_dsc[qp][i].code_tab_ptr = (uint32 *)malloc (sizeof(uint32) * MAX_DSC8X8_NUM);
			memset (g_inter_8x8_dsc[qp][i].code_tab_ptr, 0, sizeof(uint32)*MAX_DSC8X8_NUM);
			
			start_pos = 1;
			SetupHuffTab (DSC_8X8, MAX_8x8_DSC, start_pos, inter_8x8_dsc_len[qp][i], &g_inter_8x8_dsc[qp][i]);
			
			SetupVldContext (DSC_8X8, MAX_DSC8X8_NUM, &g_inter_8x8_dsc[qp][i], qp, 0);
		}		
		
		/*setup l_2x2_dsc huffman table*/
		for (i = 0; i < 2; i++)
		{
			g_inter_l2x2_dsc[qp][i].code_tab_ptr = (uint32 *)malloc(sizeof(uint32) * MAX_2x2_DSC);
			
			start_pos = 1;
			SetupHuffTab (DSC_2X2, MAX_2x2_DSC, start_pos, inter_luma_2x2_dsc_len[qp][i], &g_inter_l2x2_dsc[qp][i]);
			
			SetupVldContext (DSC_2X2, MAX_2x2_DSC, &g_inter_l2x2_dsc[qp][i], qp, 0);
		}
	
		/*setup c_2x2_dsc huffman table*/
		for (i = 0; i < 2; i++)
		{
			g_inter_c2x2_dsc[qp][i].code_tab_ptr = (uint32 *)malloc(sizeof(uint32) * MAX_2x2_DSC);
			
			start_pos = 1;
			SetupHuffTab (DSC_2X2, MAX_2x2_DSC, start_pos, inter_chroma_2x2_dsc_len[qp][i], &g_inter_c2x2_dsc[qp][i]);	

			SetupVldContext (DSC_2X2, MAX_2x2_DSC, &g_inter_c2x2_dsc[qp][i], qp, 0);
		}
		
		/*setup lev_dsc huffman table*/
		g_inter_lev_dsc[qp].code_tab_ptr = (uint32 *)malloc(sizeof(uint32) * MAX_DSCLEV_NUM);
		memset (g_inter_lev_dsc[qp].code_tab_ptr, 0, sizeof(uint32)*MAX_DSCLEV_NUM);
		
		start_pos = 0;
		SetupHuffTab (DSC_LEV, MAX_LEVEL_DSC, start_pos, inter_level_dsc_len[qp], &g_inter_lev_dsc[qp]);
		
		SetupVldContext (DSC_LEV, MAX_DSCLEV_NUM, &g_inter_lev_dsc[qp], qp, 0);
		
		g_code_pos = 0;
		g_max_base_pos = 0;
	}

//	PrintfHuffTabSdram ();
#if 0
{
	uint32 qp_idx, i;
	uint32 *tab_ptr;

	FILE *huff_tbl_fp	= fopen ("..\\..\\test_vectors\\vld\\huff_tab.txt", "w");

	fprintf(huff_tbl_fp, "const uint32 g_rvld_intra_code[MAX_INTRA_QP_REGIONS][4*1024] = \n{\n");
	for (qp_idx = 0; qp_idx < MAX_INTRA_QP_REGIONS; qp_idx++)
	{
		tab_ptr = g_rvld_intra_code[qp_idx];

		fprintf(huff_tbl_fp, "\t{\n\t\t");
		for (i = 0; i < 4*1024; i++)
		{
			fprintf (huff_tbl_fp, "0x%08x,\t", tab_ptr[i]);

			if ((i+1)%8 == 0)
			{
				fprintf(huff_tbl_fp, "\n\t\t");
			}
		}
		fprintf(huff_tbl_fp, "\n\t},\n");
	}
	fprintf(huff_tbl_fp, "};\n");


	fprintf(huff_tbl_fp, "const uint32 g_rvld_intra_max_base[MAX_INTRA_QP_REGIONS][1*1024] = \n{\n");
	for (qp_idx = 0; qp_idx < MAX_INTRA_QP_REGIONS; qp_idx++)
	{
		tab_ptr = g_rvld_intra_max_base[qp_idx];

		fprintf(huff_tbl_fp, "\t{\n\t\t");
		for (i = 0; i < 1*1024; i++)
		{
			fprintf (huff_tbl_fp, "0x%08x,\t", tab_ptr[i]);
			if ((i+1)%8 == 0)
			{
				fprintf(huff_tbl_fp, "\n\t\t");
			}
		}	
		fprintf(huff_tbl_fp, "\n\t},\n");
	}
	fprintf(huff_tbl_fp, "};\n");

	fprintf(huff_tbl_fp, "const uint32 g_rvld_inter_code[MAX_INTER_QP_REGIONS][4*1024] = \n{\n");
	for (qp_idx = 0; qp_idx < MAX_INTER_QP_REGIONS; qp_idx++)
	{
		tab_ptr = g_rvld_inter_code[qp_idx];

		fprintf(huff_tbl_fp, "\t{\n\t\t");
		for (i = 0; i < 4*1024; i++)
		{
			fprintf (huff_tbl_fp, "0x%08x,\t", tab_ptr[i]);
			if ((i+1)%8 == 0)
			{
				fprintf(huff_tbl_fp, "\n\t\t");
			}
		}	
		fprintf(huff_tbl_fp, "\n\t},\n");
	}
	fprintf(huff_tbl_fp, "};\n");

	fprintf(huff_tbl_fp, "const uint32 g_rvld_inter_max_base[MAX_INTER_QP_REGIONS][INTER_MAX_BASE_SIZE] = \n{\n");
	for (qp_idx = 0; qp_idx < MAX_INTER_QP_REGIONS; qp_idx++)
	{
		tab_ptr = g_rvld_inter_max_base[qp_idx];
		fprintf(huff_tbl_fp, "\t{\n\t\t");
		
		for (i = 0; i < 1*1024; i++)
		{
			fprintf (huff_tbl_fp, "0x%08x,\t", tab_ptr[i]);
			if ((i+1)%8 == 0)
			{
				fprintf(huff_tbl_fp, "\n\t\t");
			}
		}
		fprintf(huff_tbl_fp, "\n\t},\n");
	}
	fprintf(huff_tbl_fp, "};\n");
	
	fclose(huff_tbl_fp);

}
#endif
}

#endif

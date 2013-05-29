/******************************************************************************
 ** File Name:    vlc_mpeg4_table.c											  *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         12/14/2006                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "sc6800x_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

typedef struct mpeg4_vlc_table
{
	int codeword;
	int last;
	int run;
	int level;
	int length;
} MPEG4_VLC_TABLE;

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/

MPEG4_VLC_TABLE intra_vlc_l0r0 [28];		int numIntra_l0r0 = 0;
MPEG4_VLC_TABLE intra_vlc_l1r0 [9];			int numIntra_l1r0 = 0;
MPEG4_VLC_TABLE intra_vlc_l0l1 [15];		int numIntra_l0l1 = 0;
MPEG4_VLC_TABLE intra_vlc_l1l1 [21];		int numIntra_l1l1 = 0;
MPEG4_VLC_TABLE intra_vlc_l0l2 [10];		int numIntra_l0l2 = 0;
MPEG4_VLC_TABLE intra_vlc_l1l2 [7];			int numIntra_l1l2 = 0;
MPEG4_VLC_TABLE intra_vlc_l0l3 [8];			int numIntra_l0l3 = 0;
MPEG4_VLC_TABLE intra_vlc_l0r1 [11];		int numIntra_l0r1 = 0;
MPEG4_VLC_TABLE intra_vlc_l0r2 [6];			int numIntra_l0r2 = 0;
MPEG4_VLC_TABLE intra_vlc_l0r3 [5];			int numIntra_l0r3 = 0;
MPEG4_VLC_TABLE intra_vlc_l1l3 [2];			int numIntra_l1l3 = 0;

MPEG4_VLC_TABLE  inter_vlc_l0r0 [13];		int numInter_l0r0 = 0;
MPEG4_VLC_TABLE  inter_vlc_l0l1 [27];		int numInter_l0l1 = 0;
MPEG4_VLC_TABLE  inter_vlc_l1l1 [41];		int numInter_l1l1 = 0;
MPEG4_VLC_TABLE  inter_vlc_l0r1 [7];		int numInter_l0r1 = 0; 
MPEG4_VLC_TABLE  inter_vlc_l0l2 [11];		int numInter_l0l2 = 0;
MPEG4_VLC_TABLE  inter_vlc_l0l3 [7];		int numInter_l0l3 = 0;
MPEG4_VLC_TABLE  inter_vlc_l0r2 [5];		int numInter_l0r2 = 0;
MPEG4_VLC_TABLE  inter_vlc_l1r0 [4];		int numInter_l1r0 = 0;
MPEG4_VLC_TABLE  inter_vlc_l1r1 [3];		int numInter_l1r1 = 0;

uint32 intra_huff_tab[128];
uint32 inter_huff_tab[128];
uint32 g_huff_tab[128];

/*****************intra MB********************/
/*for prefix "1"*/
MPEG4_VLC_TABLE intraVlcTab_pre0 [4] =
{
	{10,		0,	0,	1,	3},
	{110,		0,	0,	2,	4},
	{1110,		0,	1,	1,	5},
	{1111,		0,	0,	3,	5},		
};


/*for prefix "01"*/
MPEG4_VLC_TABLE intraVlcTab_pre1 [10] =
{
	{10000,	0,	4,	1,	7},
	{10001,	0,	3,	1,	7},
	{10010,	0,	0,	8,	7},
	{10011,	0,	0,	7,	7},
	{10100,	0,	1,	2,	7},
	{10101,	0,	0,	6,	7},
	{1011,		0,	2,	1,	6},
	{1100,		0,	0,	5,	6},
	{1101,		0,	0,	4,	6},
	{111,		1,	0,	1,	5},
		
};


/*for prefix "001"*/
MPEG4_VLC_TABLE intraVlcTab_pre2 [12] =
{
	{10000,	1,	4,	1,	8},/**/
	{10001,	1,	3,	1,	8},/**/
	{10010,	0,	6,	1,	8},/**/
	{10011,	1,	5,	1,	8},/**/
	{10100,	0,	7,	1,	8},/**/
	{10101,	0,	2,	2,	8},/**/
	{10110,	0,	1,	3,	8},/**/
	{10111,	0,	0,	9,	8},/**/
	{1100,	1,	0,	2,	7},/**/
	{1101,	0,	5,	1,	7},/**/
	{1110,	1,	2,	1,	7},/**/
	{1111,	1,	1,	1,	7},/**/
		

};


/*for prefix "0001"*/
MPEG4_VLC_TABLE intraVlcTab_pre3 [19] =
{
	{100000,	0,	1,	5,	10},
	{100001,	0,	0,	16,	10},
	{100010,	0,	4,	2,	10},/**/
	{100011,	0,	0,	15,	10},/**/
	{100100,	0,	0,	14,	10},/**/
	{100101,	0,	0,	13,	10},/**/
	{10011,	1,	8,	1,	9},
	{10100,	1,	7,	1,	9},
	{10101,	1,	6,	1,	9},
	{10110,	1,	0,	3,	9},/**/
	{10111,	0,	10,	1,	9},/**/
	{11000,	0,	9,	1,	9},/**/
	{11001,	0,	8,	1,	9},/**/
	{11010,	1,	9,	1,	9},/**/
	{11011,	0,	3,	2,	9},
	{11100,	0,	1,	4,	9},
	{11101,	0,	0,	12,	9},/**/
	{11110,	0,	0,	11,	9},/**/
	{11111,	0,	0,	10,	9},/**/
};


/*for prefix "0000_1"*/
MPEG4_VLC_TABLE intraVlcTab_pre4 [17] =
{
	{100000,0,	0,	18,	11},
	{100001,0,	0,	17,	11},
	{10001,	1,	14,	1,	10},
	{10010,	1,	13,	1,	10},
	{10011,	1,	12,	1,	10},
	{10100,	1,	11,	1,	10},
	{10101,	1,	10,	1,	10},
	{10110,	1,	1,	2,	10},
	{10111,	1,	0,	4,	10},
	{11000,	0,	12,	1,	10},
	{11001,	0,	11,	1,	10},
	{11010,	0,	7,	2,	10},
	{11011,	0,	6,	2,	10},
	{11100,	0,	5,	2,	10},
	{11101,	0,	3,	3,	10},
	{11110,	0,	2,	3,	10},
	{11111,	0,	1,	6,	10},

};


/*for prefix "0000_01"*/
MPEG4_VLC_TABLE intraVlcTab_pre5 [24] =
{
	{100000,	0,	  0,	  23,	  12},
	{100001,	0,	  0,	  24,	  12},
	{100010,	0,	  1,	  8,	  12},
	{100011,	0,	  9,	  2,	  12},
	{100100,	1,	  3,	  2,	  12},
	{100101,	1,	  4,	  2,	  12},
	{100110,	1,   15,	  1,	  12},
	{100111,	1,	 16,	  1,	  12},
	{1010000,  0,	  0, 	  25,	  13},
	{1010001,  0 ,   0,      26,      13},
	{1010010,  0,    0,      27,      13},
	{1010011,  0,    1,      9 ,      13},
	{1010100,  0,    6,      3 ,      13},
	{1010101,  0,    1,      10,      13},
	{1010110,  0,    2,      5,       13},
	{1010111,  0,    7,      3,       13},
	{1011000,  0,   14,      1,       13},
	{1011001,  1,    0,       8,      13},
	{1011010,  1,    5,      2,       13},
	{1011011,  1,    6,      2,       13},
	{1011100,  1,   17,      1,       13},
	{1011101,  1,   18,      1,       13},
	{1011110,  1,   19,      1,       13},
	{1011111,  1,   20,      1,       13},
};

/*for prefix "0000_001"*/
MPEG4_VLC_TABLE intraVlcTab_pre6 [8] =
{
	{1000,	0,	     5,	      3,  	   11},
	{1001,    0,       8,       2,       11},
	{1010,    0,       4,       3,       11},
	{1011,    0,       3,       4,       11},
	{1100,    0,       2,       4,       11},
	{1101,    0,       1,       7,       11},
	{1110,    0,       0,       20,      11},
	{1111,    0,       0,       19,      11},
};

/*for prefix "0000_0001"*/
MPEG4_VLC_TABLE intraVlcTab_pre7 [4] =
{
	{100,	1, 	     2,    	  2,       11},
	{101,    1,       1,       3,       11},
	{110,    1,       0,       5,       11},
	{111,    0,       13,      1,       11},
	
};

/*for prefix "0000_00001"*/
MPEG4_VLC_TABLE intraVlcTab_pre8 [4] =
{
	{100,	1,	     0,    	  7,       12},
	{101,   1,       0,       6,       12},
	{110,   0,       0,       22,      12},
	{111,   0,       0,       21,      12},
};






/*****************inter MB********************/
/*for prefix "1"*/
MPEG4_VLC_TABLE interVlcTab_pre0 [4] = 
{
	{10	,	0,	0,	     1,	      3},
	{110,   0,  1,       1,       4},
	{1110,  0,  2,       1,       5},
	{1111,  0,  0,       2,       5},
};



/*for prefix "01"*/
MPEG4_VLC_TABLE interVlcTab_pre1 [10] = 
{
	{10000,  0,  9,  1, 7},
	{10001,  0,  8,  1, 7},
	{10010,  0,  7,  1, 7},
	{10011,  0,  6,  1, 7},
	{10100,  0,  1,  2, 7},
	{10101,  0,  0,  3, 7},
	{1011 ,  0,  5,  1, 6},
	{1100 ,  0,  4,  1, 6},
	{1101 ,  0,  3,  1, 6},
	{111  ,  1,  0,  1, 5},
};



/*for prefix "001"*/
MPEG4_VLC_TABLE interVlcTab_pre2 [12] =
{
	{10000,  1,  8,  1,  8},
	{10001,  1,  7,  1,  8},
	{10010,  1,  6,  1,  8},
	{10011,  1,  5,  1,  8},
	{10100,  0,  12, 1,  8},
	{10101,  0,  11, 1,  8},
	{10110,  0,  10, 1,  8},
	{10111,  0,  0,  4,  8},
	{1100 ,  1,  4,  1,  7},
	{1101 ,  1,  3,  1,  7},
	{1110 ,  1,  2,  1,  7},
	{1111 ,  1,  1,  1,  7},
};



/*for prefix "0001"*/
MPEG4_VLC_TABLE interVlcTab_pre3 [19] =
{
	{100000,	     0,       16,	   1, 	    10},
	{100001,      0,       15,      1,       10},
	{100010,      0,       4 ,      2,       10},
	{100011,      0,       3 ,      2,       10},
	{100100,      0,       0 ,      7,       10},
	{100101,      0,       0 ,      6,       10},
	{10011  ,     1,       16,      1,       9 },
	{10100  ,     1,       15,      1,       9 },
	{10101  ,     1,       14,      1,       9 },
	{10110  ,     1,       13,      1,       9 },
	{10111  ,     1,       12,      1,       9 },
	{11000  ,     1,       11,      1,       9 },
	{11001  ,     1,       10,      1,       9 },
	{11010  ,     1,       9 ,      1,       9 },
	{11011  ,     0,       14,      1,       9 },
	{11100  ,     0,       13,      1,       9 },
	{11101  ,     0,       2 ,      2,       9 },
	{11110  ,     0,       1 ,      3,       9 },
	{11111  ,     0,       0 ,      5,       9 },
};




/*for prefix "0000_1"*/
MPEG4_VLC_TABLE interVlcTab_pre4 [17] = 
{
	{100000,	0,	 0,	 9,  11},
	{100001,	0,	 0,	 8,  11},
	{10001,		1,	 24, 1,  10},
	{10010,     1,  23,  1,  10},
	{10011,     1,  22,  1,  10},
	{10100,     1,  21,  1,  10},
	{10101,     1,  20,  1,  10},
	{10110,     1,  19,  1,  10},
	{10111,     1,  18,  1,  10},
	{11000,     1,  17,  1,  10},
	{11001,     1,  0 ,  2,  10},
	{11010,     0,  22,  1,  10},
	{11011,     0,  21,  1,  10},
	{11100,     0,  20,  1,  10},
	{11101,     0,  19,  1,  10},
	{11110,     0,  18,  1,  10},
	{11111,     0,  17,  1,  10},
};


/*for prefix "0000_01"*/
MPEG4_VLC_TABLE interVlcTab_pre5 [24] =  
{
	{100000,  0,    0,  12,  12},
	{100001,  0,    1 ,  5,  12},
	{100010,  0,    23,  1,  12},
	{100011,  0,    24,  1,  12},
	{100100,  1,    29,  1,  12},
	{100101,  1,    30,  1,  12},
	{100110,  1,    31,  1,  12},
	{100111,  1,    32,  1,  12},
	{1010000,  0,    1 ,  6,  13},
	{1010001,  0,    2 ,  4,  13},
	{1010010,  0,    4 ,  3,  13},
	{1010011,  0,    5 ,  3,  13},
	{1010100,  0,    6 ,  3,  13},
	{1010101,  0,    10,  2,  13},
	{1010110,  0,    25,  1,  13},
	{1010111,  0,    26,  1,  13},
	{1011000,  1,    33,  1,  13},
	{1011001,  1,    34,  1,  13},
	{1011010,  1,    35,  1,  13},
	{1011011,  1,    36,  1,  13},
	{1011100,  1,    37,  1,  13},
	{1011101,  1,    38,  1,  13},
	{1011110,  1,    39,  1,  13},
	{1011111,  1,    40,  1,  13},

};

/*for prefix "0000_001"*/
MPEG4_VLC_TABLE interVlcTab_pre6 [8] = 
{
	{1000,  0,  9,  2,  11},
	{1001,  0,  8,  2,  11},
	{1010,  0,  7,  2,  11},
	{1011,  0,  6,  2,  11},
	{1100,  0,  5,  2,  11},
	{1101,  0,  3,  3,  11},
	{1110,  0,  2,  3,  11},
	{1111,  0,  1,  4,  11},
};


/*for prefix "0000_0001"*/
MPEG4_VLC_TABLE interVlcTab_pre7 [4] =
{
	{100,  1,  28, 1,  11},
	{101,  1,  27, 1,  11},
	{110,  1,  26, 1,  11},
	{111,  1,  25, 1,  11},
};

/*for prefix "0000_00001"*/
MPEG4_VLC_TABLE interVlcTab_pre8 [4] =
{
	{100, 1,  1,  2,  12},
	{101, 1,  0,  3,  12},
	{110, 0,  0, 11,  12},
	{111, 0,  0, 10,  12},
};


int bin_2_hex (int code, int len)
{
	int i, j;
	int val;
	int codeVal = 0;
	int k;
	
	for (i = len-1; i >= 0; i--)
	{
		val = 1;
		for (j = 0; j < i; j++)
			val = val * 10;
		
		if (code >= val)
		{
			k = 1;
			code = code - val;
		}
		else
		{
			k = 0;
		}
		
		codeVal = codeVal | (k << i);
	}
	
	return codeVal;
}

void buildHuffIntraVlcTab (MPEG4_VLC_TABLE *pHuffTab, int numEle)
{
	int i;
	int codeVal;	
	int last, run, level, len;
	uint32 * ptmp = intra_huff_tab;	
	MPEG4_VLC_TABLE * pTab = pHuffTab;

	for (i = 0; i < numEle; i++)
	{
		last  = pTab->last;
		run   = pTab->run;
		level = pTab->level;
		len   = pTab->length;
		codeVal = bin_2_hex (pTab->codeword, len - 1);

		if ((last == 0) && (run == 0))
		{
			(ptmp+VLC_INTRA_OFFSET_L0R0) [level] = (codeVal << 4) | len;		
			
			intra_vlc_l0r0[numIntra_l0r0] = pTab [0];
			intra_vlc_l0r0[numIntra_l0r0].codeword = codeVal;
			numIntra_l0r0++;
		}

		if ((last == 1) && (run == 0))
		{
			(ptmp+VLC_INTRA_OFFSET_L1R0) [level] = (codeVal << 4) | len;	
			
			intra_vlc_l1r0[numIntra_l1r0] = pTab [0];
			intra_vlc_l1r0[numIntra_l1r0].codeword = codeVal;
			numIntra_l1r0++;
		}

		if ((last == 0) && (level == 1))
		{
			(ptmp+VLC_INTRA_OFFSET_L0L1) [run] = (codeVal << 4) | len;
			intra_vlc_l0l1 [numIntra_l0l1] = pTab[0];
			intra_vlc_l0l1 [numIntra_l0l1].codeword = codeVal;
	
			numIntra_l0l1++;
		}

		if ((last == 1) && (level == 1))
		{
			(ptmp+VLC_INTRA_OFFSET_L1L1) [run] = (codeVal << 4) | len;

			intra_vlc_l1l1 [numIntra_l1l1] = pTab[0];
			intra_vlc_l1l1 [numIntra_l1l1].codeword = codeVal;
			
			numIntra_l1l1++;
		}

		if ((last == 0) && (level == 2))
		{
			(ptmp+VLC_INTRA_OFFSET_L0L2) [run] = (codeVal << 4) | len;
			intra_vlc_l0l2 [numIntra_l0l2] = pTab[0];
			intra_vlc_l0l2 [numIntra_l0l2].codeword = codeVal;
			
			numIntra_l0l2++;
		}

		if ((last == 1) && (level == 2))
		{
			(ptmp+VLC_INTRA_OFFSET_L1L2) [run] = (codeVal << 4) | len;
			intra_vlc_l1l2 [numIntra_l1l2] = pTab[0];
			intra_vlc_l1l2 [numIntra_l1l2].codeword = codeVal;
			
			numIntra_l1l2++;
		}

		if ((last == 0) && (level == 3))
		{
			(ptmp+VLC_INTRA_OFFSET_L0L3) [run] = (codeVal << 4) | len;
			intra_vlc_l0l3 [numIntra_l0l3] = pTab[0];
			intra_vlc_l0l3 [numIntra_l0l3].codeword = codeVal;
			
			numIntra_l0l3++;
		}

		if ((last == 0) && (run == 1))
		{
			(ptmp+VLC_INTRA_OFFSET_L0R1) [level] = (codeVal << 4) | len;
			intra_vlc_l0r1 [numIntra_l0r1] = pTab[0];
			intra_vlc_l0r1 [numIntra_l0r1].codeword = codeVal;
			
			numIntra_l0r1++;
		}

		if ((last == 0) && (run == 2))
		{
			(ptmp+VLC_INTRA_OFFSET_L0R2) [level] = (codeVal << 4) | len;
			intra_vlc_l0r2 [numIntra_l0r2] = pTab[0];
			intra_vlc_l0r2 [numIntra_l0r2].codeword = codeVal;
			
			numIntra_l0r2++;
		}

		if ((last == 0) && (run == 3))
		{
			(ptmp+VLC_INTRA_OFFSET_L0R3) [level] = (codeVal << 4) | len;
			intra_vlc_l0r3 [numIntra_l0r3] = pTab[0];
			intra_vlc_l0r3 [numIntra_l0r3].codeword = codeVal;
			
			numIntra_l0r3++;
		}

		if ((last == 1) && (level == 3))
		{
			(ptmp+VLC_INTRA_OFFSET_L1L3) [run] = (codeVal << 4) | len;
			intra_vlc_l1l3 [numIntra_l1l3] = pTab[0];
			intra_vlc_l1l3[numIntra_l1l3].codeword = codeVal;
			
			numIntra_l1l3++;
		}
	

		pTab++;
	}
	
}

void buildHuffInterVlcTab (MPEG4_VLC_TABLE *pHuffTab, int numEle)
{
	int i;
	int codeVal;
	int last, run, level, len;
	uint32 * ptmp = inter_huff_tab;
	MPEG4_VLC_TABLE * pTab = pHuffTab;
	
	for (i = 0; i < numEle; i++)
	{
		last  = pTab->last;
		run   = pTab->run;
		level = pTab->level;
		len   = pTab->length;
		codeVal = bin_2_hex (pTab->codeword, len - 1);

		if ((last == 0) && (run == 0))
		{
			(ptmp+VLC_INTER_OFFSET_L0R0) [level] = (codeVal << 4) | len;	
			
			inter_vlc_l0r0[numInter_l0r0] = pTab [0];
			inter_vlc_l0r0[numInter_l0r0].codeword = codeVal;
			numInter_l0r0++;
		}

		if ((last == 0) && (level == 1))
		{
			(ptmp+VLC_INTER_OFFSET_L0L1) [run] = (codeVal << 4) | len;

			inter_vlc_l0l1[numInter_l0l1] = pTab [0];
			inter_vlc_l0l1[numInter_l0l1].codeword = codeVal;
			numInter_l0l1++;
		}

		if ((last == 1) && (level == 1))
		{
			(ptmp+VLC_INTER_OFFSET_L1L1) [run] = (codeVal << 4) | len;

			inter_vlc_l1l1[numInter_l1l1] = pTab [0];
			inter_vlc_l1l1[numInter_l1l1].codeword = codeVal;
			numInter_l1l1++;
		}

		if ((last == 0) && (run == 1))
		{
			(ptmp+VLC_INTER_OFFSET_L0R1) [level] = (codeVal << 4) | len;
			
			inter_vlc_l0r1[numInter_l0r1] = pTab [0];
			inter_vlc_l0r1[numInter_l0r1].codeword = codeVal;
			numInter_l0r1++;
		}

		if ((last == 0) && (level == 2))
		{
			(ptmp+VLC_INTER_OFFSET_L0L2) [run] = (codeVal << 4) | len;

			inter_vlc_l0l2[numInter_l0l2] = pTab [0];
			inter_vlc_l0l2[numInter_l0l2].codeword = codeVal;
			numInter_l0l2++;
		}

		if ((last == 0) && (level == 3))
		{
			(ptmp+VLC_INTER_OFFSET_L0L3) [run] = (codeVal << 4) | len;

			inter_vlc_l0l3[numInter_l0l3] = pTab [0];
			inter_vlc_l0l3[numInter_l0l3].codeword = codeVal;
			numInter_l0l3++;			
			
		}

		if ((last == 0) && (run == 2))
		{
			(ptmp+VLC_INTER_OFFSET_L0R2) [level] = (codeVal << 4) | len;	
			
			inter_vlc_l0r2[numInter_l0r2] = pTab [0];
			inter_vlc_l0r2[numInter_l0r2].codeword = codeVal;
			numInter_l0r2++;
		}

		if ((last == 1) && (run == 0))
		{
			(ptmp+VLC_INTER_OFFSET_L1R0) [level] = (codeVal << 4) | len;		

			inter_vlc_l1r0[numInter_l1r0] = pTab [0];
			inter_vlc_l1r0[numInter_l1r0].codeword = codeVal;
			numInter_l1r0++;
		}

		if ((last == 1) && (run == 1))
		{
			(ptmp+VLC_INTER_OFFSET_L1R1) [level] = (codeVal << 4) | len;
			
			inter_vlc_l1r1[numInter_l1r1] = pTab [0];
			inter_vlc_l1r1[numInter_l1r1].codeword = codeVal;
			numInter_l1r1++;
		}		

		pTab++;

	}

}

void LeftAlignCodeword (uint32 * huff_tab_ptr, int ele_num)
{
	int i;
	uint32 val;
	uint32 length;
	uint32 codeword;
	uint32 left_algin_code;

	for (i = 0; i < ele_num; i++)
	{
		val = huff_tab_ptr[i];
		length = val & 0xf;
		codeword = val >> 4;
		left_algin_code = codeword << (16 - (length-1));
		val = left_algin_code | length;
		huff_tab_ptr[i] = val;
	}
}

void ConstructVlcTab ()
{
	int i;
	uint32 * pintra_tab = intra_huff_tab;
	uint32 * pinter_tab = inter_huff_tab;
	uint32 * phuff_tab = g_huff_tab;

	for (i = 0; i < 128; i++)
	{
		*phuff_tab++ = (*pintra_tab++ << 16) | *pinter_tab++;
	}

	for (i = 0; i < 128; i++)
	{
		vsp_huff_dcac_tab[i] = g_huff_tab[i];
	}

//	PrintfHuffTab ();

//	memcpy ((uint8 *)vsp_dct_io_1, (uint8 *)g_huff_tab, 128*sizeof(uint32));
}

/*construct huffman coding table*/
void GenMPEG4VLCTable ()
{
	buildHuffIntraVlcTab (intraVlcTab_pre0, 4);
	buildHuffIntraVlcTab (intraVlcTab_pre1, 10);
	buildHuffIntraVlcTab (intraVlcTab_pre2, 12);
	buildHuffIntraVlcTab (intraVlcTab_pre3, 19);
	buildHuffIntraVlcTab (intraVlcTab_pre4, 17);
	buildHuffIntraVlcTab (intraVlcTab_pre5, 24);
	buildHuffIntraVlcTab (intraVlcTab_pre6, 8);
	buildHuffIntraVlcTab (intraVlcTab_pre7, 4);
	buildHuffIntraVlcTab (intraVlcTab_pre8, 4);
	
	buildHuffInterVlcTab (interVlcTab_pre0, 4);
	buildHuffInterVlcTab (interVlcTab_pre1, 10);
	buildHuffInterVlcTab (interVlcTab_pre2, 12);
	buildHuffInterVlcTab (interVlcTab_pre3, 19);
	buildHuffInterVlcTab (interVlcTab_pre4, 17);
	buildHuffInterVlcTab (interVlcTab_pre5, 24);
	buildHuffInterVlcTab (interVlcTab_pre6, 8);
	buildHuffInterVlcTab (interVlcTab_pre7, 4);
	buildHuffInterVlcTab (interVlcTab_pre8, 4);

	LeftAlignCodeword (intra_huff_tab, 128);
	LeftAlignCodeword (inter_huff_tab, 118);

	ConstructVlcTab ();	
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 










































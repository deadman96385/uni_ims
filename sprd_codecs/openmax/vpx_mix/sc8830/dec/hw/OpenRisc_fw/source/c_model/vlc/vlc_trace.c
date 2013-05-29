/******************************************************************************
 ** File Name:	  vlc_trace.c                                                 *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         11/19/2008                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:   c model of bsmr module in mpeg4 decoder                    *
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
#include "sc8810_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

FILE	*	g_dc_enc_fp;//dc pred direction, dc prediction value, and dc_size_huff_code and len
FILE	*	g_rlc_fp;//run, coefficient, last
FILE	*	g_vlc_event_fp;//escape mode, lmax, rmax, addr_lut, codeword, and len
FILE	*	g_huff_tab_fp;


void PrintDCPred (int dir_pred, uint16 dc_pred)
{
if(g_trace_enable_flag&TRACE_ENABLE_VLC)
{
	VLC_FPRINTF (g_dc_enc_fp, "%08x,%08x,", dir_pred, dc_pred);
}
}

void PrintDCEnc (int dc_size_huf_code, int dc_size_huf_len)
{
if(g_trace_enable_flag&TRACE_ENABLE_VLC)
{
	VLC_FPRINTF (g_dc_enc_fp, "%08x,%08x\n", dc_size_huf_code, dc_size_huf_len);
}
}

void PrintRlcInf (int run, int level, int last)
{
if(g_trace_enable_flag&TRACE_ENABLE_VLC)
{
	VLC_FPRINTF (g_rlc_fp, "%08x,%08x,%08x\n", run, level, last);
}
}

void PrintVlcEvent (int escape_mode, int lmax, int rmax, int vlc_tbuf_addr)
{
if(g_trace_enable_flag&TRACE_ENABLE_VLC)
{
	if (escape_mode == 3)
	{
		lmax = 0;
		rmax = 0;
		vlc_tbuf_addr = 0;
	}

	if (escape_mode == 0)
	{
		lmax = 0;
		rmax = 0;
	}

	if (escape_mode == 1)
		rmax = 0;

	if (escape_mode == 2)
		lmax = 0;

	VLC_FPRINTF (g_vlc_event_fp, "%08x,%08x,%08x,%08x\n", escape_mode, lmax, rmax, vlc_tbuf_addr);
}
}

int32 vlc_out_cnt = 0;
void PrintfBsmOut (uint32 val, int nbits)
{
if(g_trace_enable_flag&TRACE_ENABLE_VLC)
{
	uint32 val_la;   //left align in one word

	static int bsm_out_cnt = 0;

	if (bsm_out_cnt == 30205)
		printf ("");

	val_la = val << (32 - nbits);

//	if (g_vlc_status)
	{
		vlc_out_cnt++;
		if (vlc_out_cnt == 182)
		{
			vlc_out_cnt = 182;
		}

	//	VLC_FPRINTF (g_fp_vlc_tv, "%08x,%08x\n", val_la, nbits);
		VLC_FPRINTF (g_fp_vlc_tv, "%08x\n", val_la);
		
	}

	bsm_out_cnt++;
}
}

void PrintfHuffTab ()
{
if(g_trace_enable_flag&TRACE_ENABLE_VLC)
{
	int i;
	uint32 val;

	for (i = 0; i < 128; i++)
	{
		val = vsp_huff_dcac_tab[i];

		fprintf (g_huff_tab_fp, "%08x\n", val);
	}
}
}

void Mp4Enc_TestVecInit ()
{
if(g_trace_enable_flag&TRACE_ENABLE_VLC)
{
	g_dc_enc_fp = fopen ("..\\..\\trace\\dc_enc.txt", "w");
	assert (g_dc_enc_fp != NULL);

	g_rlc_fp = fopen ("..\\..\\trace\\run_lev.txt", "w");
	assert (g_rlc_fp != NULL);

	g_vlc_event_fp = fopen ("..\\..\\trace\\vlc_event.txt", "w");
	assert (g_vlc_event_fp != NULL);

	g_huff_tab_fp = fopen ("..\\..\\trace\\huff_tab.txt", "w");
	assert (g_huff_tab_fp != NULL);
}
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
